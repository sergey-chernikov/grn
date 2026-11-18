/*
 * grn_menu.c: menus data, menu manipulating functions
 *
 * $Id: grn_menu.c,v 1.45 2000/07/19 06:26:21 sc Exp $
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

#include "grn_consts.h"
#include "grn_menu.h"
#include "grn_commands.h"
#include "grn_vars.h"
#include "grn_config.h"
#include "grn_util.h"
#include "grn_prefs.h"
#include "grn_nglist.h"
#include "grn_threadlist.h"
#include "grn_msgwin.h"
#include "grn_msgpost.h"
#include "grn_mhdrwin.h"
#include "grn_msgsend.h"
#include "grn_find.h"
#include "grn_mime.h"
#include "grn_perl.h"
#include "grn_attach.h"

#include "../pixmaps/ball-green.xpm"
#include "../pixmaps/ball-ltred.xpm"


static GnomeUIInfo grn_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("Show newsgroups"), N_("Show the list of available newsgroups"), evt_grouplist, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_INDEX, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Message headers.."), N_("Show all message headers"), evt_mhwin_show, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  GNOMEUIINFO_MENU_EXIT_ITEM(evt_app_quit, NULL),
  { GNOME_APP_UI_ENDOFINFO }
};

static GnomeUIInfo ng_navigation_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("Prev unread group"), N_("Go to previous group with unread messages"), evt_group_urprev, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BACK, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Next unread group"), N_("Go to next group with unread messages"), evt_group_urnext, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FORWARD, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Mark unread"), N_("Mark currently selected group unread"), evt_group_unread, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FIRST, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Mark read"), N_("Mark currently selected group read (catchup)"), evt_group_catchup, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_LAST, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo ng_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("Open"), N_("Open currently selected newsgroup"), ng_list_open_group, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_OPEN, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Subscribe"), N_("Subscribe to the current group"), evt_group_subscribe, NULL, NULL,
    GNOME_APP_PIXMAP_DATA, ball_green_xpm, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Unsubscribe"), N_("Unsubscribe from the current group"), evt_group_unsubscribe, NULL, NULL,
    GNOME_APP_PIXMAP_DATA, ball_ltred_xpm, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_SUBTREE, N_("Navigation"), NULL, ng_navigation_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("ReRead list"), N_("Re-read the current newsgroups list"), evt_groups_refresh, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REFRESH, 0, 0, NULL },
  { GNOME_APP_UI_ENDOFINFO }
};

static GnomeUIInfo msg_reply_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("To group"), NULL, evt_post_followup, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Via mail"), NULL, evt_send_reply, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
/*  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("With options..."), NULL, evt_not_impl, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },*/
  GNOMEUIINFO_END
};

static GnomeUIInfo msg_forward_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("To group"), NULL, evt_post_forward, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Via mail"), NULL, evt_send_forward, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
/*  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("With options..."), NULL, evt_not_impl, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },*/
  GNOMEUIINFO_END
};

