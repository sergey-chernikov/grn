/*
 * grn_threadlist.c: threadlist window
 *
 * $Id: grn_threadlist.c,v 1.37 2000/06/05 15:35:12 sc Exp $
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

#include <math.h>
#include "grn_consts.h"
#include <gnome.h>
#include "grn_threadlist.h"
#include "grn_menu.h"
#include "grn_vars.h"
#include "grn_util.h"
#include "grn_misc.h"
#include "grn_config.h"
#include "grn_commands.h"
#include "nntp.h"
#include "grn_news.h"
#include "grn_nglist.h"
#include "grn_msgwin.h"
#include "grn_perl.h"
#include "queue.h"


#define NCOLS	8


static void evt_tl_delete(GtkWidget *, gpointer);
static void evt_tl_hide(GtkWidget *, gpointer);
static void evt_tl_show(GtkWidget *, gpointer);
static gint evt_tl_keypress(GtkWidget *, GdkEventKey *, gpointer);
static gint evt_tl_btnpress(GtkWidget *, GdkEventButton *, gpointer);
static void evt_click_col(GtkCList *, int);


#include "grn_tl_compare_funcs.c"
static GList *sort_msghdrlist(GList *msghdr_list)
{
  GList *ret = NULL;
  g_return_val_if_fail(msghdr_list != NULL, NULL);

  grn_appbar_push(GRN->appbar, FALSE, _("Sorting articles..."));
  
  if (grn_prefs.thrd_tl_type > 0)
    ret = g_list_sort(msghdr_list, hdr_compare_dates);
  else {
    guint n;
    GCompareFunc compare_func;

    n = grn_prefs.srt_tl_col;
    if (n >= 100)  n -= 100;
    switch (n)
    {
      case 1:
        compare_func = hdr_compare_scores;
        break;
      case 3:
        compare_func = hdr_compare_subjects;
        break;
      case 4:
        compare_func = hdr_compare_authors;
        break;
      case 5:
        compare_func = hdr_compare_dates;
        break;
      case 6:
        compare_func = hdr_compare_lines;
        break;
      case 7:
        compare_func = hdr_compare_sizes;
        break;
      default:
        compare_func = hdr_compare_dates;
    }
    ret = g_list_sort(msghdr_list, compare_func);
    if (grn_prefs.srt_tl_col >= 100)  ret = g_list_reverse(ret);
  }
  grn_appbar_pop(GRN->appbar);
  return ret;
}

static GList *sort_msglist_threads(GList *msglist)
{
  guint n;
  GList *ret = NULL;
  GCompareFunc compare_func;
  if (! msglist)  return NULL;

  grn_appbar_push(GRN->appbar, FALSE, _("Sorting threads..."));

  n = grn_prefs.srt_tl_col;
  if (n >= 100)  n -= 100;
  switch (n)
  {
    case 1:
      compare_func = compare_scores;
      break;
    case 3:
      compare_func = compare_subjects;
      break;
    case 4:
      compare_func = compare_authors;
      break;
    case 5:
      compare_func = compare_dates;
      break;
    case 6:
      compare_func = compare_lines;
      break;
    case 7:
      compare_func = compare_sizes;
      break;
    default:
      compare_func = no_sort;
  }
  ret = g_list_sort(msglist, compare_func);
  if (grn_prefs.srt_tl_col >= 100)  ret = g_list_reverse(ret);

  grn_appbar_pop(GRN->appbar);
  return ret;
}


static GList *thread_by_nothing(GList *msghdr_list)
{
  GList *l, *l_end=NULL, *ret=NULL;
  gulong cnt=0, total, step;

  total = g_list_length(msghdr_list);
  step = total / 100;
  if (step <= 0)  step = 1;
  for (l=msghdr_list; l; l=l->next)
  {
    t_msgheaders *mh = (t_msgheaders *) l->data;
    grn_newsgroup *grp = mh->grp;
    t_message *msg;

    cnt++;
    if ((! grn_prefs.tl_show_read) && grp && grp->seq)
    {
        if (grn_news_art_read(grp->seq, mh->number))  continue;
    }
    msg = t_message_alloc();
    msg->mh = t_msgheaders_copy(mh);
    grn_g_list_append(&ret, &l_end, msg);

    if (! (cnt % step))
    {
      grn_lock();
      gnome_appbar_set_progress(GRN->appbar, cnt / ((gfloat) total));
      grn_unlock();
    }
  }
  return ret;
}

static GList *thread_by_ref(GList *msghdr_list)
{
  GList *l, *l_end=NULL, *ret=NULL;
  gulong cnt=0, total, step;

  total = g_list_length(msghdr_list);
  step = total / 100;
  if (step <= 0)  step = 1;
  for (l=msghdr_list; l; l=l->next)
  {
    t_msgheaders *mh = (t_msgheaders *) l->data;
    t_message *msg, *parent_msg;

    cnt++;
    msg = t_message_alloc();
    msg->mh = t_msgheaders_copy(mh);
    parent_msg = msglist_find_by_id(ret, msg);
    if (parent_msg)  parent_msg->threads = g_list_append(parent_msg->threads, msg);
    else  grn_g_list_append(&ret, &l_end, msg);

    if (! (cnt % step))
    {
      grn_lock();
      gnome_appbar_set_progress(GRN->appbar, cnt / ((gfloat) total));
      grn_unlock();
    }
  }
  return ret;
}

static GList *thread_by_subj(GList *msghdr_list)
{
  GList *l, *l_end=NULL, *ret=NULL;
  gulong cnt=0, total, step;

  total = g_list_length(msghdr_list);
  step = total / 100;
  if (step <= 0)  step = 1;
  for (l=msghdr_list; l; l=l->next)
  {
    t_msgheaders *mh = (t_msgheaders *) l->data;
    t_message *msg, *parent_msg;

    cnt++;
    msg = t_message_alloc();
    msg->mh = t_msgheaders_copy(mh);
    parent_msg = msglist_find_by_subj(ret, msg);
    if (parent_msg)  parent_msg->threads = g_list_append(parent_msg->threads, msg);
    else  grn_g_list_append(&ret, &l_end, msg);

    if (! (cnt % step))
    {
      grn_lock();
      gnome_appbar_set_progress(GRN->appbar, cnt / ((gfloat) total));
      grn_unlock();
    }
  }
  return ret;
}

static GList *thread_by_ref_and_subj(GList *msghdr_list)
{
  GList *l, *l_end=NULL, *ret=NULL;
  gulong cnt=0, total, step;

  total = g_list_length(msghdr_list);
  step = total / 100;
  if (step <= 0)  step = 1;
  for (l=msghdr_list; l; l=l->next)
  {
    t_msgheaders *mh = (t_msgheaders *) l->data;
    t_message *msg, *parent_msg;

    cnt++;
    msg = t_message_alloc();
    msg->mh = t_msgheaders_copy(mh);
    parent_msg = msglist_find_by_id(ret, msg);
    if (parent_msg)  parent_msg->threads = g_list_append(parent_msg->threads, msg);
    else {
      parent_msg = msglist_find_by_subj(ret, msg);
      if (parent_msg)  parent_msg->threads = g_list_append(parent_msg->threads, msg);
      else  grn_g_list_append(&ret, &l_end, msg);
    }
    if (! (cnt % step))
    {
      grn_lock();
      gnome_appbar_set_progress(GRN->appbar, cnt / ((gfloat) total));
      grn_unlock();
    }
  }
  return ret;
}

static GList *thread_messages(GList *msghdr_list, gboolean must_sort)
{
  GList *ret=NULL, *ret1=NULL;
  g_return_val_if_fail(msghdr_list != NULL, NULL);
  
  if (must_sort)  ret1 = sort_msghdrlist(msghdr_list);
  else  ret1 = msghdr_list;

  grn_appbar_push(GRN->appbar, FALSE, _("Threading articles..."));

  if (grn_prefs.thrd_tl_type == 0)  ret = thread_by_nothing(ret1);
  else if (grn_prefs.thrd_tl_type == 1)  ret = thread_by_ref(ret1);
  else if (grn_prefs.thrd_tl_type == 2)  ret = thread_by_subj(ret1);
  else if (grn_prefs.thrd_tl_type == 3)  ret = thread_by_ref_and_subj(ret1);

  grn_appbar_pop(GRN->appbar);
  grn_lock();
  gnome_appbar_set_progress(GRN->appbar, 0);
  grn_unlock();

  return ret;
}

static GtkCTreeNode *add_msg_to_ctree(GtkCTree *ctree, GtkCTreeNode *parent,
				      t_message *msg)
{
  gchar *row[NCOLS];
  GtkCTreeNode *node;
  gint i;
  grn_newsgroup *grp;
  GdkPixmap *pixmap=NULL, *score_pm=NULL;
  GdkBitmap *mask=NULL, *score_mask=NULL;
  g_return_val_if_fail(msg != NULL, NULL);
  g_return_val_if_fail(msg->mh != NULL, NULL);
  g_return_val_if_fail(msg->mh->grp != NULL, NULL);
  
  grp = msg->mh->grp;

  if (! t_message_is_read(msg))
  {
    pixmap = GRN->pixmaps.unread_article.pixmap;
    mask = GRN->pixmaps.unread_article.mask;
  }
  
  if ((msg->mh->score >= 1) && (msg->mh->score <= 16))
  {
    score_pm = GRN->pixmaps.balls[msg->mh->score].pixmap;
    score_mask = GRN->pixmaps.balls[msg->mh->score].mask;
  }
  else if (msg->mh->score == 99)
  {
    score_pm = GRN->pixmaps.balls[0].pixmap;
    score_mask = GRN->pixmaps.balls[0].mask;
  }
  
  row[0] = row[1] = row[2] = NULL;
  if ((! score_pm) && (msg->mh->score > 0))
    row[1] = g_strdup_printf("%u", msg->mh->score);
  row[3] = str_copy(msg->mh->subject);
  row[4] = str_copy(msg->mh->from);
  row[5] = date_get_local(msg->mh->parsed_date);
  row[6] = g_strdup_printf("%ld", msg->mh->lines);
  row[7] = g_strdup_printf("%ld", msg->mh->size);
  
  grn_lock();
  if ((! parent) && (msg->threads))
    node = gtk_ctree_insert_node(ctree, NULL, NULL, row, 3,
           GRN->pixmaps.thread_closed.pixmap, GRN->pixmaps.thread_closed.mask,
	   GRN->pixmaps.thread_opened.pixmap, GRN->pixmaps.thread_opened.mask,
	   FALSE, TRUE);
  else  node = gtk_ctree_insert_node(ctree, parent, NULL, row, 0,
               NULL, NULL, NULL, NULL, FALSE, TRUE);
  if (pixmap)  gtk_ctree_node_set_pixmap(ctree, node, 2, pixmap, mask);
  if (score_pm)  gtk_ctree_node_set_pixmap(ctree, node, 1, score_pm, score_mask);
  gtk_ctree_node_set_row_data(ctree, node, msg);
  if (msg->mh->score == 99)  gtk_ctree_node_set_selectable(ctree, node, FALSE);
  grn_unlock();
  
  for (i=0; i<NCOLS; i++)  str_free(&row[i]);

  return node;
}
static void build_msgs_tree_recursive(GtkCTree *ctree, GtkCTreeNode *parent, GList *msgs)
{
  GList *l;
  
  for (l=msgs; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    GtkCTreeNode *sibling;
    sibling = add_msg_to_ctree(ctree, parent, msg);
    if (msg->threads)  build_msgs_tree_recursive(ctree, sibling, msg->threads);
  }
}

static void collapse_root_nodes(GtkCList *clist)
{
  GList *l;

  grn_lock();
  for (l=clist->row_list; l; l=l->next)
  {
    GtkCTreeNode *node = GTK_CTREE_NODE(l);
    gtk_ctree_collapse(GTK_CTREE(clist), node);
  }
  grn_unlock();
}

static void get_unread_stats_recursive(GList *msglist, gulong *nunread)
{
  GList *l;
  if (! msglist)  return;
  
  for (l=msglist; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    if (! msg)  continue;
    
    if (! t_message_is_read(msg))  (*nunread)++;
    get_unread_stats_recursive(msg->threads, nunread);
  }
}

void get_unread_tl_stats(GList *msglist, grn_threadlist_stats *ret)
{
  GList *l;
  
  ret->nunread = 0;
  ret->nthunread = 0;
  for (l=msglist; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    gulong nunr1 = 0;
    if (! msg)  continue;

    if (! t_message_is_read(msg))  nunr1++;
    get_unread_stats_recursive(msg->threads, &nunr1);
    ret->nunread += nunr1;
    if (nunr1 > 0)  ret->nthunread++;
  }
}

static void get_stats_recursive(GList *msglist, gulong *nchildren, gulong *nmsgs, gulong *nunread)
{
  GList *l;
  if (! msglist)  return;
  
  for (l=msglist; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    if (! msg)  continue;
    
    (*nmsgs)++;
    if (! t_message_is_read(msg))  { (*nunread)++;  (*nchildren)++; }
    get_stats_recursive(msg->threads, nchildren, nmsgs, nunread);
  }
}
static void get_tl_stats(GList *msglist, grn_threadlist_stats *ret)
{
  gulong nmsgs=0, nunread=0, nthreads=0, nchildren;
  GList *l;
  
  for (l=msglist; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    gboolean unread = FALSE;
    if (! msg)  continue;

    nthreads++;  nmsgs++;
    nchildren = 0;
    unread = (! t_message_is_read(msg));
    if (unread)  nunread++;
    if (msg->threads)
    {
      if (unread)  nchildren++;
      get_stats_recursive(msg->threads, &nchildren, &nmsgs, &nunread);
    }
    msg->nchildren = nchildren;
  }
  ret->nthreads = nthreads;  ret->nmsgs = nmsgs;  ret->nunread = nunread;
}

void show_tl_stats(grn_threadlist_stats *tls)
{
  if (! tls)  return;

  grn_appbar_set_status(GRN->appbar, _("Total %ld message[s] in %ld thread[s]. %ld unread message[s] in %ld thread[s]"),
                        tls->nmsgs, tls->nthreads, tls->nunread, tls->nthunread);
}

static void show_threads_nchildren(GtkCList *clist)
{
  gint i;
  gchar buf[100];
  g_return_if_fail(clist != NULL);
  
  for (i=0; i<clist->rows; i++)
  {
    t_message *msg;
    grn_lock();
    msg = (t_message *) gtk_clist_get_row_data(clist, i);
    grn_unlock();
    if (! msg)  continue;
    
    if (msg->nchildren > 0)
    {
      snprintf(buf, 99, "%d", msg->nchildren);
      grn_lock();
      gtk_clist_set_text(clist, i, 0, buf);
      grn_unlock();
    }
  }
}

static GList *leave_unread(GList *msglist)
{
  GList *l, *ret=NULL;
  if (! msglist)  return NULL;
  
  for (l=msglist; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    if ((! msg) || (! msg->mh))  continue;
    
    msg->threads = leave_unread(msg->threads);
    if ((t_message_is_read(msg) || (msg->mh->score == 99)) && (! msg->threads))
    {
      t_message_free(&msg);
      continue;
    }
    ret = g_list_prepend(ret, msg);
  }
  g_list_free(msglist);
  ret = g_list_reverse(ret);
  return ret;
}


void threadlist_refresh(GtkWidget *threadlist, GList *msghdr_list,
			gboolean must_sort)
{
  GList *msgs, *l, *mhdr_list=NULL;
  GtkCList *clist;
  t_msgheaders *mh;
  grn_threadlist_stats *tls, *tls1;
  g_return_if_fail(threadlist != NULL);
  g_return_if_fail(msghdr_list != NULL);
  
  grn_lock();
  clist = GTK_CLIST(gtk_object_get_data(GTK_OBJECT(threadlist), KEY_CT_MSGS));
  grn_unlock();
  if (! clist)  return;
  grn_widget_set_style(GTK_WIDGET(clist), grn_prefs.fonts[3], NULL, NULL, NULL);

  for (l=msghdr_list; l; l=l->next)
  {
    t_msgheaders *mh = (t_msgheaders *) l->data;
    t_msgheaders *mhc;
    if (! mh)  continue;
    mhc = t_msgheaders_copy(mh);
    mhdr_list = g_list_prepend(mhdr_list, mhc);
  }
  mhdr_list = g_list_reverse(mhdr_list);
  
  msgs = thread_messages(mhdr_list, must_sort);
  if (! msgs)
  {
    msghdrlist_free(&mhdr_list);
    return;
  }
  
  mh = (t_msgheaders *) mhdr_list->data;
  if (mh && mh->grp && mh->grp->plain_unread)
  {
    g_list_free(mh->grp->plain_unread);
    mh->grp->plain_unread = NULL;
  }
  
  grn_appbar_push(GRN->appbar, FALSE, _("Eliminating read articles..."));
  tls = g_new(grn_threadlist_stats, 1);
  get_tl_stats(msgs, tls);
  if (! grn_prefs.tl_show_read)  msgs = leave_unread(msgs);
  tls->nthunread = g_list_length(msgs);
  grn_lock();
  tls1 = gtk_object_get_data(GTK_OBJECT(threadlist), KEY_TL_STATS);
  if (tls1)  g_free(tls1);
  gtk_object_remove_data(GTK_OBJECT(threadlist), KEY_TL_STATS);
  gtk_object_set_data(GTK_OBJECT(threadlist), KEY_TL_STATS, tls);
  grn_unlock();

  if (! grn_prefs.tl_show_read)  grn_appbar_pop(GRN->appbar);

  if (grn_prefs.thrd_tl_type > 0)  msgs = sort_msglist_threads(msgs);

  grn_lock();
  gtk_clist_freeze(clist);
  gtk_clist_clear(clist);
  grn_unlock();
  
  build_msgs_tree_recursive(GTK_CTREE(clist), NULL, msgs);
  if (grn_prefs.tl_ac)  collapse_root_nodes(clist);
  show_threads_nchildren(clist);
  
  grn_lock();
  gtk_clist_columns_autosize(clist);
  gtk_clist_thaw(clist);
  grn_unlock();
  show_tl_stats(tls);

  grn_lock();
  l = gtk_object_get_data(GTK_OBJECT(threadlist), KEY_MSGLIST);
  if (l)  msglist_free(&l);
  gtk_object_remove_data(GTK_OBJECT(threadlist), KEY_MSGLIST);
  gtk_object_set_data(GTK_OBJECT(threadlist), KEY_MSGLIST, msgs);
  msghdrlist_free(&mhdr_list);
  gtk_widget_grab_focus(GTK_WIDGET(clist));
  grn_unlock();
}


GtkWidget *threadlist_create()
{
  gchar *ttl[NCOLS] = {_("N"), NULL, NULL, _("Subject"), _("Author"), _("Date"), _("Lines"), _("Size")};
  GtkWidget *vbox, *scrwin, *w;
  GtkCList *clist;
  gshort n;

  grn_lock();  
  vbox = gtk_vbox_new(FALSE, 0);

  clist = GTK_CLIST(gtk_ctree_new_with_titles(NCOLS, 3, ttl));
  gtk_clist_set_column_justification(clist, 0, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_justification(clist, 1, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_justification(clist, 2, GTK_JUSTIFY_CENTER);
  gtk_clist_set_column_justification(clist, 5, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_justification(clist, 6, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_justification(clist, 7, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_selection_mode(clist, GTK_SELECTION_BROWSE);
  gtk_tooltips_set_tip(GRN->tooltips, clist->column[0].button, _("Select threading method"), NULL);
  gtk_tooltips_set_tip(GRN->tooltips, clist->column[1].button, _("Sort by score"), NULL);
  gtk_tooltips_set_tip(GRN->tooltips, clist->column[2].button, _("Toggle visibility of read messages"), NULL);
  gtk_tooltips_set_tip(GRN->tooltips, clist->column[3].button, _("Sort by subject"), NULL);
  gtk_tooltips_set_tip(GRN->tooltips, clist->column[4].button, _("Sort by author"), NULL);
  gtk_tooltips_set_tip(GRN->tooltips, clist->column[5].button, _("Sort by date"), NULL);
  gtk_tooltips_set_tip(GRN->tooltips, clist->column[6].button, _("Sort by lines"), NULL);
  gtk_tooltips_set_tip(GRN->tooltips, clist->column[7].button, _("Sort by size"), NULL);
#if 0
  gtk_ctree_set_line_style(GTK_CTREE(clist), GTK_CTREE_LINES_NONE);
  gtk_ctree_set_expander_style(GTK_CTREE(clist), GTK_CTREE_EXPANDER_TRIANGLE);
#endif  
  n = grn_prefs.srt_tl_col;
  clist->sort_type = GTK_SORT_ASCENDING;
  if (n >= 100)
  {
    clist->sort_type = GTK_SORT_DESCENDING;
    n -= 100;
  }
  gtk_clist_set_sort_column(clist, n);

  w = gtk_pixmap_new(GRN->pixmaps.balls[12].pixmap, GRN->pixmaps.balls[12].mask);
  gtk_clist_set_column_widget(clist, 1, w);
  w = gtk_pixmap_new(GRN->pixmaps.caption_unread.pixmap, GRN->pixmaps.caption_unread.mask);
  gtk_clist_set_column_widget(clist, 2, w);
  
  scrwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrwin), GTK_WIDGET(clist));
  gtk_box_pack_start(GTK_BOX(vbox), scrwin, TRUE, TRUE, 0);

  gtk_object_set_data(GTK_OBJECT(clist), KEY_SCRWIN, scrwin);
  gtk_object_set_data(GTK_OBJECT(clist), KEY_WND, vbox);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_CT_MSGS, clist);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_FOCUS, clist);

  gtk_signal_connect(GTK_OBJECT(vbox), "destroy",
                     GTK_SIGNAL_FUNC(evt_tl_delete), NULL);
  gtk_signal_connect(GTK_OBJECT(vbox), "hide",
                     GTK_SIGNAL_FUNC(evt_tl_hide), NULL);
  gtk_signal_connect(GTK_OBJECT(vbox), "show",
                     GTK_SIGNAL_FUNC(evt_tl_show), NULL);
  gtk_signal_connect(GTK_OBJECT(clist), "key_press_event",
                     GTK_SIGNAL_FUNC(evt_tl_keypress), NULL);
  gtk_signal_connect(GTK_OBJECT(clist), "button_press_event",
                     GTK_SIGNAL_FUNC(evt_tl_btnpress), NULL);
  gtk_signal_connect(GTK_OBJECT(clist), "click_column",
                     GTK_SIGNAL_FUNC(evt_click_col), NULL);
  
  gtk_widget_show_all(vbox);
  grn_unlock();
  enable_findagain_menu(FALSE);
  return vbox;
}


static void msg_calc_nchildren(GList *msglist, guint *nchildren)
{
  GList *l;
  if (! msglist)  return;
  for (l=msglist; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    if (! t_message_is_read(msg))  (*nchildren)++;
    msg_calc_nchildren(msg->threads, nchildren);
  }
}
static t_message *get_thread_parent(GtkCTree *ctree, GtkCTreeNode *node)
{
  GtkCTreeNode *n = node;
  GtkCTreeNode *parent = GTK_CTREE_ROW(node)->parent;
  t_message *m;

  if (GTK_CTREE_ROW(node)->level <= 1)  return NULL;
  while (parent)
  {
    n = parent;
    parent = GTK_CTREE_ROW(n)->parent;
  }
  if (n)
  {
    grn_lock();
    m = (t_message *) gtk_ctree_node_get_row_data(ctree, n);
    grn_unlock();
    return m;
  }
  else  return NULL;
}
static void threadlist_refresh_row(GtkCTree *ctree, t_message *msg)
{
  GtkCTreeNode *node;
  t_message *msg1;
  GdkPixmap *pixmap=NULL;
  GdkBitmap *mask=NULL;
  g_return_if_fail(msg != NULL);
  
  grn_lock();
  node = gtk_ctree_find_by_row_data(ctree, NULL, msg);
  grn_unlock();
  if (! node)  return;
  msg1 = get_thread_parent(ctree, node);
  if (msg1)  threadlist_refresh_row(ctree, msg1);

  if (! t_message_is_read(msg))
  {
    pixmap = GRN->pixmaps.unread_article.pixmap;
    mask = GRN->pixmaps.unread_article.mask;
  }
  grn_lock();
  if (pixmap)  gtk_ctree_node_set_pixmap(ctree, node, 2, pixmap, mask);
  else  gtk_ctree_node_set_text(ctree, node, 2, "");

  gtk_ctree_node_set_text(ctree, node, 0, "");
  grn_unlock();

  if (msg->threads && (! msg1))
  {
    guint nchildren = 0;
    msg->nchildren = 0;

    msg_calc_nchildren(msg->threads, &(msg->nchildren));
    nchildren = msg->nchildren;
    if (! t_message_is_read(msg))  nchildren++;
    if (nchildren > 0)
    {
      gchar buf[100];
      snprintf(buf, 99, "%d", nchildren);
      grn_lock();
      gtk_ctree_node_set_text(ctree, node, 0, buf);
      grn_unlock();
    }
  }
}

static void evt_tl_delete(GtkWidget *w, gpointer data)
{
  GList *msgs;
  grn_threadlist_stats *tls1;
  
  msgs = gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_MSGLIST);
  if (msgs)  msglist_free(&msgs);
  gtk_object_remove_data(GTK_OBJECT(GRN->threadlist), KEY_MSGLIST);

  tls1 = gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_TL_STATS);
  if (tls1)  g_free(tls1);
  gtk_object_remove_data(GTK_OBJECT(GRN->threadlist), KEY_TL_STATS);

  GRN->threadlist = NULL;
}

static void evt_tl_hide(GtkWidget *w, gpointer data)
{
  show_threadlist_menu(FALSE);
  grn_appbar_set_status(GRN->appbar, "");
}

static void evt_tl_show(GtkWidget *w, gpointer data)
{
  show_threadlist_menu(TRUE);
  grnq_add_threadlist_recalc(w);
}


static void evt_click_col(GtkCList *clist, int n)
{
  if (flags.open_group)  return;
  if (n == 0)
  {
    gchar *modes[] = {_("nothing"), _("references"), _("subject"), _("ref+subj")};
    gchar *m_now, *m_next, *s;
    guint i;

    grn_prefs.thrd_tl_type++;
    if (grn_prefs.thrd_tl_type > 3)  grn_prefs.thrd_tl_type = 0;
    m_now = modes[grn_prefs.thrd_tl_type];
    i = grn_prefs.thrd_tl_type+1;  m_next = modes[(i>3)?0:i];
    s = g_strdup_printf(_("Thread by %s; (now: by %s)"), m_next, m_now);
    gtk_tooltips_set_tip(GRN->tooltips, clist->column[0].button, s, NULL);
    g_free(s);
    grnq_add_draw_msglist();
    return;
  }
  if (n == 2)
  {
    grn_prefs.tl_show_read = grn_prefs.tl_show_read ? FALSE : TRUE;
    grnq_add_draw_msglist1();
    return;
  }
  if (clist->sort_column == n)
  {
    if (clist->sort_type == GTK_SORT_ASCENDING)
      clist->sort_type = GTK_SORT_DESCENDING;
    else  clist->sort_type = GTK_SORT_ASCENDING;
  }
  else  clist->sort_type = GTK_SORT_ASCENDING;
  
  grn_prefs.srt_tl_col = n;
  if (clist->sort_type == GTK_SORT_DESCENDING)  grn_prefs.srt_tl_col += 100;
  gtk_clist_set_sort_column(clist, n);

  grnq_add_draw_msglist();
}


static void select_cur_node(GtkWidget *w, GtkCTreeNode *node)
{
  GtkCList *clist = GTK_CLIST(w);
  g_return_if_fail(w != NULL);
  g_return_if_fail(GTK_IS_CTREE(w));
  if (! node)  return;

  clist->focus_row = g_list_position(clist->row_list, (GList *) node);
  grn_lock();
  gtk_ctree_select(GTK_CTREE(w), node);
  if (gtk_clist_row_is_visible(clist, clist->focus_row) != GTK_VISIBILITY_FULL)
    gtk_clist_moveto(clist, clist->focus_row, 0, 0,0);
  grn_unlock();
}

static void collapse_cur_thread(GtkWidget *w)
{
  GtkCList *clist = GTK_CLIST(w);
  GtkCTreeRow *row;
  g_return_if_fail(w != NULL);
  g_return_if_fail(GTK_IS_CTREE(w));
  
  row = GTK_CTREE_ROW(g_list_nth(clist->row_list, clist->focus_row));
  if (row->level > 1)
  {
    GtkCTreeNode *node = GTK_CTREE_NODE(row);
    GtkCTreeNode *parent = row->parent;

    while (parent)
    {
      node = parent;
      parent = GTK_CTREE_ROW(node)->parent;
    }
    select_cur_node(w, node);
    gtk_ctree_collapse(GTK_CTREE(w), node);
  }
  else  gtk_ctree_collapse(GTK_CTREE(w), GTK_CTREE_NODE(g_list_nth(clist->row_list, clist->focus_row)));
}

static void expand_thread(GtkCTree *ctree, GtkCTreeNode *node)
{
  GtkCTreeNode *parent = node, *prev = node;
  
  while (parent)
  {
    prev = parent;
    parent = GTK_CTREE_ROW(parent)->parent;
  }
  if (! GTK_CTREE_ROW(prev)->is_leaf)  gtk_ctree_expand_recursive(ctree, prev);
}

t_message *tl_get_selected_message(GtkCTree *ctree)
{
  GtkCTreeNode *node = GTK_CTREE_NODE(g_list_nth(GTK_CLIST(ctree)->row_list, GTK_CLIST(ctree)->focus_row));
  if (node)  return  (t_message *) gtk_ctree_node_get_row_data(ctree, node);
  else  return NULL;
}

void tl_select_message(GtkCTree *ctree, t_message *cur_msg, t_message *msg)
{
  GtkCTreeNode *node = gtk_ctree_find_by_row_data(ctree, NULL, msg);
  t_message *msg_thread, *cur_thread;
  GList *msglist = gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_MSGLIST);
  g_return_if_fail(ctree != NULL);
  if (! node)  return;
  if (! msglist)  return;
  g_return_if_fail(cur_msg != NULL);
  g_return_if_fail(msg != NULL);

  msg_thread = msglist_find_cur_thread(msglist, msg);
  cur_thread = msglist_find_cur_thread(msglist, cur_msg);

  gtk_clist_freeze(GTK_CLIST(ctree));
  if (msg_thread != cur_thread)
    if (grn_prefs.tl_ac)  collapse_cur_thread(GTK_WIDGET(ctree));

  expand_thread(ctree, node);
  select_cur_node(GTK_WIDGET(ctree), node);
  if (gtk_clist_row_is_visible(GTK_CLIST(ctree), GTK_CLIST(ctree)->focus_row) != GTK_VISIBILITY_FULL)
    gtk_clist_moveto(GTK_CLIST(ctree), GTK_CLIST(ctree)->focus_row, 0, 0,0);
  gtk_clist_thaw(GTK_CLIST(ctree));
}

t_message *tl_get_root_message(GtkCTree *ctree, t_message *msg)
{
  GtkCTreeNode *node;

  grn_lock();
  node = gtk_ctree_find_by_row_data(ctree, NULL, msg);
  grn_unlock();
  if (! node)  return NULL;
  return get_thread_parent(ctree, node);
}

static void select_prev_next_unread(GtkWidget *w, GList *msglist,
            t_message *cur_msg, gboolean prev, gboolean is_thread, gboolean beg)
{
  t_message *msg, *cur_thread;
  GtkCTreeNode *node;
  if ((! msglist) || (! cur_msg) || (! w))  return;
  cur_thread = msglist_find_cur_thread(msglist, cur_msg);

  if (is_thread)
  {
    if (prev)  msg = msglist_find_prev_thread_unread(msglist, cur_msg);
    else  msg = msglist_find_next_thread_unread(msglist, cur_msg);
  }
  else {
    if (prev)  msg = msglist_find_prev_unread(msglist, cur_msg);
    else  msg = msglist_find_next_unread(msglist, cur_msg);
  }
  node = gtk_ctree_find_by_row_data(GTK_CTREE(w), NULL, msg);
  if (node)
  {
    t_message *msg_thread = msglist_find_cur_thread(msglist, msg);

    gtk_clist_freeze(GTK_CLIST(w));
    if (msg_thread != cur_thread)
      if (grn_prefs.tl_ac)  collapse_cur_thread(w);
    if (beg)
    {
      GtkCTreeNode *n = gtk_ctree_find_by_row_data(GTK_CTREE(w), NULL,
                          get_thread_parent(GTK_CTREE(w), node));
      if (n)  select_cur_node(w, n);
      else  select_cur_node(w, node);
    }
    else {
      expand_thread(GTK_CTREE(w), node);
      select_cur_node(w, node);
    }
    if (gtk_clist_row_is_visible(GTK_CLIST(w), GTK_CLIST(w)->focus_row) != GTK_VISIBILITY_FULL)
      gtk_clist_moveto(GTK_CLIST(w), GTK_CLIST(w)->focus_row, 0, 0,0);
    gtk_clist_thaw(GTK_CLIST(w));
  }
}

static GtkCTree *get_ctree(GtkWidget *w)
{
  GtkCTree *ctree;

  g_return_val_if_fail(w != NULL, NULL);
  if (! GTK_IS_CTREE(w))
  {
    ctree = GTK_CTREE(gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_CT_MSGS));
    g_return_val_if_fail(ctree != NULL, NULL);
    g_return_val_if_fail(GTK_IS_CTREE(ctree), NULL);
  }
  else  ctree = GTK_CTREE(w);
  return ctree;
}

extern pthread_mutex_t ogroup_m;
void evt_tl_open_article(GtkWidget *w)
{
  GtkCTree *ctree = get_ctree(w);
  GtkCList *clist;
  t_message *msg;
  g_return_if_fail(ctree != NULL);

  if (flags.open_article || flags.open_group)  return;
  clist = GTK_CLIST(ctree);
  
  pthread_mutex_lock(&ogroup_m);
  msg = tl_get_selected_message(ctree);
  if (msg)
  {
    if (! GRN->msgwin)  GRN->msgwin = msgwin_create();
    grn_lock();
    grn_set_contents(GRN->msgwin);
    grn_unlock();
    if (msg->mh && msg->mh->grp && msg->mh->grp->name)
      grn_set_title(GTK_WINDOW(GRN->window), _("%s - %s, msg #%d"),
                    GRN_TITLE, msg->mh->grp->name, msg->mh->number);
    if (! msg->cached)  grnq_add_download_article(msg);
    grnq_add_draw_article(msg);
    if (msg->mh && msg->mh->grp && msg->mh->grp->seq)
    {
      gchar *seq = grn_news_add_read(msg->mh->grp->seq, msg->mh->number);
      grn_newsgroup_set_seq(msg->mh->grp, seq);
      threadlist_refresh_row(ctree, msg);
    }
  }
  pthread_mutex_unlock(&ogroup_m);
  show_message_menu(TRUE);
}

static void scroll_line(GtkAdjustment *adj, gint offset, gint width)
{
  gfloat value = adj->value + offset;
  if (value > (adj->upper - width + 4))  value = adj->upper - width + 4;
  if (value < adj->lower)  value = adj->lower;
  gtk_adjustment_set_value(adj, value);
  grn_main_iter();
}

static void scroll_smooth(GtkAdjustment *adj, gint n, gint width)
{
  gint i = 0;
  
  if (n > 0)
    while (i <= n)
    {
      gint step = sin(M_PI/n*i) * 23 + 2;
      if (step > (n - i))  step = n - i;
      i += step;
      scroll_line(adj, step, width);
      if (n == i)  break;
    }
  else
    while (i >= n)
    {
      gint step = -sin(M_PI/n*i) * 23 - 2;
      if (step < (n - i))  step = n - i;
      i += step;
      scroll_line(adj, step, width);
      if (n == i)  break;
    }
}

static gint evt_tl_keypress(GtkWidget *w, GdkEventKey *event, gpointer data)
{
  gint offset = 0;
  GList *msglist;
  t_message *cur_msg;
  gboolean rc = TRUE;
  
  msglist = gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_MSGLIST);
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(w != NULL, FALSE);
  cur_msg = tl_get_selected_message(GTK_CTREE(w));
  
  switch (event->keyval)
  {
    case GDK_Select:
    case GDK_Execute:
    case GDK_Return:
    case GDK_KP_Enter:
      evt_tl_open_article(w);
      break;
    case GDK_Right:
    case GDK_KP_Right:
      if ((event->state & GDK_CONTROL_MASK) || (event->state & GDK_SHIFT_MASK))
        offset = 1;
      else  evt_tl_open_article(w);
      break;
    case GDK_Left:
    case GDK_KP_Left:
      if ((event->state & GDK_CONTROL_MASK) || (event->state & GDK_SHIFT_MASK))
        offset = -1;
      else {
        evt_grouplist(w);
      }
      break;
    case GDK_Up:
      if (event->state & GDK_CONTROL_MASK)
        select_prev_next_unread(w, msglist, cur_msg, TRUE, FALSE, FALSE);
      else if (event->state & GDK_SHIFT_MASK)
        select_prev_next_unread(w, msglist, cur_msg, TRUE, TRUE, FALSE);
      else  rc = FALSE;
      break;
    case GDK_Down:
      if (event->state & GDK_CONTROL_MASK)  // next unread
        select_prev_next_unread(w, msglist, cur_msg, FALSE, FALSE, FALSE);
      else if (event->state & GDK_SHIFT_MASK)  // next [unread] thread
        select_prev_next_unread(w, msglist, cur_msg, FALSE, TRUE, FALSE);
      else  rc = FALSE;
      break;
    default:  rc = FALSE;
  }
  if (! rc)  return FALSE;
  gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "key_press_event");

  if (offset)
  {
    GtkScrolledWindow *scrwin = GTK_SCROLLED_WINDOW(gtk_object_get_data(GTK_OBJECT(w), KEY_SCRWIN));
    if (scrwin)
    {
      GtkAdjustment *adj = gtk_scrolled_window_get_hadjustment(scrwin);
      if (adj)
      {
	gint width, height, n;
	gdk_window_get_size(w->window, &width, &height);
	if (event->state & GDK_CONTROL_MASK)  n = offset * adj->page_increment;
	else  n = offset * adj->step_increment;
	if (grn_prefs.smooth_scrolling)  scroll_smooth(adj, n, width);
	else  scroll_line(adj, n, width);
      }
    }
  }
  return TRUE;
}

static gint evt_tl_btnpress(GtkWidget *w, GdkEventButton *event, gpointer data)
{
  gboolean processed = FALSE;
  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(w != NULL, FALSE);
  
  if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
  {
    evt_tl_open_article(w);
    processed = TRUE;
  }
  if (processed)
  {
    gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "button_press_event");
    return TRUE;
  }
  return FALSE;
}

static void prev_next_unread_wrapper(GtkWidget *w, gboolean prev,
            gboolean thread, gboolean beg)
{
  GtkCTree *ctree = get_ctree(w);
  GList *msglist;
  t_message *cur_msg;
  g_return_if_fail(ctree != NULL);
  
  msglist = gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_MSGLIST);
  cur_msg = tl_get_selected_message(ctree);
  select_prev_next_unread(GTK_WIDGET(ctree), msglist, cur_msg, prev, thread, beg);
}


void evt_tl_urprev_msg(GtkWidget *w)
{ prev_next_unread_wrapper(w, TRUE, FALSE, FALSE); }
void evt_tl_urnext_msg(GtkWidget *w)
{ prev_next_unread_wrapper(w, FALSE, FALSE, FALSE); }

void evt_tl_urprev_thread(GtkWidget *w)
{ prev_next_unread_wrapper(w, TRUE, TRUE, FALSE); }
void evt_tl_urnext_thread(GtkWidget *w)
{ prev_next_unread_wrapper(w, FALSE, TRUE, FALSE); }

void evt_tl_back(GtkWidget *w)
{ evt_grouplist(w); }


void evt_expand_cur_thread(GtkWidget *w)
{
  GtkCTree *ctree = get_ctree(w);
  g_return_if_fail(ctree != NULL);

  gtk_ctree_expand_recursive(ctree, GTK_CTREE_NODE(g_list_nth(
    GTK_CLIST(ctree)->row_list, GTK_CLIST(ctree)->focus_row)));
  if (gtk_clist_row_is_visible(GTK_CLIST(ctree), GTK_CLIST(ctree)->focus_row) != GTK_VISIBILITY_FULL)
    gtk_clist_moveto(GTK_CLIST(ctree), GTK_CLIST(ctree)->focus_row, 0, 0,0);
}

void evt_collapse_cur_thread(GtkWidget *w)
{
  GtkCTree *ctree = get_ctree(w);
  g_return_if_fail(ctree != NULL);
  collapse_cur_thread(GTK_WIDGET(ctree));
}

void evt_expand_all_threads(GtkWidget *w)
{
  GList *msgs, *l;
  GtkCTree *ctree = get_ctree(w);
  g_return_if_fail(ctree != NULL);

  msgs = gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_MSGLIST);
  if (! msgs)  return;
  gtk_clist_freeze(GTK_CLIST(ctree));
  for (l=msgs; l; l=l->next)
  {
    GtkCTreeNode *node;
    t_message *msg = (t_message *) l->data;
    if (! msg)  continue;
    node = gtk_ctree_find_by_row_data(ctree, NULL, msg);
    if (node)  gtk_ctree_expand_recursive(ctree, node);
  }
  gtk_clist_thaw(GTK_CLIST(ctree));
  if (gtk_clist_row_is_visible(GTK_CLIST(ctree), GTK_CLIST(ctree)->focus_row) != GTK_VISIBILITY_FULL)
    gtk_clist_moveto(GTK_CLIST(ctree), GTK_CLIST(ctree)->focus_row, 0, 0,0);
}

void evt_collapse_all_threads(GtkWidget *w)
{
  GList *msgs, *l;
  GtkCTree *ctree = get_ctree(w);
  g_return_if_fail(ctree != NULL);

  msgs = gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_MSGLIST);
  if (! msgs)  return;
  gtk_clist_freeze(GTK_CLIST(ctree));
  for (l=msgs; l; l=l->next)
  {
    GtkCTreeNode *node;
    t_message *msg = (t_message *) l->data;
    if (! msg)  continue;
    node = gtk_ctree_find_by_row_data(ctree, NULL, msg);
    if (node)  gtk_ctree_collapse_recursive(ctree, node);
  }
  gtk_clist_thaw(GTK_CLIST(ctree));
  if (gtk_clist_row_is_visible(GTK_CLIST(ctree), GTK_CLIST(ctree)->focus_row) != GTK_VISIBILITY_FULL)
    gtk_clist_moveto(GTK_CLIST(ctree), GTK_CLIST(ctree)->focus_row, 0, 0,0);
}


static void recalc_threadlist_by_ctree(GtkCTree *ctree)
{
  GtkWidget *w;
  if (! ctree)  return;
  
  w = gtk_object_get_data(GTK_OBJECT(ctree), KEY_WND);
  if (w)  grnq_add_threadlist_recalc(w);
}

void evt_tl_mark_read(GtkWidget *w)
{
  GtkCTree *ctree = get_ctree(w);
  GtkCList *clist;
  t_message *msg;
  g_return_if_fail(ctree != NULL);

  clist = GTK_CLIST(ctree);
  msg = tl_get_selected_message(ctree);
  if (msg && msg->mh && msg->mh->grp && msg->mh->grp->seq)
  {
    gchar *seq = grn_news_add_read(msg->mh->grp->seq, msg->mh->number);
    grn_newsgroup_set_seq(msg->mh->grp, seq);
    threadlist_refresh_row(ctree, msg);
    prev_next_unread_wrapper(w, FALSE, FALSE, FALSE);
    recalc_threadlist_by_ctree(ctree);
  }
}

void evt_tl_mark_unread(GtkWidget *w)
{
  GtkCTree *ctree = get_ctree(w);
  GtkCList *clist;
  t_message *msg;
  g_return_if_fail(ctree != NULL);

  clist = GTK_CLIST(ctree);
  msg = tl_get_selected_message(ctree);
  if (msg && msg->mh && msg->mh->grp && msg->mh->grp->seq)
  {
    gchar *seq = grn_news_add_unread(msg->mh->grp->seq, msg->mh->number);
    grn_newsgroup_set_seq(msg->mh->grp, seq);
    threadlist_refresh_row(ctree, msg);
    recalc_threadlist_by_ctree(ctree);
  }
}


static void mark_msglist(GList *msglist, gboolean unread, GtkCTree *ctree)
{
  GList *l;
  if (! msglist)  return;
  
  for (l=msglist; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    gchar *seq;
    if (! msg)  continue;

    if (unread)  seq = grn_news_add_unread(msg->mh->grp->seq, msg->mh->number);
    else  seq = grn_news_add_read(msg->mh->grp->seq, msg->mh->number);
    grn_newsgroup_set_seq(msg->mh->grp, seq);
    threadlist_refresh_row(ctree, msg);
    mark_msglist(msg->threads, unread, ctree);
  }
}

void tl_mark_thread_read(GtkCTree *ctree)
{
  GtkCList *clist;
  t_message *msg;

  clist = GTK_CLIST(ctree);
  
  msg = tl_get_selected_message(ctree);
  if (msg)  mark_msglist(msg->threads, FALSE, ctree);
  if (msg && msg->mh && msg->mh->grp && msg->mh->grp->seq)
  {
    gchar *seq = grn_news_add_read(msg->mh->grp->seq, msg->mh->number);
    grn_newsgroup_set_seq(msg->mh->grp, seq);
    threadlist_refresh_row(ctree, msg);
    recalc_threadlist_by_ctree(ctree);
  }
  prev_next_unread_wrapper(GTK_WIDGET(ctree), FALSE, FALSE, TRUE);
}

static void catchup_cb(gint reply, GtkCTree *ctree)
{
  g_return_if_fail(ctree != NULL);
  if (reply == GNOME_YES)  tl_mark_thread_read(ctree);
}
void evt_tl_mark_thread_read(GtkWidget *w)
{
  GtkCTree *ctree = get_ctree(w);
  g_return_if_fail(ctree != NULL);

  if (grn_prefs.cfms[2])
    gnome_app_question(GNOME_APP(GRN->window), _("Do you really wish to catchup current thread?"),
      (GnomeReplyCallback) catchup_cb, ctree);
  else  tl_mark_thread_read(ctree);
}

void tl_mark_thread_unread(GtkCTree *ctree)
{
  GtkCList *clist;
  t_message *msg;

  clist = GTK_CLIST(ctree);
  
  msg = tl_get_selected_message(ctree);
  if (msg)  mark_msglist(msg->threads, TRUE, ctree);
  if (msg && msg->mh && msg->mh->grp && msg->mh->grp->seq)
  {
    gchar *seq = grn_news_add_unread(msg->mh->grp->seq, msg->mh->number);
    grn_newsgroup_set_seq(msg->mh->grp, seq);
    threadlist_refresh_row(ctree, msg);
    recalc_threadlist_by_ctree(ctree);
  }
}

static void unread_cb(gint reply, GtkCTree *ctree)
{
  g_return_if_fail(ctree != NULL);
  if (reply == GNOME_YES)  tl_mark_thread_unread(ctree);
}
void evt_tl_mark_thread_unread(GtkWidget *w)
{
  GtkCTree *ctree = get_ctree(w);
  g_return_if_fail(ctree != NULL);

  if (grn_prefs.cfms[3])
    gnome_app_question(GNOME_APP(GRN->window), _("Do you really wish to mark current thread unread?"),
      (GnomeReplyCallback) unread_cb, ctree);
  else  tl_mark_thread_unread(ctree);
}
