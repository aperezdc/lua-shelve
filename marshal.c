
/*
 * (Un)Marshaling of Lua datatypes.
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

#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include "shelve.h"


#define GROW(_dp, _szvar, _sz) \
    _szvar += (_sz); \
    _dp = ((_dp) ? realloc(_dp, _szvar * sizeof(char)) \
                 : malloc(_szvar * sizeof(char)))

#define STOR(_dp, _szvar, _what, _sz) \
    { GROW(_dp, _szvar, _sz); \
      memcpy(&(_dp)[(_szvar) - (_sz)], _what, _sz); }


static int marshal_table(lua_State*, char**, int*);
static int unmarshal_table(lua_State*, const char**);


int
shelve_unmarshal(lua_State *L, const char **datap)
{
    size_t ssz;
    const char *data = *datap;

    assert(L);
    assert(data);

    switch (*(data++)) {
        case MARSH_FALSE: /* 'false' boolean value */
            lua_pushboolean(L, 0);
            break;
        case MARSH_TRUE: /* 'true' boolean value */
            lua_pushboolean(L, 1);
            break;
        case MARSH_NUMBER: /* numeric value */
            lua_pushnumber(L, (lua_Number) (*((double*) data)) );
            data += sizeof(double);
            break;
        case MARSH_STRING: /* string value */
            ssz   = *((size_t*) data);
            data += sizeof(size_t);
            lua_pushlstring(L, data, ssz);
            data += ssz;
            break;
        case MARSH_TABLE: /* table */
            if (!unmarshal_table(L, &data)) return 0;
            break;
        default:
            return 0;
    }

    if (*(data++) != MARSH_EOS) return 0;

    *datap = data;
    return 1;
}


int
unmarshal_table(lua_State *L, const char **datap)
{
    const char *data = *datap;

    assert(L);
    assert(data);

    lua_newtable(L);
    /*
     * stack: tbl
     * index:  -1
     */

    for (;;) {
        switch (*data) {
            case MARSH_NUMBER: /* valid key (number) */
            case MARSH_STRING: /* valid key (string) */
                if (!shelve_unmarshal(L, &data) ||
                    !shelve_unmarshal(L, &data)) return 0;
                /*
                 * stack: data key tbl
                 * index:   -1  -2  -3
                 */
                lua_rawset(L, -3);
                break;
            case MARSH_EOT: /* end of table */
                data++;
                /*
                 * I know using 'goto' is not structured programming, but
                 * I use it to avoid checking "*data" twice. Without
                 * 'goto' the code would look like the following:
                 *
                 *   while (*data != MARSH_EOS) {
                 *       switch (*data) {
                 *           ... some code ...
                 *       }
                 *   }
                 */
                goto loop_end;
            default: /* not number, not string: invalid key */
                return 0;
        }
    }
loop_end:

    *datap = data;
    return 1;
}


int
shelve_marshal(lua_State *L, char **data, int *bytes)
{
    size_t slen;
    size_t slen_aux;
    char ch;

    assert(L);
    assert(data);
    assert(bytes);
    assert(lua_gettop(L) > 0);

    switch (lua_type(L, -1)) {
        case LUA_TNUMBER: /* Encode a number. */
            slen = 1 + sizeof(double);
            {
                char __d[slen];
                __d[0] = MARSH_NUMBER;
                *((double*)(&__d[1])) = (double) lua_tonumber(L, -1);
                STOR(*data, *bytes, __d, slen);
            }
            break;
        case LUA_TBOOLEAN: /* Encode a boolean value. */
            ch = (lua_toboolean(L, -1)) ? MARSH_TRUE : MARSH_FALSE;
            STOR(*data, *bytes, &ch, 1);
            break;
        case LUA_TSTRING: /* Encode a string value. */
            slen = 1 + sizeof(size_t);
            {
                char __d[slen];
                const char *lstr = lua_tolstring (L, -1, &slen_aux);
                __d[0] = MARSH_STRING;
                *((size_t*)(&__d[1])) = slen_aux;
                STOR(*data, *bytes, __d, slen);
                STOR(*data, *bytes, lstr, slen_aux);
            }
            break;
        case LUA_TTABLE: /* Encode a table. */
            if (!marshal_table(L, data, bytes)) return 0;
        case LUA_TNIL: /* Just skip the 'nil' value. */
            break;
        case LUA_TFUNCTION:
        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA:
        default: /* Cannot handle userdatas or functions. */
            return 0;
    }

    ch = MARSH_EOS;
    STOR(*data, *bytes, &ch, 1);

    return 1;
}


int
marshal_table(lua_State *L, char **data, int *bytes)
{
    char ch = MARSH_TABLE;

    assert(L);
    assert(data);
    assert(bytes);
    assert(lua_type(L, -1) == LUA_TTABLE);

    STOR(*data, *bytes, &ch, 1);

    lua_pushnil(L);
    while (lua_next(L, -2)) {
        assert(lua_type(L, -3) == LUA_TTABLE);
        /*
         * - "key" is at index -2 and "value" at index -1
         * - "key" must be encoded first, so move it to the top
         * lua_insert() moves "value" down to -2 and shifts "key" up to -1
         * stack: key val tbl
         * index:  -1  -2  -3
         */
        lua_insert(L, -2);
        if (!shelve_marshal(L, data, bytes)) return 0; /* encode "key" */
        assert(lua_type(L, -3) == LUA_TTABLE);
        /*
         * Again, lua_insert() moves "key" down to -2 and shifted "value" up to -1.
         * stack: val key tbl
         * index:  -1  -2  -3
         */
        lua_insert(L, -2);
        if (!shelve_marshal(L, data, bytes)) return 0; /* encode "value" */
        assert(lua_type(L, -3) == LUA_TTABLE);
        /*
         * Pop "value", "key" remains in order to call lua_next() properly.
         * stack: key tbl
         * index:  -1  -2
         */
        lua_pop(L, 1);
        assert(lua_type(L, -2) == LUA_TTABLE);
    }

    assert(lua_type(L, -1) == LUA_TTABLE);

    /* Insert MARSH_EOS */
    ch = MARSH_EOT;
    STOR(*data, *bytes, &ch, 1);

    return 1;
}


int
shelve_module_marshal(lua_State *L)
{
    char *data = NULL;
    int sz  = 0;

    assert(L);

    if (shelve_marshal(L, &data, &sz)) {
        lua_pushlstring(L, data, (size_t) sz);
        return 1;
    }

    lua_pushnil(L);
    lua_pushstring(L, "Cannot encode data");
    return 2;
}


int
shelve_module_unmarshal(lua_State *L)
{
    const char *data = luaL_checkstring(L, 1);
    if (shelve_unmarshal(L, &data)) {
        return 1;
    }

    return luaL_error(L, "bad format in encoded data");
}
