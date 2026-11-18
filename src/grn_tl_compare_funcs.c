/*
 * grn_tl_compare_funcs.c: compare functions for grn_threadlist.c
 *
 * $Id: grn_tl_compare_funcs.c,v 1.3 2000/04/10 02:42:52 sc Exp $
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

static gint hdr_compare_dates(const void *p1, const void *p2)
{
  t_msgheaders *mh1 = (t_msgheaders *) p1;
  t_msgheaders *mh2 = (t_msgheaders *) p2;
  
  return (gint) (mh1->parsed_date - mh2->parsed_date);
}

static gint hdr_compare_subjects(const void *p1, const void *p2)
{
  t_msgheaders *mh1 = (t_msgheaders *) p1;
  t_msgheaders *mh2 = (t_msgheaders *) p2;
  gint rc;
  
  if (! mh2)  return (mh1 != NULL);
  if (! mh1)  return (-1);
  if (! mh2->subject)  return (mh1->subject != NULL);
  if (! mh1->subject)  return (-1);
  rc = g_strcasecmp(mh1->subject, mh2->subject);
  if (! rc)  return hdr_compare_dates(p1, p2);
  else  return rc;
}

static gint hdr_compare_authors(const void *p1, const void *p2)
{
  t_msgheaders *mh1 = (t_msgheaders *) p1;
  t_msgheaders *mh2 = (t_msgheaders *) p2;
  gint rc;
  
  if (! mh2)  return (mh1 != NULL);
  if (! mh1)  return (-1);
  if (! mh2->from)  return (mh1->from != NULL);
  if (! mh1->from)  return (-1);
  rc = g_strcasecmp(mh1->from, mh2->from);
  if (! rc)  return hdr_compare_dates(p1, p2);
  else  return rc;
}

static gint hdr_compare_scores(const void *p1, const void *p2)
{
  t_msgheaders *mh1 = (t_msgheaders *) p1;
  t_msgheaders *mh2 = (t_msgheaders *) p2;
  gint rc = (mh1->score - mh2->score);
  
  if (! rc)  return hdr_compare_dates(p1, p2);
  else  return rc;
}

static gint hdr_compare_sizes(const void *p1, const void *p2)
{
  t_msgheaders *mh1 = (t_msgheaders *) p1;
  t_msgheaders *mh2 = (t_msgheaders *) p2;
  gint rc = (mh1->size - mh2->size);
  
  if (! rc)  return hdr_compare_dates(p1, p2);
  else  return rc;
}

static gint hdr_compare_lines(const void *p1, const void *p2)
{
  t_msgheaders *mh1 = (t_msgheaders *) p1;
  t_msgheaders *mh2 = (t_msgheaders *) p2;
  gint rc = (mh1->lines - mh2->lines);
  
  if (! rc)  return hdr_compare_sizes(p1, p2);
  else  return rc;
}

static gint no_sort(const void *p1, const void *p2)
{
  return 1;
}

static gint compare_dates(const void *p1, const void *p2)
{
  t_message *m1 = (t_message *) p1;
  t_message *m2 = (t_message *) p2;
  
  return (gint) (m1->mh->parsed_date - m2->mh->parsed_date);
}

static gint compare_subjects(const void *p1, const void *p2)
{
  t_message *m1 = (t_message *) p1;
  t_message *m2 = (t_message *) p2;
  gint rc;
  
  if (! m2)  return (m1 != NULL);
  if (! m1)  return (-1);
  if (! m2->mh->subject)  return (m1->mh->subject != NULL);
  if (! m1->mh->subject)  return (-1);
  rc = g_strcasecmp(m1->mh->subject, m2->mh->subject);
  if (! rc)  return compare_dates(p1, p2);
  else  return rc;
}

static gint compare_authors(const void *p1, const void *p2)
{
  t_message *m1 = (t_message *) p1;
  t_message *m2 = (t_message *) p2;
  gint rc;
  
  if (! m2)  return (m1 != NULL);
  if (! m1)  return (-1);
  if (! m2->mh->from)  return (m1->mh->from != NULL);
  if (! m1->mh->from)  return (-1);
  rc = g_strcasecmp(m1->mh->from, m2->mh->from);
  if (! rc)  return compare_dates(p1, p2);
  else  return rc;
}

static gint compare_scores(const void *p1, const void *p2)
{
  t_message *m1 = (t_message *) p1;
  t_message *m2 = (t_message *) p2;
  gint rc = (m1->mh->score - m2->mh->score);
  
  if (! rc)  return compare_dates(p1, p2);
  else  return rc;
}

static gint compare_sizes(const void *p1, const void *p2)
{
  t_message *m1 = (t_message *) p1;
  t_message *m2 = (t_message *) p2;
  gint rc = (m1->mh->size - m2->mh->size);
  
  if (! rc)  return compare_dates(p1, p2);
  else  return rc;
}

static gint compare_lines(const void *p1, const void *p2)
{
  t_message *m1 = (t_message *) p1;
  t_message *m2 = (t_message *) p2;
  gint rc = (m1->mh->lines - m2->mh->lines);
  
  if (! rc)  return compare_sizes(p1, p2);
  else  return rc;
}
