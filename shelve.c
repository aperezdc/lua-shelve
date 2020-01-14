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


static const char SHELVE_META[]      = "shelve-file-meta";
static const char SHELVE_ITER_META[] = "shelve-file-iter-meta";


#ifndef LUALIB_API
# define LUALIB_API API
#endif

#if (LUA_VERSION_NUM < 502 && (defined(LUAJIT_VERSION_NUM) && LUAJIT_VERSION_NUM < 20100))
static void
luaL_setmetatable(lua_State *L, const char *name)
{
    lua_pushstring(L, name);
    lua_gettable(L, LUA_REGISTRYINDEX);
    lua_setmetatable(L, -2);
    lua_remove(L, -2);
}
#endif


struct Shelve {
    anydb_t db;
    int     ro;
};

struct Iterator {
    anydb_t db;
    datum k;
};


static int
iterator_next(lua_State *L)
{
    struct Iterator *i = lua_touserdata(L, lua_upvalueindex(1));
    if (i->k.dptr) {
        datum k;
        lua_pushlstring(L, i->k.dptr, (size_t) i->k.dsize);
        k = i->k;
        i->k = anydb_nextkey(i->db, k);
        free(k.dptr);
        return 1;
    }
    return 0;
}

static int
iterator_gc(lua_State *L)
{
    struct Iterator *i = lua_touserdata(L, -1);
    free(i->k.dptr);
    i->k.dptr = NULL;
    return 0;
}

static luaL_Reg iterator_meta[] = {
    { "__gc", iterator_gc },
    { NULL,   NULL        },
};


static int
shelve_index(lua_State *L)
{
    datum d, k;
    size_t slen_aux;
    struct Shelve *self = luaL_checkudata(L, 1, SHELVE_META);

    k.dptr  = (char*) lua_tolstring(L, 2, &slen_aux);
    k.dsize = (int) slen_aux;
    d = anydb_fetch(self->db, k);

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

static int
shelve_newindex(lua_State *L)
{
    datum k, d = { NULL, 0 };
    size_t slen_aux;
    struct Shelve *self = luaL_checkudata(L, 1, SHELVE_META);

    k.dptr  = (char*) lua_tolstring(L, 2, &slen_aux);
    k.dsize = (int) slen_aux;

    if (self->ro) {
        return luaL_error(L, "cannot modify read-only shelf datafile");
    }

    if (lua_isnil(L, 3)) {
        /* Remove key in database. */
        anydb_delete(self->db, k);
        return 0;
    }

    if (!shelve_marshal(L, &d.dptr, &d.dsize)) {
        return luaL_error(L, "cannot encode data");
    }

    if (anydb_store(self->db, k, d, ANYDB_REPLACE) != 0) {
        free(d.dptr);
        return luaL_error(L, "cannot update item in data file");
    }
    free(d.dptr);

    return 0;
}

static int
shelve_call(lua_State *L)
{
    struct Shelve *self = luaL_checkudata(L, -1, SHELVE_META);

    struct Iterator *i = lua_newuserdata(L, sizeof(struct Iterator));
    i->k = anydb_firstkey(self->db);
    i->db = self->db;
    luaL_setmetatable(L, SHELVE_ITER_META);

    lua_pushcclosure(L, iterator_next, 1);
    return 1;
}

static int
shelve_gc(lua_State *L)
{
    struct Shelve *self = luaL_checkudata(L, 1, SHELVE_META);
    if (!self->ro) {
        anydb_reorganize(self->db);
    }
    anydb_close(self->db);
    return 0;
}

static int
shelve_tostring(lua_State *L)
{
    struct Shelve *self = luaL_checkudata(L, 1, SHELVE_META);
    lua_pushfstring(L, "shelve (%p, %s)", self->db, self->ro ? "ro" : "rw");
    return 1;
}

static luaL_Reg shelve_meta[] = {
    { "__index",    shelve_index    },
    { "__newindex", shelve_newindex },
    { "__call",     shelve_call     },
    { "__gc",       shelve_gc       },
    { "__tostring", shelve_tostring },
    { NULL,         NULL            },
};


static int
shelve_module_open(lua_State *L)
{
    int flags = ANYDB_WRITE;
    struct Shelve *self = NULL;
    const char *rwmode = NULL;
    anydb_t db;
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
    if (!(db = anydb_open(luaL_checkstring(L, 1), flags))) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }

    self = lua_newuserdata(L, sizeof(struct Shelve));
    self->ro = (flags == ANYDB_READ);
    self->db = db;

    /* Associate metatable with userdata. */
    luaL_setmetatable(L, SHELVE_META);

    return 1;
}

static luaL_Reg shelve_module[] = {
    { "open",      shelve_module_open      },
    { "marshal",   shelve_module_marshal   },
    { "unmarshal", shelve_module_unmarshal },
    { NULL,        NULL                    },
};


LUALIB_API int
luaopen_shelve(lua_State *L)
{
    assert(L);

    /* Shelve file metatable */
    luaL_newmetatable(L, SHELVE_META);
#if LUA_VERSION_NUM < 502
    luaL_register(L, NULL, shelve_meta);
#else
    luaL_setfuncs(L, shelve_meta, 0);
#endif

    /* Shelve file iterator metatable */
    luaL_newmetatable(L, SHELVE_ITER_META);
#if LUA_VERSION_NUM < 502
    luaL_register(L, NULL, iterator_meta);
#else
    luaL_setfuncs(L, iterator_meta, 0);
#endif

    /* Module */
#if LUA_VERSION_NUM < 502
    luaL_register(L, "shelve", shelve_module);
#else
    luaL_newlib(L, shelve_module);
#endif

    return 1;
}
