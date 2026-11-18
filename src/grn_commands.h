/*
 * grn_commands.h
 *
 * $Id: grn_commands.h,v 1.15 2000/06/20 15:12:32 sc Exp $
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

#ifndef __GRN_COMMANDS_H__
#define __GRN_COMMANDS_H__

#include <gnome.h>

gint evt_app_quit(GtkWidget *);
gint evt_cleanup(GtkWidget *, gpointer);
void evt_not_impl(GtkWidget *);
void evt_about(GtkWidget *, gpointer);
int evt_save_state(GnomeClient *, gint, GnomeRestartStyle, gint, 
    GnomeInteractStyle, gint, gpointer);
void evt_appbar_destroy(GtkWidget *);

void evt_grouplist(GtkWidget *);
void evt_perl_restart(GtkWidget *);
void evt_perl_load(GtkWidget *);
void evt_perl_list(GtkWidget *);
void evt_perl_command(GtkWidget *, gpointer);
void evt_save_prefs(GtkWidget *);
void evt_prev(GtkWidget *);
void evt_next(GtkWidget *);

#endif /* __GRN_COMMANDS_H__ */
