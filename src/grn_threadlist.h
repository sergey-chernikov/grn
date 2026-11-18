/*
 * grn_threadlist.h
 *
 * $Id: grn_threadlist.h,v 1.14 2000/05/12 14:06:10 sc Exp $
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

#ifndef __GRN_THREADLIST_H__
#define __GRN_THREADLIST_H__

#include <gtk/gtk.h>
#include "grn_types.h"


void threadlist_refresh(GtkWidget *, GList *, gboolean);
GtkWidget *threadlist_create();
void evt_expand_cur_thread(GtkWidget *);
void evt_expand_all_threads(GtkWidget *);
void evt_collapse_cur_thread(GtkWidget *);
void evt_collapse_all_threads(GtkWidget *);
void evt_tl_open_article(GtkWidget *);
void evt_tl_mark_read(GtkWidget *);
void evt_tl_mark_thread_read(GtkWidget *);
void evt_tl_mark_unread(GtkWidget *);
void evt_tl_mark_thread_unread(GtkWidget *);
void evt_tl_urprev_msg(GtkWidget *);
void evt_tl_urprev_thread(GtkWidget *);
void evt_tl_urnext_msg(GtkWidget *);
void evt_tl_urnext_thread(GtkWidget *);
void evt_tl_back(GtkWidget *);

t_message *tl_get_selected_message(GtkCTree *);
void tl_select_message(GtkCTree *, t_message *cur_msg, t_message *msg);
t_message *tl_get_root_message(GtkCTree *, t_message *);
void get_unread_tl_stats(GList *, grn_threadlist_stats *);
void show_tl_stats(grn_threadlist_stats *);

#endif /* __GRN_THREADLIST_H__ */
