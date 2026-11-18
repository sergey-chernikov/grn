/*
 * grn_msgpost.h
 *
 * $Id: grn_msgpost.h,v 1.12 2000/06/05 15:35:12 sc Exp $
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

#ifndef __GRN_MSGPOST_H__
#define __GRN_MSGPOST_H__

#include <gnome.h>
#include "grn_types.h"


typedef enum msg_goodness
{
  OKAY,
  INFO,
  WARN,
  BAD
} msg_goodness;

typedef enum msg_create_mode
{
  MSG_CREATE_NEW,
  MSG_CREATE_FOLLOWUP,
  MSG_CREATE_FORWARD
} msg_create_mode;

typedef struct msg_check_res
{
  msg_goodness	gl;
  gchar*	result;
} msg_check_res;


gchar *msg_to_text(t_message *);

void evt_post_new(GtkWidget *);
void evt_post_followup(GtkWidget *);
void evt_post_forward(GtkWidget *);
void evt_post_ok(GtkWidget *);
void evt_post_cancel(GtkWidget *);
void evt_sel_groups(GtkWidget *);
void evt_mhdredit_dlg_create(GtkWidget *);
void evt_compose_load_file(GtkWidget *w);

gchar *create_forward(t_message *);
gchar *create_quoting(t_message *);
void replace_msg(GtkObject *, t_message *, const gchar *);
msg_goodness get_goodness(GSList *);
gchar *goodness_list_to_str(GSList *, msg_goodness);
void goodness_list_free(GSList *);


#endif /* __GRN_MSGPOST_H__ */
