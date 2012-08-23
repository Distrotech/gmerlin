/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * BaltanTV - like StreakTV, but following for a long time
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#define PLANES 32
#define STRIDE 8

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);


typedef struct
  {
  int state;
  RGB32 *buffer;
  RGB32 *planetable[PLANES];
  int plane;
  } baltantv_t;

static effect *baltanRegister(void)
{
	effect *entry;
        baltantv_t * priv = calloc(1, sizeof(*priv));
        
	entry = calloc(1, sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
	
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
        entry->priv = priv;
	return entry;
}

static int start(effect * e)
{
	int i;

  baltantv_t * priv = e->priv;
  priv->buffer = malloc(e->video_area * PIXEL_SIZE * PLANES);
	if(priv->buffer == NULL)
		return -1;

	memset(priv->buffer, 0, e->video_area * PIXEL_SIZE * PLANES);
	for(i=0;i<PLANES;i++)
		priv->planetable[i] = &priv->buffer[e->video_area*i];

	priv->plane = 0;

	priv->state = 1;

	return 0;
}

static int stop(effect * e)
{
  baltantv_t * priv = e->priv;
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
  baltantv_t * priv = e->priv;
	int i, cf;

	for(i=0; i<e->video_area; i++) {
		priv->planetable[priv->plane][i] = (src[i] & 0xfcfcfc)>>2;
	}

	cf = priv->plane & (STRIDE-1);
	for(i=0; i<e->video_area; i++) {
		dest[i] = priv->planetable[cf][i]
		        + priv->planetable[cf+STRIDE][i]
		        + priv->planetable[cf+STRIDE*2][i]
		        + priv->planetable[cf+STRIDE*3][i];
		priv->planetable[priv->plane][i] = (dest[i]&0xfcfcfc)>>2;
	}
	priv->plane++;
	priv->plane = priv->plane & (PLANES-1);

	return 0;
}

static void * create_baltantv()
  {
  return bg_effectv_create(baltanRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_baltantv",
      .long_name = TRS("BaltanTV"),
      .description = TRS("BaltanTV is similar to the StreakTV,but BaltanTV makes after images longer than that. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_baltantv,
      .destroy =   bg_effectv_destroy,
      //      .get_parameters =   get_parameters_invert,
      //      .set_parameter =    set_parameter_invert,
      .priority =         1,
    },

    .connect = bg_effectv_connect,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
