/*
 * smtp.h: based on Pan 0.8.0
 * Copyright (C) 1999, 2000  Pan Development Team (pan@superpimp.org)
 *
 * $Id: smtp.h,v 1.1 2000/04/12 13:16:11 sc Exp $
 */

#ifndef __GRN_SMTP_H__
#define __GRN_SMTP_H__


#include "grn_types.h"


gchar *smtp_connect(grn_socket *);
gchar *smtp_send_message(grn_socket *, t_message *);


#endif
