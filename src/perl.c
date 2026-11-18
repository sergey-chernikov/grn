/*
 * perl.c - helper functions for Perl interface
 *
 * $Id: perl.c,v 1.28 2000/07/19 06:26:21 sc Exp $
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
#undef PACKAGE
#ifdef USE_PERL
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#if 0
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#undef PACKAGE
#undef _
#include "grn_types.h"
#include "grn_util.h"
#include "grn_perl.h"
#include "grn_msgview.h"
#include "grn_config.h"
#include "grn_misc.h"


extern grn_session *GRN;
extern void xs_init(void);


static PerlInterpreter *my_perl = NULL;

GSList *perl_hooks;
GSList *perl_commands;
GSList *perl_files = NULL;

XS(XS_GRN_add_hook);
XS(XS_GRN_add_command);
XS(XS_GRN_html_escape);
XS(XS_GRN_subst_vars);
XS(XS_GRN_get_config);
XS(XS_GRN_get_font_pal);
XS(XS_GRN_get_color_pal);
XS(XS_GRN_mew_text_insert);
XS(XS_GRN_mew_text_select);
XS(XS_GRN_mew_text_get_selection);
XS(XS_GRN_mew_text_get);
XS(XS_GRN_mew_text_delete);
XS(XS_GRN_entry_text_get);
XS(XS_GRN_entry_text_set);
XS(XS_GRN_entry_text_delete);
XS(XS_GRN_error_dialog);


static gchar *escape_quotes(gchar *src)
{
  gchar *tmp_buf;
  gchar *i, *j;
  
  tmp_buf = g_malloc(strlen(src)*2 + 1);
  for (i=src, j=tmp_buf; *i; i++, j++)
  {
    if (*i == '\'' || *i == '\\')  *j++ = '\\';
    *j = *i;
  }
  *j = '\0';
  return tmp_buf;
}

static glong execute_perl(gchar *function, gchar *args)
{
  gchar *perl_cmd = NULL, *eargs;
  SV *ret;
  
  eargs = escape_quotes(args);
  perl_cmd = g_strdup_printf("&%s('%s')", function, eargs);
  str_free(&eargs);
  ret = perl_eval_pv(perl_cmd, TRUE);
  str_free(&perl_cmd);
  return SvIV(ret);
}

int perl_load_file(gchar *script_name)
{
  glong retval;
  retval = execute_perl("load_file", script_name);
  perl_files = g_slist_append(perl_files, g_strdup(script_name));
  return retval;
}
void perl_autoload_file(gchar *script_name)
{
  perl_load_file(script_name);
}

void perl_init(gboolean autoload)
{
  gchar *perl_args[] = {"", "-e", "0"};
  gchar pl_load_file[]=	"sub load_file()\n"
                	"{ (my $file_name) = @_;\n"
			"open(FH,$file_name) or return 2;\n"
			"local($/)=undef; $file=<FH>;\n"
			"close(FH); eval($file);\n"
			"eval($file) if $@;\n"
			"return 1 if $@;  return 0; }";

  my_perl = perl_alloc();
  if (! my_perl)
  {
    g_warning(_("Not enough memory!"));
    return;
  }
  perl_construct(my_perl);
  if (perl_parse(my_perl, xs_init, 3, perl_args, NULL))
  {
    g_warning(_("Error loading Perl"));
    return;
  }
  perl_eval_pv(pl_load_file, TRUE);
  perl_hooks = NULL;  perl_commands = NULL;
  newXS("GRN::add_hook", XS_GRN_add_hook, "GRN");
  newXS("GRN::add_command", XS_GRN_add_command, "GRN");
  newXS("GRN::html_escape", XS_GRN_html_escape, "GRN");
  newXS("GRN::subst_vars", XS_GRN_subst_vars, "GRN");
  newXS("GRN::get_config", XS_GRN_get_config, "GRN");
  newXS("GRN::get_font_pal", XS_GRN_get_font_pal, "GRN");
  newXS("GRN::get_color_pal", XS_GRN_get_color_pal, "GRN");
  newXS("GRN::mew_text_insert", XS_GRN_mew_text_insert, "GRN");
  newXS("GRN::mew_text_select", XS_GRN_mew_text_select, "GRN");
  newXS("GRN::mew_text_get_selection", XS_GRN_mew_text_get_selection, "GRN");
  newXS("GRN::mew_text_get", XS_GRN_mew_text_get, "GRN");
  newXS("GRN::mew_text_delete", XS_GRN_mew_text_delete, "GRN");
  newXS("GRN::entry_text_get", XS_GRN_entry_text_get, "GRN");
  newXS("GRN::entry_text_delete", XS_GRN_entry_text_delete, "GRN");
  newXS("GRN::entry_text_set", XS_GRN_entry_text_set, "GRN");
  newXS("GRN::error_dialog", XS_GRN_error_dialog, "GRN");
  
  if (autoload)
  {
    gchar *s = gnome_util_prepend_user_home(".grn/hooks/Perl");
    files_foreach(s, perl_autoload_file);
    str_free(&s);
    s = gnome_util_prepend_user_home(".grn/commands/Perl");
    files_foreach(s, perl_autoload_file);
    str_free(&s);
  }
}

void perl_end(void)
{
  while (perl_commands)
  {
    perl_command_handler *pch = perl_commands->data;
    
    perl_commands = g_slist_remove(perl_commands, pch);
    str_free(&(pch->command_name));  str_free(&(pch->handler_name));
    g_free(pch);
  }
  while (perl_hooks)
  {
    perl_hook_handler *phh = perl_hooks->data;
    
    perl_hooks = g_slist_remove(perl_hooks, phh);
    str_free(&(phh->hook_name));  str_free(&(phh->handler_name));
    g_free(phh);
  }
  while (perl_files)
  {
    gchar *s = perl_files->data;
    perl_files = g_slist_remove(perl_files, s);
    str_free(&s);
  }
  
  perl_destruct(my_perl);
  perl_free(my_perl);  my_perl = NULL;
}

void grn_perl_restart(void)
{
  GSList *l, *sl_pf = NULL;

  for (l=perl_files; l; l=l->next)  sl_pf = g_slist_append(sl_pf, g_strdup(l->data));
  if (my_perl)  perl_end();
  perl_init(FALSE);

  while (sl_pf)
  {
    gchar *s = sl_pf->data;
    perl_load_file(s);
    sl_pf = g_slist_remove(sl_pf, s);
    str_free(&s);
  }
  g_slist_free(sl_pf);
}

GSList *grn_perl_list(void)
{
  GSList *ret=NULL, *l;
  gchar **row;

  for (l=perl_commands; l; l=l->next)
  {
    perl_command_handler *pch = l->data;
    
    row = g_malloc(sizeof(gpointer));
    row[0] = g_strdup(_("command"));
    row[1] = g_strdup(pch->command_name);
    row[2] = g_strdup(pch->handler_name);
    ret = g_slist_prepend(ret, row);
  }
  for (l=perl_hooks; l; l=l->next)
  {
    perl_hook_handler *phh = l->data;
    
    row = g_malloc(sizeof(gpointer));
    row[0] = g_strdup(_("hook"));
    row[1] = g_strdup(phh->hook_name);
    row[2] = g_strdup(phh->handler_name);
    ret = g_slist_prepend(ret, row);
  }
  for (l=perl_files; l; l=l->next)
  {
    row = g_malloc(sizeof(gpointer));
    row[0] = g_strdup(_("file"));
    row[1] = g_strdup(l->data);
    row[2] = NULL;
    ret = g_slist_prepend(ret, row);
  }
  ret = g_slist_reverse(ret);
  return ret;
}


/*
  0	none
  1	t_msgheaders
  2	t_message
  3	grn_mime_part
 */
