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

#include <config.h>
#include <translation.h>

#include <plugin.h>
#include <utils.h>

#include <log.h>
#define LOG_DOMAIN "i_alsa"

#include "alsa_common.h"



static bg_parameter_info_t static_parameters[] =
  {
    {
      name:        "channel_mode",
      long_name:   TRS("Channel Mode"),
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Stereo" },
      multi_names:   (char*[]){ "mono", "stereo", (char*)0 },
      multi_labels:  (char*[]){ TRS("Mono"), TRS("Stereo"), (char*)0 },
    },
    {
      name:        "bits",
      long_name:   TRS("Bits"),
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "16" },
      multi_names:     (char*[]){ "8", "16", (char*)0 },
    },
    {
      name:        "samplerate",
      long_name:   TRS("Samplerate [Hz]"),
      type:        BG_PARAMETER_INT,
      val_default: { val_i: 44100 },
      val_min:     { val_i:  8000 },
      val_max:     { val_i: 96000 },
    },
    {
      name:        "buffer_time",
      long_name:   TRS("Buffer time"),
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 10    },
      val_max:     { val_i: 10000 },
      val_default: { val_i: 1000  },
      help_string: TRS("Set the buffer time (in milliseconds). Larger values \
improve recording performance on slow systems under load."),
    },
    {
      name:        "user_device",
      long_name:   TRS("User device"),
      type:        BG_PARAMETER_STRING,
      help_string: TRS("Enter a custom device to use for recording. Leave empty to use the\
 settings above"),
    },
  };

static int num_static_parameters =
  sizeof(static_parameters)/sizeof(static_parameters[0]);

typedef struct
  {
  bg_parameter_info_t * parameters;
  
  gavl_audio_format_t format;
  int num_channels;
  int bytes_per_sample;
  int samplerate;
  char * card;
  
  snd_pcm_t * pcm;

  gavl_audio_frame_t * f;
  int last_frame_size;

  gavl_time_t buffer_time;
  char * user_device;

  int64_t samples_read;
  } alsa_t;

static bg_parameter_info_t *
get_parameters_alsa(void * p)
  {
  int i;
  alsa_t * priv = (alsa_t*)(p);
  if(!priv->parameters)
    {
    priv->parameters = calloc(num_static_parameters + 2,
                              sizeof(*(priv->parameters)));

    bg_alsa_create_card_parameters(priv->parameters);
    
    for(i = 0; i < num_static_parameters; i++)
      {
      bg_parameter_info_copy(&(priv->parameters[i+1]), &static_parameters[i]);
      }
    }
  return priv->parameters;
  }

static void
set_parameter_alsa(void * p, char * name, bg_parameter_value_t * val)
  {
  alsa_t * priv = (alsa_t*)(p);


  if(!name)
    return;
  if(!strcmp(name, "channel_mode"))
    {
    if(!strcmp(val->val_str, "mono"))
      priv->num_channels = 1;
    else if(!strcmp(val->val_str, "stereo"))
      priv->num_channels = 2;
    }
  else if(!strcmp(name, "bits"))
    {
    if(!strcmp(val->val_str, "8"))
      priv->bytes_per_sample = 1;
    else if(!strcmp(val->val_str, "16"))
      priv->bytes_per_sample = 2;
    }
  else if(!strcmp(name, "buffer_time"))
    {
    priv->buffer_time = val->val_i;
    priv->buffer_time *= (GAVL_TIME_SCALE/1000);
    }
  else if(!strcmp(name, "samplerate"))
    {
    priv->samplerate = val->val_i;
    }
  else if(!strcmp(name, "card"))
    {
    priv->card = bg_strdup(priv->card, val->val_str);
    }
  else if(!strcmp(name, "user_device"))
    {
    priv->user_device = bg_strdup(priv->user_device, val->val_str);
    }
  }



static void * create_alsa()
  {
  alsa_t * ret = calloc(1, sizeof(*ret));
    
  return ret;
  }

