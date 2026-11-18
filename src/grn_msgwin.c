/*
 * grn_msgwin.c: message window functions
 *
 * $Id: grn_msgwin.c,v 1.27 2000/06/27 13:05:13 sc Exp $
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

#include <math.h>
#include "grn_consts.h"
#include <gnome.h>
#include "grn_vars.h"
#include "grn_msgwin.h"
#include "grn_util.h"
#include "grn_misc.h"
#include "grn_commands.h"
#include "grn_menu.h"
#include "grn_config.h"
#include "grn_msgview.h"
#include "grn_threadlist.h"
#include "grn_perl.h"
#include "grn_mhdrwin.h"
#include "queue.h"
#include "grn_mime.h"


static gint evt_mw_event(GtkWidget *, GdkEvent *, GtkText *);
static gint evt_mw_keypress(GtkWidget *, GdkEventKey *, gpointer);
static gint evt_mw_btnpress(GtkWidget *, GdkEventButton *, gpointer);
static void evt_mw_hide(GtkWidget *, gpointer);
static void evt_mw_show(GtkWidget *, gpointer);


GtkWidget *msgwin_create()
{
  GtkWidget *vbox=NULL, *table, *text, *label, *entry;
  GtkAdjustment *adj;
  
  grn_lock();
  vbox = gtk_vbox_new(FALSE, 0);
  
  table = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("From:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,0,1, GTK_FILL, GTK_FILL, 3, 1);
  entry = gtk_entry_new();
  gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
  gtk_table_attach(GTK_TABLE(table), entry, 1,3,0,1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 1);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_MW_FROM, entry);
  
  label = gtk_label_new(_("To:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,1,2, GTK_FILL, GTK_FILL, 3, 1);
  entry = gtk_entry_new();
  gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
  gtk_table_attach(GTK_TABLE(table), entry, 1,2,1,2,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 1);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_MW_TO, entry);
  label = gtk_label_new("");
  gtk_table_attach(GTK_TABLE(table), label, 2,3,1,2, GTK_FILL, GTK_FILL, 3, 1);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_MW_DATE, label);

  label = gtk_label_new(_("Subject:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,2,3, GTK_FILL, GTK_FILL, 3, 1);
  entry = gtk_entry_new();
  gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
  gtk_table_attach(GTK_TABLE(table), entry, 1,3,2,3,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 1);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_MW_SUBJ, entry);
  
  table = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
  
  adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 0, 0, 0));
  text = gtk_text_new(NULL, adj);
  gtk_text_set_line_wrap(GTK_TEXT(text), TRUE);
  gtk_text_set_word_wrap(GTK_TEXT(text), TRUE);


  gtk_table_attach(GTK_TABLE(table), text, 0,1,0,1,
    GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0,0);

  gtk_table_attach(GTK_TABLE(table), gtk_vscrollbar_new(adj), 1,2,0,1,
    GTK_FILL, GTK_FILL, 0,0);
  
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_TXT_BODY, text);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_FOCUS, text);
  gtk_object_set_data(GTK_OBJECT(text), KEY_WND, vbox);
  
  gtk_signal_connect(GTK_OBJECT(text), "key_press_event",
                     GTK_SIGNAL_FUNC(evt_mw_keypress), NULL);
  gtk_signal_connect(GTK_OBJECT(text), "button_press_event",
                     GTK_SIGNAL_FUNC(evt_mw_btnpress), NULL);
  gtk_signal_connect(GTK_OBJECT(text), "event",
                     GTK_SIGNAL_FUNC(evt_mw_event), text);
  gtk_signal_connect(GTK_OBJECT(vbox), "hide",
                     GTK_SIGNAL_FUNC(evt_mw_hide), NULL);
  gtk_signal_connect(GTK_OBJECT(vbox), "show",
                     GTK_SIGNAL_FUNC(evt_mw_show), NULL);
  
  gtk_widget_show_all(vbox);
  grn_unlock();
  return vbox;
}


void msgwin_set_hdr(GtkWidget *mw, gchar *id, gchar *text)
{
  GtkWidget *hw;
  g_return_if_fail(mw != NULL);
  g_return_if_fail(id != NULL);
  
  grn_lock();
  hw = gtk_object_get_data(GTK_OBJECT(mw), id);
  grn_unlock();
  if (! hw)  return;
  
  grn_widget_set_style(hw, grn_prefs.fonts[2], NULL, NULL, NULL);
  grn_lock();
  if (GTK_IS_ENTRY(hw))
  {
    if (text)  gtk_entry_set_text(GTK_ENTRY(hw), text);
    else  gtk_entry_set_text(GTK_ENTRY(hw), "");
  }
  else if (GTK_IS_LABEL(hw))
  {
    if (text)  gtk_label_set_text(GTK_LABEL(hw), text);
    else  gtk_label_set_text(GTK_LABEL(hw), "");
  }
  grn_unlock();
}

static void show_mw_stats(grn_msgwin_stats *mws)
{
  if (! mws)  return;
  grn_appbar_set_status(GRN->appbar, _("Size: %lu bytes"), mws->msg_size);
}

void msgwin_refresh(GtkWidget *mw, t_message *msg)
{
  GtkText *text;
  grn_msgwin_stats *mws, *mws1;
  grn_html_buf *gxb;
  glong ph_res = -1;
  t_message *m = NULL;
  g_return_if_fail(mw != NULL);
  g_return_if_fail(msg != NULL);
  
  grn_lock();
  gtk_object_remove_data(GTK_OBJECT(mw), KEY_MSG);
  gtk_object_set_data(GTK_OBJECT(mw), KEY_MSG, msg);
  if (gtk_object_get_data(GTK_OBJECT(GRN->window), KEY_MHWIN))
    evt_mhwin_show(mw);
  mws1 = gtk_object_get_data(GTK_OBJECT(mw), KEY_MW_STATS);
  if (mws1)
  {
    g_free(mws1);
    gtk_object_remove_data(GTK_OBJECT(mw), KEY_MW_STATS);
  }

  text = GTK_TEXT(gtk_object_get_data(GTK_OBJECT(mw), KEY_TXT_BODY));
  grn_unlock();

  gxb = html_init_doc();
  gxb->data = text;
  m = t_message_copy(msg);
  grn_mime_parse_message(m);
  if (m->filename)
  {
    GSList *mps = NULL;
    FILE *fp;
    
#ifdef USE_PERL
    ph_res = perl_hook_run("PreShowHeader", GRN, m, NULL);
    if ((ph_res >= 1) && m->body)  gxb->html = g_strdup(m->body);
#endif
    fp = fopen(m->filename, "r");
    if (fp)
    {
      mps = body_to_mime_parts((BODY *)m->data, NULL, fp);
      fclose(fp);
    }
    if (mps)
    {
      gchar *buf;
      GSList *l;

      for (l=mps; l; l=l->next)
      {
        grn_mime_part *mp = (grn_mime_part *) l->data;
	if (! mp)  continue;
	
#ifdef USE_PERL
	ph_res = perl_hook_run("PreShowPart", GRN, mp, NULL);
        if ((ph_res >= 1) && mp->data)  str_add(&(gxb->html), mp->data);
#endif
	if ((! (ph_res >= 1)) && (strcmp(mp->type, "multipart")) &&
	    (strcmp(mp->type, "message")))
	{
	  buf = g_strdup_printf("<BR>_____ MIME part: %s/%s; Size = %ld; Encoding: %s _____<BR>",
	        mp->type, mp->subtype, mp->size, mp->enc);
	  str_add(&(gxb->html), buf);
	  str_free(&buf);
	  if (! g_strcasecmp(mp->type, "text"))
	  { /* libxml's strictness workaround, waiting for gtkhtml */
