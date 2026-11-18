/*
 * xml-io.c: manipulating XML data
 *
 * $Id: xml-io.c,v 1.4 2000/04/10 02:42:53 sc Exp $
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
#include "xml-io.h"


xmlAttrPtr htmlSearchAttr(xmlAttrPtr attr, const gchar *name)
{
  xmlAttrPtr ret;
  g_return_val_if_fail(name != NULL, NULL);
  
  ret = attr;
  while (ret)
  {
    if (ret->name)
    {
      if (! g_strcasecmp(ret->name, name))  return ret;
    }
    ret = ret->next;
  }
  return NULL;
}

gchar *htmlAttrGet(xmlAttrPtr attr, const gchar *name)
{
  gchar *ret;
  xmlAttrPtr found;
  g_return_val_if_fail(name != NULL, NULL);
  if (! attr)  return NULL;
  found = htmlSearchAttr(attr, name);
  if (! found)  return NULL;
  
  ret = xmlNodeGetContent(found->val);
  return ret;
}

gint htmlAttrGetInt(xmlAttrPtr attr, const gchar *name, gint *val)
{
  gchar *ret;
  gint res;
  gint i;
  g_return_val_if_fail(name != NULL, NULL);
  g_return_val_if_fail(val != NULL, NULL);
  
  ret = htmlAttrGet(attr, name);
  if (! ret)  return (0);
  res = sscanf(ret, "%d", &i);
  free(ret);
  if (res == 1)
  {
    *val = i;
    return 1;
  }
  return 0;
}
