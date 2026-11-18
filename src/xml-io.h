/*
 * xml-io.h
 *
 * $Id: xml-io.h,v 1.5 2000/04/10 02:42:53 sc Exp $
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

#ifndef __GRN_XML_IO_H__
#define __GRN_XML_IO_H__

#include <gnome-xml/HTMLparser.h>


typedef struct grn_html_buf
{
  htmlDocPtr doc;
  gchar *html;
  gpointer data;
} grn_html_buf;


xmlAttrPtr htmlSearchAttr(xmlAttrPtr, const gchar *);
gchar *htmlAttrGet(xmlAttrPtr, const gchar *);
gint htmlAttrGetInt(xmlAttrPtr, const gchar *, gint *);


#define HTML_COLOR_IDX		"ColorIndex"
#define HTML_COLOR_PIDX		"ColorPalIndex"
#define HTML_COLOR_VAL		"Color"
#define HTML_FONT_IDX		"FontIndex"
#define HTML_FONT_PIDX		"FontPalIndex"
#define HTML_FONT_NAME		"Name"

#endif /* __GRN_XML_IO_H__ */
