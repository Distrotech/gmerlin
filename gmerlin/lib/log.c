/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
#include <pthread.h>
#include <syslog.h>

#include <gmerlin/log.h>
#include <gmerlin/utils.h>

static int log_mask = BG_LOG_ERROR | BG_LOG_WARNING | BG_LOG_INFO;

static char * last_error = NULL;
pthread_mutex_t last_error_mutex = PTHREAD_MUTEX_INITIALIZER;

static const struct
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
    { 0,              NULL }
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
  return NULL;
  }



static bg_msg_queue_t * log_queue = NULL;

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
        if(level == BG_LOG_ERROR)
          {
          pthread_mutex_lock(&last_error_mutex);
          last_error = bg_strdup(last_error, lines[i]);
          pthread_mutex_unlock(&last_error_mutex);
          }
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
  len = vsnprintf(NULL, 0, format, argp);
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

char * bg_log_last_error()
  {
  char * ret;
  pthread_mutex_lock(&last_error_mutex);
  ret = bg_strdup(NULL, last_error);
  pthread_mutex_unlock(&last_error_mutex);
  return ret;
  }

/* Syslog stuff */

static bg_msg_queue_t * syslog_queue = NULL;

static struct
  {
  int gmerlin_level;
  int syslog_level;
  }
loglevels[] =
  {
    { BG_LOG_ERROR,   LOG_ERR },
    { BG_LOG_WARNING, LOG_WARNING },
    { BG_LOG_INFO,    LOG_INFO },
    { BG_LOG_DEBUG,   LOG_DEBUG },
  };

void bg_log_syslog_init(const char * name)
  {
  syslog_queue = bg_msg_queue_create();
  bg_log_set_dest(syslog_queue);

  /* Initialize Logging */
  openlog(name, LOG_PID, LOG_USER);
  }

void bg_log_syslog_flush()
  {
  bg_msg_t * msg;
  char * domain;
  char * message;
  int i;
  
  int gmerlin_level;
  int syslog_level = LOG_INFO;
  while((msg = bg_msg_queue_try_lock_read(syslog_queue)))
    {
    gmerlin_level = bg_msg_get_id(msg);

    if(!(gmerlin_level & log_mask))
      {
      bg_msg_queue_unlock_read(syslog_queue);
      continue;
      }
    
    domain = bg_msg_get_arg_string(msg, 0);
    message = bg_msg_get_arg_string(msg, 1);


    i = 0;
    for(i = 0; i < sizeof(loglevels) / sizeof(loglevels[0]); i++)
      {
      if(loglevels[i].gmerlin_level == gmerlin_level)
        {
        loglevels[i].syslog_level = syslog_level;
        break;
        }
      }
    syslog(syslog_level, "%s: %s", domain, message);
    free(domain);
    free(message);
    bg_msg_queue_unlock_read(syslog_queue);
    }
  
  }


#if defined(__GNUC__)

static void cleanup_log() __attribute__ ((destructor));

static void cleanup_log()
  {
  if(last_error)
    free(last_error);
  if(syslog_queue)
    bg_msg_queue_destroy(syslog_queue);
  }

#endif
