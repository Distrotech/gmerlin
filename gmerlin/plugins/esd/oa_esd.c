/*****************************************************************
 
  oa_esd.c
 
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <plugin.h>
#include <utils.h>
#include <esd.h>

typedef struct
  {
  int esd_socket;
  char * hostname;
  int bytes_per_sample;
  int esd_format;
  int samplerate;
  } esd_t;

static bg_parameter_info_t parameters[] =
  {
    {
      name:      "esd_host",
      long_name: "Host (empty: local)",
      type:      BG_PARAMETER_STRING,
    },
    { /* End of parameters */ }
  };

static void set_parameter_esd(void * data, char * name,
                          bg_parameter_value_t * val)
  {
  esd_t * e = (esd_t *)data;

  if(!name)
    return;
  
  if(!strcmp(name, "esd_host"))
    {
    e->hostname = bg_strdup(e->hostname, val->val_str);
    }
  }

static void * create_esd()
  {
  esd_t * ret = calloc(1, sizeof(*ret));

  return ret;
  }

static void destroy_esd(void *data)
  {
  esd_t * e = (esd_t *)data;

  if(e->hostname)
    free(e->hostname);
  free(e);
  }

static int open_esd(void * data, gavl_audio_format_t * format)
  {
  esd_t * e = (esd_t *)data;
  e->bytes_per_sample = 1;

  format->interleave_mode = GAVL_INTERLEAVE_ALL;
    
  e->esd_format = ESD_STREAM | ESD_PLAY;
  
  /* Delete unwanted channels */

  switch(format->num_channels)
    {
    case 1:
      e->esd_format |= ESD_MONO;
      format->num_channels = 1;
      format->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      break;
    default:
      e->bytes_per_sample *= 2;
      e->esd_format |= ESD_STEREO;
      format->num_channels = 2;
      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      break;
    }
  /* Set bits */
  
  if(gavl_bytes_per_sample(format->sample_format)==1)
    {
    format->sample_format = GAVL_SAMPLE_U8;
    e->esd_format |= ESD_BITS8;
    }
  else
    {
    format->sample_format = GAVL_SAMPLE_S16;
    e->esd_format |= ESD_BITS16;
    e->bytes_per_sample *= 2;
    }

  e->samplerate = format->samplerate;
  
  return 1;
  }

// static int stream_count = 0;

static int start_esd(void * p)
  {
  esd_t * e = (esd_t *)p;
//  char * stream_name;
//   fprintf(stderr,"start_esd...");
//  stream_name = bg_sprintf("gmerlin_output%08x", stream_count++);
  e->esd_socket = esd_play_stream(e->esd_format,
                                  e->samplerate,
                                  e->hostname,
                                  "gmerlin output");
//  fprintf(stderr,"done\n");
//  free(stream_name);
  if(e->esd_socket < 0)
    return 0;
  return 1;
  }

static void write_esd(void * p, gavl_audio_frame_t * f)
  {
  esd_t * e = (esd_t*)(p);
  write(e->esd_socket, f->channels.s_8[0], f->valid_samples *
        e->bytes_per_sample);
  }

static void close_esd(void * p)
  {

  }

static void stop_esd(void * p)
  {
  esd_t * e = (esd_t*)(p);
  esd_close(e->esd_socket);
  }

static bg_parameter_info_t *
get_parameters_esd(void * priv)
  {
  return parameters;
  }

bg_oa_plugin_t the_plugin =
  {
    common:
    {
      name:          "oa_esd",
      long_name:     "EsounD output driver",
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      type:          BG_PLUGIN_OUTPUT_AUDIO,
      flags:         BG_PLUGIN_PLAYBACK,
      priority:      BG_PLUGIN_PRIORITY_MIN,
      create:        create_esd,
      destroy:       destroy_esd,

      get_parameters: get_parameters_esd,
      set_parameter:  set_parameter_esd
    },
    open:          open_esd,
    start:         start_esd,
    write_frame:   write_esd,
    stop:          stop_esd,
    close:         close_esd,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
