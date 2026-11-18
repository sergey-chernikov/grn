/*
 * grn_msgsend.c
 *
 * $Id: grn_msgsend.c,v 1.12 2000/07/19 06:26:21 sc Exp $
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
#include "mutt.h"
#include "grn_consts.h"
#include <gnome.h>
#include "grn_msgpost.h"
#include "grn_msgsend.h"
#include "grn_util.h"
#include "grn_config.h"
#include "grn_vars.h"
#include "queue.h"
#include "grn_perl.h"
#include "grn_misc.h"
#include "grn_menu.h"
#include "grn_msgwin.h"
#include "grn_mime.h"


void msgsend_widget_refresh(GtkWidget *mpw, t_message *msg);


GSList *rcpts2list(gchar *src)
{ /* ';' can also be a delimiter of addresses */
  gchar **arr = g_strsplit(src, ",", 0);
  gint i = 0;
  GSList *ret = NULL;

  while (arr[i])  ret = g_slist_prepend(ret, g_strstrip(g_strdup(arr[i++])));
  ret = g_slist_reverse(ret);
  g_strfreev(arr);
  return ret;
}

gboolean check_rcpt(gchar *rcpt)
{
  if (! strchr(rcpt, '@'))  return FALSE;
  return TRUE;
}

msg_goodness valid_rcpts(gchar *rcpts)
{
  GSList *rl, *l;
  msg_goodness ret = OKAY;
  gboolean bad = TRUE;

  rl = rcpts2list(rcpts);
  for (l=rl; l; l=l->next)
  {
    gchar *rcpt = (gchar *) l->data;
    if (check_rcpt(rcpt))  bad = FALSE;
    else  ret = WARN;
    str_free(&rcpt);
  }
  g_slist_free(rl);
  if (bad)  ret = BAD;
  return ret;
}

static void msg_check_body(t_message *msg, GSList **l)
{
  msg_check_res *mcr = NULL;
  gchar **lines = NULL;
  gint i, nlines=0, nqlines=0;
  regex_t regex;
  regmatch_t regmatch;

  if (! str_check(msg->body))
  {
    mcr = g_new(msg_check_res, 1);
    mcr->result = g_strdup(_("Message body is empty"));
    mcr->gl = WARN;
    *l = g_slist_append(*l, mcr);
    return;
  }
  if (! strchr(msg->body, '\n'))
  {
    mcr = g_new(msg_check_res, 1);
    mcr->result = g_strdup(_("No linefeeds in the body"));
    mcr->gl = WARN;
    *l = g_slist_append(*l, mcr);
  }
  
  lines = g_strsplit(msg->body, "\n", -1);

  if (! regcomp(&regex, grn_prefs.re_quoting, REG_EXTENDED|REG_NOSUB))
  {
    for (i=0; lines[i]; i++)
    {
      nlines++;
      if (! regexec(&regex, lines[i], 0, &regmatch, 0))  nqlines++;
    }
    regfree(&regex);
  }
  if ((nlines > 0) && (((gfloat) nqlines / (gfloat) nlines) > 0.75))
  {
    mcr = g_new(msg_check_res, 1);
    mcr->result = g_strdup(_("The body has more than 75% of quoting"));
    mcr->gl = WARN;
    *l = g_slist_append(*l, mcr);
  }
  g_strfreev(lines);
}

static GSList *msg_check(t_message *msg)
{
  GSList *ret = NULL;
  msg_check_res *mcr = NULL;
  
  if (! str_check(msg->mh->from))
  {
    mcr = g_new(msg_check_res, 1);
    mcr->result = g_strdup(_("'From' field is empty"));
    mcr->gl = BAD;
    ret = g_slist_append(ret, mcr);
  }
  if (! str_check(msg->mh->to))
  {
    mcr = g_new(msg_check_res, 1);
    mcr->result = g_strdup(_("No recipients specified"));
    mcr->gl = BAD;
    ret = g_slist_append(ret, mcr);
  }
  else {
    msg_goodness mg = valid_rcpts(msg->mh->to);
    if ((mg == BAD) || (mg == WARN))
    {
      mcr = g_new(msg_check_res, 1);
      mcr->gl = mg;
      if (mg == BAD)  mcr->result = g_strdup(_("Invalid recipient[s]"));
      else  mcr->result = g_strdup(_("Some recipients are invalid"));
    }
  }
  if (! str_check(msg->mh->subject))
  {
    mcr = g_new(msg_check_res, 1);
    mcr->result = g_strdup(_("Subject is empty"));
    mcr->gl = WARN;
    ret = g_slist_append(ret, mcr);
  }
  msg_check_body(msg, &ret);
  return ret;
}


