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
  int esd_format;
  esd_t * e = (esd_t *)data;
  e->bytes_per_sample = 1;

  format->interleave_mode = GAVL_INTERLEAVE_ALL;
    
  esd_format = ESD_STREAM | ESD_PLAY;
  
  /* Delete unwanted channels */

  switch(format->channel_setup)
    {
    case GAVL_CHANNEL_MONO:
      esd_format |= ESD_MONO;
      format->num_channels = 1;
      break;
    default:
      e->bytes_per_sample *= 2;
      esd_format |= ESD_STEREO;
      format->channel_setup = GAVL_CHANNEL_STEREO;
      format->num_channels = 2;
      break;
    }
  format->lfe = 0;

  /* Set bits */

  if(gavl_bytes_per_sample(format->sample_format)==1)
    {
    format->sample_format = GAVL_SAMPLE_U8;
    esd_format |= ESD_BITS8;
    }
  else
    {
    format->sample_format = GAVL_SAMPLE_S16;
    esd_format |= ESD_BITS16;
    e->bytes_per_sample *= 2;
    }

  /* Open stream */
  
  if(!e->hostname || (*(e->hostname) == '\0'))
    {
    fprintf(stderr, "Opening local esd\n");
    e->esd_socket = esd_play_stream(esd_format,
                                    format->samplerate,
                                    (char*)0,
                                    "gmerlin output");
    if(e->esd_socket < 0)
      return 0;
    }
  else
    {
    fprintf(stderr, "Opening remote esd\n");
    e->esd_socket = esd_play_stream(esd_format,
                                    format->samplerate,
                                    e->hostname,
                                    "gmerlin output");
    if(e->esd_socket < 0)
      return 0;
    }
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
  esd_t * e = (esd_t*)(p);
  esd_close(e->esd_socket);
  
  }


bg_parameter_info_t *
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
      create:        create_esd,
      destroy:       destroy_esd,

      get_parameters: get_parameters_esd,
      set_parameter:  set_parameter_esd
    },

    open:          open_esd,
    write_frame:   write_esd,
    close:         close_esd,
  };
