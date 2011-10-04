/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * NervousHalf - Or your bitter half.
 * Copyright (C) 2002 TANNENBAUM Edo
 * Copyright (C) 2004 Kentaro Fukuchi
 *
 * 2004/11/27 
 *  The most of this code has been taken from Edo's NervousTV.
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#define PLANES 32

static int start(effect *);
static int stop(effect *);
static int draw(effect * e, RGB32 *src, RGB32 *dest);

typedef struct
  {
  int state;
  RGB32 *buffer;
  RGB32 *planetable[PLANES];
  int mode;
  int plane;
  int stock;

  int scratchTimer;
  int scratchStride;
  int scratchCurrent;
  int delay;
  int dir;
  int mirror;
  } half_t;

static int nextDelay(effect * e);
static int nextNervous(effect * e);
static int nextScratch(effect * e);

static void left(effect * e, RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror);
static void right(effect * e, RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror);
static void upper(effect * e, RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror);
static void bottom(effect * e, RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror);

static effect *nervousHalfRegister(void)
{
	effect *entry;
        half_t * priv;
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
  half_t * priv = (half_t*)(e->priv);
  priv->buffer = (RGB32 *)malloc(e->video_area*PIXEL_SIZE*PLANES);
  if(priv->buffer == NULL)
    return -1;
  memset(priv->buffer, 0, e->video_area*PIXEL_SIZE*PLANES);
  for(i=0;i<PLANES;i++)
    priv->planetable[i] = &priv->buffer[e->video_area*i];
  
  priv->plane = 0;
  priv->stock = 0;
  priv->scratchTimer = 0;
  priv->scratchCurrent = 0;
  
  priv->state = 1;
  return 0;
  }

static int stop(effect * e)
{
  half_t * priv = (half_t*)(e->priv);
	if(priv->state) {
		if(priv->buffer)
			free(priv->buffer);
		priv->state = 0;
	}

	return 0;
}

static int nextDelay(effect * e)
{
  half_t * priv = (half_t*)(e->priv);
	return (priv->plane - priv->delay + PLANES) % PLANES;
}

static int nextScratch(effect * e)
{
  half_t * priv = (half_t*)(e->priv);
	if(priv->scratchTimer) {
		priv->scratchCurrent = priv->scratchCurrent + priv->scratchStride;
		while(priv->scratchCurrent < 0) priv->scratchCurrent += priv->stock;
		while(priv->scratchCurrent >= priv->stock) priv->scratchCurrent -= priv->stock;
		priv->scratchTimer--;
	} else {
		priv->scratchCurrent = inline_fastrand(e) % priv->stock;
		priv->scratchStride = inline_fastrand(e) % 5 - 2;
		if(priv->scratchStride >= 0) priv->scratchStride++;
		priv->scratchTimer = inline_fastrand(e) % 6 + 2;
	}

	return priv->scratchCurrent;
}

static int nextNervous(effect * e)
{
  half_t * priv = (half_t*)(e->priv);
	if(priv->stock > 0)
		return inline_fastrand(e) % priv->stock;

	return 0;
}

static int draw(effect * e, RGB32 *src, RGB32 *dest)
{
	int readplane;
	RGB32 *buf;
  half_t * priv = (half_t*)(e->priv);

	memcpy(priv->planetable[priv->plane], src, e->video_area * PIXEL_SIZE);
	if(priv->stock < PLANES) {
		priv->stock++;
	}

	switch(priv->mode) {
		default:
		case 0:
			readplane = nextDelay(e);
			break;
		case 1:
			readplane = nextScratch(e);
			break;
		case 2:
			readplane = nextNervous(e);
			break;
	}
	buf = priv->planetable[readplane];

	priv->plane++;
	if (priv->plane == PLANES) priv->plane=0;

	switch(priv->dir) {
		default:
		case 0:
			left(e, src, buf, dest, priv->mirror);
			break;
		case 1:
			right(e, src, buf, dest, priv->mirror);
			break;
		case 2:
			upper(e, src, buf, dest, priv->mirror);
			break;
		case 3:
			bottom(e, src, buf, dest, priv->mirror);
			break;
	}

	return 0;
}


static void upper(effect * e, RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror)
{
	int len;
	int y;
	RGB32 *p;

	len = e->video_height / 2 * e->video_width;
	memcpy(dest, src, len * PIXEL_SIZE);

	switch(mirror) {
		case 1:
			p = buf + len - e->video_width;
			dest += len;
			len = PIXEL_SIZE * e->video_width;
			for(y = e->video_height / 2; y > 0; y--) {
				memcpy(dest, p, len);
				p -= e->video_width;
				dest += e->video_width;
			}
			break;
		case 2:
			memcpy(dest + len, buf, len * PIXEL_SIZE);
			break;
		case 0:
		default:
			memcpy(dest + len, buf + len, len * PIXEL_SIZE);
			break;
	}
}

static void bottom(effect * e, RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror)
{
	int len;
	int y;
	RGB32 *p;

	len = e->video_height / 2 * e->video_width;
	memcpy(dest + len, src + len, len * PIXEL_SIZE);

	switch(mirror) {
		case 1:
			p = buf + e->video_area - e->video_width;
			len = PIXEL_SIZE * e->video_width;
			for(y = e->video_height / 2; y > 0; y--) {
				memcpy(dest, p, len);
				p -= e->video_width;
				dest += e->video_width;
			}
			break;
		case 2:
			memcpy(dest, buf + len, len * PIXEL_SIZE);
			break;
		case 0:
		default:
			memcpy(dest, buf, len * PIXEL_SIZE);
			break;
	}
}

