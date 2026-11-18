/*
 * nntp.h
 *
 * $Id: nntp.h,v 1.9 2000/04/20 14:47:08 sc Exp $
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

#ifndef __NNTP_H__
#define __NNTP_H__

#include <gnome.h>
#include "grn_types.h"


gchar *nntp_connect(grn_socket *);
void nntp_wait_connect();
void nntp_disconnect(grn_socket *);
void nntp_offline(grn_socket *);
gboolean nntp_is_online(grn_socket *);

gchar *nntp_send_noop(grn_socket *);
gchar *nntp_set_group(grn_socket *, gchar *, gint *, gint *, gint *);
GList *nntp_get_grouplist(grn_socket *);
GHashTable *nntp_get_groupdesc(grn_socket *);
GList *nntp_get_msghdrlist(grn_socket *, gint, gint, gint, grn_newsgroup *, GnomeAppBar *);
gchar *nntp_get_article(grn_socket *, t_message *);
gchar *nntp_post_article(grn_socket *, t_message *);

void nntp_download_article(t_message *);

#endif /* __NNTP_H__ */
