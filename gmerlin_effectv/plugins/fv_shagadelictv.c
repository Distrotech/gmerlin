/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * ShagadelicTV - makes you shagadelic! Yeah baby yeah!
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * Inspired by Adrian Likin's script for the GIMP.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gmerlin_effectv.h>
#include <utils.h>

static int start(effect*e);
static int stop(effect*e);
static int draw(effect*e, RGB32 *src, RGB32 *dest);


typedef struct
  {
  int stat;
  char *ripple;
  char *spiral;
  unsigned char phase;
  int rx, ry;
  int bx, by;
  int rvx, rvy;
  int bvx, bvy;
  int mask_r, mask_g, mask_b;
  } shagadelictv_t;

static effect *shagadelicRegister(void)
{
	effect *entry;
	shagadelictv_t * priv;
        
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
	int i, x, y;
#ifdef PS2
	float xx, yy;
#else
	double xx, yy;
#endif
	shagadelictv_t * priv = e->priv;

        priv->ripple = malloc(e->video_area*4);
	priv->spiral = malloc(e->video_area);
	if(priv->ripple == NULL || priv->spiral == NULL) {
		return -1;
	}
        
	i = 0;
	for(y=0; y<e->video_height*2; y++) {
		yy = (double)y / e->video_width - 1.0;
		yy *= yy;
		for(x=0; x<e->video_width*2; x++) {
			xx = (double)x / e->video_width - 1.0;
#ifdef PS2
			priv->ripple[i++] = ((unsigned int)(sqrtf(xx*xx+yy)*3000))&255;
#else
			priv->ripple[i++] = ((unsigned int)(sqrt(xx*xx+yy)*3000))&255;
#endif
		}
	}
	i = 0;
	for(y=0; y<e->video_height; y++) {
		yy = (double)(y - e->video_height / 2) / e->video_width;
		for(x=0; x<e->video_width; x++) {
			xx = (double)x / e->video_width - 0.5;
#ifdef PS2
			priv->spiral[i++] = ((unsigned int)
				((atan2f(xx, yy)/((float)M_PI)*256*9) + (sqrtf(xx*xx+yy*yy)*1800)))&255;
#else
			priv->spiral[i++] = ((unsigned int)
				((atan2(xx, yy)/M_PI*256*9) + (sqrt(xx*xx+yy*yy)*1800)))&255;
#endif
/* Here is another Swinger!
 * ((atan2(xx, yy)/M_PI*256) + (sqrt(xx*xx+yy*yy)*3000))&255;
 */
		}
	}
	priv->rx = fastrand(e)%e->video_width;
	priv->ry = fastrand(e)%e->video_height;
	priv->bx = fastrand(e)%e->video_width;
	priv->by = fastrand(e)%e->video_height;
	priv->rvx = -2;
	priv->rvy = -2;
	priv->bvx = 2;
	priv->bvy = 2;
	priv->phase = 0;

	priv->stat = 1;
	return 0;
}

static int stop(effect * e)
  {
  shagadelictv_t * priv = e->priv;
  priv->stat = 0;
  if(priv->ripple)
    {
    free(priv->ripple);
    priv->ripple = NULL;
    }
  if(priv->spiral)
    {
    free(priv->spiral);
    priv->spiral = NULL;
    }
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {

  int x, y;
  RGB32 v;
  unsigned char r, g, b;
  char *pr, *pg, *pb;
  int mask;
  shagadelictv_t * priv = e->priv;

  mask =
    (priv->mask_r * 0xff0000) | 
    (priv->mask_g * 0x00ff00) |
    (priv->mask_b * 0x0000ff);
  
  pr = &priv->ripple[priv->ry*e->video_width*2 + priv->rx];
  pg = priv->spiral;
  pb = &priv->ripple[priv->by*e->video_width*2 + priv->bx];

  for(y=0; y<e->video_height; y++) {
  for(x=0; x<e->video_width; x++) {
  v = *src++ | 0x1010100;
  v = (v - 0x707060) & 0x1010100;
  v -= v>>8;
  /* Try another Babe! 
   * v = *src++;
   * *dest++ = v & ((r<<16)|(g<<8)|b);
   */
  r = (char)(*pr+priv->phase*2)>>7;
  g = (char)(*pg+priv->phase*3)>>7;
  b = (char)(*pb-priv->phase)>>7;
  *dest++ = v & ((r<<16)|(g<<8)|b) & mask;
  pr++;
  pg++;
  pb++;
  }
  pr += e->video_width;
  pb += e->video_width;
  }

  priv->phase -= 8;
  if((priv->rx+priv->rvx)<0 || (priv->rx+priv->rvx)>=e->video_width) priv->rvx =-priv->rvx;
  if((priv->ry+priv->rvy)<0 || (priv->ry+priv->rvy)>=e->video_height) priv->rvy =-priv->rvy;
  if((priv->bx+priv->bvx)<0 || (priv->bx+priv->bvx)>=e->video_width) priv->bvx =-priv->bvx;
  if((priv->by+priv->bvy)<0 || (priv->by+priv->bvy)>=e->video_height) priv->bvy =-priv->bvy;
  priv->rx += priv->rvx;
  priv->ry += priv->rvy;
  priv->bx += priv->bvx;
  priv->by += priv->bvy;

  return 0;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "mask_r",
      .long_name = TRS("Red mask"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_i = 1 },
    },
    {
      .name =      "mask_g",
      .long_name = TRS("Green mask"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_i = 1 },
    },
    {
      .name =      "mask_b",
      .long_name = TRS("Blue mask"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_i = 1 },
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
  shagadelictv_t * priv = vp->e->priv;
  int changed = 0;
  if(!name)
    return;

  EFFECTV_SET_PARAM_INT(mask_r);
  EFFECTV_SET_PARAM_INT(mask_g);
  EFFECTV_SET_PARAM_INT(mask_b);
  }

static void * create_shagadelictv()
  {
  return bg_effectv_create(shagadelicRegister, 0);
  }



const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_shagadelictv",
      .long_name = TRS("ShagadelicTV"),
      .description = TRS("Oh behave, ShagedelicTV makes images shagadelic! Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_shagadelictv,
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


