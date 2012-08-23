/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * NoiseTV - make incoming objects noisy.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <gmerlin_effectv.h>
#include "utils.h"

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dest);


typedef struct
  {
  int stat;
  int bgIsSet;
  
  } noise_t;

static int setBackground(effect * e, RGB32 *src)
  {
  noise_t * priv = e->priv;
  image_bgset_y(e, src);
  priv->bgIsSet = 1;
  
  return 0;
  }

static effect *noiseRegister(void)
  {
  effect *entry;
  noise_t * priv;
  
  entry = calloc(1, sizeof(effect));
  if(entry == NULL)
    {
    return NULL;
    }
  priv = calloc(1, sizeof(*priv));
  entry->priv = priv;
  entry->start = start;
  entry->stop = stop;
  entry->draw = draw;
  
  return entry;
  }

static int start(effect * e)
  {
  noise_t * priv = e->priv;
  image_init(e);
  image_set_threshold_y(e, 40);
  priv->bgIsSet = 0;
  
  priv->stat = 1;
  return 0;
  }

static int stop(effect * e)
  {
  noise_t * priv = e->priv;
  priv->stat = 0;
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  unsigned char *diff;
  noise_t * priv = e->priv;
  
  if(!priv->bgIsSet) {
  setBackground(e, src);
  }
  
	diff = image_diff_filter(e, image_bgsubtract_y(e, src));
	for(y=e->video_height; y>0; y--) {
		for(x=e->video_width; x>0; x--) {
			if(*diff++) {
				*dest = 0 - ((inline_fastrand(e)>>31) & y);
			} else {
				*dest = *src;
			}
			src++;
			dest++;
		}
	}

	return 0;
}


static void * create_noisetv()
  {
  return bg_effectv_create(noiseRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_noisetv",
      .long_name = TRS("NoiseTV"),
      .description = TRS("Black & White noise is rendered over the incoming objects. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_noisetv,
      .destroy =   bg_effectv_destroy,
      //      .get_parameters =   get_parameters,
      //      .set_parameter =    set_parameter,
      .priority =         1,
    },
    
    .connect = bg_effectv_connect,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
