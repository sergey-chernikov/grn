/*
 * grn_config.c: functions for configuration saving/retrieving
 *
 * $Id: grn_config.c,v 1.32 2000/06/28 11:28:28 sc Exp $
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
#include <unistd.h>
#include <pwd.h>
#include <gnome.h>
#include "grn_config.h"
#include "grn_vars.h"
#include "grn_util.h"


static void grn_prefs_alloc(void)
{
}

static void grn_prefs_free(void)
{
  gint i;
  GSList *l;

  str_free(&(grn_prefs.nntp_server));
  str_free(&(grn_prefs.nntp_login));
  str_free(&(grn_prefs.nntp_passwd));
  str_free(&(grn_prefs.smtp_server));
  str_free(&(grn_prefs.re_quoting));
  str_free(&(grn_prefs.name));
  str_free(&(grn_prefs.email));
  str_free(&(grn_prefs.replyto));
  str_free(&(grn_prefs.org));
  str_free(&(grn_prefs.ua));
  for (i=0; i<NFONTS; i++)  str_free(&(grn_prefs.fonts[i]));
  for (i=0; i<NPIXMAPS; i++)
  {
    if (grn_prefs.pixmaps[i])  gdk_pixmap_unref(grn_prefs.pixmaps[i]);
    str_free(&(grn_prefs.pm_names[i]));
  }
  for (l=grn_prefs.fontpal; l; l=l->next)  g_free(l->data);
  for (l=grn_prefs.colorpal; l; l=l->next)  g_free(l->data);
  g_slist_free(grn_prefs.fontpal);   grn_prefs.fontpal = NULL;
  g_slist_free(grn_prefs.colorpal);  grn_prefs.colorpal = NULL;
}

#define FNT_BODY_P	"-*-helvetica-medium-r-normal--12-*"
#define FNT_BODY_M	"-*-fixed-medium-r-normal-*-*-140-*"
static void fill_fonts()
{
  grn_prefs.fonts[0] = g_strdup(FNT_BODY_P);
  grn_prefs.fonts[1] = g_strdup(FNT_BODY_M);
  grn_prefs.fonts[2] = g_strdup(FNT_BODY_P);
  grn_prefs.fonts[3] = g_strdup(FNT_BODY_P);
  grn_prefs.fonts[4] = g_strdup(FNT_BODY_P);
}

gchar *prefs_fg_colors_ids[NCOLORS] =
{
  "/GRN/Fg_Colors/article_body",
  "/GRN/Fg_Colors/quoted_text",
  "/GRN/Fg_Colors/signature"
};
gchar *prefs_bg_colors_ids[NCOLORS] =
{
  "/GRN/Bg_Colors/article_body",
  "/GRN/Bg_Colors/quoted_text",
  "/GRN/Bg_Colors/signature"
};
static void fill_def_fg_colors(GdkColor *colors)
{
  colors[0].red = 0;       colors[0].green = 0;       colors[0].blue = 0;
  colors[1].red = 0xffff;  colors[1].green = 0;       colors[1].blue = 0xffff;
  colors[2].red = 0;       colors[2].green = 0;       colors[2].blue = 0xffff;
}
static void fill_def_bg_colors(GdkColor *colors)
{
  colors[0].red = 0xffff;  colors[0].green = 0xffff;  colors[0].blue = 0xffff;
  colors[1].red = 0;       colors[1].green = 0;       colors[1].blue = 0;
  colors[2].red = 0;       colors[2].green = 0;       colors[2].blue = 0;
}

void grn_prefs_init(void)
{
  gint i;
  struct passwd *pwd;
  gchar *buf, *p;

  grn_prefs_alloc();

  grn_prefs.srv_autoconnect = FALSE;
  grn_prefs.nntp_auth = FALSE;
  grn_prefs.groups_desc = TRUE;
  grn_prefs.scripts_autoload = TRUE;
  grn_prefs.tl_show_read = FALSE;
  grn_prefs.tl_ac = TRUE;
  grn_prefs.mw_prop = TRUE;
  grn_prefs.tooltips = TRUE;
  grn_prefs.smooth_scrolling = FALSE;
  for (i=0; i<NCFMS; i++)  grn_prefs.cfms[i] = TRUE;
  grn_prefs.toolbar_style = -1;
  grn_prefs.srt_ng_subscr = 0;
  grn_prefs.srt_ng_col = 3;
  grn_prefs.thrd_tl_type = 0;
  grn_prefs.srt_tl_col = 5;
  for (i=0; i<NWINSTATES; i++)
  {
    grn_prefs.ws[i].width = 0;  grn_prefs.ws[i].height = 0;
    grn_prefs.ws[i].x = 0;        grn_prefs.ws[i].y = 0;
  }
  
  pwd = getpwuid(getuid());
  buf = g_strdup(pwd->pw_gecos);
  p = strchr(buf, ',');
  if (p)  *p = 0;
  grn_prefs.name = g_strdup(buf);
  str_free(&buf);
  grn_prefs.nntp_server = g_strdup("news");
  grn_prefs.nntp_login = NULL;  grn_prefs.nntp_passwd = NULL;
  grn_prefs.smtp_server = g_strdup("mail");
  grn_prefs.email = g_strdup_printf("%s@%s", g_get_user_name(), get_fqdn(NULL));
  grn_prefs.replyto = g_strdup("");
  grn_prefs.org = g_strdup(_("@un_n (change this!)"));
  grn_prefs.ua = g_strdup(_("@longpid v.@ver [@uname]"));
  grn_prefs.re_quoting = g_strdup("^ *[A-Za-z_@.]*[>|]+");
  grn_prefs.paths[0] = g_strdup("~/.newsrc");
  grn_prefs.paths[1] = g_strdup("~/.oldnewsrc");
  fill_fonts();
  for (i=0; i<NPIXMAPS; i++)
  {
    grn_prefs.pm_names[i] = NULL;
    grn_prefs.pixmaps[i] = NULL;
  }
  grn_prefs.fontpal = NULL;  grn_prefs.colorpal = NULL;
}

void grn_prefs_done(void)
{
  grn_prefs_free();
}


void grn_find_load(grn_find *gf)
{
  g_return_if_fail(gf != NULL);

  str_free(&(gf->data));
  gf->data = gnome_config_get_string(GRN_SRCH_DATA);
  str_free(&(gf->hdr));
  gf->hdr = gnome_config_get_string(GRN_SRCH_HDR_DATA"=X-Comment-To");
  gf->search_from = gnome_config_get_bool(GRN_SRCH_FROM"=false");
  gf->search_subj = gnome_config_get_bool(GRN_SRCH_SUBJ"=true");
  gf->search_body = gnome_config_get_bool(GRN_SRCH_BODY"=false");
  gf->search_hdr = gnome_config_get_bool(GRN_SRCH_HDR"=false");
  gf->is_regex = gnome_config_get_bool(GRN_SRCH_REGEX"=false");
  gf->sensitive = gnome_config_get_bool(GRN_SRCH_SENS"=false");
  gf->unread_only = gnome_config_get_bool(GRN_SRCH_UNREAD"=true");
}

void grn_prefs_load(void)
{
  gint flag, i, n;
  GdkColor def_fg_colors[NCOLORS], def_bg_colors[NCOLORS];
  gchar *buf;

  grn_prefs.srv_autoconnect = gnome_config_get_bool(GRN_PREFS_AUTOCONNECT"=false");
  grn_prefs.nntp_auth = gnome_config_get_bool(GRN_PREFS_NNTP_AUTH"=false");
  grn_prefs.groups_desc = gnome_config_get_bool(GRN_PREFS_GROUPS_DESC"=true");
  grn_prefs.scripts_autoload = gnome_config_get_bool(GRN_PREFS_SCR_AUTOLOAD"=true");
  grn_prefs.tl_show_read = gnome_config_get_bool(GRN_PREFS_TL_SHOW_READ"=false");
  grn_prefs.tl_ac = gnome_config_get_bool(GRN_PREFS_TL_AC"=true");
  grn_prefs.mw_prop = gnome_config_get_bool(GRN_PREFS_MW_PROP"=true");
  grn_prefs.tooltips = gnome_config_get_bool(GRN_PREFS_TOOLTIPS"=true");
  grn_prefs.smooth_scrolling = gnome_config_get_bool(GRN_PREFS_SSCROLL"=false");
  grn_prefs.srt_ng_subscr = gnome_config_get_int(GRN_PREFS_SRT_NG_SUBSCR"=0");
  grn_prefs.srt_ng_col = gnome_config_get_int(GRN_PREFS_SRT_NG_COL"=3");
  grn_prefs.thrd_tl_type = gnome_config_get_int(GRN_PREFS_THRD_TL_TYPE"=0");
  grn_prefs.srt_tl_col = gnome_config_get_int(GRN_PREFS_SRT_TL_COL"=6");

  grn_prefs.toolbar_style = gnome_config_get_int_with_default(GRN_PREFS_TBSTYLE, &flag);
  if (flag)  grn_prefs.toolbar_style = -1;

  str_free(&(grn_prefs.nntp_server));
  grn_prefs.nntp_server = gnome_config_get_string(GRN_PREFS_NNTPSERV"=news");
  
  str_free(&(grn_prefs.nntp_login));
  grn_prefs.nntp_login = gnome_config_get_string(GRN_PREFS_NNTP_LOGIN"=login");

  str_free(&(grn_prefs.nntp_passwd));
  grn_prefs.nntp_passwd = gnome_config_get_string(GRN_PREFS_NNTP_PWD"=password");

  str_free(&(grn_prefs.smtp_server));
  grn_prefs.smtp_server = gnome_config_get_string(GRN_PREFS_SMTPSERV"=mail");

  buf = gnome_config_get_string_with_default(GRN_PREFS_NAME, &flag);
  if (! flag)
  {
    str_free(&(grn_prefs.name));
    grn_prefs.name = buf;
  }
  else  str_free(&buf);

  buf = gnome_config_get_string_with_default(GRN_PREFS_EMAIL, &flag);
  if (! flag)
  {
    str_free(&(grn_prefs.email));
    grn_prefs.email = buf;
  }
  else  str_free(&buf);

  str_free(&(grn_prefs.replyto));
  grn_prefs.replyto = gnome_config_get_string(GRN_PREFS_REPLYTO"=");

  buf = gnome_config_get_string_with_default(GRN_PREFS_ORG, &flag);
  if (! flag)
  {
    str_free(&(grn_prefs.org));
    grn_prefs.org = buf;
  }
  else  str_free(&buf);

  buf = gnome_config_get_string_with_default(GRN_PREFS_UA, &flag);
  if (! flag)
  {
    str_free(&(grn_prefs.ua));  grn_prefs.ua = buf;
  }
  else  str_free(&buf);

  str_free(&(grn_prefs.re_quoting));
  grn_prefs.re_quoting = gnome_config_get_string(GRN_PREFS_QUOTING"=^ *[A-Za-z_@.]*[>|]+.+");

  for (i=0; i<NPATHS; i++)
  {
    gchar *prefs_id = g_strdup_printf("%s%d", GRN_PREFS_PATHS, i);
    buf = gnome_config_get_string_with_default(prefs_id, &flag);
    str_free(&prefs_id);
    if (! flag)
    {
      str_free(&(grn_prefs.paths[i]));  grn_prefs.paths[i] = buf;
    }
    else  str_free(&buf);
  }

  str_free(&(grn_prefs.fonts[0]));
  grn_prefs.fonts[0] = gnome_config_get_string(GRN_PREFS_FNT_BODY_P"="FNT_BODY_P);
  str_free(&(grn_prefs.fonts[1]));
  grn_prefs.fonts[1] = gnome_config_get_string(GRN_PREFS_FNT_BODY_M"="FNT_BODY_M);
  str_free(&(grn_prefs.fonts[2]));
  grn_prefs.fonts[2] = gnome_config_get_string(GRN_PREFS_FNT_HDRS"="FNT_BODY_P);
  str_free(&(grn_prefs.fonts[3]));
  grn_prefs.fonts[3] = gnome_config_get_string(GRN_PREFS_FNT_THRDLIST"="FNT_BODY_P);
  str_free(&(grn_prefs.fonts[4]));
  grn_prefs.fonts[4] = gnome_config_get_string(GRN_PREFS_FNT_NGLIST"="FNT_BODY_P);
  
  fill_def_fg_colors(def_fg_colors);  fill_def_bg_colors(def_bg_colors);
  for (i=0; i<NCOLORS; i++)
  {
    gchar prefs_id[512];
    g_snprintf(prefs_id, 512, "%s_r=%d", prefs_fg_colors_ids[i], def_fg_colors[i].red);
    grn_prefs.fg_colors[i].red = gnome_config_get_int(prefs_id);
    g_snprintf(prefs_id, 512, "%s_g=%d", prefs_fg_colors_ids[i], def_fg_colors[i].green);
    grn_prefs.fg_colors[i].green = gnome_config_get_int(prefs_id);
    g_snprintf(prefs_id, 512, "%s_b=%d", prefs_fg_colors_ids[i], def_fg_colors[i].blue);
    grn_prefs.fg_colors[i].blue = gnome_config_get_int(prefs_id);

    g_snprintf(prefs_id, 512, "%s_r=%d", prefs_bg_colors_ids[i], def_bg_colors[i].red);
    grn_prefs.bg_colors[i].red = gnome_config_get_int(prefs_id);
    g_snprintf(prefs_id, 512, "%s_g=%d", prefs_bg_colors_ids[i], def_bg_colors[i].green);
    grn_prefs.bg_colors[i].green = gnome_config_get_int(prefs_id);
    g_snprintf(prefs_id, 512, "%s_b=%d", prefs_bg_colors_ids[i], def_bg_colors[i].blue);
    grn_prefs.bg_colors[i].blue = gnome_config_get_int(prefs_id);
  }
  for (i=0; i<NPIXMAPS; i++)
  {
    gchar *buf = g_strdup_printf("%s%d", GRN_PREFS_PIXMAPS, i);
    str_free(&grn_prefs.pm_names[i]);
    grn_prefs.pm_names[i] = gnome_config_get_string(buf);
    grn_prefs.pixmaps[i] = grn_load_pixmap(grn_prefs.pm_names[i]);
    str_free(&buf);
  }
  for (i=0; i<NCFMS; i++)
  {
    gchar *buf = g_strdup_printf("%s%d=true", GRN_PREFS_CFM, i);
    grn_prefs.cfms[i] = gnome_config_get_bool(buf);
    str_free(&buf);
  }

  n = gnome_config_get_int(GRN_PREFS_FONT_N_PAL"=0");
  for (i=0; i<n; i++)
  {
    gchar *buf = g_strdup_printf("%s%d=", GRN_PREFS_FONT_PAL, i);
    grn_prefs.fontpal = g_slist_prepend(grn_prefs.fontpal, gnome_config_get_string(buf));
  }
  grn_prefs.fontpal = g_slist_reverse(grn_prefs.fontpal);

  n = gnome_config_get_int(GRN_PREFS_COLOR_N_PAL"=0");
  for (i=0; i<n; i++)
  {
    GdkColor *clr = g_new(GdkColor, 1);
    gchar *buf;
    buf = g_strdup_printf("%s%d_r=", GRN_PREFS_COLOR_PAL, i);
    clr->red = gnome_config_get_int(buf);  str_free(&buf);
    buf = g_strdup_printf("%s%d_g=", GRN_PREFS_COLOR_PAL, i);
    clr->green = gnome_config_get_int(buf);  str_free(&buf);
    buf = g_strdup_printf("%s%d_b=", GRN_PREFS_COLOR_PAL, i);
    clr->blue = gnome_config_get_int(buf);  str_free(&buf);
    grn_prefs.colorpal = g_slist_prepend(grn_prefs.colorpal, clr);
  }
  grn_prefs.colorpal = g_slist_reverse(grn_prefs.colorpal);

  for (i=0; i<NWINSTATES; i++)
  {
    gchar *buf;
    gint n;
    buf = g_strdup_printf("%s%d_w", GRN_PREFS_WINSTATES, i);
    n = gnome_config_get_int_with_default(buf, &flag);
    if (! flag)  grn_prefs.ws[i].width = n;
    buf = g_strdup_printf("%s%d_h", GRN_PREFS_WINSTATES, i);
    n = gnome_config_get_int_with_default(buf, &flag);
    if (! flag)  grn_prefs.ws[i].height = n;
    buf = g_strdup_printf("%s%d_x", GRN_PREFS_WINSTATES, i);
    n = gnome_config_get_int_with_default(buf, &flag);
    if (! flag)  grn_prefs.ws[i].x = n;
    buf = g_strdup_printf("%s%d_y", GRN_PREFS_WINSTATES, i);
    n = gnome_config_get_int_with_default(buf, &flag);
    if (! flag)  grn_prefs.ws[i].y = n;
  }
}

void grn_find_save(grn_find *gf)
{
  g_return_if_fail(gf != NULL);

  gnome_config_set_string(GRN_SRCH_DATA, gf->data);
  gnome_config_set_string(GRN_SRCH_HDR_DATA, gf->hdr);
  gnome_config_set_bool(GRN_SRCH_FROM, gf->search_from);
  gnome_config_set_bool(GRN_SRCH_SUBJ, gf->search_subj);
  gnome_config_set_bool(GRN_SRCH_BODY, gf->search_body);
  gnome_config_set_bool(GRN_SRCH_HDR, gf->search_hdr);
  gnome_config_set_bool(GRN_SRCH_REGEX, gf->is_regex);
  gnome_config_set_bool(GRN_SRCH_SENS, gf->sensitive);
  gnome_config_set_bool(GRN_SRCH_UNREAD, gf->unread_only);
  gnome_config_sync();
}


void grn_prefs_save(void)
{
  gint i, n;
  GSList *l;

  gnome_config_set_bool(GRN_PREFS_AUTOCONNECT, grn_prefs.srv_autoconnect);
  gnome_config_set_bool(GRN_PREFS_NNTP_AUTH, grn_prefs.nntp_auth);
  gnome_config_set_bool(GRN_PREFS_GROUPS_DESC, grn_prefs.groups_desc);
  gnome_config_set_bool(GRN_PREFS_SCR_AUTOLOAD, grn_prefs.scripts_autoload);
  gnome_config_set_bool(GRN_PREFS_TL_SHOW_READ, grn_prefs.tl_show_read);
  gnome_config_set_bool(GRN_PREFS_TL_AC, grn_prefs.tl_ac);
  gnome_config_set_bool(GRN_PREFS_MW_PROP, grn_prefs.mw_prop);
  gnome_config_set_bool(GRN_PREFS_TOOLTIPS, grn_prefs.tooltips);
  gnome_config_set_bool(GRN_PREFS_SSCROLL, grn_prefs.smooth_scrolling);
  gnome_config_set_int(GRN_PREFS_TBSTYLE, grn_prefs.toolbar_style);
  gnome_config_set_int(GRN_PREFS_SRT_NG_SUBSCR, grn_prefs.srt_ng_subscr);
  gnome_config_set_int(GRN_PREFS_SRT_NG_COL, grn_prefs.srt_ng_col);
  gnome_config_set_int(GRN_PREFS_THRD_TL_TYPE, grn_prefs.thrd_tl_type);
  gnome_config_set_int(GRN_PREFS_SRT_TL_COL, grn_prefs.srt_tl_col);
  gnome_config_set_string(GRN_PREFS_NNTPSERV, grn_prefs.nntp_server);
  gnome_config_set_string(GRN_PREFS_NNTP_LOGIN, grn_prefs.nntp_login);
  gnome_config_set_string(GRN_PREFS_NNTP_PWD, grn_prefs.nntp_passwd);
  gnome_config_set_string(GRN_PREFS_SMTPSERV, grn_prefs.smtp_server);
  gnome_config_set_string(GRN_PREFS_NAME, grn_prefs.name);
  gnome_config_set_string(GRN_PREFS_EMAIL, grn_prefs.email);
  gnome_config_set_string(GRN_PREFS_REPLYTO, grn_prefs.replyto);
  gnome_config_set_string(GRN_PREFS_ORG, grn_prefs.org);
  gnome_config_set_string(GRN_PREFS_UA, grn_prefs.ua);
  gnome_config_set_string(GRN_PREFS_QUOTING, grn_prefs.re_quoting);
  gnome_config_set_string(GRN_PREFS_FNT_BODY_P, grn_prefs.fonts[0]);
  gnome_config_set_string(GRN_PREFS_FNT_BODY_M, grn_prefs.fonts[1]);
  gnome_config_set_string(GRN_PREFS_FNT_HDRS, grn_prefs.fonts[2]);
  gnome_config_set_string(GRN_PREFS_FNT_THRDLIST, grn_prefs.fonts[3]);
  gnome_config_set_string(GRN_PREFS_FNT_NGLIST, grn_prefs.fonts[4]);
  for (i=0; i<NPATHS; i++)
  {
    gchar *prefs_id = g_strdup_printf("%s%d", GRN_PREFS_PATHS, i);
    gnome_config_set_string(prefs_id, grn_prefs.paths[i]);
    str_free(&prefs_id);
  }
  for (i=0; i<NCOLORS; i++)
  {
    gchar prefs_id[512];
    snprintf(prefs_id, 512, "%s_r", prefs_fg_colors_ids[i]);
    gnome_config_set_int(prefs_id, grn_prefs.fg_colors[i].red);
    snprintf(prefs_id, 512, "%s_g", prefs_fg_colors_ids[i]);
    gnome_config_set_int(prefs_id, grn_prefs.fg_colors[i].green);
    snprintf(prefs_id, 512, "%s_b", prefs_fg_colors_ids[i]);
    gnome_config_set_int(prefs_id, grn_prefs.fg_colors[i].blue);

    snprintf(prefs_id, 512, "%s_r", prefs_bg_colors_ids[i]);
    gnome_config_set_int(prefs_id, grn_prefs.bg_colors[i].red);
    snprintf(prefs_id, 512, "%s_g", prefs_bg_colors_ids[i]);
    gnome_config_set_int(prefs_id, grn_prefs.bg_colors[i].green);
    snprintf(prefs_id, 512, "%s_b", prefs_bg_colors_ids[i]);
    gnome_config_set_int(prefs_id, grn_prefs.bg_colors[i].blue);
  }
  for (i=0; i<NPIXMAPS; i++)
  {
    gchar *buf = g_strdup_printf("%s%d", GRN_PREFS_PIXMAPS, i);
    gnome_config_set_string(buf, grn_prefs.pm_names[i]);
    str_free(&buf);
  }
  for (i=0; i<NCFMS; i++)
  {
    gchar *buf = g_strdup_printf("%s%d", GRN_PREFS_CFM, i);
    gnome_config_set_bool(buf, grn_prefs.cfms[i]);
    str_free(&buf);
  }
  
  n = g_slist_length(grn_prefs.fontpal);
  if (n > 0)
  {
    gnome_config_set_int(GRN_PREFS_FONT_N_PAL, n);
    l = grn_prefs.fontpal;
    i = 0;
    while (l && (i < n))
    {
      gchar *buf = g_strdup_printf("%s%d", GRN_PREFS_FONT_PAL, i);
      gnome_config_set_string(buf, l->data);  str_free(&buf);
      l = l->next;  i++;
    }
  }
  n = g_slist_length(grn_prefs.colorpal);
  if (n > 0)
  {
    gnome_config_set_int(GRN_PREFS_COLOR_N_PAL, n);
    l = grn_prefs.colorpal;
    i = 0;
    while (l && (i < n))
    {
      gchar *buf;
      GdkColor *clr = (GdkColor *) l->data;
      buf = g_strdup_printf("%s%d_r", GRN_PREFS_COLOR_PAL, i);
      gnome_config_set_int(buf, clr->red);  str_free(&buf);
      buf = g_strdup_printf("%s%d_g", GRN_PREFS_COLOR_PAL, i);
      gnome_config_set_int(buf, clr->green);  str_free(&buf);
      buf = g_strdup_printf("%s%d_b", GRN_PREFS_COLOR_PAL, i);
      gnome_config_set_int(buf, clr->blue);  str_free(&buf);
      l = l->next;  i++;
    }
  }
  for (i=0; i<NWINSTATES; i++)
  {
    gchar *buf;
    buf = g_strdup_printf("%s%d_w", GRN_PREFS_WINSTATES, i);
    gnome_config_set_int(buf, grn_prefs.ws[i].width);  str_free(&buf);
    buf = g_strdup_printf("%s%d_h", GRN_PREFS_WINSTATES, i);
    gnome_config_set_int(buf, grn_prefs.ws[i].height);  str_free(&buf);
    buf = g_strdup_printf("%s%d_x", GRN_PREFS_WINSTATES, i);
    gnome_config_set_int(buf, grn_prefs.ws[i].x);  str_free(&buf);
    buf = g_strdup_printf("%s%d_y", GRN_PREFS_WINSTATES, i);
    gnome_config_set_int(buf, grn_prefs.ws[i].y);  str_free(&buf);
  }
  gnome_config_sync();
}

void grn_prefs_realize(void)
{
  GnomeDockItem *item;
  GtkWidget *toolbar;

  item = gnome_app_get_dock_item_by_name(GNOME_APP(GRN->window), GNOME_APP_TOOLBAR_NAME);
  if (item)
  {
    toolbar = gnome_dock_item_get_child(item);
    if (toolbar)  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), grn_get_toolbar_style());
  }
  if (GRN->tooltips)
  {
    if (grn_prefs.tooltips)  gtk_tooltips_enable(GRN->tooltips);
    else  gtk_tooltips_disable(GRN->tooltips);
  }
  if (GRN->msgwin)
  {
    GtkWidget *text = gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_TXT_BODY);
    GtkWidget *w;
    gchar *fnt_name;
    
    if (grn_prefs.mw_prop)  fnt_name = grn_prefs.fonts[0];
    else  fnt_name = grn_prefs.fonts[1];
    if (text)  grn_widget_set_style(text, fnt_name, NULL,
                 &(grn_prefs.bg_colors[0]), grn_prefs.pixmaps[0]);
    w = gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_MW_FROM);
    if (w)  grn_widget_set_style(w, grn_prefs.fonts[2], NULL, NULL, NULL);
    w = gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_MW_TO);
    if (w)  grn_widget_set_style(w, grn_prefs.fonts[2], NULL, NULL, NULL);
    w = gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_MW_DATE);
    if (w)  grn_widget_set_style(w, grn_prefs.fonts[2], NULL, NULL, NULL);
    w = gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_MW_SUBJ);
    if (w)  grn_widget_set_style(w, grn_prefs.fonts[2], NULL, NULL, NULL);
  }
  if (GRN->threadlist)
  {
    GtkWidget *ctree = gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_CT_MSGS);
    if (ctree)  grn_widget_set_style(ctree, grn_prefs.fonts[3], NULL, NULL, NULL);
  }
  if (GRN->grouplist)
  {
    GtkWidget *clist = gtk_object_get_data(GTK_OBJECT(GRN->grouplist), KEY_CL_GROUPS);
    if (clist)  grn_widget_set_style(clist, grn_prefs.fonts[4], NULL, NULL, NULL);
  }
}


gint grn_get_toolbar_style(void)
{
  if (grn_prefs.toolbar_style > (-1))  return grn_prefs.toolbar_style;
  if (gnome_preferences_get_toolbar_labels())  return GTK_TOOLBAR_BOTH;
  else  return GTK_TOOLBAR_ICONS;
}
