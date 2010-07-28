/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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
    { 0,              (char*)0 }
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
  return (char*)0;
  }


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
  len = vsnprintf((char*)0, 0, format, argp);
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
    fprintf(stderr, "[%s] %s: %s\n",
            domain,
            TRD(log_level_to_string(level)),
            msg_string);
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
