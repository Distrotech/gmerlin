/*****************************************************************
 
  i_alsa.c
 
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

#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <plugin.h>
#include <utils.h>
#include "alsa_common.h"

#define SAMPLES_PER_FRAME 1024

static bg_parameter_info_t per_card_parameters[] =
  {
    {
      name:        "channel_mode",
      long_name:   "Channel Mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Stereo" },
      options:     (char*[]){ "Mono", "Stereo", (char*)0 },
    },
    {
      name:        "bits",
      long_name:   "Bits",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "16" },
      options:     (char*[]){ "8", "16", (char*)0 },
    },
    {
      name:        "samplerate",
      long_name:   "Samplerate [Hz]",
      type:        BG_PARAMETER_INT,
      val_default: { val_i: 44100 },
      val_min:     { val_i:  8000 },
      val_max:     { val_i: 96000 },
    },
    { /* End of parameters */ }
  };

typedef struct
  {
  bg_parameter_info_t parameters[2];
  
  gavl_audio_format_t format;
  int num_channels;
  int bytes_per_sample;
  int samplerate;
  } alsa_t;

static bg_parameter_info_t *
get_parameters_alsa(void * p)
  {
  alsa_t * priv = (alsa_t*)(p);
  return priv->parameters;
  }

static void
set_parameter_alsa(void * p, char * name, bg_parameter_value_t * val)
  {
  alsa_t * priv = (alsa_t*)(p);

  fprintf(stderr, "Set parameter %s\n", name);

  if(!name)
    return;
  if(!strcmp(name, "channel_mode"))
    {
    if(!strcmp(val->val_str, "Mono"))
      priv->num_channels = 1;
    else if(!strcmp(val->val_str, "Stereo"))
      priv->num_channels = 2;
    }
  else if(!strcmp(name, "bits"))
    {
    if(!strcmp(val->val_str, "8"))
      priv->bytes_per_sample = 1;
    else if(!strcmp(val->val_str, "16"))
      priv->bytes_per_sample = 2;
    }
  else if(!strcmp(name, "samplerate"))
    {
    priv->samplerate = val->val_i;
    }
  }



static void * create_alsa()
  {
  alsa_t * ret = calloc(1, sizeof(*ret));

  bg_alsa_create_card_parameters(ret->parameters);
  
  return ret;
  }

static int open_alsa(void * data,
                    gavl_audio_format_t * format)
  {
  //  alsa_t * priv = (alsa_t*)data;
  return 0;
  }

static void close_alsa(void * p)
  {
  //  alsa_t * priv = (alsa_t*)(p);
  
  }

static void read_frame_alsa(void * p, gavl_audio_frame_t * f)
  {
  //  alsa_t * priv = (alsa_t*)(p);
  
  }

static void destroy_alsa(void * p)
  {
  alsa_t * priv = (alsa_t*)(p);
  free(priv);
  }

bg_ra_plugin_t the_plugin =
  {
    common:
    {
      name:          "i_alsa",
      long_name:     "ALSA Recorder",
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      type:          BG_PLUGIN_RECORDER_AUDIO,
      flags:         BG_PLUGIN_RECORDER,
      create:        create_alsa,
      destroy:       destroy_alsa,

      get_parameters: get_parameters_alsa,
      set_parameter:  set_parameter_alsa
    },

    open:          open_alsa,
    read_frame:    read_frame_alsa,
    close:         close_alsa,
  };