static gint get_data_type(gchar *hookname)
{
  if (! strcmp(hookname, "GetScore"))  return 1;
  if (! strcmp(hookname, "PreShow"))  return 2;
  if (! strcmp(hookname, "TemplateNew"))  return 2;
  if (! strcmp(hookname, "TemplateMNew"))  return 2;
  if (! strcmp(hookname, "TemplateReply"))  return 2;
  if (! strcmp(hookname, "TemplateMReply"))  return 2;
  if (! strcmp(hookname, "TemplateForward"))  return 2;
  if (! strcmp(hookname, "TemplateMForward"))  return 2;
  if (! strcmp(hookname, "Quote"))  return 2;
  if (! strcmp(hookname, "PrePost"))  return 2;
  if (! strcmp(hookname, "PreSend"))  return 2;
  if (! strcmp(hookname, "PreShowHeader"))  return 2;
  if (! strcmp(hookname, "PreShowFooter"))  return 2;
  if (! strcmp(hookname, "PreShowPart"))  return 3;
  return 0;
}

static gint get_data1_type(gchar *hookname)
{
  if (! strcmp(hookname, "GetScore"))  return 0;
  if (! strcmp(hookname, "PreShow"))  return 0;
  if (! strcmp(hookname, "TemplateNew"))  return 0;
  if (! strcmp(hookname, "TemplateMNew"))  return 0;
  if (! strcmp(hookname, "TemplateReply"))  return 2;
  if (! strcmp(hookname, "TemplateMReply"))  return 2;
  if (! strcmp(hookname, "TemplateForward"))  return 2;
  if (! strcmp(hookname, "TemplateMForward"))  return 2;
  if (! strcmp(hookname, "Quote"))  return 2;
  if (! strcmp(hookname, "PrePost"))  return 0;
  if (! strcmp(hookname, "PreSend"))  return 0;
  if (! strcmp(hookname, "PreShowHeader"))  return 0;
  if (! strcmp(hookname, "PreShowFooter"))  return 0;
  if (! strcmp(hookname, "PreShowPart"))  return 0;
  return 0;
}

