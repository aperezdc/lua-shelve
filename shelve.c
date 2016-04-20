/*
 * Lua interface of the 'shelve' module.
 * Copyright (C) 2003-2016 Adrian Perez de Castro <aperez@igalia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include "shelve.h"
#include "anydb.h"

#ifndef LUALIB_API
# define LUALIB_API API
#endif

#if LUA_VERSION_NUM < 502
static void
luaL_setmetatable(lua_State *L, const char *name)
{
    lua_pushstring(L, name);
    lua_gettable(L, LUA_REGISTRYINDEX);
    lua_setmetatable(L, -2);
    lua_remove(L, -2);
}
#endif


/* Function protos */
static int l_shelve_index(lua_State*);
static int l_shelve_nindex(lua_State*);
static int l_shelve_trv(lua_State*);
static int l_shelve_gc(lua_State*);
static int l_shelve_open(lua_State*);
static int l_shelve_tostring(lua_State*);
static int l_shelve_iter_gc(lua_State*);

/* Lua userdata type */
typedef struct shelve_file_t {
    anydb_t dbf;
    int rdonly;
} shelve_file;

/* Metatable items */


static luaL_Reg meta[] =
{
    { "__index",    l_shelve_index    },
    { "__newindex", l_shelve_nindex   },
    { "__call",     l_shelve_trv      },
    { "__gc",       l_shelve_gc       },
    { "__tostring", l_shelve_tostring },
    { NULL,         NULL              },
};

static luaL_Reg iter_meta[] =
{
    { "__gc", l_shelve_iter_gc },
    { NULL,   NULL             },
};

static luaL_Reg lib[] =
{
    { "open",      l_shelve_open      },
    { "marshal",   l_shelve_marshal   },
    { "unmarshal", l_shelve_unmarshal },
    { NULL,        NULL               },
};


LUALIB_API int
luaopen_shelve(lua_State *L)
{
    assert(L);

    /* Shelve file metatable */
    luaL_newmetatable(L, SHELVE_REGISTRY_KEY);
#if LUA_VERSION_NUM < 502
    luaL_register(L, NULL, meta);
#else
    luaL_setfuncs(L, meta, 0);
#endif

    /* Shelve file iterator metatable */
    luaL_newmetatable(L, SHELVE_ITER_META);
#if LUA_VERSION_NUM < 502
    luaL_register(L, NULL, iter_meta);
#else
    luaL_setfuncs(L, iter_meta, 0);
#endif

    /* Module */
#if LUA_VERSION_NUM < 502
    luaL_register(L, "shelve", lib);
#else
    luaL_newlib(L, lib);
#endif

    return 1;
}


int
l_shelve_open(lua_State *L)
{
    int flags = ANYDB_WRITE;
    shelve_file *udata = NULL;
    const char *rwmode = NULL;
    anydb_t dbh;
    int n = lua_gettop(L);

    /* Check arguments. */
    if ((n != 1) && (n != 2)) {
        luaL_error(L, "function takes one or two arguments");
    }

    /* Check & get second argument (if needed). */
    if (n == 2) {
        rwmode = luaL_checkstring(L, 2);
        flags  = (*rwmode == 'r') ? ANYDB_READ : ANYDB_WRITE;
    }

    /* Open the DB and remove filename from the stack. */
    if (!(dbh = anydb_open(luaL_checkstring(L, 1), flags))) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }

    /* Configure returned userdata. */
    udata = (shelve_file*) lua_newuserdata(L, sizeof(shelve_file));
    udata->dbf    = dbh;
    udata->rdonly = (flags == ANYDB_READ);

    /* Associate metatable with userdata. */
    luaL_setmetatable(L, SHELVE_REGISTRY_KEY);

    return 1;
}


int
l_shelve_index(lua_State *L)
{
    datum d, k;
    size_t slen_aux;
    shelve_file *shelf = luaL_checkudata(L, 1, SHELVE_REGISTRY_KEY);

    k.dptr  = (char*) lua_tolstring(L, 2, &slen_aux);
    k.dsize = (int) slen_aux;
    d = anydb_fetch(shelf->dbf, k);

    if (d.dptr) {
        const char *datap = d.dptr;
        if (!shelve_unmarshal(L, &datap)) {
            luaL_error(L, "bad format in encoded data");
        }
        free(d.dptr);
    } else {
        lua_pushnil(L);
    }
    return 1;
}


int
l_shelve_nindex(lua_State *L)
{
    datum k, d = { NULL, 0 };
    size_t slen_aux;
    shelve_file *shelf = luaL_checkudata(L, 1, SHELVE_REGISTRY_KEY);

    k.dptr  = (char*) lua_tolstring(L, 2, &slen_aux);
    k.dsize = (int) slen_aux;

    if (shelf->rdonly) {
        luaL_error(L, "cannot modify read-only shelf datafile");
    }

    if (lua_isnil(L, 3)) {
        /* Remove key in database. */
        anydb_delete(shelf->dbf, k);
        return 0;
    }

    if (!shelve_marshal(L, &d.dptr, &d.dsize)) {
        return luaL_error(L, "cannot encode data");
    }

    if (anydb_store(shelf->dbf, k, d, ANYDB_REPLACE) != 0) {
        free(d.dptr);
        return luaL_error(L, "cannot update item in data file");
    }
    free(d.dptr);

    return 0;
}


struct dbiter {
    anydb_t *dbh;
    datum k;
};

static int
l_shelve_trv_next(lua_State *L)
{
    datum k;
    struct dbiter *i = (struct dbiter*) lua_touserdata(L, lua_upvalueindex(1));
    if (i->k.dptr) {
        lua_pushlstring(L, i->k.dptr, (size_t) i->k.dsize);
        k = i->k;
        i->k = anydb_nextkey(*i->dbh, k);
        free(k.dptr);
        return 1;
    }
    return 0;
}

static int
l_shelve_iter_gc(lua_State *L)
{
    struct dbiter *i = (struct dbiter*) lua_touserdata(L, -1);
    free(i->k.dptr);
    i->k.dptr = NULL;
}

int
l_shelve_trv(lua_State *L)
{
    anydb_t *dbh = (anydb_t*) luaL_checkudata(L, -1, SHELVE_REGISTRY_KEY);
    struct dbiter *i = lua_newuserdata(L, sizeof(struct dbiter));
    luaL_setmetatable(L, SHELVE_ITER_META);
    i->k = anydb_firstkey(*dbh);
    i->dbh = dbh;
    lua_pushcclosure(L, l_shelve_trv_next, 1);
    return 1;
}


int
l_shelve_gc(lua_State *L)
{
    shelve_file *udata;

    assert(L);
    assert(lua_gettop(L) == 1);
    assert(lua_isuserdata(L, -1));

    udata = (shelve_file*) lua_touserdata(L, -1);
    if (!udata->rdonly) anydb_reorganize(udata->dbf);
    anydb_close(udata->dbf);
    lua_pop(L, 1);
    return 0;
}


int
l_shelve_tostring(lua_State *L)
{
    shelve_file *udata = luaL_checkudata(L, 1, SHELVE_REGISTRY_KEY);
    lua_pushfstring(L, "shelf (%p, %s)", udata,
                    (udata->rdonly) ? "ro" : "rw");
    return 1;
}

