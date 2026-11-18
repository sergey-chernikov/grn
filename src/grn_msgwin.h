/*
 * grn_msgwin.h
 *
 * $Id: grn_msgwin.h,v 1.6 2000/04/10 02:42:52 sc Exp $
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

#ifndef __GRN_MSGWIN_H__
#define __GRN_MSGWIN_H__

#include "grn_types.h"


typedef struct grn_msgwin_stats
{
  guint msg_size;
} grn_msgwin_stats;


GtkWidget *msgwin_create();
void msgwin_refresh(GtkWidget *, t_message *);

void evt_mw_prop_font(GtkWidget *);
void evt_mw_monosp_font(GtkWidget *);

void msgwin_set_hdr(GtkWidget *, gchar *, gchar *);
t_message *get_cur_msg(GtkWidget *);
gchar *get_cur_group(GtkWidget *);


#endif /* __GRN_MSGWIN_H__ */
