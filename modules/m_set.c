/*
 *  ircd-hybrid: an advanced, lightweight Internet Relay Chat Daemon (ircd)
 *
 *  Copyright (c) 1997-2017 ircd-hybrid development team
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 *  USA
 */

/*! \file m_set.c
 * \brief Includes required functions for processing the SET command.
 * \version $Id$
 */

#include "stdinc.h"
#include "client.h"
#include "event.h"
#include "irc_string.h"
#include "ircd.h"
#include "numeric.h"
#include "send.h"
#include "conf.h"
#include "parse.h"
#include "modules.h"
#include "misc.h"


/* SET AUTOCONN */
static void
quote_autoconn(struct Client *source_p, const char *arg, int newval)
{
  if (!EmptyString(arg))
  {
    struct MaskItem *conf = connect_find(arg, NULL, irccmp);

    if (conf)
    {
      if (newval)
        SetConfAllowAutoConn(conf);
      else
        ClearConfAllowAutoConn(conf);

      sendto_realops_flags(UMODE_SERVNOTICE, L_ALL, SEND_NOTICE,
                           "%s has changed AUTOCONN for %s to %i",
                           get_oper_name(source_p), conf->name, newval);
      sendto_one_notice(source_p, &me, ":AUTOCONN for %s is now set to %i",
                        conf->name, newval);
    }
    else
      sendto_one_notice(source_p, &me, ":Cannot find %s", arg);
  }
  else
    sendto_one_notice(source_p, &me, ":Please specify a server name!");
}

/* SET AUTOCONNALL */
static void
quote_autoconnall(struct Client *source_p, const char *arg, int newval)
{
  if (newval >= 0)
  {
    GlobalSetOptions.autoconn = newval;
    sendto_realops_flags(UMODE_SERVNOTICE, L_ALL, SEND_NOTICE,
                         "%s has changed AUTOCONNALL to %u",
                         get_oper_name(source_p), GlobalSetOptions.autoconn);
  }
  else
    sendto_one_notice(source_p, &me, ":AUTOCONNALL is currently %u",
                      GlobalSetOptions.autoconn);
}

/* SET FLOODCOUNT */
static void
quote_floodcount(struct Client *source_p, const char *arg, int newval)
{
  if (newval >= 0)
  {
    GlobalSetOptions.floodcount = newval;
    sendto_realops_flags(UMODE_SERVNOTICE, L_ALL, SEND_NOTICE,
                         "%s has changed FLOODCOUNT to %u",
                         get_oper_name(source_p), GlobalSetOptions.floodcount);
  }
  else
    sendto_one_notice(source_p, &me, ":FLOODCOUNT is currently %u",
                      GlobalSetOptions.floodcount);
}

/* SET FLOODTIME */
static void
quote_floodtime(struct Client *source_p, const char *arg, int newval)
{
  if (newval >= 0)
  {
    GlobalSetOptions.floodtime = newval;
    sendto_realops_flags(UMODE_SERVNOTICE, L_ALL, SEND_NOTICE,
                         "%s has changed FLOODTIME to %u",
                         get_oper_name(source_p), GlobalSetOptions.floodtime);
  }
  else
    sendto_one_notice(source_p, &me, ":FLOODTIME is currently %u",
                      GlobalSetOptions.floodtime);
}

/* SET IDENTTIMEOUT */
static void
quote_identtimeout(struct Client *source_p, const char *arg, int newval)
{
  if (!HasUMode(source_p, UMODE_ADMIN))
  {
    sendto_one_numeric(source_p, &me, ERR_NOPRIVS, "set");
    return;
  }

  if (newval > 0)
  {
    GlobalSetOptions.ident_timeout = newval;
    sendto_realops_flags(UMODE_SERVNOTICE, L_ALL, SEND_NOTICE,
                         "%s has changed IDENTTIMEOUT to %u",
                         get_oper_name(source_p), GlobalSetOptions.ident_timeout);
  }
  else
    sendto_one_notice(source_p, &me, ":IDENTTIMEOUT is currently %u",
                      GlobalSetOptions.ident_timeout);
}

