/*
 * Gdbm/Ndbm wrapper header.
 * Wrappers are modelled after the Gdbm prototypes.
 *
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

#ifndef __anydb__h
#define __anydb__h

/* Useable DBM backends */
#define ANYDB_NDBM  1
#define ANYDB_GDBM  2

/* Handle Autoconf like HAVE_GDBM and HAVE_NDBM, Gdbm used by default */
#ifndef ANYDB_BACKEND
# if defined(HAVE_GDBM)
#  define ANYDB_BACKEND  ANYDB_GDBM
# elif defined(HAVE_NDBM)
#  define ANYDB_BACKEND  ANYDB_NDBM
# else
#  define ANYDB_BACKEND  ANYDB_GDBM
# endif
#endif

/* Use rw-rw--- mode by default */
#ifndef ANYDB_OPEN_MODE
# define ANYDB_OPEN_MODE  0660
#endif

/* Define wrappers depending on the used backend */
#if (ANYDB_BACKEND == ANYDB_NDBM)
# include <ndbm.h>             /* Backend header file */
# include <fcntl.h>            /* O_CREAT and O_RDWR macros */
# define ANYDB_READ            O_RDONLY
# define ANYDB_WRITE           (O_CREAT | O_RDWR)
# define ANYDB_INSERT          DBM_INSERT
# define ANYDB_REPLACE         DBM_REPLACE
  typedef DBM* anydb_t;        /* Database handle type */
# define anydb_open(_n,_f)     dbm_open(_n, _f, ANYDB_OPEN_MODE)
# define anydb_close           dbm_close
# define anydb_store           dbm_store
# define anydb_fetch           dbm_fetch
# define anydb_delete          dbm_delete
# define anydb_firstkey        dbm_firstkey
# define anydb_nextkey(_h,_k)  dbm_nextkey(_h)
# define anydb_reorganize(_i)  ((void)0)
#elif (ANYDB_BACKEND == ANYDB_GDBM)
# include <gdbm.h>             /* Backend header file */
# define ANYDB_READ            GDBM_READER
# define ANYDB_WRITE           GDBM_WRCREAT
# define ANYDB_INSERT          GDBM_INSERT
# define ANYDB_REPLACE         GDBM_REPLACE
  typedef GDBM_FILE anydb_t;   /* Database handle type */
# define anydb_open(_n,_f)     gdbm_open(_n, 0, _f, ANYDB_OPEN_MODE, NULL)
# define anydb_close           gdbm_close
# define anydb_store           gdbm_store
# define anydb_fetch           gdbm_fetch
# define anydb_delete          gdbm_delete
# define anydb_firstkey        gdbm_firstkey
# define anydb_nextkey         gdbm_nextkey
# define anydb_reorganize      gdbm_reorganize
#else
  /* No backend defined -> error */
# error Neither Ndbm or Gdbm configured.
#endif

#endif /* !__anydb__h */