/*	    if (! strcmp(mp->subtype, "html"))  buf = g_strdup(mp->data);
	    else*/  buf = html_escape(mp->data);
	    str_add(&(gxb->html), buf);
	    str_free(&buf);
	  }
	}
        grn_mime_part_free(&mp);
      }
      g_slist_free(mps);
    }
#ifdef USE_PERL
    ph_res = perl_hook_run("PreShowFooter", GRN, m, NULL);
    if ((ph_res >= 1) && m->body)  str_add(&(gxb->html), m->body);
#endif
  }
  else {
#ifdef USE_PERL
    ph_res = perl_hook_run("PreShow", GRN, m, NULL);
    if ((ph_res >= 1) && m->body)  gxb->html = g_strdup(m->body);
#endif
  }
  if (! (ph_res >= 1) && (! gxb->html))
  {
    str_add(&(gxb->html), "<HTML>\n");
    if (m->mh && str_check(m->mh->subject))
    {
      gchar *buf1 = html_escape(m->mh->subject);
      gchar *buf = g_strdup_printf("<HEAD>\n<TITLE>%s</TITLE>\n</HEAD>\n", buf1);
      str_free(&buf1);
      str_add(&(gxb->html), buf);
      str_free(&buf);
    }
    str_add(&(gxb->html), "<BODY>");
    html_parse_msg_body(gxb, msg->body);
    str_add(&(gxb->html), "</BODY>\n</HTML>\n");
  }

  if (m->mh)
  {
    msgwin_set_hdr(mw, KEY_MW_FROM, m->mh->from);
    msgwin_set_hdr(mw, KEY_MW_DATE, date_get_local(m->mh->parsed_date));
    msgwin_set_hdr(mw, KEY_MW_TO, t_msgheaders_get_extra_hdr(m->mh, "X-Comment-To"));
    msgwin_set_hdr(mw, KEY_MW_SUBJ, m->mh->subject);
  }
  if (text && m->body)
  {
    gchar *fnt_name;

    if (grn_prefs.mw_prop)  fnt_name = grn_prefs.fonts[0];
    else  fnt_name = grn_prefs.fonts[1];
    grn_widget_set_style(GTK_WIDGET(text), fnt_name, NULL,
      &(grn_prefs.bg_colors[0]), grn_prefs.pixmaps[0]);

    grn_lock();
    gtk_editable_delete_text(GTK_EDITABLE(text), 0, -1);
    gtk_text_freeze(text);
    grn_unlock();

    html_draw_body(gxb);
    grn_lock();
    gtk_text_set_point(text, 0);
    gtk_text_thaw(text);
    grn_unlock();
    html_free_doc(&gxb);

    mws = g_new(grn_msgwin_stats, 1);
    mws->msg_size = strlen(msg->body);
    grn_lock();
    gtk_object_set_data(GTK_OBJECT(mw), KEY_MW_STATS, mws);
    grn_unlock();
    show_mw_stats(mws);
  }
  if (m)  t_message_free(&m);
}


