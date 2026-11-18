/*
 * grn_news.h
 *
 * $Id: grn_news.h,v 1.12 2000/04/10 02:42:52 sc Exp $
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

#ifndef __GRN_NEWS_H__
#define __GRN_NEWS_H__

#include <glib.h>
#include "grn_types.h"

GHashTable *read_newsrc(const gchar *);
void write_newsrc(const gchar *, const gchar *, GHashTable *);
void free_newsrc(GHashTable *);
gboolean change_newsrc(GHashTable *, const gchar *, const gchar *);
glong grn_news_get_unread(grn_newsgroup *);
void fill_newsrc_ng_list(GList *, GHashTable *);
void fill_newsrc_ng_desc(GList *, GHashTable *);
gboolean grn_news_art_read(gchar *, gulong);
gchar *grn_news_add_read(gchar *, gulong);
gchar *grn_news_add_unread(gchar *, gulong);
gchar *grn_news_read_all(gulong);
void grn_news_group_subscribe(GHashTable *, grn_newsgroup *);
void grn_news_group_unsubscribe(GHashTable *, grn_newsgroup *);

#endif /* __GRN_NEWS_H__ */
