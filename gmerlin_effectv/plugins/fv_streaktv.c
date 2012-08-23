/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * StreakTV - afterimage effector.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include <utils.h>

#define PLANES 32
#define STRIDE 4
#define STRIDE_MASK 0xf8f8f8f8
#define STRIDE_SHIFT 3

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);


typedef struct
  {
  int state;
  RGB32 *buffer;
  RGB32 *planetable[PLANES];
  int plane;
  } streaktv_t;

static effect *streakRegister(void)
{
	effect *entry;
        streaktv_t * priv;
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
        streaktv_t * priv = e->priv;
        
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
        streaktv_t * priv = e->priv;
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
        streaktv_t * priv = e->priv;
	int i, cf;

	for(i=0; i<e->video_area; i++) {
		priv->planetable[priv->plane][i] = (src[i] & STRIDE_MASK)>>STRIDE_SHIFT;
	}

	cf = priv->plane & (STRIDE-1);
	for(i=0; i<e->video_area; i++) {
		dest[i] = priv->planetable[cf][i]
		        + priv->planetable[cf+STRIDE][i]
		        + priv->planetable[cf+STRIDE*2][i]
		        + priv->planetable[cf+STRIDE*3][i]
		        + priv->planetable[cf+STRIDE*4][i]
		        + priv->planetable[cf+STRIDE*5][i]
		        + priv->planetable[cf+STRIDE*6][i]
		        + priv->planetable[cf+STRIDE*7][i];
	}
	priv->plane++;
	priv->plane = priv->plane & (PLANES-1);

	return 0;
}

static void * create_streaktv()
  {
  return bg_effectv_create(streakRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_streaktv",
      .long_name = TRS("StreakTV"),
      .description = TRS("StreakTV makes after images of moving objects. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_streaktv,
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
