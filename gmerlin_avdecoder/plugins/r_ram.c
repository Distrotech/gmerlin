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

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

static char * plugin_name = "i_avdec";

typedef struct
  {
  int num_streams;
  char ** locations;
  char ** names;
  } ram_t;

static void free_info(ram_t * ram)
  {
  int i;
  for(i = 0; i < ram->num_streams; i++)
    {
    if(ram->names && ram->names[i])
      free(ram->names[i]);
    if(ram->locations && ram->locations[i])
      free(ram->locations[i]);
    }
  if(ram->locations)
    free(ram->locations);
  if(ram->names)
    free(ram->names);
  }

static int parse_ram(void * priv, const char * filename)
  {
  return 0;
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
  return p->names[stream];
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
      extensions:    "ram",
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