static void add_hdr(gchar **dst, gchar *hdrname, gchar *hdr)
{
  str_add(dst, hdrname);
  str_add(dst, hdr);
  str_add(dst, "\r\n");
}

static void extra_hdrs_add(gchar *key, gchar *data, gchar **ret)
{
  if (str_check(data))
  {
    gchar *buf = g_strdup_printf("%s: %s\r\n", key, data);
    str_add(ret, buf);
    str_free(&buf);
  }
}

gchar *msg_to_smtp_text(t_message *msg)
{
  gchar *ret = NULL;
  
  if (! str_check(msg->mh->subject))  msg->mh->subject = g_strdup(_("<none>"));

#if 0
  if (msg->attachments)
  {
    BODY *b = mutt_new_body(), *last = NULL;
    GSList *l;
    
    for (l=msg->attachments; l; l=l->next)
    {
      BODY *b1 = mutt_copy_body((BODY *) l->data, NULL);
      
      if (last)  last->next = b1;
      else  b->parts = b1;
      last = b1;
    }
    b = mutt_make_multipart(b);
  }
  else  add_mime_to_plaintext(msg);
#endif
  ret = g_strconcat("From: ", msg->mh->from, "\n",
                    "To: ", msg->mh->to, "\n",
		    "Subject: ", msg->mh->subject, "\n", NULL);
  if (str_check(msg->mh->date))  add_hdr(&ret, "Date: ", msg->mh->date);
  if (str_check(msg->mh->id))  add_hdr(&ret, "Message-ID: ", msg->mh->id);
  if (msg->mh->extra_hdrs)
    g_hash_table_foreach(msg->mh->extra_hdrs, (GHFunc) extra_hdrs_add, &ret);
  str_add(&ret, "\r\n");
  str_add(&ret, msg->body);
  str_add(&ret, "\r\n");
		    
  return ret;
}

static void fill_msg_headers(t_message *msg)
{
  gchar *tmp = NULL;
  g_return_if_fail(msg != NULL);

  if (! msg->mh)  msg->mh = t_msgheaders_alloc();

  str_free(&(msg->mh->id));
  str_free(&(msg->mh->ref));
  str_free(&(msg->mh->date));
  str_free(&(msg->mh->to));
  t_msgheaders_rm_extra_hdrs(msg->mh);

  str_free(&(msg->mh->from));
  msg->mh->from = g_strdup(grn_prefs.name);
  if (grn_prefs.email)  tmp = g_strdup_printf(" <%s>", grn_prefs.email);
  if (tmp)  str_add(&(msg->mh->from), tmp);
  str_free(&tmp);
  
  if (str_check(grn_prefs.ua))
  {
    tmp = g_strdup(grn_prefs.ua);  grn_subst_vars(&tmp);
    t_msgheaders_add_extra_hdr(msg->mh, "X-Mailer", tmp);
    str_free(&tmp);
  }
  if (str_check(grn_prefs.org))
  {
    tmp = g_strdup(grn_prefs.org);  grn_subst_vars(&tmp);
    t_msgheaders_add_extra_hdr(msg->mh, "Organization", tmp);
    str_free(&tmp);
  }

  if (str_check(grn_prefs.replyto))
    t_msgheaders_add_extra_hdr(msg->mh, "Reply-To", grn_prefs.replyto);

}

static void subject_changed(GtkWidget *w, GtkWidget *p)
{
  gchar *subj, *title;
  GtkWidget *wnd = gtk_object_get_data(GTK_OBJECT(p), KEY_WND);

  if (! wnd)  return;
  subj = gtk_entry_get_text(GTK_ENTRY(w));
  title = g_strdup_printf("%s: %s", _("GRN send message"), subj);
  gtk_window_set_title(GTK_WINDOW(wnd), title);
  g_free(title);
}

