/*
 * grn_commands.c
 *
 * $Id: grn_commands.c,v 1.30 2000/06/28 11:28:28 sc Exp $
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
#include "grn_commands.h"
#include "grn_vars.h"
#include "grn_config.h"
#include "grn_menu.h"
#include "grn_news.h"
#include "grn_util.h"
#include "grn_nglist.h"
#include "grn_threadlist.h"
#include "queue.h"
#include "nntp.h"
#include "grn_perl.h"
#include "tcp.h"
#include "grn_misc.h"


static void grn_done(void)
{
  grn_find_save(GRN->find);
  grn_find_free(&(GRN->find));
  grn_prefs_save();
  grn_prefs_done();
  gnome_config_pop_prefix();
}

gint evt_app_quit(GtkWidget *widget)
{
  evt_cleanup(widget, NULL);
  return TRUE;
}

gint evt_cleanup(GtkWidget *widget, gpointer data)
{
  nntp_disconnect(GRN->nntp_sock);
  if (GRN->nntp_sock->error)
    grn_error(_("Socket error"), GRN->nntp_sock->err_msg);
  grn_socket_free(&(GRN->nntp_sock));

  write_newsrc(grn_prefs.paths[0], grn_prefs.paths[0], newsrc_ht);

  grn_save_ws(0, GRN->window);
  grn_done();
  gtk_main_quit();
  return TRUE;
}

void evt_not_impl(GtkWidget *widget)
{
  grn_lock();
  gnome_ok_dialog(_("Not implemented yet"));
  grn_unlock();
}


extern pthread_mutex_t groups_m;
void evt_grouplist(GtkWidget *w)
{
  if (flags.open_grouplist)  return;
  pthread_mutex_lock(&groups_m);
  if (! GRN->grouplist)  GRN->grouplist = grouplist_create();
  grn_set_contents(GRN->grouplist);
  if (! GRN->ng_list)
  {
    grnq_add_download_grouplist();
    grnq_add_draw_grouplist();
  }
  pthread_mutex_unlock(&groups_m);
}


void evt_perl_restart(GtkWidget *w)
{
  menu_delete_scripts();
  grn_perl_restart();
  build_scripts_menu();
}


static void perl_list_destroy(GtkWidget *w, GtkWidget **dlg)
{
  *dlg = NULL;
}
static void perl_list_ok(GtkWidget *w, GtkWidget *dlg)
{
  gtk_widget_destroy(dlg);
}

void evt_perl_list(GtkWidget *w)
{
  gchar *titles[] = {_("type"), _("name"), _("handler")};
  GSList *rows, *l;
  gint i;
  static GtkWidget *dlg = NULL;
  GtkWidget *clist, *scrwin;
  
  if (dlg)  return;

  dlg = gnome_dialog_new(_("Perl scripts list"), GNOME_STOCK_BUTTON_CLOSE, NULL);
  gnome_dialog_set_parent(GNOME_DIALOG(dlg), GTK_WINDOW(GRN->window));
  gnome_dialog_button_connect(GNOME_DIALOG(dlg), 0,
                              GTK_SIGNAL_FUNC(perl_list_ok), dlg);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     GTK_SIGNAL_FUNC(perl_list_destroy), &dlg);

  clist = gtk_clist_new_with_titles(3, titles);
  gtk_widget_set_usize(clist, 450, 200);
  gtk_clist_column_titles_passive(GTK_CLIST(clist));
  gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_BROWSE);
  scrwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrwin), clist);
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg)->vbox), scrwin, TRUE, TRUE, 3);

  rows = grn_perl_list();
  for (l=rows; l; l=l->next)
  {
    gchar **row = l->data;
    gtk_clist_append(GTK_CLIST(clist), row);
    for (i=0; i<3; i++)
      if (row[i])  g_free(row[i]);
    g_free(row);
  }
  g_slist_free(rows);
  gtk_clist_columns_autosize(GTK_CLIST(clist));
  gtk_widget_show_all(dlg);
}

static void plfs_destroy(GtkWidget *w, GtkWidget **dlg)
{
  *dlg = NULL;
}
static void plfs_cancel(GtkWidget *w, GtkWidget *dlg)
{
  gtk_widget_destroy(dlg);
}
static void plfs_ok(GtkWidget *w, GtkWidget *dlg)
{
  gchar *fn = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(dlg)));
  if (fn)  perl_load_file(fn);
  str_free(&fn);
  gtk_widget_destroy(dlg);
}
void evt_perl_load(GtkWidget *w)
{
  static GtkWidget *dlg = NULL;
  
  if (dlg)  return;
  dlg = gtk_file_selection_new(_("Open Perl script"));
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     GTK_SIGNAL_FUNC(plfs_destroy), &dlg);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(dlg)->ok_button),
                     "clicked", GTK_SIGNAL_FUNC(plfs_ok), dlg);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(dlg)->cancel_button),
                     "clicked", GTK_SIGNAL_FUNC(plfs_cancel), dlg);
  gtk_widget_show_all(dlg);
}

void evt_perl_command(GtkWidget *w, gpointer data)
{
  gchar *cmd_name = (gchar *) data;
  
  if (cmd_name)  perl_command_run(cmd_name, GRN, NULL);
}


void evt_save_prefs(GtkWidget *w)
{
  grn_prefs_save();
}

void evt_prev(GtkWidget *w)
{
  GtkObject *o = gtk_object_get_data(GTK_OBJECT(GRN->window), KEY_FOCUS);
  GdkEventKey *event;

  if (! o)  return;
  event = g_new(GdkEventKey, 1);
  event->keyval = GDK_Left;  event->state = 0;
  gtk_signal_emit_by_name(o, "key_press_event", event, event);
}

void evt_next(GtkWidget *w)
{
  GtkObject *o = gtk_object_get_data(GTK_OBJECT(GRN->window), KEY_FOCUS);
  GdkEventKey *event;

  if (! o)  return;
  event = g_new(GdkEventKey, 1);
  event->keyval = GDK_Right;  event->state = 0;
  gtk_signal_emit_by_name(o, "key_press_event", event, event);
}


int evt_save_state(GnomeClient *client, gint phase, GnomeRestartStyle save_style,
    gint shutdown, GnomeInteractStyle istyle, gint fast, gpointer data)
{
  return FALSE;
}

void evt_appbar_destroy(GtkWidget *w)
{  GRN->appbar = NULL;  }