/*
 1	param in global scope
 2	param as an argument
 */
static gint get_arg_type(gchar *hookname)
{
  if (! strcmp(hookname, "GetScore"))  return 1;
  if (! strcmp(hookname, "PreShow"))  return 1;
  if (! strcmp(hookname, "TemplateNew"))  return 1;
  if (! strcmp(hookname, "TemplateMNew"))  return 1;
  if (! strcmp(hookname, "TemplateReply"))  return 1;
  if (! strcmp(hookname, "TemplateMReply"))  return 1;
  if (! strcmp(hookname, "TemplateForward"))  return 1;
  if (! strcmp(hookname, "TemplateMForward"))  return 1;
  if (! strcmp(hookname, "Quote"))  return 1;
  if (! strcmp(hookname, "PrePost"))  return 1;
  if (! strcmp(hookname, "PreSend"))  return 1;
  if (! strcmp(hookname, "PreShowHeader"))  return 1;
  if (! strcmp(hookname, "PreShowFooter"))  return 1;
  if (! strcmp(hookname, "PreShowPart"))  return 1;
  return 0;
}

/*
 0	none
 1	param in global scope
 2	param as an argument
 */
static gint get_arg1_type(gchar *hookname)
{
  if (! strcmp(hookname, "GetScore"))  return 0;
  if (! strcmp(hookname, "PreShow"))  return 0;
  if (! strcmp(hookname, "TemplateNew"))  return 0;
  if (! strcmp(hookname, "TemplateMNew"))  return 0;
  if (! strcmp(hookname, "TemplateReply"))  return 1;
  if (! strcmp(hookname, "TemplateMReply"))  return 1;
  if (! strcmp(hookname, "TemplateForward"))  return 1;
  if (! strcmp(hookname, "TemplateMForward"))  return 1;
  if (! strcmp(hookname, "Quote"))  return 1;
  if (! strcmp(hookname, "PrePost"))  return 0;
  if (! strcmp(hookname, "PreSend"))  return 0;
  if (! strcmp(hookname, "PreShowHeader"))  return 0;
  if (! strcmp(hookname, "PreShowFooter"))  return 0;
  if (! strcmp(hookname, "PreShowPart"))  return 0;
  return 0;
}

static gboolean read_only_data(gchar *hookname)
{
  if (! strcmp(hookname, "GetScore"))  return TRUE;
  return FALSE;
}

/*static gboolean read_only_data1(gchar *hookname)
{
  return TRUE;
}*/

static void put_str_to_hash(HV *hash, gchar *key, gchar *src)
{
  if (! src)  return;
  hv_store(hash, key, strlen(key), newSVpv(src, 0), 0);
}

static void put_int_to_hash(HV *hash, gchar *key, glong src)
{
  hv_store(hash, key, strlen(key), newSViv(src), 0);
}

static void rm_from_hash(HV *hash, gchar *key)
{
  hv_delete(hash, key, strlen(key), G_DISCARD);
}

static gchar *get_str_from_hash(HV *hash, gchar *key)
{
  SV **sv;
  gchar *tmp;
  guint len;
  
  sv = hv_fetch(hash, key, strlen(key), 0);
  if (! sv)  return NULL;
  SvPV(*sv, len);
  if (len <= 0)  return NULL;
  tmp = (gchar *) g_malloc(len + 1);
  strcpy(tmp, SvPV(*sv, len));
  tmp[len] = '\0';
  return tmp;
}

static glong get_int_from_hash(HV *hash, gchar *key)
{
  SV **sv;
  
  sv = hv_fetch(hash, key, strlen(key), 0);
  if (sv)  return SvIV(*sv);
  return 0;
}

