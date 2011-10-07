/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * QuarkTV - motion disolver.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#define PLANES 16

static int start(effect*);
static int stop(effect*);
static int draw(effect*,RGB32 *src, RGB32 *dest);


typedef struct
  {
  int state;
  RGB32 *buffer;
  RGB32 *planetable[PLANES];
  int plane;
  } quarktv_t;

static effect *quarkRegister(void)
{
	effect *entry;
        quarktv_t * priv;
        
	entry = calloc(1, sizeof(effect));
	if(entry == NULL) {
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
	int i;
        quarktv_t * priv = e->priv;

	priv->buffer = malloc(e->video_area *
                                       PIXEL_SIZE * PLANES);
	if(priv->buffer == NULL)
		return -1;
	memset(priv->buffer, 0, e->video_area * PIXEL_SIZE * PLANES);
	for(i=0;i<PLANES;i++)
		priv->planetable[i] = &priv->buffer[e->video_area*i];
	priv->plane = PLANES - 1;
        
	priv->state = 1;

	return 0;
}

static int stop(effect * e)
{
        quarktv_t * priv = e->priv;
	if(priv->state) {
		if(priv->buffer) {
			free(priv->buffer);
			priv->buffer = NULL;
		}
		priv->state = 0;
	}

	return 0;
}

static int draw(effect * e, RGB32 *src, RGB32 *dest)
{
        quarktv_t * priv = e->priv;
	int i;
	int cf;

	memcpy(priv->planetable[priv->plane], src, e->video_area * PIXEL_SIZE);

	for(i=0; i<e->video_area; i++) {
		cf = (inline_fastrand(e)>>24)&(PLANES-1);
		dest[i] = (priv->planetable[cf])[i];
		/* The reason why I use high order 8 bits is written in utils.c
		(or, 'man rand') */
	}

	priv->plane--;
	if(priv->plane<0)
		priv->plane = PLANES - 1;

	return 0;
}

static void * create_quarktv()
  {
  return bg_effectv_create(quarkRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_quarktv",
      .long_name = TRS("QuarkTV"),
      .description = TRS("QuarkTV dissolves moving objects. It picks up pixels from the last eight frames randomly. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_quarktv,
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
