/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

// #ifdef HAVE_VASPRINTF
// #define _GNU_SOURCE
// #endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ISATTY
#include <unistd.h>
#endif

#include <avdec_private.h>

static const struct
  {
  bgav_log_level_t level;
  const char * name;
  }
level_names[] =
  {
    { BGAV_LOG_DEBUG,   TRS("Debug") },
    { BGAV_LOG_WARNING, TRS("Warning") },
    { BGAV_LOG_ERROR,   TRS("Error") },
    { BGAV_LOG_INFO,    TRS("Info") },
    { 0,                NULL }
  };
  
static const char * log_level_to_string(bgav_log_level_t level)
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

static void log_default_nocolor(bgav_log_level_t level,
                                const char * domain, const char * msg)
  {
  fprintf(stderr, "[%s] %s: %s\n",
          domain,
          TRD(log_level_to_string(level)),
          msg);
  }

#ifdef HAVE_ISATTY
static int log_initialized = 0;
static int use_color = 0;

static char term_error[]  = "\33[31m";
static char term_warn[]   = "\33[33m";
static char term_debug[]  = "\33[32m";
static char term_reset[]  = "\33[0m";

static void log_default(bgav_log_level_t level, const char * domain, const char * msg)
  {
  if(!log_initialized)
    {
    if(isatty(fileno(stderr)) && getenv("TERM"))
      use_color = 1;
    else
      use_color = 0;
    log_initialized = 1;
    }

  if(use_color)
    {
    switch(level)
      {
      case BGAV_LOG_INFO:
        fprintf(stderr, "[%s] %s\n",
                domain,
                msg);
        break;
      case BGAV_LOG_WARNING:
        fprintf(stderr, "%s[%s] %s%s\n",
                term_warn,
                domain,
                msg,
                term_reset);
        break;
      case BGAV_LOG_ERROR:
        fprintf(stderr, "%s[%s] %s%s\n",
                term_error,
                domain,
                msg,
                term_reset);
        break;
      case BGAV_LOG_DEBUG:
        fprintf(stderr, "%s[%s] %s%s\n",
                term_debug,
                domain,
                msg,
                term_reset);
        break;
      }
    
    }
  else
    {
    log_default_nocolor(level, domain, msg);
    }
  }

#else
#define log_default log_default_nocolor
#endif


void bgav_log(const bgav_options_t * opt,
              bgav_log_level_t level, const char * domain, const char * format, ...)
  {
  char * msg_string;
  va_list argp; /* arg ptr */
  int len;

  if(opt && !(level & opt->log_level))
    return;
  
  format = TRD(format);
    
  va_start( argp, format);

#ifndef HAVE_VASPRINTF
  len = vsnprintf(NULL, 0, format, argp);
  msg_string = malloc(len+1);
  vsnprintf(msg_string, len+1, format, argp);
#else
  vasprintf(&msg_string, format, argp);
#endif
  len = strlen(msg_string);
  
  if(msg_string[len - 1] == '\n')
    msg_string[len - 1] = '\0';
  
  va_end(argp);
  
  if(!opt || !opt->log_callback)
    {
    log_default(level, domain, msg_string);
    }
  else
    {
    opt->log_callback(opt->log_callback_data,
                      level, domain, msg_string);
#if 0
    fprintf(stderr, "[%s] %s: %s\n",
            domain,
            log_level_to_string(level),
            msg_string);
#endif
    }
  free(msg_string);
  }
