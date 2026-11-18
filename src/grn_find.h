/*
 * grn_find.h - "Find" dialog routines
 *
 *
 * $Id: grn_find.h,v 1.3 2000/06/06 15:45:45 sc Exp $
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

#ifndef __GRN_FIND_H__
#define __GRN_FIND_H__


#include <fnmatch.h>
#include <gnome.h>
#include "grn_types.h"


#ifndef FNM_CASEFOLD
#define	FNM_CASEFOLD	(1 << 4)
#endif


void evt_find_dlg(GtkWidget *);
void evt_find_again(GtkWidget *);


#endif /* __GRN_FIND_H__ */