static GtkWidget *create_msgsend_pane(GtkWidget *parent)
{
  GtkWidget *table, *label, *entry;
  
  table = gtk_table_new(2, 2, FALSE);
  
  label = gtk_label_new(_("From:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,0,1, GTK_FILL, GTK_FILL, 3,1);
  entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), entry, 1,3,0,1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,1);
  gtk_object_set_data(GTK_OBJECT(parent), KEY_MW_FROM, entry);
  
  label = gtk_label_new(_("To:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,1,2, GTK_FILL, GTK_FILL, 3,1);
  entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), entry, 1,2,1,2,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,1);
  gtk_object_set_data(GTK_OBJECT(parent), KEY_MW_TO, entry);
  
  label = gtk_label_new(_("Subject:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,2,3, GTK_FILL, GTK_FILL, 3,1);
  entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), entry, 1,3,2,3,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,1);
  gtk_object_set_data(GTK_OBJECT(parent), KEY_MW_SUBJ, entry);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
                     GTK_SIGNAL_FUNC(subject_changed), parent);
  
  return table;
}


static void abort_cb(gint reply, GtkWidget *w)
{
  g_return_if_fail(w != NULL);
  if (reply == GNOME_YES)  gtk_widget_destroy(w);
}

static gboolean delete_wnd(GtkWidget *w)
{
  grn_save_ws(5, w);
  if (grn_prefs.cfms[4])
  {
    gnome_app_question(GNOME_APP(GRN->window), _("Are you sure you wish to abort sending?"),
                       (GnomeReplyCallback) abort_cb, w);
    return TRUE;
  }
  else  return FALSE;
}

static gint evt_msgsend_keypress(GtkWidget *w, GdkEventKey *event, GtkWidget *mpw)
{
  GtkWidget *text = gtk_object_get_data(GTK_OBJECT(mpw), KEY_TXT_BODY);

  switch (event->keyval)
  {
    case GDK_Escape:
      if (! delete_wnd(w))  gtk_widget_destroy(w);
      break;
    case GDK_Return:
    case GDK_KP_Enter:
      if (text && (! GTK_WIDGET_HAS_FOCUS(text)))  gtk_widget_grab_focus(text);
      else  return FALSE;
      break;
    default:  return FALSE;
  }
  gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "key_press_event");
  return TRUE;
}

static void text_entered_new(GtkWidget *w)
{
  glong ph_res;
  t_message *m;

  gtk_signal_disconnect_by_func(GTK_OBJECT(w), text_entered_new, NULL);
  if (! GTK_IS_TEXT(w))  return;
  m = gtk_object_get_data(GTK_OBJECT(w), KEY_MSG);
  if (! m)  return;
#ifdef USE_PERL
  ph_res = perl_hook_run("TemplateMNew", GRN, m, NULL);
  if (ph_res >= 1)
  {
    GtkWidget *mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_WND);
    if (! m->body)  m->body = g_strdup("");

    if (m->pos >= strlen(m->body))  m->pos = strlen(m->body) - 1;
    if (mpw)  msgsend_widget_refresh(mpw, m);
    else {
      gtk_text_insert(GTK_TEXT(w), NULL, NULL, NULL, m->body, strlen(m->body));
      gtk_text_set_point(GTK_TEXT(w), m->pos);
      gtk_editable_set_position(GTK_EDITABLE(w), m->pos);
    }
  }
#endif
}

static void text_entered_reply(GtkWidget *w)
{
  glong ph_res = 0;
  t_message *m, *m1;
  GtkWidget *mpw;

  gtk_signal_disconnect_by_func(GTK_OBJECT(w), text_entered_reply, NULL);
  if (! GTK_IS_TEXT(w))  return;
  m1 = gtk_object_get_data(GTK_OBJECT(w), KEY_MSG1);
  m = gtk_object_get_data(GTK_OBJECT(w), KEY_MSG);
  if (! m)  return;
#ifdef USE_PERL
  ph_res = perl_hook_run("TemplateMReply", GRN, m, m1);
#endif
  if (ph_res < 1)
  {
    m->body = create_quoting(m1);  m->pos = 0;
  }
  if (m->pos >= strlen(m->body))  m->pos = strlen(m->body) - 1;

  mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_WND);

  if (mpw)  msgsend_widget_refresh(mpw, m);
  else {
    gtk_text_insert(GTK_TEXT(w), NULL, NULL, NULL, m->body, strlen(m->body));
    gtk_text_set_point(GTK_TEXT(w), m->pos);
    gtk_editable_set_position(GTK_EDITABLE(w), m->pos);
  }
}

