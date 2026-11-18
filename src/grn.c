/*
 * grn.c: main() function and initialization routines
 *
 * $Id: grn.c,v 1.39 2000/07/19 06:26:21 sc Exp $
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <gdk/gdkprivate.h>
#include "grn_util.h"
#include "grn_misc.h"
#include "grn_commands.h"
#include "grn_menu.h"
#include "grn_types.h"
#include "grn_config.h"
#include "queue.h"
#include "grn_nglist.h"
#include "tcp.h"
#include "grn_mime.h"

#ifdef USE_PERL
#include "grn_perl.h"
#endif

#include "grn_pixmaps.h"


grn_session *GRN = NULL;
grn_flags flags;
grn_preferences grn_prefs;

gboolean must_exit = FALSE;


static void grn_pixmaps_init(void)
{
  GRN->pixmaps.balls[1].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[1].mask), NULL, ball_blue_xpm);
  GRN->pixmaps.balls[2].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[2].mask), NULL, ball_green_xpm);
  GRN->pixmaps.balls[3].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[3].mask), NULL, ball_cyan_xpm);
  GRN->pixmaps.balls[4].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[4].mask), NULL, ball_red_xpm);
  GRN->pixmaps.balls[5].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[5].mask), NULL, ball_magenta_xpm);
  GRN->pixmaps.balls[6].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[6].mask), NULL, ball_brown_xpm);
  GRN->pixmaps.balls[7].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[7].mask), NULL, ball_grey_xpm);
  GRN->pixmaps.balls[8].pixmap = NULL;
  GRN->pixmaps.balls[9].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[9].mask), NULL, ball_ltblue_xpm);
  GRN->pixmaps.balls[10].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[10].mask), NULL, ball_ltgreen_xpm);
  GRN->pixmaps.balls[11].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[11].mask), NULL, ball_ltcyan_xpm);
  GRN->pixmaps.balls[12].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[12].mask), NULL, ball_ltred_xpm);
  GRN->pixmaps.balls[13].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[13].mask), NULL, ball_ltmagenta_xpm);
  GRN->pixmaps.balls[14].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[14].mask), NULL, ball_yellow_xpm);
  GRN->pixmaps.balls[15].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[15].mask), NULL, ball_white_xpm);
  GRN->pixmaps.balls[16].pixmap = NULL;
  GRN->pixmaps.balls[0].pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.balls[0].mask), NULL, ball_xpm);
  GRN->pixmaps.new_art_group.pixmap = GRN->pixmaps.balls[10].pixmap;
  GRN->pixmaps.new_art_group.mask = GRN->pixmaps.balls[10].mask;
  GRN->pixmaps.subscr_group.pixmap = GRN->pixmaps.balls[2].pixmap;
  GRN->pixmaps.subscr_group.mask = GRN->pixmaps.balls[2].mask;
  GRN->pixmaps.caption_subscr.pixmap = GRN->pixmaps.balls[0].pixmap;
  GRN->pixmaps.caption_subscr.mask = GRN->pixmaps.balls[0].mask;
  GRN->pixmaps.caption_unread.pixmap = GRN->pixmaps.balls[0].pixmap;
  GRN->pixmaps.caption_unread.mask = GRN->pixmaps.balls[0].mask;
  GRN->pixmaps.unread_article.pixmap = GRN->pixmaps.balls[2].pixmap;
  GRN->pixmaps.unread_article.mask = GRN->pixmaps.balls[2].mask;
  GRN->pixmaps.thread_opened.pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.thread_opened.mask), NULL, mball_green_xpm);
  GRN->pixmaps.thread_closed.pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.thread_closed.mask), NULL, mball_xpm);
  GRN->pixmaps.mini.pixmap = gdk_pixmap_create_from_xpm_d(GRN->window->window,
    &(GRN->pixmaps.mini.mask), &(GRN->window->style->bg[GTK_STATE_NORMAL]), grn_mini_xpm);
}

static void set_small_icon(GtkWindow *wnd)
{
  GdkImlibImage *im;

  im = gdk_imlib_create_image_from_xpm_data(grn_mini_xpm);
  gnome_window_icon_set_default_from_imlib(im);
  gnome_window_icon_set_from_imlib(wnd, im);
}

static gboolean open_nglist(GtkWidget *w)
{
  GRN->grouplist = grouplist_create();
  grn_set_contents(GRN->grouplist);
  usleep(23);
  grnq_add_download_grouplist();
  grnq_add_draw_grouplist();
  return FALSE;
}

static void wnd_focused(GtkWindow *wnd, gpointer data)
{
  GRN->focused = wnd;
}

static void prepare_app()
{
  grn_lock();
  GRN->container = GTK_CONTAINER(gtk_frame_new(NULL));
  gtk_frame_set_shadow_type(GTK_FRAME(GRN->container), GTK_SHADOW_ETCHED_OUT);
  gtk_widget_show(GTK_WIDGET(GRN->container));
  GRN->window = gnome_app_new("GRN", _("Gnome ReadNews"));
  gtk_widget_realize(GRN->window);
  grn_pixmaps_init();
  set_small_icon(GTK_WINDOW(GRN->window));
  gtk_window_set_policy(GTK_WINDOW(GRN->window), TRUE, TRUE, FALSE);
  gnome_app_set_contents(GNOME_APP(GRN->window), GTK_WIDGET(GRN->container));
  gtk_signal_connect(GTK_OBJECT(GRN->window), "delete_event",
                     GTK_SIGNAL_FUNC(evt_cleanup), NULL);
  gtk_signal_connect(GTK_OBJECT(GRN->window), "focus_in_event",
		     GTK_SIGNAL_FUNC(wnd_focused), NULL);
  gtk_window_set_default_size(GTK_WINDOW(GRN->window), grn_prefs.ws[0].width,
                              grn_prefs.ws[0].height);
  gtk_widget_set_uposition(GRN->window, grn_prefs.ws[0].x, grn_prefs.ws[0].y);
  gnome_app_create_menus(GNOME_APP(GRN->window), main_menu);
  gnome_app_create_toolbar(GNOME_APP(GRN->window), main_toolbar);

  GRN->appbar = GNOME_APPBAR(gnome_appbar_new(TRUE, TRUE, GNOME_PREFERENCES_USER));
  gtk_signal_connect(GTK_OBJECT(GRN->appbar), "destroy",
                     GTK_SIGNAL_FUNC(evt_appbar_destroy), NULL);
  gnome_app_set_statusbar(GNOME_APP(GRN->window), GTK_WIDGET(GRN->appbar));
  gnome_app_install_menu_hints(GNOME_APP(GRN->window), main_menu);

  GRN->tooltips = gtk_tooltips_new();

  gtk_widget_set_name(GNOME_APP(GRN->window)->menubar, "main-menu");
  gtk_widget_set_name(GTK_WIDGET(GRN->appbar), "main-appbar");
  grn_unlock();

  grn_prefs_realize();
  grn_lock();
  gtk_widget_show_all(GRN->window);
  grn_unlock();

  grn_menus_init();
  build_scripts_menu();
  grn_lock();
  if (grn_prefs.srv_autoconnect)
    gtk_idle_add((GtkFunction) open_nglist, GTK_WINDOW(GRN->window));
  grn_unlock();
}

static gchar *cf_prefix = "/GRN/";
static struct poptOption prog_options[] =
{
  POPT_AUTOHELP
  {"config", 'c', POPT_ARG_STRING, &cf_prefix, 0,
   N_("Specify other configuration file ('=/dir/file=/' is for absolute pathname, "
      "'/dir/file/' is for relative to '~/.gnome' pathname, default is '/GRN/')."
      " All slashes are mandatory"),
   N_("config_prefix")},
  {NULL, '\0', 0, NULL, 0}
};


void grn_flags_init(void)
{
  flags.open_group = FALSE;
  flags.open_grouplist = FALSE;
  flags.open_article = FALSE;
  flags.post_article = FALSE;
}

static void grn_dirs_init(void)
{
#define NDIRS	5
  gchar *dir[NDIRS] = {
    ".grn", ".grn/hooks", ".grn/hooks/Perl",
    ".grn/commands", ".grn/commands/Perl"};
  gint i;
  
  for (i=0; i<NDIRS; i++)
  {
    gchar *s = gnome_util_prepend_user_home(dir[i]);
    if (mkdir(s, 0700) == 0)  g_message(_("Missing directory '~/%s' was created."), dir[i]);
    str_free(&s);
  }
#undef NDIRS
}

static void grn_init(void)
{
  gchar *s = gnome_util_prepend_user_home(".grn/gtkrc");
  gtk_rc_parse(s);
  str_free(&s);
  gnome_config_push_prefix(cf_prefix);
  grn_prefs_init();
  grn_prefs_load();
  
  GRN = g_malloc(sizeof(grn_session));
  GRN->ng_list = NULL;
  GRN->msghdr_list = NULL;
  GRN->appbar = NULL;
  GRN->client = NULL;
  GRN->grouplist = NULL;
  GRN->threadlist = NULL;
  GRN->msgwin = NULL;
  GRN->window = NULL;
  GRN->srv.name = NULL;
  GRN->srv.posting = FALSE;
  GRN->nntp_sock = grn_socket_new();
  GRN->find = grn_find_alloc();
  grn_find_load(GRN->find);

  grn_flags_init();
  grn_dirs_init();
  grnq_init();
  grn_mime_init();

#ifdef USE_PERL
  perl_init(grn_prefs.scripts_autoload);
#endif
}


int main(int argc, char *argv[])
{
  gtk_set_locale();
#ifdef ENABLE_NLS
  bindtextdomain(PACKAGE, GNOMELOCALEDIR);
  textdomain(PACKAGE);
#endif

  g_thread_init(NULL);

  gnome_init_with_popt_table(_("ReadNews"), VERSION, argc, argv, prog_options, 0, NULL);
#if 0
  session->client = gnome_master_client();
  gtk_signal_connect(GTK_OBJECT(client), "save_yourself",
                     GTK_SIGNAL_FUNC(evt_save_state), argv[0]);
#endif
  grn_init();
  gdk_threads_enter();
  prepare_app();
  gtk_main();
  gdk_threads_leave();

  return 0;
}
