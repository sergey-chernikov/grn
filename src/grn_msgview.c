/*
 * grn_msgview.c
 *
 * $Id: grn_msgview.c,v 1.18 2000/06/06 15:45:45 sc Exp $
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
#include "grn_msgview.h"
#include <regex.h>
#include "grn_vars.h"
#include "grn_config.h"
#include "grn_util.h"


grn_html_buf *html_init_doc()
{
  grn_html_buf *ret = g_new(grn_html_buf, 1);
  
  ret->doc = NULL;
  ret->html = NULL;
  ret->data = NULL;
  return ret;
}

void html_free_doc(grn_html_buf **gxb)
{
  if (! gxb)  return;
  if (! *gxb)  return;
  if ((*gxb)->doc)  xmlFreeDoc((*gxb)->doc);
  str_free(&((*gxb)->html));
  g_free(*gxb);  *gxb = NULL;
}

xmlChar *html_escape(gchar *src)
{
  return xmlEncodeEntitiesReentrant(NULL, src);
}


static void add_coloured_text(grn_html_buf *gxb, gchar *s, gint idx)
{
  gchar *buf, *src;
  if (! s)  return;
  
  src = html_escape(s);
  if (idx <= 0)  buf = g_strdup(src);
  else {
    gchar *s = c2s(grn_prefs.fg_colors[idx]);
    buf = g_strdup_printf("<FONT COLOR=\"%s\">%s</FONT>", s, src);
    str_free(&s);
  }
  str_add(&(gxb->html), buf);
  str_free(&buf);  str_free(&src);
}

void html_parse_msg_body(grn_html_buf *gxb, gchar *body)
{
  gchar *tmp, *tmp1=NULL, *s, *src;
  regex_t regex;
  regmatch_t regmatch;
  gboolean signature = FALSE;
  g_return_if_fail(gxb != NULL);
  if (! body)  return;
  if (! str_check(grn_prefs.re_quoting))  return;
  
  if (regcomp(&regex, grn_prefs.re_quoting, REG_EXTENDED|REG_NOSUB))
  {
    grn_error(_("message parser"), _("Can't compile quoting regexp"));
    return;
  }

  src = g_strdup(body);  s = src;
  while (*s)
  {
    gchar *buf;

    tmp = strchr(s, '\n');
    if (tmp)
    {
      tmp1 = tmp + 1;
      *tmp = '\0';
    }
    if (! strcmp(s, "-- "))  signature = TRUE;
    buf = g_strconcat(s, "\n", NULL);
    if (signature)  add_coloured_text(gxb, buf, 2);
    else if (! regexec(&regex, s, 0, &regmatch, 0))
      add_coloured_text(gxb, buf, 1);
    else  add_coloured_text(gxb, buf, 0);
    str_free(&buf);
    if (tmp)  s = tmp1;
    else  break;
  }
  str_free(&src);  regfree(&regex);
}


static void process_html_node(GtkText *text, xmlNodePtr node);
static void parse_node_list(GtkText *text, xmlNodePtr node)
{
  while (node)
  {
    process_html_node(text, node);
    node = node->next;
  }
}

static void insert_br(GtkText *text)
{
  grn_lock();
  gtk_text_insert(text, NULL, NULL, NULL, "\n", strlen("\n"));
  grn_unlock();
}

typedef struct font_props
{
  gint idx, pidx, cidx, cpidx;
  gchar *name;
  gchar *cval;
} font_props;
static font_props *font_props_alloc()
{
  font_props *ret = g_new(font_props, 1);
  ret->idx = 0;  ret->pidx = -1;  ret->cidx = 0;  ret->cpidx = -1;
  ret->name = NULL;  ret->cval = NULL;
  return ret;
}
static font_props *font_props_copy(font_props *fp)
{
  font_props *ret = font_props_alloc();
  if (! fp)  return ret;
  ret->idx = fp->idx;  ret->pidx = fp->pidx;  ret->cidx = fp->cidx;
  ret->cpidx = fp->cpidx;
  ret->name = str_copy(fp->name);  ret->cval = str_copy(fp->cval);
  return ret;
}
static void font_props_free(font_props **fp)
{
  if ((! fp) || (! (*fp)))  return;

  str_free(&((*fp)->name));
  str_free(&((*fp)->cval));
  g_free(*fp);  *fp = NULL;
}

static void font_props_get(xmlAttrPtr attr, font_props *fp)
{
  gchar *s;
  if (! attr)  return;
  g_return_if_fail(fp != NULL);

  htmlAttrGetInt(attr, HTML_FONT_IDX, &(fp->idx));
  htmlAttrGetInt(attr, HTML_FONT_PIDX, &(fp->pidx));
  htmlAttrGetInt(attr, HTML_COLOR_IDX, &(fp->cidx));
  htmlAttrGetInt(attr, HTML_COLOR_PIDX, &(fp->cpidx));
  s = htmlAttrGet(attr, HTML_FONT_NAME);
  if (str_check(s))  fp->name = g_strdup(s);
  s = htmlAttrGet(attr, "Face");
  if (str_check(s))  fp->name = g_strdup(s);
  s = htmlAttrGet(attr, HTML_COLOR_VAL);
  if (str_check(s))  fp->cval = g_strdup(s);
}

static GdkFont *font_props_get_font(font_props *fp)
{
  GdkFont *font = NULL;
  if (! fp)  return NULL;

  if (str_check(fp->name))
  {
    grn_lock();
    font = gdk_fontset_load(fp->name);
    grn_unlock();
    if (font)  return font;
  }
  if (grn_prefs.fontpal)
  {
    gint n = g_slist_length(grn_prefs.fontpal);
    gchar *fname = NULL;
    gint pidx = fp->pidx - 1;

    if ((pidx >= 0) && (pidx < n))
      fname = (gchar *) g_slist_nth_data(grn_prefs.fontpal, pidx);
    if (str_check(fname))
    {
      grn_lock();
      font = gdk_fontset_load(fname);
      grn_unlock();
      if (font)  return font;
    }
  }
  if ((fp->idx > 0) && (fp->idx < NFONTS))
  {
    grn_lock();
    font = gdk_fontset_load(grn_prefs.fonts[fp->idx]);
    grn_unlock();
  }
  return font;
}

static GdkColor *font_props_get_clr(font_props *fp)
{
  GdkColor *clr = g_new(GdkColor, 1);
  GdkColormap *cmap;
  gboolean ret = FALSE;
  if (! fp)
  {
    GdkColor c = grn_prefs.fg_colors[0];
    clr->red = c.red;  clr->green = c.green;  clr->blue = c.blue;
    return clr;
  }

  grn_lock();
  cmap = gdk_colormap_get_system();
  grn_unlock();

  if (str_check(fp->cval))
  {
    grn_lock();
    ret = gdk_color_parse(fp->cval, clr);
    gdk_color_alloc(cmap, clr);
    grn_unlock();
    if (ret)  return clr;
  }
  if (grn_prefs.colorpal)
  {
    gint n = g_slist_length(grn_prefs.colorpal);
    GdkColor *c = NULL;
    gint cpidx = fp->cpidx - 1;

    if ((cpidx >= 0) && (cpidx < n))
      c = (GdkColor *) g_slist_nth_data(grn_prefs.colorpal, cpidx);
    if (c)
    {
      clr->red = c->red;  clr->green = c->green;  clr->blue = c->blue;
      grn_lock();
      ret = gdk_color_alloc(cmap, clr);
      grn_unlock();
      if (ret)  return clr;
    }
  }
  if ((fp->cidx >= 0) && (fp->cidx < NCOLORS))
  {
    GdkColor c = grn_prefs.fg_colors[fp->cidx];
    clr->red = c.red;  clr->green = c.green;  clr->blue = c.blue;
    ret = gdk_color_alloc(cmap, clr);
    if (ret)  return clr;
  }
  g_free(clr);
  return NULL;
}

static void set_field(GtkText *text, gchar *s, const gchar *key)
{
  GtkWidget *mw, *fld;
  if (! str_check(s)) return;
  
  mw = gtk_object_get_data(GTK_OBJECT(text), KEY_WND);
  if (! mw)  return;
  fld = gtk_object_get_data(GTK_OBJECT(mw), key);
  if (! fld)  return;
  if (GTK_IS_ENTRY(fld)) gtk_entry_set_text(GTK_ENTRY(fld), s);
}

static void process_input(GtkText *text, xmlNodePtr node)
{
  gchar *s, *s1;
  if (! node)  return;

  s = htmlAttrGet(node->properties, "Name");
  if (! str_check(s))  return;
  if (! g_strcasecmp(s, "from"))
  {
    s1 = htmlAttrGet(node->properties, "Value");
    if (str_check(s1))  set_field(text, s1, KEY_MW_FROM);
  }
  else if (! g_strcasecmp(s, "to"))
  {
    s1 = htmlAttrGet(node->properties, "Value");
    if (str_check(s1))  set_field(text, s1, KEY_MW_TO);
  }
  else if (! g_strcasecmp(s, "subject"))
  {
    s1 = htmlAttrGet(node->properties, "Value");
    if (str_check(s1))  set_field(text, s1, KEY_MW_SUBJ);
  }
}

static void process_headings(GtkText *text, xmlNodePtr node)
{
  xmlNodePtr n;
  if (node->type == XML_HTML_DOCUMENT_NODE)
  {
    for (n = ((xmlDocPtr) node)->root; n; n=n->next)  process_headings(text, node);
    return;
  }
  if (! g_strcasecmp(node->name, "input"))
    process_input(text, node);
  for (n=node->childs; n; n=n->next)  process_headings(text, n);
}

static void process_form(GtkText *text, xmlNodePtr node)
{
  gchar *s;
  xmlNodePtr n;
  if (! node)  return;

  s = htmlAttrGet(node->properties, "Name");
  if (! str_check(s))  return;
  if (! g_strcasecmp(s, "headings"))
  {
    for (n=node->childs; n; n=n->next)  process_headings(text, n);
  }
}

static void insert_text(GtkText *text, xmlNodePtr node, font_props **fp)
{
  font_props *fpr = NULL;
  gchar *s;
  xmlNodePtr n;
  
  if (node->type == XML_HTML_DOCUMENT_NODE)
  {
    for (n = ((xmlDocPtr) node)->root; n; n=n->next)
      insert_text(text, node, NULL);
    font_props_free(fp);
    return;
  }

  if (! g_strcasecmp(node->name, "form"))
    process_form(text, node);
  else if (! g_strcasecmp(node->name, "br"))  insert_br(text);
  else if (! g_strcasecmp(node->name, "p"))
  {
    insert_br(text);
    if (str_check(node->content))  insert_text(text, node, fp);
  }
  else if (! g_strcasecmp(node->name, "font"))
  {
    if (fp)  fpr = font_props_copy(*fp);
    else  fpr = font_props_alloc();
    font_props_get(node->properties, fpr);
  }

  if (node->type == XML_TEXT_NODE)
  {
    font_props_free(&fpr);
    if (fp)  fpr = font_props_copy(*fp);
  }
  s = node->content;
  if (s && (! g_strcasecmp(node->name, "text")))
  {
    GdkFont *font = font_props_get_font(fpr);
    GdkColor *color = font_props_get_clr(fpr);

    grn_lock();
    gtk_text_insert(text, font, color, NULL, s, strlen(s));
    if (font)  gdk_font_unref(font);
    if (color)  g_free(color);
    grn_unlock();
  }
  for (n=node->childs; n; n=n->next)  insert_text(text, n, &fpr);
  font_props_free(&fpr);
}

static void add_to_title(xmlNodePtr node)
{
  if (! node)  return;
  if (! str_check(node->content))  return;
  grn_set_title(GTK_WINDOW(GRN->window), "%s - %s", _(GRN_TITLE), node->content);
}
static void process_head(GtkText *text, xmlNodePtr node)
{
  xmlNodePtr n;
  if (node->type == XML_HTML_DOCUMENT_NODE)
  {
    for (n = ((xmlDocPtr) node)->root; n; n=n->next)  process_head(text, node);
    return;
  }
  if (! g_strcasecmp(node->name, "title"))
  {
    add_to_title(node->childs);
    return;
  }
  for (n=node->childs; n; n=n->next)  process_head(text, n);
}

static void process_html_node(GtkText *text, xmlNodePtr node)
{
  xmlNodePtr n;

  if (node->type == XML_HTML_DOCUMENT_NODE)
  {
    for (n=((xmlDocPtr) node)->root; n; n=n->next)
      process_html_node(text, node);
    return;
  }
  if (! g_strcasecmp(node->name, "head"))
  {
    for (n=node->childs; n; n=n->next)  process_head(text, n);
  }
  if (! g_strcasecmp(node->name, "body"))
  {
    for (n=node->childs; n; n=n->next)  insert_text(text, n, NULL);
  }
  for (n=node->childs; n; n=n->next)  process_html_node(text, n);
}

void html_draw_body(grn_html_buf *gxb)
{
  g_return_if_fail(gxb != NULL);
  g_return_if_fail(gxb->data != NULL);
  if (! gxb->html)  return;
  
  str_add(&(gxb->html), "\n");
  gxb->doc = htmlParseDoc(gxb->html, NULL);
  if (! gxb->doc)  return;
  parse_node_list(GTK_TEXT(gxb->data), gxb->doc->root);
}