static void text_entered_forward(GtkWidget *w)
{
  glong ph_res = 0;
  t_message *m, *m1;
  GtkWidget *mpw;

  gtk_signal_disconnect_by_func(GTK_OBJECT(w), text_entered_forward, NULL);
  if (! GTK_IS_TEXT(w))  return;
  m1 = gtk_object_get_data(GTK_OBJECT(w), KEY_MSG1);
  m = gtk_object_get_data(GTK_OBJECT(w), KEY_MSG);
  if (! m)  return;
#ifdef USE_PERL
  ph_res = perl_hook_run("TemplateMForward", GRN, m, m1);
#endif
  if (ph_res < 1)
  {
    m->body = create_forward(m1);  m->pos = 0;
  }
  if (m->pos >= strlen(m->body))  m->pos = strlen(m->body) - 1;

  mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_WND);
  if (mpw)  msgsend_widget_refresh(mpw, m);
  else {
    gtk_text_insert(GTK_TEXT(w), NULL, NULL, NULL, m->body, strlen(m->body));
    gtk_text_set_point(GTK_TEXT(w), m->pos);
    gtk_editable_set_position(GTK_EDITABLE(w), m->pos);
  }
}

static void msw_focused(GtkWindow *wnd, gpointer data)
{
  GRN->focused = wnd;
}
static void msw_destroy(GtkWidget *wnd, gpointer data)
{
  GRN->focused = NULL;
}

GtkWidget *create_msgsend_widget(msg_create_mode mcm)
{
  GtkWidget *vbox, *wnd, *text, *bar, *table, *cntr;
  GnomeDockItem *item;
  GtkAdjustment *adj;
  GtkSignalFunc text_entered_func = NULL;
  
  grn_lock();
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), create_msgsend_pane(vbox), FALSE, FALSE, 0);
  table = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
  adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 0, 0, 0));

  text = gtk_text_new(NULL, adj);
  gtk_text_set_editable(GTK_TEXT(text), TRUE);
  gtk_text_set_word_wrap(GTK_TEXT(text), TRUE);
  gtk_table_attach(GTK_TABLE(table), text, 0,1,0,1,
    GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0,0);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_TXT_BODY, text);
  gtk_table_attach(GTK_TABLE(table), gtk_vscrollbar_new(adj), 1,2,0,1, GTK_FILL, GTK_FILL, 0,0);
  
  wnd = gnome_app_new("GRN_send", _("GRN send message"));
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_WND, wnd);
  gtk_object_set_data(GTK_OBJECT(wnd), KEY_MSGPOST, vbox);
  gtk_window_set_default_size(GTK_WINDOW(wnd), grn_prefs.ws[5].width,
                              grn_prefs.ws[5].height);
  gtk_widget_set_uposition(GTK_WIDGET(wnd), grn_prefs.ws[5].x, grn_prefs.ws[5].y);
  cntr = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(cntr), GTK_SHADOW_ETCHED_OUT);
  gtk_container_add(GTK_CONTAINER(cntr), vbox);
  gtk_widget_show(cntr);
  gnome_app_set_contents(GNOME_APP(wnd), cntr);
  gnome_app_create_menus(GNOME_APP(wnd), msgsend_menu);
  gnome_app_create_toolbar(GNOME_APP(wnd), msgsend_toolbar);
  item = gnome_app_get_dock_item_by_name(GNOME_APP(wnd), GNOME_APP_TOOLBAR_NAME);
  if (item)
  {
    bar = gnome_dock_item_get_child(item);
    if (bar)
    {
      gtk_toolbar_set_style(GTK_TOOLBAR(bar), grn_get_toolbar_style());
      gtk_object_set_data(GTK_OBJECT(bar), KEY_MSGPOST, vbox);
    }
  }
  item = gnome_app_get_dock_item_by_name(GNOME_APP(wnd), GNOME_APP_MENUBAR_NAME);
  if (item)
  {
    bar = gnome_dock_item_get_child(item);
    if (bar)
    {
      gchar *s1 = _("Message");
      grn_menu_set_data(bar, s1, _("Send"), KEY_MSGPOST, vbox);
      grn_menu_set_data(bar, s1, _("Cancel"), KEY_MSGPOST, vbox);
      grn_menu_set_data(bar, s1, _("Edit headers"), KEY_MSGPOST, vbox);
      grn_menu_set_data(bar, s1, _("Attachments..."), KEY_MSGPOST, vbox);
      s1 = _("File");
      grn_menu_set_data(bar, s1, _("Load..."), KEY_MSGPOST, vbox);
    }
  }
  
  switch (mcm)
  {
    case MSG_SEND_REPLY:
      text_entered_func = GTK_SIGNAL_FUNC(text_entered_reply);
      break;
    case MSG_SEND_FORWARD:
      text_entered_func = GTK_SIGNAL_FUNC(text_entered_forward);
      break;
    default:
      text_entered_func = GTK_SIGNAL_FUNC(text_entered_new);
  }

  gtk_signal_connect(GTK_OBJECT(wnd), "delete_event",
                     GTK_SIGNAL_FUNC(delete_wnd), NULL);
  gtk_signal_connect(GTK_OBJECT(wnd), "key_press_event",
                     GTK_SIGNAL_FUNC(evt_msgsend_keypress), vbox);
  gtk_signal_connect(GTK_OBJECT(wnd), "focus_in_event",
                     GTK_SIGNAL_FUNC(msw_focused), NULL);
  gtk_signal_connect(GTK_OBJECT(wnd), "destroy",
                     GTK_SIGNAL_FUNC(msw_destroy), NULL);
  if (text_entered_func)
    gtk_signal_connect(GTK_OBJECT(text), "focus_in_event",
                       text_entered_func, NULL);

  gtk_widget_show_all(wnd);
  grn_unlock();
  build_macros_menu(GNOME_APP(wnd)->menubar, 4);
  return vbox;
}


