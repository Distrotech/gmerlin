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

void bg_log(bg_log_level_t level, const char * domain,
            const char * format, ...)
  {
  char ** lines;
  int i;
  char * msg_string;
  bg_msg_t * msg;
  FILE * out = stderr;
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
  free(msg_string);
  }

void bg_log_set_dest(bg_msg_queue_t * q)
  {
  log_queue = q;
  }

void bg_log_set_verbose(int mask)
  {
  log_mask = mask;
  }