static void fill_extra_hdr(gchar *key, gchar *value, HV *mht)
{
  if (! value)  return;
  put_str_to_hash(mht, key, value);
}
static void fill_mhdrs_hash(HV *mht, t_msgheaders *mh)
{
  if (! mh)  return;
  put_int_to_hash(mht, "Number", mh->number);
  put_int_to_hash(mht, "Lines", mh->lines);
  put_int_to_hash(mht, "Size", mh->size);
  put_int_to_hash(mht, "ParsedDate", mh->parsed_date);
  put_str_to_hash(mht, "From", mh->from);
  put_str_to_hash(mht, "Subject", mh->subject);
  put_str_to_hash(mht, "Date", mh->date);
  put_str_to_hash(mht, "Message-ID", mh->id);
  put_str_to_hash(mht, "References", mh->ref);
  put_str_to_hash(mht, "Xref", mh->xref);
  put_str_to_hash(mht, "Path", mh->path);
  if (mh->newsgroups)  put_str_to_hash(mht, "Newsgroups", mh->newsgroups);
  else if (mh->grp)  put_str_to_hash(mht, "Newsgroups", mh->grp->name);
  put_str_to_hash(mht, "To", mh->to);
  if (mh->grp)  put_str_to_hash(mht, "Group", mh->grp->name);
  if (mh->extra_hdrs)
    g_hash_table_foreach(mh->extra_hdrs, (GHFunc) fill_extra_hdr, mht);
}

static void fill_msg_hash(HV *mht, t_message *m)
{
  if (! m)  return;
  fill_mhdrs_hash(mht, m->mh);
  put_str_to_hash(mht, "body", m->body);
  put_int_to_hash(mht, "nChildren", m->nchildren);
  put_int_to_hash(mht, "tagged", m->tagged);
}

static void fill_mp_hash(HV *mph, grn_mime_part *mp)
{
  if (! mp)  return;
  put_int_to_hash(mph, "size", mp->size);
  put_str_to_hash(mph, "type", mp->type);
  put_str_to_hash(mph, "subtype", mp->subtype);
  put_str_to_hash(mph, "encoding", mp->enc);
  put_str_to_hash(mph, "filename", mp->filename);
  put_str_to_hash(mph, "data", mp->data);
}

static void fill_mhdrs_from_hash(t_msgheaders *mh, HV *mht)
{
  HE *iter;
  if (! mh)  return;
  
  mh->lines = get_int_from_hash(mht, "Lines");
  rm_from_hash(mht, "Lines");
  mh->size = get_int_from_hash(mht, "Size");
  rm_from_hash(mht, "Size");
  mh->parsed_date = get_int_from_hash(mht, "ParsedDate");
  rm_from_hash(mht, "ParsedDate");
  str_free(&(mh->from));
  mh->from = get_str_from_hash(mht, "From");
  rm_from_hash(mht, "From");
  str_free(&(mh->subject));
  mh->subject = get_str_from_hash(mht, "Subject");
  rm_from_hash(mht, "Subject");
  str_free(&(mh->date));
  mh->date = get_str_from_hash(mht, "Date");
  rm_from_hash(mht, "Date");
  str_free(&(mh->id));
  mh->id = get_str_from_hash(mht, "Message-ID");
  rm_from_hash(mht, "Message-ID");
  str_free(&(mh->ref));
  mh->ref = get_str_from_hash(mht, "References");
  rm_from_hash(mht, "References");
  str_free(&(mh->newsgroups));
  mh->newsgroups = get_str_from_hash(mht, "Newsgroups");
  rm_from_hash(mht, "Newsgroups");
  str_free(&(mh->to));
  mh->to = get_str_from_hash(mht, "To");
  rm_from_hash(mht, "To");
  str_free(&(mh->path));
  mh->path = get_str_from_hash(mht, "Path");
  rm_from_hash(mht, "Path");
  rm_from_hash(mht, "Xref");
  rm_from_hash(mht, "Group");
  rm_from_hash(mht, "Number");
  
  t_msgheaders_rm_extra_hdrs(mh);
  mh->extra_hdrs = g_hash_table_new(g_str_hash, g_str_equal);
  hv_iterinit(mht);
  while ((iter = hv_iternext(mht)))
  {
    I32 retlen;
    gchar *key = g_strdup(hv_iterkey(iter, &retlen));
    gchar *value;
    if (str_check(key))
    {
      value = get_str_from_hash(mht, key);
      g_hash_table_insert(mh->extra_hdrs, key, value);
    }
  }
}

static void fill_msg_from_hash(t_message *m, HV *mht)
{
  if (! m)  return;
  
  str_free(&(m->body));
  m->body = get_str_from_hash(mht, "body");
  rm_from_hash(mht, "body");
  m->pos = get_int_from_hash(mht, "pos");
  rm_from_hash(mht, "pos");
  m->tagged = get_int_from_hash(mht, "tagged");
  rm_from_hash(mht, "tagged");
  rm_from_hash(mht, "nChildren");
  fill_mhdrs_from_hash(m->mh, mht);
}

