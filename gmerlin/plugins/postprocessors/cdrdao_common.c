/*****************************************************************
 
  cdrdao_common.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <pthread.h>
#include <signal.h>


#include <plugin.h>
#include <utils.h>
#include <log.h>
#include <subprocess.h>
#include "cdrdao_common.h"

#define LOG_DOMAIN "cdrdao"

struct bg_cdrdao_s
  {
  int run;
  char * device;
  char * driver;
  int eject;
  int simulate;
  int speed;
  int nopause;
  bg_e_pp_callbacks_t * callbacks;
  pthread_mutex_t stop_mutex;
  int do_stop;
  };

bg_cdrdao_t * bg_cdrdao_create()
  {
  bg_cdrdao_t * ret;
  ret = calloc(1, sizeof(*ret));
  pthread_mutex_init(&ret->stop_mutex, (pthread_mutexattr_t *)0);
  return ret;
  }

void bg_cdrdao_destroy(bg_cdrdao_t * cdrdao)
  {
  if(cdrdao->device)
    free(cdrdao->device);
  free(cdrdao);
  }

void bg_cdrdao_set_parameter(void * data, char * name,
                             bg_parameter_value_t * val)
  {
  bg_cdrdao_t * c;
  if(!name)
    return;
  c = (bg_cdrdao_t*)(data);
  if(!strcmp(name, "cdrdao_run"))
    c->run = val->val_i;
  else if(!strcmp(name, "cdrdao_device"))
    c->device = bg_strdup(c->device, val->val_str);
  else if(!strcmp(name, "cdrdao_driver"))
    c->driver = bg_strdup(c->driver, val->val_str);
  else if(!strcmp(name, "cdrdao_eject"))
    c->eject = val->val_i;
  else if(!strcmp(name, "cdrdao_simulate"))
    c->simulate = val->val_i;
  else if(!strcmp(name, "cdrdao_speed"))
    c->speed = val->val_i;
  else if(!strcmp(name, "cdrdao_nopause"))
    c->nopause = val->val_i;
  }

static int check_stop(bg_cdrdao_t * c)
  {
  int ret;
  pthread_mutex_lock(&c->stop_mutex);
  ret = c->do_stop;
  c->do_stop = 0;
  pthread_mutex_unlock(&c->stop_mutex);
  return ret;
  }

int bg_cdrdao_run(bg_cdrdao_t * c, const char * toc_file)
  {
  bg_subprocess_t * cdrdao;
  char * str;
  char * commandline = (char*)0;

  char * line = (char*)0;
  int line_alloc = 0;
  
  int mb_written, mb_total;
  
  if(!c->run)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Not running cdrdao (disabled by user)");
    return 0;
    }
  if(!bg_search_file_exec("cdrdao", &commandline))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "cdrdao executable not found");
    return 0;
    }
  commandline = bg_strcat(commandline, " write");
  
  /* Device */
  if(c->device)
    {
    str = bg_sprintf(" --device %s", c->device);
    commandline = bg_strcat(commandline, str);
    free(str);
    }
  /* Driver */
  if(c->driver)
    {
    str = bg_sprintf(" --driver %s", c->driver);
    commandline = bg_strcat(commandline, str);
    free(str);
    }
  /* Eject */
  if(c->eject)
    commandline = bg_strcat(commandline, " --eject");
  /* Skip pause */
  if(c->nopause)
    commandline = bg_strcat(commandline, " -n");

  /* Simulate */
  if(c->simulate)
    commandline = bg_strcat(commandline, " --simulate");

  /* Speed */
  if(c->speed > 0)
    {
    str = bg_sprintf(" --speed %d", c->speed);
    commandline = bg_strcat(commandline, str);
    free(str);
    }
  
  /* TOC-File and stderr redirection */
  str = bg_sprintf(" \"%s\"", toc_file);
  commandline = bg_strcat(commandline, str);
  free(str);
  
  if(check_stop(c))
    {
    free(commandline);
    return 0;
    }

  /* Launching command (cdrdao sends everything to stderr) */
  cdrdao = bg_subprocess_create(commandline, 0, 0, 1);
  free(commandline);
  /* Read lines */

  while(bg_subprocess_read_line(cdrdao->stderr, &line, &line_alloc, -1))
    {
    if(check_stop(c))
      {
      bg_subprocess_kill(cdrdao, SIGQUIT);
      bg_subprocess_close(cdrdao);
      return 0;
      }

    if(!strncmp(line, "ERROR", 5))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, line);	   
      //      break;
      }
    else if(!strncmp(line, "WARNING", 7))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, line);	   
      //      break;
      }
    else if(!strncmp(line, "Writing", 7))
      {
      if(c->callbacks && c->callbacks->action_callback)
        c->callbacks->action_callback(c->callbacks->data,
                                      line);
      bg_log(BG_LOG_INFO, LOG_DOMAIN, line);

      if(c->callbacks && c->callbacks->progress_callback)
        {
        if(!strncmp(line, "Writing track 01", 16) ||
           strncmp(line, "Writing track", 13))
          c->callbacks->progress_callback(c->callbacks->data, 0.0);
        }
      }
    else if(sscanf(line, "Wrote %d of %d", &mb_written, &mb_total) == 2)
      {
      if(c->callbacks && c->callbacks->progress_callback)
        c->callbacks->progress_callback(c->callbacks->data,
                                        (float)mb_written/(float)mb_total);
      else
        bg_log(BG_LOG_INFO, LOG_DOMAIN, line);
      }
    else
      bg_log(BG_LOG_INFO, LOG_DOMAIN, line);
    }
  bg_subprocess_close(cdrdao);

  if(c->simulate)
    return 0;
  else
    return 1;
  }

void bg_cdrdao_set_callbacks(bg_cdrdao_t * c, bg_e_pp_callbacks_t * callbacks)
  {
  c->callbacks = callbacks;
  }

void bg_cdrdao_stop(bg_cdrdao_t * c)
  {
  pthread_mutex_lock(&c->stop_mutex);
  c->do_stop = 1;
  pthread_mutex_unlock(&c->stop_mutex);
  
  }
