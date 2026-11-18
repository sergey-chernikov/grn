/*
 * grn_misc.h - Definition of project-wide miscellaneous functions
 *
 * $Id: grn_misc.h,v 1.22 2000/05/03 13:02:35 sc Exp $
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

#ifndef __GRN_MISC_H__
#define __GRN_MISC_H__

#include <gtk/gtk.h>
#include "grn_types.h"

t_msgheaders *t_msgheaders_alloc(void);
t_msgheaders *t_msgheaders_copy(t_msgheaders *);
void t_msgheaders_free(t_msgheaders **);
gchar *t_msgheaders_get_hdr(t_msgheaders *, gchar *);
gchar *t_msgheaders_get_extra_hdr(t_msgheaders *, gchar *);
void t_msgheaders_set_hdr(t_msgheaders *, gchar *, gchar *);
void t_msgheaders_add_extra_hdr(t_msgheaders *, gchar *, gchar *);
void t_msgheaders_rm_hdr(t_msgheaders *, gchar *);
void t_msgheaders_rm_extra_hdr(t_msgheaders *, gchar *);
void t_msgheaders_rm_extra_hdrs(t_msgheaders *);
void msghdrlist_free(GList **);

t_message *t_message_alloc(void);
t_message *t_message_copy(t_message *);
void t_message_free(t_message **);
void t_message_destroy(t_message *);
void msglist_free(GList **);
void msglist_destroy(GList *);
gboolean t_message_is_read(t_message *);
t_message *msglist_find_cur_thread(GList *, t_message *);
t_message *msglist_find_prev_unread(GList *, t_message *);
t_message *msglist_find_next_unread(GList *, t_message *);
t_message *msglist_find_prev_thread_unread(GList *, t_message *);
t_message *msglist_find_next_thread_unread(GList *, t_message *);
t_message *msglist_find_by_id(GList *, t_message *);
t_message *msglist_find_by_subj(GList *, t_message *);

grn_newsgroup *grn_newsgroup_alloc(void);
void grn_newsgroup_free(grn_newsgroup **);
void grn_newsgroup_set_seq(grn_newsgroup *, gchar *);
void grouplist_free(GList **);
gulong grouplist_get_nunread(GList *);

grn_find *grn_find_alloc(void);
void grn_find_free(grn_find **);


#endif /* __GRN_MISC_H__ */
