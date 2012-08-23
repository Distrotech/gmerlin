/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentarou
 *
 * RandomDotStereoTV - makes random dot stereogram.
 * Copyright (C) 2002 FUKUCHI Kentarou
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gmerlin_effectv.h>
#include "utils.h"

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dest);

static const int stride = 40;

typedef struct
  {
  int stat;
  int method;
  } rds_t;

static effect *rdsRegister(void)
{
	effect *entry;
	rds_t * priv;
        
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

static int start(effect*e)
  {
  rds_t * priv = e->priv;
  priv->stat = 1;
  return 0;
  }

static int stop(effect*e)
  {
  rds_t * priv = e->priv;
  priv->stat = 0;
  return 0;
  }

static int draw(effect*e, RGB32 *src, RGB32 *dest)
{
	int x, y, i;
	RGB32 *target;
	RGB32 v;
	RGB32 R, G, B;
        rds_t * priv = e->priv;

	memset(dest, 0, e->video_area * PIXEL_SIZE);
	target = dest;

	if(priv->method) {
		for(y=0; y<e->video_height; y++) {
			for(i=0; i<stride; i++) {
				if(inline_fastrand(e)&0xc0000000)
					continue;

				x = e->video_width / 2 + i;
				*(dest + x) = 0xffffff;
	
				while(x + stride/2 < e->video_width) {
					v = *(src + x + stride/2);
					R = (v&0xff0000)>>(16+6);
					G = (v&0xff00)>>(8+6);
					B = (v&0xff)>>7;
					x += stride + R + G + B;
					if(x >= e->video_width) break;
					*(dest + x) = 0xffffff;
				}

				x = e->video_width / 2 + i;
				while(x - stride/2 >= 0) {
					v = *(src + x - stride/2);
					R = (v&0xff0000)>>(16+6);
					G = (v&0xff00)>>(8+6);
					B = (v&0xff)>>7;
					x -= stride + R + G + B;
					if(x < 0) break;
					*(dest + x) = 0xffffff;
				}
			}
			src += e->video_width;
			dest += e->video_width;
		}
	} else {
		for(y=0; y<e->video_height; y++) {
			for(i=0; i<stride; i++) {
				if(inline_fastrand(e)&0xc0000000)
					continue;

				x = e->video_width / 2 + i;
				*(dest + x) = 0xffffff;
	
				while(x + stride/2 < e->video_width) {
					v = *(src + x + stride/2);
					R = (v&0xff0000)>>(16+6);
					G = (v&0xff00)>>(8+6);
					B = (v&0xff)>>7;
					x += stride - R - G - B;
					if(x >= e->video_width) break;
					*(dest + x) = 0xffffff;
				}

				x = e->video_width / 2 + i;
				while(x - stride/2 >= 0) {
					v = *(src + x - stride/2);
					R = (v&0xff0000)>>(16+6);
					G = (v&0xff00)>>(8+6);
					B = (v&0xff)>>7;
					x -= stride - R - G - B;
					if(x < 0) break;
					*(dest + x) = 0xffffff;
				}
			}
			src += e->video_width;
			dest += e->video_width;
		}
	}

	target += e->video_width + (e->video_width - stride) / 2;
	for(y=0; y<4; y++) {
		for(x=0; x<4; x++) {
			target[x] = 0xff0000;
			target[x+stride] = 0xff0000;
		}
		target += e->video_width;
	}

	return 0;
}


static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "method",
      .long_name = TRS("Method"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "wall" },
      .multi_names = (char const *[]){ "wall", "cross", NULL },
      .multi_labels = (char const *[]){ TRS("Wall eyed"), TRS("Cross Eyed"),
                                        NULL},
    },
    { /* */ },
  };

static const bg_parameter_info_t * get_parameters(void * data)
  {
  return parameters;
  }

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t *val)
  {
  bg_effectv_plugin_t * vp = data;
  rds_t * priv = vp->e->priv;
  
  if(!name)
    return;
  if(!strcmp(name, "mode"))
    {
    if(!strcmp(val->val_str, "wall"))
      priv->method = 0;
    else
      priv->method = 1;
    }
  }

static void * create_rdstv()
  {
  return bg_effectv_create(rdsRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_rdstv",
      .long_name = TRS("RandomDotStereoTV"),
      .description = TRS("RdsTV does rds mark effect on the video input. The rds is caused by a motion or random rain drops. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_rdstv,
      .destroy =   bg_effectv_destroy,
      .get_parameters =   get_parameters,
      .set_parameter =    set_parameter,
      .priority =         1,
    },
    
    .connect = bg_effectv_connect,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
