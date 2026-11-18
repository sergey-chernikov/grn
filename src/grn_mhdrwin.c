/*
 * grn_mhdrwin.c: message header window functions
 *
 * $Id: grn_mhdrwin.c,v 1.11 2000/06/28 11:28:28 sc Exp $
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
#include <gnome.h>
#include "grn_vars.h"
#include "grn_util.h"
#include "grn_misc.h"
#include "grn_config.h"
#include "grn_msgwin.h"


void mhdredit_refresh(GtkWidget *, t_msgheaders *);


static void evt_mhw_destroy(GtkWidget *w, gpointer data)
{
  gtk_object_remove_data(GTK_OBJECT(GRN->window), KEY_MHWIN);
}

static gboolean evt_mhw_delete(GtkWidget *w)
{
  grn_save_ws(1, w);
  return FALSE;
}

static void scroll_line(GtkAdjustment *adj, gint offset)
{
  gfloat new_val = adj->value + offset * adj->step_increment;
  if (new_val < adj->lower)  new_val = adj->lower;
  if (new_val > (adj->upper - adj->page_size))
    new_val = adj->upper - adj->page_size;
  gtk_adjustment_set_value(adj, new_val);
}

static gint evt_mhw_keypress(GtkWidget *w, GdkEventKey *event, gpointer data)
{
  GtkWidget *scrwin;
  GtkAdjustment *vadj = NULL;
  gint offset = 0;
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(w != NULL, FALSE);
  
  scrwin = gtk_object_get_data(GTK_OBJECT(w), KEY_MHWIN_SW);
  if (scrwin)  vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrwin));

  switch (event->keyval)
  {
    case GDK_Escape:
      if (! evt_mhw_delete(w))  gtk_widget_destroy(w);
      break;
    case GDK_Up:
      offset = -1;
      break;
    case GDK_Down:
      offset = 1;
      break;
    default:  return FALSE;
  }
  gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "key_press_event");
  if (vadj && offset)  scroll_line(vadj, offset);
  return TRUE;
}

void mhwin_create()
{
  GtkWindow *wnd=NULL;
  GtkWidget *scrwin;
  gpointer tmp;
  
  grn_lock();
  tmp = gtk_object_get_data(GTK_OBJECT(GRN->window), KEY_MHWIN);
  grn_unlock();
  if (tmp)  return;

  grn_lock();
  wnd = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  gtk_window_set_default_size(wnd, grn_prefs.ws[1].width, grn_prefs.ws[1].height);
  gtk_widget_set_uposition(GTK_WIDGET(wnd), grn_prefs.ws[1].x, grn_prefs.ws[1].y);
  grn_unlock();
  grn_set_title(wnd, _("Message headers"));
  
  grn_lock();
  scrwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(wnd), scrwin);

  gtk_signal_connect(GTK_OBJECT(wnd), "delete_event",
                     GTK_SIGNAL_FUNC(evt_mhw_delete), NULL);
  gtk_signal_connect(GTK_OBJECT(wnd), "destroy",
                     GTK_SIGNAL_FUNC(evt_mhw_destroy), NULL);
  gtk_signal_connect(GTK_OBJECT(wnd), "key_press_event",
                     GTK_SIGNAL_FUNC(evt_mhw_keypress), NULL);
  gtk_object_set_data(GTK_OBJECT(wnd), KEY_MHWIN_SW, scrwin);
  gtk_object_set_data(GTK_OBJECT(wnd), KEY_MHWIN_TBL, NULL);
  gtk_object_set_data(GTK_OBJECT(GRN->window), KEY_MHWIN, wnd);
  gtk_widget_show_all(GTK_WIDGET(wnd));
  grn_unlock();
}

static void mhwin_add_header(GtkWidget *table, gint pos, gchar *name, gchar *value)
{
  GtkWidget *label, *en;
  
  if (! value)  return;
  if (! name)  return;
  grn_lock();
  label = gtk_label_new(name);
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, pos, pos + 1,
    GTK_FILL, GTK_FILL, 2, 3);

  if (strchr(value, '\n'))
  {
    en = gtk_text_new(NULL, NULL);
    gtk_text_set_editable(GTK_TEXT(en), FALSE);
    gtk_text_set_line_wrap(GTK_TEXT(en), FALSE);
    gtk_text_insert(GTK_TEXT(en), NULL, NULL, NULL, value, strlen(value));
    gtk_text_set_point(GTK_TEXT(en), 0);
  }
  else {
    en = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(en), FALSE);
    gtk_entry_set_text(GTK_ENTRY(en), value);
    gtk_entry_set_position(GTK_ENTRY(en), 0);
  }
  gtk_table_attach(GTK_TABLE(table), en, 1,2, pos, pos + 1,
    GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 2, 3);
  grn_unlock();
}

static gint compare_mhdrs(t_msgheader *mh1, t_msgheader *mh2)
{
  if (! mh2)  return (mh1 != NULL);
  if (! mh1)  return (-1);
  return strcmp(mh1->name, mh2->name);
}

static void add_hdr(gchar *key, gchar *data, GList **l)
{
  t_msgheader *mhdr = g_new(t_msgheader, 1);
  mhdr->name = key;  mhdr->value = data;
  *l = g_list_insert_sorted(*l, mhdr, (GCompareFunc) compare_mhdrs);
}

static void mhwin_add_extra_hdrs(GtkWidget *table, gint pos, GHashTable *eh)
{
  GList *mh_list = NULL, *l;
  gint row = pos;
  if (! eh)  return;
  g_hash_table_foreach(eh, (GHFunc) add_hdr, &mh_list);
  for (l=mh_list; l; l=l->next)
  {
    t_msgheader *mhdr = l->data;
    if (! mhdr)  continue;
    if (str_check(mhdr->name) && str_check(mhdr->value))
    {
      gchar *buf = g_strconcat(mhdr->name, ":", NULL);
      mhwin_add_header(table, row++, buf, mhdr->value);
      str_free(&buf);
    }
    g_free(mhdr);
  }
  g_list_free(mh_list);
}

void mhdrwin_refresh(t_msgheaders *mh)
{
  GtkWindow *mhwin;
  GtkWidget *table, *scrwin;

  mhwin = GTK_WINDOW(gtk_object_get_data(GTK_OBJECT(GRN->window), KEY_MHWIN));
  if (! mhwin)  return;
  grn_lock();
  table = gtk_object_get_data(GTK_OBJECT(mhwin), KEY_MHWIN_TBL);
  scrwin = gtk_object_get_data(GTK_OBJECT(mhwin), KEY_MHWIN_SW);
  grn_unlock();
  if (! scrwin)  return;
  if (table)
  {
    grn_lock();
    gtk_widget_destroy(table);
    grn_unlock();
  }

  if (mh)
  {
    gchar buf[100];

    if (mh->xref)  grn_set_title(mhwin, _("Headers for '%s'"), mh->xref);
    else  grn_set_title(mhwin, _("Headers for msg #%ld in %s"), mh->number, mh->grp->name);
    grn_lock();
    table = gtk_table_new(2, 2, FALSE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrwin), table);
    grn_unlock();
    mhwin_add_header(table, 0, "From:", mh->from);
    mhwin_add_header(table, 1, "Date:", mh->date);
    mhwin_add_header(table, 2, "Subject:", mh->subject);
    mhwin_add_header(table, 3, "Newsgroups:", mh->newsgroups);
    mhwin_add_header(table, 4, "Message-ID:", mh->id);
    mhwin_add_header(table, 5, "References:", mh->ref);
    mhwin_add_header(table, 6, "Path:", mh->path);
    snprintf(buf, 99, "%ld", mh->lines);
    mhwin_add_header(table, 7, "Lines:", buf);
    mhwin_add_header(table, 8, "Xref:", mh->xref);
    grn_lock();
    gtk_table_attach(GTK_TABLE(table), gtk_hseparator_new(), 0,2,9,10, GTK_FILL, GTK_FILL, 2,3);
    grn_unlock();
    mhwin_add_extra_hdrs(table, 10, mh->extra_hdrs);
    grn_lock();
    gtk_object_set_data(GTK_OBJECT(mhwin), KEY_MHWIN_TBL, table);
    gtk_widget_show_all(table);
    grn_unlock();
  }
  else {
    grn_set_title(mhwin, _("Message headers"));
    grn_lock();
    gtk_object_set_data(GTK_OBJECT(mhwin), KEY_MHWIN_TBL, NULL);
    grn_unlock();
  }
}


void evt_mhwin_show(GtkWidget *w)
{
  t_message *msg;
  g_return_if_fail(w != NULL);
  
  mhwin_create();
  if (! GRN->msgwin)  return;
  grn_lock();
  msg = (t_message *) gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_MSG);
  grn_unlock();
  if (! msg)  return;
  mhdrwin_refresh(msg->mh);
}



static void hdr_sel(GtkCList *clist, gint row, gint col, GdkEventButton *ev, GtkWidget *mhe)
{
  gchar *rd = (gchar *) gtk_clist_get_row_data(clist, row);
  t_msgheaders *mh = gtk_object_get_data(GTK_OBJECT(mhe), KEY_MH);
  gchar *value;
  GtkEntry *en_hdr, *en_val;
  GtkWidget *btn_del, *btn_add, *btn_edit;
  if ((! mh) || (! rd))  return;
  
  value = t_msgheaders_get_hdr(mh, rd);
  en_hdr = (GtkEntry *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_HEADER);
  en_val = (GtkEntry *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_VALUE);
  gtk_entry_set_text(en_hdr, rd);
  gtk_entry_set_text(en_val, value);
  
  btn_del = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_BTN_DEL);
  btn_add = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_BTN_ADD);
  btn_edit = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_BTN_EDIT);
  gtk_widget_set_sensitive(btn_del, TRUE);
  gtk_widget_set_sensitive(btn_add, FALSE);
  gtk_widget_set_sensitive(btn_edit, FALSE);
}

static void hdr_changed(GtkWidget *w, GtkWidget *mhe)
{
  GtkWidget *btn_add, *btn_edit;

  btn_add = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_BTN_ADD);
  btn_edit = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_BTN_EDIT);
  gtk_widget_set_sensitive(btn_add, TRUE);
  gtk_widget_set_sensitive(btn_edit, FALSE);
}

static void val_changed(GtkWidget *w, GtkWidget *mhe)
{
  GtkWidget *btn_edit;

  btn_edit = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_BTN_EDIT);
  gtk_widget_set_sensitive(btn_edit, TRUE);
}

static void mhe_edit(GtkWidget *w, GtkWidget *mhe)
{
  GtkEntry *en_hdr, *en_val;
  t_msgheaders *mh;
  gchar *hdr, *val;
  g_return_if_fail(mhe != NULL);

  mh = gtk_object_get_data(GTK_OBJECT(mhe), KEY_MH);
  if (! mh)  return;
  en_hdr = (GtkEntry *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_HEADER);
  en_val = (GtkEntry *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_VALUE);
  hdr = gtk_entry_get_text(en_hdr);
  val = gtk_entry_get_text(en_val);
  t_msgheaders_set_hdr(mh, hdr, val);
  mhdredit_refresh(mhe, mh);
}

static void mhe_del(GtkWidget *w, GtkWidget *mhe)
{
  GtkEntry *en_hdr;
  t_msgheaders *mh;
  gchar *hdr;
  g_return_if_fail(mhe != NULL);

  mh = gtk_object_get_data(GTK_OBJECT(mhe), KEY_MH);
  if (! mh)  return;
  en_hdr = (GtkEntry *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_HEADER);
  hdr = gtk_entry_get_text(en_hdr);
  t_msgheaders_rm_hdr(mh, hdr);
  mhdredit_refresh(mhe, mh);
}

GtkWidget *mhdredit_create(void)
{
  gchar *ttl[2] = {_("Header"), _("Value")};
  GtkWidget *vbox, *scrwin, *hbox, *w, *bbox, *en_hdr, *en_val;
  GtkCList *clist;
  
  grn_lock();
  vbox = gtk_vbox_new(FALSE, FALSE);
  clist = GTK_CLIST(gtk_clist_new_with_titles(2, ttl));
  gtk_widget_set_usize(GTK_WIDGET(clist), 400, 250);
  gtk_clist_column_titles_passive(clist);
  gtk_clist_set_selection_mode(clist, GTK_SELECTION_BROWSE);
  
  scrwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrwin), GTK_WIDGET(clist));
  gtk_box_pack_start(GTK_BOX(vbox), scrwin, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_CLIST, clist);
  
  hbox = gtk_hbox_new(FALSE, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
  w = gtk_label_new(_("Header:"));
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);
  en_hdr = gtk_entry_new();
  gtk_widget_set_usize(en_hdr, 150, -1);
  gtk_box_pack_start(GTK_BOX(hbox), en_hdr, FALSE, FALSE, 0);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_HEADER, en_hdr);

  w = gtk_label_new(_("Value:"));
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);
  en_val = gtk_entry_new();
  gtk_widget_set_usize(en_val, 150, -1);
  gtk_box_pack_start(GTK_BOX(hbox), en_val, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_VALUE, en_val);
  
  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), gnome_preferences_get_button_layout());
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), GNOME_PAD);
  gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 2);

  w = gnome_pixmap_button(gnome_stock_pixmap_widget(NULL, GNOME_STOCK_PIXMAP_ADD), _("Add"));
  gtk_box_pack_start(GTK_BOX(bbox), w, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_BTN_ADD, w);
  gtk_signal_connect(GTK_OBJECT(w), "clicked",
                     GTK_SIGNAL_FUNC(mhe_edit), vbox);
  w = gnome_pixmap_button(gnome_stock_pixmap_widget(NULL, GNOME_STOCK_PIXMAP_PROPERTIES), _("Modify"));
  gtk_box_pack_start(GTK_BOX(bbox), w, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_BTN_EDIT, w);
  gtk_signal_connect(GTK_OBJECT(w), "clicked",
                     GTK_SIGNAL_FUNC(mhe_edit), vbox);
  w = gnome_pixmap_button(gnome_stock_pixmap_widget(NULL, GNOME_STOCK_PIXMAP_REMOVE), _("Delete"));
  gtk_box_pack_start(GTK_BOX(bbox), w, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_BTN_DEL, w);
  gtk_signal_connect(GTK_OBJECT(w), "clicked",
                     GTK_SIGNAL_FUNC(mhe_del), vbox);
  
  gtk_signal_connect(GTK_OBJECT(clist), "select_row",
                     GTK_SIGNAL_FUNC(hdr_sel), vbox);
  gtk_signal_connect(GTK_OBJECT(en_hdr), "changed",
                     GTK_SIGNAL_FUNC(hdr_changed), vbox);
  gtk_signal_connect(GTK_OBJECT(en_val), "changed",
                     GTK_SIGNAL_FUNC(val_changed), vbox);
  
  grn_unlock();
  return vbox;
}

static void add_mh_hdr(GtkCList *clist, gchar *hname, gchar *hval)
{
  gchar *row[2];
  gint idx;

  if ((! str_check(hname)) || (! str_check(hval)))  return;
  row[0] = g_strdup(hname);  row[1] = g_strdup(hval);
  grn_lock();
  idx = gtk_clist_append(clist, row);
  gtk_clist_set_row_data_full(clist, idx, row[0], (GtkDestroyNotify) str_destroy);
  grn_unlock();
  str_free(&row[1]);
}

static void add_extra_hdr(gpointer key, gpointer data, GtkCList *clist)
{
  add_mh_hdr(clist, key, data);
}
void mhdredit_refresh(GtkWidget *mhe, t_msgheaders *mh)
{
  GtkWidget *btn_add, *btn_edit, *btn_del;
  GtkEntry *en_hdr, *en_val;
  GtkCList *clist;
  t_msgheaders *mh1;
  g_return_if_fail(mh != NULL);
  
  grn_lock();
  clist = GTK_CLIST(gtk_object_get_data(GTK_OBJECT(mhe), KEY_CLIST));
  mh1 = gtk_object_get_data(GTK_OBJECT(mhe), KEY_MH);
  if (mh1)  gtk_object_remove_data(GTK_OBJECT(mhe), KEY_MH);
  gtk_object_set_data(GTK_OBJECT(mhe), KEY_MH, mh);
  grn_unlock();
  if (! clist)  return;
  
  gtk_clist_freeze(clist);
  gtk_clist_clear(clist);
  add_mh_hdr(clist, "From", mh->from);
  add_mh_hdr(clist, "To", mh->to);
  add_mh_hdr(clist, "Subject", mh->subject);
  add_mh_hdr(clist, "Date", mh->date);
  add_mh_hdr(clist, "Newsgroups", mh->newsgroups);
  add_mh_hdr(clist, "Message-ID", mh->id);
  add_mh_hdr(clist, "References", mh->ref);
  add_mh_hdr(clist, "Xref", mh->xref);
  add_mh_hdr(clist, "Path", mh->path);
  if (mh->extra_hdrs)
    g_hash_table_foreach(mh->extra_hdrs, (GHFunc) add_extra_hdr, clist);
  gtk_clist_columns_autosize(clist);
  gtk_clist_thaw(clist);

  btn_add = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_BTN_ADD);
  btn_edit = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_BTN_EDIT);
  btn_del = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_BTN_DEL);
  en_hdr = (GtkEntry *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_HEADER);
  en_val = (GtkEntry *) gtk_object_get_data(GTK_OBJECT(mhe), KEY_VALUE);
  gtk_entry_set_text(en_hdr, "");  gtk_entry_set_text(en_val, "");
  gtk_widget_set_sensitive(btn_add, FALSE);
  gtk_widget_set_sensitive(btn_edit, FALSE);
  gtk_widget_set_sensitive(btn_del, FALSE);
}

static gboolean mhdredit_dlg_delete(GtkWidget *w)
{
  grn_save_ws(4, w);
  return FALSE;
}
static void btn_event(GtkWidget *btn, GtkWidget *w)
{
  mhdredit_dlg_delete(w);
}
GtkWidget *mhdredit_dlg_create(t_msgheaders *mh)
{
  GtkWidget *dlg, *mhe;
  
  grn_lock();
  dlg = gnome_dialog_new(_("Edit headers"), GNOME_STOCK_BUTTON_CLOSE, NULL);
  gnome_dialog_set_close(GNOME_DIALOG(dlg), TRUE);
  gtk_window_set_policy(GTK_WINDOW(dlg), FALSE, TRUE, FALSE);
  gtk_window_set_default_size(GTK_WINDOW(dlg), grn_prefs.ws[4].width, grn_prefs.ws[4].height);
  gtk_widget_set_uposition(dlg, grn_prefs.ws[4].x, grn_prefs.ws[4].y);
  gtk_signal_connect(GTK_OBJECT(dlg), "delete_event",
                     GTK_SIGNAL_FUNC(mhdredit_dlg_delete), NULL);
  gnome_dialog_button_connect(GNOME_DIALOG(dlg), 0,
                              GTK_SIGNAL_FUNC(btn_event), dlg);
  grn_unlock();
  
  mhe = mhdredit_create();
  grn_lock();
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg)->vbox), mhe, TRUE, TRUE, 0);
  gtk_widget_show_all(dlg);
  grn_unlock();
  mhdredit_refresh(mhe, mh);
  return dlg;
}
