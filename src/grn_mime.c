/*
 * grn_mime.c - MIME handling routines
 *
 * Some code borrowed from Balsa
 *
 * $Id: grn_mime.c,v 1.7 2000/07/19 06:26:21 sc Exp $
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

#include <errno.h>
#include "grn_consts.h"
#include "mutt.h"
#include "mime.h"
#include "rfc2047.h"
#include "grn_vars.h"
#include "grn_types.h"
#include "grn_util.h"
#include "grn_misc.h"
#include "grn_msgpost.h"
#include "grn_config.h"


static void decode_hdrs(gpointer key, gpointer value, GHashTable *ht)
{
  char s[8192];
  gchar *new_key = g_strdup(key);
  
  rfc2047_decode(s, value, 8191);
  g_hash_table_insert(ht, new_key, g_strdup(s));
}
void grn_decode_headers(t_msgheaders *mh)
{
  char s[8192];
  g_return_if_fail(mh != NULL);
  
  if (str_check(mh->subject))
  {
    rfc2047_decode(s, mh->subject, 8191);
    str_free(&(mh->subject));
    mh->subject = g_strdup(s);
  }
  if (str_check(mh->from))
  {
    rfc2047_decode(s, mh->from, 8191);
    str_free(&(mh->from));
    mh->from = g_strdup(s);
  }
  if (str_check(mh->to))
  {
    rfc2047_decode(s, mh->to, 8191);
    str_free(&(mh->to));
    mh->to = g_strdup(s);
  }
  if (mh->extra_hdrs)
  {
    GHashTable *ht = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_foreach(mh->extra_hdrs, (GHFunc) decode_hdrs, ht);
    t_msgheaders_rm_extra_hdrs(mh);
    mh->extra_hdrs = ht;
  }
}


static void postparse_mime_parts(BODY *b, STATE *s, t_message *m)
{
  gboolean need_decode = FALSE;
  if (! b)  return;

  if ((b->type == TYPEMULTIPART) || (b->type == TYPEMESSAGE)) ;
  else {
    if (b->disposition == DISPINLINE)
    {
      gchar sh[8192], *p;
    
      if (! b->hdr)  b->hdr = mutt_new_header();
      if (! b->hdr->env)  b->hdr->env = mutt_new_envelope();
      fseek(s->fpin, b->hdr_offset, 0);
      do {
        fgets(sh, 8191, s->fpin);
        p = strrchr(sh, '\n');
        if (p)  *p = '\0';
        if (strstr(sh, ": "))
        {
          if (! mutt_strncasecmp("content-id", sh, 10))
            b->hdr->env->userhdrs = mutt_add_list(b->hdr->env->userhdrs, g_strdup(sh));
        }
      } while (strlen(sh) > 0);
      need_decode = TRUE;
    }
    if (str_check(b->filename))  need_decode = TRUE;
    if (b->encoding >= 3)  need_decode = TRUE;
    if (b->type != TYPETEXT)  need_decode = TRUE;
  }
  if (need_decode)
  {
    gchar *fn = NULL;
    if (! str_check(b->filename))
    {
      char tempfn[64] = "/tmp/grn.XXXXXXX";
      int fd = mkstemp(tempfn);
      if (fd != -1)
      {
	fn = g_strdup(tempfn);
	close(fd);
      }
      b->filename = g_strdup(fn);
    }
    else  fn = g_strdup(b->filename);
    if (fn)
    {
      s->fpout = fopen(fn, "w");
      m->temp_files = g_slist_append(m->temp_files, fn);
    }
  }
  if (s->fpout)
  {
    fseek(s->fpin, b->offset, 0);
    mutt_decode_attachment(b, s);
    fclose(s->fpout);
  }
  postparse_mime_parts(b->parts, s, m);
  postparse_mime_parts(b->next, s, m);
}

void grn_mime_parse_message(t_message *m)
{
  char tempfn[64] = "/tmp/grn.XXXXX";
  int fd;
  gchar *s;
  FILE *f;
  BODY *b, *body;
  
  fd = mkstemp(tempfn);
  if (fd == -1)
  {
    perror("Grn");  return;
  }
  s = msg_to_text(m);
  if (write(fd, s, strlen(s)) == -1 )
  {
    str_free(&s);
    perror("Grn");  return;
  }
  str_free(&s);
  lseek(fd, 0, 0);
  f = fdopen(fd, "r");
  m->fp = f;
  body = (BODY *) m->data;
  if (body)  mutt_free_body(&body);

  b = mutt_read_mime_header(f, 0);
  fseek(f, 0, SEEK_END);
  b->length = ftell(f) - b->offset;
  fseek(f, b->offset, 0);

  if ((b->type == TYPETEXT) && (b->encoding < 3) && (! strcmp(b->subtype, "plain")))
  {
    m->data = b;
    unlink(tempfn);
  }
  else {
    STATE state;
    memset(&state, 0, sizeof(state));
    mutt_parse_part(f, b);
    state.fpin = f;  state.fpout = NULL;
    postparse_mime_parts(b, &state, m);
    m->data = b;
    m->temp_files = g_slist_append(m->temp_files, g_strdup(tempfn));
    m->filename = g_strdup(tempfn);
  }
}


grn_mime_part *grn_mime_part_alloc()
{
  grn_mime_part *mp = g_new(grn_mime_part, 1);
  
  mp->size = 0;
  mp->type = NULL;		mp->subtype = NULL;
  mp->enc = NULL;		mp->filename = NULL;
  mp->data = NULL;
  return mp;
}

void grn_mime_part_free(grn_mime_part **mp)
{
  g_return_if_fail(mp != NULL);
  g_return_if_fail(*mp != NULL);
  
  str_free(&((*mp)->type));
  str_free(&((*mp)->subtype));
  str_free(&((*mp)->enc));
  str_free(&((*mp)->filename));
  str_free(&((*mp)->data));
  g_free(*mp);  *mp = NULL;
}

static gchar *mtype2str(BODY *b)
{
  gchar *ret = NULL;

  switch (b->type)
  {
    case TYPEOTHER:
      ret = g_strdup(b->xtype);
      break;
    case TYPEAUDIO:
      ret = g_strdup("audio");
      break;
    case TYPEAPPLICATION:
      ret = g_strdup("application");
      break;
    case TYPEIMAGE:
      ret = g_strdup("image");
      break;
    case TYPEMESSAGE:
      ret = g_strdup("message");
      break;
    case TYPEMODEL:
      ret = g_strdup("model");
      break;
    case TYPEMULTIPART:
      ret = g_strdup("multipart");
      break;
    case TYPETEXT:
      ret = g_strdup("text");
      break;
    case TYPEVIDEO:
      ret = g_strdup("video");
      break;
  }
  return ret;
}

gchar *body_mtype(BODY *b)
{
  gchar *s;
  gchar *ret;
  g_return_val_if_fail(b != NULL, NULL);

  s = mtype2str(b);
  ret = g_strdup_printf("%s/%s", s, b->subtype);
  g_free(s);
  return ret;
}

gchar *menc2str(BODY *b)
{
  gchar *ret = NULL;
  if (! b)  return NULL;

  switch (b->encoding)
  {
    case ENCOTHER:
      ret = g_strdup("other");
      break;
    case ENC7BIT:
      ret = g_strdup("7bit");
      break;
    case ENC8BIT:
      ret = g_strdup("8bit");
      break;
    case ENCQUOTEDPRINTABLE:
      ret = g_strdup("quoted-printable");
      break;
    case ENCBASE64:
      ret = g_strdup("base64");
      break;
    case ENCBINARY:
      ret = g_strdup("binary");
      break;
    case ENCUUENCODED:
      ret = g_strdup("uuencoded");
      break;
  }
  return ret;
}

static gchar *mdisp2str(BODY *b)
{
  gchar *ret = NULL;

  switch (b->disposition)
  {
    case DISPINLINE:
      ret = g_strdup("inline");
      break;
    case DISPATTACH:
      ret = g_strdup("attach");
      break;
    case DISPFORMDATA:
      ret = g_strdup("form data");
      break;
  }
  return ret;
}

GSList *body_to_mime_parts(BODY *b, GSList *l, FILE *fp)
{
  grn_mime_part *mp = NULL;
  GSList *ret = l;
  size_t nread;

  if (! b)  return l;
  mp = grn_mime_part_alloc();
  mp->size = b->length;
  mp->type = mtype2str(b);
  mp->subtype = g_strdup(b->subtype);
  mp->enc = menc2str(b);
  mp->filename = str_copy(b->filename);

  fseek(fp, b->offset, 0);
  if (b->encoding < 3)
  {
    mp->data = g_malloc(mp->size + 1);
    nread = fread(mp->data, 1, mp->size, fp);
    mp->data[nread] = '\0';
  }
  else {
    STATE s;
    char tempfn[64] = "/tmp/grn.XXXXXXX";
    int fd = mkstemp(tempfn);
    glong sz;
    close(fd);

    memset(&s, 0, sizeof(s));
    s.fpin = fp;
    s.fpout = fopen(tempfn, "w+");
    mutt_decode_attachment(b, &s);
    fseek(s.fpout, 0, SEEK_END);  sz = ftell(s.fpout);
    fseek(s.fpout, 0, 0);
    mp->data = g_malloc(sz + 1);
    nread = fread(mp->data, 1, sz, s.fpout);
    mp->data[nread] = '\0';
    fclose(s.fpout);  unlink(tempfn);
  }

  ret = g_slist_append(l, mp);

  ret = body_to_mime_parts(b->parts, ret, fp);
  ret = body_to_mime_parts(b->next, ret, fp);
  return ret;
}



gboolean is_8bit(gchar *s)
{
  gchar *p = s;
  while (*p)
  {
    if ((guchar) *p > 127)  return TRUE;
    p++;
  }
  return FALSE;
}

void add_mime_to_plaintext(t_message *msg)
{
  gchar *s1 = get_charset(), *s2;

  t_msgheaders_add_extra_hdr(msg->mh, "Mime-Version", "1.0");
  s2 = g_strdup_printf("text/plain; charset=%s", s1);
  t_msgheaders_add_extra_hdr(msg->mh, "Content-Type", s2);
  str_free(&s1);  str_free(&s2);

  if (is_8bit(msg->body))
    t_msgheaders_add_extra_hdr(msg->mh, "Content-Transfer-Encoding", "8bit");
  else  t_msgheaders_add_extra_hdr(msg->mh, "Content-Transfer-Encoding", "7bit");
}


void mutt_message(const char *fmt, ...)
{
  va_list arglist;
  gchar *s;
  
  va_start(arglist, fmt);
  s = g_strdup_vprintf(fmt, arglist);
  va_end(arglist);
  grn_appbar_set_status(GRN->appbar, _("Mutt message: %s"), s);
  str_free(&s);
}
void grn_mutt_error(const char *fmt, ...)
{
  va_list arglist;
  gchar *s;
  
  va_start(arglist, fmt);
  s = g_strdup_vprintf(fmt, arglist);
  va_end(arglist);
  grn_appbar_set_status(GRN->appbar, _("Mutt error: %s"), s);
  str_free(&s);
}
void mutt_exit(int code)
{
  g_print("mutt_exit() called, code = %d\n", code);
}
int mutt_yesorno(const char *msg, int def)
{
  g_print("mutt_yesorno() called, msg = %s, def = %d\n", msg, def);
  return 1;
}
void mutt_clear_error(void)
{
  g_print("mutt_clear_error() called\n");
}


void grn_mime_init(void)
{
  mutt_error = grn_mutt_error;
}


static void mp_sel(GtkCList *clist, gint row, gint col, GdkEventButton *ev,
                   GtkWidget *vbox)
{
  BODY *b = (BODY *) gtk_clist_get_row_data(clist, row);
  GtkWidget *btn = gtk_object_get_data(GTK_OBJECT(vbox), KEY_BTN_SAVE);
  GtkWidget *btn1 = gtk_object_get_data(GTK_OBJECT(vbox), KEY_BTN_OPEN);
  GtkWidget *btn2 = gtk_object_get_data(GTK_OBJECT(vbox), KEY_BTN_VIEW);
  GtkWidget *btn3 = gtk_object_get_data(GTK_OBJECT(vbox), KEY_BTN_EXEC);
  gchar *mime_type;
  const gchar *s1;
  if ((! b) || (! btn))  return;
  
  if ((b->type == TYPEMULTIPART) || (b->type == TYPEMESSAGE))
    gtk_widget_set_sensitive(btn, FALSE);
  else  gtk_widget_set_sensitive(btn, TRUE);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_DATA, b);
  
  if (btn3)  gtk_widget_set_sensitive(btn3, TRUE);

  if ((! btn1) || (! btn2))  return;
  mime_type = body_mtype(b);
  s1 = gnome_mime_program(mime_type);
  gtk_widget_set_sensitive(btn1, (s1 != NULL));
  s1 = gnome_mime_get_value(mime_type, "view");
  gtk_widget_set_sensitive(btn2, (s1 != NULL));
  g_free(mime_type);
}

static void mp_save_ok(GtkWidget *w, GtkFileSelection *fsel)
{
  gchar *fn = gtk_file_selection_get_filename(fsel);
  GtkWidget *cb = gtk_object_get_data(GTK_OBJECT(fsel), KEY_BTN);
  t_message *m = gtk_object_get_data(GTK_OBJECT(fsel), KEY_MSG);
  BODY *b = gtk_object_get_data(GTK_OBJECT(fsel), KEY_DATA);
  FILE *fpout;
  
  if (g_file_exists(fn))
  {
    grn_error(_("Already exists"), fn);
    return;
  }

  fpout = fopen(fn, "w");
  if (cb && m && b && m->fp && fpout)
  {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb)))
    {
      STATE s;

      memset(&s, 0, sizeof(s));
      s.fpin = m->fp;  s.fpout = fpout;
      fseek(s.fpin, b->offset, 0);
      mutt_decode_attachment(b, &s);
    }
    else {
      gchar *buf = g_malloc(8192);
      glong len = b->length;

      fseek(m->fp, b->offset, 0);
      while (! feof(m->fp))
      {
	size_t nwritten, nread = fread(buf, 1, MIN(8192, len), m->fp);
	nwritten = fwrite(buf, 1, nread, fpout);
	len -= nwritten;
	if (nread != nwritten)  grn_error(_("I/O problem"), _("Read and written lengths mismatch"));
	if (len <= 0)  break;
      }
      g_free(buf);
    }
    fclose(fpout);
  }
  else {
    if (! fpout)  grn_error(_("The following file can't be opened for writing"), fn);
  }
  gtk_widget_destroy(GTK_WIDGET(fsel));
}
static void mp_save_cancel(GtkWidget *w, GtkWidget *fsel)
{
  gtk_widget_destroy(fsel);
}
static void mp_save(GtkWidget *w, GtkWidget *vbox)
{
  BODY *b = (BODY *) gtk_object_get_data(GTK_OBJECT(vbox), KEY_DATA);
  t_message *m = gtk_object_get_data(GTK_OBJECT(vbox), KEY_MSG);
  GtkFileSelection *fsel;
  GtkWidget *cb;
  if ((! b) || (! m))  return;
  
  fsel = GTK_FILE_SELECTION(gtk_file_selection_new(_("Save MIME part")));
  if (str_check(b->filename))  gtk_file_selection_set_filename(fsel, b->filename);
  cb = gtk_check_button_new_with_label(_("Decode before saving"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), TRUE);
  gtk_box_pack_start(GTK_BOX(fsel->main_vbox), cb, FALSE, FALSE, 2);
  gtk_object_set_data(GTK_OBJECT(fsel), KEY_BTN, cb);
  gtk_signal_connect(GTK_OBJECT(fsel->ok_button), "clicked",
		     GTK_SIGNAL_FUNC(mp_save_ok), fsel);
  gtk_signal_connect(GTK_OBJECT(fsel->cancel_button), "clicked",
		     GTK_SIGNAL_FUNC(mp_save_cancel), fsel);
  gtk_object_set_data(GTK_OBJECT(fsel), KEY_MSG, m);
  gtk_object_set_data(GTK_OBJECT(fsel), KEY_DATA, b);

  gtk_widget_show_all(GTK_WIDGET(fsel));
}

static gchar *mp_temp(BODY *b, t_message *m)
{
  gchar tempfn[64] = "/tmp/grn.XXXXXXX";
  gchar *filename = NULL;
  gint fd = mkstemp(tempfn);
  STATE s;

  filename = g_strdup(tempfn);
  memset(&s, 0, sizeof(s));
  s.fpin = m->fp;
  s.fpout = fdopen(fd, "w+");
  fseek(s.fpin, b->offset, 0);
  mutt_decode_attachment(b, &s);
  fclose(s.fpout);
  m->temp_files = g_slist_append(m->temp_files, g_strdup(filename));
  if (! b->filename)  b->filename = g_strdup(filename);
  return filename;
}

static void mp_open(GtkWidget *w, GtkWidget *vbox)
{
  BODY *b = (BODY *) gtk_object_get_data(GTK_OBJECT(vbox), KEY_DATA);
  t_message *m = gtk_object_get_data(GTK_OBJECT(vbox), KEY_MSG);
  gchar *mime_type, *s;
  const gchar *s1;
  if (! b)  return;
  
  mime_type = body_mtype(b);
  s1 = gnome_mime_program(mime_type);
  if (s1)
  {
    gchar *fn = mp_temp(b, m);
    s = str_subst(s1, "%f", fn);
    if (_mutt_system(s, M_DETACH_PROCESS) == -1)
      grn_error(s, strerror(errno));
    g_free(s);  g_free(fn);
  }
  g_free(mime_type);
}

static void mp_view(GtkWidget *w, GtkWidget *vbox)
{
  BODY *b = (BODY *) gtk_object_get_data(GTK_OBJECT(vbox), KEY_DATA);
  t_message *m = gtk_object_get_data(GTK_OBJECT(vbox), KEY_MSG);
  gchar *mime_type, *s;
  const gchar *s1;
  if (! b)  return;
  
  mime_type = body_mtype(b);
  s1 = gnome_mime_get_value(mime_type, "view");
  if (s1)
  {
    gchar *fn = mp_temp(b, m);
    s = str_subst(s1, "%f", fn);
    if (_mutt_system(s, M_DETACH_PROCESS) == -1)
      grn_error(s, strerror(errno));
    g_free(s);  g_free(fn);
  }
  g_free(mime_type);
}

static void mp_exec_ok(GtkWidget *w, GtkWidget *dlg)
{
  GtkWidget *vbox = gtk_object_get_data(GTK_OBJECT(dlg), KEY_WND);
  BODY *b = (BODY *) gtk_object_get_data(GTK_OBJECT(vbox), KEY_DATA);
  t_message *m = gtk_object_get_data(GTK_OBJECT(vbox), KEY_MSG);
  GtkWidget *en;
  gchar *s, *s1, *fn;
  if (! dlg)  return;

  en = gtk_object_get_data(GTK_OBJECT(dlg), KEY_ENTRY);
  if (! en)  return;

  s1 = gtk_entry_get_text(GTK_ENTRY(en));
  fn = mp_temp(b, m);
  s = str_subst(s1, "%f", fn);
  if (_mutt_system(s, M_DETACH_PROCESS) == -1)  grn_error(s, strerror(errno));
  g_free(s);  g_free(fn);
  gtk_widget_destroy(dlg);
}

static void mp_exec(GtkWidget *w, GtkWidget *vbox)
{
  BODY *b = (BODY *) gtk_object_get_data(GTK_OBJECT(vbox), KEY_DATA);
  GtkWidget *dlg;
  GtkWidget *box, *en, *l;
  if (! b)  return;
  
  grn_lock();
  box = gtk_vbox_new(FALSE, 0);
  l = gtk_label_new(_("Enter command (%f may be used):"));
  gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 3);
  en = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(box), en, TRUE, TRUE, 3);

  dlg = gnome_dialog_new(_("Execute command"), GNOME_STOCK_BUTTON_CANCEL,
                         GNOME_STOCK_BUTTON_OK, NULL);
  gnome_dialog_set_close(GNOME_DIALOG(dlg), TRUE);
  gnome_dialog_editable_enters(GNOME_DIALOG(dlg), GTK_EDITABLE(en));
  gnome_dialog_button_connect(GNOME_DIALOG(dlg), 1,
		     GTK_SIGNAL_FUNC(mp_exec_ok), dlg);
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg)->vbox), box, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(dlg), KEY_ENTRY, en);
  gtk_object_set_data(GTK_OBJECT(dlg), KEY_WND, vbox);
  gtk_widget_show_all(dlg);
  gtk_widget_grab_focus(en);
  grn_unlock();
}


#define NCOLS	5
static GtkWidget *mime_tree_create()
{
  gchar *ttl[NCOLS] = {_("Type"), _("Description"), _("Size"), _("Encoding"), _("Disposition")};
  GtkWidget *vbox, *scrwin, *bbox, *btn;
  GtkCList *clist;
  
  grn_lock();
  vbox = gtk_vbox_new(FALSE, 0);
  
  clist = GTK_CLIST(gtk_ctree_new_with_titles(NCOLS, 0, ttl));
  gtk_clist_set_column_justification(clist, 2, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_selection_mode(clist, GTK_SELECTION_BROWSE);
  gtk_clist_column_titles_passive(clist);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_CTREE, clist);
  gtk_signal_connect(GTK_OBJECT(clist), "select_row",
		     GTK_SIGNAL_FUNC(mp_sel), vbox);
  
  scrwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrwin), GTK_WIDGET(clist));
  gtk_box_pack_start(GTK_BOX(vbox), scrwin, TRUE, TRUE, 0);

  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), gnome_preferences_get_button_layout());
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), GNOME_PAD);
  gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 5);
  gtk_widget_set_sensitive(bbox, TRUE);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_BTN, bbox);
  
  btn = gnome_pixmap_button(gnome_stock_pixmap_widget(NULL, GNOME_STOCK_PIXMAP_SAVE),
                            _("Save..."));
  gtk_box_pack_start(GTK_BOX(bbox), btn, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(btn), "clicked",
                     GTK_SIGNAL_FUNC(mp_save), vbox);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_BTN_SAVE, btn);
  gtk_widget_set_sensitive(btn, FALSE);

  btn = gtk_button_new_with_label(_("Open"));
  gtk_box_pack_start(GTK_BOX(bbox), btn, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_BTN_OPEN, btn);
  gtk_signal_connect(GTK_OBJECT(btn), "clicked",
		     GTK_SIGNAL_FUNC(mp_open), vbox);
  gtk_widget_set_sensitive(btn, FALSE);

  btn = gtk_button_new_with_label(_("View"));
  gtk_box_pack_start(GTK_BOX(bbox), btn, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_BTN_VIEW, btn);
  gtk_signal_connect(GTK_OBJECT(btn), "clicked",
		     GTK_SIGNAL_FUNC(mp_view), vbox);
  gtk_widget_set_sensitive(btn, FALSE);

  btn = gtk_button_new_with_label(_("Execute..."));
  gtk_box_pack_start(GTK_BOX(bbox), btn, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_BTN_EXEC, btn);
  gtk_signal_connect(GTK_OBJECT(btn), "clicked",
		     GTK_SIGNAL_FUNC(mp_exec), vbox);
  gtk_widget_set_sensitive(btn, FALSE);

  grn_unlock();
  return vbox;
}

static gboolean mime_tree_dlg_delete(GtkWidget *w)
{
  grn_save_ws(7, w);
  return FALSE;
}
static void btn_event(GtkWidget *btn, GtkWidget *w)
{
  mime_tree_dlg_delete(w);
}
static GtkWidget *mime_tree_dlg_create(t_message *m)
{
  GtkWidget *dlg, *vbox;
  
  grn_lock();
  dlg = gnome_dialog_new(_("MIME structure"), GNOME_STOCK_BUTTON_CLOSE, NULL);
  gnome_dialog_set_close(GNOME_DIALOG(dlg), TRUE);
  gtk_window_set_policy(GTK_WINDOW(dlg), FALSE, TRUE, FALSE);
  gtk_window_set_default_size(GTK_WINDOW(dlg), grn_prefs.ws[7].width, grn_prefs.ws[7].height);
  gtk_widget_set_uposition(dlg, grn_prefs.ws[7].x, grn_prefs.ws[7].y);
  gtk_signal_connect(GTK_OBJECT(dlg), "delete_event",
                     GTK_SIGNAL_FUNC(mime_tree_dlg_delete), NULL);
  gnome_dialog_button_connect(GNOME_DIALOG(dlg), 0,
		     GTK_SIGNAL_FUNC(btn_event), dlg);
  grn_unlock();
  vbox = mime_tree_create();
  grn_lock();
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg)->vbox), vbox, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(dlg), KEY_WND, vbox);
  gtk_object_set_data(GTK_OBJECT(vbox), KEY_MSG, m);
  gtk_widget_show_all(dlg);
  grn_unlock();
  return dlg;
}

static void show_mime_part(BODY *b, GtkCTree *ctree, GtkCTreeNode *parent)
{
  gchar *s;
  gchar *row[NCOLS];
  GtkCTreeNode *node;
  gint i;
  if (! b)  return;
  if (! ctree)  return;

  s = mtype2str(b);
  row[0] = g_strdup_printf("%s/%s", s, b->subtype);
  str_free(&s);
  row[1] = str_copy(b->description);
  row[2] = g_strdup_printf("%ld", b->length);
  row[3] = menc2str(b);
  row[4] = mdisp2str(b);
  node = gtk_ctree_insert_node(ctree, parent, NULL, row, 0,
                               NULL, NULL, NULL, NULL, FALSE, TRUE);
  gtk_ctree_node_set_row_data(ctree, node, b);
  for (i=0; i<NCOLS; i++)  str_free(&row[i]);

  show_mime_part(b->parts, ctree, node);
  show_mime_part(b->next, ctree, parent);
}
#undef NCOLS

typedef struct mt_data
{
  GtkWidget **dlg;
  GtkWidget *btn;
  t_message *m;
} mt_data;

static void mt_destroy(GtkWidget *w, mt_data *mtd)
{
  *(mtd->dlg) = NULL;
  gtk_widget_set_sensitive(mtd->btn, TRUE);
  t_message_destroy(mtd->m);
  g_free(mtd);
}
static gboolean show_mime_tree(GtkWidget *w, t_message *m)
{
  static GtkWidget *dlg = NULL, *mtw = NULL;
  mt_data *mtd;
  GtkCTree *ctree = NULL;
  
  if (dlg)  return FALSE;
  dlg = mime_tree_dlg_create(m);
  mtw = gtk_object_get_data(GTK_OBJECT(dlg), KEY_WND);
  if (mtw)  ctree = gtk_object_get_data(GTK_OBJECT(mtw), KEY_CTREE);
  if (! ctree)  return FALSE;
  gtk_widget_set_sensitive(w, FALSE);
  
  mtd = g_new(mt_data, 1);
  mtd->dlg = &dlg;  mtd->btn = w;  mtd->m = m;
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                     GTK_SIGNAL_FUNC(mt_destroy), mtd);
  show_mime_part((BODY *) m->data, ctree, NULL);
  gtk_clist_columns_autosize(GTK_CLIST(ctree));
  gtk_widget_grab_focus(GTK_WIDGET(ctree));
  return TRUE;
}

void evt_mime_view(GtkWidget *w)
{
  t_message *m;
  static t_message *m1;

  if (! GRN->msgwin)  return;
  m = gtk_object_get_data(GTK_OBJECT(GRN->msgwin), KEY_MSG);
  if (! m)  return;
  m1 = t_message_copy(m);
  grn_mime_parse_message(m1);
  if (! show_mime_tree(w, m1))  t_message_free(&m1);
}