static void return_to_threadlist(GtkWidget *w)
{
  if (! GRN->threadlist)
  {
    evt_grouplist(w);
    return;
  }
  grn_set_contents(GRN->threadlist);
}

static void scroll_line(GtkText *text, gint offset)
{
  gfloat new_val = text->vadj->value + offset;
  if (new_val < text->vadj->lower)  new_val = text->vadj->lower;
  if (new_val > (text->vadj->upper - text->vadj->page_size))
    new_val = text->vadj->upper - text->vadj->page_size;
  gtk_adjustment_set_value(text->vadj, new_val);
}

static void scroll_smooth(GtkText *text, gint n)
{
  gint i=0;

  if (n > 0)
    while (i <= n)
    {
      gint step = sin(M_PI/n*i) * 4 + 1;
      if (step > (n - i))  step = n - i;
      i += step;
      scroll_line(text, step);
      if (n == i)  break;
    }
  else
    while (i >= n)
    {
      gint step = -sin(M_PI/n*i) * 4 - 1;
      if (step < (n - i))  step = n - i;
      i += step;
      scroll_line(text, step);
      if (n == i)  break;
    }
}

static gint evt_mw_keypress(GtkWidget *w, GdkEventKey *event, gpointer data)
{
  gint offset = 0;
  GtkText *text;
  GtkWidget *tmp;
  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);
  
  tmp = gtk_object_get_data(GTK_OBJECT(w), KEY_TXT_BODY);
  if ((! tmp) && (GRN->msgwin))
    tmp = gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_TXT_BODY);
  if (! tmp)  return FALSE;
  text = GTK_TEXT(tmp);

  switch (event->keyval)
  {
    case GDK_Left:
    case GDK_KP_Left:
      return_to_threadlist(w);
      break;
    case GDK_Right:
    case GDK_KP_Right:
      if (GRN->threadlist)
      {
        evt_tl_urnext_msg(GRN->threadlist);
	evt_tl_open_article(GRN->threadlist);
      }
      break;
    case GDK_Up:
    case GDK_KP_Up:
      if (event->state & GDK_CONTROL_MASK)  offset = -1;
      else if (event->state & GDK_SHIFT_MASK)  offset = -2;
      else  offset = -5;
      break;
    case GDK_Down:
    case GDK_KP_Down:
      if (event->state & GDK_CONTROL_MASK)  offset = 1;
      else if (event->state & GDK_SHIFT_MASK)  offset = 2;
      else  offset = 5;
      break;
    case GDK_Page_Up:
    case GDK_KP_Page_Up:
      if (grn_prefs.smooth_scrolling && (event->state == 0))  offset = -4;
      else  return FALSE;
      break;
    case GDK_Page_Down:
    case GDK_KP_Page_Down:
      if (grn_prefs.smooth_scrolling && (event->state == 0))  offset = 4;
      else  return FALSE;
      break;
    default:  return FALSE;
  }
  gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "key_press_event");
  if (offset && text)
  {
    gint n, lh = gdk_char_height(GTK_WIDGET(text)->style->font, 'X');

    if (offset == 5)  n = lh;
    else if (offset == -5)  n = -lh;
    else  n = offset * text->vadj->page_increment / 4;

    if (grn_prefs.smooth_scrolling)  scroll_smooth(text, n);
    else  scroll_line(text, n);
  }
  return TRUE;
}