static void entry_left(GtkWidget *entry, GdkEventFocus *e, gchar **text)
{
  gchar *en_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
  str_free(text);
  *text = en_text;
}
static void subject_left(GtkWidget *entry, GdkEventFocus *e, t_msgheaders *mh)
{
  gchar *en_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
  str_free(&(mh->subject));
  mh->subject = en_text;
}

static GtkWidget *msgsend_set_hdr(GtkWidget *mw, gchar *id, gchar **text,
                                  gboolean set_signal)
{
  GtkWidget *hw;
  g_return_val_if_fail(mw != NULL, NULL);
  g_return_val_if_fail(id != NULL, NULL);
  if (! text)  return NULL;
  
  grn_lock();
  hw = gtk_object_get_data(GTK_OBJECT(mw), id);
  grn_unlock();
  if (! hw)  return NULL;
  
  grn_widget_set_style(hw, grn_prefs.fonts[2], NULL, NULL, NULL);
  grn_lock();
  if (GTK_IS_ENTRY(hw))
  {
    if (*text)
    {
      gtk_entry_set_text(GTK_ENTRY(hw), *text);
      if (set_signal)
        gtk_signal_connect(GTK_OBJECT(hw), "focus_out_event",
                           GTK_SIGNAL_FUNC(entry_left), text);
    }
    else  gtk_entry_set_text(GTK_ENTRY(hw), "");
  }
  else if (GTK_IS_LABEL(hw))
  {
    if (*text)  gtk_label_set_text(GTK_LABEL(hw), *text);
    else  gtk_label_set_text(GTK_LABEL(hw), "");
  }
  grn_unlock();
  return hw;
}

void msgsend_widget_refresh(GtkWidget *mpw, t_message *msg)
{
  GtkText *text;
  g_return_if_fail(mpw != NULL);
  g_return_if_fail(msg != NULL);
  
  msgsend_set_hdr(mpw, KEY_MW_FROM, &(msg->mh->from), FALSE);
  msgsend_set_hdr(mpw, KEY_MW_TO, &(msg->mh->to), FALSE);
  msgsend_set_hdr(mpw, KEY_MW_SUBJ, &(msg->mh->subject), FALSE);
  
  grn_lock();
  text = GTK_TEXT(gtk_object_get_data(GTK_OBJECT(mpw), KEY_TXT_BODY));
  if (text)
  {
    if (str_check(msg->body))
    {
      gtk_text_insert(text, NULL, NULL, NULL, msg->body, strlen(msg->body));
      gtk_text_set_point(text, msg->pos);
      gtk_editable_set_position(GTK_EDITABLE(text), msg->pos);
    }
  }
  grn_unlock();
}

