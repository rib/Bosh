/* Header file for GDB CLI command implementation library.
   Copyright (c) 2000,2006,2007,2008 Free Software Foundation, Inc.

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

#if !defined (CLI_CMDS_H)
#define CLI_CMDS_H 1

#include <glib.h>

/* Chain containing all defined commands.  */
extern struct cmd_list_element *cmdlist;

extern struct cmd_list_element *infolist;

void bosh_init_commands (void);

void bosh_command_error_no_argument (char *why);

#endif /* !defined (CLI_CMDS_H) */
