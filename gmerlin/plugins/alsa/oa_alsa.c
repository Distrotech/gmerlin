/*****************************************************************
 
  oa_alsa.c
 
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

/* Playback modes */

#define PLAYBACK_NONE       0
#define PLAYBACK_GENERIC    1
#define PLAYBACK_SURROUND40 2
#define PLAYBACK_SURROUND41 3
#define PLAYBACK_SURROUND50 4
#define PLAYBACK_SURROUND51 5


static bg_parameter_info_t global_parameters[] =
  {
    {
      name:        "surround40",
      long_name:   "Enable 4.0 Surround",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    {
      name:        "surround41",
      long_name:   "Enable 4.1 Surround",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    {
      name:        "surround50",
      long_name:   "Enable 5.0 Surround",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    {
      name:        "surround51",
      long_name:   "Enable 5.1 Surround",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
  };

static int num_global_parameters =
  sizeof(global_parameters)/sizeof(global_parameters[0]);



typedef struct
  {
  bg_parameter_info_t * parameters;
  gavl_audio_format_t format;
  snd_pcm_t * pcm;

  /* Configuration stuff */

  int enable_surround40;
  int enable_surround41;
  int enable_surround50;
  int enable_surround51;
  
  int card_index;

  char * error_msg;
  } alsa_t;

static void * create_alsa()
  {
  int i;
  alsa_t * ret = calloc(1, sizeof(*ret));
  
  ret->parameters = calloc(num_global_parameters+2, sizeof(*ret->parameters));
  
  bg_alsa_create_card_parameters(ret->parameters);
  
  for(i = 0; i < num_global_parameters; i++)
    {
    bg_parameter_info_copy(&(ret->parameters[i+1]), &global_parameters[i]);
    }
  
  return ret;
  }

static int start_alsa(void * data)
  {
  alsa_t * priv = (alsa_t*)(data);
  //  fprintf(stderr, "start_alsa\n");

  if(snd_pcm_prepare(priv->pcm) < 0)
    return 0;
  snd_pcm_start(priv->pcm);
  return 1;
  }

static void stop_alsa(void * data)
  {
  alsa_t * priv = (alsa_t*)(data);
  //  fprintf(stderr, "stop_alsa: ");
  snd_pcm_drop(priv->pcm);

  //  fprintf(stderr, "%s\n", snd_pcm_state_name(snd_pcm_state(priv->pcm)));
  }

static int open_alsa(void * data, gavl_audio_format_t * format)
  {
  int playback_mode;
  int num_front_channels;
  int num_rear_channels;
  
  char * card = (char*)0;
  alsa_t * priv = (alsa_t*)(data);
  
  /* Figure out the right channel setup */
  
  num_front_channels = gavl_front_channels(format);
  num_rear_channels = gavl_rear_channels(format);

  playback_mode = PLAYBACK_NONE;
  
  if(num_front_channels > 2)
    {
    if(format->lfe)
      {
      if(priv->enable_surround51)
        playback_mode = PLAYBACK_SURROUND51;
      else if(priv->enable_surround50)
        playback_mode = PLAYBACK_SURROUND50;
      }
    else if(priv->enable_surround50)
      playback_mode = PLAYBACK_SURROUND50;
    }
  
  else if((playback_mode == PLAYBACK_NONE) && num_rear_channels)
    {
    if(format->lfe)
      {
      if(priv->enable_surround41)
        playback_mode = PLAYBACK_SURROUND41;
      else if(priv->enable_surround40)
        playback_mode = PLAYBACK_SURROUND40;
      }
    else if(priv->enable_surround40)
      playback_mode = PLAYBACK_SURROUND40;
    }

  if(playback_mode == PLAYBACK_NONE)
    playback_mode = PLAYBACK_GENERIC;

  switch(playback_mode)
    {
    case PLAYBACK_GENERIC:
      if(format->num_channels > 2)
        format->num_channels = 2;
      format->lfe = 0;
      format->channel_locations[0] = GAVL_CHID_NONE;
      format->channel_setup = (format->num_channels > 1) ?
        GAVL_CHANNEL_STEREO : GAVL_CHANNEL_MONO;
      gavl_set_channel_setup(format);
      card = bg_sprintf("hw:%d,0", priv->card_index);

      //      fprintf(stderr, "Playback mode: generic\n");

      break;
    case PLAYBACK_SURROUND40:
      format->num_channels = 4;
      format->channel_setup = GAVL_CHANNEL_2F2R;
      format->lfe = 0;

      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[2] = GAVL_CHID_REAR_LEFT;
      format->channel_locations[3] = GAVL_CHID_REAR_RIGHT;

      card = bg_sprintf("surround40");

      //      fprintf(stderr, "Playback mode: surround40\n");
      
      break;
    case PLAYBACK_SURROUND41:
      format->num_channels = 5;
      format->channel_setup = GAVL_CHANNEL_2F2R;
      format->lfe = 1;

      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[2] = GAVL_CHID_REAR_LEFT;
      format->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
      format->channel_locations[4] = GAVL_CHID_LFE;

      card = bg_sprintf("surround41");

      //      fprintf(stderr, "Playback mode: surround41\n");
      
      break;
    case PLAYBACK_SURROUND50:
      format->num_channels = 5;
      format->channel_setup = GAVL_CHANNEL_3F2R;
      format->lfe = 0;

      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[2] = GAVL_CHID_REAR_LEFT;
      format->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
      format->channel_locations[4] = GAVL_CHID_FRONT_CENTER;

      card = bg_sprintf("surround50");
      //      fprintf(stderr, "Playback mode: surround50\n");
      
      break;
    case PLAYBACK_SURROUND51:
      format->num_channels = 6;
      format->channel_setup = GAVL_CHANNEL_3F2R;
      format->lfe = 1;

      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[2] = GAVL_CHID_REAR_LEFT;
      format->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
      format->channel_locations[4] = GAVL_CHID_FRONT_CENTER;
      format->channel_locations[5] = GAVL_CHID_LFE;

      card = bg_sprintf("surround51");
      //      fprintf(stderr, "Playback mode: surround51\n");
      break;
    }

  //  fprintf(stderr, "Opening card %s...", card);
    
  priv->pcm = bg_alsa_open_write(card, format, &priv->error_msg);
#if 0  
  if(priv->pcm)
    fprintf(stderr, "done\n");
  else
    fprintf(stderr, "failed\n");
#endif
  free(card);
  
  if(!priv->pcm)
    return 0;
  gavl_audio_format_copy(&(priv->format), format);
  return 1;
  }

static void close_alsa(void * p)
  {
  alsa_t * priv = (alsa_t*)(p);
  //  fprintf(stderr, "close_alsa\n");
  if(priv->pcm)
    {
    snd_pcm_close(priv->pcm);
    priv->pcm = NULL;
    }
  if(priv->error_msg)
    {
    free(priv->error_msg);
    priv->error_msg = NULL;
    }
  }


static void write_frame_alsa(void * p, gavl_audio_frame_t * f)
  {
  int result = -1;
  alsa_t * priv = (alsa_t*)(p);
  
  //  fprintf(stderr, "PCM: %p\n", priv->pcm);

  while(result <= 0)
    {
    if(priv->format.interleave_mode == GAVL_INTERLEAVE_ALL)
      {
      result = snd_pcm_writei(priv->pcm,
                              f->samples.s_8,
                              f->valid_samples);
      }
    else if(priv->format.interleave_mode == GAVL_INTERLEAVE_NONE)
      {
      result = snd_pcm_writen(priv->pcm,
                              (void**)(f->channels.s_8),
                              f->valid_samples);
      }
    if(result == -EPIPE)
      {
      //    fprintf(stderr, "Warning: Buffer underrun\n");
      //    snd_pcm_drop(priv->pcm);
      if(snd_pcm_prepare(priv->pcm) < 0)
        return;
      }
    else if(result < 0)
      {
      fprintf(stderr, "snd_pcm_write returned %s\n", snd_strerror(result));
      }
    }
  }

static void destroy_alsa(void * p)
  {
  alsa_t * priv = (alsa_t*)(p);
  close_alsa(priv);

  if(priv->parameters)
    bg_parameter_info_destroy_array(priv->parameters);


  free(priv);
  }

static bg_parameter_info_t *
get_parameters_alsa(void * p)
  {
  alsa_t * priv = (alsa_t*)(p);
  return priv->parameters;
  }

static const char * get_error_alsa(void* p)
  {
  alsa_t * priv = (alsa_t*)(p);
  return priv->error_msg;
  }

static int get_delay_alsa(void * p)
  {
  int result;
  snd_pcm_sframes_t frames;
  alsa_t * priv;
  priv = (alsa_t*)(p); 
  result = snd_pcm_delay(priv->pcm, &frames);
  if(!result)
    return frames;
  return 0;
  }
/* Set parameter */

static void
set_parameter_alsa(void * p, char * name, bg_parameter_value_t * val)
  {
  alsa_t * priv = (alsa_t*)(p);
  if(!name)
    return;

  if(!strcmp(name, "surround40"))
    {
    priv->enable_surround40 = val->val_i;
    }
  else if(!strcmp(name, "surround41"))
    {
    priv->enable_surround41 = val->val_i;
    }
  else if(!strcmp(name, "surround50"))
    {
    priv->enable_surround50 = val->val_i;
    }
  else if(!strcmp(name, "surround51"))
    {
    priv->enable_surround51 = val->val_i;
    }
  else if(!strcmp(name, "card"))
    {
    priv->card_index = 0;

    if(val->val_str)
      {
      while(strcmp(priv->parameters[0].multi_names[priv->card_index],
                   val->val_str))
        priv->card_index++;
      }
    //    fprintf(stderr, "Card index: %d\n", priv->card_index);
    }
  }

bg_oa_plugin_t the_plugin =
  {
    common:
    {
      name:          "oa_alsa",
      long_name:     "ALSA output driver",
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      type:          BG_PLUGIN_OUTPUT_AUDIO,
      flags:         BG_PLUGIN_PLAYBACK,
      create:        create_alsa,
      destroy:       destroy_alsa,
      
      get_parameters: get_parameters_alsa,
      set_parameter:  set_parameter_alsa,
      get_error:      get_error_alsa,
    },

    open:          open_alsa,
    start:         start_alsa,
    write_frame:   write_frame_alsa,
    stop:          stop_alsa,
    close:         close_alsa,
    get_delay:     get_delay_alsa,
  };