/* SET MAX */
static void
quote_max(struct Client *source_p, const char *arg, int newval)
{
  if (newval > 0)
  {
    if (newval > MAXCLIENTS_MAX)
    {
      sendto_one_notice(source_p, &me, ":You cannot set MAXCLIENTS to > %d, restoring to %u",
                        MAXCLIENTS_MAX, GlobalSetOptions.maxclients);
      return;
    }

    if (newval < MAXCLIENTS_MIN)
    {
      sendto_one_notice(source_p, &me, ":You cannot set MAXCLIENTS to < %d, restoring to %u",
                        MAXCLIENTS_MIN, GlobalSetOptions.maxclients);
      return;
    }

    GlobalSetOptions.maxclients = newval;
    sendto_realops_flags(UMODE_SERVNOTICE, L_ALL, SEND_NOTICE,
                         "%s set new MAXCLIENTS to %u (%u current)",
                         get_oper_name(source_p), GlobalSetOptions.maxclients, dlink_list_length(&local_client_list));
  }
  else
    sendto_one_notice(source_p, &me, ":Current MAXCLIENTS = %u (%u)",
                      GlobalSetOptions.maxclients, dlink_list_length(&local_client_list));
}

/* SET SPAMNUM */
static void
quote_spamnum(struct Client *source_p, const char *arg, int newval)
{
  if (newval >= 0)
  {
    if (newval == 0)
    {
      GlobalSetOptions.spam_num = newval;
      sendto_realops_flags(UMODE_SERVNOTICE, L_ALL, SEND_NOTICE,
                           "%s has disabled ANTI_SPAMBOT", source_p->name);
      return;
    }

    GlobalSetOptions.spam_num = IRCD_MAX(newval, MIN_SPAM_NUM);
    sendto_realops_flags(UMODE_SERVNOTICE, L_ALL, SEND_NOTICE,
                         "%s has changed SPAMNUM to %i",
                         get_oper_name(source_p), GlobalSetOptions.spam_num);
  }
  else
    sendto_one_notice(source_p, &me, ":SPAMNUM is currently %i",
                      GlobalSetOptions.spam_num);
}

/* SET SPAMTIME */
static void
quote_spamtime(struct Client *source_p, const char *arg, int newval)
{
  if (newval > 0)
  {
    GlobalSetOptions.spam_time = IRCD_MAX(newval, MIN_SPAM_TIME);
    sendto_realops_flags(UMODE_SERVNOTICE, L_ALL, SEND_NOTICE,
                         "%s has changed SPAMTIME to %u",
                         get_oper_name(source_p), GlobalSetOptions.spam_time);
  }
  else
    sendto_one_notice(source_p, &me, ":SPAMTIME is currently %u",
                      GlobalSetOptions.spam_time);
}

/* SET JFLOODTIME */
static void
quote_jfloodtime(struct Client *source_p, const char *arg, int newval)
{
  if (newval >= 0)
  {
    GlobalSetOptions.joinfloodtime = newval;
    sendto_realops_flags(UMODE_SERVNOTICE, L_ALL, SEND_NOTICE,
                         "%s has changed JFLOODTIME to %u",
                         get_oper_name(source_p), GlobalSetOptions.joinfloodtime);
  }
  else
    sendto_one_notice(source_p, &me, ":JFLOODTIME is currently %u",
                      GlobalSetOptions.joinfloodtime);
}

/* SET JFLOODCOUNT */
static void
quote_jfloodcount(struct Client *source_p, const char *arg, int newval)
{
  if (newval >= 0)
  {
    GlobalSetOptions.joinfloodcount = newval;
    sendto_realops_flags(UMODE_SERVNOTICE, L_ALL, SEND_NOTICE,
                         "%s has changed JFLOODCOUNT to %u",
                         get_oper_name(source_p), GlobalSetOptions.joinfloodcount);
  }
  else
    sendto_one_notice(source_p, &me, ":JFLOODCOUNT is currently %u",
                      GlobalSetOptions.joinfloodcount);
}

/* Structure used for the SET table itself */
struct SetStruct
{
  const char *const name;
  void (*const handler)(struct Client *, const char *, int);
  const unsigned int wants_char;  /* 1 if it expects (char *, [int]) */
  const unsigned int wants_int;  /* 1 if it expects ([char *], int) */
  /* eg:  0, 1 == only an int arg
   * eg:  1, 1 == char and int args */
};

/*
 * If this ever needs to be expanded to more than one arg of each
 * type, want_char/want_int could be the count of the arguments,
 * instead of just a boolean flag...
 *
 * -davidt
 */