static void fill_mp_from_hash(grn_mime_part *mp, HV *mph)
{
  if (! mp)  return;

  mp->size = get_int_from_hash(mph, "size");
  str_free(&(mp->type));
  mp->type = get_str_from_hash(mph, "type");
  str_free(&(mp->subtype));
  mp->subtype = get_str_from_hash(mph, "subtype");
  str_free(&(mp->enc));
  mp->enc = get_str_from_hash(mph, "encoding");
  str_free(&(mp->filename));
  mp->filename = get_str_from_hash(mph, "filename");
  str_free(&(mp->data));
  mp->data = get_str_from_hash(mph, "data");
}


glong perl_hook_run(gchar *hookname, grn_session *sess, gpointer data, gpointer data1)
{
  dSP;
  HV *mht=NULL, *mht1=NULL;
  GSList *handler;
  perl_hook_handler *phh;
  gint data_type, data1_type, arg_type, arg1_type;
  glong ret = 0;
  gchar *htname = NULL;
  
  arg_type = get_arg_type(hookname);
  data_type = get_data_type(hookname);
  arg1_type = get_arg1_type(hookname);
  data1_type = get_data1_type(hookname);

  if (data_type == 3)  htname = g_strdup("MPART");
  else  htname = g_strdup("MSG");
  if (arg_type == 1)  mht = perl_get_hv(htname, TRUE);
  else  mht = newHV();
  str_free(&htname);

  if (data_type == 3)  htname = g_strdup("MPART1");
  else  htname = g_strdup("MSG1");
  if (arg1_type == 1)  mht1 = perl_get_hv(htname, TRUE);
  else if (arg1_type == 2)  mht1 = newHV();
  str_free(&htname);

  if ((data_type == 1) || (data_type == 2))
  {
    if (data_type == 1)  fill_mhdrs_hash(mht, (t_msgheaders *) data);
    else if (data_type == 2)  fill_msg_hash(mht, (t_message *) data);
  }
  else if (data_type == 3)  fill_mp_hash(mht, (grn_mime_part *) data);

  if ((data1_type == 1) || (data1_type == 2))
  {
    if (data1_type == 1)  fill_mhdrs_hash(mht1, (t_msgheaders *) data1);
    else if (data1_type == 2)  fill_msg_hash(mht1, (t_message *) data1);
  }
  else if (data1_type == 3)  fill_mp_hash(mht, (grn_mime_part *) data1);

  for (handler=perl_hooks; handler; handler=handler->next)
  {
    phh = (perl_hook_handler *) handler->data;
    if (! strcmp(phh->hook_name, hookname))
    {
      if (arg_type >= 2)
      {
        ENTER;
	SAVETMPS;
	PUSHMARK(SP);
//	XPUSHs(sv_2mortal(newSViv(123456789)));
//	XPUSHs(sv_2mortal((SV *) mht));
	PUTBACK;
      }
      if (arg_type <= 1)  ret = execute_perl(phh->handler_name, "");
      if (arg_type >= 2)
      {
        ret = perl_call_pv(phh->handler_name, G_SCALAR);
        SPAGAIN;
	if (ret == 1)  ret = POPi;
	else  ret = 0;
        PUTBACK;
	FREETMPS;
	LEAVE;
      }
      if (ret > 0)
      {
        if (! read_only_data(hookname))
        {
          if (data_type == 1)  fill_mhdrs_from_hash((t_msgheaders *) data, mht);
	  else if (data_type == 2)  fill_msg_from_hash((t_message *) data, mht);
	  else if (data_type == 3)  fill_mp_from_hash((grn_mime_part *) data, mht);
        }
        hv_undef(mht);	hv_undef(mht1);
        return ret;
      }
    }
  }
  hv_undef(mht);  hv_undef(mht1);
  if (ret)  return ret;
  return (-1);
}


glong perl_command_run(gchar *cmd_name, grn_session *sess, gpointer data)
{
//  dSP;
  GSList *handler;
  perl_command_handler *pch;
  glong ret = 0;
  
  for (handler=perl_commands; handler; handler=handler->next)
  {
    pch = (perl_command_handler *) handler->data;
    if (! strcmp(pch->handler_name, cmd_name))
      ret = execute_perl(pch->handler_name, "");
  }
  if (ret)  return ret;
  return (-1);
}


