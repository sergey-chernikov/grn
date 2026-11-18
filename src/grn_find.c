/*
 * grn_find.c - "Find" dialog
 *
 * $Id: grn_find.c,v 1.6 2000/06/28 11:28:28 sc Exp $
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

#include <sys/types.h>
#include <regex.h>
#include "grn_consts.h"
#include "grn_vars.h"
#include "grn_find.h"
#include "grn_util.h"
#include "grn_config.h"
#include "grn_threadlist.h"
#include "grn_misc.h"
#include "grn_menu.h"
#include "nntp.h"


typedef struct grn_find_data
{
  GtkWidget *en_data;
  GtkWidget *en_hdr;
  GtkWidget *cb_from;
  GtkWidget *cb_subj;
  GtkWidget *cb_body;
  GtkWidget *cb_hdr;
  GtkWidget *cb_regex;
  GtkWidget *cb_csens;
  GtkWidget *cb_unread;
  GtkWidget **dlg;
} grn_find_data;


static void hdr_toggled(GtkWidget *w, GtkWidget *en)
{
  gtk_widget_set_sensitive(en, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}
static GtkWidget *grn_find_create(grn_find_data *gfd)
{
  GtkWidget *vbox, *table;
  
  grn_lock();
  vbox = gtk_vbox_new(FALSE, 0);
  table = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
  
  gfd->en_data = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), gfd->en_data, 0, 2, 0, 1,
    GTK_EXPAND|GTK_FILL, GTK_FILL, 2, 2);

  gfd->cb_from = gtk_check_button_new_with_label(_("'From' header"));
  gtk_table_attach(GTK_TABLE(table), gfd->cb_from, 0, 1, 2, 3,
    GTK_EXPAND|GTK_FILL, GTK_FILL, 2, 2);
  gfd->cb_subj = gtk_check_button_new_with_label(_("'Subject' header"));
  gtk_table_attach(GTK_TABLE(table), gfd->cb_subj, 0, 1, 3, 4,
    GTK_EXPAND|GTK_FILL, GTK_FILL, 2, 2);
  gfd->cb_body = gtk_check_button_new_with_label(_("Message body"));
  gtk_table_attach(GTK_TABLE(table), gfd->cb_body, 0, 1, 4, 5,
    GTK_EXPAND|GTK_FILL, GTK_FILL, 2, 2);
  gfd->cb_hdr = gtk_check_button_new_with_label(_("Other header:"));
  gtk_table_attach(GTK_TABLE(table), gfd->cb_hdr, 0, 1, 5, 6,
    GTK_EXPAND|GTK_FILL, GTK_FILL, 2, 2);

  gfd->en_hdr = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), gfd->en_hdr, 1, 2, 5, 6,
    GTK_EXPAND|GTK_FILL, GTK_FILL, 2, 2);
  gtk_signal_connect(GTK_OBJECT(gfd->cb_hdr), "toggled",
                     GTK_SIGNAL_FUNC(hdr_toggled), gfd->en_hdr);

  gfd->cb_regex = gtk_check_button_new_with_label(_("Regular expression"));
  gtk_table_attach(GTK_TABLE(table), gfd->cb_regex, 1, 2, 2, 3,
    GTK_EXPAND|GTK_FILL, GTK_FILL, 2, 2);
  gfd->cb_csens = gtk_check_button_new_with_label(_("Case sensitive"));
  gtk_table_attach(GTK_TABLE(table), gfd->cb_csens, 1, 2, 3, 4,
    GTK_EXPAND|GTK_FILL, GTK_FILL, 2, 2);
  gfd->cb_unread = gtk_check_button_new_with_label(_("Unread only"));
  gtk_table_attach(GTK_TABLE(table), gfd->cb_unread, 1, 2, 4, 5,
    GTK_EXPAND|GTK_FILL, GTK_FILL, 2, 2);

  gtk_object_set_data(GTK_OBJECT(vbox), KEY_DATA, gfd);
  grn_unlock();
  return vbox;
}

void grn_find_set_data(GtkWidget *gfw, grn_find *gf)
{
  grn_find_data *gfd = NULL;
  g_return_if_fail(gfw != NULL);
  g_return_if_fail(gfw != NULL);
  
  gfd = gtk_object_get_data(GTK_OBJECT(gfw), KEY_DATA);
  if (! gfd)  return;
  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gfd->cb_from), gf->search_from);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gfd->cb_subj), gf->search_subj);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gfd->cb_body), gf->search_body);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gfd->cb_hdr), gf->search_hdr);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gfd->cb_regex), gf->is_regex);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gfd->cb_csens), gf->sensitive);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gfd->cb_unread), gf->unread_only);
  if (str_check(gf->data))  gtk_entry_set_text(GTK_ENTRY(gfd->en_data), gf->data);
  if (str_check(gf->hdr))  gtk_entry_set_text(GTK_ENTRY(gfd->en_hdr), gf->hdr);
  gtk_widget_set_sensitive(gfd->en_hdr, gf->search_hdr);
}

void grn_find_get_data(GtkWidget *gfw, grn_find *gf)
{
  grn_find_data *gfd = NULL;
  gchar *s;
  g_return_if_fail(gfw != NULL);
  g_return_if_fail(gfw != NULL);
  
  gfd = gtk_object_get_data(GTK_OBJECT(gfw), KEY_DATA);
  if (! gfd)  return;
  
  gf->search_from = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfd->cb_from));
  gf->search_subj = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfd->cb_subj));
  gf->search_body = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfd->cb_body));
  gf->search_hdr = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfd->cb_hdr));
  gf->is_regex = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfd->cb_regex));
  gf->sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfd->cb_csens));
  gf->unread_only = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfd->cb_unread));
  str_free(&(gf->data));  str_free(&(gf->hdr));
  s = gtk_entry_get_text(GTK_ENTRY(gfd->en_data));
  if (str_check(s))  gf->data = g_strdup(s);
  s = gtk_entry_get_text(GTK_ENTRY(gfd->en_hdr));
  if (str_check(s))  gf->hdr = g_strdup(s);
}


static gboolean find_dlg_delete(GtkWidget *w)
{
  grn_save_ws(6, w);
  return FALSE;
}

static void find_dlg_destroy(GtkWidget *w, grn_find_data *gfd)
{
  *(gfd->dlg) = NULL;
  g_free(gfd);
}

static void find_dlg_ok(GtkWidget *w, GtkWidget *dlg);
static void find_dlg_cancel(GtkWidget *w, GtkWidget *dlg)
{
  gtk_widget_destroy(dlg);
}
GtkWidget *grn_find_dlg_create(grn_find *gf)
{
  GtkWidget *dlg, *gfw;
  grn_find_data *gfd;
  g_return_val_if_fail(gf != NULL, NULL);
  
  gfd = g_new(grn_find_data, 1);
  
  grn_lock();
  dlg = gnome_dialog_new(_("Find message"), GNOME_STOCK_BUTTON_CANCEL,
                         GNOME_STOCK_BUTTON_OK, NULL);
  gtk_widget_set_uposition(dlg, grn_prefs.ws[6].x, grn_prefs.ws[6].y);
  gnome_dialog_button_connect(GNOME_DIALOG(dlg), 0,
                              GTK_SIGNAL_FUNC(find_dlg_cancel), dlg);
  gnome_dialog_button_connect(GNOME_DIALOG(dlg), 1,
                              GTK_SIGNAL_FUNC(find_dlg_ok), dlg);
  gtk_signal_connect(GTK_OBJECT(dlg), "delete_event",
		     GTK_SIGNAL_FUNC(find_dlg_delete), NULL);
  grn_unlock();
  gfw = grn_find_create(gfd);
  grn_lock();
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg)->vbox), gfw, TRUE, TRUE, 0);
  gtk_widget_show_all(dlg);
  gtk_object_set_data(GTK_OBJECT(dlg), KEY_WND, gfw);
  gnome_dialog_editable_enters(GNOME_DIALOG(dlg), GTK_EDITABLE(gfd->en_data));
  gnome_dialog_editable_enters(GNOME_DIALOG(dlg), GTK_EDITABLE(gfd->en_hdr));
  grn_unlock();
  grn_find_set_data(gfw, gf);
  return dlg;
}

void evt_find_dlg(GtkWidget *w)
{
  static GtkWidget *dlg = NULL;
  grn_find_data *gfd = NULL;
  GtkWidget *gfw;
  
  if (dlg)  return;
  dlg = grn_find_dlg_create(GRN->find);

  gfw = gtk_object_get_data(GTK_OBJECT(dlg), KEY_WND);
  if (gfw)  gfd = gtk_object_get_data(GTK_OBJECT(gfw), KEY_DATA);
  if (! gfd)  return;
  gtk_widget_grab_focus(gfd->en_data);
  gfd->dlg = &dlg;
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     GTK_SIGNAL_FUNC(find_dlg_destroy), gfd);
}


static gboolean search_str(gchar *s, grn_find *gf)
{
  gboolean ret = FALSE;
  if (! str_check(s))  return FALSE;

  if (gf->is_regex)
  {
    regex_t regex;
    regmatch_t regmatch;
    int cflags = REG_EXTENDED | REG_NOSUB;
    
    if (! gf->sensitive)  cflags |= REG_ICASE;
    if (regcomp(&regex, gf->data, cflags))
    {
      grn_error(_("Searching"), _("Can't compile regular expression"));
      return FALSE;
    }
    if (! regexec(&regex, s, 0, &regmatch, 0))  ret = TRUE;
    regfree(&regex);
  }
  else {
    gchar *pattern = g_strdup_printf("*%s*", gf->data);
    int flags = 0;
    
    if (! gf->sensitive)  flags = FNM_CASEFOLD;
    if (! fnmatch(pattern, s, flags))  ret = TRUE;
    str_free(&pattern);
  }
  return ret;
}

static t_message *search_recursive(t_message *msg, grn_find *gf, t_message *cur)
{
  if (gf->unread_only && t_message_is_read(msg)) ;
  else if (msg == cur) ;
  else if (msg->mh) {
    if ((gf->search_body || gf->search_hdr) && (! msg->cached))
    {
      grn_appbar_push(GRN->appbar, TRUE, _("Searching in message #%d"), msg->mh->number);
      t_msgheaders_rm_extra_hdrs(msg->mh);
      nntp_download_article(msg);
      grn_appbar_pop(GRN->appbar);
    }
    if (gf->search_from && search_str(msg->mh->from, gf))  return msg;
    if (gf->search_subj && search_str(msg->mh->subject, gf))  return msg;
    if (gf->search_hdr)
    {
      gchar *s = t_msgheaders_get_hdr(msg->mh, gf->hdr);
      if (search_str(s, gf))  return msg;
    }
    if (gf->search_body && search_str(msg->body, gf))  return msg;
  }
  
  if (msg->threads)
  {
    GList *l;
    for (l=msg->threads; l; l=l->next)
    {
      t_message *ret = NULL;
      t_message *m = (t_message *) l->data;
      if (! m)  continue;
      ret = search_recursive(m, gf, cur);
      if (ret)  return ret;
    }
  }
  return NULL;
}

static gboolean do_search(GtkCTree *ctree, grn_find *gf)
{
  t_message *msg, *found = NULL, *m_root;
  GList *msglist, *ml, *l_start;
  
  msg = tl_get_selected_message(ctree);
  if (! msg)  return FALSE;
  m_root = tl_get_root_message(ctree, msg);
  msglist = gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_MSGLIST);
  if (! msglist)  return FALSE;
  l_start = msglist;
  for (ml = msglist; ml; ml = ml->next)
  {
    t_message *m1 = (t_message *) ml->data;
    if (! m1)  continue;
    if ((! m_root) && (msg == m1))
    {
      l_start = ml;  break;
    }
    if (m1 == m_root)
    {
      l_start = ml;  break;
    }
  }
  for (ml = l_start; ml; ml = ml->next)
  {
    t_message *m1 = (t_message *) ml->data;
    if (! m1)  continue;

    found = search_recursive(m1, gf, msg);
    if (found)
    {
      tl_select_message(ctree, msg, found);
      break;
    }
  }
  if (found)  return TRUE;
  else  return FALSE;
}

static void find_dlg_ok(GtkWidget *w, GtkWidget *dlg)
{
  GtkWidget *gfw;
  grn_find_data *gfd = NULL;
  
  gfw = gtk_object_get_data(GTK_OBJECT(dlg), KEY_WND);
  if (gfw)  gfd = gtk_object_get_data(GTK_OBJECT(gfw), KEY_DATA);
  if (! gfd)  return;
  grn_find_get_data(gfw, GRN->find);
  gtk_widget_destroy(dlg);
  enable_findagain_menu(TRUE);
  evt_find_again(w);
}


void evt_find_again(GtkWidget *w)
{
  GtkCTree *ctree = GTK_CTREE(gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_CT_MSGS));
  g_assert(ctree != NULL);
  g_assert(GRN->find != NULL);
  if ((! do_search(ctree, GRN->find)) && (grn_prefs.cfms[6]))
    grn_info(_("Search string not found"));
}
