/*
 * tcp.c: TCP communicating code
 *
 * $Id: tcp.c,v 1.4 2000/07/19 06:26:22 sc Exp $
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
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <strings.h>
#define _POSIX_SOURCE
#include <signal.h>
#include <sys/signal.h>
#undef _POSIX_SOURCE
#include <glib.h>
#include "tcp.h"
#include "grn_util.h"


#define BUFSIZE	4096


void grn_socket_init(grn_socket *socket)
{
  socket->fd = -1;
  socket->error = FALSE;
  socket->closed = TRUE;
  socket->addr = NULL;
  socket->service = NULL;
  socket->err_msg = NULL;
  socket->buf = g_malloc(BUFSIZE);
  socket->endbuf = socket->buf;
  socket->inptr = socket->buf;
  socket->bytes_rx = 0L;
  socket->bytes_tx = 0L;
}

grn_socket *grn_socket_new(void)
{
  grn_socket *ret = g_new(grn_socket, 1);
  grn_socket_init(ret);
  return ret;
}

void grn_socket_free(grn_socket **socket)
{
  if (! socket)  return;
  if (! *socket)  return;

  g_print("%s:%s\tRX:%lu\tTX:%lu\n", (*socket)->addr, (*socket)->service,
    (*socket)->bytes_rx,  (*socket)->bytes_tx);                       

  if (! (*socket)->closed)  grn_socket_close(*socket);
  if ((*socket)->addr)  str_free(&((*socket)->addr));
  if ((*socket)->service)  str_free(&((*socket)->service));
  if ((*socket)->buf)  str_free(&((*socket)->buf));
  if ((*socket)->err_msg)  str_free(&((*socket)->err_msg));
  g_free(*socket);  *socket = NULL;
}

void grn_socket_destroy(grn_socket *socket)
{  grn_socket_free(&socket);  }


void grn_socket_open(grn_socket *sock)
{
  struct sockaddr_in serv_addr;
  struct servent serv_info, *sp;
  struct hostent host_info, *hp;
  struct sigaction act, oact;
  int on;
  unsigned long inaddr;
  g_return_if_fail(sock != NULL);
  
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  
  sock->error = FALSE;
  if ((! str_check(sock->addr)) || (! str_check(sock->service)))
  {
    grn_socket_set_error(sock, _("Invalid input parameters"));
    return;
  }
  if ((sp = getservbyname(sock->service, "tcp")) == NULL)
  {
    grn_socket_set_error(sock, _("tcp_open(): getservbyname() failed"));
    return;
  }
  serv_info = *sp;
  serv_addr.sin_port = sp->s_port;
  
  if ((inaddr = inet_addr(sock->addr)) != -1)
  {
    bcopy((char *) &inaddr, (char *) &serv_addr.sin_addr, sizeof(inaddr));
    host_info.h_name = NULL;
  }
  else {
    while ((hp = gethostbyname(sock->addr)) == NULL)
    {
      grn_socket_set_error(sock,  _("tcp_open(): gethostbyname() failed"));
      return;
    }
    host_info = *hp;
    bcopy(hp->h_addr, (char *) &serv_addr.sin_addr, hp->h_length);
  }
  if ((sock->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    grn_socket_set_error(sock, _("tcp_open(): socket() failed"));
    return;
  }
  if (connect(sock->fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
    grn_socket_close(sock);
    grn_socket_set_error(sock, _("tcp_open(): connect() failed"));
    return;
  }
  on = 1;
  if (setsockopt(sock->fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &on, sizeof(on)) < 0)
  {
    grn_socket_close(sock);
    grn_socket_set_error(sock, _("tcp_open(): setsockopt() failed"));
    return;
  }
  
  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGPIPE, &act, &oact);
  sock->closed = FALSE;
}

void grn_socket_close(grn_socket *socket)
{
  int rc;
  g_return_if_fail(socket != NULL);

  socket->error = FALSE;
  if (socket->closed)
  {
    grn_socket_set_error(socket, _("Already closed"));
    return;
  }
  rc = close(socket->fd);
  if (rc == 0)  socket->closed = TRUE;
  if (rc == -1)
  {
    switch (errno)
    {
      case EBADF:
        grn_socket_set_error(socket, _("Invalid descriptor"));
	break;
      case EINTR:
        grn_socket_set_error(socket, _("Interrupted by user"));
	break;
      default:  grn_socket_set_error(socket, _("Undefined error"));
    }
  }
}

gint grn_socket_read_char(grn_socket *socket, gchar *c)
{
  socket->error = FALSE;
  if (socket->closed)
  {
    grn_socket_set_error(socket, _("The socket is closed"));
    return (-1);
  }
  if (socket->inptr == socket->endbuf)
  {
    int read_count=0, retval=0;
    struct timeval tv;
    fd_set rfds;
    
    FD_ZERO(&rfds);
    FD_SET(socket->fd, &rfds);
    
    tv.tv_sec = 120;  tv.tv_usec = 0;
    retval = select(socket->fd + 1, &rfds, NULL, NULL, &tv);
    if (retval <=0)
    {
      grn_socket_set_error(socket, _("select() timeout"));
      return (-1);
    }
    read_count = read(socket->fd, socket->buf, BUFSIZE);
    socket->endbuf = socket->buf + read_count;
    socket->inptr = socket->buf;
    if (read_count == 0)  return 0;
    if (read_count < 0)
    {
      switch (errno)
      {
        case EBADF:
	  grn_socket_set_error(socket, _("Invalid descriptor"));
	  break;
        case EINTR:
	  grn_socket_set_error(socket, _("Interrupted by user"));
	  break;
        case EFAULT:
	case EINVAL:
	  grn_socket_set_error(socket, _("Invalid pointer"));
	  break;
        case EIO:
	case EAGAIN:
	  grn_socket_set_error(socket, _("I/O error"));
	  break;
	default:  grn_socket_set_error(socket, _("Undefined error"));
      }
      return read_count;
    }
  }
  *c = *(socket->inptr)++;
  socket->bytes_rx++;
  return 1;
}

gchar *grn_socket_get(grn_socket *socket)
{
  gchar *line_buffer;
  int maxline = 128;
  char c;
  int i, done=0;
  g_return_val_if_fail(socket != NULL, NULL);
  
  socket->error = FALSE;
  if (socket->closed)
  {
    grn_socket_set_error(socket, _("The socket is closed"));
    return NULL;
  }
  line_buffer = (gchar *) g_malloc(maxline);
  for (i=0; !done; i++)
  {
    if (grn_socket_read_char(socket, &c) != 1)
    {
      g_free(line_buffer);
      return NULL;
    }
    if (i >= (maxline - 1))
    {
      maxline *= 2;
      line_buffer = (gchar *) g_realloc(line_buffer, maxline);
    }
    line_buffer[i] = c;
    if (c == '\n')  done++;
  }
  line_buffer[i] = '\0';
  if (i == 0 || line_buffer[i-1] != '\n')
  {
    g_free(line_buffer);
    return NULL;
  }
  return line_buffer;
}

gint grn_socket_put(grn_socket *socket, gchar *line)
{
  gint nleft, nwritten, nbytes;
  g_return_val_if_fail(socket != NULL, (-1));

  socket->error = FALSE;
  if (socket->closed)
  {
    grn_socket_set_error(socket, _("The socket is closed"));
    return (-1);
  }
  nbytes = strlen(line);  nleft = nbytes;
  while (nleft > 0)
  {
    nwritten = write(socket->fd, line, nleft);
    if (nwritten < 0)
    {
      switch (errno)
      {
        case EBADF:
	  grn_socket_set_error(socket, _("Invalid descriptor"));
	  break;
        case EPIPE:
	  grn_socket_set_error(socket, _("Invalid pipe/socket"));
	  break;
        case EFBIG:
	  grn_socket_set_error(socket, _("File size limit exceeded"));
	  break;
        case EFAULT:
	case EINVAL:
	  grn_socket_set_error(socket, _("Invalid pointer"));
	  break;
        case ENOSPC:
	  grn_socket_set_error(socket, _("No free space on FS"));
	  break;
        case EIO:
	case EAGAIN:
	  grn_socket_set_error(socket, _("I/O error"));
	  break;
	default:  grn_socket_set_error(socket, _("Undefined error"));
      }
      return (-1);
    }
    nleft -= nwritten;
    line += nwritten;
    socket->bytes_tx += nwritten;
  }
  return (nbytes - nleft);
}

gint grn_socket_put_va(grn_socket *sock, const gchar *format, ...)
{
  va_list args;
  gchar *ptr;
  gint nwritten;
  g_return_val_if_fail(format != NULL, (-1));
  
  va_start(args, format);
  ptr = g_strdup_vprintf(format, args);
  va_end(args);
  
  nwritten = grn_socket_put(sock, ptr);
  str_free(&ptr);
  return nwritten;
}

void grn_socket_set_error(grn_socket *socket, gchar *msg)
{
  g_return_if_fail(socket != NULL);

  if (socket->err_msg)  str_free(&(socket->err_msg));
  socket->error = TRUE;
  if (str_check(msg))  socket->err_msg = g_strdup(msg);
  else  socket->err_msg = NULL;
}
