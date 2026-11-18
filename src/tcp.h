/*
 * tcp.h
 *
 * $Id: tcp.h,v 1.1 2000/04/11 14:14:14 sc Exp $
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

#ifndef __GRN_TCP_H__
#define __GRN_TCP_H__

#include <gnome.h>
#include "grn_types.h"


grn_socket *grn_socket_new();
void grn_socket_free(grn_socket **);
void grn_socket_destroy(grn_socket *);
void grn_socket_open(grn_socket *);
void grn_socket_close(grn_socket *);
gchar *grn_socket_get(grn_socket *);
gint grn_socket_put(grn_socket *, gchar *);
gint grn_socket_put_va(grn_socket *, const gchar *format, ...);
void grn_socket_set_error(grn_socket *, gchar *);


#endif /* __GRN_TCP_H__ */