static int open_alsa(void * data,
                    gavl_audio_format_t * format)
  {
  const char * card = (char*)0;
  alsa_t * priv = (alsa_t*)(data);
  
  if(priv->user_device)
  card = priv->user_device;
  else
  card = priv->card;

  priv->samples_read = 0;
  
  if(!card)
    card = "default";
  
  memset(format, 0, sizeof(*format));
  
  format->num_channels      = priv->num_channels;
  format->samples_per_frame = 1024;
  
  if(priv->bytes_per_sample == 1)
    format->sample_format = GAVL_SAMPLE_U8;
  else if(priv->bytes_per_sample == 2)
    format->sample_format = GAVL_SAMPLE_S16;
  format->samplerate = priv->samplerate;
    
  priv->pcm = bg_alsa_open_read(card, format, priv->buffer_time);
  
  if(!priv->pcm)
    return 0;
  gavl_audio_format_copy(&(priv->format), format);

  priv->f = gavl_audio_frame_create(&(priv->format));
    
  if(snd_pcm_prepare(priv->pcm) < 0)
    return 0;
  snd_pcm_start(priv->pcm);

  return 1;
  }

static void close_alsa(void * p)
  {
  alsa_t * priv = (alsa_t*)(p);
  if(priv->pcm)
    {
    snd_pcm_close(priv->pcm);
    priv->pcm = NULL;
    }
  if(priv->f)
    {
    gavl_audio_frame_destroy(priv->f);
    priv->f = (gavl_audio_frame_t*)0;
    }
  }


static int read_frame(alsa_t * priv)
  {
  int result = 0;

  
  while(1)
    {
    if(priv->format.interleave_mode == GAVL_INTERLEAVE_ALL)
      {
      result = snd_pcm_readi(priv->pcm,
                             priv->f->samples.s_8,
                             priv->format.samples_per_frame);
      }
    else if(priv->format.interleave_mode == GAVL_INTERLEAVE_NONE)
      {
      result = snd_pcm_readn(priv->pcm,
                             (void**)(priv->f->channels.s_8),
                             priv->format.samples_per_frame);
      }
    
    if(result > 0)
      {
      priv->f->valid_samples = result;
      priv->last_frame_size = result;
      return 1;
      }
    else if(result == -EPIPE)
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Dropping samples");
      snd_pcm_drop(priv->pcm);
      if(snd_pcm_prepare(priv->pcm) < 0)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_prepare failed");
        return 0;
        }
      snd_pcm_start(priv->pcm);
      }
    else
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Unknown error");
      break;
      }
    }
  return 0;
  }

static void read_frame_alsa(void * p, gavl_audio_frame_t * f,
                            int num_samples)
  {
  int samples_read;
  int samples_copied;

  alsa_t * priv = (alsa_t*)(p);

  samples_read = 0;

  while(samples_read < num_samples)
    {
    if(!priv->f->valid_samples)
      {
      read_frame(priv);
      }
    samples_copied =
      gavl_audio_frame_copy(&priv->format,                                  /* format  */
                            f,                                              /* dst     */
                            priv->f,                                        /* src     */
                            samples_read,                                   /* dst_pos */
                            priv->last_frame_size - priv->f->valid_samples, /* src_pos */
                            num_samples - samples_read,                     /* dst_size */
                            priv->f->valid_samples                          /* src_size */ );
    priv->f->valid_samples -= samples_copied;
    samples_read += samples_copied;
    }

  if(f)
    {
    f->valid_samples = samples_read;
    f->timestamp = priv->samples_read;
    }
  priv->samples_read += samples_read;
  }

static void destroy_alsa(void * p)
  {
  alsa_t * priv = (alsa_t*)(p);
  if(priv->parameters)
    bg_parameter_info_destroy_array(priv->parameters);
  free(priv);
  }

bg_ra_plugin_t the_plugin =
  {
    common:
    {
      BG_LOCALE,
      name:          "i_alsa",
      long_name:     TRS("Alsa"),
      description:   TRS("Alsa recorder"),
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      type:          BG_PLUGIN_RECORDER_AUDIO,
      flags:         BG_PLUGIN_RECORDER,
      priority:      BG_PLUGIN_PRIORITY_MAX,
      create:        create_alsa,
      destroy:       destroy_alsa,

      get_parameters: get_parameters_alsa,
      set_parameter:  set_parameter_alsa,
    },

    open:          open_alsa,
    read_frame:    read_frame_alsa,
    close:         close_alsa,
  };
/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
