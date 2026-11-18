/*
 * grn_types.h - Project-wide types for GRN
 *
 * $Id: grn_types.h,v 1.30 2000/06/28 11:28:26 sc Exp $
 *
 */

#ifndef __GRN_TYPES_H__
#define __GRN_TYPES_H__

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <gnome.h>
#include "grn_consts.h"


typedef struct grn_socket
{
  int fd;
  gboolean error;
  gboolean closed;
  gchar *addr;
  gchar *service;
  gchar *buf;
  gchar *endbuf, *inptr;
  gchar *err_msg;
  gulong bytes_rx;
  gulong bytes_tx;
} grn_socket;


typedef struct grn_server
{
  gboolean	posting;
  gchar*	name;
} grn_server;

typedef struct grn_newsgroup
{
  gulong total;
  gulong first;
  glong  nunread;
  gchar *name;
  gchar *descr;
  gchar *seq;
  gboolean tagged;
  GList *plain_unread;
} grn_newsgroup;


typedef struct t_msgheader
{
  gchar *name;
  gchar *value;
} t_msgheader;

typedef struct t_msgheaders
{
  gulong number;
  gulong lines;
  gulong size;
  guint score;
  time_t parsed_date;
  gchar *subject;
  gchar *from;
  gchar *date;
  gchar *id;
  gchar *ref;
  gchar *xref;
  gchar *path;
  gchar *newsgroups;
  gchar *to;			// only for mail
  GHashTable *extra_hdrs;
  grn_newsgroup *grp;		// reference
} t_msgheaders;

typedef struct t_message
{
  t_msgheaders *mh;
  gchar *body;
  GList *threads;
  guint nchildren;
  gint  pos;
  gboolean tagged;
  gboolean cached;
  gpointer data;
  GSList *attachments;
  GSList *temp_files;
  gchar *filename;
  FILE *fp;
} t_message;

typedef struct grn_mime_part
{
  gulong size;
  gchar *type;
  gchar *subtype;
  gchar *enc;
  gchar *filename;
  gchar *data;
} grn_mime_part;


typedef struct grn_find
{
  gchar *data;
  gboolean search_from;
  gboolean search_subj;
  gboolean search_body;
  gboolean search_hdr;
  gchar *hdr;
  gboolean is_regex;
  gboolean sensitive;
  gboolean unread_only;
} grn_find;

typedef struct grn_threadlist_stats
{
  gulong nthreads;
  gulong nmsgs;
  gulong nunread;
  gulong nthunread;
} grn_threadlist_stats;

typedef struct grn_pixmap
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
} grn_pixmap;

typedef struct grn_pixmaps
{
  grn_pixmap new_art_group;
  grn_pixmap subscr_group;
  grn_pixmap unread_article;
  grn_pixmap caption_subscr;
  grn_pixmap caption_unread;
  grn_pixmap thread_opened;
  grn_pixmap thread_closed;
  grn_pixmap balls[17];
  grn_pixmap mini;
} grn_pixmaps;

typedef struct grn_session
{
  GnomeAppBar *appbar;
  GnomeClient *client;
  GtkTooltips *tooltips;
  GtkWidget *grouplist;
  GtkWidget *threadlist;
  GtkWidget *msgwin;
  GtkWidget *window;
  GtkContainer *container;
  GtkWindow *focused;		// currently edited message window

  grn_server srv;
  grn_socket *nntp_sock;
  grn_pixmaps pixmaps;
  GList *ng_list;		// list of grn_newsgroup*
  GList *msghdr_list;
  grn_find *find;
  gulong total_groups;
  pthread_t tid_queue;
} grn_session;

typedef struct grn_flags
{
  gboolean open_group;
  gboolean open_grouplist;
  gboolean open_article;
  gboolean post_article;
} grn_flags;


#endif /* __GRN_TYPES_H__ */

