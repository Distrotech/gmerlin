/*****************************************************************
 
  i_oss.c
 
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
#include "oss_common.h"

#define SAMPLES_PER_FRAME 1024

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "device",
      long_name:   "Device",
      type:        BG_PARAMETER_DEVICE,
      val_default: { val_str: "/dev/dsp" },
    },
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
  char * device;
  int fd;

  /* Configuration vairables */
  
  int num_channels;
  int bytes_per_sample;
  int samplerate;

  /* Runtime variables */
    
  int bytes_per_frame;
  } oss_t;

static bg_parameter_info_t *
get_parameters_oss(void * priv)
  {
  return parameters;
  }

static void
set_parameter_oss(void * p, char * name, bg_parameter_value_t * val)
  {
  oss_t * priv = (oss_t*)(p);
  if(!name)
    return;
  if(!strcmp(name, "device"))
    {
    priv->device = bg_strdup(priv->device, val->val_str);
    }
  else if(!strcmp(name, "channel_mode"))
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



static void * create_oss()
  {
  oss_t * ret = calloc(1, sizeof(*ret));
  ret->fd = -1;
  return ret;
  }

static int open_oss(void * data,
                    gavl_audio_format_t * format)
  {
  gavl_sample_format_t sample_format;
  int test_value;
  oss_t * priv = (oss_t*)data;

  priv->fd = -1;

  memset(format, 0, sizeof(*format));
  
  /* Set up the format */

  format->interleave_mode = GAVL_INTERLEAVE_ALL;
  format->samples_per_frame = 1024;

  switch(priv->bytes_per_sample)
    {
    case 1:
      format->sample_format = GAVL_SAMPLE_U8;
      break;
    case 2:
      format->sample_format = GAVL_SAMPLE_S16;
      break;
    default:
      fprintf(stderr, "Invalid number of bits\n");
      return 0;
    }

  switch(priv->num_channels)
    {
    case 1:
      format->num_channels = 1;
      format->interleave_mode = GAVL_INTERLEAVE_NONE;
      format->channel_setup  = GAVL_CHANNEL_MONO;
      break;
    case 2:
      format->num_channels = 2;
      format->interleave_mode = GAVL_INTERLEAVE_ALL;
      format->channel_setup  = GAVL_CHANNEL_STEREO;
      break;
    default:
      fprintf(stderr, "Invalid number of channels\n");
      return 0;
    }
  
  format->samplerate = priv->samplerate;
  format->samples_per_frame = SAMPLES_PER_FRAME;
  format->lfe = 0;
  gavl_set_channel_setup(format);
  
  fprintf(stderr, "Opening device %s\n", priv->device);
  
  priv->fd = open(priv->device, O_RDONLY, 0);
  if(priv->fd == -1)
    goto fail;

  
  sample_format = bg_oss_set_sample_format(priv->fd,
                                           format->sample_format);
  
  if(sample_format == GAVL_SAMPLE_NONE)
    {
    fprintf(stderr, "Cannot set sampleformat for %s\n", priv->device);
    goto fail;
    }

  test_value =
    bg_oss_set_channels(priv->fd, format->num_channels);
  if(test_value != format->num_channels)
    {
    fprintf(stderr, "Device %s doesn't support %d channel sound\n",
            priv->device, format->num_channels);
    goto fail;
    }

  test_value =
    bg_oss_set_samplerate(priv->fd, format->samplerate);
  if(test_value != format->samplerate)
    {
    fprintf(stderr, "Samplerate %f KHz not supported by device %s\n",
              format->samplerate / 1000.0,
              priv->device);
    goto fail;
    }

  priv->bytes_per_frame = priv->bytes_per_sample * priv->num_channels;
  return 1;
  fail:
  if(priv->fd != -1)
    close(priv->fd);
  return 0;
  }

static void close_oss(void * p)
  {
  oss_t * priv = (oss_t*)(p);
  
  if(priv->fd != -1)
    {
    close(priv->fd);
    priv->fd = -1;
    }
  }

static void read_frame_oss(void * p, gavl_audio_frame_t * f, int num_samples)
  {
  oss_t * priv = (oss_t*)(p);
  
  f->valid_samples = read(priv->fd,
                          f->samples.s_8,
                          num_samples * priv->bytes_per_frame);
  f->valid_samples /= priv->bytes_per_frame;
  }

static void destroy_oss(void * p)
  {
  oss_t * priv = (oss_t*)(p);

  if(priv->device)
    free(priv->device);
  free(priv);
  }

bg_ra_plugin_t the_plugin =
  {
    common:
    {
      name:          "i_oss",
      long_name:     "OSS Recorder",
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      type:          BG_PLUGIN_RECORDER_AUDIO,
      flags:         BG_PLUGIN_RECORDER,
      create:        create_oss,
      destroy:       destroy_oss,

      get_parameters: get_parameters_oss,
      set_parameter:  set_parameter_oss
    },

    open:          open_oss,
    read_frame:    read_frame_oss,
    close:         close_oss,
  };
