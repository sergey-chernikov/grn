/*
 * grn_perl.h: prototypes for perl.c
 *
 * $Id: grn_perl.h,v 1.8 2000/06/20 15:12:32 sc Exp $
 */
/* Copyright (C) 1999-2000  Sergey Chernikov (sc@ivvs.ul.ru)
 *
 * Authors: Sergey Chernikov <sc@ivvs.ul.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef __GRN_PERL_H__
#define __GRN_PERL_H__

#include <glib.h>


typedef struct perl_command_handler
{
  gchar *command_name;
  gint  context;
  gchar *handler_name;
} perl_command_handler;

typedef struct perl_hook_handler
{
  gchar *hook_name;
  gchar *handler_name;
} perl_hook_handler;


extern GSList *perl_commands;


int perl_load_file(gchar *);
void perl_init(gboolean);
void perl_end();
void grn_perl_restart();
GSList *grn_perl_list();

glong perl_hook_run(gchar *, grn_session *, gpointer, gpointer);
glong perl_command_run(gchar *, grn_session *, gpointer);


#endif /* __GRN_PERL_H__ */
