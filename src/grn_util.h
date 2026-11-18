/*
 * grn_util.h - Definition of project-wide functions
 *
 * $Id: grn_util.h,v 1.32 2000/06/28 11:28:29 sc Exp $
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

#ifndef __GRN_UTIL_H__
#define __GRN_UTIL_H__

#include <time.h>
#include <gnome.h>
#include <pthread.h>
#include <sys/time.h>
#include "grn_types.h"


typedef struct _t_recode_mb t_recode_mb;
struct _t_recode_mb
{
  char from[3];
  char to;
};

typedef struct t_tz_table
{
  gchar name[5];
  gint  tz;
} t_tz_table;


gint grn_compare_strings(gchar *, gchar *);

void grn_appbar_push(GnomeAppBar *, gboolean, gchar *, ...);
void grn_appbar_pop(GnomeAppBar *);
void grn_appbar_set_status(GnomeAppBar *, gchar *, ...);
void grn_set_title(GtkWindow *, gchar *, ...);

void grn_set_contents(GtkWidget *);
void grn_widget_set_style(GtkWidget *, const gchar *, GdkColor *, GdkColor *,
			  GdkPixmap *);
GdkPixmap *grn_load_pixmap(const gchar *);
void grn_error(const gchar *area, gchar *message);
void grn_info(gchar *, ...);

void grn_main_iter(void);
void grn_g_list_append(GList **, GList **, gpointer);

gchar *str_subst(const gchar *src, gchar *var, gchar *value);
gboolean str_check(const gchar *);
gchar *str_copy(const gchar *);
void str_add(gchar **str, gchar *text);
void str_free(gchar **);
void str_destroy(gchar *);
void grn_subst_vars(gchar **);
gchar *grn_subst_tilde(const gchar *);

t_msgheader *parse_hdr(gchar *);
gchar *get_hdr_val(gchar *);

time_t parse_date(gchar *);
gchar *date_get_local(time_t);

gchar *get_host_name(void);
gchar *get_fqdn(gchar *);
gchar *c2s(GdkColor);

//void recode(char *, int);
gchar *get_charset(void);

void grn_lock();
void grn_unlock();

gint files_foreach(gchar *, void callback (gchar *));

void grn_save_ws(gint, GtkWidget *);

#endif /* __GRN_UTIL_H__ */