XS(XS_GRN_add_hook)
{
  int junk;
  perl_hook_handler *handler;
  dXSARGS;
  items = 0;
  
  handler = g_malloc(sizeof(perl_hook_handler));
  handler->hook_name = g_strdup(SvPV(ST(0), junk));
  handler->handler_name = g_strdup(SvPV(ST(1), junk));
  perl_hooks = g_slist_append(perl_hooks, handler);
  XSRETURN_EMPTY;
}

XS(XS_GRN_add_command)
{
  int junk;
  perl_command_handler *handler;
  dXSARGS;
  items = 0;
  
  handler = g_malloc(sizeof(perl_command_handler));
  handler->context = SvIV(ST(0));
  handler->command_name = g_strdup(SvPV(ST(1), junk));
  handler->handler_name = g_strdup(SvPV(ST(2), junk));
  perl_commands = g_slist_append(perl_commands, handler);
  XSRETURN_EMPTY;
}

XS(XS_GRN_html_escape)
{
  int junk;
  gchar *s;
  dXSARGS;
  items = 0;

  s = html_escape(SvPV(ST(0), junk));
  XST_mPV(0, s);
  str_free(&s);
  XSRETURN(1);
}

XS(XS_GRN_subst_vars)
{
  int junk;
  gchar *s;
  dXSARGS;
  items = 0;
  
  s = g_strdup(SvPV(ST(0), junk));
  grn_subst_vars(&s);
  XST_mPV(0, s);
  str_free(&s);
  XSRETURN(1);
}

XS(XS_GRN_get_config)
{
  int junk;
  gchar *s, *buf;
  dXSARGS;
  items = 0;
  
  s = SvPV(ST(0), junk);
  if ((! g_strcasecmp(s, "ver")) || (! g_strcasecmp(s, "version")))
    XST_mPV(0, VERSION);
  else if (! g_strcasecmp(s, "srv_autoconnect"))  XST_mIV(0, grn_prefs.srv_autoconnect);
  else if (! g_strcasecmp(s, "scripts_autoload"))  XST_mIV(0, grn_prefs.scripts_autoload);
  else if (! g_strcasecmp(s, "server"))  XST_mPV(0, grn_prefs.nntp_server);
  else if (! g_strcasecmp(s, "name"))  XST_mPV(0, grn_prefs.name);
  else if ((! g_strcasecmp(s, "email")) || (! g_strcasecmp(s, "e-mail")))
    XST_mPV(0, grn_prefs.email);
  else if ((! g_strcasecmp(s, "replyto")) || (! g_strcasecmp(s, "reply-to")))
    XST_mPV(0, grn_prefs.replyto);
  else if (! g_strcasecmp(s, "organization"))  XST_mPV(0, grn_prefs.org);
  else if (! g_strcasecmp(s, "re_quoting"))  XST_mPV(0, grn_prefs.re_quoting);
  else if (! g_strcasecmp(s, "font0"))  XST_mPV(0, grn_prefs.fonts[0]);
  else if (! g_strcasecmp(s, "font1"))  XST_mPV(0, grn_prefs.fonts[1]);
  else if (! g_strcasecmp(s, "font2"))  XST_mPV(0, grn_prefs.fonts[2]);
  else if (! g_strcasecmp(s, "font3"))  XST_mPV(0, grn_prefs.fonts[3]);
  else if (! g_strcasecmp(s, "font4"))  XST_mPV(0, grn_prefs.fonts[4]);
  else if (! g_strcasecmp(s, "fgcolor0"))
  {
    buf = c2s(grn_prefs.fg_colors[0]);  XST_mPV(0, buf);  str_free(&buf);
  }
  else if (! g_strcasecmp(s, "fgcolor1"))
  {
    buf = c2s(grn_prefs.fg_colors[1]);  XST_mPV(0, buf);  str_free(&buf);
  }
  else if (! g_strcasecmp(s, "fgcolor2"))
  {
    buf = c2s(grn_prefs.fg_colors[2]);  XST_mPV(0, buf);  str_free(&buf);
  }
  else if (! g_strcasecmp(s, "bgcolor0"))
  {
    buf = c2s(grn_prefs.bg_colors[0]);  XST_mPV(0, buf);  str_free(&buf);
  }
  else if (! g_strcasecmp(s, "bgcolor1"))
  {
    buf = c2s(grn_prefs.bg_colors[1]);  XST_mPV(0, buf);  str_free(&buf);
  }
  else if (! g_strcasecmp(s, "bgcolor2"))
  {
    buf = c2s(grn_prefs.bg_colors[2]);  XST_mPV(0, buf);  str_free(&buf);
  }
  else if (! g_strcasecmp(s, "cfm_group_read"))  XST_mIV(0, grn_prefs.cfms[0]);
  else if (! g_strcasecmp(s, "cfm_group_unread")) XST_mIV(0, grn_prefs.cfms[1]);
  else if (! g_strcasecmp(s, "cfm_thread_read"))  XST_mIV(0, grn_prefs.cfms[2]);
  else if (! g_strcasecmp(s, "cfm_thread_unread"))  XST_mIV(0, grn_prefs.cfms[3]);
  else if (! g_strcasecmp(s, "cfm_abort_posting"))  XST_mIV(0, grn_prefs.cfms[4]);
  else if (! g_strcasecmp(s, "cfm_show_warnings"))  XST_mIV(0, grn_prefs.cfms[5]);
  else if (! g_strcasecmp(s, "cfm_show_messages"))  XST_mIV(0, grn_prefs.cfms[6]);
  else XST_mPV(0, "");
  XSRETURN(1);
}

