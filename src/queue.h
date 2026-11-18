/*
 * queue.h
 *
 * $Id: queue.h,v 1.12 2000/04/20 14:47:08 sc Exp $
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

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "grn_types.h"
#include "nntp.h"


#define GRNQ_DOWNLOAD_ARTICLE		1
#define GRNQ_POST_ARTICLE		2
#define GRNQ_DOWNLOAD_GROUPLIST		3
#define GRNQ_DOWNLOAD_MSGHDRLIST	4
#define GRNQ_GROUPLIST_RECALC		5
#define GRNQ_THREADLIST_RECALC		6
#define GRNQ_SEND_MESSAGE		7
#define GRNQ_DRAW_GROUPLIST		40
#define GRNQ_DRAW_MSGLIST		41
#define GRNQ_DRAW_MSGLIST1		42
#define GRNQ_DRAW_ARTICLE		43
#define GRNQ_FOREGROUND			128
#define GRNQ_BACKGROUND			256


typedef struct grnq_data
{
  guint	flags;
  void	*ptr;
  gpointer data;
} grnq_data;


void grnq_append(grnq_data *);
void grnq_prepend(grnq_data *);
grnq_data *grnq_get_next(void);
void grnq_init(void);
void grnq_processing(void);
void grnq_purge(void);
gboolean grnq_run(grnq_data *, grnq_data *);

void grnq_add_download_grouplist(void);
void grnq_add_download_msghdrlist(grn_newsgroup *);
void grnq_add_download_article(t_message *);
void grnq_add_post_article(t_message *);
void grnq_add_grouplist_recalc(void);
void grnq_add_threadlist_recalc(GtkWidget *);
void grnq_add_send_message(t_message *);
void grnq_add_draw_grouplist(void);
void grnq_add_draw_msglist(void);
void grnq_add_draw_msglist1(void);
void grnq_add_draw_article(t_message *);

#endif /* __QUEUE_H__ */
