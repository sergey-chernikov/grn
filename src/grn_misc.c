/*
 * grn_misc.c - Implementation of miscellaneous project-wide functions
 *
 * $Id: grn_misc.c,v 1.39 2000/06/28 11:28:28 sc Exp $
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
#include "grn_vars.h"
#include "grn_misc.h"
#include "grn_util.h"
#include "grn_news.h"
#include "mutt.h"


t_msgheaders *t_msgheaders_alloc(void)
{
  t_msgheaders *mh = g_new(t_msgheaders, 1);

  mh->number = 0;  mh->lines = 0;
  mh->size = 0;    mh->parsed_date = 0;
  mh->score = 0;
  mh->subject = NULL;  mh->from = NULL;
  mh->date = NULL;     mh->id = NULL;
  mh->ref = NULL;      mh->xref = NULL;
  mh->path = NULL;     mh->newsgroups = NULL;
  mh->to = NULL;
  mh->grp = NULL;
  mh->extra_hdrs = NULL;
  return mh;
}

static void cp_extra_hdrs(gpointer key, gpointer data, GHashTable *ht)
{
  gchar *new_key = g_strdup(key);
  gchar *new_data = str_copy(data);
  g_hash_table_insert(ht, new_key, new_data);
}
t_msgheaders *t_msgheaders_copy(t_msgheaders *src)
{
  t_msgheaders *mh = t_msgheaders_alloc();

  mh->number = src->number;  mh->lines = src->lines;
  mh->size = src->size;      mh->parsed_date = src->parsed_date;
  mh->score = src->score;
  mh->subject = str_copy(src->subject);
  mh->from = str_copy(src->from);
  mh->date = str_copy(src->date);
  mh->id = str_copy(src->id);
  mh->ref = str_copy(src->ref);
  mh->xref = str_copy(src->xref);
  mh->path = str_copy(src->path);
  mh->newsgroups = str_copy(src->newsgroups);
  mh->to = str_copy(src->to);
  mh->grp = src->grp;
  mh->extra_hdrs = g_hash_table_new(g_str_hash, g_str_equal);
  if (src->extra_hdrs)
    g_hash_table_foreach(src->extra_hdrs, (GHFunc) cp_extra_hdrs, mh->extra_hdrs);
  return mh;
}

static void rm_extra_hdrs(gchar *key, gchar *data, GHashTable *ht)
{
  str_free(&key);  str_free(&data);
}
void t_msgheaders_rm_extra_hdrs(t_msgheaders *mh)
{
  if (! mh->extra_hdrs)  return;
  g_hash_table_foreach(mh->extra_hdrs, (GHFunc) rm_extra_hdrs, mh->extra_hdrs);
  g_hash_table_destroy(mh->extra_hdrs);  mh->extra_hdrs = NULL;
}

void t_msgheaders_free(t_msgheaders **mh)
{
  g_return_if_fail(*mh != NULL);
  str_free(&((*mh)->subject));
  str_free(&((*mh)->from));
  str_free(&((*mh)->date));
  str_free(&((*mh)->id));
  str_free(&((*mh)->ref));
  str_free(&((*mh)->xref));
  str_free(&((*mh)->path));
  str_free(&((*mh)->newsgroups));
  str_free(&((*mh)->to));
  t_msgheaders_rm_extra_hdrs(*mh);
  g_free(*mh);   *mh = NULL;
}


gchar *t_msgheaders_get_hdr(t_msgheaders *mh, gchar *hdr)
{
  g_return_val_if_fail(mh != NULL, NULL);
  g_return_val_if_fail(hdr != NULL, NULL);
  
  if (! strcmp(hdr, "Subject"))  return mh->subject;
  if (! strcmp(hdr, "From"))  return mh->from;
  if (! strcmp(hdr, "Date"))  return mh->date;
  if (! strcmp(hdr, "Message-ID"))  return mh->id;
  if (! strcmp(hdr, "References"))  return mh->ref;
  if (! strcmp(hdr, "Xref"))  return mh->xref;
  if (! strcmp(hdr, "Path"))  return mh->path;
  if (! strcmp(hdr, "Newsgroups"))  return mh->newsgroups;
  if (! strcmp(hdr, "To"))  return mh->to;
  if (! strcmp(hdr, "Lines"))
  {
    static gchar s[10];
    g_snprintf(s, 9, "%lu", mh->lines);  return s;
  }
  if (! strcmp(hdr, "Size"))
  {
    static gchar s[10];
    g_snprintf(s, 9, "%lu", mh->size);  return s;
  }
  return t_msgheaders_get_extra_hdr(mh, hdr);
}

gchar *t_msgheaders_get_extra_hdr(t_msgheaders *mh, gchar *hdr)
{
  g_return_val_if_fail(mh != NULL, NULL);
  g_return_val_if_fail(hdr != NULL, NULL);
  
  if (! mh->extra_hdrs)  return NULL;
  return g_hash_table_lookup(mh->extra_hdrs, hdr);
}

void t_msgheaders_set_hdr(t_msgheaders *mh, gchar *hdr, gchar *data)
{
  g_return_if_fail(mh != NULL);
  g_return_if_fail(hdr != NULL);
  if (! data)  return;

  if (! strcmp(hdr, "Subject"))
  {
    str_free(&(mh->subject));  mh->subject = g_strdup(data);
  }
  else if (! strcmp(hdr, "From"))
  {
    str_free(&(mh->from));  mh->from = g_strdup(data);
  }
  else if (! strcmp(hdr, "Date"))
  {
    str_free(&(mh->date));  mh->date = g_strdup(data);
  }
  else if (! strcmp(hdr, "Message-ID"))
  {
    str_free(&(mh->id));  mh->id = g_strdup(data);
  }
  else if (! strcmp(hdr, "References"))
  {
    str_free(&(mh->ref));  mh->ref = g_strdup(data);
  }
  else if (! strcmp(hdr, "Xref"))
  {
    str_free(&(mh->xref));  mh->xref = g_strdup(data);
  }
  else if (! strcmp(hdr, "Path"))
  {
    str_free(&(mh->path));  mh->path = g_strdup(data);
  }
  else if (! strcmp(hdr, "Newsgroups"))
  {
    str_free(&(mh->newsgroups));  mh->newsgroups = g_strdup(data);
  }
  else if (! strcmp(hdr, "To"))
  {
    str_free(&(mh->to));  mh->to = g_strdup(data);
  }
  else  t_msgheaders_add_extra_hdr(mh, hdr, data);
}

/* hdr and data are g_strdup()'ed inside the function */
void t_msgheaders_add_extra_hdr(t_msgheaders *mh, gchar *hdr, gchar *data)
{
  g_return_if_fail(mh != NULL);
  g_return_if_fail(hdr != NULL);
  if (! data)  return;

  if (! mh->extra_hdrs)  mh->extra_hdrs = g_hash_table_new(g_str_hash, g_str_equal);
  t_msgheaders_rm_extra_hdr(mh, hdr);
  g_hash_table_insert(mh->extra_hdrs, g_strdup(hdr), g_strdup(data));
}