XS(XS_GRN_get_font_pal)
{
  gint iarg, nmax;
  dXSARGS;
  items = 0;
  
  iarg = SvIV(ST(0));
  nmax = g_slist_length(grn_prefs.fontpal);
  if (iarg > nmax)  iarg = nmax;
  if (iarg < 1)  iarg = 1;
  XST_mPV(0, g_slist_nth_data(grn_prefs.fontpal, iarg - 1));
  XSRETURN(1);
}

XS(XS_GRN_get_color_pal)
{
  gint iarg, nmax;
  GdkColor *clr;
  gchar *buf;
  dXSARGS;
  items = 0;
  
  iarg = SvIV(ST(0));
  nmax = g_slist_length(grn_prefs.colorpal);
  if (iarg > nmax)  iarg = nmax;
  if (iarg < 1)  iarg = 1;
  clr = (GdkColor *) g_slist_nth_data(grn_prefs.colorpal, iarg - 1);
  buf = c2s(*clr);  XST_mPV(0, buf);  str_free(&buf);
  XSRETURN(1);
}


XS(XS_GRN_mew_text_insert)
{
  int junk;
  gchar *s;
  GtkWidget *vbox, *text = NULL;
  dXSARGS;
  items = 0;

  if (! GRN->focused)  XSRETURN_EMPTY;
  s = SvPV(ST(0), junk);
  vbox = gtk_object_get_data(GTK_OBJECT(GRN->focused), KEY_MSGPOST);
  if (vbox)  text = gtk_object_get_data(GTK_OBJECT(vbox), KEY_TXT_BODY);
  if ((! text) || (! GTK_IS_TEXT(text)))  XSRETURN_EMPTY;
  gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL, s, strlen(s));
  XSRETURN_EMPTY;
}

XS(XS_GRN_mew_text_select)
{
  gint start, end;
  GtkWidget *vbox, *text = NULL;
  dXSARGS;
  items = 0;

  if (! GRN->focused)  XSRETURN_EMPTY;
  start = SvIV(ST(0));  end = SvIV(ST(1));
  vbox = gtk_object_get_data(GTK_OBJECT(GRN->focused), KEY_MSGPOST);
  if (vbox)  text = gtk_object_get_data(GTK_OBJECT(vbox), KEY_TXT_BODY);
  if ((! text) || (! GTK_IS_TEXT(text)))  XSRETURN_EMPTY;
  gtk_editable_select_region(GTK_EDITABLE(text), start, end);
  XSRETURN_EMPTY;
}

XS(XS_GRN_mew_text_get_selection)
{
  GtkWidget *vbox, *text = NULL;
  dXSARGS;
  items = 0;

  if (! GRN->focused)  XSRETURN_EMPTY;
  vbox = gtk_object_get_data(GTK_OBJECT(GRN->focused), KEY_MSGPOST);
  if (vbox)  text = gtk_object_get_data(GTK_OBJECT(vbox), KEY_TXT_BODY);
  if ((! text) || (! GTK_IS_TEXT(text)))  XSRETURN_EMPTY;
  if (! GTK_EDITABLE(text)->has_selection)  XSRETURN_EMPTY;
  else {
    XST_mIV(0, GTK_EDITABLE(text)->selection_start_pos);
    XST_mIV(1, GTK_EDITABLE(text)->selection_end_pos);
    XSRETURN(2);
  }
}