static gint evt_mw_event(GtkWidget *w, GdkEvent *event, GtkText *text)
{
  static gboolean dragging = FALSE;
  static gint oy = 0;
  gint y = 0;
  GdkCursor *cursor;
  g_return_val_if_fail(event != NULL, FALSE);
  

  switch (event->type)
  {
    case GDK_BUTTON_PRESS:
      dragging = FALSE;
      if (event->button.button == 2)
      {
        dragging = TRUE;
        cursor = gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW);
	gdk_window_set_cursor(w->window, cursor);
	gdk_cursor_destroy(cursor);
	gtk_grab_add(w);
	oy = event->button.y;
      }
      break;
    case GDK_BUTTON_RELEASE:
      dragging = FALSE;
      gtk_grab_remove(w);
      cursor = gdk_cursor_new(GDK_LEFT_PTR);
      gdk_window_set_cursor(w->window, cursor);
      gdk_cursor_destroy(cursor);
      break;
    case GDK_MOTION_NOTIFY:
      if (! dragging)  break;
      y = event->motion.y;
      if (y != oy)
      {
        gfloat val = text->vadj->value - (y - oy) /** text->vadj->step_increment*/;
        if (val < text->vadj->lower)  val = text->vadj->lower;
        if (val > (text->vadj->upper - text->vadj->page_size))
          val = text->vadj->upper - text->vadj->page_size;
        gtk_adjustment_set_value(text->vadj, val);
        oy = y;
      }
      break;
    default: break;
  }
  return FALSE;
}

static gint evt_mw_btnpress(GtkWidget *w, GdkEventButton *event, gpointer data)
{
  GtkText *text;
  GtkWidget *tmp;
  gint offset = 0;
  gboolean processed = FALSE;
  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  tmp = gtk_object_get_data(GTK_OBJECT(w), KEY_TXT_BODY);
  if ((! tmp) && (GRN->msgwin))
    tmp = gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_TXT_BODY);
  if (! tmp)  return FALSE;
  text = GTK_TEXT(tmp);

  if (event->button == 3)
  {
    GtkWidget *mpopup;
    
    mpopup = gnome_popup_menu_new(mw_context_menu);
    grn_menuitem_check(mpopup, _("Proportional font"), grn_prefs.mw_prop);
    grn_menuitem_check(mpopup, _("Monospaced font"), !grn_prefs.mw_prop);
    gnome_popup_menu_do_popup(mpopup, NULL, NULL, event, NULL);
    processed = TRUE;
  }
  else if (event->button == 4)
  {
    if (event->state & GDK_CONTROL_MASK)  offset = -4;
    else if (event->state & GDK_SHIFT_MASK)  offset = -2;
    else  offset = -1;
  }
  else if (event->button == 5)
  {
    if (event->state & GDK_CONTROL_MASK)  offset = 4;
    else if (event->state & GDK_SHIFT_MASK)  offset = 2;
    else  offset = 1;
  }

  if (offset && text)
  {
    gint n = offset * text->vadj->page_increment / 4;

    if (grn_prefs.smooth_scrolling)  scroll_smooth(text, n);
    else  scroll_line(text, n);
    processed = TRUE;
  }
  if (processed)
  {
    gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "button_press_event");
    return TRUE;
  }
  return FALSE;
}

static void evt_mw_hide(GtkWidget *w, gpointer data)
{
  GtkText *text  = GTK_TEXT(gtk_object_get_data(GTK_OBJECT(w), KEY_TXT_BODY));
  show_message_menu(FALSE);
  if (text)  gtk_editable_delete_text(GTK_EDITABLE(text), 0, -1);
}

