/*
 * grn_prefs.c: preferences dialog
 *
 * $Id: grn_prefs.c,v 1.22 2000/05/19 14:51:46 sc Exp $
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
#include "grn_vars.h"
#include "grn_config.h"
#include "grn_prefs.h"
#include "grn_util.h"
#include "grn_nglist.h"


typedef struct GrnPrefs
{
  GnomePropertyBox *pbox;
  GtkRadioButton *toolbar_type[PREFS_NUM_TOOLBAR_STYLES];
  GtkWidget *nntp_server;
  GtkWidget *nntp_auth;
  GtkWidget *nntp_login;
  GtkWidget *nntp_passwd;
  GtkWidget *groups_desc;
  GtkWidget *smtp_server;
  GtkWidget *en_name;
  GtkWidget *en_email;
  GtkWidget *en_replyto;
  GtkWidget *en_org;
  GtkWidget *en_qre;
  GtkWidget *en_ua;
  GtkWidget *srv_autoconnect;
  GtkWidget *cb_tooltips;
  GtkWidget *cb_sscroll;
  GtkWidget *cfms[NCFMS];
  GtkWidget *fp[NFONTS];
  GtkWidget *fg_cp[NCOLORS];
  GtkWidget *bg_cp[NCOLORS];
  GtkWidget *en_path[NPATHS];
} GrnPrefs;

typedef struct FontPal
{
  GnomeDialog *dialog;
  GtkCList  *clist;
  GtkWidget *lbl_test;
  GtkWidget *btn_edit;
  GtkWidget *btn_del;
  GtkWidget *btn_up;
  GtkWidget *btn_down;
} FontPal;

typedef struct ColorPal
{
  GnomeDialog *dialog;
  GtkCList  *clist;
  GtkWidget *btn_edit;
  GtkWidget *btn_del;
  GtkWidget *btn_up;
  GtkWidget *btn_down;
} ColorPal;


static GrnPrefs *pui=NULL;
static FontPal *fpal=NULL;
static ColorPal *cpal=NULL;

static guint toolbar_stl[PREFS_NUM_TOOLBAR_STYLES] =
             { GTK_TOOLBAR_TEXT, GTK_TOOLBAR_ICONS, GTK_TOOLBAR_BOTH, -1 };
static gchar *toolbar_stl_label[PREFS_NUM_TOOLBAR_STYLES] =
	     { N_("Text"), N_("Icons"), N_("Both"), N_("Gnome setting") };

static GtkWidget *create_server_page(void);
static GtkWidget *create_identity_page(void);
static GtkWidget *create_display_page(void);
static GtkWidget *create_fonts_page(void);
static GtkWidget *create_colors_page(void);
static GtkWidget *create_confirm_page(void);
static GtkWidget *create_paths_page(void);
static void properties_modified_cb(GtkWidget *, GnomePropertyBox *);
static void set_prefs(void);
static void cancel_prefs(void);
static void apply_prefs(GnomePropertyBox *, gint, GrnPrefs *);


void evt_preferences(GtkWidget *w)
{
  GtkWidget *label;
  GnomeApp *active_win;
  gint i;
  
  if (pui)
  {
    gdk_window_raise(GTK_WIDGET(GNOME_DIALOG(pui->pbox))->window);
    return;
  }
  pui = (GrnPrefs *) g_malloc(sizeof(GrnPrefs));
  pui->pbox = GNOME_PROPERTY_BOX(gnome_property_box_new());
  gtk_window_set_title(GTK_WINDOW(pui->pbox), _("GRN Preferences"));
  active_win = GNOME_APP(GRN->window);
  gnome_dialog_set_parent(GNOME_DIALOG(pui->pbox), GTK_WINDOW(active_win));
  
  gtk_signal_connect(GTK_OBJECT(pui->pbox), "destroy",
                     GTK_SIGNAL_FUNC(cancel_prefs), pui);
  gtk_signal_connect(GTK_OBJECT(pui->pbox), "delete_event",
                     GTK_SIGNAL_FUNC(gtk_false), NULL);
  gtk_signal_connect(GTK_OBJECT(pui->pbox), "apply",
                     GTK_SIGNAL_FUNC(apply_prefs), pui);

  label = gtk_label_new(_("Server"));
  gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(pui->pbox)->notebook),
                           create_server_page(), label);
  label = gtk_label_new(_("Identity"));
  gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(pui->pbox)->notebook),
                           create_identity_page(), label);
  label = gtk_label_new(_("Display"));
  gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(pui->pbox)->notebook),
                           create_display_page(), label);
  label = gtk_label_new(_("Fonts"));
  gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(pui->pbox)->notebook),
                           create_fonts_page(), label);
  label = gtk_label_new(_("Colors"));
  gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(pui->pbox)->notebook),
                           create_colors_page(), label);
  label = gtk_label_new(_("Confirmations"));
  gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(pui->pbox)->notebook),
                           create_confirm_page(), label);
  label = gtk_label_new(_("Paths"));
  gtk_notebook_append_page(GTK_NOTEBOOK(GNOME_PROPERTY_BOX(pui->pbox)->notebook),
                           create_paths_page(), label);
  set_prefs();
  
  gtk_signal_connect(GTK_OBJECT(pui->srv_autoconnect), "clicked",
		    GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->nntp_auth), "clicked",
		    GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->groups_desc), "clicked",
		    GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->cb_tooltips), "clicked",
		    GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->cb_sscroll), "clicked",
		    GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->nntp_server), "changed",
                     GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->nntp_login), "changed",
                     GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->nntp_passwd), "changed",
                     GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->smtp_server), "changed",
                     GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->en_name), "changed",
                     GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->en_email), "changed",
                     GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->en_replyto), "changed",
                     GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->en_org), "changed",
                     GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->en_qre), "changed",
                     GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  gtk_signal_connect(GTK_OBJECT(pui->en_ua), "changed",
                     GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  for (i=0; i<PREFS_NUM_TOOLBAR_STYLES; i++)
    gtk_signal_connect(GTK_OBJECT(pui->toolbar_type[i]), "clicked",
		       GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  for (i=0; i<NPATHS; i++)
    gtk_signal_connect(GTK_OBJECT(pui->en_path[i]), "changed",
		       GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  for (i=0; i<NCFMS; i++)
    gtk_signal_connect(GTK_OBJECT(pui->cfms[i]), "clicked",
		      GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  for (i=0; i<NFONTS; i++)
    gtk_signal_connect(GTK_OBJECT(pui->fp[i]), "clicked",
		       GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  for (i=0; i<NCOLORS; i++)
    gtk_signal_connect(GTK_OBJECT(pui->fg_cp[i]), "clicked",
		       GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);
  for (i=0; i<NCOLORS; i++)
    if (pui->bg_cp[i])  gtk_signal_connect(GTK_OBJECT(pui->bg_cp[i]), "clicked",
		          GTK_SIGNAL_FUNC(properties_modified_cb), pui->pbox);

  gtk_widget_show_all(GTK_WIDGET(pui->pbox));
}

static void nntp_auth_clicked(GtkWidget *w, gpointer data)
{
  gboolean active = GTK_TOGGLE_BUTTON(pui->nntp_auth)->active;
  gtk_widget_set_sensitive(pui->nntp_login, active);
  gtk_widget_set_sensitive(pui->nntp_passwd, active);
}

static GtkWidget *create_server_page(void)
{
  GtkWidget *vbox, *table, *label;
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
  table = gtk_table_new(4, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 5);
  
  label = gtk_label_new(_("NNTP Server:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,0,1, GTK_FILL, GTK_FILL, 5,2);
  pui->nntp_server = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), pui->nntp_server, 1,2,0,1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,2);

  pui->srv_autoconnect = gtk_check_button_new_with_label(_("Connect to NNTP server at startup"));
  gtk_table_attach(GTK_TABLE(table), pui->srv_autoconnect, 1,2,1,2,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 2);

  pui->nntp_auth = gtk_check_button_new_with_label(_("NNTP server needs authentication"));
  gtk_table_attach(GTK_TABLE(table), pui->nntp_auth, 1,2,2,3,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 2);

  label = gtk_label_new(_("Login:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,3,4, GTK_FILL, GTK_FILL, 5,2);
  pui->nntp_login = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), pui->nntp_login, 1,2,3,4,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,2);

  label = gtk_label_new(_("Password:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,4,5, GTK_FILL, GTK_FILL, 5,2);
  pui->nntp_passwd = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(pui->nntp_passwd), FALSE);
  gtk_table_attach(GTK_TABLE(table), pui->nntp_passwd, 1,2,4,5,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,2);
  pui->groups_desc = gtk_check_button_new_with_label(_("Download groups descriptions"));
  gtk_table_attach(GTK_TABLE(table), pui->groups_desc, 1,2,5,6,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 2);


  gtk_widget_set_sensitive(pui->nntp_login, grn_prefs.nntp_auth);
  gtk_widget_set_sensitive(pui->nntp_passwd, grn_prefs.nntp_auth);
  gtk_signal_connect(GTK_OBJECT(pui->nntp_auth), "clicked",
		    GTK_SIGNAL_FUNC(nntp_auth_clicked), NULL);

  gtk_table_attach(GTK_TABLE(table), gtk_hseparator_new(), 0,2,6,7,
                   GTK_FILL, GTK_FILL, 0,2);

  label = gtk_label_new(_("SMTP Server:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,7,8, GTK_FILL, GTK_FILL, 5,2);
  pui->smtp_server = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), pui->smtp_server, 1,2,7,8,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,2);

  return vbox;
}

static GtkWidget *create_identity_page(void)
{
  GtkWidget *vbox, *table, *label;
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
  table = gtk_table_new(4, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 5);
  
  label = gtk_label_new(_("Your name:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,0,1, GTK_FILL, GTK_FILL, 5,2);
  pui->en_name = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), pui->en_name, 1,2,0,1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,2);

  label = gtk_label_new(_("e-mail address:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,1,2, GTK_FILL, GTK_FILL, 5,2);
  pui->en_email = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), pui->en_email, 1,2,1,2,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,2);

  label = gtk_label_new(_("Reply-to address:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,2,3, GTK_FILL, GTK_FILL, 5,2);
  pui->en_replyto = gtk_entry_new();
  gtk_tooltips_set_tip(GRN->tooltips, pui->en_replyto, _("Specify your Reply-to address if only it differs from e-mail address"), NULL);
  gtk_table_attach(GTK_TABLE(table), pui->en_replyto, 1,2,2,3,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,2);

  label = gtk_label_new(_("Organization:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,3,4, GTK_FILL, GTK_FILL, 5,2);
  pui->en_org = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), pui->en_org, 1,2,3,4,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,2);

  label = gtk_label_new(_("User-Agent:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,4,5, GTK_FILL, GTK_FILL, 5,2);
  pui->en_ua = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), pui->en_ua, 1,2,4,5,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,2);

  return vbox;
}

static GtkWidget *create_paths_page(void)
{
  GtkWidget *vbox, *table, *label;
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
  table = gtk_table_new(4, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 5);
  
  label = gtk_label_new(_("newsrc:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,0,1, GTK_FILL, GTK_FILL, 5,2);
  pui->en_path[0] = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), pui->en_path[0], 1,2,0,1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,2);

  label = gtk_label_new(_("backup newsrc:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,1,2, GTK_FILL, GTK_FILL, 5,2);
  pui->en_path[1] = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), pui->en_path[1], 1,2,1,2,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,2);

  return vbox;
}

static GtkWidget *create_display_page(void)
{
  GtkWidget *vbox, *vbox1, *frame, *table, *label;
  GSList *group;
  gint i;
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
  table = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 5);

  frame = gtk_frame_new(_("Toolbars"));
  gtk_table_attach(GTK_TABLE(table), frame, 0,1,0,4, GTK_FILL, GTK_FILL, 2,0);
  
  vbox1 = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(vbox1));
  
  group = NULL;
  for (i=0; i<PREFS_NUM_TOOLBAR_STYLES; i++)
  {
    pui->toolbar_type[i] = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(group, _(toolbar_stl_label[i])));
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(pui->toolbar_type[i]), TRUE, TRUE, 2);
    group = gtk_radio_button_group(pui->toolbar_type[i]);
  }
  
  pui->cb_tooltips = gtk_check_button_new_with_label(_("Show tooltips"));
  gtk_table_attach(GTK_TABLE(table), pui->cb_tooltips, 1,2,0,1, GTK_FILL, GTK_FILL, 3,0);

  pui->cb_sscroll = gtk_check_button_new_with_label(_("Smooth scrolling"));
  gtk_table_attach(GTK_TABLE(table), pui->cb_sscroll, 1,2,1,2, GTK_FILL, GTK_FILL, 3,0);

  table = gtk_table_new(4, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 5);

  label = gtk_label_new(_("Quoting regexp:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1,0,1,
                   GTK_FILL, GTK_FILL, 5,5);
  pui->en_qre = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), pui->en_qre, 1,2,0,1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0,5);

  return vbox;
}

static void fp_set_font(GnomeFontPicker *fp, gchar *font_name, GtkLabel *label)
{
  g_return_if_fail(GTK_IS_LABEL(label));
  grn_widget_set_style(GTK_WIDGET(label), font_name, NULL, NULL, NULL);
  gtk_label_set_text(label, font_name);
}
static GtkWidget *create_fonts_page(void)
{
  gchar *frame_title[NFONTS] =
  {
    _("Article body proportional font"),
    _("Article body monospaced font"),
    _("Message headers font"),
    _("Threadlist font"),
    _("Groups listing font")
  };
  GtkWidget *vbox, *table;
  gint i;
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
  table = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
  
  for (i=0; i<NFONTS; i++)
  {
    GtkWidget *frame, *hbox, *label;

    frame = gtk_frame_new(frame_title[i]);
    gtk_table_attach(GTK_TABLE(table), frame, 0, 1, i, i + 1,
                     GTK_FILL|GTK_EXPAND, GTK_FILL, 2, 2);
    hbox = gtk_hbox_new(FALSE, FALSE);
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 1);
    pui->fp[i] = gnome_font_picker_new();
    fp_set_font(GNOME_FONT_PICKER(pui->fp[i]), grn_prefs.fonts[i], GTK_LABEL(label));
    gtk_signal_connect(GTK_OBJECT(pui->fp[i]), "font_set",
                       GTK_SIGNAL_FUNC(fp_set_font), label);
    gtk_box_pack_start(GTK_BOX(hbox), pui->fp[i], FALSE, FALSE, 1);
  }

  return vbox;
}

static GtkWidget *create_colors_page(void)
{
  GtkWidget *vbox, *table, *label;
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
  table = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
  
  label = gtk_label_new(_("Foreground"));
  gtk_table_attach(GTK_TABLE(table), label, 2,3,0,1, GTK_FILL, GTK_FILL, GNOME_PAD_SMALL,1);
  label = gtk_label_new(_("Background"));
  gtk_table_attach(GTK_TABLE(table), label, 3,4,0,1, GTK_FILL, GTK_FILL, GNOME_PAD_SMALL,1);
  gtk_table_attach(GTK_TABLE(table), gtk_hseparator_new(), 0,4,1,2,
                   GTK_FILL, GTK_FILL, 0,GNOME_PAD_SMALL);
  gtk_table_attach(GTK_TABLE(table), gtk_vseparator_new(), 1,2,0,5,
                   GTK_FILL, GTK_FILL, GNOME_PAD_SMALL,1);

  label = gtk_label_new(_("Article body"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1,2,3, GTK_FILL, GTK_FILL, GNOME_PAD_SMALL,GNOME_PAD_SMALL);
  pui->fg_cp[0] = gnome_color_picker_new();
  gtk_table_attach(GTK_TABLE(table), pui->fg_cp[0], 2,3,2,3, GTK_FILL, GTK_FILL, GNOME_PAD_SMALL,GNOME_PAD_SMALL);
  pui->bg_cp[0] = gnome_color_picker_new();
  gtk_table_attach(GTK_TABLE(table), pui->bg_cp[0], 3,4,2,3, GTK_FILL, GTK_FILL, GNOME_PAD_SMALL,GNOME_PAD_SMALL);

  label = gtk_label_new(_("Quoted text"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1,3,4, GTK_FILL, GTK_FILL, GNOME_PAD_SMALL,GNOME_PAD_SMALL);
  pui->fg_cp[1] = gnome_color_picker_new();
  gtk_table_attach(GTK_TABLE(table), pui->fg_cp[1], 2,3,3,4, GTK_FILL, GTK_FILL, GNOME_PAD_SMALL,GNOME_PAD_SMALL);
  pui->bg_cp[1] = NULL;

  label = gtk_label_new(_("Signature"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1,4,5, GTK_FILL, GTK_FILL, GNOME_PAD_SMALL,GNOME_PAD_SMALL);
  pui->fg_cp[2] = gnome_color_picker_new();
  gtk_table_attach(GTK_TABLE(table), pui->fg_cp[2], 2,3,4,5, GTK_FILL, GTK_FILL, GNOME_PAD_SMALL,GNOME_PAD_SMALL);
  pui->bg_cp[2] = NULL;

  return vbox;
}

static GtkWidget *create_confirm_page(void)
{
  gchar *cfm_names[NCFMS] =
  {
    _("Catchup group"),
    _("Mark group unread"),
    _("Mark thread read"),
    _("Mark thread unread"),
    _("Abort posting"),
    _("Show warnings"),
    _("Show informational messages")
  };
  GtkWidget *vbox;
  gint i;
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
  
  for (i=0; i<NCFMS; i++)
  {
    pui->cfms[i] = gtk_check_button_new_with_label(cfm_names[i]);
    gtk_box_pack_start(GTK_BOX(vbox), pui->cfms[i], FALSE, FALSE, 2);
  }

  return vbox;
}

static void properties_modified_cb(GtkWidget *w, GnomePropertyBox *gpb)
{
  gnome_property_box_changed(gpb);
}

static void set_prefs(void)
{
  gint i;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pui->srv_autoconnect), grn_prefs.srv_autoconnect);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pui->nntp_auth), grn_prefs.nntp_auth);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pui->groups_desc), grn_prefs.groups_desc);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pui->cb_tooltips), grn_prefs.tooltips);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pui->cb_sscroll), grn_prefs.smooth_scrolling);
  gtk_entry_set_text(GTK_ENTRY(pui->nntp_server), grn_prefs.nntp_server);
  gtk_entry_set_text(GTK_ENTRY(pui->nntp_login), grn_prefs.nntp_login);
  gtk_entry_set_text(GTK_ENTRY(pui->nntp_passwd), grn_prefs.nntp_passwd);
  gtk_entry_set_text(GTK_ENTRY(pui->smtp_server), grn_prefs.smtp_server);
  gtk_entry_set_text(GTK_ENTRY(pui->en_name), grn_prefs.name);
  gtk_entry_set_text(GTK_ENTRY(pui->en_email), grn_prefs.email);
  gtk_entry_set_text(GTK_ENTRY(pui->en_replyto), grn_prefs.replyto);
  gtk_entry_set_text(GTK_ENTRY(pui->en_org), grn_prefs.org);
  gtk_entry_set_text(GTK_ENTRY(pui->en_ua), grn_prefs.ua);
  gtk_entry_set_text(GTK_ENTRY(pui->en_qre), grn_prefs.re_quoting);

  for (i=0; i<NPATHS; i++)
    gtk_entry_set_text(GTK_ENTRY(pui->en_path[i]), grn_prefs.paths[i]);

  for (i=0; i<PREFS_NUM_TOOLBAR_STYLES; i++)
    if (grn_prefs.toolbar_style == toolbar_stl[i])
    {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pui->toolbar_type[i]), TRUE);
      break;
    }
  for (i=0; i<NCFMS; i++)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pui->cfms[i]), grn_prefs.cfms[i]);
  for (i=0; i<NFONTS; i++)
    gnome_font_picker_set_font_name(GNOME_FONT_PICKER(pui->fp[i]), g_strdup(grn_prefs.fonts[i]));
  for (i=0; i<NCOLORS; i++)
  {
    gnome_color_picker_set_i16(GNOME_COLOR_PICKER(pui->fg_cp[i]), grn_prefs.fg_colors[i].red,
      grn_prefs.fg_colors[i].green,  grn_prefs.fg_colors[i].blue, 65534);
    if (pui->bg_cp[i])
      gnome_color_picker_set_i16(GNOME_COLOR_PICKER(pui->bg_cp[i]), grn_prefs.bg_colors[i].red,
        grn_prefs.bg_colors[i].green,  grn_prefs.bg_colors[i].blue, 65534);
  }
}

static void cancel_prefs(void)
{
  gtk_widget_destroy(GTK_WIDGET(pui->pbox));
  g_free(pui);
  pui = NULL;
}

static void apply_prefs(GnomePropertyBox *pbox, gint page, GrnPrefs *gp)
{
  gint i;
  GdkColormap *cmap;

  grn_prefs.srv_autoconnect = GTK_TOGGLE_BUTTON(pui->srv_autoconnect)->active;
  grn_prefs.nntp_auth = GTK_TOGGLE_BUTTON(pui->nntp_auth)->active;
  grn_prefs.groups_desc = GTK_TOGGLE_BUTTON(pui->groups_desc)->active;
  grn_prefs.tooltips = GTK_TOGGLE_BUTTON(pui->cb_tooltips)->active;
  grn_prefs.smooth_scrolling = GTK_TOGGLE_BUTTON(pui->cb_sscroll)->active;
  str_free(&(grn_prefs.nntp_server));
  grn_prefs.nntp_server = g_strdup(gtk_entry_get_text(GTK_ENTRY(pui->nntp_server)));
  str_free(&(grn_prefs.nntp_login));
  grn_prefs.nntp_login = g_strdup(gtk_entry_get_text(GTK_ENTRY(pui->nntp_login)));
  str_free(&(grn_prefs.nntp_passwd));
  grn_prefs.nntp_passwd = g_strdup(gtk_entry_get_text(GTK_ENTRY(pui->nntp_passwd)));
  str_free(&(grn_prefs.smtp_server));
  grn_prefs.smtp_server = g_strdup(gtk_entry_get_text(GTK_ENTRY(pui->smtp_server)));
  str_free(&(grn_prefs.name));
  grn_prefs.name = g_strdup(gtk_entry_get_text(GTK_ENTRY(pui->en_name)));
  str_free(&(grn_prefs.email));
  grn_prefs.email = g_strdup(gtk_entry_get_text(GTK_ENTRY(pui->en_email)));
  str_free(&(grn_prefs.replyto));
  grn_prefs.replyto = g_strdup(gtk_entry_get_text(GTK_ENTRY(pui->en_replyto)));
  str_free(&(grn_prefs.org));
  grn_prefs.org = g_strdup(gtk_entry_get_text(GTK_ENTRY(pui->en_org)));
  str_free(&(grn_prefs.ua));
  grn_prefs.ua = g_strdup(gtk_entry_get_text(GTK_ENTRY(pui->en_ua)));
  str_free(&(grn_prefs.re_quoting));
  grn_prefs.re_quoting = g_strdup(gtk_entry_get_text(GTK_ENTRY(pui->en_qre)));

  for (i=0; i<NPATHS; i++)
  {
    str_free(&(grn_prefs.paths[i]));
    grn_prefs.paths[i] = g_strdup(gtk_entry_get_text(GTK_ENTRY(pui->en_path[i])));
  }

  for (i=0; i<PREFS_NUM_TOOLBAR_STYLES; i++)
    if (GTK_TOGGLE_BUTTON(pui->toolbar_type[i])->active)
    {
      grn_prefs.toolbar_style = toolbar_stl[i];
      break;
    }
  for (i=0; i<NCFMS; i++)
    grn_prefs.cfms[i] = GTK_TOGGLE_BUTTON(pui->cfms[i])->active;
  for (i=0; i<NFONTS; i++)
  {
    str_free(&(grn_prefs.fonts[i]));
    grn_prefs.fonts[i] = g_strdup(gnome_font_picker_get_font_name(GNOME_FONT_PICKER(pui->fp[i])));
  }
  cmap = gdk_colormap_get_system();
  for (i=0; i<NCOLORS; i++)
  {
    gushort r, g, b, a;
    gnome_color_picker_get_i16(GNOME_COLOR_PICKER(pui->fg_cp[i]), &r, &g, &b, &a);
    grn_prefs.fg_colors[i].red = r;  grn_prefs.fg_colors[i].green = g;
    grn_prefs.fg_colors[i].blue = b;
    gdk_color_alloc(cmap, &(grn_prefs.fg_colors[i]));
    if (pui->bg_cp[i])
    {
      gnome_color_picker_get_i16(GNOME_COLOR_PICKER(pui->bg_cp[i]), &r, &g, &b, &a);
      grn_prefs.bg_colors[i].red = r;  grn_prefs.bg_colors[i].green = g;
      grn_prefs.bg_colors[i].blue = b;
      gdk_color_alloc(cmap, &(grn_prefs.bg_colors[i]));
    }
  }
  grn_prefs_realize();
}


static void destroy_fpal(GtkWidget *w)
{
  g_free(fpal);  fpal = NULL;
}

static void fpal_ok(GtkWidget *w)
{
  gint i;
  GSList *l;
  if (! fpal)  return;

  for (l=grn_prefs.fontpal; l; l=l->next)  g_free(l->data);
  g_slist_free(grn_prefs.fontpal);  grn_prefs.fontpal = NULL;

  for (i=0; i<GTK_CLIST(fpal->clist)->rows; i++)
  {
    gchar *font;
    gtk_clist_get_text(GTK_CLIST(fpal->clist), i, 1, &font);
    grn_prefs.fontpal = g_slist_prepend(grn_prefs.fontpal, g_strdup(font));
  }
  grn_prefs.fontpal = g_slist_reverse(grn_prefs.fontpal);
  gtk_widget_destroy(GTK_WIDGET(fpal->dialog));
}

static void fpal_clist_renumber(GtkWidget *clist)
{
  clist_renumber(GTK_CLIST(clist));
}

static void fpal_add_to_clist(GtkWidget *clist, gchar *font)
{
  gchar *text[2] = {"", font};
  gint row = gtk_clist_append(GTK_CLIST(clist), text);
  gtk_clist_select_row(GTK_CLIST(clist), row, 1);
}

static void fpal_sel(GtkWidget *clist, gint row, gint col, GdkEventButton *ev, GtkWidget *lbl)
{
  gchar *font;
  gtk_clist_get_text(GTK_CLIST(clist), row, 1, &font);
  grn_widget_set_style(lbl, font, NULL, NULL, NULL);
}

typedef struct font_data
{
  GtkWidget **dlg;
  GtkWidget *clist;
  gint	     row;
} font_data;

static void font_destroy(GtkWidget *w, font_data *fd)
{
  *(fd->dlg) = NULL;
  g_free(fd);
}
static void font_cancel(GtkWidget *w, font_data *fd)
{
  gtk_widget_destroy(*(fd->dlg));
}
static void font_ok(GtkWidget *w, font_data *fd)
{
  fpal_add_to_clist(fd->clist, gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(*(fd->dlg))));
  fpal_clist_renumber(fd->clist);
  gtk_widget_destroy(*(fd->dlg));
}
static void fpal_add(GtkWidget *w, GtkWidget *clist)
{
  static GtkWidget *dlg = NULL;
  font_data *fd;
  
  if (dlg)  return;
  fd = g_new(font_data, 1);
  dlg = gtk_font_selection_dialog_new(_("Add new font"));
  fd->clist = clist;  fd->dlg = &dlg;
  gtk_widget_destroy(GTK_FONT_SELECTION_DIALOG(dlg)->apply_button);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     GTK_SIGNAL_FUNC(font_destroy), fd);
  gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(dlg)->cancel_button),
                     "clicked", GTK_SIGNAL_FUNC(font_cancel), fd);
  gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(dlg)->ok_button),
                     "clicked", GTK_SIGNAL_FUNC(font_ok), fd);
  gtk_widget_show_all(dlg);
}

static void edit_ok(GtkWidget *w, font_data *fd)
{
  gtk_clist_set_text(GTK_CLIST(fd->clist), fd->row, 1,
    gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(*(fd->dlg))));
  gtk_clist_select_row(GTK_CLIST(fd->clist), fd->row, 1);
  gtk_widget_destroy(*(fd->dlg));
}
static void fpal_edit(GtkWidget *w, GtkWidget *clist)
{
  static GtkWidget *dlg = NULL;
  font_data *fd;
  gchar *font;
  
  if (dlg)  return;
  fd = g_new(font_data, 1);
  dlg = gtk_font_selection_dialog_new(_("Edit font"));
  fd->clist = clist;  fd->dlg = &dlg;
  if (GTK_CLIST(clist)->selection)
    fd->row = GPOINTER_TO_INT(GTK_CLIST(clist)->selection->data);
  else  fd->row = 0;
  gtk_clist_get_text(GTK_CLIST(clist), fd->row, 1, &font);
  gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(*(fd->dlg)), font);
  gtk_widget_destroy(GTK_FONT_SELECTION_DIALOG(dlg)->apply_button);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     GTK_SIGNAL_FUNC(font_destroy), fd);
  gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(dlg)->cancel_button),
                     "clicked", GTK_SIGNAL_FUNC(font_cancel), fd);
  gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(dlg)->ok_button),
                     "clicked", GTK_SIGNAL_FUNC(edit_ok), fd);
  gtk_widget_show_all(dlg);
}

static void fpal_del(GtkWidget *w, GtkWidget *clist)
{
  gint row;
  if (GTK_CLIST(clist)->selection)
    row = GPOINTER_TO_INT(GTK_CLIST(clist)->selection->data);
  else row = 0;
  gtk_clist_remove(GTK_CLIST(clist), row);
  fpal_clist_renumber(clist);
}

static void fpal_up(GtkWidget *w, GtkWidget *clist)
{
  gint row, drow;
  
  if (GTK_CLIST(clist)->selection)
    row = GPOINTER_TO_INT(GTK_CLIST(clist)->selection->data);
  else  row = 0;
  drow = row - 1;
  if (drow < 0)  return;
  gtk_clist_row_move(GTK_CLIST(clist), row, drow);
  fpal_clist_renumber(clist);
}

static void fpal_down(GtkWidget *w, GtkWidget *clist)
{
  gint row, drow;
  
  if (GTK_CLIST(clist)->selection)
    row = GPOINTER_TO_INT(GTK_CLIST(clist)->selection->data);
  else  row = 0;
  drow = row + 1;
  if (drow >= GTK_CLIST(clist)->rows)  return;
  gtk_clist_row_move(GTK_CLIST(clist), row, drow);
  fpal_clist_renumber(clist);
}

void evt_fontpal(GtkWidget *w)
{
  gchar *titles[] = {_("Index"), _("Font")};
  GtkWidget *table, *table1, *scrwin, *btn;
  GnomeApp *active_win;
  GSList *l;
  
  if (fpal)
  {
    gdk_window_raise(GTK_WIDGET(GNOME_DIALOG(fpal->dialog))->window);
    return;
  }
  fpal = (FontPal *) g_malloc(sizeof(FontPal));
  fpal->dialog = GNOME_DIALOG(gnome_dialog_new(_("Font palette"),
                 GNOME_STOCK_BUTTON_OK, NULL));
  active_win = GNOME_APP(GRN->window);
  gnome_dialog_set_parent(GNOME_DIALOG(fpal->dialog), GTK_WINDOW(active_win));
  
  gnome_dialog_button_connect(fpal->dialog, 0,
                              GTK_SIGNAL_FUNC(fpal_ok), fpal);
  gtk_signal_connect(GTK_OBJECT(fpal->dialog), "destroy",
                     GTK_SIGNAL_FUNC(destroy_fpal), fpal);
  gtk_signal_connect(GTK_OBJECT(fpal->dialog), "delete_event",
                     GTK_SIGNAL_FUNC(gtk_false), NULL);

  table = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(fpal->dialog->vbox), table, FALSE, FALSE, 3);
  
  fpal->clist = GTK_CLIST(gtk_clist_new_with_titles(2, titles));
  gtk_widget_set_usize(GTK_WIDGET(fpal->clist), 400, 200);
  gtk_clist_set_column_justification(fpal->clist, 0, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_selection_mode(fpal->clist, GTK_SELECTION_BROWSE);
  gtk_clist_column_titles_passive(fpal->clist);
  scrwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrwin), GTK_WIDGET(fpal->clist));
  gtk_table_attach(GTK_TABLE(table), scrwin, 0,1,0,1,
    GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 3,2);

  fpal->lbl_test = gtk_label_new(_("Test"));
  gtk_table_attach(GTK_TABLE(table), fpal->lbl_test, 1,2,0,1, GTK_FILL, GTK_FILL, 0,2);
  gtk_signal_connect(GTK_OBJECT(fpal->clist), "select_row",
                     GTK_SIGNAL_FUNC(fpal_sel), fpal->lbl_test);

  table1 = gtk_table_new(5, 1, FALSE);
  gtk_table_attach(GTK_TABLE(table), table1, 0,2,1,2, GTK_FILL|GTK_EXPAND, GTK_FILL, 0,0);
  btn = gnome_pixmap_button(gnome_stock_pixmap_widget(NULL, GNOME_STOCK_PIXMAP_ADD), _("Add..."));
  gtk_table_attach(GTK_TABLE(table1), btn, 0,1,0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 3,2);
  gtk_signal_connect(GTK_OBJECT(btn), "clicked",
                     GTK_SIGNAL_FUNC(fpal_add), fpal->clist);
  fpal->btn_edit = gnome_pixmap_button(gnome_stock_pixmap_widget(NULL, GNOME_STOCK_PIXMAP_PROPERTIES), _("Edit..."));
  gtk_table_attach(GTK_TABLE(table1), fpal->btn_edit, 1,2,0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 3,2);
  gtk_signal_connect(GTK_OBJECT(fpal->btn_edit), "clicked",
                     GTK_SIGNAL_FUNC(fpal_edit), fpal->clist);
  fpal->btn_del = gnome_pixmap_button(gnome_stock_pixmap_widget(NULL, GNOME_STOCK_PIXMAP_REMOVE), _("Delete"));
  gtk_table_attach(GTK_TABLE(table1), fpal->btn_del, 2,3,0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 3,2);
  gtk_signal_connect(GTK_OBJECT(fpal->btn_del), "clicked",
                     GTK_SIGNAL_FUNC(fpal_del), fpal->clist);
  fpal->btn_up = gnome_stock_button(GNOME_STOCK_BUTTON_UP);
  gtk_table_attach(GTK_TABLE(table1), fpal->btn_up, 3,4,0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 3,2);
  gtk_signal_connect(GTK_OBJECT(fpal->btn_up), "clicked",
                     GTK_SIGNAL_FUNC(fpal_up), fpal->clist);
  fpal->btn_down = gnome_stock_button(GNOME_STOCK_BUTTON_DOWN);
  gtk_table_attach(GTK_TABLE(table1), fpal->btn_down, 4,5,0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 3,2);
  gtk_signal_connect(GTK_OBJECT(fpal->btn_down), "clicked",
                     GTK_SIGNAL_FUNC(fpal_down), fpal->clist);
  
  for (l=grn_prefs.fontpal; l; l=l->next)
    fpal_add_to_clist(GTK_WIDGET(fpal->clist), l->data);
  gtk_widget_show_all(GTK_WIDGET(fpal->dialog));
  fpal_clist_renumber(GTK_WIDGET(fpal->clist));
}


static void cancel_cpal(GtkWidget *w)
{
  if (! cpal)  return;
  gtk_widget_destroy(GTK_WIDGET(cpal->dialog));
  g_free(cpal);  cpal = NULL;
}

typedef struct clr_data
{
  GtkWidget **dlg;
  GtkWidget *clist;
  gint	     row;
} clr_data;

static gchar *get_clr_string(GdkColor *clr)
{
  return g_strdup_printf("%s=%.02f, %s=%.02f, %s=%.02f", _("Red"),
    (gdouble) clr->red/0xffff, _("Green"), (gdouble) clr->green/0xffff,
    _("Blue"), (gdouble) clr->blue/0xffff);
}
static void cpal_add_to_clist(GtkWidget *clist, GdkColor *clr)
{
  gchar *text[2] = {"", NULL};
  gint row;
  gchar *buf = get_clr_string(clr);

  text[1] = buf;
  row = gtk_clist_append(GTK_CLIST(clist), text);
//  gtk_clist_set_foreground(GTK_CLIST(clist), row, clr);
  gtk_clist_set_row_data(GTK_CLIST(clist), row, clr);
  gtk_clist_select_row(GTK_CLIST(clist), row, 1);
  g_free(buf);
}

static void cpal_sel(GtkWidget *clist, gint row, gint col, GdkEventButton *ev, GtkWidget *btn)
{
  GdkColor *clr;
  clr = gtk_clist_get_row_data(GTK_CLIST(clist), row);
  if (! clr)  return;
  gnome_color_picker_set_i16(GNOME_COLOR_PICKER(btn), clr->red, clr->green,
                             clr->blue, 65535);
}

static void clr_destroy(GtkWidget *w, clr_data *cd)
{
  *(cd->dlg) = NULL;
  g_free(cd);
}
static void clr_cancel(GtkWidget *w, clr_data *cd)
{
  gtk_widget_destroy(*(cd->dlg));
}
static void clr_ok(GtkWidget *w, clr_data *cd)
{
  gdouble dc[4];
  GdkColor *clr = g_new(GdkColor, 1);
  
  gtk_color_selection_get_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(*(cd->dlg))->colorsel), dc);
  clr->red = dc[0] * 0xffff;  clr->green = dc[1] * 0xffff;
  clr->blue = dc[2] * 0xffff;
  cpal_add_to_clist(cd->clist, clr);
  fpal_clist_renumber(cd->clist);
  gtk_widget_destroy(*(cd->dlg));
}
static void cpal_add(GtkWidget *w, GtkWidget *clist)
{
  static GtkWidget *dlg = NULL;
  clr_data *cd;
  
  if (dlg)  return;
  cd = g_new(clr_data, 1);
  dlg = gtk_color_selection_dialog_new(_("Add new color"));
  cd->clist = clist;  cd->dlg = &dlg;
  gtk_widget_destroy(GTK_COLOR_SELECTION_DIALOG(dlg)->help_button);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     GTK_SIGNAL_FUNC(clr_destroy), cd);
  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(dlg)->cancel_button),
                     "clicked", GTK_SIGNAL_FUNC(clr_cancel), cd);
  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(dlg)->ok_button),
                     "clicked", GTK_SIGNAL_FUNC(clr_ok), cd);
  gtk_widget_show_all(dlg);
}

static void cpal_edit(GnomeColorPicker *cp, guint r, guint g, guint b, guint a, GtkWidget *clist)
{
  GdkColor *clr;
  gint row;
  gchar *buf;

  if (GTK_CLIST(clist)->selection)
    row = GPOINTER_TO_INT(GTK_CLIST(clist)->selection->data);
  else  row = 0;
  clr = gtk_clist_get_row_data(GTK_CLIST(clist), row);
  if (! clr)  return;
  clr->red = r;  clr->green = g;  clr->blue = b;
  buf = get_clr_string(clr);
  gtk_clist_set_text(GTK_CLIST(clist), row, 1, buf);
  g_free(buf);
//  gtk_clist_set_foreground(GTK_CLIST(clist), row, clr);
}

static void cpal_del(GtkWidget *w, GtkWidget *clist)
{
  gint row;
  GdkColor *clr;

  if (GTK_CLIST(clist)->selection)
    row = GPOINTER_TO_INT(GTK_CLIST(clist)->selection->data);
  else  row = 0;
  clr = gtk_clist_get_row_data(GTK_CLIST(clist), row);
  if (clr)  g_free(clr);
  gtk_clist_remove(GTK_CLIST(clist), row);
  fpal_clist_renumber(clist);
}

static void cpal_ok(GtkWidget *w, ColorPal *cpal)
{
  gint i;
  GSList *l;
  if (! cpal)  return;

  for (l=grn_prefs.colorpal; l; l=l->next)  g_free(l->data);
  g_slist_free(grn_prefs.colorpal);  grn_prefs.colorpal = NULL;

  for (i=0; i<GTK_CLIST(cpal->clist)->rows; i++)
  {
    GdkColor *clr = gtk_clist_get_row_data(GTK_CLIST(cpal->clist), i);
    if (clr)  grn_prefs.colorpal = g_slist_prepend(grn_prefs.colorpal, clr);
  }
  grn_prefs.colorpal = g_slist_reverse(grn_prefs.colorpal);
  gtk_widget_destroy(GTK_WIDGET(cpal->dialog));
}

void evt_colorpal(GtkWidget *w)
{
  gchar *titles[] = {_("Index"), _("Color")};
  GtkWidget *table, *table1, *scrwin, *btn;
  GnomeApp *active_win;
  GSList *l;
  
  if (cpal)
  {
    gdk_window_raise(GTK_WIDGET(GNOME_DIALOG(cpal->dialog))->window);
    return;
  }
  cpal = (ColorPal *) g_malloc(sizeof(ColorPal));
  cpal->dialog = GNOME_DIALOG(gnome_dialog_new(_("Color palette"),
                 GNOME_STOCK_BUTTON_OK, NULL));
  active_win = GNOME_APP(GRN->window);
  gnome_dialog_set_parent(GNOME_DIALOG(cpal->dialog), GTK_WINDOW(active_win));
  
  gnome_dialog_button_connect(cpal->dialog, 0,
                              GTK_SIGNAL_FUNC(cpal_ok), cpal);
  gtk_signal_connect(GTK_OBJECT(cpal->dialog), "destroy",
                     GTK_SIGNAL_FUNC(cancel_cpal), cpal);
  gtk_signal_connect(GTK_OBJECT(cpal->dialog), "delete_event",
                     GTK_SIGNAL_FUNC(gtk_false), NULL);

  table = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(cpal->dialog->vbox), table, FALSE, FALSE, 3);
  
  cpal->clist = GTK_CLIST(gtk_clist_new_with_titles(2, titles));
  gtk_widget_set_usize(GTK_WIDGET(cpal->clist), 300, 150);
  gtk_clist_set_column_justification(cpal->clist, 0, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_selection_mode(cpal->clist, GTK_SELECTION_BROWSE);
  gtk_clist_set_reorderable(cpal->clist, TRUE);
  gtk_clist_column_titles_passive(cpal->clist);
  scrwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrwin), GTK_WIDGET(cpal->clist));
  gtk_table_attach(GTK_TABLE(table), scrwin, 0,1,0,1,
    GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 3,2);

  table1 = gtk_table_new(5, 1, FALSE);
  gtk_table_attach(GTK_TABLE(table), table1, 0,1,1,2, GTK_FILL|GTK_EXPAND, GTK_FILL, 0,0);
  btn = gnome_pixmap_button(gnome_stock_pixmap_widget(NULL, GNOME_STOCK_PIXMAP_ADD), _("Add..."));
  gtk_table_attach(GTK_TABLE(table1), btn, 0,1,0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 3,2);
  gtk_signal_connect(GTK_OBJECT(btn), "clicked",
                     GTK_SIGNAL_FUNC(cpal_add), cpal->clist);
  cpal->btn_edit = gnome_color_picker_new();
  gtk_tooltips_set_tip(GRN->tooltips, cpal->btn_edit, _("Edit current color"), NULL);
  gtk_table_attach(GTK_TABLE(table1), cpal->btn_edit, 1,2,0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 3,2);
  gtk_signal_connect(GTK_OBJECT(cpal->btn_edit), "color_set",
                     GTK_SIGNAL_FUNC(cpal_edit), cpal->clist);
  cpal->btn_del = gnome_pixmap_button(gnome_stock_pixmap_widget(NULL, GNOME_STOCK_PIXMAP_REMOVE), _("Delete"));
  gtk_table_attach(GTK_TABLE(table1), cpal->btn_del, 2,3,0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 3,2);
  gtk_signal_connect(GTK_OBJECT(cpal->btn_del), "clicked",
                     GTK_SIGNAL_FUNC(cpal_del), cpal->clist);
  cpal->btn_up = gnome_stock_button(GNOME_STOCK_BUTTON_UP);
  gtk_table_attach(GTK_TABLE(table1), cpal->btn_up, 3,4,0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 3,2);
  gtk_signal_connect(GTK_OBJECT(cpal->btn_up), "clicked",
                     GTK_SIGNAL_FUNC(fpal_up), cpal->clist);
  cpal->btn_down = gnome_stock_button(GNOME_STOCK_BUTTON_DOWN);
  gtk_table_attach(GTK_TABLE(table1), cpal->btn_down, 4,5,0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 3,2);
  gtk_signal_connect(GTK_OBJECT(cpal->btn_down), "clicked",
                     GTK_SIGNAL_FUNC(fpal_down), cpal->clist);
  gtk_signal_connect(GTK_OBJECT(cpal->clist), "select_row",
                     GTK_SIGNAL_FUNC(cpal_sel), cpal->btn_edit);
  
  for (l=grn_prefs.colorpal; l; l=l->next)
  {
    GdkColor *clr = g_new(GdkColor, 1), *c = (GdkColor *) l->data;
    clr->red = c->red;  clr->green = c->green;  clr->blue = c->blue;
    cpal_add_to_clist(GTK_WIDGET(cpal->clist), clr);
  }
  gtk_widget_show_all(GTK_WIDGET(cpal->dialog));
  fpal_clist_renumber(GTK_WIDGET(cpal->clist));
}
