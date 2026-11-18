/*
 * grn_mime.h - MIME handling routines
 *
 *
 * $Id: grn_mime.h,v 1.5 2000/07/19 06:26:21 sc Exp $
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

#ifndef __GRN_MIME_H__
#define __GRN_MIME_H__

#include "mutt.h"
#include "grn_types.h"


void grn_mime_init(void);
void grn_decode_headers(t_msgheaders *);
void grn_mime_parse_message(t_message *);

grn_mime_part *grn_mime_part_alloc(void);
void grn_mime_part_free(grn_mime_part **);
GSList *body_to_mime_parts(BODY *, GSList *, FILE *);

gboolean is_8bit(gchar *);
void add_mime_to_plaintext(t_message *);
gchar *body_mtype(BODY *);
gchar *menc2str(BODY *);

void evt_mime_view(GtkWidget *);


#endif /* __GRN_MIME_H__ */
