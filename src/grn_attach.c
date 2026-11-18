/*
 * grn_attach.c - attaching a file
 *
 *
 * $Id: grn_attach.c,v 1.2 2000/07/19 06:26:21 sc Exp $
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
#include "mutt.h"
#include "mime.h"
#include "grn_types.h"
#include "grn_attach.h"
#include "grn_mime.h"
#include "grn_util.h"
#include "grn_config.h"


static void mtype2body(const gchar *mtype, BODY *b)
{
  gchar **arr = g_strsplit(mtype, "/", 2);
  if (str_check(arr[0]))
  {
    if (! g_strcasecmp(arr[0], "audio"))  b->type = TYPEAUDIO;
    else if (! g_strcasecmp(arr[0], "application"))  b->type = TYPEAPPLICATION;
    else if (! g_strcasecmp(arr[0], "image"))  b->type = TYPEIMAGE;
    else if (! g_strcasecmp(arr[0], "message"))  b->type = TYPEMESSAGE;
    else if (! g_strcasecmp(arr[0], "model"))  b->type = TYPEMODEL;
    else if (! g_strcasecmp(arr[0], "multipart"))  b->type = TYPEMULTIPART;
    else if (! g_strcasecmp(arr[0], "text"))  b->type = TYPETEXT;
    else if (! g_strcasecmp(arr[0], "video"))  b->type = TYPEVIDEO;
    else {
      b->type = TYPEOTHER;
      b->xtype = g_strdup(arr[0]);
    }
  }
  b->subtype = str_copy(arr[1]);
  if (arr)  g_strfreev(arr);
}


static void aw_list_refresh(GtkCList *clist, t_message *m)
{
  GSList *l;

  gtk_clist_freeze(clist);
  gtk_clist_clear(clist);
  for (l=m->attachments; l; l=l->next)
  {
    BODY *b = (BODY *) l->data;
    gchar *row[3];
    gint idx;

    if (! b)  continue;
    row[0] = str_copy(b->filename);
    row[1] = body_mtype(b);
    row[2] = menc2str(b);
    idx = gtk_clist_append(clist, row);
    gtk_clist_set_row_data(clist, idx, b);
    for (idx=0; idx<3; idx++)  str_free(&row[idx]); 
  }
  gtk_clist_columns_autosize(clist);
  gtk_clist_thaw(clist);
}

static void aw_sel(GtkCList *clist, gint row, gint col, GdkEventButton *ev,
                   GtkWidget *box)
{
  GtkWidget *btn_del = gtk_object_get_data(GTK_OBJECT(box), KEY_BTN_DEL);

  if (btn_del)  gtk_widget_set_sensitive(btn_del, TRUE);
  if (! clist->selection)  gtk_widget_set_sensitive(btn_del, FALSE);
}

static void aw_fs_ok(GtkWidget *btn, GtkFileSelection *fs)
{
  gchar *fn = gtk_file_selection_get_filename(fs);
  GtkWidget *box = gtk_object_get_data(GTK_OBJECT(fs), KEY_WND);
  GtkCList *clist = gtk_object_get_data(GTK_OBJECT(box), KEY_CLIST);
  t_message *m = gtk_object_get_data(GTK_OBJECT(box), KEY_MSG);

  if (str_check(fn) && box && clist && m)
  {
    BODY *b = mutt_make_file_attach(fn);
    mtype2body(gnome_mime_type_of_file(fn), b);
    m->attachments = g_slist_append(m->attachments, b);
    aw_list_refresh(clist, m);
  }
  gtk_widget_destroy(GTK_WIDGET(fs));
}
static void aw_fs_cancel(GtkWidget *btn, GtkFileSelection *fs)
{
  gtk_widget_destroy(GTK_WIDGET(fs));
}
static void aw_add(GtkWidget *w, GtkWidget *box)
{
  GtkFileSelection *fs;
  
  fs = GTK_FILE_SELECTION(gtk_file_selection_new(_("Attach file")));
  gtk_object_set_data(GTK_OBJECT(fs), KEY_WND, box);
  gtk_signal_connect(GTK_OBJECT(fs->ok_button), "clicked",
		     GTK_SIGNAL_FUNC(aw_fs_ok), fs);
  gtk_signal_connect(GTK_OBJECT(fs->cancel_button), "clicked",
		     GTK_SIGNAL_FUNC(aw_fs_cancel), fs);
  gtk_widget_show(GTK_WIDGET(fs));
}

static void aw_del(GtkWidget *w, GtkWidget *box)
{
  GtkWidget *btn_del = gtk_object_get_data(GTK_OBJECT(box), KEY_BTN_DEL);
  GtkCList *clist = gtk_object_get_data(GTK_OBJECT(box), KEY_CLIST);
  t_message *m = gtk_object_get_data(GTK_OBJECT(box), KEY_MSG);
  GList *l;

  if ((! clist) || (! m))  return;
  if (btn_del)  gtk_widget_set_sensitive(btn_del, FALSE);

  for (l=clist->selection; l; l=l->next)
  {
    gint row = GPOINTER_TO_INT(l->data);
    BODY *b = gtk_clist_get_row_data(clist, row);
    if (! b)  continue;

    m->attachments = g_slist_remove(m->attachments, b);
    mutt_free_body(&b);
  }
  aw_list_refresh(clist, m);
}

static GtkWidget *aw_create(void)
{
  gchar *ttl[3] = {_("File name"), _("Type"), _("Encoding")};
  GtkWidget *box, *scrwin, *bbox, *btn;
  GtkCList *clist;
  
  box = gtk_hbox_new(FALSE, FALSE);
  clist = GTK_CLIST(gtk_clist_new_with_titles(3, ttl));
  gtk_widget_set_usize(GTK_WIDGET(clist), 300, 200);
  gtk_clist_column_titles_passive(clist);
  gtk_clist_set_selection_mode(clist, GTK_SELECTION_EXTENDED);
  gtk_signal_connect(GTK_OBJECT(clist), "select_row",
                     GTK_SIGNAL_FUNC(aw_sel), box);
  
  scrwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrwin), GTK_WIDGET(clist));
  gtk_box_pack_start(GTK_BOX(box), scrwin, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(box), KEY_CLIST, clist);
  
  bbox = gtk_vbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), gnome_preferences_get_button_layout());
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), GNOME_PAD);
  gtk_box_pack_start(GTK_BOX(box), bbox, FALSE, FALSE, 2);
  
  btn = gnome_pixmap_button(gnome_stock_pixmap_widget(NULL, GNOME_STOCK_PIXMAP_ADD),
                            _("Add..."));
  gtk_box_pack_start(GTK_BOX(bbox), btn, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(btn), "clicked",
		     GTK_SIGNAL_FUNC(aw_add), box);
  
  btn = gnome_pixmap_button(gnome_stock_pixmap_widget(NULL, GNOME_STOCK_PIXMAP_REMOVE),
                            _("Delete"));
  gtk_box_pack_start(GTK_BOX(bbox), btn, TRUE, TRUE, 0);
  gtk_widget_set_sensitive(btn, FALSE);
  gtk_signal_connect(GTK_OBJECT(btn), "clicked",
		     GTK_SIGNAL_FUNC(aw_del), box);
  gtk_object_set_data(GTK_OBJECT(box), KEY_BTN_DEL, btn);
  
  return box;
}

static void aw_set_data(GtkWidget *aw, t_message *m)
{
  GtkCList *clist;
  g_return_if_fail(m != NULL);
  
  gtk_object_set_data(GTK_OBJECT(aw), KEY_MSG, m);
  clist = GTK_CLIST(gtk_object_get_data(GTK_OBJECT(aw), KEY_CLIST));
  if (! clist)  return;

  aw_list_refresh(clist, m);
}


static gboolean adlg_delete(GtkWidget *w)
{
  grn_save_ws(8, w);
  return FALSE;
}
static void adlg_btn_event(GtkWidget *btn, GtkWidget *w)
{
  grn_save_ws(8, w);
}
GtkWidget *dlg_attachments(t_message *m)
{
  GtkWidget *dlg, *aw;
  
  dlg = gnome_dialog_new(_("Attachments"), GNOME_STOCK_BUTTON_CLOSE, NULL);
  gnome_dialog_set_close(GNOME_DIALOG(dlg), TRUE);
  gtk_window_set_policy(GTK_WINDOW(dlg), FALSE, TRUE, FALSE);
  gtk_window_set_default_size(GTK_WINDOW(dlg), grn_prefs.ws[8].width, grn_prefs.ws[8].height);
  gtk_widget_set_uposition(dlg, grn_prefs.ws[8].x, grn_prefs.ws[8].y);
  gtk_signal_connect(GTK_OBJECT(dlg), "delete_event",
		     GTK_SIGNAL_FUNC(adlg_delete), NULL);
  gnome_dialog_button_connect(GNOME_DIALOG(dlg), 0,
			      GTK_SIGNAL_FUNC(adlg_btn_event), dlg);
  
  aw = aw_create();
  aw_set_data(aw, m);
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg)->vbox), aw, TRUE, TRUE, 0);
  gtk_widget_show_all(dlg);
  return dlg;
}

typedef struct att_data
{
  GtkWidget **dlg;
  GtkWidget *btn;
} att_data;

static void adlg_destroy(GtkWidget *w, att_data *ad)
{
  *(ad->dlg) = NULL;
  gtk_widget_set_sensitive(ad->btn, TRUE);
  g_free(ad);
}
void evt_attachments(GtkWidget *w)
{
  GtkWidget *mpw;
  t_message *m;
  static GtkWidget *dlg;
  att_data *ad;
  if (! w->parent)  return;
  if (dlg)  return;
  
  mpw = gtk_object_get_data(GTK_OBJECT(w->parent), KEY_MSGPOST);
  if (! mpw)  mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_MSGPOST);
  if (! mpw)  return;
  m = gtk_object_get_data(GTK_OBJECT(mpw), KEY_MSG);
  if (! m)  return;

  dlg = dlg_attachments(m);
  gtk_widget_set_sensitive(w, FALSE);
  ad = g_new(att_data, 1);
  ad->dlg = &dlg;  ad->btn = w;
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
		     GTK_SIGNAL_FUNC(adlg_destroy), ad);
}