void t_msgheaders_rm_hdr(t_msgheaders *mh, gchar *hdr)
{
  g_return_if_fail(mh != NULL);
  g_return_if_fail(hdr != NULL);
  
  if (! strcmp(hdr, "Subject"))  str_free(&(mh->subject));
  else if (! strcmp(hdr, "From"))  str_free(&(mh->from));
  else if (! strcmp(hdr, "Date"))  str_free(&(mh->date));
  else if (! strcmp(hdr, "Message-ID"))  str_free(&(mh->id));
  else if (! strcmp(hdr, "References"))  str_free(&(mh->ref));
  else if (! strcmp(hdr, "Xref"))  str_free(&(mh->xref));
  else if (! strcmp(hdr, "Path"))  str_free(&(mh->path));
  else if (! strcmp(hdr, "Newsgroups"))  str_free(&(mh->newsgroups));
  else if (! strcmp(hdr, "To"))  str_free(&(mh->to));
  else if (! strcmp(hdr, "Lines"))  mh->lines = 0;
  else  t_msgheaders_rm_extra_hdr(mh, hdr);
}

void t_msgheaders_rm_extra_hdr(t_msgheaders *mh, gchar *hdr)
{
  gchar *key, *value;
  g_return_if_fail(mh != NULL);
  g_return_if_fail(hdr != NULL);
  if (! mh->extra_hdrs)  return;
  
  if (g_hash_table_lookup_extended(mh->extra_hdrs, hdr, (gpointer *) &key,
        (gpointer *) &value))
  {
    g_hash_table_remove(mh->extra_hdrs, hdr);
    str_free(&key);  str_free(&value);
  }
}

