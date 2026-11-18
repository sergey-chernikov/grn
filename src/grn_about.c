/*
 * grn_about.c - 'About' dialog
 *
 * $Id: grn_about.c,v 1.17 2000/05/25 07:47:31 sc Exp $
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
#include <gnome.h>
#include "grn_util.h"

static gint about_visible = FALSE;

static void about_destroyed(void)
{
  about_visible = FALSE;
}

void evt_about(GtkWidget *w, gpointer data)
{
  GtkWidget *about;
  const gchar *authors[] =
  { "Sergey Chernikov <sc@ivvs.ul.ru>",
    NULL };
  
  if (about_visible)  return;
  else about_visible = TRUE;
  
  grn_lock();
  about = gnome_about_new(_("Gnome ReadNews"), VERSION, "(C) 1999-2000 Sergey Chernikov",
          authors,
	  _("You can find some more info on Grn at http://ivvs.ul.ru/~sc/grn\n"
          "Please, report bugs to grn-bugs@ivvs.ul.ru"),
	  "grn.png");
  gtk_signal_connect(GTK_OBJECT(about), "destroy",
                     GTK_SIGNAL_FUNC(about_destroyed), NULL);
  gtk_widget_show(about);
  grn_unlock();
}
