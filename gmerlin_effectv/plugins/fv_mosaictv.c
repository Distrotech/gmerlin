/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * MosaicTV - censors incoming objects
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#define MAGIC_THRESHOLD 50
#define CENSOR_LEVEL 20

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dest);


typedef struct
  {
  int stat;
  int bgIsSet;
  } mosaic_t;


static int setBackground(effect* e, RGB32 *src)
{
mosaic_t * priv = e->priv;
	image_bgset_y(e, src);
	priv->bgIsSet = 1;

	return 0;
}

static effect *mosaicRegister(void)
{
	effect *entry;
        mosaic_t * priv;
	
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

static int start(effect* e)
  {
  mosaic_t * priv = e->priv;

  image_init(e);
  image_set_threshold_y(e, MAGIC_THRESHOLD);
  priv->bgIsSet = 0;
  
  priv->stat = 1;
  return 0;
  }

static int stop(effect* e)
{
mosaic_t * priv = e->priv;
	priv->stat = 0;
	return 0;
}

static int draw(effect* e, RGB32 *src, RGB32 *dest)
{
	int x, y, xx, yy, v;
	int count;
	RGB32 *p, *q;
	unsigned char *diff, *r;
mosaic_t * priv = e->priv;

	if(!priv->bgIsSet) {
		setBackground(e, src);
	}

	diff = image_bgsubtract_y(e, src);

	for(y=0; y<e->video_height-7; y+=8) {
		for(x=0; x<e->video_width-7; x+=8) {
			count = 0;
			p = &src[y*e->video_width+x];
			q = &dest[y*e->video_width+x];
			r = &diff[y*e->video_width+x];
			for(yy=0; yy<8; yy++) {
				for(xx=0; xx<8; xx++) {
					count += r[yy*e->video_width+xx];
				}
			}
			if(count > CENSOR_LEVEL*255) {
				v = p[3*e->video_width+3];
				for(yy=0; yy<8; yy++) {
					for(xx=0; xx<8; xx++){
						q[yy*e->video_width+xx] = v;
					}
				}
			} else {
				for(yy=0; yy<8; yy++) {
					for(xx=0; xx<8; xx++){
						q[yy*e->video_width+xx] = p[yy*e->video_width+xx];
					}
				}
			}
		}
	}

	return 0;
}


static void * create_mosaictv()
  {
  return bg_effectv_create(mosaicRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_mosaictv",
      .long_name = TRS("MosaicTV"),
      .description = TRS("MosaicTV censors the incoming objects and gives it mosaic effect. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_mosaictv,
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
