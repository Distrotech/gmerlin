/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentarou
 *
 * BrokenTV - simulate broken VTR.
 * Copyright (C) 2002 Jun IIO
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include <utils.h>

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dest);

#define SCROLL_STEPS	30


typedef struct
  {
  int state;
  int offset;
  } brokentv_t;

static void add_noise (effect*, RGB32 *dest);

static effect *scrollRegister(void)
{
	effect *entry;
        brokentv_t * priv;
        
	entry = (effect *)calloc(1, sizeof(effect));
	if(entry == NULL) return NULL;

        priv = calloc(1, sizeof(*priv));
        entry->priv = priv;
	
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;

	return entry;
}

static int start(effect * e)
{
        brokentv_t * priv = (brokentv_t *)e->priv;
	priv->offset = 0;
	priv->state = 1;

	return 0;
}

static int stop(effect * e)
{
        brokentv_t * priv = (brokentv_t *)e->priv;
	priv->state = 0;
	return 0;
}

static int draw(effect * e, RGB32 *src, RGB32 *dest)
{
        brokentv_t * priv = (brokentv_t *)e->priv;
	memcpy (dest, src+(e->video_height - priv->offset)*e->video_width, 
		priv->offset * e->video_width * sizeof (RGB32));
	memcpy (dest+priv->offset*e->video_width, src,
		(e->video_height - priv->offset) * e->video_width * sizeof (RGB32));
	add_noise (e, dest);

	priv->offset += SCROLL_STEPS;
	if (priv->offset >= e->video_height)
          priv->offset = 0;
        
	return 0;
}

void add_noise (effect * e, RGB32 *dest)
{
        brokentv_t * priv = (brokentv_t *)e->priv;
	int i, x, y, dy;

	for (y = priv->offset, dy = 0; ((dy < 3) && (y < e->video_height)); y++, dy++) {
		i = y * e->video_width;
		for (x = 0; x < e->video_width; x++, i++) {
			if ((dy == 2) && (inline_fastrand(e)>>31)) {
				continue;
			}
			dest[i] = (inline_fastrand(e)>>31) ? 0xffffff : 0;
		}
	}
}


static void * create_brokentv()
  {
  return bg_effectv_create(scrollRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_brokentv",
      .long_name = TRS("BrokenTV"),
      .description = TRS("BrokenTV simulates mistuned television or mistracking video image. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_brokentv,
      .destroy =   bg_effectv_destroy,
      //      .get_parameters =   get_parameters_invert,
      //      .set_parameter =    set_parameter_invert,
      .priority =         1,
    },
    
    .connect_input_port = bg_effectv_connect_input_port,
    
    .set_input_format = bg_effectv_set_input_format,
    .get_output_format = bg_effectv_get_output_format,

    .read_video = bg_effectv_read_video,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
