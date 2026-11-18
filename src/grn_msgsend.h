/*
 * grn_msgsend.h
 *
 * $Id: grn_msgsend.h,v 1.3 2000/04/20 14:47:07 sc Exp $
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

#ifndef __GRN_MSGSEND_H__
#define __GRN_MSGSEND_H__

#include <gnome.h>
#include "grn_types.h"


typedef enum msg_send_mode
{
  MSG_SEND_NEW,
  MSG_SEND_REPLY,
  MSG_SEND_FORWARD
} msg_send_mode;

gchar *msg_to_smtp_text(t_message *);
GSList *rcpts2list(gchar *);
void evt_send_new(GtkWidget *);
void evt_send_reply(GtkWidget *);
void evt_send_forward(GtkWidget *);
void evt_send_ok(GtkWidget *);
void evt_send_cancel(GtkWidget *);


#endif /* __GRN_MSGSEND_H__ */