static GnomeUIInfo mw_fonts_group[] =
{
  { GNOME_APP_UI_ITEM, N_("Proportional font"), N_("Variable width font"), evt_mw_prop_font, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Monospaced font"), N_("Single width font"), evt_mw_monosp_font, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ENDOFINFO }
};

static GnomeUIInfo msg_view_menu[] =
{
  GNOMEUIINFO_RADIOLIST(mw_fonts_group),
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("MIME"), N_("View MIME structure"), evt_mime_view, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ATTACH, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo message_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("Open"), N_("View current message"), evt_tl_open_article, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_OPEN, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Post new"), N_("Compose and post message to current newsgroup"), evt_post_new, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_MAIL_NEW, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Send new"), N_("Compose and send message via SMTP"), evt_send_new, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_MAIL_NEW, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Reply"), NULL, msg_reply_menu, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_MAIL_RPL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Forward"), NULL, msg_forward_menu, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_MAIL_FWD, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_SUBTREE, N_("View"), N_("Some message viewing options"), msg_view_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
//  { GNOME_APP_UI_SUBTREE, N_("File"), NULL, msg_file_menu, NULL, NULL,
//    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo tl_navigation_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("Expand current thread"), N_("Expand current thread"), evt_expand_cur_thread, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Expand all threads"), N_("Expand all threads"), evt_expand_all_threads, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Collapse current thread"), N_("Collapse current thread"), evt_collapse_cur_thread, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Collapse all threads"), N_("Collapse all threads"), evt_collapse_all_threads, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Prev unread message"), N_("Go to previous unread message"), evt_tl_urprev_msg, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BACK, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Prev unread thread"), N_("Go to previous unread thread"), evt_tl_urprev_thread, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BACK, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Next unread message"), N_("Go to next unread message"), evt_tl_urnext_msg, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FORWARD, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Next unread thread"), N_("Go to next unread thread"), evt_tl_urnext_thread, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FORWARD, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Mark message unread"), N_("Mark current message as unread"), evt_tl_mark_unread, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FIRST, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Mark thread unread"), N_("Mark current thread as unread"), evt_tl_mark_thread_unread, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FIRST, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Mark message read"), N_("Mark current message as read"), evt_tl_mark_read, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_LAST, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Mark thread read"), N_("Mark current thread as read"), evt_tl_mark_thread_read, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_LAST, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo threadlist_menu[] =
{
  { GNOME_APP_UI_SUBTREE, N_("Navigation"), NULL, tl_navigation_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Search..."), N_("Search a message in current group"), evt_find_dlg, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Search again"), NULL, evt_find_again, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REFRESH, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Back"), N_("Return to newsgroups listing"), evt_tl_back, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BACK, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo settings_menu[] =
{
  GNOMEUIINFO_MENU_PREFERENCES_ITEM(evt_preferences, NULL),
  { GNOME_APP_UI_ITEM, N_("Font palette..."), N_("Edit font palette"), evt_fontpal, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FONT, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Color palette..."), N_("Edit color palette"), evt_colorpal, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_INDEX, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Save preferences"), NULL, evt_save_prefs, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 0, 0, NULL },
  GNOMEUIINFO_END
};


static GnomeUIInfo scripts_perl_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("Load..."), N_("Load script"), evt_perl_load, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("List.."), N_("View list of loaded scripts"), evt_perl_list, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_INDEX, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Restart all"), N_("Restart all loaded scripts"), evt_perl_restart, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REFRESH, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo scripts_menu[] =
{
  { GNOME_APP_UI_SUBTREE, N_("Perl"), NULL, scripts_perl_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] =
{
  GNOMEUIINFO_HELP("grn"),
  { GNOME_APP_UI_SEPARATOR },
  GNOMEUIINFO_MENU_ABOUT_ITEM(evt_about, NULL),
  GNOMEUIINFO_END
};

GnomeUIInfo main_menu[] =
{
  { GNOME_APP_UI_SUBTREE, N_("GRN"), NULL, grn_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Newsgroup"), N_("Newsgroup operations"), ng_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Threadlist"), N_("Threadlist operations"), threadlist_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Message"), N_("Article (message) opertaions"), message_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Settings"), NULL, settings_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Scripts"), NULL, scripts_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Help"), NULL, help_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ENDOFINFO }
};


GnomeUIInfo main_toolbar[] =
{
  { GNOME_APP_UI_ITEM, N_("Back"), N_("Return to previous window"), evt_prev, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_BACK, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Next"), N_("Go forward"), evt_next, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_FORWARD, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Preferences"), N_("Configure the application"), evt_preferences, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PREFERENCES, 0, 0, NULL },
  GNOMEUIINFO_END
};