XS(XS_GRN_mew_text_get)
{
  gint start, end;
  gchar *s;
  GtkWidget *vbox, *text = NULL;
  dXSARGS;
  items = 0;

  if (! GRN->focused)  XSRETURN_EMPTY;
  start = SvIV(ST(0));  end = SvIV(ST(1));
  vbox = gtk_object_get_data(GTK_OBJECT(GRN->focused), KEY_MSGPOST);
  if (vbox)  text = gtk_object_get_data(GTK_OBJECT(vbox), KEY_TXT_BODY);
  if ((! text) || (! GTK_IS_TEXT(text)))  XSRETURN_EMPTY;
  s = gtk_editable_get_chars(GTK_EDITABLE(text), start, end);
  if (! s)  XSRETURN_EMPTY;
  else {
    XST_mPV(0, s);
    g_free(s);
    XSRETURN(1);
  }
}

XS(XS_GRN_mew_text_delete)
{
  gint start, end;
  GtkWidget *vbox, *text = NULL;
  dXSARGS;
  items = 0;

  if (! GRN->focused)  XSRETURN_EMPTY;
  start = SvIV(ST(0));  end = SvIV(ST(1));
  vbox = gtk_object_get_data(GTK_OBJECT(GRN->focused), KEY_MSGPOST);
  if (vbox)  text = gtk_object_get_data(GTK_OBJECT(vbox), KEY_TXT_BODY);
  if ((! text) || (! GTK_IS_TEXT(text)))  XSRETURN_EMPTY;
  gtk_editable_delete_text(GTK_EDITABLE(text), start, end);
  XSRETURN_EMPTY;
}

XS(XS_GRN_entry_text_get)
{
  int junk;
  gint start, end;
  gchar *s, *s1;
  GtkWidget *vbox, *entry = NULL;
  dXSARGS;
  items = 0;

  if (! GRN->focused)  XSRETURN_EMPTY;
  s1 = SvPV(ST(0), junk);
  start = SvIV(ST(1));  end = SvIV(ST(2));
  vbox = gtk_object_get_data(GTK_OBJECT(GRN->focused), KEY_MSGPOST);
  if (! vbox)  vbox = gtk_object_get_data(GTK_OBJECT(GRN->focused), KEY_WND);
  if (vbox)  entry = gtk_object_get_data(GTK_OBJECT(vbox), s1);
  if (! entry)  XSRETURN_EMPTY;
  if (! GTK_IS_EDITABLE(entry))  XSRETURN_EMPTY;
  s = gtk_editable_get_chars(GTK_EDITABLE(entry), start, end);
  if (! s)  XSRETURN_EMPTY;
  else {
    XST_mPV(0, s);
    g_free(s);
    XSRETURN(1);
  }
}

XS(XS_GRN_entry_text_delete)
{
  int junk;
  gchar *s1;
  gint start, end;
  GtkWidget *vbox, *entry = NULL;
  dXSARGS;
  items = 0;

  if (! GRN->focused)  XSRETURN_EMPTY;;
  s1 = SvPV(ST(0), junk);
  start = SvIV(ST(1));  end = SvIV(ST(2));
  vbox = gtk_object_get_data(GTK_OBJECT(GRN->focused), KEY_MSGPOST);
  if (! vbox)  vbox = gtk_object_get_data(GTK_OBJECT(GRN->focused), KEY_WND);
  if (vbox)  entry = gtk_object_get_data(GTK_OBJECT(vbox), s1);
  if (! entry)  XSRETURN_EMPTY;
  if (! GTK_IS_EDITABLE(entry))  XSRETURN_EMPTY;
  gtk_editable_delete_text(GTK_EDITABLE(entry), start, end);
  XSRETURN_EMPTY;
}

XS(XS_GRN_entry_text_set)
{
  int junk;
  gchar *s, *s1;
  GtkWidget *vbox, *entry = NULL;
  dXSARGS;
  items = 0;

  if (! GRN->focused)  XSRETURN_EMPTY;;
  s1 = SvPV(ST(0), junk);
  s = SvPV(ST(1), junk);
  vbox = gtk_object_get_data(GTK_OBJECT(GRN->focused), KEY_MSGPOST);
  if (! vbox)  vbox = gtk_object_get_data(GTK_OBJECT(GRN->focused), KEY_WND);
  if (vbox)  entry = gtk_object_get_data(GTK_OBJECT(vbox), s1);
  if (! entry)  XSRETURN_EMPTY;
  if (! GTK_IS_EDITABLE(entry))  XSRETURN_EMPTY;
  if (! GTK_EDITABLE(entry)->editable)  XSRETURN_EMPTY;
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  XSRETURN_EMPTY;
}


XS(XS_GRN_error_dialog)
{
  int junk;
  gchar *s1, *s2;
  dXSARGS;
  items = 0;

  s1 = SvPV(ST(0), junk);
  s2 = SvPV(ST(1), junk);
  grn_error(s1, s2);
  XSRETURN_EMPTY;
}


#endif