void msghdrlist_free(GList **gl)
{
  GList *l;
  if (! gl)  return;
  if (! *gl)  return;

  for(l=*gl; l; l=l->next)
  {
    t_msgheaders *mh = (t_msgheaders *) l->data;
    if (mh)  t_msgheaders_free(&mh);
  }
  g_list_free(*gl);  *gl = NULL;
}


t_message *t_message_alloc(void)
{
  t_message *msg = g_new(t_message, 1);

  msg->mh = NULL;
  msg->body = NULL;
  msg->threads = NULL;
  msg->nchildren = NULL;
  msg->pos = 0;
  msg->tagged = FALSE;
  msg->cached = FALSE;
  msg->data = NULL;
  msg->attachments = NULL;
  msg->temp_files = NULL;
  msg->filename = NULL;
  msg->fp = NULL;
  return msg;
}

t_message *t_message_copy(t_message *src)
{
  t_message *m;
  GSList *l;
  if (! src)  return NULL;

  m = t_message_alloc();
  if (src->mh)  m->mh = t_msgheaders_copy(src->mh);
  m->body = str_copy(src->body);
  m->tagged = src->tagged;
  m->pos = src->pos;
  for (l=src->attachments; l; l=l->next)
  {
    BODY *b = (BODY *) l->data;
    if (! b)  continue;
    m->attachments = g_slist_append(m->attachments, mutt_copy_body(b, NULL));
  }
  return m;
}

void t_message_free(t_message **msg)
{
  GSList *l;

  g_return_if_fail(*msg != NULL);
  if ((*msg)->mh)  t_msgheaders_free(&((*msg)->mh));
  str_free(&((*msg)->body));

  if ((*msg)->fp)  fclose((*msg)->fp);
  if ((*msg)->attachments)
  {
    for (l=(*msg)->attachments; l; l=l->next)
    {
      BODY *b = (BODY *) l->data;
      if (! b)  continue;
      mutt_free_body(&b);
    }
    g_slist_free((*msg)->attachments);
  }
  if ((*msg)->temp_files)
  {
    for (l=(*msg)->temp_files; l; l=l->next)
    {
      gchar *s = l->data;
      if (str_check(s))  unlink(s);
      str_free(&s);
    }
    g_slist_free((*msg)->temp_files);
  }
  if ((*msg)->data)
  {
    BODY *b = (BODY *) ((*msg)->data);
    mutt_free_body(&b);
  }
  str_free(&((*msg)->filename));

  msglist_free(&((*msg)->threads));
  g_free(*msg);   *msg = NULL;
}

void t_message_destroy(t_message *msg)
{  t_message_free(&msg);  }

gboolean t_message_is_read(t_message *msg)
{
  if (! msg)  return FALSE;
  if (! msg->mh)  return FALSE;
  if (! msg->mh->grp)  return FALSE;
  return grn_news_art_read(msg->mh->grp->seq, msg->mh->number);
}

void msglist_free(GList **gl)
{
  GList *l;
  if (! gl)  return;
  if (! *gl)  return;

  for(l=*gl; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    if (! msg)  continue;
    t_message_free(&msg);
  }
  g_list_free(*gl);  *gl = NULL;
}

void msglist_destroy(GList *gl)
{  msglist_free(&gl);  }

static gboolean msg_in_cur_thread(GList *ml, t_message *msg)
{
  GList *l;
  if (! msg)  return FALSE;
  if (! ml)  return FALSE;
  
  for (l=ml; l; l=l->next)
  {
    t_message *m = (t_message *) l->data;
    if (! m)  continue;
    
    if (m == msg)  return TRUE;
    if (m->threads)
      if (msg_in_cur_thread(m->threads, msg))  return TRUE;
  }
  return FALSE;
}

t_message *msglist_find_cur_thread(GList *ml, t_message *msg)
{
  GList *l;
  if (! msg)  return NULL;
  
  for (l=ml; l; l=l->next)
  {
    t_message *m = (t_message *) l->data;
    if (! m)  continue;
    if (m == msg)  return msg;
    if (! m->threads)  continue;
    if (msg_in_cur_thread(m->threads, msg))  return m;
  }
  return NULL;
}