void msgsend_widget_set_data(GtkWidget *mpw, t_message *msg, t_message *msg1)
{
  GtkText *text;
  GtkWidget *w;
  g_return_if_fail(mpw != NULL);
  g_return_if_fail(msg != NULL);
  
  replace_msg(GTK_OBJECT(mpw), msg, KEY_MSG);
  
  msgsend_set_hdr(mpw, KEY_MW_FROM, &(msg->mh->from), TRUE);
  msgsend_set_hdr(mpw, KEY_MW_TO, &(msg->mh->to), TRUE);
  w = msgsend_set_hdr(mpw, KEY_MW_SUBJ, &(msg->mh->subject), TRUE);
  if (! msg->mh->subject)
    gtk_signal_connect(GTK_OBJECT(w), "focus_out_event",
                       GTK_SIGNAL_FUNC(subject_left), msg->mh);
  
  grn_lock();
  text = GTK_TEXT(gtk_object_get_data(GTK_OBJECT(mpw), KEY_TXT_BODY));
  if (text)
  {
    gchar *fnt_name;
    if (grn_prefs.mw_prop)  fnt_name = grn_prefs.fonts[0];
    else  fnt_name = grn_prefs.fonts[1];
    grn_widget_set_style(GTK_WIDGET(text), fnt_name, &(grn_prefs.fg_colors[0]),
      &(grn_prefs.bg_colors[0]), grn_prefs.pixmaps[0]);

    if (str_check(msg->body))
    {
      gtk_text_insert(text, NULL, NULL, NULL, msg->body, strlen(msg->body));
      gtk_text_set_point(text, msg->pos);
      gtk_editable_set_position(GTK_EDITABLE(text), msg->pos);
    }
    gtk_object_set_data(GTK_OBJECT(text), KEY_MSG, msg);
    if (msg1)
    {
      grn_unlock();
      replace_msg(GTK_OBJECT(text), msg1, KEY_MSG1);
      grn_lock();
    }
    gtk_object_set_data(GTK_OBJECT(text), KEY_WND, mpw);
  }
  
  w = gtk_object_get_data(GTK_OBJECT(mpw), KEY_MW_TO);
  if (w)  gtk_widget_grab_focus(w);
  grn_unlock();
}


static t_message *msgsend_get_message(GtkWidget *mpw)
{
  t_message *m = NULL;
  GtkWidget *w;
  if (! mpw)  return NULL;

  m = gtk_object_get_data(GTK_OBJECT(mpw), KEY_MSG);
  if (! m)
  {
    m = t_message_alloc();
    m->mh = t_msgheaders_alloc();
    gtk_object_set_data_full(GTK_OBJECT(mpw), KEY_MSG, m,
                             (GtkDestroyNotify) t_message_destroy);
  }
  
  w = gtk_object_get_data(GTK_OBJECT(mpw), KEY_TXT_BODY);
  if (w)
  {
    str_free(&(m->body));
    m->body = g_strdup(gtk_editable_get_chars(GTK_EDITABLE(w), 0, -1));
  }
  w = gtk_object_get_data(GTK_OBJECT(mpw), KEY_MW_FROM);
  if (w)
  {
    str_free(&(m->mh->from));
    m->mh->from = g_strdup(gtk_entry_get_text(GTK_ENTRY(w)));
  }
  w = gtk_object_get_data(GTK_OBJECT(mpw), KEY_MW_TO);
  if (w)
  {
    str_free(&(m->mh->to));
    m->mh->to = g_strdup(gtk_entry_get_text(GTK_ENTRY(w)));
  }
  w = gtk_object_get_data(GTK_OBJECT(mpw), KEY_MW_SUBJ);
  if (w)
  {
    str_free(&(m->mh->subject));
    m->mh->subject = g_strdup(gtk_entry_get_text(GTK_ENTRY(w)));
  }
  
  return m;
}


void evt_send_new(GtkWidget *w)
{
  t_message *m;
  GtkWidget *mpw;
  g_return_if_fail(w != NULL);

  m = t_message_alloc();  m->mh = t_msgheaders_alloc();
  fill_msg_headers(m);
  m->mh->to = g_strdup("");
  mpw = create_msgsend_widget(MSG_SEND_NEW);
  msgsend_widget_set_data(mpw, m, NULL);
}

void evt_send_reply(GtkWidget *w)
{
  t_message *m, *m1;
  GtkWidget *mpw;
  gchar *rt;
  g_return_if_fail(w != NULL);

  m = t_message_alloc();  m->mh = t_msgheaders_alloc();
  m1 = get_cur_msg(w);
  fill_msg_headers(m);

  rt = t_msgheaders_get_extra_hdr(m1->mh, "Reply-To");
  if (rt)  m->mh->to = str_copy(rt);
  else  m->mh->to = str_copy(m1->mh->from);
  m->mh->subject = str_copy(m1->mh->subject);

  mpw = create_msgsend_widget(MSG_SEND_REPLY);
  msgsend_widget_set_data(mpw, m, m1);
}