static GnomeUIInfo msgpost_file_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("Load..."), N_("Load file to current window"), evt_compose_load_file, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Save..."), N_("Save current message to file"), evt_not_impl, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo msgpost_msg_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("Post"), N_("Post message to group"), evt_post_ok, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_MAIL_SND, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Cancel"), N_("Cancel message posting"), evt_post_cancel, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Attachments..."), N_("View attachments dialog"), evt_attachments, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ATTACH, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Newsgroups"), N_("Select newsgroup[s]"), evt_sel_groups, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_INDEX, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Edit headers"), N_("Edit article headers"), evt_mhdredit_dlg_create, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo msgpost_macro_menu[] =
{
  GNOMEUIINFO_END
};

GnomeUIInfo msgpost_menu[] =
{
  { GNOME_APP_UI_SUBTREE, N_("File"), NULL, msgpost_file_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Article"), NULL, msgpost_msg_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Macros"), NULL, msgpost_macro_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Help"), NULL, help_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ENDOFINFO }
};


GnomeUIInfo msgpost_toolbar[] =
{
  { GNOME_APP_UI_ITEM, N_("Post"), N_("Post article"), evt_post_ok, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_MAIL_SND, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Cancel"), N_("Cancel posting"), evt_post_cancel, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CLOSE, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Newsgroups"), N_("Select newsgroups"), evt_sel_groups, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_INDEX, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Headers"), N_("Edit article headers"), evt_mhdredit_dlg_create, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PROPERTIES, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo msgsend_msg_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("Send"), N_("Send message via SMTP"), evt_send_ok, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_MAIL_SND, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Cancel"), N_("Cancel message sending"), evt_send_cancel, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Attachments..."), N_("View attachments dialog"), evt_attachments, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ATTACH, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Edit headers"), N_("Edit message headers"), evt_mhdredit_dlg_create, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP, 0, 0, NULL },
  GNOMEUIINFO_END
};

GnomeUIInfo msgsend_menu[] =
{
  { GNOME_APP_UI_SUBTREE, N_("File"), NULL, msgpost_file_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Message"), NULL, msgsend_msg_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Macros"), NULL, msgpost_macro_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, N_("Help"), NULL, help_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ENDOFINFO }
};


