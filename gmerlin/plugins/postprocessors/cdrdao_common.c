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
#include <plugin.h>
#include <utils.h>
#include <log.h>
#include "cdrdao_common.h"

#define LOG_DOMAIN "cdrdao"

struct bg_cdrdao_s
  {
  int run;
  char * device;
  char * driver;
  int eject;
  int simulate;
  };

bg_cdrdao_t * bg_cdrdao_create()
  {
  bg_cdrdao_t * ret;
  ret = calloc(1, sizeof(*ret));
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
  if(!name)
    return;
  bg_cdrdao_t * c;
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
  }

/* Read line without trailing '\r' or '\n' */
static int read_line(FILE * in, char ** ret, int * ret_alloc)
  {
  int bytes_read = 0;
  char c = 0;
  while((c != '\n') && (c != '\r'))
    {
    c = fgetc(in);
    if(feof(in))
      return 0;

    if((c != '\n') && (c != '\r'))
      {
      if(bytes_read + 1 > *ret_alloc)
        {
        *ret_alloc += 256;
        *ret = realloc(*ret, *ret_alloc);
        }
      (*ret)[bytes_read] = c;
      bytes_read++;
      }
    }

  if(bytes_read + 1 > *ret_alloc)
    {
    *ret_alloc += 256;
    *ret = realloc(*ret, *ret_alloc);
    }
  ret[bytes_read] = '\0';
  return 1;
  }

void bg_cdrdao_run(bg_cdrdao_t * c, const char * toc_file)
  {
  FILE * cdrdao;
  char * str;
  char * commandline = (char*)0;

  char * line;
  int line_alloc = 0;
  
  if(!c->run)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Not running cdrdao (disabled by user)");
    return;
    }
  if(!bg_search_file_exec("cdrdao", &commandline))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "cdrdao executable not found");
    return;
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
  /* Simulate */
  if(c->simulate)
    commandline = bg_strcat(commandline, " --simulate");

  /* TOC-File and stderr redirection */
  str = bg_sprintf(" %s 2>&1", toc_file);
  commandline = bg_strcat(commandline, str);
  free(str);
  
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Launching %s", commandline);
  
  /* Launching command */
  cdrdao = popen(commandline, "r");

  /* Read lines */

  while(read_line(cdrdao, &line, &line_alloc))
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, line);
    fprintf(stderr, "Got line: %s\n", line);
    }
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "cdrdao process finished");
  pclose(cdrdao);
  }
