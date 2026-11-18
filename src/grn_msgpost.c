/*
 * grn_msgpost.c
 *
 * $Id: grn_msgpost.c,v 1.27 2000/07/19 06:26:21 sc Exp $
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
#include <gnome.h>
#include "grn_msgpost.h"
#include "grn_util.h"
#include "grn_config.h"
#include "grn_msgwin.h"
#include "grn_misc.h"
#include "grn_menu.h"
#include "grn_vars.h"
#include "queue.h"
#include "grn_perl.h"
#include "grn_nglist.h"
#include "grn_mhdrwin.h"


void msgpost_widget_refresh(GtkWidget *mpw, t_message *msg);


static GSList *newsgroups2list(gchar *src)
{
  gchar **arr = g_strsplit(src, ",", 0);
  gint i = 0;
  GSList *ret = NULL;

  while (arr[i])  ret = g_slist_prepend(ret, g_strstrip(g_strdup(arr[i++])));
  ret = g_slist_reverse(ret);
  g_strfreev(arr);
  return ret;
}


static gchar *ne_groups(gchar *groups)
{
  GSList *gl, *l;
  GList *ng;
  gchar *ret = NULL;

  gl = newsgroups2list(groups);
  for (l=gl; l; l=l->next)
  {
    gchar *group = (gchar *) l->data;
    gboolean found = FALSE;
    for (ng = GRN->ng_list; ng; ng=ng->next)
    {
      grn_newsgroup *grp = (grn_newsgroup *) ng->data;
      if ((! grp) || (! str_check(grp->name)))  continue;
      if (! strcmp(grp->name, group))
      {
        found = TRUE;  break;
      }
    }
    if (! found)
    {
      if (! str_check(ret))  ret = g_strdup(group);
      else {
        str_add(&ret, ", ");  str_add(&ret, group);
      }
    }
    str_free(&group);
  }
  g_slist_free(gl);
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
  if (! str_check(msg->mh->newsgroups))
  {
    mcr = g_new(msg_check_res, 1);
    mcr->result = g_strdup(_("No newsgroups specified"));
    mcr->gl = BAD;
    ret = g_slist_append(ret, mcr);
  }
  else {
    gchar *igroups = ne_groups(msg->mh->newsgroups);
    if (str_check(igroups))
    {
      mcr = g_new(msg_check_res, 1);
      mcr->result = g_strdup_printf(_("Invalid group[s]: %s"), igroups);
      mcr->gl = BAD;
      ret = g_slist_append(ret, mcr);
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

msg_goodness get_goodness(GSList *gl)
{
  GSList *l;
  msg_goodness ret = OKAY;

  for (l=gl; l; l=l->next)
  {
    msg_check_res *mcr = (msg_check_res *) l->data;
    if (! mcr)  continue;

    if (mcr->gl == BAD)  return BAD;
    ret = mcr->gl;
  }
  return ret;
}

gchar *goodness_list_to_str(GSList *gl, msg_goodness mg)
{
  GSList *l;
  gchar *ret = NULL;
  
  for (l=gl; l; l=l->next)
  {
    msg_check_res *mcr = (msg_check_res *) l->data;
    if (! mcr)  continue;
    
    if (mcr->gl == mg)
    {
      str_add(&ret, " - ");  str_add(&ret, mcr->result);  str_add(&ret, "\n");
    }
  }
  return ret;
}

void goodness_list_free(GSList *gl)
{
  GSList *l;
  
  for (l=gl; l; l=l->next)
  {
    msg_check_res *mcr = (msg_check_res *) l->data;
    if (! mcr)  continue;
    
    str_free(&(mcr->result));  g_free(mcr);
  }
  g_slist_free(gl);
}


gchar *trimmed_refs(const gchar *refs)
{
  const gint gnksa_cutoff = 998 - (gint) strlen("References: ");
  gint len, i, n=0, k=0;
  gchar **ref_arr;
  gchar *ret = NULL;
  g_return_val_if_fail(refs != NULL, NULL);
  
  len = strlen(refs);
  if (len <= gnksa_cutoff)  return g_strdup(refs);
  ref_arr = g_strsplit(refs, ">", -1);
  for (i=0; ref_arr[i]; i++)  n++;
  len = 0;
  for (i=n-1; i>=0; i--)
  {
    gchar *s1, *s = ref_arr[i];
    gchar *p = strchr(s, '<');
    gint l1;
    if (p)  *p = ' ';
    g_strstrip(s);
    if (strlen(s) <= 0)  continue;
    s1 = g_strdup_printf("<%s>", s);
    l1 = strlen(s1);  str_free(&s1);
    len += l1 + 1;
    if (len > gnksa_cutoff)
    {
      k = i+1;  break;
    }
  }
  for (i=k; ref_arr[i]; i++)
  {
    gchar *s1, *s = ref_arr[i];
    gchar *p = strchr(s, '<');
    if (p)  *p = ' ';
    g_strstrip(s);
    if (strlen(s) <= 0)  continue;
    s1 = g_strdup_printf("<%s>", s);
    if (str_check(ret))
    {
      str_add(&ret, " ");  str_add(&ret, s1);
      str_free(&s1);
    }
    else  ret = s1;
  }
  g_strfreev(ref_arr);
  return ret;
}


static void add_hdr(gchar **dst, gchar *hdrname, gchar *hdr)
{
  str_add(dst, hdrname);
  str_add(dst, hdr);
  str_add(dst, "\n");
}

static void extra_hdrs_add(gchar *key, gchar *data, gchar **ret)
{
  if (str_check(data))
  {
    gchar *buf = g_strdup_printf("%s: %s\n", key, data);
    str_add(ret, buf);
    str_free(&buf);
  }
}

gchar *msg_to_text(t_message *msg)
{
  gchar *ret = NULL;
  
  if (! str_check(msg->mh->subject))  msg->mh->subject = g_strdup(_("<none>"));

  ret = g_strconcat("From: ", msg->mh->from, "\n",
                    "Path: ", msg->mh->path, "\n",
		    "Newsgroups: ", msg->mh->newsgroups, "\n",
		    "Subject: ", msg->mh->subject, "\n", NULL);
  if (str_check(msg->mh->date))  add_hdr(&ret, "Date: ", msg->mh->date);
  if (str_check(msg->mh->id))  add_hdr(&ret, "Message-ID: ", msg->mh->id);
  if (str_check(msg->mh->ref))
  {
    gchar *p = msg->mh->ref;
    msg->mh->ref = trimmed_refs(p);  str_free(&p);
    add_hdr(&ret, "References: ", msg->mh->ref);
  }
  if (msg->mh->extra_hdrs)
    g_hash_table_foreach(msg->mh->extra_hdrs, (GHFunc) extra_hdrs_add, &ret);
  str_add(&ret, "\n");
  str_add(&ret, msg->body);
		    
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
  str_free(&(msg->mh->path));
  t_msgheaders_rm_extra_hdrs(msg->mh);

  str_free(&(msg->mh->from));
  msg->mh->from = g_strdup(grn_prefs.name);
  if (grn_prefs.email)  tmp = g_strdup_printf(" <%s>", grn_prefs.email);
  if (tmp)  str_add(&(msg->mh->from), tmp);
  str_free(&tmp);
  
  tmp = g_strconcat(get_host_name(), "!", g_get_user_name(), NULL);
  str_add(&(msg->mh->path), tmp);
  str_free(&tmp);
  
  if (str_check(grn_prefs.ua))
  {
    tmp = g_strdup(grn_prefs.ua);  grn_subst_vars(&tmp);
    t_msgheaders_add_extra_hdr(msg->mh, "User-Agent", tmp);
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

gchar *create_forward(t_message *m)
{
  gchar *ret = NULL;
  gchar *s = NULL, *s1 = NULL;
  g_return_val_if_fail(m != NULL, NULL);
  g_return_val_if_fail(m->mh != NULL, NULL);
  
  ret = g_strdup("\n\n");
  s = g_strnfill(63, '=');
  str_add(&ret, s);  str_free(&s);
  str_add(&ret, "\n");
  if (str_check(m->mh->newsgroups))
  {
    s = g_strdup_printf("Newsgroups: %s\n", m->mh->newsgroups);
    str_add(&ret, s);  str_free(&s);
  }
  else if (m->mh->grp)
  {
    s = NULL;
    if (m->mh->grp->name)
    {
      if (m->mh->grp->descr)
        s = g_strdup_printf("Newsgroup:  %s (%s)\n", m->mh->grp->name, m->mh->grp->descr);
      else  s = g_strdup_printf("Newsgroup:  %s\n", m->mh->grp->name);
    }
    if (s)  str_add(&ret, s);
    str_free(&s);
  }
  if (str_check(m->mh->from))
  {
    s = g_strdup_printf("From:       %s\n", m->mh->from);
    str_add(&ret, s);  str_free(&s);
  }
  s1 = t_msgheaders_get_extra_hdr(m->mh, "X-Comment-To");
  if (str_check(s1))
  {
    s = g_strdup_printf("To:         %s\n", s1);
    str_add(&ret, s);  str_free(&s);
  }
  if (str_check(m->mh->subject))
  {
    s = g_strdup_printf("Subject:    %s\n", m->mh->subject);
    str_add(&ret, s);  str_free(&s);
  }
  s = g_strnfill(63, '=');
  str_add(&ret, s);  str_free(&s);
  str_add(&ret, "\n");
  if (str_check(m->body))  str_add(&ret, m->body);
  return ret;
}

gchar *create_quoting(t_message *m)
{
  gchar **lines, *ret = NULL;
  gint i;
  g_return_val_if_fail(m != NULL, NULL);
  
  lines = g_strsplit(m->body, "\n", -1);
  for (i=0; lines[i]; i++)
  {
    gchar *s = g_strdup_printf("%c %s\n", '>', lines[i]);
    str_add(&ret, s);  str_free(&s);
  }
  g_strfreev(lines);
  return ret;
}


static void subject_changed(GtkWidget *w, GtkWidget *p)
{
  gchar *subj, *title;
  GtkWidget *wnd = gtk_object_get_data(GTK_OBJECT(p), KEY_WND);

  if (! wnd)  return;
  subj = gtk_entry_get_text(GTK_ENTRY(w));
  title = g_strdup_printf("%s: %s", _("GRN post article"), subj);
  gtk_window_set_title(GTK_WINDOW(wnd), title);
  g_free(title);
}

static GtkWidget *create_msgpost_pane(GtkWidget *parent)
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
  
  label = gtk_label_new(_("Newsgroups:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,1,2, GTK_FILL, GTK_FILL, 3,1);
  entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), entry, 1,2,1,2,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,1);
  gtk_object_set_data(GTK_OBJECT(parent), KEY_MW_NEWSGROUPS, entry);
  
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
  grn_save_ws(2, w);
  if (grn_prefs.cfms[4])
  {
    gnome_app_question(GNOME_APP(GRN->window), _("Are you sure you wish to abort posting?"),
                       (GnomeReplyCallback) abort_cb, w);
    return TRUE;
  }
  else  return FALSE;
}

static gint evt_msgpost_keypress(GtkWidget *w, GdkEventKey *event, GtkWidget *mpw)
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

void replace_msg(GtkObject *mpw, t_message *msg, const gchar *key)
{
  t_message *m;
  g_return_if_fail(mpw != NULL);
  g_return_if_fail(msg != NULL);
  g_return_if_fail(key != NULL);

  grn_lock();
  m = gtk_object_get_data(mpw, key);
  if (m)
  {
    t_message_free(&m);
    gtk_object_remove_data(mpw, key);
  }
  gtk_object_set_data_full(mpw, key, msg, (GtkDestroyNotify) t_message_destroy);
  grn_unlock();
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
  ph_res = perl_hook_run("TemplateNew", GRN, m, NULL);
  if (ph_res >= 1)
  {
    GtkWidget *mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_WND);

    if (m->pos >= strlen(m->body))  m->pos = strlen(m->body) - 1;
    if (mpw)  msgpost_widget_refresh(mpw, m);
    else {
      gtk_text_insert(GTK_TEXT(w), NULL, NULL, NULL, m->body, strlen(m->body));
      gtk_text_set_point(GTK_TEXT(w), m->pos);
      gtk_editable_set_position(GTK_EDITABLE(w), m->pos);
    }
  }
#endif
}

static void text_entered_followup(GtkWidget *w)
{
  glong ph_res = 0;
  t_message *m, *m1;
  GtkWidget *mpw;

  gtk_signal_disconnect_by_func(GTK_OBJECT(w), text_entered_followup, NULL);
  if (! GTK_IS_TEXT(w))  return;
  m1 = gtk_object_get_data(GTK_OBJECT(w), KEY_MSG1);
  m = gtk_object_get_data(GTK_OBJECT(w), KEY_MSG);
  if (! m)  return;
#ifdef USE_PERL
  ph_res = perl_hook_run("TemplateReply", GRN, m, m1);
#endif
  if (ph_res < 1)
  {
    m->body = create_quoting(m1);  m->pos = 0;
  }
  if (m->pos >= strlen(m->body))  m->pos = strlen(m->body) - 1;

  mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_WND);

  if (mpw)  msgpost_widget_refresh(mpw, m);
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
  ph_res = perl_hook_run("TemplateForward", GRN, m, m1);
#endif
  if (ph_res < 1)
  {
    m->body = create_forward(m1);  m->pos = 0;
  }
  if (m->pos >= strlen(m->body))  m->pos = strlen(m->body) - 1;

  mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_WND);
  if (mpw)  msgpost_widget_refresh(mpw, m);
  else {
    gtk_text_insert(GTK_TEXT(w), NULL, NULL, NULL, m->body, strlen(m->body));
    gtk_text_set_point(GTK_TEXT(w), m->pos);
    gtk_editable_set_position(GTK_EDITABLE(w), m->pos);
  }
}

static void mpw_focus(GtkWindow *wnd, gpointer data)
{
  GRN->focused = wnd;
}
static void mpw_destroy(GtkWidget *wnd, gpointer data)
{
  GRN->focused = NULL;
}

GtkWidget *create_msgpost_widget(msg_create_mode mcm)
{
  GtkWidget *vbox, *wnd, *text, *bar, *table, *cntr;
  GnomeDockItem *item;
  GtkAdjustment *adj;
  GtkSignalFunc text_entered_func = NULL;
  
  grn_lock();
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), create_msgpost_pane(vbox), FALSE, FALSE, 0);
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
  
  wnd = gnome_app_new("GRN_post", _("GRN post article"));
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_WND, wnd);
  gtk_object_set_data(GTK_OBJECT(wnd), KEY_MSGPOST, vbox);
  gtk_window_set_default_size(GTK_WINDOW(wnd), grn_prefs.ws[2].width,
                              grn_prefs.ws[2].height);
  gtk_widget_set_uposition(GTK_WIDGET(wnd), grn_prefs.ws[2].x, grn_prefs.ws[2].y);
  cntr = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(cntr), GTK_SHADOW_ETCHED_OUT);
  gtk_container_add(GTK_CONTAINER(cntr), vbox);
  gtk_widget_show(cntr);
  gnome_app_set_contents(GNOME_APP(wnd), cntr);
  gnome_app_create_menus(GNOME_APP(wnd), msgpost_menu);
  gnome_app_create_toolbar(GNOME_APP(wnd), msgpost_toolbar);
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
      gchar *s1 = _("Article");
      grn_menu_set_data(bar, s1, _("Post"), KEY_MSGPOST, vbox);
      grn_menu_set_data(bar, s1, _("Cancel"), KEY_MSGPOST, vbox);
      grn_menu_set_data(bar, s1, _("Newsgroups"), KEY_MSGPOST, vbox);
      grn_menu_set_data(bar, s1, _("Edit headers"), KEY_MSGPOST, vbox);
      grn_menu_set_data(bar, s1, _("Attachments..."), KEY_MSGPOST, vbox);
      s1 = _("File");
      grn_menu_set_data(bar, s1, _("Load..."), KEY_MSGPOST, vbox);
    }
  }
  
  switch (mcm)
  {
    case MSG_CREATE_FOLLOWUP:
      text_entered_func = GTK_SIGNAL_FUNC(text_entered_followup);
      break;
    case MSG_CREATE_FORWARD:
      text_entered_func = GTK_SIGNAL_FUNC(text_entered_forward);
      break;
    default:
      text_entered_func = GTK_SIGNAL_FUNC(text_entered_new);
  }

  gtk_signal_connect(GTK_OBJECT(wnd), "delete_event",
                     GTK_SIGNAL_FUNC(delete_wnd), NULL);
  gtk_signal_connect(GTK_OBJECT(wnd), "key_press_event",
                     GTK_SIGNAL_FUNC(evt_msgpost_keypress), vbox);
  gtk_signal_connect(GTK_OBJECT(wnd), "focus_in_event",
                     GTK_SIGNAL_FUNC(mpw_focus), NULL);
  gtk_signal_connect(GTK_OBJECT(wnd), "destroy",
                     GTK_SIGNAL_FUNC(mpw_destroy), NULL);
  if (text_entered_func)
    gtk_signal_connect(GTK_OBJECT(text), "focus_in_event",
                       text_entered_func, NULL);

  gtk_widget_show_all(wnd);
  grn_unlock();
  build_macros_menu(GNOME_APP(wnd)->menubar, 3);
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

static GtkWidget *msgpost_set_hdr(GtkWidget *mw, gchar *id, gchar **text,
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

void msgpost_widget_refresh(GtkWidget *mpw, t_message *msg)
{
  GtkText *text;
  g_return_if_fail(mpw != NULL);
  g_return_if_fail(msg != NULL);
  
  msgpost_set_hdr(mpw, KEY_MW_FROM, &(msg->mh->from), FALSE);
  msgpost_set_hdr(mpw, KEY_MW_NEWSGROUPS, &(msg->mh->newsgroups), FALSE);
  msgpost_set_hdr(mpw, KEY_MW_SUBJ, &(msg->mh->subject), FALSE);
  
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

void msgpost_widget_set_data(GtkWidget *mpw, t_message *msg, t_message *msg1)
{
  GtkText *text;
  GtkWidget *w;
  g_return_if_fail(mpw != NULL);
  g_return_if_fail(msg != NULL);
  
  replace_msg(GTK_OBJECT(mpw), msg, KEY_MSG);
  
  msgpost_set_hdr(mpw, KEY_MW_FROM, &(msg->mh->from), TRUE);
  msgpost_set_hdr(mpw, KEY_MW_NEWSGROUPS, &(msg->mh->newsgroups), TRUE);
  w = msgpost_set_hdr(mpw, KEY_MW_SUBJ, &(msg->mh->subject), TRUE);
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
  
  w = gtk_object_get_data(GTK_OBJECT(mpw), KEY_MW_SUBJ);
  if (w)  gtk_widget_grab_focus(w);
  grn_unlock();
}


static t_message *msgpost_get_message(GtkWidget *mpw)
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
  w = gtk_object_get_data(GTK_OBJECT(mpw), KEY_MW_NEWSGROUPS);
  if (w)
  {
    str_free(&(m->mh->newsgroups));
    m->mh->newsgroups = g_strdup(gtk_entry_get_text(GTK_ENTRY(w)));
  }
  w = gtk_object_get_data(GTK_OBJECT(mpw), KEY_MW_SUBJ);
  if (w)
  {
    str_free(&(m->mh->subject));
    m->mh->subject = g_strdup(gtk_entry_get_text(GTK_ENTRY(w)));
  }
  
  return m;
}


void evt_post_new(GtkWidget *w)
{
  t_message *m;
  GtkWidget *mpw;
  g_return_if_fail(w != NULL);

  if (! GRN->srv.posting)
  {
    grn_error(NULL, _("Posting disabled by the server"));
    return;
  }
  m = t_message_alloc();  m->mh = t_msgheaders_alloc();
  m->mh->newsgroups = get_cur_group(w);
  fill_msg_headers(m);
  mpw = create_msgpost_widget(MSG_CREATE_NEW);
  msgpost_widget_set_data(mpw, m, NULL);
}

void evt_post_followup(GtkWidget *w)
{
  t_message *m, *m1;
  GtkWidget *mpw;
  g_return_if_fail(w != NULL);

  if (! GRN->srv.posting)
  {
    grn_error(NULL, _("Posting disabled by the server"));
    return;
  }
  m = t_message_alloc();  m->mh = t_msgheaders_alloc();
  m1 = get_cur_msg(w);
  m->mh->newsgroups = get_cur_group(w);
  fill_msg_headers(m);

  m->mh->subject = g_strdup(m1->mh->subject);
  if (m1->mh->ref)  m->mh->ref = g_strconcat(m1->mh->ref, " ", NULL);
  str_add(&(m->mh->ref), m1->mh->id);

  mpw = create_msgpost_widget(MSG_CREATE_FOLLOWUP);
  msgpost_widget_set_data(mpw, m, m1);
}

void evt_post_forward(GtkWidget *w)
{
  t_message *m, *m1;
  GtkWidget *mpw;
  g_return_if_fail(w != NULL);

  if (! GRN->srv.posting)
  {
    grn_error(NULL, _("Posting disabled by the server"));
    return;
  }
  m = t_message_alloc();  m->mh = t_msgheaders_alloc();
  m1 = get_cur_msg(w);
  m->mh->newsgroups = get_cur_group(w);
  fill_msg_headers(m);

  m->mh->subject = g_strdup(m1->mh->subject);

  mpw = create_msgpost_widget(MSG_CREATE_FORWARD);
  msgpost_widget_set_data(mpw, m, m1);
}


typedef struct sg_data
{
  GtkWidget **dlg;
  GtkWidget *btn;
  t_message *m;
} sg_data;

static void sg_ok(GtkWidget *w, sg_data *sgd)
{
  GtkWidget *gs, *clist, *en;
  GList *l;
  gchar *s = NULL;
  
  if ((! sgd->m) || (! sgd->m->mh))  return;
  en = gtk_object_get_data(GTK_OBJECT(*(sgd->dlg)), KEY_MW_NEWSGROUPS);
  if (! en)  return;
  gs = gtk_object_get_data(GTK_OBJECT(*(sgd->dlg)), KEY_GROUPSEL);
  if (! gs)  return;
  clist = gtk_object_get_data(GTK_OBJECT(gs), KEY_CL_GROUPS);
  if (! clist)  return;
  
  for (l=GTK_CLIST(clist)->selection; l; l=l->next)
  {
    gint row = GPOINTER_TO_INT(l->data);
    gchar *text;
    
    gtk_clist_get_text(GTK_CLIST(clist), row, 3, &text);
    if (s)  str_add(&s, ",");
    str_add(&s, text);
  }
  str_free(&(sgd->m->mh->newsgroups));
  if (s)
  {
    gtk_entry_set_text(GTK_ENTRY(en), s);
    sgd->m->mh->newsgroups = g_strdup(s);
    str_free(&s);
  }
  else  gtk_entry_set_text(GTK_ENTRY(en), "");
}

static void sg_destroy(GtkWidget *w, sg_data *sgd)
{
  *(sgd->dlg) = NULL;
  gtk_widget_set_sensitive(sgd->btn, TRUE);
  g_free(sgd);
}

void evt_sel_groups(GtkWidget *w)
{
  GtkWidget *mpw, *en;
  static GtkWidget *dlg = NULL;
  GSList *ngl, *l;
  sg_data *sgd;
  if (! w->parent)  return;
  if (dlg)  return;

  mpw = gtk_object_get_data(GTK_OBJECT(w->parent), KEY_MSGPOST);
  if (! mpw)  mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_MSGPOST);
  if (! mpw)  return;
  
  en = gtk_object_get_data(GTK_OBJECT(mpw), KEY_MW_NEWSGROUPS);
  if (! en)  return;
  ngl = newsgroups2list(gtk_entry_get_text(GTK_ENTRY(en)));
  dlg = groupsel_dlg_create(ngl);
  for (l=ngl; l; l=l->next)  g_free(l->data);
  g_slist_free(ngl);
  
  gtk_widget_set_sensitive(w, FALSE);
  sgd = g_new(sg_data, 1);
  sgd->dlg = &dlg;  sgd->btn = w;
  sgd->m = msgpost_get_message(mpw);
  gtk_object_set_data(GTK_OBJECT(dlg), KEY_MW_NEWSGROUPS, en);
  gnome_dialog_button_connect(GNOME_DIALOG(dlg), 1,
                              GTK_SIGNAL_FUNC(sg_ok), sgd);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     GTK_SIGNAL_FUNC(sg_destroy), sgd);
}


typedef struct mhe_data
{
  GtkWidget **dlg;
  GtkWidget *btn;
} mhe_data;

static void mhe_destroy(GtkWidget *w, mhe_data *mhd)
{
  *(mhd->dlg) = NULL;
  gtk_widget_set_sensitive(mhd->btn, TRUE);
  g_free(mhd);
}
void evt_mhdredit_dlg_create(GtkWidget *w)
{
  GtkWidget *mpw;
  static GtkWidget *dlg = NULL;
  t_message *m;
  mhe_data *mhd;
  if (! w->parent)  return;
  if (dlg)  return;

  mpw = gtk_object_get_data(GTK_OBJECT(w->parent), KEY_MSGPOST);
  if (! mpw)  mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_MSGPOST);
  if (! mpw)  return;
  m = gtk_object_get_data(GTK_OBJECT(mpw), KEY_MSG);
  if (! m)  return;
  dlg = mhdredit_dlg_create(m->mh);

  gtk_widget_set_sensitive(w, FALSE);
  mhd = g_new(mhe_data, 1);
  mhd->dlg = &dlg;  mhd->btn = w;
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     GTK_SIGNAL_FUNC(mhe_destroy), mhd);
}


typedef struct post_data
{
  t_message *m;
  GtkWidget *wnd;
} post_data;

static void warn_cb(gint reply, post_data *pd)
{
  g_return_if_fail(pd != NULL);
  if (reply == GNOME_YES)
  {
#ifdef USE_PERL
    perl_hook_run("PrePost", GRN, pd->m, NULL);
#endif
    if (pd->m)  grnq_add_post_article(pd->m);
    if (pd->wnd)  gtk_widget_destroy(pd->wnd);
  }
  else  t_message_free(&(pd->m));
  g_free(pd);
}

void evt_post_ok(GtkWidget *w)
{
  GtkWidget *mpw, *wnd;
  t_message *m;
  GSList *gl;
  msg_goodness mg;
  if (flags.post_article)  return;
  if (! w->parent)  return;

  mpw = gtk_object_get_data(GTK_OBJECT(w->parent), KEY_MSGPOST);
  if (! mpw)  mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_MSGPOST);
  if (! mpw)  return;
  
  m = t_message_copy(msgpost_get_message(mpw));
  wnd = gtk_object_get_data(GTK_OBJECT(mpw), KEY_WND);
  
  gl = msg_check(m);
  mg = get_goodness(gl);
  if (mg == BAD)
  {
    gchar *text1 = goodness_list_to_str(gl, BAD);
    gchar *text2 = goodness_list_to_str(gl, WARN);
    gchar *text = g_strdup(_("Posting abandoned!\n\n"));
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
    grn_error(NULL, text);
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
      post_data *pd = g_new(post_data, 1);
      str_add(&text, text1);  str_free(&text1);
      str_add(&text, _("\nDo you wish to continue posting?"));
      pd->m = m;  pd->wnd = wnd;
      gnome_app_question(GNOME_APP(GRN->window), text,
        (GnomeReplyCallback) warn_cb, pd);
      str_free(&text);  goodness_list_free(gl);
      return;
    }
  }
  goodness_list_free(gl);
#ifdef USE_PERL
  perl_hook_run("PrePost", GRN, m, NULL);
#endif
  if (m)  grnq_add_post_article(m);
  if (wnd)  gtk_widget_destroy(wnd);
}

void evt_post_cancel(GtkWidget *w)
{
  GtkWidget *mpw, *wnd;
  if (! w->parent)  return;
  
  mpw = gtk_object_get_data(GTK_OBJECT(w->parent), KEY_MSGPOST);
  if (! mpw)  mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_MSGPOST);
  if (! mpw)  return;
  wnd = gtk_object_get_data(GTK_OBJECT(mpw), KEY_WND);
  if (wnd)
  {
    if (grn_prefs.cfms[4])
      gnome_app_question(GNOME_APP(GRN->window), _("Are you sure you wish to abort posting?"),
        (GnomeReplyCallback) abort_cb, wnd);
    else  gtk_widget_destroy(wnd);
  }
}


static void load_file_ok(GtkWidget *w, GtkFileSelection *fs)
{
  FILE *f;
  GtkText *text;
  gchar *fn = gtk_file_selection_get_filename(fs);

  f = fopen(fn, "r");
  if (f)
  {
    gchar *s = g_malloc(8192);
    
    text = gtk_object_get_data(GTK_OBJECT(fs), KEY_TXT_BODY);
    if (text)
    {
      gtk_text_freeze(text);
      while (! feof(f))
      {
        s[0] = '\0';
        fgets(s, 8190, f);
        gtk_text_insert(text, NULL, NULL, NULL, s, strlen(s));
      }
      gtk_text_thaw(text);
      str_free(&s);
    }
    else  g_warning("load_file_ok(): internal error");
    fclose(f);
  }
  else  grn_error(fn, _("Can't open file"));
  gtk_widget_destroy(GTK_WIDGET(fs));
}
static void load_file_cancel(GtkWidget *w, GtkFileSelection *fs)
{
  gtk_widget_destroy(GTK_WIDGET(fs));
}
void evt_compose_load_file(GtkWidget *w)
{
  GtkWidget *mpw, *fs, *text;
  if (! w->parent)  return;
  
  mpw = gtk_object_get_data(GTK_OBJECT(w->parent), KEY_MSGPOST);
  if (! mpw)  mpw = gtk_object_get_data(GTK_OBJECT(w), KEY_MSGPOST);
  if (! mpw)  return;
  text = gtk_object_get_data(GTK_OBJECT(mpw), KEY_TXT_BODY);
  if (! text)  return;
  
  fs = gtk_file_selection_new(_("Load file"));
  gtk_object_set_data(GTK_OBJECT(fs), KEY_TXT_BODY, text);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button), "clicked",
		     GTK_SIGNAL_FUNC(load_file_ok), fs);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button), "clicked",
		     GTK_SIGNAL_FUNC(load_file_cancel), NULL);
  gtk_widget_show(fs);
}