static const struct SetStruct set_cmd_table[] =
{
  /* name               function        string arg  int arg */
  /* -------------------------------------------------------- */
  { "AUTOCONN",         quote_autoconn,         1,      1 },
  { "AUTOCONNALL",      quote_autoconnall,      0,      1 },
  { "FLOODCOUNT",       quote_floodcount,       0,      1 },
  { "FLOODTIME",        quote_floodtime,        0,      1 },
  { "IDENTTIMEOUT",     quote_identtimeout,     0,      1 },
  { "MAX",              quote_max,              0,      1 },
  { "SPAMNUM",          quote_spamnum,          0,      1 },
  { "SPAMTIME",         quote_spamtime,         0,      1 },
  { "JFLOODTIME",       quote_jfloodtime,       0,      1 },
  { "JFLOODCOUNT",      quote_jfloodcount,      0,      1 },
  /* -------------------------------------------------------- */
  { NULL,               NULL,                   0,      0 }
};

/*
 * list_quote_commands() sends the client all the available commands.
 * Four to a line for now.
 */
static void
list_quote_commands(struct Client *source_p)
{
  unsigned int j = 0;
  const char *names[4] = { "", "", "", "" };

  sendto_one_notice(source_p, &me, ":Available QUOTE SET commands:");

  for (const struct SetStruct *tab = set_cmd_table; tab->handler; ++tab)
  {
    names[j++] = tab->name;

    if (j > 3)
    {
      sendto_one_notice(source_p, &me, ":%s %s %s %s",
                        names[0], names[1],
                        names[2], names[3]);
      j = 0;
      names[0] = names[1] = names[2] = names[3] = "";
    }
  }

  if (j)
    sendto_one_notice(source_p, &me, ":%s %s %s %s",
                      names[0], names[1],
                      names[2], names[3]);
}

/*
 * mo_set - SET command handler
 * set options while running
 */
static int
mo_set(struct Client *source_p, int parc, char *parv[])
{
  int n;
  int newval;
  const char *strarg = NULL;
  const char *intarg = NULL;

  if (!HasOFlag(source_p, OPER_FLAG_SET))
  {
    sendto_one_numeric(source_p, &me, ERR_NOPRIVS, "set");
    return 0;
  }

  if (parc > 1)
  {
    /*
     * Go through all the commands in set_cmd_table, until one is
     * matched.
     */
    for (const struct SetStruct *tab = set_cmd_table; tab->handler; ++tab)
    {
      if (irccmp(tab->name, parv[1]))
        continue;

      /*
       * Command found; now execute the code
       */
      n = 2;

      if (tab->wants_char)
        strarg = parv[n++];

      if (tab->wants_int)
        intarg = parv[n++];

      if ((n - 1) > parc)
        sendto_one_notice(source_p, &me, ":SET %s expects (\"%s%s\") args", tab->name,
                          (tab->wants_char ? "string, " : ""),
                          (tab->wants_int ? "int" : ""));

      if (parc <= 2)
      {
        strarg = NULL;
        intarg = NULL;
      }

      if (tab->wants_int && parc > 2)
      {
        if (intarg)
        {
          if (!irccmp(intarg, "yes") || !irccmp(intarg, "on"))
            newval = 1;
          else if (!irccmp(intarg, "no") || !irccmp(intarg, "off"))
            newval = 0;
          else
            newval = atoi(intarg);
        }
        else
          newval = -1;

        if (newval < 0)
        {
          sendto_one_notice(source_p, &me, ":Value less than 0 illegal for %s",
                            tab->name);
          return 0;
        }
      }
      else
        newval = -1;

      tab->handler(source_p, strarg, newval);
      return 0;
    }

    /*
     * Code here will be executed when a /QUOTE SET command is not
     * found within set_cmd_table.
     */
    sendto_one_notice(source_p, &me, ":Variable not found.");
    return 0;
  }

  list_quote_commands(source_p);
  return 0;
}

static struct Message set_msgtab =
{
  .cmd = "SET",
  .args_max = MAXPARA,
  .handlers[UNREGISTERED_HANDLER] = m_unregistered,
  .handlers[CLIENT_HANDLER] = m_not_oper,
  .handlers[SERVER_HANDLER] = m_ignore,
  .handlers[ENCAP_HANDLER] = m_ignore,
  .handlers[OPER_HANDLER] = mo_set
};

static void
module_init(void)
{
  mod_add_cmd(&set_msgtab);
}

static void
module_exit(void)
{
  mod_del_cmd(&set_msgtab);
}

struct module module_entry =
{
  .version = "$Revision$",
  .modinit = module_init,
  .modexit = module_exit,
};