static void generate_plain_unread_list(GList *ml, GList **ret)
{
  GList *l;
  if (! ml)  return;
  
  for (l=ml; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    if (! msg)  continue;

    *ret = g_list_prepend(*ret, msg);
    if (msg->threads)  generate_plain_unread_list(msg->threads, ret);
  }
}
static GList *get_prev_next_unread(GList *pml, gboolean prev)
{
  GList *l = pml;
  if (! pml)  return NULL;
  
  if (prev)  l = l->prev;
  else  l = l->next;
  while (l)
  {
    t_message *msg = (t_message *) l->data;
    if (! msg)  continue;
    if (! t_message_is_read(msg))  return l;
    if (prev)  l = l->prev;
    else  l = l->next;
  }
  return NULL;
}

t_message *msglist_find_prev_unread(GList *ml, t_message *msg)
{
  t_message *prev = NULL;
  GList *found;
  if (! ml)  return NULL;
  if (! msg)  return NULL;
  if ((! msg->mh) && (! msg->mh->grp))  return NULL;

  if (! msg->mh->grp->plain_unread)
  {
    generate_plain_unread_list(ml, &(msg->mh->grp->plain_unread));
    msg->mh->grp->plain_unread = g_list_reverse(msg->mh->grp->plain_unread);
  }
  found = g_list_find(msg->mh->grp->plain_unread, msg);
  if (found)  found = get_prev_next_unread(found, TRUE);
  if (found)  prev = (t_message *) found->data;
  return  prev;  
}

t_message *msglist_find_next_unread(GList *ml, t_message *msg)
{
  t_message *next = NULL;
  GList *found;
  if (! ml)  return NULL;
  if (! msg)  return NULL;
  if ((! msg->mh) && (! msg->mh->grp))  return NULL;

  if (! msg->mh->grp->plain_unread)
  {
    generate_plain_unread_list(ml, &(msg->mh->grp->plain_unread));
    msg->mh->grp->plain_unread = g_list_reverse(msg->mh->grp->plain_unread);
  }
  found = g_list_find(msg->mh->grp->plain_unread, msg);
  if (found)  found = get_prev_next_unread(found, FALSE);
  if (found)  next = (t_message *) found->data;
  return  next;
}

static t_message *get_next_unread_in_thread(t_message *thread)
{
  GList *l;
  if (! thread)  return NULL;
  if (! t_message_is_read(thread))  return thread;
  if (! thread->threads)  return NULL;
  for (l=thread->threads; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    t_message *ret_msg = NULL;
    if (! msg)  continue;
    ret_msg = get_next_unread_in_thread(msg);
    if (ret_msg)  return ret_msg;
  }
  return NULL;
}

t_message *msglist_find_prev_thread_unread(GList *ml, t_message *msg)
{
  t_message *cur_thread, *prev=NULL;
  GList *l;
  if (! ml)  return NULL;
  if (! msg)  return NULL;
  
  cur_thread = msglist_find_cur_thread(ml, msg);
  l = g_list_find(ml, cur_thread);
  if (! l)  return NULL;
  l = l->prev;
  while (! prev)
  {
    if (! l)  break;
    prev = get_next_unread_in_thread((t_message *) l->data);
    l = l->prev;
  }
  return prev;
}

t_message *msglist_find_next_thread_unread(GList *ml, t_message *msg)
{
  t_message *cur_thread, *next=NULL;
  GList *l;
  if (! ml)  return NULL;
  if (! msg)  return NULL;
  
  cur_thread = msglist_find_cur_thread(ml, msg);
  l = g_list_find(ml, cur_thread);
  if (! l)  return NULL;
  l = l->next;
  while (! next)
  {
    if (! l)  break;
    next = get_next_unread_in_thread((t_message *) l->data);
    l = l->next;
  }
  return next;
}


