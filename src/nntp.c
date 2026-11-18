/*
 * nntp.c: some code borrowed from Pan
 *
 * $Id: nntp.c,v 1.24 2000/06/06 15:45:46 sc Exp $
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <glib.h>
#include "tcp.h"
#include "nntp.h"
#include "grn_vars.h"
#include "grn_config.h"
#include "grn_misc.h"
#include "queue.h"
#include "grn_util.h"
#include "grn_perl.h"
#include "grn_msgpost.h"
#include "grn_mime.h"


static pthread_cond_t online_c = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t online_m = PTHREAD_MUTEX_INITIALIZER;


void nntp_wait_connect()
{
  pthread_mutex_lock(&online_m);
  pthread_cond_wait(&online_c, &online_m);
  pthread_mutex_unlock(&online_m);
}

static gchar *connect_to_server(grn_socket *sock)
{
  gchar *buffer;
  int rc;
  g_return_val_if_fail(sock != NULL, NULL);
  
  if (! str_check(grn_prefs.nntp_server))  return _("no server name given");
  sock->addr = g_strdup(grn_prefs.nntp_server);
  sock->service = g_strdup("nntp");
  
  grn_socket_open(sock);
  if (sock->error)  return sock->err_msg;
  if ((buffer = grn_socket_get(sock)) == NULL)  return _("server not responding");
  rc = atoi(buffer);
  if ((rc != 200) && (rc != 201))
  {
    grn_info(_("server said:\n%s"), buffer);  str_free(&buffer);
    return _("error in server reply");
  }
  str_free(&buffer);

  if (grn_prefs.nntp_auth)
  {
    if (grn_socket_put_va(sock, "AUTHINFO user %s\r\n", grn_prefs.nntp_login) == -1)
      return _("AUTHINFO user failed");
    if ((buffer = grn_socket_get(sock)) == NULL)  return _("server not responding");
    if (atoi(buffer) == 381)
    {
      str_free(&buffer);
      if (grn_socket_put_va(sock, "AUTHINFO pass %s\r\n", grn_prefs.nntp_passwd) == -1)
        return _("AUTHINFO pass failed");
      if ((buffer = grn_socket_get(sock)) == NULL)  return _("server not responding");
      if (atoi(buffer) != 281)
      {
	grn_info(_("server said:\n%s"), buffer);  str_free(&buffer);
	return _("Authentication failed");
      }
    }
    else {
      str_free(&buffer);
      grn_info(_("Authentication doesn't required for\n%s"), grn_prefs.nntp_server);
    }
  }
  if (grn_socket_put(sock, "MODE READER\r\n") == -1)  return _("MODE READER failed");
  if ((buffer = grn_socket_get(sock)) == NULL)  return _("server not responding");

  if (atoi(buffer) == 200)  GRN->srv.posting = TRUE;
  else if (atoi(buffer) == 201)  GRN->srv.posting = FALSE;
  else  { str_free(&buffer);  return _("error in server reply"); }
  str_free(&buffer);

  return NULL;
}

gchar *nntp_connect(grn_socket *sock)
{
  gchar *rc;
  g_return_val_if_fail(sock != NULL, NULL);

  if (! sock->closed)  return NULL;
  rc = connect_to_server(sock);
  if (rc)  return rc;
  pthread_mutex_lock(&online_m);
#if 0
  pthread_cond_broadcast(&online_c);
#endif
  pthread_mutex_unlock(&online_m);
  return NULL;
}

void nntp_disconnect(grn_socket *sock)
{
  if (sock->closed)  return;
  grn_socket_put(sock, "QUIT\r\n");
  grn_socket_close(sock);
  must_exit = TRUE;
  grnq_purge();
}

void nntp_offline(grn_socket *sock)
{
  if (sock->closed)  return;
  grn_socket_close(sock);
  grnq_purge();
}

gboolean nntp_is_online(grn_socket *sock)
{
  return (! sock->closed);
}


gchar *nntp_send_noop(grn_socket *sock)
{
  gchar *buffer;
  
  if (grn_socket_put(sock, "MODE READER\r\n") == -1)
    return _("can't send command to server");
  if ((buffer = grn_socket_get(sock)) == NULL)
    return _("server not responding");
  str_free(&buffer);
  return NULL;
}


gchar *nntp_set_group(grn_socket *sock, gchar *name, gint *total, gint *first, gint *last)
{
  gchar *buffer;
  
  if (grn_socket_put_va(sock, "GROUP %s\r\n", name) == -1)
    return _("can't send command to server");
  if ((buffer = grn_socket_get(sock)) != NULL)
  {
    if (atoi(buffer) == 211)
    {
      sscanf(buffer, "%*d %d %d %d", total, first, last);
      str_free(&buffer);
      return NULL;
    }
    str_free(&buffer);
    return _("error in server reply");
  }
  else return _("server not responding");
}

GList *nntp_get_grouplist(grn_socket *sock)
{
  gchar *buffer = NULL;
  grn_newsgroup *grp = NULL;
  gint count = 0;
  GList *ret = NULL;
  
  if (grn_socket_put(sock, "LIST active\r\n") == -1)
  {
    grn_socket_set_error(sock, _("can't send command to server"));
    return NULL;
  }
  if ((buffer = grn_socket_get(sock)) == NULL)
  {
    grn_socket_set_error(sock, _("server not responding"));
    return NULL;
  }
  if (atoi(buffer) != 215)
  {
    grn_socket_set_error(sock, _("error in server reply"));
    grn_info(_("server said:\n%s"), buffer);
    str_free(&buffer);   return NULL;
  }
  else {
    str_free(&buffer);
    if ((buffer = grn_socket_get(sock)) == NULL)
    {
      grn_socket_set_error(sock, _("server not responding"));
      return NULL;
    }
    while (strncmp(buffer, ".\r\n", 3))
    {
      gchar **arr;
      if (must_exit)
      {
        grouplist_free(&ret);
	nntp_offline(sock);
	return NULL;
      }
      arr = g_strsplit(buffer, " ", 4);
      if (arr[3])
      {
        if ((! strcmp(g_strstrip(arr[3]), "y")) || (! strcmp(g_strstrip(arr[3]), "m")))
        {
          grp = grn_newsgroup_alloc();
          if (arr[0])  grp->name = g_strdup(arr[0]);
          if (arr[1])  grp->total = atoi(arr[1]);
          if (arr[2])  grp->first = atoi(arr[2]);
          ret = g_list_append(ret, grp);
	  g_strfreev(arr);
          count++;
        }
      }
      str_free(&buffer);
      if ((buffer = grn_socket_get(sock)) == NULL)
      {
        grn_socket_set_error(sock, _("server not responding"));
	grouplist_free(&ret);
	return NULL;
      }
    }
    str_free(&buffer);
  }
  return ret;
}

GHashTable *nntp_get_groupdesc(grn_socket *sock)
{
  gchar *buffer = NULL;
  GHashTable *ret = NULL;

  if (! grn_prefs.groups_desc)  return NULL;

  if (grn_socket_put(sock, "LIST newsgroups\r\n") == -1)
  {
    grn_socket_set_error(sock, _("can't send command to server"));
    return NULL;
  }
  if ((buffer = grn_socket_get(sock)) == NULL)
  {
    grn_socket_set_error(sock, _("server not responding"));
    return NULL;
  }
  if (atoi(buffer) != 215)
  {
    grn_socket_set_error(sock, _("error in server reply"));
    grn_info(_("server said:\n%s"), buffer);
    str_free(&buffer);   return NULL;
  }
  else
  {
    str_free(&buffer);
    if ((buffer = grn_socket_get(sock)) == NULL)
    {
      grn_socket_set_error(sock, _("server not responding"));
      return NULL;
    }

    ret = g_hash_table_new(g_str_hash, g_str_equal);

    while (strncmp(buffer, ".\r\n", 3))
    {
      gchar *tmp1, *tmp2;
      gchar *name, *desc;

      if (must_exit)
      {
//        grouplist_free(ret);
	nntp_offline(sock);
	str_free(&buffer);  return NULL;
      }
      tmp1 = buffer;
      while ((*tmp1 != '\t') && (*tmp1 != ' ') && *tmp1) tmp1++;
      name = (gchar *) g_malloc(tmp1 - buffer + 1);
      strncpy(name, buffer, tmp1 - buffer);  name[tmp1 - buffer] = '\0';
      tmp2 = tmp1;
      while (((*tmp2 == '\t') || (*tmp2 == ' ')) && *tmp2)  tmp2++;
      tmp1 = tmp2;
      while ((*tmp1 != '\r') && (*tmp1 != '\n') && *tmp1)  tmp1++;
      desc = (gchar *) g_malloc(tmp1 - tmp2 + 1);
      strncpy(desc, tmp2, tmp1 - tmp2);  desc[tmp1 - tmp2] = '\0';
      g_hash_table_insert(ret, name, desc);

      str_free(&buffer);
      if ((buffer = grn_socket_get(sock)) == NULL)
      {
        grn_socket_set_error(sock, _("server not responding"));
//	grouplist_free(ret);
	return NULL;
      }
    }
    str_free(&buffer);
  }
  return ret;
}


GList *nntp_get_msghdrlist(grn_socket *sock, gint total, gint first, gint last,
			   grn_newsgroup *grp, GnomeAppBar *ab)
{
  gchar *buffer = NULL, *buf;
  GList *ret = NULL;
  GtkProgress *pb;
  
  if (total <= 0)  return NULL;
  
  if (grn_socket_put_va(sock, "XOVER %d-%d\r\n", first, last) == -1)
  {
    grn_socket_set_error(sock, _("can't send command to server"));
    return NULL;
  }
  if ((buffer = grn_socket_get(sock)) == NULL)
  {
    grn_socket_set_error(sock, _("server not responding"));
    return NULL;
  }
  if (atoi(buffer) != 224)
  {
    grn_socket_set_error(sock, _("error in server reply"));
    grn_info(_("server said:\n%s\n"), buffer);
    str_free(&buffer);  return NULL;
  }
  str_free(&buffer);
  
  if ((buffer = grn_socket_get(sock)) == NULL)
  {
    grn_socket_set_error(sock, _("server not responding"));
    return NULL;
  }

  grn_lock();
  pb = gnome_appbar_get_progress(ab);
  gtk_progress_set_show_text(pb, TRUE);
  grn_unlock();

  while (strncmp(buffer, ".\r\n", 3))
  {
    gchar **sarr;
    t_msgheaders *mh;

    if (must_exit)
    {
      msghdrlist_free(&ret);
      nntp_offline(sock);
      str_free(&buffer);
      return NULL;
    }
    sarr = g_strsplit(buffer, "\t", 0);
    mh = (t_msgheaders *) g_malloc0(sizeof(t_msgheaders));
    mh->number = atoi(sarr[0]);
    mh->subject = g_strdup(sarr[1]);
    mh->from = g_strdup(sarr[2]);
    mh->date = g_strdup(sarr[3]);
    mh->parsed_date = parse_date(mh->date);
    mh->id = g_strdup(sarr[4]);
    mh->ref = g_strdup(sarr[5]);
    mh->size = atoi(sarr[6]);
    mh->lines = atoi(sarr[7]);
    mh->xref = get_hdr_val(sarr[8]);
    mh->path = NULL;  mh->extra_hdrs = NULL;
    mh->grp = grp;
#ifdef USE_PERL
    mh->score = perl_hook_run("GetScore", GRN, mh, NULL);
    if (mh->score == (gulong) (-1L))  mh->score = 0;
#else
    mh->score = 0;
#endif
    g_strfreev(sarr);
    grn_decode_headers(mh);

    buf = g_strdup_printf("%ld/%d", mh->number, last);
    grn_lock();
    gnome_appbar_set_progress(ab, (mh->number - first + 1)/((gfloat) total));
    gtk_progress_set_format_string(pb, buf);
    grn_unlock();
    
    str_free(&buf);
    ret = g_list_prepend(ret, mh);

    str_free(&buffer);
    if ((buffer = grn_socket_get(sock)) == NULL)
    {
      grn_socket_set_error(sock, _("server not responding"));
      msghdrlist_free(&ret);
      break;
    }
  }
  str_free(&buffer);
  grn_lock();
  gtk_progress_set_format_string(pb, "%P %%");
  gtk_progress_set_show_text(pb, FALSE);
  grn_unlock();

  if (ret)  return g_list_reverse(ret);
  else  return NULL;
}


static gchar *update_header(t_message *msg, gchar *hdr)
{
  t_msgheader *mhdr;
  t_msgheaders *mh;
  gchar *key, *value, *ret=NULL;
  g_return_val_if_fail(msg != NULL, NULL);
  g_return_val_if_fail(msg->mh != NULL, NULL);
  g_return_val_if_fail(hdr != NULL, NULL);
  
  mh = msg->mh;
  mhdr = parse_hdr(hdr);
  if (! mhdr)  return NULL;
  key = g_strdup(mhdr->name);  value = g_strdup(mhdr->value);
  str_free(&(mhdr->name));  str_free(&(mhdr->value));  g_free(mhdr);

  if (! strcmp(key, "From"))
  {
    str_free(&(mh->from));
    mh->from = g_strdup(value);
    ret = g_strdup("From");
  }
  else if (! strcmp(key, "Subject"))
  {
    str_free(&(mh->subject));
    mh->subject = g_strdup(value);
    ret = g_strdup("Subject");
  }
  else if (! strcmp(key, "Date"))
  {
    str_free(&(mh->date));
    mh->date = g_strdup(value);
    mh->parsed_date = parse_date(mh->date);
    ret = g_strdup("Date");
  }
  else if (! strcmp(key, "Message-ID"))
  {
    str_free(&(mh->id));
    mh->id = g_strdup(value);
    ret = g_strdup("Message-ID");
  }
  else if (! strcmp(key, "References"))
  {
    str_free(&(mh->ref));
    mh->ref = g_strdup(value);
    ret = g_strdup("References");
  }
  else if (! strcmp(key, "Xref"))
  {
    str_free(&(mh->xref));
    mh->xref = g_strdup(value);
    ret = g_strdup("Xref");
  }
  else if (! strcmp(key, "Path"))
  {
    str_free(&(mh->path));
    mh->path = g_strdup(value);
    ret = g_strdup("Path");
  }
  else if (! strcmp(key, "Lines"))
  {
    mh->lines = atoi(value);
    ret = g_strdup("Lines");
  }
  else if (! strcmp(key, "Newsgroups"))
  {
    str_free(&(mh->newsgroups));
    mh->newsgroups = g_strdup(value);
    ret = g_strdup("Newsgroups");
  }
  else {
    gchar *new_val, *new_key;

    if (! mh->extra_hdrs)  mh->extra_hdrs = g_hash_table_new(g_str_hash, g_str_equal);
    if (g_hash_table_lookup_extended(mh->extra_hdrs, key, (gpointer *) &new_key,
                                     (gpointer *) &new_val))
    {
      gchar *tmp;

      g_hash_table_remove(mh->extra_hdrs, key);
      str_free(&new_key);
      tmp = new_val;
      new_val = g_strconcat(tmp, "\n", value, NULL);
      str_free(&tmp);
    }
    else  new_val = g_strdup(value);
    g_hash_table_insert(mh->extra_hdrs, g_strdup(key), new_val);
    ret = g_strdup(key);
  }
  str_free(&key);  str_free(&value);
  return ret;
}

gchar *nntp_get_article(grn_socket *sock, t_message *msg)
{
#define BUFSZ	16384
  gchar *buffer, *buf;
  gchar *prev_hdr=NULL, *hdr;
  gint bufsz = BUFSZ, bufpos = 0;
  g_return_val_if_fail(msg != NULL, FALSE);
  g_return_val_if_fail(msg->mh != NULL, FALSE);
  
  if (grn_socket_put_va(sock, "HEAD %lu\r\n", msg->mh->number) == -1)
    return _("server<-HEAD failed");
  if ((buffer = grn_socket_get(sock)) == NULL)  return _("server not responding");
  if (atoi(buffer) != 221)
  {
    g_free(buffer);
    return _("error in server reply");
  }
  g_free(buffer);
  if ((buffer = grn_socket_get(sock)) == NULL)  return _("server not responding");
  while (strncmp(buffer, ".\r\n", 3))
  {
    gchar *tmp;
    
    if (must_exit)
    {
      g_free(buffer);
      return _("transfer interrupted");
    }
    
    if ((tmp = strrchr(buffer, '\x0D')))  *tmp = '\0';
    if (strrchr(buffer, '\\'))
    {
      gchar *buf1 = str_subst(buffer, "\\t", "\t");  g_free(buffer);
      buffer = str_subst(buf1, "\\n", "\n");	     str_free(&buf1);
    }
    hdr = update_header(msg, buffer);
    if ((! hdr) && (prev_hdr))
    {
      gchar *s = g_strdup_printf("%s\n%s", t_msgheaders_get_hdr(msg->mh, prev_hdr), buffer);
      t_msgheaders_set_hdr(msg->mh, prev_hdr, s);
      str_free(&s);
      hdr = g_strdup(prev_hdr);
    }
    str_free(&prev_hdr);
    prev_hdr = hdr;
    
    g_free(buffer);
    if ((buffer = grn_socket_get(sock)) == NULL)  return _("server not responding");
  }
  g_free(buffer);

  if (grn_socket_put_va(sock, "BODY %lu\r\n", msg->mh->number) == -1)
    return _("server<-BODY failed");
  if ((buffer = grn_socket_get(sock)) == NULL)  return _("server not responding");
  if (atoi(buffer) != 222)
  {
    g_free(buffer);
    return _("error in server reply");
  }
  g_free(buffer);
  if ((buffer = grn_socket_get(sock)) == NULL)  return _("server not responding");
  str_free(&(msg->body));
  buf = g_malloc(bufsz);
  while (strncmp(buffer, ".\r\n", 3))
  {
    gchar *tmp;
    gint len;
    
    if (must_exit)
    {
      g_free(buffer);  g_free(buf);
      return _("transfer interrupted");
    }

    if ((tmp = strrchr(buffer, '\x0D')))  *tmp = '\0';
    len = strlen(buffer);
    if ((bufpos + len + 1) > bufsz)
    {
      bufsz += MAX(len + 1, BUFSZ);
      buf = (gchar *) g_realloc(buf, bufsz);
    }
    strcpy(buf + bufpos, buffer);	bufpos += len;
    buf[bufpos] = '\n';			bufpos++;

    g_free(buffer);
    if ((buffer = grn_socket_get(sock)) == NULL)
    {
      g_free(buf);
      return _("server not responding");
    }
  }
  if (buffer)  g_free(buffer);

  buf[bufpos++] = '\0';
  buf = (gchar *) g_realloc(buf, bufpos);
  msg->body = buf;
  grn_decode_headers(msg->mh);
  msg->cached = TRUE;
  return NULL;
#undef BUFSZ
}

void nntp_download_article(t_message *msg)
{
  gint total=0, first=0, last=0;
  gchar *rc;
  g_return_if_fail(msg != NULL);
  g_return_if_fail(msg->mh != NULL);
  g_return_if_fail(msg->mh->grp != NULL);
  g_return_if_fail(msg->mh->grp->name != NULL);
  
  grn_appbar_push(GRN->appbar, FALSE, _("Fetching article #%d from group %s..."),
                  msg->mh->number, msg->mh->grp->name);
  rc = nntp_connect(GRN->nntp_sock);
  if (! rc)
  {
    rc = nntp_set_group(GRN->nntp_sock, msg->mh->grp->name, &total, &first, &last);
    if (! rc)
    {
      if ((msg->mh->number <= last) && (msg->mh->number >= first))
        rc = nntp_get_article(GRN->nntp_sock, msg);
      if (rc)  grn_error(_("NNTP get article"), rc);
    }
    else  grn_error(_("NNTP set group"), rc);
  }
  else  grn_error(_("NNTP connect"), rc);
  grn_appbar_pop(GRN->appbar);
}


gchar *nntp_post_article(grn_socket *sock, t_message *msg)
{
  gchar *buffer;
  g_return_val_if_fail(msg != NULL, FALSE);

  if (grn_socket_put(sock, "POST\r\n") == -1)  return _("server<-POST failed");
  if ((buffer = grn_socket_get(sock)) == NULL)  return _("server not responding");
  if (atoi(buffer) == 340)
  {
    gchar *mbuf = msg_to_text(msg);
    str_free(&buffer);
    if (grn_socket_put_va(sock, "%s\r\n.\r\n", mbuf) == -1)
      return _("posting failed");
    str_free(&mbuf);
    if ((buffer = grn_socket_get(sock)) == NULL)  return _("server not responding");
    if (atoi(buffer) == 240)
    {
      str_free(&buffer);  return NULL;
    }
    str_free(&buffer);
    return _("article refused by server");
  }
  else {
    str_free(&buffer);
    return _("error in server reply");
  }
  return _("undefined error");
}