void evt_send_forward(GtkWidget *w)
{
  t_message *m, *m1;
  GtkWidget *mpw;
  g_return_if_fail(w != NULL);

  m = t_message_alloc();  m->mh = t_msgheaders_alloc();
  m1 = get_cur_msg(w);
  fill_msg_headers(m);

  m->mh->subject = str_copy(m1->mh->subject);
  m->mh->to = g_strdup("");

  mpw = create_msgsend_widget(MSG_SEND_FORWARD);
  msgsend_widget_set_data(mpw, m, m1);
}


typedef struct send_data
{
  t_message *m;
  GtkWidget *wnd;
} send_data;

static void warn_cb(gint reply, send_data *sd)
{
  g_return_if_fail(sd != NULL);
  if (reply == GNOME_YES)
  {
#ifdef USE_PERL
    perl_hook_run("PreSend", GRN, sd->m, NULL);
#endif
    if (sd->m)  grnq_add_send_message(sd->m);
    if (sd->wnd)  gtk_widget_destroy(sd->wnd);
  }
  else  t_message_free(&(sd->m));
  g_free(sd);
}

void evt_send_ok(GtkWidget *w)
{
  GtkWidget *mpw, *wnd;
  t_message *m;
  GSList *gl;
  msg_goodness mg;
  if (! w->parent)  return;

  mpw = gtk_object_get_data(GTK_OBJECT(w->parent), KEY_MSGPOST);
  if (! mpw)
  {
    mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_MSGPOST);
    if (! mpw)  return;
  }
  
  m = t_message_copy(msgsend_get_message(mpw));
  wnd = gtk_object_get_data(GTK_OBJECT(mpw), KEY_WND);
  
  gl = msg_check(m);
  mg = get_goodness(gl);
  if (mg == BAD)
  {
    gchar *text1 = goodness_list_to_str(gl, BAD);
    gchar *text2 = goodness_list_to_str(gl, WARN);
    gchar *text = g_strdup(_("Sending abandoned!\n\n"));
    if (str_check(text1))
    {
      str_add(&text, _("Errors:\n"));
      str_add(&text, text1);  str_free(&text1);
    }
    if (str_check(text2))
    {
      str_add(&text, _("Warnings:\n"));
      str_add(&text, text2);  str_free(&text2);
    }
    gnome_app_error(GNOME_APP(GRN->window), text);
    str_free(&text);  goodness_list_free(gl);
    t_message_free(&m);
    return;
  }
  if (grn_prefs.cfms[5])
  {
    if (mg == WARN)
    {
      gchar *text1 = goodness_list_to_str(gl, WARN);
      gchar *text = g_strdup(_("Some warnings encountered:\n"));
      send_data *sd = g_new(send_data, 1);
      str_add(&text, text1);  str_free(&text1);
      str_add(&text, _("\nDo you wish to continue sending?"));
      sd->m = m;  sd->wnd = wnd;
      gnome_app_question(GNOME_APP(GRN->window), text,
        (GnomeReplyCallback) warn_cb, sd);
      str_free(&text);  goodness_list_free(gl);
      return;
    }
  }
  goodness_list_free(gl);
#ifdef USE_PERL
  perl_hook_run("PreSend", GRN, m, NULL);
#endif
  if (m)  grnq_add_send_message(m);
  if (wnd)  gtk_widget_destroy(wnd);
}

void evt_send_cancel(GtkWidget *w)
{
  GtkWidget *mpw, *wnd;
  if (! w->parent)  return;
  
  mpw = gtk_object_get_data(GTK_OBJECT(w->parent), KEY_MSGPOST);
  if (! mpw)
  {
    mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_MSGPOST);
    if (! mpw)  return;
  }
  wnd = gtk_object_get_data(GTK_OBJECT(mpw), KEY_WND);
  if (wnd)
  {
    if (grn_prefs.cfms[4])
      gnome_app_question(GNOME_APP(GRN->window), _("Are you sure you wish to abort sending?"),
        (GnomeReplyCallback) abort_cb, wnd);
    else  gtk_widget_destroy(wnd);
  }
}
