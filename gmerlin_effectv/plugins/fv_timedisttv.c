/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * TimeDistortionTV - scratch the surface and playback old images.
 * Copyright (C) 2005 Ryo-ta
 *
 * Ported and arranged by Kentaro Fukuchi
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#define PLANES 32
#define MAGIC_THRESHOLD 40

static int start(effect *);
static int stop(effect *);
static int draw(effect *, RGB32 *src, RGB32 *dest);


typedef struct
  {
  int state;
  RGB32 *buffer;
  RGB32 *planetable[PLANES];
  int plane;
  int *warptime[2];
  int warptimeFrame;
  } timedist_t;

static effect *timeDistortionRegister(void)
{
	effect *entry;
        timedist_t * priv;

	entry = (effect *)calloc(1, sizeof(effect));
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
        timedist_t * priv = (timedist_t *)e->priv;
        image_init(e);
	priv->warptime[0] = malloc(e->video_area * sizeof(int));
	priv->warptime[1] = malloc(e->video_area * sizeof(int));
	if(priv->warptime[0] == NULL || priv->warptime[1] == NULL) {
		return -1;
	}
        
	priv->buffer = (RGB32 *)malloc(e->video_area * PIXEL_SIZE * PLANES);
	if(priv->buffer == NULL)
		return -1;
	memset(priv->buffer, 0, e->video_area * PIXEL_SIZE * PLANES);
	for(i=0;i<PLANES;i++)
          priv->planetable[i] = &priv->buffer[e->video_area*i];

	memset(priv->warptime[0], 0, e->video_area * sizeof(int));
	memset(priv->warptime[1], 0, e->video_area * sizeof(int));

	priv->plane = 0;
	image_set_threshold_y(e, MAGIC_THRESHOLD);

	priv->state = 1;
	return 0;
}

static int stop(effect * e)
{
        timedist_t * priv = (timedist_t *)e->priv;
	if(priv->state) {
		if(priv->buffer) {
			free(priv->buffer);
			priv->buffer = NULL;
		}
                
                free(priv->warptime[0]);
                free(priv->warptime[1]);
                
                priv->state = 0;
	}
	return 0;
}

static int draw(effect * e, RGB32 *src, RGB32 *dest)
{
	int i, x, y;
	unsigned char *diff;
	int *p, *q;
        timedist_t * priv = (timedist_t *)e->priv;

	memcpy(priv->planetable[priv->plane], src, PIXEL_SIZE * e->video_area);
	diff = image_bgsubtract_update_y(e, src);

	p = priv->warptime[priv->warptimeFrame    ] + e->video_width + 1;
	q = priv->warptime[priv->warptimeFrame ^ 1] + e->video_width + 1;
	for(y=e->video_height - 2; y>0; y--) {
		for(x=e->video_width - 2; x>0; x--) {
			i = *(p - e->video_width) + *(p - 1) + *(p + 1) + *(p + e->video_width);
			if(i > 3) i -= 3;
			p++;
			*q++ = i >> 2;
		}
		p += 2;
		q += 2;
	}

        //	q = priv->warptime[priv->warptimeFrame ^ 1] + e->video_width + 1;
        q = priv->warptime[priv->warptimeFrame ^ 1];
        for(i=0; i<e->video_area; i++) {
		if(*diff++) {
			*q = PLANES - 1;
		}
		*dest++ = priv->planetable[(priv->plane - *q + PLANES) & (PLANES - 1)][i];
		q++;
	}

	priv->plane++;
	priv->plane = priv->plane & (PLANES-1);

	priv->warptimeFrame ^= 1;

	return 0;
}


static void * create_timedisttv()
  {
  return bg_effectv_create(timeDistortionRegister, 0);
  }


const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_timedisttv",
      .long_name = TRS("TimedistTV"),
      .description = TRS("Distorts moving objects in the sight. When it detects a moving part, it rollbacks to an old frame around that. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_timedisttv,
      .destroy =   bg_effectv_destroy,
      //      .get_parameters =   get_parameters,
      //      .set_parameter =    set_parameter,
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
