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

  int surround40;
  int surround41;
  int surround50;
  int surround51;
  
  int card_index;
  
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
  fprintf(stderr, "start_alsa\n");

  if(snd_pcm_prepare(priv->pcm) < 0)
    return 0;
  snd_pcm_start(priv->pcm);
  return 1;
  }

static void stop_alsa(void * data)
  {
  alsa_t * priv = (alsa_t*)(data);
  fprintf(stderr, "stop_alsa: ");
  snd_pcm_drop(priv->pcm);

  fprintf(stderr, "%s\n", snd_pcm_state_name(snd_pcm_state(priv->pcm)));
  }

static int open_alsa(void * data, gavl_audio_format_t * format)
  {
  char * card = (char*)0;
  alsa_t * priv = (alsa_t*)(data);

  card = bg_sprintf("hw:%d,0", priv->card_index);
  
  /* Check, which card we want to have */
#if 0
  switch(format->channel_setup)
    {
    
    }
#endif
  priv->pcm = bg_alsa_open_write(card, format);
  
  free(card);
  
  if(!priv->pcm)
    return 0;
  gavl_audio_format_copy(&(priv->format), format);
  return 1;
  }

static void close_alsa(void * p)
  {
  alsa_t * priv = (alsa_t*)(p);
  fprintf(stderr, "close_alsa\n");
  if(priv->pcm)
    {
    snd_pcm_close(priv->pcm);
    priv->pcm = NULL;
    }
  }


static void write_frame_alsa(void * p, gavl_audio_frame_t * f)
  {
  alsa_t * priv = (alsa_t*)(p);

  //  fprintf(stderr, "PCM: %p\n", priv->pcm);
  if(priv->format.interleave_mode == GAVL_INTERLEAVE_ALL)
    {
    snd_pcm_writei(priv->pcm,
                   f->samples.s_8,
                   f->valid_samples);
    }
  else if(priv->format.interleave_mode == GAVL_INTERLEAVE_NONE)
    {
    snd_pcm_writen(priv->pcm,
                   (void**)(f->channels.s_8),
                   f->valid_samples);
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
    priv->surround40 = val->val_i;
    }
  else if(!strcmp(name, "surround41"))
    {
    priv->surround41 = val->val_i;
    }
  else if(!strcmp(name, "surround50"))
    {
    priv->surround50 = val->val_i;
    }
  else if(!strcmp(name, "surround51"))
    {
    priv->surround51 = val->val_i;
    }
  else if(!strcmp(name, "card"))
    {
    priv->card_index = 0;

    if(val->val_str)
      {
      while(strcmp(priv->parameters[0].options[priv->card_index],
                   val->val_str))
        priv->card_index++;
      }
    fprintf(stderr, "Card index: %d\n", priv->card_index);
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
      set_parameter:  set_parameter_alsa
    },

    open:          open_alsa,
    start:         start_alsa,
    write_frame:   write_frame_alsa,
    stop:          stop_alsa,
    close:         close_alsa,
    get_delay:     get_delay_alsa,
  };
