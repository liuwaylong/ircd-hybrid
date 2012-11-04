/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  resv.c: Functions to reserve(jupe) a nick/channel.
 *
 *  Copyright (C) 2001-2002 Hybrid Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id$
 */

#include "stdinc.h"
#include "list.h"
#include "ircd.h"
#include "send.h"
#include "client.h"   
#include "memory.h"
#include "numeric.h"
#include "resv.h"
#include "hash.h"
#include "irc_string.h"
#include "ircd_defs.h"
#include "conf.h"
#include "conf_db.h"

dlink_list resv_channel_list = { NULL, NULL, 0 };


/* create_channel_resv()
 *
 * inputs	- name of channel to create resv for
 *		- reason for resv
 *		- flag, 1 for from ircd.conf 0 from elsehwere
 * output	- pointer to struct ResvChannel
 * side effects	-
 */
struct MaskItem *
create_channel_resv(char *name, char *reason, int in_conf)
{
  struct MaskItem *conf = NULL;

  if (name == NULL || reason == NULL)
    return NULL;

  if (hash_find_resv(name))
    return NULL;

  if (strlen(reason) > REASONLEN)
    reason[REASONLEN] = '\0';

  conf = conf_make(CONF_CRESV);

  DupString(conf->name, name);
  DupString(conf->reason, reason);

  dlinkAdd(conf, &conf->node, &resv_channel_list);
  hash_add_resv(conf);

  return conf;
}

/* create_nick_resv()
 *
 * inputs	- name of nick to create resv for
 *		- reason for resv
 *		- 1 if from ircd.conf, 0 if from elsewhere
 * output	- pointer to struct ResvNick
 * side effects	-
 */
struct MaskItem *
create_nick_resv(char *name, char *reason, int in_conf)
{
  struct MaskItem *conf = NULL;

  if (name == NULL || reason == NULL)
    return NULL;

  if (find_matching_name_conf(CONF_NRESV, name, NULL, NULL, 0))
    return NULL;

  if (strlen(reason) > REASONLEN)
    reason[REASONLEN] = '\0';

  conf = conf_make(CONF_NRESV);

  DupString(conf->name, name);
  DupString(conf->reason, reason);

  return conf;
}

/* clear_conf_resv()
 *
 * inputs	- none
 * output	- none
 * side effects	- All resvs are cleared out
 */
void
clear_conf_resv(void)
{
  dlink_node *ptr = NULL, *next_ptr = NULL;

  DLINK_FOREACH_SAFE(ptr, next_ptr, resv_channel_list.head)
  {
    struct MaskItem *conf = ptr->data;

    if (!IsConfDatabase(conf))
      delete_channel_resv(conf);
  }
}

/* delete_channel_resv()
 *
 * inputs	- pointer to channel resv to delete
 * output	- none
 * side effects	- given struct ResvChannel * is removed
 */
int
delete_channel_resv(struct MaskItem *conf)
{
  hash_del_resv(conf);
  dlinkDelete(&conf->node, &resv_channel_list);
  conf_free(conf);

  return 1;
}

/* match_find_resv()
 *
 * inputs       - pointer to name
 * output       - pointer to a struct ResvChannel
 * side effects - Finds a reserved channel whose name matches 'name',
 *                if can't find one returns NULL.
 */
struct MaskItem *
match_find_resv(const char *name)
{
  dlink_node *ptr = NULL;

  if (EmptyString(name))
    return NULL;

  DLINK_FOREACH(ptr, resv_channel_list.head)
  {
    struct MaskItem *conf = ptr->data;

    if (match_chan(name, conf->name))
      return conf;
  }

  return NULL;
}

/* report_resv()
 *
 * inputs	- pointer to client pointer to report to.
 * output	- NONE
 * side effects	- report all resvs to client.
 */
void
report_resv(struct Client *source_p)
{
  dlink_node *ptr = NULL;
  struct MaskItem *conf = NULL;

  DLINK_FOREACH(ptr, resv_channel_list.head)
  {
    conf = ptr->data;
    sendto_one(source_p, form_str(RPL_STATSQLINE),
               me.name, source_p->name,
	       conf->hold ? 'q' : 'Q',
	       conf->name, conf->reason);
  }

  DLINK_FOREACH(ptr, nresv_items.head)
  {
    conf = ptr->data;;
    sendto_one(source_p, form_str(RPL_STATSQLINE),
               me.name, source_p->name,
	       conf->hold ? 'q' : 'Q',
	       conf->name, conf->reason);
  }
}

/* valid_wild_card_simple()
 *
 * inputs	- data to check for sufficient non-wildcard characters
 * outputs	- 1 if valid, else 0
 * side effects	- none
 */
int
valid_wild_card_simple(const char *data)
{
  const unsigned char *p = (const unsigned char *)data;
  int nonwild = 0;

  while (*p != '\0')
  {
    if ((*p == '\\' && *++p) || (*p && !IsMWildChar(*p)))
      if (++nonwild == ConfigFileEntry.min_nonwildcard_simple)
        return 1;
    if (*p != '\0')
      ++p;
  }

  return 0;
}
