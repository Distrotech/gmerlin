/*****************************************************************
 
  log.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <config.h>

#ifdef HAVE_VASPRINTF
#define _GNU_SOURCE
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <avdec_private.h>

static struct
  {
  bgav_log_level_t level;
  const char * name;
  }
level_names[] =
  {
    { BGAV_LOG_DEBUG,   "Debug" },
    { BGAV_LOG_WARNING, "Warning" },
    { BGAV_LOG_ERROR,   "Error" },
    { BGAV_LOG_INFO,    "Info" },
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
              bgav_log_level_t level, const char * domain, char * format, ...)
  {
  char * msg_string;
  va_list argp; /* arg ptr */
#ifndef HAVE_VASPRINTF
  int len;
#endif

  va_start( argp, format);

#ifndef HAVE_VASPRINTF
  len = vsnprintf((char*)0, 0, format, argp);
  msg_string = malloc(len+1);
  vsnprintf(msg_string, len+1, format, argp);
#else
  vasprintf(&msg_string, format, argp);
#endif

  va_end(argp);
  
  if(!opt || !opt->log_callback)
    {
    fprintf(stderr, "[%s] %s: %s\n",
            domain,
            log_level_to_string(level),
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
