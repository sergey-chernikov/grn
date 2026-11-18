/*
 * grn_nglist.c: newsgroups window
 *
 * $Id: grn_nglist.c,v 1.38 2000/06/28 11:28:28 sc Exp $
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
#include <regex.h>
#include "grn_nglist.h"
#include "grn_menu.h"
#include "grn_vars.h"
#include "grn_util.h"
#include "grn_threadlist.h"
#include "grn_news.h"
#include "grn_config.h"
#include "queue.h"
#include "nntp.h"
#include "grn_misc.h"
#include "grn_commands.h"

#define NCOLS 5


static gint evt_ng_btnpress(GtkWidget *, GdkEventButton *, gpointer);
static gint evt_ng_keypress(GtkWidget *, GdkEventKey *, gpointer);
static gint evt_fg_keypress(GtkWidget *, GdkEventKey *, gpointer);
static void evt_ng_delete(GtkWidget *, gpointer);
static void evt_ng_hide(GtkWidget *, gpointer);
static void evt_ng_show(GtkWidget *, gpointer);
static void evt_click_col(GtkCList *, int);
static void en_find_changed(GtkWidget *, GtkWidget *);
static void en_find_activate(GtkWidget *, GtkWidget *);
static void ng_list_sort(GtkCList *);


static gboolean add_group(GtkCList *clist, grn_newsgroup *grp, gboolean flag)
{
  GdkPixmap *pixmap=NULL;
  GdkBitmap *mask=NULL;
  gint i, idx;
  gchar *grp_col[NCOLS];
    
  if (grp->seq && newsrc_ht && (grp->nunread < 0))
    grp->nunread = grn_news_get_unread(grp);
    
  if ((grn_prefs.srt_ng_subscr >= 1) && (! grp->seq) && flag)  return FALSE;
  if ((grn_prefs.srt_ng_subscr >= 2) && (grp->nunread <= 0) && flag)  return FALSE;

  if (grp->nunread > 0)
  {
    pixmap = GRN->pixmaps.new_art_group.pixmap;
    mask = GRN->pixmaps.new_art_group.mask;
  }
  else if (grp->seq)
  {
    pixmap = GRN->pixmaps.subscr_group.pixmap;
    mask = GRN->pixmaps.subscr_group.mask;
  }
        
  grp_col[0] = NULL;
  grp_col[1] = NULL;

  if (grp->nunread > 0)  grp_col[2] = g_strdup_printf("%lu", grp->nunread);
  else  grp_col[2] = NULL;

  if (grp->name)  grp_col[3] = g_strdup(grp->name);
  else  grp_col[3] = NULL;
  if (grp->descr)  grp_col[4] = g_strdup(grp->descr);
  else  grp_col[4] = NULL;
    
  grn_lock();
  idx = gtk_clist_append(clist, grp_col);
  gtk_clist_set_row_data(clist, idx, grp);
  if (pixmap)  gtk_clist_set_pixmap(clist, idx, 1, pixmap, mask);
  grn_unlock();

  for (i=0; i<NCOLS; i++)  str_free(&grp_col[i]);
  return TRUE;
}

void grouplist_refresh(GtkWidget *grouplist, GList *ng_list)
{
  GtkCList *clist;
  GList *l;
  g_return_if_fail(grouplist != NULL);
  g_return_if_fail(ng_list != NULL);
  
  grn_lock();
  clist = GTK_CLIST(gtk_object_get_data(GTK_OBJECT(grouplist), KEY_CL_GROUPS));
  grn_unlock();
  if ((! clist) || (! GTK_IS_CLIST(clist)))  return;
  
  grn_widget_set_style(GTK_WIDGET(clist), grn_prefs.fonts[4], NULL, NULL, NULL);
  grn_lock();
  gtk_clist_freeze(clist);
  gtk_clist_clear(clist);
  grn_unlock();
  for (l=ng_list; l; l=l->next)
  {
    grn_newsgroup *grp = (grn_newsgroup *) l->data;
    if (! add_group(clist, grp, TRUE))  continue;
  }
  grn_lock();
  gtk_clist_columns_autosize(clist);
  grn_unlock();
  ng_list_sort(clist);
  grn_lock();
  gtk_clist_thaw(clist);
  grn_unlock();
  grn_appbar_set_status(GRN->appbar, _("Total %ld group[s]; %ld group[s] with unread messages"),
                        g_list_length(ng_list), grouplist_get_nunread(ng_list));
}

static void grouplist_refresh_row(GtkCList *clist, gint row, grn_newsgroup *grp)
{
  GdkPixmap *pixmap=NULL;
  GdkBitmap *mask=NULL;
  gchar *s_tmp = NULL;

  if (flags.open_grouplist)  return;
  if (grp->nunread > 0)
  {
    pixmap = GRN->pixmaps.new_art_group.pixmap;
    mask = GRN->pixmaps.new_art_group.mask;
    s_tmp = g_strdup_printf("%lu", grp->nunread);
   }
  else if (grp->seq)
  {
    pixmap = GRN->pixmaps.subscr_group.pixmap;
    mask = GRN->pixmaps.subscr_group.mask;
  }
  grn_lock();
  if (pixmap)  gtk_clist_set_pixmap(clist, row, 1, pixmap, mask);
  else  gtk_clist_set_text(clist, row, 1, "");
  if (s_tmp)
  {
    gtk_clist_set_text(clist, row, 2, s_tmp);
    str_free(&s_tmp);
  }
  else  gtk_clist_set_text(clist, row, 2, "");
  grn_unlock();
}

void grouplist_recalc_unread(GtkWidget *grouplist)
{
  gint i;
  GtkCList *clist;
  g_return_if_fail(grouplist != NULL);
  
  grn_lock();
  clist = GTK_CLIST(gtk_object_get_data(GTK_OBJECT(grouplist), KEY_CL_GROUPS));
  grn_unlock();
  if (! clist)  return;
  if (! clist->selection)  return;

  i = GPOINTER_TO_INT(clist->selection->data);
  {
    grn_newsgroup *grp;
    grn_lock();
    grp = (grn_newsgroup *) gtk_clist_get_row_data(clist, i);
    grn_unlock();
    if (grp)  grouplist_refresh_row(clist, i, grp);
  }
  grn_appbar_set_status(GRN->appbar, _("Total %ld group[s]; %ld group[s] with unread messages"),
                        g_list_length(GRN->ng_list), grouplist_get_nunread(GRN->ng_list));
}

static GtkWidget *grn_get_filt_ball(void)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkWidget *ret;

  if (grn_prefs.srt_ng_subscr == 1)
  {
    pixmap = GRN->pixmaps.subscr_group.pixmap;
    mask = GRN->pixmaps.subscr_group.mask;
  }
  else if (grn_prefs.srt_ng_subscr == 2)
  {
    pixmap = GRN->pixmaps.new_art_group.pixmap;
    mask = GRN->pixmaps.new_art_group.mask;
  }
  else {
    pixmap = GRN->pixmaps.caption_subscr.pixmap;
    mask = GRN->pixmaps.caption_subscr.mask;
  }
  grn_lock();
  ret = gtk_pixmap_new(pixmap, mask);
  grn_unlock();
  return ret;
}

GtkWidget *grouplist_create()
{
  gchar *ttl[NCOLS] = {_("No"), "", _("Unread"), _("Name"), _("Description")};
  GtkWidget *vbox, *scrwin, *hbox, *w, *en_find;
  GtkCList *clist;
  gchar *title;
  gshort n;
  
  grn_lock();
  vbox = gtk_vbox_new(FALSE, 0);
  clist = GTK_CLIST(gtk_clist_new_with_titles(NCOLS, ttl));
  gtk_clist_set_column_justification(clist, 0, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_justification(clist, 1, GTK_JUSTIFY_CENTER);
  gtk_clist_set_column_justification(clist, 2, GTK_JUSTIFY_RIGHT);
  gtk_clist_column_title_passive(clist, 0);
  gtk_clist_column_title_passive(clist, 4);
  gtk_tooltips_set_tip(GRN->tooltips, clist->column[1].button, _("Toggle all/subscribed/unread groups"), NULL);
  gtk_tooltips_set_tip(GRN->tooltips, clist->column[2].button, _("Sort by unread messages"), NULL);
  gtk_tooltips_set_tip(GRN->tooltips, clist->column[3].button, _("Sort by name"), NULL);
  gtk_clist_set_selection_mode(clist, GTK_SELECTION_BROWSE);
  gtk_clist_set_reorderable(clist, TRUE);

  n = grn_prefs.srt_ng_col;
  clist->sort_type = GTK_SORT_ASCENDING;
  if (n >= 100)
  {
    clist->sort_type = GTK_SORT_DESCENDING;
    n -= 100;
  }
  gtk_clist_set_sort_column(clist, n);
  gtk_clist_set_column_widget(clist, 1, grn_get_filt_ball());
  
  scrwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrwin), GTK_WIDGET(clist));
  gtk_box_pack_start(GTK_BOX(vbox), scrwin, TRUE, TRUE, 0);
  
  hbox = gtk_hbox_new(FALSE, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  w = gtk_label_new(_("Find newsgroup:"));
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);
  en_find = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), en_find, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(en_find), "key_press_event",
                     GTK_SIGNAL_FUNC(evt_fg_keypress), NULL);

  title = g_strdup_printf(_(GRN_TITLE" - %s"), _("Groups listing"));

  gtk_object_set_data(GTK_OBJECT(vbox), KEY_CL_GROUPS, clist);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_FOCUS, clist);
  gtk_object_set_data_full(GTK_OBJECT(vbox), KEY_TITLE, title,
                           (GtkDestroyNotify) str_destroy);

  gtk_signal_connect(GTK_OBJECT(clist), "key_press_event",
                     GTK_SIGNAL_FUNC(evt_ng_keypress), NULL);
  gtk_signal_connect(GTK_OBJECT(clist), "button_press_event",
                     GTK_SIGNAL_FUNC(evt_ng_btnpress), NULL);
  gtk_signal_connect(GTK_OBJECT(clist), "click_column",
                     GTK_SIGNAL_FUNC(evt_click_col), NULL);
  gtk_signal_connect(GTK_OBJECT(en_find), "changed",
                     GTK_SIGNAL_FUNC(en_find_changed), clist);
  gtk_signal_connect(GTK_OBJECT(en_find), "activate",
                     GTK_SIGNAL_FUNC(en_find_activate), clist);
  gtk_signal_connect(GTK_OBJECT(vbox), "destroy",
                     GTK_SIGNAL_FUNC(evt_ng_delete), NULL);
  gtk_signal_connect(GTK_OBJECT(vbox), "hide",
                     GTK_SIGNAL_FUNC(evt_ng_hide), NULL);
  gtk_widget_show_all(vbox);
  gtk_signal_connect(GTK_OBJECT(vbox), "show",
                     GTK_SIGNAL_FUNC(evt_ng_show), NULL);
  grn_unlock();
  show_newsgroups_menu(TRUE);

  return vbox;
}

void ng_list_download(gboolean update)
{
  GList *grouplist;
  gchar *rc;
  
  if (GRN->ng_list)  grouplist_free(&(GRN->ng_list));
  grn_appbar_set_status(GRN->appbar, "");
  grn_appbar_push(GRN->appbar, update, _("Connecting to server '%s'..."), grn_prefs.nntp_server);
  rc = nntp_connect(GRN->nntp_sock);
  if (rc)  grn_error(_("NNTP connect"), rc);
  grn_appbar_pop(GRN->appbar);
  if (! nntp_is_online(GRN->nntp_sock))
  {
    grn_appbar_set_status(GRN->appbar, _("Connection to server '%s' failed"), grn_prefs.nntp_server);
    return;
  }
  grn_appbar_push(GRN->appbar, update, _("Fetching grouplist from server..."));
  if ((grouplist = nntp_get_grouplist(GRN->nntp_sock)) != NULL)
  {
    GRN->ng_list = grouplist;
    grn_appbar_pop(GRN->appbar);
    grn_appbar_push(GRN->appbar, update, _("Reading newsrc file..."));
    if (! newsrc_ht)  newsrc_ht = read_newsrc(grn_prefs.paths[0]);
    grn_appbar_pop(GRN->appbar);
    grn_appbar_push(GRN->appbar, update, _("Calculating groups status..."));
    if (newsrc_ht)  fill_newsrc_ng_list(GRN->ng_list, newsrc_ht);
    grn_appbar_pop(GRN->appbar);
    grn_appbar_push(GRN->appbar, update, _("Fetching newsgroups descriptions from server..."));
    if (! ng_desc_ht)  ng_desc_ht = nntp_get_groupdesc(GRN->nntp_sock);
    if (ng_desc_ht)  fill_newsrc_ng_desc(GRN->ng_list, ng_desc_ht);
    else {
      if (GRN->nntp_sock->error)
        grn_error(_("Socket error"), GRN->nntp_sock->err_msg);
    }
  }
  else {
    if (GRN->nntp_sock->error)
      grn_error(_("Socket error"), GRN->nntp_sock->err_msg);
  }
  grn_appbar_pop(GRN->appbar);
}


void clist_renumber(GtkCList *clist)
{
  gint i;
  for (i=0; i<clist->rows; i++)
  {
    gchar *buf = g_strdup_printf("%d", i+1);
    grn_lock();
    gtk_clist_set_text(clist, i, 0, buf);
    grn_unlock();
    str_free(&buf);
  }
}
static grn_newsgroup *get_newsgroup_from_row(gconstpointer ptr)
{
  const GtkCListRow *row = (GtkCListRow *) ptr;
  if (row->data)  return (grn_newsgroup *) row->data;
  else return NULL;
}
static gint ng_list_compare_unread(GtkCList *clist, gconstpointer a, gconstpointer b)
{
  grn_newsgroup *grp1, *grp2;
  g_return_val_if_fail(a != NULL, -1);
  g_return_val_if_fail(b != NULL, 1);
  
  grp1 = get_newsgroup_from_row(a);
  grp2 = get_newsgroup_from_row(b);

  if (! grp2)  return (grp1 != NULL);
  if (! grp1)  return (-1);
  if (grp2->nunread < 0)  return (grp1->nunread >= 0);
  if (grp1->nunread < 0)  return (-1);
  if (grp2->nunread == 0)  return (grp1->nunread >= 0);
  if (grp1->nunread == 0)  return (-1);
  return (grp1->nunread - grp2->nunread);
}
static gint ng_list_compare_name(GtkCList *clist, gconstpointer a, gconstpointer b)
{
  grn_newsgroup *grp1, *grp2;
  g_return_val_if_fail(a != NULL, -1);
  g_return_val_if_fail(b != NULL, -1);

  grp1 = get_newsgroup_from_row(a);
  grp2 = get_newsgroup_from_row(b);

  if (! grp2)  return (grp1 != NULL);
  if (! grp1)  return (-1);
  if (! grp2->name)  return (grp1->name != NULL);
  if (! grp1->name)  return (-1);
  return (g_strcasecmp(grp1->name, grp2->name));
}
static void ng_list_sort(GtkCList *clist)
{
  g_return_if_fail(clist != NULL);

  grn_lock();
  switch (clist->sort_column)
  {
    case 2:
      gtk_clist_set_compare_func(clist, (GtkCListCompareFunc) ng_list_compare_unread);
      break;
    case 3:
      gtk_clist_set_compare_func(clist, (GtkCListCompareFunc) ng_list_compare_name);
      break;
    default:
      g_assert_not_reached();
      break;
  }
  gtk_clist_sort(clist);
  clist_renumber(clist);
  if (GTK_WIDGET_REALIZED(clist))
  {
    gtk_clist_select_row(clist, 0, 0);
    gtk_widget_grab_focus(GTK_WIDGET(clist));
  }
  grn_unlock();
  grn_prefs.srt_ng_col = clist->sort_column;
  if (clist->sort_type == GTK_SORT_DESCENDING)  grn_prefs.srt_ng_col += 100;
}

static GtkCList *get_clist(GtkWidget *w)
{
  GtkCList *clist;

  g_return_val_if_fail(w != NULL, NULL);
  if (! GTK_IS_CLIST(w))
  {
    clist = (GtkCList *) gtk_object_get_data(GTK_OBJECT(GRN->grouplist), KEY_CL_GROUPS);
    g_return_val_if_fail(clist != NULL, NULL);
    g_return_val_if_fail(GTK_IS_CLIST(clist), NULL);
  }
  else  clist = GTK_CLIST(w);
  return clist;
}

extern pthread_mutex_t groups_m;
void ng_list_open_group(GtkWidget *w)
{
  gint row;
  grn_newsgroup *grp;
  GtkCList *clist = get_clist(w);
  g_return_if_fail(clist != NULL);

  if (flags.open_group)  return;
  if ((! clist) || (! clist->selection))  return;
  pthread_mutex_lock(&groups_m);
  row = GPOINTER_TO_INT(clist->selection->data);
  grp = gtk_clist_get_row_data(clist, row);
  if (grp)
  {
    gchar *title = g_strdup_printf(_("%s - %s - %s"), GRN_TITLE, _("Threadlist"),
                                   grp->name);

    if (GRN->threadlist)  gtk_widget_destroy(GRN->threadlist);
    GRN->threadlist = threadlist_create();
    gtk_object_set_data_full(GTK_OBJECT(GRN->threadlist), KEY_TITLE, title,
                             (GtkDestroyNotify) str_destroy);
    grn_set_contents(GRN->threadlist);
    grnq_add_download_msghdrlist(grp);
    grnq_add_draw_msglist();
  }
  pthread_mutex_unlock(&groups_m);
}

static gint evt_ng_btnpress(GtkWidget *w, GdkEventButton *event, gpointer data)
{
  gboolean processed = FALSE;
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(w != NULL, FALSE);
  
  if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
  {
    ng_list_open_group(w);
    processed = TRUE;
  }
  if (processed)
  {
    gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "button_press_event");
    return TRUE;
  }
  return FALSE;
}

static gint evt_ng_keypress(GtkWidget *w, GdkEventKey *event, gpointer data)
{
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(w != NULL, FALSE);

  switch (event->keyval)
  {
    case GDK_Select:
    case GDK_Execute:
    case GDK_Return:
    case GDK_KP_Enter:
    case GDK_Right:
    case GDK_KP_Right:
      ng_list_open_group(w);
      break;
    case GDK_Up:
      if (event->state & GDK_CONTROL_MASK)  evt_group_urprev(w);
      else  return FALSE;
      break;
    case GDK_Down:
      if (event->state & GDK_CONTROL_MASK)  evt_group_urnext(w);
      else  return FALSE;
      break;
    default:  return FALSE;
  }
  gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "key_press_event");
  return TRUE;
}

static void evt_ng_delete(GtkWidget *w, gpointer data)
{
  g_return_if_fail(w != NULL);
  GRN->grouplist = NULL;
}
  
static void evt_ng_show(GtkWidget *w, gpointer data)
{
  if (GRN->ng_list)
    grn_appbar_set_status(GRN->appbar, _("Total %ld groups; %ld groups with unread messages"),
                          g_list_length(GRN->ng_list), grouplist_get_nunread(GRN->ng_list));
  show_newsgroups_menu(TRUE);
  grnq_add_grouplist_recalc();
}

static void evt_ng_hide(GtkWidget *w, gpointer data)
{
  grn_appbar_set_status(GRN->appbar, "");
  show_newsgroups_menu(FALSE);
}

static void evt_click_col(GtkCList *clist, int n)
{
  if (n == 1)
  {
    grn_prefs.srt_ng_subscr++;
    if (grn_prefs.srt_ng_subscr > 2)  grn_prefs.srt_ng_subscr = 0;
    grouplist_refresh(GRN->grouplist, GRN->ng_list);
    ng_list_sort(clist);
    grn_lock();
    gtk_clist_set_column_widget(clist, 1, grn_get_filt_ball());
    grn_unlock();
    return;
  }
  if (clist->sort_column == n)
  {
    if (clist->sort_type == GTK_SORT_ASCENDING)
      clist->sort_type = GTK_SORT_DESCENDING;
    else clist->sort_type = GTK_SORT_ASCENDING;
  }
  else clist->sort_type = GTK_SORT_ASCENDING;

  switch (n)
  {
    case 2:
    case 3:
      gtk_clist_set_sort_column(clist, n);
      break;
    default:
      g_assert_not_reached();
      break;
  }
  ng_list_sort(clist);
}

void evt_group_subscribe(GtkWidget *w)
{
  GList *l;
  GtkCList *clist = get_clist(w);
  g_return_if_fail(clist != NULL);

  for (l=clist->selection; l; l=l->next)
  {
    gint row = GPOINTER_TO_INT(l->data);
    grn_newsgroup *grp = gtk_clist_get_row_data(clist, row);
    if (grp && grp->name && newsrc_ht)
    {
      gchar *seq;
      grn_news_group_subscribe(newsrc_ht, grp);
      seq = g_hash_table_lookup(newsrc_ht, grp->name);
      if (seq)
      {
	grn_newsgroup_set_seq(grp, g_strdup(seq));
	grouplist_refresh_row(clist, row, grp);
	grnq_add_grouplist_recalc();
      }
    }
  }
}

void evt_group_unsubscribe(GtkWidget *w)
{
  GList *l;
  GtkCList *clist = get_clist(w);
  g_return_if_fail(clist != NULL);

  for (l=clist->selection; l; l=l->next)
  {
    gint row = GPOINTER_TO_INT(l->data);
    grn_newsgroup *grp = gtk_clist_get_row_data(clist, row);
    if (grp && grp->name && newsrc_ht)
    {
      gchar *seq;
      grn_news_group_unsubscribe(newsrc_ht, grp);
      seq = g_hash_table_lookup(newsrc_ht, grp->name);
      if (! seq)
      {
        grn_newsgroup_set_seq(grp, NULL);
	grouplist_refresh_row(clist, row, grp);
	grnq_add_grouplist_recalc();
      }
    }
  }
}

typedef struct catchup_data
{
  grn_newsgroup *grp;
  gint row;
  GtkCList *clist;
} catchup_data;
static void catchup_cb(gint reply, catchup_data *cdata)
{
  g_return_if_fail(cdata != NULL);
  if (reply == GNOME_YES)
  {
    grn_newsgroup_set_seq(cdata->grp, grn_news_read_all(MAX(1, cdata->grp->total)));
    grouplist_refresh_row(cdata->clist, cdata->row, cdata->grp);
    grnq_add_grouplist_recalc();
  }
  g_free(cdata);
}
void evt_group_catchup(GtkWidget *w)
{
  GList *l;
  GtkCList *clist = get_clist(w);
  g_return_if_fail(clist != NULL);

  for (l=clist->selection; l; l=l->next)
  {
    gint row = GPOINTER_TO_INT(l->data);
    grn_newsgroup *grp = gtk_clist_get_row_data(clist, row);
    if (grp && grp->name && newsrc_ht)
    {
      gchar *seq;
      seq = g_hash_table_lookup(newsrc_ht, grp->name);
      if (seq)
      {
        if (grn_prefs.cfms[0])
	{
          catchup_data *cdata = g_new(catchup_data, 1);
	  gchar *buf = g_strdup_printf(_("Do you really wish to catchup group\n%s?"), grp->name);
	  cdata->grp = grp;  cdata->clist = clist;  cdata->row = row;
          gnome_app_question(GNOME_APP(GRN->window), buf,
	                     (GnomeReplyCallback) catchup_cb, cdata);
	  str_free(&buf);
	}
	else {
	  grn_newsgroup_set_seq(grp, grn_news_read_all(MAX(1, grp->total)));
	  grouplist_refresh_row(clist, row, grp);
	  grnq_add_grouplist_recalc();
        }
      }
    }
  }
}

static void unread_cb(gint reply, catchup_data *cdata)
{
  g_return_if_fail(cdata != NULL);
  if (reply == GNOME_YES)
  {
    grn_newsgroup_set_seq(cdata->grp, grn_news_read_all(MAX(1, cdata->grp->first - 1)));
    grouplist_refresh_row(cdata->clist, cdata->row, cdata->grp);
    grnq_add_grouplist_recalc();
  }
  g_free(cdata);
}
void evt_group_unread(GtkWidget *w)
{
  GList *l;
  GtkCList *clist = get_clist(w);
  g_return_if_fail(clist != NULL);

  for (l=clist->selection; l; l=l->next)
  {
    gint row = GPOINTER_TO_INT(l->data);
    grn_newsgroup *grp = gtk_clist_get_row_data(clist, row);
    if (grp && grp->name && newsrc_ht)
    {
      gchar *seq;
      seq = g_hash_table_lookup(newsrc_ht, grp->name);
      if (seq)
      {
        if (grn_prefs.cfms[1])
	{
          catchup_data *cdata = g_new(catchup_data, 1);
	  gchar *buf = g_strdup_printf(_("Do you really wish to mark group\n'%s' unread?"), grp->name);
	  cdata->grp = grp;  cdata->clist = clist;  cdata->row = row;
          gnome_app_question(GNOME_APP(GRN->window), buf,
	                     (GnomeReplyCallback) unread_cb, cdata);
	  str_free(&buf);
	}
	else {
	  grn_newsgroup_set_seq(grp, grn_news_read_all(MAX(1, grp->first - 1)));
	  grouplist_refresh_row(clist, row, grp);
	  grnq_add_grouplist_recalc();
	}
      }
    }
  }
}

static void group_next_prev(GtkWidget *w, gboolean prev)
{
  GtkCList *clist = get_clist(w);
  gint row;
  g_return_if_fail(clist != NULL);
  if (! clist->selection)  return;
  
  row = GPOINTER_TO_INT(clist->selection->data);
  if (prev)  row--;
  else  row++;

  while ((row >= 0) && (row < clist->rows))
  {
    grn_newsgroup *grp = gtk_clist_get_row_data(clist, row);
    if (grp->nunread > 0)
    {
      clist->focus_row = row;
      gtk_clist_select_row(clist, row, 0);
      if (gtk_clist_row_is_visible(clist, row) != GTK_VISIBILITY_FULL)
        gtk_clist_moveto(clist, row, 0, 0,0);
      break;
    }
    if (prev)  row--;
    else  row++;
  }
}

void evt_group_urprev(GtkWidget *w)
{  group_next_prev(w, TRUE);  }

void evt_group_urnext(GtkWidget *w)
{  group_next_prev(w, FALSE);  }


void evt_groups_refresh(GtkWidget *w)
{
  if (newsrc_ht)
  {
    write_newsrc(grn_prefs.paths[0], grn_prefs.paths[0], newsrc_ht);
    free_newsrc(newsrc_ht);  newsrc_ht = NULL;
  }
  if (GRN->ng_list)  grouplist_free(&(GRN->ng_list));
  if (! GRN->grouplist)  GRN->grouplist = grouplist_create();
  grn_set_contents(GRN->grouplist);
  grnq_add_download_grouplist();
  grnq_add_draw_grouplist();
//  ng_list_download(TRUE);
//  grouplist_refresh(GRN->grouplist, GRN->ng_list);
}


static void find_changed(GtkWidget *en, GtkWidget *clist, gboolean flag)
{
  const gchar *en_text = gtk_entry_get_text(GTK_ENTRY(en));
  gchar *buf;
  regex_t re;
  gint i;
  if (! str_check(en_text))  return;
  g_return_if_fail(en != NULL);
  g_return_if_fail(clist != NULL);
  g_return_if_fail(GTK_IS_CLIST(clist));
  
  buf = g_strconcat("^", en_text, NULL);
  if (regcomp(&re, buf, REG_EXTENDED|REG_NOSUB|REG_ICASE))
  {
    str_free(&buf);  return;
  }
  if (flag)  flags.open_group = TRUE;
  for (i=0; i<GTK_CLIST(clist)->rows; i++)
  {
    gchar *text;
    gtk_clist_get_text(GTK_CLIST(clist), i, 3, &text);
    if (! regexec(&re, text, 0, NULL, 0))
    {
      if (flag)  gtk_clist_select_row(GTK_CLIST(clist), i, 3);
      else  GTK_CLIST(clist)->focus_row = i;
      if (gtk_clist_row_is_visible(GTK_CLIST(clist), i) != GTK_VISIBILITY_FULL)
        gtk_clist_moveto(GTK_CLIST(clist), i, 0, 0,0);
      break;
    }
  }
  if (flag)  flags.open_group = FALSE;
}

static void en_find_changed(GtkWidget *en, GtkWidget *clist)
{
  if (flags.open_group)  return;
  find_changed(en, clist, TRUE);
}

static void gs_find_changed(GtkWidget *en, GtkWidget *clist)
{
  find_changed(en, clist, FALSE);
}

static void en_find_activate(GtkWidget *en, GtkWidget *clist)
{
  if (flags.open_group)  return;
  g_return_if_fail(clist != NULL);
  ng_list_open_group(clist);
}

static void gs_find_activate(GtkWidget *en, GtkWidget *clist)
{
  const gchar *en_text = gtk_entry_get_text(GTK_ENTRY(en));
  gint i;
  if (! str_check(en_text))  return;
  g_return_if_fail(clist != NULL);

  for (i=0; i<GTK_CLIST(clist)->rows; i++)
  {
    gchar *text;
    gtk_clist_get_text(GTK_CLIST(clist), i, 3, &text);
    if (! strcmp(text, en_text))
    {
      gtk_clist_select_row(GTK_CLIST(clist), i, 3);
      if (gtk_clist_row_is_visible(GTK_CLIST(clist), i) != GTK_VISIBILITY_FULL)
        gtk_clist_moveto(GTK_CLIST(clist), i, 0, 0,0);
      break;
    }
  }
}

static gint evt_fg_keypress(GtkWidget *w, GdkEventKey *event, gpointer data)
{
  if (event->keyval == GDK_Tab)
  {
    GCompletion *cmpl;
    GList *l, *list=NULL;
    gchar *prefix, *nprefix = NULL;
    gint pos;
    
    if (! GRN->ng_list)  return FALSE;
    gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "key_press_event");
    for (l=GRN->ng_list; l; l=l->next)
    {
      grn_newsgroup *grp = l->data;
      if (! grp)  continue;
      list = g_list_prepend(list, g_strdup(grp->name));
    }
    list = g_list_reverse(list);
    cmpl = g_completion_new(NULL);
    g_completion_add_items(cmpl, list);
    pos = GTK_EDITABLE(w)->current_pos;
    prefix = gtk_editable_get_chars(GTK_EDITABLE(w), 0, pos);
    g_completion_complete(cmpl, prefix, &nprefix);
    if (nprefix)
    {
      gtk_editable_insert_text(GTK_EDITABLE(w), nprefix+pos,
        strlen(nprefix) - strlen(prefix), &pos);
      GTK_EDITABLE(w)->current_pos = pos;
      g_free(nprefix);
    }
    g_free(prefix);  g_completion_free(cmpl);
    for (l=list; l; l=l->next)  g_free(l->data);
    g_list_free(list);
    return TRUE;
  }
  return FALSE;
}

  
GtkWidget *groupsel_create()
{
  gchar *ttl[NCOLS] = {_("No"), "", _("Unread"), _("Name"), _("Description")};
  GtkWidget *vbox, *scrwin, *hbox, *w, *en_find;
  GtkCList *clist;
  GList *l;
  
  grn_lock();
  vbox = gtk_vbox_new(FALSE, 0);
  clist = GTK_CLIST(gtk_clist_new_with_titles(NCOLS, ttl));
  gtk_widget_set_usize(GTK_WIDGET(clist), 512, 300);
  gtk_clist_set_column_justification(clist, 0, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_justification(clist, 1, GTK_JUSTIFY_CENTER);
  gtk_clist_set_column_justification(clist, 2, GTK_JUSTIFY_RIGHT);
  gtk_clist_column_titles_passive(clist);
  gtk_clist_set_selection_mode(clist, GTK_SELECTION_MULTIPLE);
  w = gtk_pixmap_new(GRN->pixmaps.caption_subscr.pixmap, GRN->pixmaps.caption_subscr.mask);
  gtk_clist_set_column_widget(clist, 1, w);
  
  scrwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrwin), GTK_WIDGET(clist));
  gtk_box_pack_start(GTK_BOX(vbox), scrwin, TRUE, TRUE, 0);
  
  hbox = gtk_hbox_new(FALSE, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 1);
  w = gtk_label_new(_("Find newsgroup:"));
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);
  en_find = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), en_find, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(en_find), "key_press_event",
                     GTK_SIGNAL_FUNC(evt_fg_keypress), NULL);

  gtk_object_set_data(GTK_OBJECT(vbox), KEY_CL_GROUPS, clist);

  gtk_signal_connect(GTK_OBJECT(en_find), "changed",
                     GTK_SIGNAL_FUNC(gs_find_changed), clist);
  gtk_signal_connect(GTK_OBJECT(en_find), "activate",
                     GTK_SIGNAL_FUNC(gs_find_activate), clist);

  gtk_clist_freeze(clist);
  gtk_clist_clear(clist);
  grn_unlock();
  for (l=GRN->ng_list; l; l=l->next)
  {
    grn_newsgroup *grp = (grn_newsgroup *) l->data;
    if (! add_group(clist, grp, FALSE))  continue;
  }
  clist_renumber(clist);
  grn_lock();
  gtk_clist_columns_autosize(clist);
  gtk_clist_thaw(clist);
  gtk_widget_show_all(vbox);
  grn_unlock();

  return vbox;
}

void groupsel_refresh(GtkWidget *groupsel, GSList *groups)
{
  GtkCList *clist;
  GSList *sl;
  GList *l;
  gint min, i;
  g_return_if_fail(groupsel != NULL);
  
  grn_lock();
  clist = GTK_CLIST(gtk_object_get_data(GTK_OBJECT(groupsel), KEY_CL_GROUPS));
  grn_unlock();
  if ((! clist) || (! GTK_IS_CLIST(clist)))  return;

  grn_lock();
  for (sl=groups; sl; sl=sl->next)
  {
    gchar *grp_name = (gchar *) sl->data;
    gint i;
    for (i=0; i<clist->rows; i++)
    {
      gchar *text;
      gtk_clist_get_text(GTK_CLIST(clist), i, 3, &text);
      if (! strcmp(text, grp_name))
      {
        gtk_clist_select_row(GTK_CLIST(clist), i, 3);
        break;
      }
    }
  }
  grn_unlock();
  min = clist->rows;
  for (l=clist->selection; l; l=l->next)
  {
    i = GPOINTER_TO_INT(l->data);
    if (i < min)  min = i;
  }
  clist->focus_row = min;
  grn_lock();
  if (gtk_clist_row_is_visible(clist, min) != GTK_VISIBILITY_FULL)
    gtk_clist_moveto(clist, min, 0, 0,0);
  gtk_widget_grab_focus(GTK_WIDGET(clist));
  grn_unlock();
}


static gboolean gsdlg_delete(GtkWidget *w)
{
  grn_save_ws(3, w);
  return FALSE;
}
static void btn_event(GtkWidget *btn, GtkWidget *w)
{  gsdlg_delete(w);  }

GtkWidget *groupsel_dlg_create(GSList *list)
{
  GtkWidget *dlg, *gs;
  
  grn_lock();
  dlg = gnome_dialog_new(_("Select newsgroups"), GNOME_STOCK_BUTTON_CANCEL,
                          GNOME_STOCK_BUTTON_OK, NULL);
  gnome_dialog_set_close(GNOME_DIALOG(dlg), TRUE);
  gtk_window_set_policy(GTK_WINDOW(dlg), FALSE, TRUE, FALSE);
  gtk_window_set_default_size(GTK_WINDOW(dlg), grn_prefs.ws[3].width, grn_prefs.ws[3].height);
  gtk_widget_set_uposition(dlg, grn_prefs.ws[3].x, grn_prefs.ws[3].y);
  gtk_signal_connect(GTK_OBJECT(dlg), "delete_event",
                     GTK_SIGNAL_FUNC(gsdlg_delete), NULL);
  gnome_dialog_button_connect(GNOME_DIALOG(dlg), 0,
                              GTK_SIGNAL_FUNC(btn_event), dlg);
  gnome_dialog_button_connect(GNOME_DIALOG(dlg), 1,
                              GTK_SIGNAL_FUNC(btn_event), dlg);
  grn_unlock();
  
  gs = groupsel_create();
  grn_lock();
  gtk_object_set_data(GTK_OBJECT(dlg), KEY_GROUPSEL, gs);
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg)->vbox), gs, TRUE, TRUE, 0);
  gtk_widget_show_all(dlg);
  groupsel_refresh(gs, list);
  grn_unlock();
  return dlg;
}
