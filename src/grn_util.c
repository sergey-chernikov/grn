/*
 * grn_util.c - Implementation of some project-wide functions
 *
 * $Id: grn_util.c,v 1.43 2000/07/19 06:26:21 sc Exp $
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

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <locale.h>
#include "grn_consts.h"
#include "grn_types.h"
#include "grn_vars.h"
#include "grn_util.h"
#include "grn_config.h"


gint grn_compare_strings(gchar *a, gchar *b)
{
  return strcmp(a, b);
}

void grn_appbar_push(GnomeAppBar *ab, gboolean update, gchar *fmt, ...)
{
  gchar *s_tmp;
  va_list args;
  
  if (! ab)  return;
  va_start(args, fmt);
  s_tmp = g_strdup_vprintf(fmt, args);
  va_end(args);
  grn_lock();
  gnome_appbar_push(ab, s_tmp);
  grn_unlock();
  if (update)  grn_main_iter();
  str_free(&s_tmp);
}

void grn_appbar_pop(GnomeAppBar *ab)
{
  if (! ab)  return;
  grn_lock();
  gnome_appbar_pop(ab);
  grn_unlock();
}

void grn_appbar_set_status(GnomeAppBar *ab, gchar *fmt, ...)
{
  gchar *s_tmp;
  va_list args;
  
  if (! ab)  return;
  va_start(args, fmt);
  s_tmp = g_strdup_vprintf(fmt, args);
  va_end(args);
  grn_lock();
  gnome_appbar_set_status(ab, s_tmp);
  grn_unlock();
  str_free(&s_tmp);
}

void grn_set_title(GtkWindow *w, gchar *fmt, ...)
{
  gchar *s_tmp ;
  va_list args;
  g_return_if_fail(w != NULL);
  
  va_start(args, fmt);
  s_tmp = g_strdup_vprintf(fmt, args);
  va_end(args);
  grn_lock();
  gtk_window_set_title(w, s_tmp);
  grn_unlock();
  str_free(&s_tmp);
}

void grn_set_contents(GtkWidget *contents)
{
  GtkWidget *old_contents = gtk_object_get_data(GTK_OBJECT(GRN->container), KEY_CONTENTS);

  if (old_contents)
  {
    grn_lock();
    gtk_widget_hide(old_contents);
    grn_unlock();
  }
  if (contents)
  {
    GtkWidget *w_focused;
    gchar *title;
    
    grn_lock();
    w_focused = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(contents), KEY_FOCUS));
    title = (gchar *) gtk_object_get_data(GTK_OBJECT(contents), KEY_TITLE);

    gtk_widget_show(contents);
    gtk_container_add(GRN->container, contents);
    gtk_object_set_data(GTK_OBJECT(GRN->window), KEY_WND, contents);
    grn_unlock();
    if (title)  grn_set_title(GTK_WINDOW(GRN->window), title);
    if (w_focused)
    {
      grn_lock();
      gtk_widget_grab_focus(w_focused);
      gtk_object_remove_data(GTK_OBJECT(GRN->window), KEY_FOCUS);
      gtk_object_set_data(GTK_OBJECT(GRN->window), KEY_FOCUS, w_focused);
      grn_unlock();
    }
  }
  grn_lock();
  gtk_object_set_data(GTK_OBJECT(GRN->container), KEY_CONTENTS, contents);
  grn_unlock();
}

void grn_widget_set_style(GtkWidget *w, const gchar *font_name,
			  GdkColor *fg, GdkColor *bg, GdkPixmap *bg_pm)
{
  GtkStyle *style = NULL;
  GdkColormap *cmap = NULL;
  g_return_if_fail(w != NULL);
  
  grn_lock();
  style = gtk_style_copy(gtk_widget_get_style(w));

  if (str_check(font_name))
  {
    GdkFont *font;
    gchar *fn = g_strdup(font_name);
    font = gdk_fontset_load(fn);
    if (font)
    {
      if (style->font)  gdk_font_unref(style->font);
      style->font = font;
    }
    else  g_warning(_("Error loading font '%s'"), font_name);
    str_free(&fn);
  }
  if (fg || bg)  cmap = gdk_colormap_get_system();
  if (fg)
  {
    if (gdk_color_alloc(cmap, fg))
    {
      if (GTK_IS_TEXT(w))  style->text[0] = *fg;
      else  style->fg[0] = *fg;
    }
    else  g_warning(_("Can't allocate foreground color"));
  }
  if (bg)
  {
    if (gdk_color_alloc(cmap, bg))
    {
      if (GTK_IS_TEXT(w))  style->base[0] = *bg;
      else  style->bg[0] = *bg;
    }
    else  g_warning(_("Can't allocate background color"));
  }
  if (bg_pm)  style->bg_pixmap[0] = bg_pm;

  gtk_widget_set_style(w, style);
  grn_unlock();
}

GdkPixmap *grn_load_pixmap(const gchar *pm_name)
{
  GnomePixmap *gpm;
  if (! str_check(pm_name))  return NULL;

  g_print(_("Loading image from '%s'..."), pm_name);
  grn_lock();
  gpm = GNOME_PIXMAP(gnome_pixmap_new_from_file(pm_name));
  if (! gpm)
  {
    g_print(_("Error!\n"));
    return NULL;
  }
  if (gpm->mask)  gdk_bitmap_unref(gpm->mask);
  grn_unlock();
  if (! gpm->pixmap)  g_print(_("Failed!\n"));
  else  g_print(_("Ok.\n"));
  return gpm->pixmap;
}

void grn_error(const gchar *area, gchar *message)
{
  gchar *s;
  if (! str_check(message))  return;
  
  if (area)  s = g_strdup_printf("%s:\n%s", area, message);
  else  s = g_strdup(message);
  grn_lock();
  gnome_app_error(GNOME_APP(GRN->window), s);
  grn_unlock();
  str_free(&s);
}

void grn_info(gchar *fmt, ...)
{
  va_list args;
  gchar *s;
  
  va_start(args, fmt);
  s = g_strdup_vprintf(fmt, args);
  va_end(args);

  grn_lock();
  gnome_app_message(GNOME_APP(GRN->window), s);
  grn_unlock();
  str_free(&s);
}


void grn_main_iter(void)
{
  grn_lock();
  while (gtk_events_pending())  gtk_main_iteration();
  grn_unlock();
}

GtkWidget *grn_get_widget(GtkObject *obj, gchar *name)
{
  GtkWidget *found_widget;
  
  grn_lock();
  found_widget = (GtkWidget *) gtk_object_get_data(GTK_OBJECT(obj), name);
  grn_unlock();
#ifdef GRN_DEBUG
  if (! found_widget)
    printf("grn_get_widget@grn_util.c: Widget with name %s not found\n", name);
#endif
  return found_widget;
}

// efficient_append() from GGAD
void grn_g_list_append(GList **list, GList **list_end, gpointer data)
{
  g_return_if_fail(list != NULL);
  g_return_if_fail(list_end != NULL);
  
  if (*list == NULL)
  {
    g_assert(*list_end == NULL);
    *list = g_list_append(*list, data);
    *list_end = *list;
  }
  else  *list_end = g_list_append(*list_end, data)->next;
}

gchar *get_charset(void)
{
  gchar *s, *ret=NULL;
  
  s = g_strdup(setlocale(LC_CTYPE, NULL));
  if (s)
  {
    gchar **s_arr = g_strsplit(s, ".", 2);
    str_free(&s);
    if (str_check(s_arr[1]))  ret = g_strdup(s_arr[1]);
    g_strfreev(s_arr);
  }
  return ret;
}

static int locked = 0;
void grn_lock(void)
{
  if (pthread_self() != GRN->tid_queue)  return;
  if (locked > 0) ;// g_warning("Nested locking: %d", locked);
  else  gdk_threads_enter();
  locked++;
}
void grn_unlock(void)
{
  if (locked <= 0)  return;
  gdk_threads_leave();
  locked--;
  if (locked < 0)  locked = 0;
}


gchar *str_subst(const gchar *src, gchar *var, gchar *value)
{
  gchar *s;
  
  g_return_val_if_fail(src != NULL, NULL);
  g_return_val_if_fail(var != NULL, NULL);
  s = g_strdup(src);
  while (strstr(s, var))
  {
    gchar **arr = g_strsplit(s, var, 1);
    str_free(&s);
    s = g_strdup_printf("%s%s%s", (arr[0]!=NULL)?arr[0]:"", value, (arr[1]!=NULL)?arr[1]:"");
    str_free(&arr[0]);  str_free(&arr[1]);
  }
  return s;
}

gboolean str_check(const gchar *s)
{
  if (! s)  return FALSE;
  if (strlen(s) <= 0)  return FALSE;
  return TRUE;
}

gchar *str_copy(const gchar *s)
{
  if (! s)  return NULL;
  if (strlen(s) <= 0)  return NULL;
  return  g_strdup(s);
}

void str_add(gchar **str, gchar *text)
{
  gchar *ret;
  
  if (*str)
  {
    ret = g_strconcat(*str, text, NULL);
    str_free(str);
  }
  else  ret = g_strdup(text);
  *str = ret;
}

void str_free(gchar **str)
{
  if (! str)  return;
  if (! *str)  return;
  g_free(*str);  *str = NULL;
}

void str_destroy(gchar *s)
{  str_free(&s);  }

void grn_subst_vars(gchar **src)
{
  gchar *ret, *tmp;
  gchar *ldate;
  struct utsname un;
  if (! src)  return;
  if (! str_check(*src))  return;
  
  ldate = date_get_local(time(NULL));
  ret = str_subst(*src, "@ctime", ldate);
  g_free(ldate);
  tmp = str_subst(ret, "@cname", grn_prefs.name);	g_free(ret);
  ret = str_subst(tmp, "@email", grn_prefs.email);	g_free(tmp);
  tmp = str_subst(ret, "@longpid", _(GRN_TITLE));	g_free(ret);
  ret = str_subst(tmp, "@pid", "Grn");			g_free(tmp);
  tmp = str_subst(ret, "@version", VERSION);		g_free(ret);
  ret = str_subst(tmp, "@ver", VERSION);		g_free(tmp);
  uname(&un);  ldate = g_strdup_printf("%s %s (%s)", un.sysname, un.release, un.machine);
  tmp = str_subst(ret, "@uname", ldate);		g_free(ret);
  g_free(ldate);
  ret = str_subst(tmp, "@un_s", un.sysname);		g_free(tmp);
  tmp = str_subst(ret, "@un_r", un.release);		g_free(ret);
  ret = str_subst(tmp, "@un_m", un.machine);		g_free(tmp);
  tmp = str_subst(ret, "@un_n", un.nodename);		g_free(ret);
  ret = str_subst(tmp, "@replyto", grn_prefs.replyto);	g_free(tmp);
  g_free(*src);  *src = ret;
}

gchar *grn_subst_tilde(const gchar *src)
{
  gchar *s, *ret;
  g_return_val_if_fail(src != NULL, NULL);

  if (src[0] != '~')  return g_strdup(src);
  s = g_strconcat(g_get_home_dir(), "/", NULL);
  ret = str_subst(src, "~/", s);
  str_free(&s);
  return ret;
}


t_msgheader *parse_hdr(gchar *src)
{
  gchar **sarr;
  gchar *tmp;
  t_msgheader *ret = NULL;
  g_return_val_if_fail(src != NULL, ret);
  
  sarr = g_strsplit(src, ": ", 1);
  if (sarr[0] && sarr[1])
  {
    ret = g_new(t_msgheader, 1);
    ret->name = g_strdup(sarr[0]);
    tmp = sarr[1];
    while (*tmp)  tmp++;
    ret->value = (gchar *) g_malloc(tmp - sarr[1] + 1);
    strncpy(ret->value, sarr[1], tmp - sarr[1]);
    ret->value[tmp - sarr[1]] = '\0';
  }
  g_strfreev(sarr);
  return ret;
}

gchar *get_hdr_val(gchar *src)
{
  gchar *ret = NULL;
  t_msgheader *mh;
  g_return_val_if_fail(src != NULL, ret);
  
  if ((mh = parse_hdr(src)) != NULL)
  {
    ret = g_strdup(mh->value);
    str_free(&(mh->name));  str_free(&(mh->value));
    g_free(mh);
  }
  return ret;
}


#define THE_YEAR_0	1970L
t_tz_table tz_table[] =
{
  {"EDT", -500},
  {"PST", -800},
  {"CST", -600},
  {"MST", -700},
  {"MET", 100},
  {"MEZ", 100},
  {"MSK", 300},
  {"MSD", 400},
  {"CET", 100},
  {"HKT", 800},
  {"JST", 900},
  {"CAST", 930},
  {"EAST", 1000},
  {"NZST", 1200},
  {"EET", 200},
  {"", 0}
};

const gshort dpm[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static gchar *skip_whitespace(gchar *src)
{
  gchar ch;
  
  if (! src)  return NULL;
  while (((ch = *src) == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r'))
    src++;
  return src;
}

static gint parse_timezone(gchar *tz)
{
  gchar ch;
  t_tz_table *table;
  
  table = tz_table;
  while ((ch = table->name[0]) != 0)
  {
    if ((*tz == ch) && (! strncmp(tz, table->name, sizeof(table->name))))
      return table->tz;
    table++;
  }
  return 0;
}

time_t parse_date(gchar *date)
{
  gchar ch;
  glong day=0, month=0, year=0, minutes=0, hours=0, tz=0;
  time_t seconds = 0;
  gchar smonth[4];
  gint sign, i;
  
  date = skip_whitespace(date);
  if ((! date) || (*date == 0))  return 0;

  while(isalpha((gint)*date))  date++;
  if (*date == ',')  date++;
  date = skip_whitespace(date);
  
  while (ch = *date, isdigit((gint) ch))
  {
    day = 10 * day + (glong) (ch - '0');
    date++;
  }
  if ((day <= 0) || (day > 31))  return 0;
  day--;
  
  date = skip_whitespace(date);
  strncpy(smonth, date, 3);  smonth[3] = '\0';
  if (! g_strncasecmp(smonth, "Jan", 3))  month = 1;
  else if (! g_strncasecmp(smonth, "Feb", 3))  month = 2;
  else if (! g_strncasecmp(smonth, "Mar", 3))  month = 3;
  else if (! g_strncasecmp(smonth, "Apr", 3))  month = 4;
  else if (! g_strncasecmp(smonth, "May", 3))  month = 5;
  else if (! g_strncasecmp(smonth, "Jun", 3))  month = 6;
  else if (! g_strncasecmp(smonth, "Jul", 3))  month = 7;
  else if (! g_strncasecmp(smonth, "Aug", 3))  month = 8;
  else if (! g_strncasecmp(smonth, "Sep", 3))  month = 9;
  else if (! g_strncasecmp(smonth, "Oct", 3))  month = 10;
  else if (! g_strncasecmp(smonth, "Nov", 3))  month = 11;
  else if (! g_strncasecmp(smonth, "Dec", 3))  month = 12;
  if (month == 0)  return 0;
  month--;
  while (isalpha((gint) *date))  date++;
  
  date = skip_whitespace(date);
  if (*date == '9')  year = 19;
  else if (*date == '0')  year = 20;
  while (ch = *date, isdigit((gint) ch))
  {
    year = year * 10 + (glong) (ch - '0');
    date++;
  }
  date = skip_whitespace(date);
  
  while (ch = *date, isdigit((gint) ch))
  {
    hours = hours * 10 + (glong) (ch - '0');
    date++;
  }
  if ((ch != ':') || (hours >= 24))  return 0;
  date++;
  
  while (ch = *date, isdigit((gint) ch))
  {
    minutes = minutes * 10 + (glong) (ch - '0');
    date++;
  }
  if (minutes >= 60)  return 0;
  
  if (ch == ':')
  {
    date++;
    while (ch = *date, isdigit((gint) ch))
    {
      seconds = seconds * 10 + (glong) (ch - '0');
      date++;
    }
    if (seconds >= 60)  return 0;
  }
  date = skip_whitespace(date);
  
  sign = 1;
  if (*date == '+')  date++;
  else if (*date == '-')
  {
    sign = -1;  date++;
  }
  while(ch = *date, isdigit((gint) ch))
  {
    tz = tz * 10 + (glong) (ch - '0');
    date++;
  }
  tz = sign * tz;
  
  date = skip_whitespace(date);
  if (isalpha((gint) *date))
  {
    sign = 1;
    tz = parse_timezone(date);
  }
  
  for (i=0; i<month; i++)  day += dpm[i];
  if (month > 0)
  {
    if ((year % 4) == 0)
    {
      day++;
      if ((year % 100) == 0)
      {
        day--;
	if ((year % 400) == 0)  day++;
      }
    }
  }
  year -= THE_YEAR_0;
  day += year * 365.25;
  
  hours -= tz / 100;
  minutes -= tz % 100;
  seconds += (time_t) (60L * (minutes + 60L * (hours + 24L * day)));

  return seconds;
}

gchar *date_get_local(time_t date)
{
  gchar *dow[7] = { _("Sun"), _("Mon"), _("Tue"), _("Wed"), _("Thu"),
                    _("Fri"), _("Sat") };
  gchar *month[12] = { _("Jan"), _("Feb"), _("Mar"), _("Apr"), _("May"),
                       _("Jun"), _("Jul"), _("Aug"), _("Sep"), _("Oct"),
		       _("Nov"), _("Dec")};
  struct tm *lt = localtime(&date);
#ifdef __SVR4
  return g_strdup_printf("%s, %s %02d %04d, %02d:%02d:%02d", dow[lt->tm_wday],
                         month[lt->tm_mon], lt->tm_mday, lt->tm_year+1900,
			 lt->tm_hour, lt->tm_min, lt->tm_sec);
#else
  return g_strdup_printf("%s, %s %02d %04d, %02d:%02d:%02d %s", dow[lt->tm_wday],
                         month[lt->tm_mon], lt->tm_mday, lt->tm_year+1900,
			 lt->tm_hour, lt->tm_min, lt->tm_sec, lt->tm_zone);
#endif
}


gint files_foreach(gchar *dirname, void callback(gchar *file))
{
  DIR *dir;
  struct dirent *ent;
  gchar *buf;
  gint i = 0;
  
  dir = opendir(dirname);
  if (dir)
  {
    while ((ent = readdir(dir)))
    {
      if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, ".."))
      {
        i++;
	buf = g_malloc(strlen(dirname) + strlen(ent->d_name) + 2);
	sprintf(buf, "%s/%s", dirname, ent->d_name);
	callback(buf);
	str_free(&buf);
      }
    }
    closedir(dir);
  }
  return i;
}


/* The following 2 functions were borrowed from pan-0.7.2 */
gchar* get_host_name(void)
{
  gchar *ptr;
  static gchar hostname[256];

  hostname[0] = '\0';

  if ((gethostname(hostname, sizeof(hostname))) == -1)
  {
    if ((ptr = getenv("HOST")) != NULL)
      strncpy(hostname, ptr, MAX_HOSTNAME_LEN);
    else if ((ptr = getenv("HOSTNAME")) != NULL)
      strncpy(hostname, ptr, MAX_HOSTNAME_LEN);
    else  hostname[0] = '\0';
  }
  hostname[MAX_HOSTNAME_LEN] = '\0';
  ptr = strtok (hostname, ".");
  return g_strdup ( ptr );
}