static void left(effect * e, RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror)
{
	int x, y, len, st;
	RGB32 *s1, *s2, *d, *d1;

	len = e->video_width / 2;
	st = len * PIXEL_SIZE;

	switch(mirror) {
		case 1:
			s1 = src;
			s2 = buf + len;
			d = dest;
			for(y=0; y<e->video_height; y++) {
				memcpy(d, s1, st);
				d1 = d + len;
				for(x=0; x<len; x++) {
					*d1++ = *s2--;
				}
				d += e->video_width;
				s1 += e->video_width;
				s2 += e->video_width + len;
			}
			break;
		case 2:
			s1 = src;
			s2 = buf;
			d = dest;
			for(y=0; y<e->video_height; y++) {
				memcpy(d, s1, st);
				memcpy(d + len, s2, st);
				d += e->video_width;
				s1 += e->video_width;
				s2 += e->video_width;
			}
			break;
		case 0:
		default:
			s1 = src;
			s2 = buf + len;
			d = dest;
			for(y=0; y<e->video_height; y++) {
				memcpy(d, s1, st);
				memcpy(d + len, s2, st);
				d += e->video_width;
				s1 += e->video_width;
				s2 += e->video_width;
			}
			break;
	}
}

static void right(effect * e, RGB32 *src, RGB32 *buf, RGB32 *dest, int mirror)
{
	int x, y, len, st;
	RGB32 *s1, *s2, *d, *d1;

	len = e->video_width / 2;
	st = len * PIXEL_SIZE;

	switch(mirror) {
		case 1:
			s1 = src + len;
			s2 = buf + e->video_width - 1;
			d = dest;
			for(y=0; y<e->video_height; y++) {
				memcpy(d + len, s1, st);
				d1 = d;
				for(x=0; x<len; x++) {
					*d1++ = *s2--;
				}
				d += e->video_width;
				s1 += e->video_width;
				s2 += e->video_width + len;
			}
			break;
		case 2:
			s1 = src + len;
			s2 = buf + len;
			d = dest;
			for(y=0; y<e->video_height; y++) {
				memcpy(d + len, s1, st);
				memcpy(d, s2, st);
				d += e->video_width;
				s1 += e->video_width;
				s2 += e->video_width;
			}
			break;
		case 0:
		default:
			s1 = src + len;
			s2 = buf;
			d = dest;
			for(y=0; y<e->video_height; y++) {
				memcpy(d + len, s1, st);
				memcpy(d, s2, st);
				d += e->video_width;
				s1 += e->video_width;
				s2 += e->video_width;
			}
			break;
	}
}

/* Config stuff */

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "delay" },
      .multi_names = (char const *[]){ "delay",
                                       "scratch",
                                       "nervous",
                                       NULL },
      .multi_labels = (char const *[]){ TRS("Delay"),
                                        TRS("Scratch"),
                                        TRS("Nervous"),
                                        NULL },
    },
    {
      .name =      "direction",
      .long_name = TRS("Direction"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "right" },
      .multi_names = (char const *[]){ "left",
                                       "right",
                                       "top",
                                       "bottom",
                                       NULL },
      .multi_labels = (char const *[]){ TRS("Left"),
                                        TRS("Right"),
                                        TRS("Top"),
                                        TRS("Bottom"), NULL },
    },
    {
      .name =      "mirror",
      .long_name = TRS("Mirror"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "mirror" },
      .multi_names = (char const *[]){ "normal",
                                       "mirror",
                                       "copy",
                                       NULL },
      .multi_labels = (char const *[]){ TRS("Normal"),
                                        TRS("Mirror"),
                                        TRS("Copy"),
                                        NULL },
    },
    {
      .name =      "delay",
      .long_name = TRS("Delay"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 0   },
      .val_max =     { .val_i = PLANES-1 },
      .val_default = { .val_i = 10  },
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters(void * data)
  {
  return parameters;
  }

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t *val)
  {
  int changed;
  bg_effectv_plugin_t * vp = (bg_effectv_plugin_t *)data;
  half_t * priv = (half_t*)vp->e->priv;
  
  if(!name)
    return;
  EFFECTV_SET_PARAM_INT(delay);
  if(!strcmp(name, "mode"))
    {
    if(!strcmp(val->val_str, "delay"))
      priv->mode = 0;
    else if(!strcmp(val->val_str, "scratch"))
      priv->mode = 1;
    else if(!strcmp(val->val_str, "nervous"))
      priv->mode = 2;
    else
      priv->mode = 0;
    }
  if(!strcmp(name, "direction"))
    {
    if(!strcmp(val->val_str, "left"))
      priv->dir = 1;
    else if(!strcmp(val->val_str, "right"))
      priv->dir = 0;
    else if(!strcmp(val->val_str, "top"))
      priv->dir = 3;
    else if(!strcmp(val->val_str, "bottom"))
      priv->dir = 2;
    else
      priv->dir = 0;
    }
  if(!strcmp(name, "mirror"))
    {
    if(!strcmp(val->val_str, "normal"))
      priv->mirror = 0;
    else if(!strcmp(val->val_str, "mirror"))
      priv->mirror = 1;
    else if(!strcmp(val->val_str, "copy"))
      priv->mirror = 2;
    else
      priv->mirror = 1;
    }
  }

static void * create_halftv()
  {
  return bg_effectv_create(nervousHalfRegister, BG_EFFECTV_COLOR_AGNOSTIC);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_halftv",
      .long_name = TRS("NervousHalfTV"),
      .description = TRS("SimuraTV and NervousTV mixed, make more magic! Delaying, scratching or our famous \"nervous\" effect can be added to a half of the screen. Additionally you can add mirroring effect to it. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_halftv,
      .destroy =   bg_effectv_destroy,
      .get_parameters =   get_parameters,
      .set_parameter =    set_parameter,
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
