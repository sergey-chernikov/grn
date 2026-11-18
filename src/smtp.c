/*
 * smtp.c: based on Pan 0.8.0
 * Copyright (C) 1999, 2000  Pan Development Team (pan@superpimp.org)
 *
 * $Id: smtp.c,v 1.2 2000/04/20 14:47:08 sc Exp $
 */

#include <config.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <gnome.h>
#include "smtp.h"
#include "tcp.h"
#include "grn_util.h"
#include "grn_config.h"
#include "grn_msgsend.h"


static gchar *smtp_handshake(grn_socket *sock)
{
  gchar my_name[256+1];
  struct hostent *hp;
  gchar *buf;

  gethostname (my_name, sizeof(my_name));
  hp = gethostbyname (my_name);
  strcpy (my_name, hp->h_name);
  
  if (grn_socket_put_va(sock, "HELO %s\r\n", my_name) == (-1))
    return _("SMTP HELO failed");

  if ((buf = grn_socket_get(sock)) == NULL)  return _("server not responding");
  if (atoi(buf) != 250)
  {
    str_free(&buf);
    return _("error in server reply");
  }
  str_free(&buf);
  
  return NULL;
}

gchar *smtp_connect(grn_socket *sock)
{
  gchar *buf, *rc;
  g_return_val_if_fail(sock != NULL, NULL);

  if (! str_check(grn_prefs.smtp_server))  return _("no server name given");
  sock->addr = g_strdup(grn_prefs.smtp_server);
  sock->service = g_strdup("smtp");
  grn_socket_open(sock);
  if (sock->error)  return sock->err_msg;

  if ((buf = grn_socket_get(sock)) == NULL)  return _("server not responding");
  if (atoi(buf) != 220)
  {
    str_free(&buf);
    return _("mail server not ready");
  }
  str_free(&buf);

  rc = smtp_handshake(sock);
  if (rc)  return rc;
  return NULL;
}

gchar *smtp_disconnect (grn_socket *sock)
{
  gchar *buf;
  g_return_val_if_fail(sock != NULL, NULL);

  if (grn_socket_put(sock, "QUIT\r\n") == (-1))  return _("SMTP QUIT failed");
  if ((buf = grn_socket_get(sock)) == NULL)  return _("server not responding");
  if (atoi(buf) != 221)
  {
    str_free(&buf);
    return _("error in server reply");
  }
  str_free(&buf);
  return NULL;
}

gchar *smtp_send_message(grn_socket *sock, t_message *msg)
{
  gchar *buf, *data, *err = NULL;
  GSList *rcpts, *l;
  g_return_val_if_fail(sock != NULL, NULL);
  g_return_val_if_fail(msg != NULL, NULL);
  g_return_val_if_fail(msg->mh != NULL, NULL);
  
  if (grn_socket_put_va(sock, "MAIL FROM: %s\r\n", msg->mh->from) == (-1))
    return _("can't send command to server");
  if ((buf = grn_socket_get(sock)) == NULL)  return _("server not responding");
  if (atoi(buf) != 250)
  {
    g_warning("server replied:\n%s", buf);
    str_free(&buf);
    return _("message rejected by the server");
  }
  str_free(&buf);

  rcpts = rcpts2list(msg->mh->to);
  for (l=rcpts; l; l=l->next)
  {
    gchar *recip = (gchar *) l->data;
    if (! recip)  continue;

    if (grn_socket_put_va(sock, "RCPT TO: %s\r\n", recip) == (-1))
    {
      str_free(&recip);
      err = _("can't send command to server");  break;
    }
    str_free(&recip);
    if ((buf = grn_socket_get(sock)) == NULL)
    {
      err = _("server not responding");  break;
    }
    if ((atoi(buf) == 250) || (atoi(buf) == 550))  str_free(&buf);
    else {
      g_warning("server replied:\n%s", buf);	str_free(&buf);
      err = _("error in server reply");		break;
    }
  }
  g_slist_free(rcpts);
  if (err)  return err;

  if (grn_socket_put(sock, "DATA\r\n") == (-1))
    return _("can't send command to server");
  if ((buf = grn_socket_get(sock)) == NULL)  return _("server not responding");
  if (atoi(buf) != 354)
  {
    g_warning("server replied:\n%s", buf);
    str_free(&buf);
    return _("error in server reply");
  }
  
  data = msg_to_smtp_text(msg);
  if (! data)  return _("internal error");

  if (grn_socket_put(sock, data) <= (-1))
    return _("can't send data to server");
  str_free(&data);
  
  if (grn_socket_put(sock, ".\r\n") <= (-1))
    return _("can't send data to server");
  if ((buf = grn_socket_get(sock)) == NULL)  return _("server not responding");
  if (atoi(buf) != 250)
  {
    g_warning("server replied:\n%s\n", buf);
    str_free(&buf);
    return _("message not sent");
  }
  str_free(&buf);
  
  return NULL;
}
