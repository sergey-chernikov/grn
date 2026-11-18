/*
 * grn_news.c: manipulating .newsrc file's "sets" (or sequences)
 *
 * $Id: grn_news.c,v 1.23 2000/04/28 12:32:31 sc Exp $
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
#include <unistd.h>
#include <gnome.h>
#include "nntp.h"
#include "grn_news.h"
#include "grn_vars.h"
#include "grn_types.h"
#include "grn_util.h"
#include "grn_config.h"


GHashTable *newsrc_ht = NULL, *ng_desc_ht = NULL;


static void cp_ng_hash(gpointer key, gpointer data, GHashTable *ht)
{
  gchar *new_key = g_strdup(key);
  gchar *new_data = g_strdup(data);
  g_hash_table_insert(ht, new_key, new_data);
}
static void write_ng_hash(gpointer key, gchar *value, FILE *f)
{
  fprintf(f, "%s: %s\n", (gchar *) key, (value != NULL) ? value : "");
}

void write_newsrc(const gchar *dstname, const gchar *srcname, GHashTable *ng_ht)
{
  gchar *backup_newsrc = grn_subst_tilde(grn_prefs.paths[1]);
  FILE *fsrc, *fdst;
  GHashTable *ht;
  gchar *sn, *dn;
  g_return_if_fail(dstname != NULL);
  if (! ng_ht)  return;
  
  ht = g_hash_table_new(g_str_hash, g_str_equal);
  g_hash_table_foreach(ng_ht, (GHFunc) cp_ng_hash, ht);

  sn = grn_subst_tilde(srcname);  dn = grn_subst_tilde(dstname);
  if (str_check(sn))  rename(sn, backup_newsrc);
  fdst = fopen(dn, "wt");
  if (fdst)
  {
    fsrc = fopen(backup_newsrc, "rt");
    if (fsrc)
    {
      gchar *s_tmp = (gchar *) g_malloc(64*1024 + 1);

      while (! feof(fsrc))
      {
        s_tmp[0] = '\0';
        fgets(s_tmp, 64*1024, fsrc);
        if (str_check(s_tmp))
        {
          t_msgheader *mh = parse_hdr(s_tmp);
	  if (mh)
	  {
            if (str_check(mh->name))
	    {
	      gchar *key, *value;
	      if (g_hash_table_lookup_extended(ht, mh->name, (gpointer *) &key,
	                                       (gpointer *) &value))
	      {
		write_ng_hash(mh->name, value, fdst);
	        g_hash_table_remove(ht, mh->name);
	        str_free(&key);  str_free(&value);
	      }
	    }
	    str_free(&(mh->name));  str_free(&(mh->value));
	    g_free(mh);
	  }
        }
      }
      str_free(&s_tmp);
      fclose(fsrc);
    }
    g_hash_table_foreach(ht, (GHFunc) write_ng_hash, fdst);
    fclose(fdst);
  }
  str_free(&backup_newsrc);  str_free(&sn);  str_free(&dn);
  free_newsrc(ht);
}

GHashTable *read_newsrc(const gchar *fname)
{
  GHashTable *out_ht;
  gchar *s_tmp;
  FILE *f;
  gchar *fn;

  out_ht = g_hash_table_new(g_str_hash, g_str_equal);

  fn = grn_subst_tilde(fname);
  if (! g_file_exists(fn))
  {
#ifdef GRN_DEBUG
    printf("read_newsrc@grn_news.c: %s not found\n", fn);
#endif
    return out_ht;
  }
  
  if (!(f=fopen(fn, "rt")))
  {
#ifdef GRN_DEBUG
    printf("read_newsrc@grn_news.c: Can't open %s for reading\n", newsrc_fn);
#endif
    return out_ht;
  }
  
  s_tmp = (gchar *) g_malloc(64*1024);
  while (! feof(f))
  {
    s_tmp[0] = '\0';
    fgets(s_tmp, 64*1024, f);
    if (strlen(s_tmp) > 0)
    {
      t_msgheader *mh = parse_hdr(s_tmp);
      if (mh)
      {
        if (str_check(mh->name))
        {
	  if (! mh->value)  mh->value = g_strdup("");
          g_hash_table_insert(out_ht, mh->name, mh->value);
        }
        else {
          str_free(&(mh->name));  str_free(&(mh->value));
        }
        g_free(mh);
      }
    }
  }
  fclose(f);
  str_free(&s_tmp);  str_free(&fn);

  return out_ht;
}

static void free_ng_hash(gchar *key, gchar *value, gpointer data)
{
  str_free(&key);  str_free(&value);
}
void free_newsrc(GHashTable *ng_ht)
{
  g_return_if_fail(ng_ht != NULL);
  g_hash_table_foreach(ng_ht, (GHFunc) free_ng_hash, NULL);
  g_hash_table_destroy(ng_ht);
}

gboolean change_newsrc(GHashTable *ng_ht, const gchar *key, const gchar *seq)
{
  gchar *key1, *value;
  g_return_val_if_fail(ng_ht != NULL, FALSE);
  g_return_val_if_fail(key != NULL, FALSE);
  if (! seq)  return FALSE;

  if (g_hash_table_lookup_extended(ng_ht, key, (gpointer *) &key1, (gpointer *) &value))
  {
    g_hash_table_remove(ng_ht, key);
    str_free(&key1);  str_free(&value);
    g_hash_table_insert(ng_ht, g_strdup(key), g_strdup(seq));
    return TRUE;
  }
  return FALSE;
}


static gchar *parse_get_seq(gchar *seq, glong *low, glong *high)
{
  *low = strtol(seq, &seq, 10);
  if (*seq == '-')
  {
    seq++;
    *high = strtol(seq, &seq, 10);
  }
  else  *high = *low;

  while(*seq && (*seq < '0' || *seq > '9'))  seq++;
  return seq;
}

static glong parse_seq(grn_newsgroup *ng)
{
  gchar *ptr;
  glong low=0, high=0, sum=0;
  
  if (! str_check(ng->seq))
  {
    if ((ng->first <= 0) || (ng->total <= 0))  return (-1);
    return ng->total;
  }
  ptr = ng->seq;
  while (*ptr)
  {
    ptr = parse_get_seq(ptr, &low, &high);

    if ((ng->first == ng->total) && (high == ng->first) && (high > 1))  return 2;
    if ((sum <= 0) && (high >= ng->first))  sum++;
    if (high < ng->first)  high = ng->first;
    if (low < ng->first)  low = ng->first;
    if (low == high)  sum++;
    else  sum += (high - low) + 1;
  }
  return sum;
}

glong grn_news_get_unread(grn_newsgroup *ng)
{
  glong nread;
  
  nread = parse_seq(ng);
  if ((nread >= 0))
  {
    if ((ng->total == ng->first) && (ng->first == 1))  nread++;
    return (ng->total - ng->first - nread + 2);
  }
  else  return -1;
}


void fill_newsrc_ng_list(GList *ng_list, GHashTable *ng_ht)
{
  GList *l;
  g_return_if_fail(ng_ht != NULL);
  
  for (l=ng_list; l; l=l->next)
  {
    grn_newsgroup *grp = (grn_newsgroup *) l->data;
    if (grp && grp->name)
    {
      gchar *seq = g_hash_table_lookup(ng_ht, grp->name);
      if (seq)
      {
        str_free(&(grp->seq));
	grp->seq = g_strdup(seq);
	grp->nunread = grn_news_get_unread(grp);
      }
    }
  }
}

void fill_newsrc_ng_desc(GList *ng_list, GHashTable *desc_ht)
{
  GList *l;
  g_return_if_fail(desc_ht != NULL);
  
  for (l=ng_list; l; l=l->next)
  {
    grn_newsgroup *grp = (grn_newsgroup *) l->data;
    if (grp && grp->name)
    {
      gchar *desc = g_hash_table_lookup(desc_ht, grp->name);
      if (desc)
      {
        str_free(&(grp->descr));
	grp->descr = g_strdup(desc);
      }
    }
  }
}


gboolean grn_news_art_read(gchar *seq, gulong msg_id)
{
  gchar *ptr;
  glong low=0, high=0;
  
  if (! str_check(seq))  return FALSE;
  ptr = seq;
  while (*ptr)
  {
    ptr = parse_get_seq(ptr, &low, &high);
    if ((msg_id >= low) && (msg_id <= high))  return TRUE;
  }
  return FALSE;
}


static gchar *subseq_construct(gulong low, gulong high)
{
  if (low == high)  return g_strdup_printf("%ld", high);
  else  return g_strdup_printf("%ld-%ld", low, high);
}

// Doesn't free the seq on return. Returns newly allocated string.
gchar *grn_news_add_read(gchar *seq, gulong msg_id)
{
  gchar *ret=NULL, *ret1=NULL, *ptr, *ptr1, *buf, *buf1;
  glong low=0, high=0, olow=0, ohigh=0, nlow=0, nhigh=0, min=0, max=0;

  if ((str_check(seq) && (grn_news_art_read(seq, msg_id))) || (msg_id <= 0))
    return g_strdup(seq);
  ptr = seq;
  while (ptr && (*ptr))
  {
    ptr = parse_get_seq(ptr, &low, &high);
    ptr1 = parse_get_seq(ptr, &nlow, &nhigh);
    if (msg_id == (low - 1))  low = msg_id;
    if (msg_id == (high + 1))  high = msg_id;
    if (ptr && (nhigh > 1) && ((high + 1) >= nlow))
    {
      high = nhigh;  ptr = ptr1;
    }
    if (low == ohigh)  low++;
    if ((low < min) || (min == 0))  min = low;
    if (high > max)  max = high;
    buf = subseq_construct(low, high);
    if ((olow > 0) && (ohigh > 0) && (msg_id > ohigh) && (msg_id < low))
    {
      buf1 = subseq_construct(msg_id, msg_id);
      if (ret)
      {
	ret1 = ret;
	ret = g_strconcat(ret1, ",", buf1, ",", buf, NULL);
	str_free(&ret1);
      }
      else  ret = g_strconcat(buf1, ",", buf, NULL);
      str_free(&buf1);
    }
    else {
      if (ret)
      {
        ret1 = ret;
	ret = g_strconcat(ret1, ",", buf, NULL);
	str_free(&ret1);
      }
      else  ret = g_strdup(buf);
    }
    str_free(&buf);
    olow = low;  ohigh = high;
  }
  buf1 = subseq_construct(msg_id, msg_id);
  if (msg_id < min)
  {
    if (ret)
    {
      ret1 = ret;
      ret = g_strconcat(buf1, ",", ret1, NULL);
      str_free(&ret1);
    }
    else  ret = g_strdup(buf1);
  }
  else if (msg_id > max)
  {
    if (ret)
    {
      ret1 = ret;
      ret = g_strconcat(ret1, ",", buf1, NULL);
      str_free(&ret1);
    }
    else  ret = g_strdup(buf1);
  }
  str_free(&buf1);
  return ret;
}

// Doesn't free the seq on return. Returns newly allocated string.
gchar *grn_news_add_unread(gchar *seq, gulong msg_id)
{
  gchar *ret=NULL, *ret1=NULL, *ptr, *buf, *buf1, *buf2;
  glong low=0, high=0, olow=0, ohigh=0;

  if (! str_check(seq))  return g_strdup("");
  if ((! grn_news_art_read(seq, msg_id)) || (msg_id <= 0))
    return g_strdup(seq);
  ptr = seq;
  while (*ptr)
  {
    ptr = parse_get_seq(ptr, &low, &high);
    olow = low;  ohigh = high;
    if (msg_id == low)  low++;
    if (msg_id == high)  high--;

    buf = subseq_construct(low, high);
    if ((olow == ohigh) && (olow == msg_id)) ;
    else if ((msg_id > low) && (msg_id < high))
    {
      buf1 = subseq_construct(low, msg_id - 1);
      buf2 = subseq_construct(msg_id + 1, high);
      if (ret)
      {
        ret1 = ret;
	ret = g_strconcat(ret1, ",", buf1, ",", buf2, NULL);
	str_free(&ret1);
      }
      else  ret = g_strconcat(buf1, ",", buf2, NULL);
      str_free(&buf1);  str_free(&buf2);
    }
    else {
      if (ret)
      {
        ret1 = ret;
	ret = g_strconcat(ret1, ",", buf, NULL);
	str_free(&ret1);
      }
      else  ret = g_strdup(buf);
    }
    str_free(&buf);
  }
  return ret;
}

// msg_id is first or last id (for marking all unread and read, respectively).
gchar *grn_news_read_all(gulong msg_id)
{
  return subseq_construct(1, msg_id);
}

void grn_news_group_subscribe(GHashTable *ng_ht, grn_newsgroup *grp)
{
  gchar *seq, *key;
  g_return_if_fail(ng_ht != NULL);
  g_return_if_fail(grp != NULL);
  
  if (! str_check(grp->name))  return;
  seq = g_hash_table_lookup(ng_ht, grp->name);
  if (seq)  return;
  
  seq = grn_news_read_all(MAX(1, grp->first - 1));
  key = g_strdup(grp->name);
  g_hash_table_insert(ng_ht, key, seq);
}

void grn_news_group_unsubscribe(GHashTable *ng_ht, grn_newsgroup *grp)
{
  gchar *seq;
  g_return_if_fail(ng_ht != NULL);
  g_return_if_fail(grp != NULL);
  
  if (! str_check(grp->name))  return;
  seq = g_hash_table_lookup(ng_ht, grp->name);
  if (! seq)  return;
  g_hash_table_remove(ng_ht, grp->name);
}
