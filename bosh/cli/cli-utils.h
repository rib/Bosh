/* General utility routines for GDB, the GNU debugger.

   Copyright (C) 1986, 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996,
   1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef GDB_UTILS_H
#define GDB_UTILS_H

#include <glib.h>

void
fputs_filtered (const char *linebuffer, GIOChannel *stream);

void
fprintf_filtered (GIOChannel *stream, const char *format, ...);

void
printf_filtered (const char *format, ...);

void
wrap_here (char *indent);

char *
savestring (const char *ptr, size_t size);

long long
parse_and_eval_long (char *exp);

#endif /* GDB_UTILS_H */

