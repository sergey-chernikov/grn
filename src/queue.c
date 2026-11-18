/*
 * queue.c: Based on Pan's queue.[ch]
 *
 * $Id: queue.c,v 1.22 2000/05/19 14:51:46 sc Exp $
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
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <gnome.h>
#include "grn_vars.h"
#include "grn_misc.h"
#include "grn_util.h"
#include "grn_config.h"
#include "grn_news.h"
#include "tcp.h"
#include "smtp.h"
#include "queue.h"
#include "grn_nglist.h"
#include "grn_threadlist.h"
#include "grn_msgwin.h"


GSList *queue = NULL, *queue_last = NULL;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t qcond_m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t qcond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t groups_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ogroup_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t omsg_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pmsg_m = PTHREAD_MUTEX_INITIALIZER;


static gboolean grnq_run_download_grouplist(grnq_data *);
static gboolean grnq_run_download_msghdrlist(grnq_data *);
static gboolean grnq_run_download_article(grnq_data *);
static gboolean grnq_run_post_article(grnq_data *);
static gboolean grnq_run_grouplist_recalc(grnq_data *);
static gboolean grnq_run_threadlist_recalc(grnq_data *);
static gboolean grnq_run_send_message(grnq_data *);
static gboolean grnq_run_draw_grouplist(grnq_data *);
static gboolean grnq_run_draw_msglist(grnq_data *);
static gboolean grnq_run_draw_msglist1(grnq_data *);
static gboolean grnq_run_draw_article(grnq_data *);


void grnq_init(void)
{
  pthread_create(&(GRN->tid_queue), NULL, (void *) &grnq_processing, NULL);
  pthread_detach(GRN->tid_queue);
}

void grnq_append(grnq_data *qdata)
{
  GSList *l;
  
  l = g_slist_alloc();
  l->data = qdata;
  if (queue)
  {
    g_assert(queue_last);
    g_slist_concat(queue_last, l);
    queue_last = l;
  }
  else  queue = queue_last = l;
}

void grnq_prepend(grnq_data *qdata)
{
  if (queue_last)  queue = g_slist_prepend(queue, qdata);
  else  queue_last = queue = g_slist_prepend(queue, qdata);
}

grnq_data *grnq_get_next(void)
{
  grnq_data *qdata = NULL;
  
  pthread_mutex_lock(&qlock);
  if (queue)
  {
    qdata = (grnq_data *) queue->data;
    queue = g_slist_remove_link(queue, queue);
    if (queue == NULL)  queue_last = NULL;
  }
  pthread_mutex_unlock(&qlock);
  return qdata;
}

static grnq_data *grnq_get_next_ro(void)
{
  grnq_data *qdata = NULL;
  
  pthread_mutex_lock(&qlock);
  if (queue)
    qdata = (grnq_data *) queue->data;

  pthread_mutex_unlock(&qlock);
  return qdata;
}

void grnq_processing(void)
{
  struct timeval now;
  struct timespec timeout;
  int retcode = 0;
  grnq_data *qdata;
  time_t last_run, current_time;
  
  while (1)
  {
    pthread_mutex_lock(&qcond_m);
    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + 230;
    timeout.tv_nsec = now.tv_usec * 1000;
    
    retcode = pthread_cond_timedwait(&qcond, &qcond_m, &timeout);
    if ((retcode == ETIMEDOUT) && (queue == NULL))
    {
      if (nntp_is_online(GRN->nntp_sock))
      {
	gchar *rc = NULL;

        time(&current_time);
	if ((current_time - last_run) > 300)  nntp_offline(GRN->nntp_sock); //! disconnect_timeout to prefs
	else  rc = nntp_send_noop(GRN->nntp_sock);
	if (rc)  grn_error(_("NNTP noop"), rc);
      }
    }
    else {
      pthread_mutex_unlock(&qcond_m);
      while ((qdata = grnq_get_next()))  grnq_run(qdata, grnq_get_next_ro());
      pthread_mutex_lock(&qcond_m);
      time(&last_run);
    }
    pthread_mutex_unlock(&qcond_m);
  }
}

void grnq_free(grnq_data *qdata)
{
  t_message *m;
  g_assert(qdata);
  
  switch (qdata->flags & 0x003F)
  {
    case GRNQ_DOWNLOAD_GROUPLIST:
      break;
    case GRNQ_DOWNLOAD_MSGHDRLIST:
      break;
    case GRNQ_DOWNLOAD_ARTICLE:
      break;
    case GRNQ_POST_ARTICLE:
      m = (t_message *) qdata->data;
      t_message_free(&m);
      break;
    case GRNQ_GROUPLIST_RECALC:
      break;
    case GRNQ_THREADLIST_RECALC:
      break;
    case GRNQ_SEND_MESSAGE:
      m = (t_message *) qdata->data;
      t_message_free(&m);
      break;
    case GRNQ_DRAW_GROUPLIST:
      break;
    case GRNQ_DRAW_MSGLIST:
    case GRNQ_DRAW_MSGLIST1:
      break;
    case GRNQ_DRAW_ARTICLE:
      break;
    default:
      g_error(_("Some strange behavior of grnq_free()"));
      break;
  }
  g_free(qdata);
}

gboolean grnq_run(grnq_data *qdata, grnq_data *qdata_next)
{
  gboolean rc = TRUE;
  gint f1=0, f2=0;
  g_assert(qdata);
  
  f1 = qdata->flags & 0x003F;
  if (qdata_next)  f2 = qdata_next->flags & 0x003F;
  if (f1 == f2)
    switch (f1)
    {
      case GRNQ_THREADLIST_RECALC:
      case GRNQ_GROUPLIST_RECALC:
        grnq_free(qdata);  return TRUE;
        break;
      default: ;
    }
  
  switch (f1)
  {
    case GRNQ_DOWNLOAD_GROUPLIST:
      rc = grnq_run_download_grouplist(qdata);
      break;
    case GRNQ_DOWNLOAD_MSGHDRLIST:
      rc = grnq_run_download_msghdrlist(qdata);
      break;
    case GRNQ_DOWNLOAD_ARTICLE:
      rc = grnq_run_download_article(qdata);
      break;
    case GRNQ_POST_ARTICLE:
      rc = grnq_run_post_article(qdata);
      break;
    case GRNQ_GROUPLIST_RECALC:
      rc = grnq_run_grouplist_recalc(qdata);
      break;
    case GRNQ_THREADLIST_RECALC:
      rc = grnq_run_threadlist_recalc(qdata);
      break;
    case GRNQ_SEND_MESSAGE:
      rc = grnq_run_send_message(qdata);
      break;
    case GRNQ_DRAW_GROUPLIST:
      rc = grnq_run_draw_grouplist(qdata);
      break;
    case GRNQ_DRAW_MSGLIST:
      rc = grnq_run_draw_msglist(qdata);
      break;
    case GRNQ_DRAW_MSGLIST1:
      rc = grnq_run_draw_msglist1(qdata);
      break;
    case GRNQ_DRAW_ARTICLE:
      rc = grnq_run_draw_article(qdata);
      break;
    default:
      g_error(_("Some strange behavior of grnq_run()"));
      rc = FALSE;
      break;
  }
  grnq_free(qdata);
  return rc;
}

void grnq_purge(void)
{
  GSList *p;
  
  pthread_mutex_lock(&qlock);
  for (p = queue; p; p=p->next)  grnq_free((grnq_data *)p->data);
  g_slist_free(queue);
  queue = NULL;  queue_last = NULL;
  pthread_mutex_unlock(&qlock);
}


void grnq_add_download_grouplist(void)
{
  grnq_data *qdata;
  
  qdata = g_new(grnq_data, 1);
  qdata->flags = GRNQ_DOWNLOAD_GROUPLIST;
  pthread_mutex_lock(&qlock);
  queue = g_slist_append(queue, qdata);
  pthread_mutex_unlock(&qlock);
  
  pthread_mutex_lock(&qcond_m);
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qcond_m);
}

void grnq_add_download_msghdrlist(grn_newsgroup *grp)
{
  grnq_data *qdata;
  
  qdata = g_new(grnq_data, 1);
  qdata->flags = GRNQ_DOWNLOAD_MSGHDRLIST;
  qdata->data = grp;
  pthread_mutex_lock(&qlock);
  queue = g_slist_append(queue, qdata);
  pthread_mutex_unlock(&qlock);
  
  pthread_mutex_lock(&qcond_m);
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qcond_m);
}

void grnq_add_download_article(t_message *msg)
{
  grnq_data *qdata;
  
  qdata = g_new(grnq_data, 1);
  qdata->flags = GRNQ_DOWNLOAD_ARTICLE;
  qdata->data = msg;
  pthread_mutex_lock(&qlock);
  queue = g_slist_append(queue, qdata);
  pthread_mutex_unlock(&qlock);
  
  pthread_mutex_lock(&qcond_m);
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qcond_m);
}

void grnq_add_post_article(t_message *msg)
{
  grnq_data *qdata;
  g_return_if_fail(msg != NULL);
  
  qdata = g_new(grnq_data, 1);
  qdata->flags = GRNQ_POST_ARTICLE;
  qdata->data = t_message_copy(msg);
  pthread_mutex_lock(&qlock);
  queue = g_slist_append(queue, qdata);
  pthread_mutex_unlock(&qlock);
  
  pthread_mutex_lock(&qcond_m);
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qcond_m);
}

void grnq_add_grouplist_recalc(void)
{
  grnq_data *qdata;
  
  qdata = g_new(grnq_data, 1);
  qdata->flags = GRNQ_GROUPLIST_RECALC;
  pthread_mutex_lock(&qlock);
  queue = g_slist_append(queue, qdata);
  pthread_mutex_unlock(&qlock);
  
  pthread_mutex_lock(&qcond_m);
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qcond_m);
}

void grnq_add_threadlist_recalc(GtkWidget *w)
{
  grnq_data *qdata;
  
  qdata = g_new(grnq_data, 1);
  qdata->flags = GRNQ_THREADLIST_RECALC;
  qdata->data = w;
  pthread_mutex_lock(&qlock);
  queue = g_slist_append(queue, qdata);
  pthread_mutex_unlock(&qlock);
  
  pthread_mutex_lock(&qcond_m);
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qcond_m);
}

void grnq_add_send_message(t_message *msg)
{
  grnq_data *qdata;
  g_return_if_fail(msg != NULL);
  
  qdata = g_new(grnq_data, 1);
  qdata->flags = GRNQ_SEND_MESSAGE;
  qdata->data = t_message_copy(msg);
  pthread_mutex_lock(&qlock);
  queue = g_slist_append(queue, qdata);
  pthread_mutex_unlock(&qlock);
  
  pthread_mutex_lock(&qcond_m);
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qcond_m);
}

void grnq_add_draw_grouplist(void)
{
  grnq_data *qdata;
  
  qdata = g_new(grnq_data, 1);
  qdata->flags = GRNQ_DRAW_GROUPLIST;
  pthread_mutex_lock(&qlock);
  queue = g_slist_append(queue, qdata);
  pthread_mutex_unlock(&qlock);
  
  pthread_mutex_lock(&qcond_m);
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qcond_m);
}

void grnq_add_draw_msglist(void)
{
  grnq_data *qdata;
  
  qdata = g_new(grnq_data, 1);
  qdata->flags = GRNQ_DRAW_MSGLIST;
  pthread_mutex_lock(&qlock);
  queue = g_slist_append(queue, qdata);
  pthread_mutex_unlock(&qlock);
  
  pthread_mutex_lock(&qcond_m);
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qcond_m);
}

void grnq_add_draw_msglist1(void)
{
  grnq_data *qdata;
  
  qdata = g_new(grnq_data, 1);
  qdata->flags = GRNQ_DRAW_MSGLIST1;
  pthread_mutex_lock(&qlock);
  queue = g_slist_append(queue, qdata);
  pthread_mutex_unlock(&qlock);
  
  pthread_mutex_lock(&qcond_m);
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qcond_m);
}

void grnq_add_draw_article(t_message *msg)
{
  grnq_data *qdata;
  
  qdata = g_new(grnq_data, 1);
  qdata->flags = GRNQ_DRAW_ARTICLE;
  qdata->data = msg;
  pthread_mutex_lock(&qlock);
  queue = g_slist_append(queue, qdata);
  pthread_mutex_unlock(&qlock);
  
  pthread_mutex_lock(&qcond_m);
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qcond_m);
}


gboolean grnq_run_download_grouplist(grnq_data *qdata)
{
  pthread_mutex_lock(&groups_m);
  flags.open_grouplist = TRUE;
  ng_list_download(FALSE);
  flags.open_grouplist = FALSE;
  pthread_mutex_unlock(&groups_m);
  return TRUE;
}

gboolean grnq_run_download_msghdrlist(grnq_data *qdata)
{
  GList *msglist=NULL;
  gint total=0, first=0, last=0;
  grn_newsgroup *grp = (grn_newsgroup *) qdata->data;
  gchar *rc = NULL;
  g_return_val_if_fail(grp != NULL, FALSE);
  
  pthread_mutex_lock(&ogroup_m);
  flags.open_group = TRUE;
  if (GRN->msghdr_list)  msghdrlist_free(&(GRN->msghdr_list));
  grn_appbar_push(GRN->appbar, FALSE, _("Fetching article list for group %s..."), grp->name);
  rc = nntp_connect(GRN->nntp_sock);
  if (! rc)
  {
    if (grp->name)
      rc = nntp_set_group(GRN->nntp_sock, grp->name, &total, &first, &last);
    if (! rc)
    {
      msglist = nntp_get_msghdrlist(GRN->nntp_sock, total, first, last, grp, GRN->appbar);
      if (msglist)  GRN->msghdr_list = msglist;
      else {
        if (GRN->nntp_sock->error)
	  grn_error(_("Socket error"), GRN->nntp_sock->err_msg);
      }
    }
    else  grn_error(_("NNTP set group"), rc);
  }
  else  grn_error(_("NNTP connect"), rc);
  grn_appbar_pop(GRN->appbar);
  grn_lock();
  gnome_appbar_set_progress(GRN->appbar, 0);
  grn_unlock();
  flags.open_group = FALSE;
  pthread_mutex_unlock(&ogroup_m);
  return TRUE;
}

gboolean grnq_run_download_article(grnq_data *qdata)
{
  t_message *msg = (t_message *) qdata->data;
  g_return_val_if_fail(msg != NULL, FALSE);
  
  pthread_mutex_lock(&omsg_m);
  flags.open_article = TRUE;
  nntp_download_article(msg);
  flags.open_article = FALSE;
  pthread_mutex_unlock(&omsg_m);
  return TRUE;
}

gboolean grnq_run_post_article(grnq_data *qdata)
{
  t_message *msg = (t_message *) qdata->data;
  gchar *rc;
  g_return_val_if_fail(msg != NULL, FALSE);
  g_return_val_if_fail(msg->mh != NULL, FALSE);
  g_return_val_if_fail(msg->mh->newsgroups != NULL, FALSE);
  
  pthread_mutex_lock(&pmsg_m);
  flags.post_article = TRUE;
  grn_appbar_push(GRN->appbar, FALSE, _("Posting article to %s..."), msg->mh->newsgroups);
  rc = nntp_connect(GRN->nntp_sock);
  if (! rc)
  {
    rc = nntp_post_article(GRN->nntp_sock, msg);
    if (rc)  grn_error(_("NNTP post article"), rc);
  }
  else  grn_error(_("NNTP connect"), rc);
  grn_appbar_pop(GRN->appbar);
  flags.post_article = FALSE;
  pthread_mutex_unlock(&pmsg_m);
  if (grn_prefs.cfms[6])  grn_info(_("Article posted succesfully"));
  return TRUE;
}

gboolean grnq_run_grouplist_recalc(grnq_data *qdata)
{
  pthread_mutex_lock(&groups_m);
  if (GRN->grouplist)  grouplist_recalc_unread(GRN->grouplist);
  pthread_mutex_unlock(&groups_m);
  return TRUE;
}

gboolean grnq_run_threadlist_recalc(grnq_data *qdata)
{
  GtkWidget *w = (GtkWidget *) qdata->data;
  grn_threadlist_stats *tls;
  GList *msgs;

  if (! w)  return FALSE;
  if ((! GTK_WIDGET_REALIZED(w)) || (! GTK_WIDGET_MAPPED(w)) ||
      (! GTK_WIDGET_VISIBLE(w)))  return FALSE;
  tls = gtk_object_get_data(GTK_OBJECT(w), KEY_TL_STATS);
  msgs = gtk_object_get_data(GTK_OBJECT(w), KEY_MSGLIST);
  if ((! tls) || (! msgs))  return FALSE;
  get_unread_tl_stats(msgs, tls);
  show_tl_stats(tls);
  return TRUE;
}

gboolean grnq_run_send_message(grnq_data *qdata)
{
  t_message *msg = (t_message *) qdata->data;
  gchar *rc;
  grn_socket *sock;
  g_return_val_if_fail(msg != NULL, FALSE);
  g_return_val_if_fail(msg->mh != NULL, FALSE);
  
  grn_appbar_push(GRN->appbar, FALSE, _("Sending message via SMTP..."));
  sock = grn_socket_new();
  rc = smtp_connect(sock);
  if (! rc)
  {
    rc = smtp_send_message(sock, msg);
    if (rc)  grn_error(_("SMTP send message"), rc);
  }
  else  grn_error(_("SMTP connect"), rc);

  grn_socket_free(&sock);
  grn_appbar_pop(GRN->appbar);
  return TRUE;
}

gboolean grnq_run_draw_grouplist(grnq_data *qdata)
{
  pthread_mutex_lock(&groups_m);
  flags.open_grouplist = TRUE;
  if (GRN->grouplist && GRN->ng_list)
    grouplist_refresh(GRN->grouplist, GRN->ng_list);
  flags.open_grouplist = FALSE;
  pthread_mutex_unlock(&groups_m);
  return TRUE;
}

gboolean grnq_run_draw_msglist(grnq_data *qdata)
{
  pthread_mutex_lock(&ogroup_m);
  flags.open_group = TRUE;
  if (GRN->threadlist && GRN->msghdr_list)
    threadlist_refresh(GRN->threadlist, GRN->msghdr_list, TRUE);
  pthread_mutex_unlock(&ogroup_m);
  flags.open_group = FALSE;
  return TRUE;
}

gboolean grnq_run_draw_msglist1(grnq_data *qdata)
{
  pthread_mutex_lock(&ogroup_m);
  flags.open_group = TRUE;
  if (GRN->threadlist && GRN->msghdr_list)
    threadlist_refresh(GRN->threadlist, GRN->msghdr_list, FALSE);
  flags.open_group = FALSE;
  pthread_mutex_unlock(&ogroup_m);
  return TRUE;
}

gboolean grnq_run_draw_article(grnq_data *qdata)
{
  t_message *msg = (t_message *) qdata->data;
  g_return_val_if_fail(msg != NULL, FALSE);
  
  if (flags.open_article)  return FALSE;
  pthread_mutex_lock(&omsg_m);
  flags.open_article = TRUE;
  if (GRN->msgwin)  msgwin_refresh(GRN->msgwin, msg);
  flags.open_article = FALSE;
  pthread_mutex_unlock(&omsg_m);
  return TRUE;
}