t_message *msglist_find_by_id(GList *ml, t_message *msrc)
{
  GList *l;
  gchar *ref, *ref_prev;
  t_message *ret = NULL;
  
  if (! ml)  return NULL;
  if (! msrc)  return NULL;
  if (! msrc->mh)  return NULL;

  for (l=ml; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    if (! msg)  continue;

    ret = msglist_find_by_id(msg->threads, msrc);
    if (ret)  break;

    if (msg->mh && msrc->mh->ref)
    {
      gchar *buf = g_strdup(msrc->mh->ref);
      ref = strtok(buf, " ");
      while (1)
      {
        ref_prev = ref;
	ref = strtok(NULL, " ");
	if ((! ref) || (ref[0] == '\0'))  break;
      }
      ref = ref_prev;
      str_free(&buf);
      if (msg->mh->id && (! strcmp(msg->mh->id, ref)))  { ret=msg; break; }
    }
  }
  return ret;
}

static gchar *strip_re(gchar *src)
{
  while (*src && (strstr(src, "Re: ") == src))  src += 4;
  return src;
}

t_message *msglist_find_by_subj(GList *ml, t_message *msrc)
{
  GList *l;
  t_message *ret = NULL;
  
  if (! ml)  return NULL;
  if (! msrc)  return NULL;
  if (! msrc->mh)  return NULL;

  for (l=ml; l; l=l->next)
  {
    t_message *msg = (t_message *) l->data;
    if (! msg)  continue;

    if (msg->mh)
    {
      if (msg->mh->ref && msrc->mh->ref &&
          (! strcmp(msrc->mh->ref, msg->mh->ref)))  { ret=msg;  break; }
      if (msg->mh->subject && msrc->mh->subject)
      {
        gchar *str1 = strip_re(msg->mh->subject);
        gchar *str2 = strip_re(msrc->mh->subject);
        if (! strcmp(str1, str2))  { ret=msg;  break; }
      }
    }
    ret = msglist_find_by_subj(msg->threads, msrc);
    if (ret)  break;
  }
  return ret;
}


grn_newsgroup *grn_newsgroup_alloc(void)
{
  grn_newsgroup *grp = g_new(grn_newsgroup, 1);
  
  grp->name = NULL;
  grp->descr = NULL;
  grp->seq = NULL;
  grp->plain_unread = NULL;
  grp->nunread = -1;

  return grp;
}

void grn_newsgroup_free(grn_newsgroup **grp)
{
  g_return_if_fail(*grp != NULL);

  str_free(&((*grp)->name));
  str_free(&((*grp)->descr));
  str_free(&((*grp)->seq));
  if ((*grp)->plain_unread)  g_list_free((*grp)->plain_unread);
  g_free(*grp);   *grp = NULL;
}

void grn_newsgroup_set_seq(grn_newsgroup *grp, gchar *seq)
{
  g_return_if_fail(grp != NULL);
  
  str_free(&(grp->seq));
  grp->seq = seq;
  grp->nunread = grn_news_get_unread(grp);
  if (newsrc_ht && grp->name)  change_newsrc(newsrc_ht, grp->name, grp->seq);
}


void grouplist_free(GList **gl)
{
  GList *l;
  g_return_if_fail(gl != NULL);
  if (! *gl)  return;

  for(l=*gl; l; l=l->next)
  {
    grn_newsgroup *grp = (grn_newsgroup *) l->data;
    if (grp)  grn_newsgroup_free(&grp);
  }
  g_list_free(*gl);  *gl = NULL;
}

gulong grouplist_get_nunread(GList *gl)
{
  GList *l;
  gulong cnt=0;
  if (! gl)  return 0;
  
  for (l=gl; l; l=l->next)
  {
    grn_newsgroup *grp = (grn_newsgroup *) l->data;
    if (grp && (grp->nunread > 0))  cnt++;
  }
  return cnt;
}


grn_find *grn_find_alloc(void)
{
  grn_find *gf = g_new(grn_find, 1);

  gf->data = g_strdup("");	gf->hdr = g_strdup("X-Comment-To");
  gf->search_from = FALSE;	gf->search_subj = TRUE;
  gf->search_body = FALSE;	gf->search_hdr = FALSE;
  gf->is_regex = FALSE;		gf->sensitive = FALSE;
  gf->unread_only = TRUE;
  return gf;
}

void grn_find_free(grn_find **gf)
{
  g_return_if_fail(*gf != NULL);

  str_free(&((*gf)->data));
  str_free(&((*gf)->hdr));
  g_free(*gf);   *gf = NULL;
}
