/*
 * grn_msgview.h
 *
 * $Id: grn_msgview.h,v 1.5 2000/04/10 02:42:52 sc Exp $
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

#ifndef __GRN_MSGVIEW_H__
#define __GRN_MSGVIEW_H__

#include <gnome.h>
#include "xml-io.h"


grn_html_buf *html_init_doc(void);
void html_free_doc(grn_html_buf **);
xmlChar *html_escape(gchar *);
void html_parse_msg_body(grn_html_buf *, gchar *);
void html_draw_body(grn_html_buf *);

#endif /* __GRN_MSGVIEW_H__ */
