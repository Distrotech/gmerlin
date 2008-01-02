/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <config.h>

#ifdef HAVE_VASPRINTF
#define _GNU_SOURCE
#endif

#include <libintl.h>

#include <stdarg.h>
#include <stdio.h>

#include <log.h>
#include <utils.h>

static int log_mask = BG_LOG_ERROR | BG_LOG_WARNING | BG_LOG_INFO;

static struct
  {
  bg_log_level_t level;
  const char * name;
  }
level_names[] =
  {
    { BG_LOG_DEBUG,   "Debug" },
    { BG_LOG_WARNING, "Warning" },
    { BG_LOG_ERROR,   "Error" },
    { BG_LOG_INFO,    "Info" },
    { 0,              (char*)0 }
  };
  
const char * bg_log_level_to_string(bg_log_level_t level)
  {
  int index = 0;
  while(level_names[index].name)
    {
    if(level_names[index].level == level)
      return level_names[index].name;
    index++;
    }
  return (char*)0;
  }



static bg_msg_queue_t * log_queue = (bg_msg_queue_t*)0;

static void logs_internal(bg_log_level_t level, const char * domain,
                          const char * msg_string)
  {
  bg_msg_t * msg;
  char ** lines;
  int i;
  FILE * out = stderr;
  if(!log_queue)
    {
    if(level & log_mask)
      {
      lines = bg_strbreak(msg_string, '\n');
      i = 0;
      while(lines[i])
        {
        fprintf(out, "[%s] %s: %s\n",
                domain,
                bg_log_level_to_string(level),
                lines[i]);
        i++;
        }
      bg_strbreak_free(lines);
      }
    }
  else
    {
    msg = bg_msg_queue_lock_write(log_queue);
    bg_msg_set_id(msg, level);
    bg_msg_set_arg_string(msg, 0, domain);
    bg_msg_set_arg_string(msg, 1, msg_string);
    bg_msg_queue_unlock_write(log_queue);
    }
  }

static void log_internal(bg_log_level_t level, const char * domain,
                         const char * format, va_list argp)
  {
  char * msg_string;

#ifndef HAVE_VASPRINTF
  int len;
  len = vsnprintf((char*)0, 0, format, argp);
  msg_string = malloc(len+1);
  vsnprintf(msg_string, len+1, format, argp);
#else
  vasprintf(&msg_string, format, argp);
#endif
  logs_internal(level, domain, msg_string);
  free(msg_string);
  }

void bg_log_notranslate(bg_log_level_t level, const char * domain,
                        const char * format, ...)
  {
  va_list argp; /* arg ptr */
  
  va_start( argp, format);
  log_internal(level, domain, format, argp);
  va_end(argp);
  }

void bg_logs_notranslate(bg_log_level_t level, const char * domain,
                        const char * str)
  {
  logs_internal(level, domain, str);
  }

void bg_log_translate(const char * translation_domain,
                      bg_log_level_t level, const char * domain,
                      const char * format, ...)
  {
  va_list argp; /* arg ptr */

  /* Translate format string */
  format = dgettext(translation_domain, format);
  
  va_start( argp, format);
  log_internal(level, domain, format, argp);
  va_end(argp);
  }

void bg_log_set_dest(bg_msg_queue_t * q)
  {
  log_queue = q;
  }

void bg_log_set_verbose(int mask)
  {
  log_mask = mask;
  }