static void evt_mw_show(GtkWidget *w, gpointer data)
{
  grn_msgwin_stats *mws;

  show_message_menu(TRUE);
  mws = gtk_object_get_data(GTK_OBJECT(GRN->window), KEY_MW_STATS);
  show_mw_stats(mws);
}

static void mw_toggle_font(GtkWidget *w, gboolean is_prop)
{
  GtkCheckMenuItem *cmi;
  GtkWidget *text;
  gchar *fnt_name;
  g_return_if_fail(w != NULL);
  g_return_if_fail(GTK_IS_CHECK_MENU_ITEM(w));
  
  cmi = GTK_CHECK_MENU_ITEM(w);
  if (! cmi->active)  return;
  
  if (! GRN->msgwin)  return;
  text = gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_TXT_BODY);
  if (! text)  return;
  
  if (is_prop)  fnt_name = grn_prefs.fonts[0];
  else  fnt_name = grn_prefs.fonts[1];
  grn_widget_set_style(text, fnt_name, NULL, NULL, NULL);
  grn_prefs.mw_prop = is_prop;
}

void evt_mw_prop_font(GtkWidget *w)
{  mw_toggle_font(w, TRUE);  }
void evt_mw_monosp_font(GtkWidget *w)
{  mw_toggle_font(w, FALSE);  }


/* Returns newly allocated (t_message *) or NULL on fail. */
t_message *get_cur_msg(GtkWidget *w)
{
  GtkCTree *ctree;
  GtkCList *clist;
  t_message *ret=NULL;

  if (w && GRN->threadlist)
  {
    if (! GTK_IS_CTREE(w))
    {
      grn_lock();
      ctree = GTK_CTREE(gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_CT_MSGS));
      grn_unlock();
    }
    else  ctree = GTK_CTREE(w);
    if (ctree)
    {
      clist = GTK_CLIST(ctree);
      ret = t_message_copy(tl_get_selected_message(ctree));
      if (ret)
      {
        if (! ret->cached)
	{
	  if (ret->mh)  t_msgheaders_rm_extra_hdrs(ret->mh);
	  nntp_download_article(ret);
//	  grnq_add_download_article(ret);
	}
        return ret;
      }
    }
  }
  if (GRN->msgwin)
  {
    grn_lock();
    ret = t_message_copy((t_message *) gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_MSG));
    grn_unlock();
  }
  if (ret)  return ret;
  if (GRN->grouplist)
  {
    ret = t_message_alloc();
    ret->mh = t_msgheaders_alloc();
    grn_lock();
    clist = GTK_CLIST(gtk_object_get_data(GTK_OBJECT(GRN->grouplist), KEY_CL_GROUPS));
    if (clist && clist->selection)
    {
      gint row = GPOINTER_TO_INT(clist->selection->data);
      ret->mh->grp = gtk_clist_get_row_data(clist, row);
    }
    grn_unlock();
  }
  return ret;
}

gchar *get_cur_group(GtkWidget *w)
{
  GtkCTree *ctree;
  GtkCList *clist;
  t_message *m = NULL;

  if (GRN->grouplist)
  {
    grn_lock();
    clist = GTK_CLIST(gtk_object_get_data(GTK_OBJECT(GRN->grouplist), KEY_CL_GROUPS));
    if (clist && clist->selection)
    {
      gint row = GPOINTER_TO_INT(clist->selection->data);
      grn_newsgroup *grp = gtk_clist_get_row_data(clist, row);
      if (grp && grp->name)  return g_strdup(grp->name);
    }
    grn_unlock();
  }
  if (GRN->threadlist)
  {
    grn_lock();
    ctree = GTK_CTREE(gtk_object_get_data(GTK_OBJECT(GRN->threadlist), KEY_CT_MSGS));
    grn_unlock();
    if (ctree)
    {
      clist = GTK_CLIST(ctree);
      m = tl_get_selected_message(ctree);
      if (m && m->mh && m->mh->grp && m->mh->grp->name)
        return g_strdup(m->mh->grp->name);
    }
  }
  if (GRN->msgwin)
  {
    grn_lock();
    m = (t_message *) gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_MSG);
    grn_unlock();
    if (m && m->mh && m->mh->grp && m->mh->grp->name)
      return g_strdup(m->mh->grp->name);
  }
  return NULL;
}
