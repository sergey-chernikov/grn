/*
 * grn_mhdrwin.h
 *
 * $Id: grn_mhdrwin.h,v 1.4 2000/04/10 02:42:52 sc Exp $
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

#ifndef __GRN_MHDRWIN_H__
#define __GRN_MHDRWIN_H__

#include "grn_types.h"


void evt_mhwin_show(GtkWidget *);
GtkWidget *mhdredit_create(void);
GtkWidget *mhdredit_dlg_create(t_msgheaders *mh);
void mhdredit_refresh(GtkWidget *mhe, t_msgheaders *mh);


#endif /* __GRN_MHDRWIN_H__ */
