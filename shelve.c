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

/* Deal with autoconf. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include "defs.h"
#include "shelve.h"
#include "anydb.h"

#ifndef LUALIB_API
# define LUALIB_API API
#endif


/* Function protos */
int l_shelve_index(lua_State*);
int l_shelve_nindex(lua_State*);
int l_shelve_trv(lua_State*);
int l_shelve_gc(lua_State*);
int l_shelve_open(lua_State*);
int l_shelve_tostring(lua_State*);


/* Lua userdata type */
typedef struct shelve_file_t {
    anydb_t dbf;
    char *fname;
    int  rdonly;
} shelve_file;

/* Metatable items */


#define SHELVE_META_ITEMS 5
static luaL_Reg meta[] =
{
    { "__index",    l_shelve_index    },
    { "__newindex", l_shelve_nindex   },
    { "__call",     l_shelve_trv      },
    { "__gc",       l_shelve_gc       },
    { "__tostring", l_shelve_tostring },
};


LUALIB_API int
luaopen_shelve(lua_State *L)
{
    unsigned i;

    assert(L);

    /* This is more polite to loadmodule and luacheia */
    if (lua_gettop(L) == 0) {
        lua_pushstring(L, "shelve");
    } else {
        luaL_checktype(L, -1, LUA_TSTRING);
    }

    /* Create the namespace table. */
    lua_newtable(L);

    /* Create the metatable. */
    lua_pushstring(L, SHELVE_REGISTRY_KEY);
    lua_newtable(L);
    for (i=0 ; i<SHELVE_META_ITEMS ; i++) {
        lua_pushstring(L, meta[i].name);
        lua_pushcfunction(L, meta[i].func);
        lua_rawset(L, -3);
    }
    lua_settable(L, LUA_REGISTRYINDEX);

    /* Set the "open" function. */
    lua_pushstring(L, "open");
    lua_pushcfunction(L, l_shelve_open);
    lua_rawset(L, -3);

    /* Set the marshal/unmarshal functions. */
    lua_pushstring(L, "marshal");
    lua_pushcfunction(L, l_shelve_marshal);
    lua_rawset(L, -3);
    lua_pushstring(L, "unmarshal");
    lua_pushcfunction(L, l_shelve_unmarshal);
    lua_rawset(L, -3);

    return 1;
}


int
l_shelve_open(lua_State *L)
{
    int flags = ANYDB_WRITE;
    char *filename = NULL;
    shelve_file *udata = NULL;
    const char *rwmode = NULL;
    anydb_t dbh;
    int n;

    assert(L);

    n = lua_gettop(L);
    /* Check arguments. */
    if ((n != 1) && (n != 2)) {
        luaL_error(L, "function takes one or two arguments");
    }

    /* Check & get second argument (if needed). */
    if (n == 2) {
        rwmode = luaL_checkstring(L, -1);
        flags  = (*rwmode == 'r') ? ANYDB_READ : ANYDB_WRITE;
        lua_pop(L, 1);
    }

    /* Check & get DB file name. */
    filename = xstrdup(luaL_checkstring(L, -1));
    lua_pop(L, 1);

    /* Open the DB and remove filename from the stack. */
    if ( !(dbh = anydb_open(filename, flags)) ) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }

    /* Configure returned userdata. */
    udata = (shelve_file*) lua_newuserdata(L, sizeof(shelve_file));
    udata->fname  = filename;
    udata->dbf    = dbh;
    udata->rdonly = (flags == ANYDB_READ);

    /* Associate metatable with userdata. */
    lua_pushstring(L, SHELVE_REGISTRY_KEY);
    lua_gettable(L, LUA_REGISTRYINDEX);
    lua_setmetatable(L, -2);
    lua_remove(L, -2);

    return 1;
}


int
l_shelve_index(lua_State *L)
{
    datum d, k;
    anydb_t *dbh;
    const char *datap;
    size_t slen_aux;

    assert(L);
    assert(lua_isuserdata(L, -2));

    dbh     = (anydb_t*) lua_touserdata(L, -2);
    k.dptr  = (char*) lua_tolstring(L, -1, &slen_aux);
    k.dsize = (int) slen_aux;

    d = anydb_fetch(*dbh, k);
    lua_pop(L, 2);

    if (!d.dptr) {
        lua_pushnil(L);
    }
    else {
        datap = d.dptr;

        if (!shelve_unmarshal(L, &datap)) {
            luaL_error(L, "bad format in encoded data");
        }
        xfree(d.dptr);
    }
    return 1;
}


int
l_shelve_nindex(lua_State *L)
{
    datum k, d = { NULL, 0 };
    shelve_file *udata;
    size_t slen_aux;

    assert(L);
    assert(lua_gettop(L) == 3);
    assert(lua_isuserdata(L, -3));

    udata   = (shelve_file*) lua_touserdata(L, -3);
    k.dptr  = (char*) lua_tolstring(L, -2, &slen_aux);
    k.dsize = (int) slen_aux;

    if (udata->rdonly) {
        luaL_error(L, "cannot modify read-only shelf datafile");
    }

    if (lua_isnil(L, -1)) {
        /* Remove key in database. */
        anydb_delete(udata->dbf, k);
        lua_pop(L, 3);
        return 0;
    }

    if (!shelve_marshal(L, &d.dptr, &d.dsize)) {
        luaL_error(L, "cannot encode data");
    }

    if (anydb_store(udata->dbf, k, d, ANYDB_REPLACE) != 0) {
        xfree(d.dptr);
        luaL_error(L, "cannot update item in data file");
    }
    xfree(d.dptr);

    lua_pop(L, 3);
    return 0;
}


int
l_shelve_trv(lua_State *L)
{
    anydb_t *dbh;
    datum k, tk;

    assert(L);
    assert(lua_gettop(L) == 1);
    assert(lua_isuserdata(L, -1));

    dbh = (anydb_t*) lua_touserdata(L, -1);
    lua_pop(L, 1);

    /*
     * A table containing all the keys is returned, all the keys
     * are assigned to 'true' boolean values. This is useful to
     * write Lua code similar to the following:
     *
     *    db = shelve.open("test.db")
     *    for key in db() do
     *        print("key: " .. key, "data: " .. db[key])
     *    end
     *
     */
    lua_newtable(L);
    for (k=anydb_firstkey(*dbh) ;
         k.dptr ;
         tk=k, k=anydb_nextkey(*dbh, k), xfree(tk.dptr))
    {
        lua_pushlstring(L, k.dptr, (size_t) k.dsize);
        lua_pushboolean(L, 1);
        lua_rawset(L, -3);
    }

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
    xfree(udata->fname);
    lua_pop(L, 1);
    return 0;
}


int
l_shelve_tostring(lua_State *L)
{
    shelve_file *udata;

    assert(L);
    assert(lua_gettop(L) == 1);
    assert(lua_isuserdata(L, -1));

    udata = (shelve_file*) lua_touserdata(L, -1);
    lua_pushfstring(L, "shelf (%s, %s)", udata->fname,
                    (udata->rdonly) ? "ro" : "rw");
    lua_remove(L, -2);
    return 1;
}