gchar *get_fqdn (gchar *host)
{
  static gchar name[512];
  static gchar h[MAX_HOSTNAME_LEN];
  char line[1025];
  char *cp, *domain;
  static char fqdn[1024];
  struct hostent *hp;
  struct in_addr in;
  FILE *inf;

  *fqdn = '\0';
  domain = NULL;

  name[MAX_HOSTNAME_LEN] = '\0';
  if (! str_check(host))  strncpy(h, get_host_name(), MAX_HOSTNAME_LEN);
  else  strncpy(h, host, MAX_HOSTNAME_LEN);

  if (str_check(h))
  {
    if (strchr(h, '.'))  return h;
    strncpy(name, h, MAX_HOSTNAME_LEN);
  }
  else {
    if (gethostname(name, MAX_HOSTNAME_LEN))  return NULL;
  }

  if ('0' <= *name && *name <= '9')
  {
    in.s_addr = inet_addr(name);
    if ((hp = gethostbyaddr((char *) &in.s_addr, 4, AF_INET)))
      in.s_addr = (*hp->h_addr);
    return (hp && strchr(hp->h_name, '.') ? hp->h_name : (gchar *) inet_ntoa(in));
  }
  if ((hp = gethostbyname(name)) && !strchr(hp->h_name, '.'))
    if ((hp = gethostbyaddr(hp->h_addr, hp->h_length, hp->h_addrtype)))
      in.s_addr = (*hp->h_addr);

  sprintf(fqdn, "%s", hp ? strchr(hp->h_name, '.')
          ? hp->h_name : (gchar *) inet_ntoa(in) : "");
  if (!*fqdn || (fqdn[strlen(fqdn) - 1] <= '9'))
  {
    *fqdn = '\0';
    inf = fopen("/etc/resolv.conf", "r");
    if (inf)
    {
      while (fgets(line, 1024, inf))
      {
	line[1024] = '\0';
	g_strchug(line);  g_strchomp(line);
	if (strncmp(line, "domain ", 7) == 0)
	{
	  domain = line + 7;
	  break;
	}
	if (strncmp(line, "search ", 7) == 0)
	{
	  domain = line + 7;
	  cp = strchr(domain, ' ');
	  if (cp)  *cp = '\0';
	  break;
	}
      }
      if (domain)  sprintf(fqdn, "%s.%s", name, domain);
      fclose(inf);
    }
  }
  return (fqdn);
}

gchar *c2s(GdkColor c)
{
  return g_strdup_printf("#%02X%02X%02X", c.red/256, c.green/256, c.blue/256);
}


void grn_save_ws(gint idx, GtkWidget *w)
{
  gint x, y;
  g_return_if_fail(w != NULL);
  
  grn_prefs.ws[idx].width = w->allocation.width;
  grn_prefs.ws[idx].height = w->allocation.height;
  gdk_window_get_root_origin(w->window, &x, &y);
  grn_prefs.ws[idx].x = x;  grn_prefs.ws[idx].y = y;
}
