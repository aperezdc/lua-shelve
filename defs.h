/*
 * Common definitions.
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

#ifndef __defs__h
#define __defs__h

/*
 * Header declarations start/end macros. These protect definitions
 * to be mangled by C++ compilers, so linking with plain C functions
 * and variables is done correctly.
 */
#if defined(__cplusplus) || defined(c_plusplus)
# define BEGIN_DECLS  extern "C" {
# define END_DECLS    }
#else
# define BEGIN_DECLS
# define END_DECLS
#endif


/*
 * Chain of fools to detect wether we are using Win32.
 */
#if defined(__WINDOWS__) || defined(_WINDOWS) || defined(_Windows) || \
    defined(__WIN32__)   || defined(_WIN32)   || defined(WIN32)    || \
    defined(__NT__) || defined(__NT_DLL__) || defined(__WINDOWS_386__)
# define PLAT_WIN32
#else
# define PLAT_POSIX
#endif


/*
 * The API macro is used to mark functions that must be exported to
 * the world. Under Win32 it's expanded to export symbols in DLLs
 * when appropiate.
 */
#if defined(PLAT_WIN32) && defined(BUILD_DLL)
# define API extern __declspec(dllexport)
#elif defined(PLAT_WIN32) && defined(USE_DLL)
# define API extern __declspec(dllimport)
#else
# define API extern
#endif


/*
 * Exits the program printing a string to stdout.
 */
extern void exit();
#include <stdio.h>
#define __die(__str) \
    (fprintf(stderr, "fatal: %s\n", __str), exit(-1))

/*
 * The xmalloc(), xrealloc() and xfree() act as expected, but they
 * do pointer checks when needed. They are defined as inlines or
 * macros, so using them does not involve extra function calls.
 * "__inline" is used to prevent complains of old silly compilers.
 */
extern void* malloc();
extern void* realloc();
extern void free();

#ifdef __GNUC__
# define INLINE inline
#else
# define INLINE __inline
#endif

static INLINE void* xmalloc(size_t sz)
{
    register void *p = malloc(sz);
    return (p ? p : (__die("out of memory"), (void*)0));
}

#define xrealloc(__p, __nsz) \
    ((__p) = realloc(__p, __nsz), (__p) ? __p : (__die("out of memory"), (void*)0))

#define xfree(__p) \
    (free(__p), (__p) = NULL)


/*
 * strdup() replacement
 */
#if !defined(HAVE_STRDUP) && (defined(_OSX) || defined(_LINUX))
# define HAVE_STRDUP
#endif

#if defined(HAVE_STRDUP) && defined(__GNUC__)
# define xstrdup(__strp) \
    ({ char *__$strp = strdup(__strp); \
       (__$strp) ? : (__die("out of memory"), (char*)0); })
#else
# ifdef __GNUC__
#  define xstrdup(__strp) \
    ({ const char* __$strp = __strp; \
       strcpy((char*) xmalloc(sizeof(char)*(strlen(__$strp)+1)), __$strp); })
# else
static INLINE char* xstrdup(const char *strp)
{
    return strcpy((char*) xmalloc(sizeof(char) * (strlen(strp) + 1)), strp);
}
# endif
#endif

#endif /* !__defs__h */
