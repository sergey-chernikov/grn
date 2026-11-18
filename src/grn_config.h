/*
 * grn_config.h
 *
 * $Id: grn_config.h,v 1.32 2000/06/28 11:28:28 sc Exp $
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

#ifndef __GRN_CONFIG_H__
#define __GRN_CONFIG_H__

#include <glib.h>
#include "grn_find.h"

#define NFONTS		5
#define NCOLORS		3
#define NPIXMAPS	1
#define NCFMS		7
#define NWINSTATES	9
#define NPATHS		2


typedef struct grn_preferences
{
  gboolean	srv_autoconnect;
  gboolean	nntp_auth;
  gboolean	groups_desc;
  gboolean	scripts_autoload;
  gboolean	tl_ac;
  gboolean	tl_show_read;
  gboolean	mw_prop;
  gboolean	cfms[NCFMS];
  gboolean	tooltips;
  gboolean	smooth_scrolling;
  gshort	srt_ng_subscr;
  gshort	srt_ng_col;
  gshort	thrd_tl_type;
  gshort	srt_tl_col;
  gint		toolbar_style;
  gchar*	nntp_server;
  gchar*	nntp_login;
  gchar*	nntp_passwd;
  gchar*	smtp_server;
  gchar*	name;
  gchar*	email;
  gchar*	replyto;
  gchar*	org;
  gchar*	ua;
  gchar*	re_quoting;
  gchar*	paths[NPATHS];
  gchar*	fonts[NFONTS];
  gchar*	pm_names[NPIXMAPS];
  GdkColor	fg_colors[NCOLORS];
  GdkColor	bg_colors[NCOLORS];
  GdkPixmap*	pixmaps[NPIXMAPS];
  GtkAllocation ws[NWINSTATES];
  GSList*	fontpal;
  GSList*	colorpal;
} grn_preferences;

extern grn_preferences grn_prefs;


void grn_prefs_init(void);
void grn_prefs_done(void);
void grn_prefs_load(void);
void grn_prefs_save(void);
void grn_prefs_realize(void);
void grn_find_load(grn_find *);
void grn_find_save(grn_find *);

gint grn_get_toolbar_style();


#define GRN_PREFS_AUTOCONNECT	"NNTP/auto_groups"
#define GRN_PREFS_CFM		"Confirmations/cfm"
#define GRN_PREFS_COLOR_PAL	"ColorPalette/"
#define GRN_PREFS_COLOR_N_PAL	"ColorPalette/qty"
#define GRN_PREFS_FNT_BODY_P	"Fonts/article_body_prop"
#define GRN_PREFS_FNT_BODY_M	"Fonts/article_body_monosp"
#define GRN_PREFS_FNT_HDRS	"Fonts/article_headers"
#define GRN_PREFS_FNT_NGLIST	"Fonts/groups_listing"
#define GRN_PREFS_FNT_THRDLIST	"Fonts/threadlist"
#define GRN_PREFS_FONT_PAL	"FontPalette/"
#define GRN_PREFS_FONT_N_PAL	"FontPalette/qty"
#define GRN_PREFS_MW_PROP	"MsgWin/proportional_font"
#define GRN_PREFS_NAME		"Identity/name"
#define GRN_PREFS_EMAIL		"Identity/email_addr"
#define GRN_PREFS_REPLYTO	"Identity/replyto_addr"
#define GRN_PREFS_ORG		"Identity/organization"
#define GRN_PREFS_NNTPSERV	"NNTP/server_name"
#define GRN_PREFS_NNTP_AUTH	"NNTP/auth"
#define GRN_PREFS_NNTP_LOGIN	"NNTP/login"
#define GRN_PREFS_NNTP_PWD	"NNTP/password"
#define GRN_PREFS_GROUPS_DESC	"NNTP/groups_description"
#define GRN_PREFS_PATHS		"Paths/"
#define GRN_PREFS_PIXMAPS	"Pixmaps/"
#define GRN_PREFS_SCR_AUTOLOAD	"Scripts/autoload"
#define GRN_PREFS_SMTPSERV	"SMTP/server_name"
#define GRN_PREFS_SSCROLL	"General/smooth_scrolling"
#define GRN_PREFS_SRT_NG_SUBSCR	"GroupList/sort_subscr"
#define GRN_PREFS_SRT_NG_COL	"GroupList/sort_column"
#define GRN_PREFS_SRT_TL_COL	"ThreadList/sort_column"
#define GRN_PREFS_THRD_TL_TYPE	"ThreadList/threading"
#define GRN_PREFS_TL_AC		"ThreadList/auto_collapse"
#define GRN_PREFS_TL_SHOW_READ	"ThreadList/show_read"
#define GRN_PREFS_TBSTYLE	"Toolbar/style"
#define GRN_PREFS_TOOLTIPS	"General/tooltips"
#define GRN_PREFS_QUOTING	"General/quouting_regexp"
#define GRN_PREFS_UA		"Identity/user_agent"
#define GRN_PREFS_WINSTATES	"WindowStates/"
#define GRN_SRCH_DATA		"Search/Data"
#define GRN_SRCH_HDR_DATA	"Search/Hdr_data"
#define GRN_SRCH_SUBJ		"Search/Subject"
#define GRN_SRCH_FROM		"Search/From"
#define GRN_SRCH_BODY		"Search/Body"
#define GRN_SRCH_HDR		"Search/Header"
#define GRN_SRCH_REGEX		"Search/Regex"
#define GRN_SRCH_SENS		"Search/Case_Sensitive"
#define GRN_SRCH_UNREAD		"Search/Unread_only"


#endif /* __GRN_CONFIG_H__ */
