/*****************************************************************
 
  r_ram.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>


#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

static char * plugin_name = "i_avdec";

typedef struct
  {
  int num_streams;
  char ** locations;
  int locations_alloc;
  } ram_t;

static void free_info(ram_t * ram)
  {
  int i;
  for(i = 0; i < ram->num_streams; i++)
    {
    if(ram->locations && ram->locations[i])
      free(ram->locations[i]);
    }
  if(ram->locations)
    free(ram->locations);
  }

static char * trim_string(char * str)
  {
  char * ret, *pos;
  ret = str;
  while(isspace(*ret))
    ret++;
  pos = &(ret[strlen(ret)-1]);

  while(1)
    {
    if(pos < ret)
      break;

    if(isspace(*pos))
      {
      *pos = '\0';
      pos--;
      }
    else
      break;
    }
  return ret;
  }

static int parse_ram(void * priv, const char * filename)
  {
  int fd;
  char * buffer;
  int buffer_size;
  char * str, *pos;
  ram_t * ram;
  ram = (ram_t*)(priv);
  fd = open(filename, O_RDONLY, 0);
  
  if(fd == -1)
    return 0;
  
  buffer = (char*)0;
  buffer_size = 0;
  while(1)
    {
    if(!bg_read_line_fd(fd, &buffer, &buffer_size))
      break;
    pos = strchr(buffer, '#');
    if(pos)
      *pos = '\0';
    if(*buffer == '\0')
      continue;

    str = trim_string(buffer);

    if(*str == '\0')
      continue;
    
    if(!strcmp(str, "--stop--"))
      break;
        
    if(ram->locations_alloc < ram->num_streams + 1)
      {
      ram->locations_alloc += 8;
      ram->locations = realloc(ram->locations,
                               ram->locations_alloc * sizeof(char*));
      }
    ram->locations[ram->num_streams] = bg_strdup(NULL, str);
    ram->num_streams++;
    }
  if(buffer)
    free(buffer);
  close(fd);
  return 1;
  }

static int get_num_streams_ram(void * priv)
  {
  ram_t * p;
  p = (ram_t *)priv;
  return p->num_streams;
  }

static const char * get_stream_name_ram(void * priv, int stream)
  {
  ram_t * p;
  p = (ram_t *)priv;
  return p->locations[stream];
  }

static const char * get_stream_url_ram(void * priv, int stream)
  {
  ram_t * p;
  p = (ram_t *)priv;
  return p->locations[stream];
  }

static const char * get_stream_plugin_ram(void * priv, int stream)
  {
  return plugin_name;
  }

static void * create_ram()
  {
  ram_t * p;
  p = calloc(1, sizeof(*p));
  return p;
  }

static void destroy_ram(void * data)
  {
  ram_t * p = (ram_t *)data;
  free_info(p);
  free(p);
  }

bg_redirector_plugin_t the_plugin =
  {
    common:
    {
      name:          "r_ram",
      long_name:     "RAM Redirector",
      mimetypes:     "audio/x-pn-realaudio audio/x-pn-realvideo",
      extensions:    "ram rm rpm",
      type:          BG_PLUGIN_REDIRECTOR,
      flags:         BG_PLUGIN_FILE|BG_PLUGIN_URL,
      create:        create_ram,
      destroy:       destroy_ram,
      get_parameters: NULL,
      set_parameter:  NULL,
    },
    parse:      parse_ram,
    get_num_streams:   get_num_streams_ram,
    get_stream_name:   get_stream_name_ram,
    get_stream_url:    get_stream_url_ram,
    get_stream_plugin: get_stream_plugin_ram
  };