GnomeUIInfo msgsend_toolbar[] =
{
  { GNOME_APP_UI_ITEM, N_("Send"), N_("Send message"), evt_send_ok, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_MAIL_SND, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, N_("Cancel"), N_("Cancel sending"), evt_send_cancel, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CLOSE, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Attachments"), N_("View message attachments"), evt_attachments, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_ATTACH, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("Headers"), N_("Edit message headers"), evt_mhdredit_dlg_create, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PROPERTIES, 0, 0, NULL },
  GNOMEUIINFO_END
};


GnomeUIInfo mw_context_menu[] =
{
  { GNOME_APP_UI_SEPARATOR },
  GNOMEUIINFO_RADIOLIST(mw_fonts_group),
  { GNOME_APP_UI_SEPARATOR },
  { GNOME_APP_UI_ITEM, N_("MIME structure"), N_("MIME structure dialog"), evt_mime_view, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ATTACH, 0, 0, NULL },
  { GNOME_APP_UI_ENDOFINFO }
};


#if 0
GnomeUIInfo view_menu[] =
{
  { GNOME_APP_UI_ITEM, N_("HTML source"), NULL, evt_app_quit, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR },
  GNOMEUIINFO_RADIOLIST(view_radio_group),
  { GNOME_APP_UI_SEPARATOR },
  GNOMEUIINFO_END
};
#endif


static void grn_menuitem_set_sensitive(gchar *path, gboolean enabled)
{
  GtkWidget *mshell, *mi;
  gint pos;

  mshell = gnome_app_find_menu_pos(GNOME_APP(GRN->window)->menubar, path, &pos);
  if ((mshell)  && (GTK_MENU_SHELL(mshell)->children))
  {
    mi = (GtkWidget *) g_list_nth_data(GTK_MENU_SHELL(mshell)->children, pos-1);
    if (mi)
    {
      grn_lock();
      gtk_widget_set_sensitive(mi, enabled);
      grn_unlock();
    }
  }
}

void grn_menuitem_show(gchar *path, gboolean visible)
{
  GtkWidget *mshell, *mi;
  gint pos;
  g_return_if_fail(path != NULL);

  mshell = gnome_app_find_menu_pos(GNOME_APP(GRN->window)->menubar, path, &pos);
  if ((mshell)  && (GTK_MENU_SHELL(mshell)->children))
  {
    mi = (GtkWidget *) g_list_nth_data(GTK_MENU_SHELL(mshell)->children, pos-1);
    if (mi)
    {
      grn_lock();
      if (visible)  gtk_widget_show(mi);
      else  gtk_widget_hide(mi);
      grn_unlock();
    }
  }
}

void grn_menuitem_check(GtkWidget *menu, gchar *path, gboolean is_active)
{
  GtkWidget *mshell, *mi;
  gint pos;
  g_return_if_fail(menu != NULL);
  g_return_if_fail(path != NULL);

  mshell = gnome_app_find_menu_pos(menu, path, &pos);
  if ((mshell)  && (GTK_MENU_SHELL(mshell)->children))
  {
    mi = (GtkWidget *) g_list_nth_data(GTK_MENU_SHELL(mshell)->children, pos-1);
    if (mi && GTK_IS_CHECK_MENU_ITEM(mi))
      GTK_CHECK_MENU_ITEM(mi)->active = is_active;
  }
  else g_warning("'%s' menu item not found", path);
}


void grn_menus_init(void)
{
  gchar *buf1 = g_strdup_printf("%s/%s/%s", _("Message"), _("View"), _("Proportional font"));
  gchar *buf2 = g_strdup_printf("%s/%s/%s", _("Message"), _("View"), _("Monospaced font"));

  grn_menuitem_check(GNOME_APP(GRN->window)->menubar, buf1, grn_prefs.mw_prop);
  grn_menuitem_check(GNOME_APP(GRN->window)->menubar, buf2, !grn_prefs.mw_prop);
  str_free(&buf1);  str_free(&buf2);

  show_newsgroups_menu(FALSE);
  show_threadlist_menu(FALSE);
  grn_menuitem_set_sensitive(_("Message"), FALSE);
}

void build_scripts_menu(void)
{
  GtkWidget *mshell, *mi = NULL, *item;
  GList *ml;
  GSList *l;
  gint pos;
  gchar *path = g_strdup_printf("%s/%s", _("Scripts"), _("Perl"));

  mshell = gnome_app_find_menu_pos(GNOME_APP(GRN->window)->menubar, path, &pos);
  g_free(path);
  if (! mshell)  return;
  ml = GTK_MENU_SHELL(mshell)->children;
  ml = ml->next;
  mi = (GtkWidget *) ml->data;
  if (! mi)  return;
  
  mi = mi->parent;
  item = gtk_menu_item_new();
  gtk_widget_show(item);
  gtk_menu_append(GTK_MENU(mi), item);
  for (l=perl_commands; l; l=l->next)
  {
    perl_command_handler *pch = l->data;
    if (! pch)  continue;

    if ((pch->context == 1) || (pch->context == 0))
    {
      item = gtk_menu_item_new_with_label(pch->command_name);
      gtk_signal_connect(GTK_OBJECT(item), "activate",
                         GTK_SIGNAL_FUNC(evt_perl_command), pch->handler_name);
      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(mi), item);
    }
  }
}

void build_macros_menu(GtkWidget *menubar, gint context)
{
  GtkWidget *mshell, *mi = NULL, *item;
  GList *ml;
  GSList *l;
  gint pos;
  gchar *path = g_strdup_printf("%s/", _("Macros"));

  mshell = gnome_app_find_menu_pos(menubar, path, &pos);
  g_free(path);
  if (! mshell)  return;
  ml = GTK_MENU_SHELL(mshell)->children;
  mi = (GtkWidget *) ml->data;
  if (! mi)  return;
  
  mi = mi->parent;
  for (l=perl_commands; l; l=l->next)
  {
    perl_command_handler *pch = l->data;
    if (! pch)  continue;

    if ((pch->context == 2) || (pch->context == context) || (pch->context == 0))
    {
      item = gtk_menu_item_new_with_label(pch->command_name);
      gtk_signal_connect(GTK_OBJECT(item), "activate",
                         GTK_SIGNAL_FUNC(evt_perl_command), pch->handler_name);
      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(mi), item);
    }
  }
}

