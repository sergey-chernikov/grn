/*
 * grn_vars.h - GRN global variables
 *
 * $Id: grn_vars.h,v 1.11 1999/12/10 15:44:02 sc Exp $
 *
 */

#ifndef __GRN_VARS_H__
#define __GRN_VARS_H__

#include <gnome.h>
#include "grn_types.h"

extern grn_session *GRN;
extern grn_flags flags;

extern GHashTable *newsrc_ht;
extern GHashTable *ng_desc_ht;
extern gint idle_id;
extern gboolean must_exit;

#endif /* __GRN_VARS_H__ */
