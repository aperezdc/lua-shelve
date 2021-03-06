/*
 * Marshaling/Unmarshaling functions, Shelve module exports.
 * Copyright (C) 2003-2016 Adrian Perez de Castro
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


#ifndef __lua_module_shelve__h
#define __lua_module_shelve__h

#include <lua.h>

#if (defined(__GNUC__) && __GNUC__ >= 4) || __clang__
# define LOCAL_SYMBOL __attribute__((visibility("hidden")))
#else
# define LOCAL_SYMBOL
#endif

/*
 * Lua to byte-array marshaling functions. Representation of
 * numbers is machine-dependant.
 *
 * Format:
 *   'n'XXXX   Number: XXXX=value, sizeof(lua_Number)
 *   's'XXXXS  String: XXXX=length, sizeof(size_t), S=contents.
 *   'b'       Boolean, false.
 *   'B'       Boolean, true.
 *   't'XXXX{} Table: XXXX=number of items, sizeof(size_t).
 *             Each element: KV.
 *    KV       K: key, either nXXXX or sXXXXS (only in tables).
 *             V: value, either nXXXX, sXXXXS, b, B or t{}.
 *    '!'      End-of-sequence marker.
 *    'T'      End-of-table marker.
 */

#define MARSH_NUMBER  'n'
#define MARSH_STRING  's'
#define MARSH_TRUE    'B'
#define MARSH_FALSE   'b'
#define MARSH_TABLE   't'
#define MARSH_EOT     'T'
#define MARSH_EOS     '!'

LOCAL_SYMBOL int shelve_marshal(lua_State*, char**, int*);
LOCAL_SYMBOL int shelve_unmarshal(lua_State*, const char**);
LOCAL_SYMBOL int shelve_module_marshal(lua_State*);
LOCAL_SYMBOL int shelve_module_unmarshal(lua_State*);

#endif /* !__lua_module_shelve__h */