void menu_delete_scripts(void)
{
  GtkWidget *mshell, *mi = NULL;
  GList *ml;
  gint pos;
  gchar *path = g_strdup_printf("%s/%s", _("Scripts"), _("Perl"));

  mshell = gnome_app_find_menu_pos(GNOME_APP(GRN->window)->menubar, path, &pos);
  g_free(path);
  if (! mshell)  return;
  ml = GTK_MENU_SHELL(mshell)->children;
  ml = ml->next;
  mi = (GtkWidget *) ml->data;
  if (! mi)  return;
  
  mi = mi->parent;
  ml = GTK_MENU_SHELL(mi)->children;
  ml = ml->next;  ml = ml->next;
  while (ml)
  {
    if (ml->data) 
    {
      gtk_widget_hide(GTK_WIDGET(ml->data));
//      gtk_widget_destroy(GTK_WIDGET(ml->data));
    }
    ml = ml->next;
  }
}


void enable_newsgroups_menu(gboolean visible)
{
  gchar buf[1024];

  snprintf(buf, 1000, "%s/%s", _("Message"), _("Open"));
  grn_menuitem_set_sensitive(buf, ! visible);
  snprintf(buf, 1000, "%s/%s", _("Message"), _("Reply"));
  grn_menuitem_set_sensitive(buf, ! visible);
  snprintf(buf, 1000, "%s/%s", _("Message"), _("Forward"));
  grn_menuitem_set_sensitive(buf, ! visible);
  snprintf(buf, 1000, "%s/%s", _("Message"), _("File"));
  grn_menuitem_set_sensitive(buf, ! visible);
  if (visible)
  {
    grn_menuitem_set_sensitive(_("Message"), visible);
    snprintf(buf, 1000, "%s/%s", _("Message"), _("View"));
    grn_menuitem_set_sensitive(buf, ! visible);
  }
}

void show_newsgroups_menu(gboolean visible)
{
  grn_menuitem_set_sensitive(_("Newsgroup"), visible);
  grn_menuitem_show(_("Newsgroup"), visible);
  enable_newsgroups_menu(visible);
}

void enable_threadlist_menu(gboolean visible)
{
  gchar buf[1024];

  grn_menuitem_set_sensitive(_("Threadlist"), visible);

  snprintf(buf, 1000, "%s/%s", _("Message"), _("Open"));
  grn_menuitem_set_sensitive(buf, visible);
  snprintf(buf, 1000, "%s/%s", _("Message"), _("View"));
  grn_menuitem_set_sensitive(buf, ! visible);
}

void show_threadlist_menu(gboolean visible)
{
  enable_threadlist_menu(visible);
  grn_menuitem_show(_("Threadlist"), visible);
}

void show_message_menu(gboolean visible)
{
  gchar buf[1024];

  snprintf(buf, 1000, "%s/%s", _("Message"), _("Open"));
  grn_menuitem_set_sensitive(buf, ! visible);
  snprintf(buf, 1000, "%s/%s", _("Message"), _("View"));
  grn_menuitem_set_sensitive(buf, visible);
}

void enable_findagain_menu(gboolean enabled)
{
  gchar buf[1024];
  
  snprintf(buf, 1000, "%s/%s", _("Threadlist"), _("Search again"));
  grn_menuitem_set_sensitive(buf, enabled);
}


void grn_menu_set_data(GtkWidget *menu, gchar *base, gchar *path,
                       const gchar *key, gpointer data)
{
  gchar *s = g_strconcat(base, "/", path, NULL);
  gint pos;
  GtkWidget *w = gnome_app_find_menu_pos(menu, s, &pos);
  str_free(&s);
  if (w)  gtk_object_set_data(GTK_OBJECT(w), key, data);
}
