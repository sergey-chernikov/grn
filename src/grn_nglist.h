/*
 * grn_nglist.h
 *
 * $Id: grn_nglist.h,v 1.16 2000/04/10 02:42:52 sc Exp $
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

#ifndef __GRN_NGLIST_H__
#define __GRN_NGLIST_H__

#include <gtk/gtk.h>

GtkWidget *grouplist_create();
void grouplist_refresh(GtkWidget *, GList *);
void grouplist_recalc_unread(GtkWidget *);
void ng_list_download(gboolean);
void ng_list_open_group(GtkWidget *);
void evt_group_subscribe(GtkWidget *);
void evt_group_unsubscribe(GtkWidget *);
void evt_group_catchup(GtkWidget *);
void evt_group_unread(GtkWidget *);
void evt_groups_refresh(GtkWidget *);
void evt_group_urprev(GtkWidget *);
void evt_group_urnext(GtkWidget *);

GtkWidget *groupsel_create();
GtkWidget *groupsel_dlg_create(GSList *);
void groupsel_refresh(GtkWidget *, GSList *);

void clist_renumber(GtkCList *);

#endif /* __GRN_NGLIST_H__ */
