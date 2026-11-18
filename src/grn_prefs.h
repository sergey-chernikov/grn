/*
 * grn_prefs.h
 *
 * $Id: grn_prefs.h,v 1.6 2000/04/10 02:42:52 sc Exp $
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

#ifndef __GRN_PREFS_H__
#define __GRN_PREFS_H__

#include <gnome.h>

#define PREFS_NUM_TOOLBAR_STYLES	4

void evt_preferences(GtkWidget *);
void evt_fontpal(GtkWidget *);
void evt_colorpal(GtkWidget *);

#endif /* __GRN_PREFS_H__ */
