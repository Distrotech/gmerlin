/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * OpTV - Optical art meets real-time video effect.
 * Copyright (C) 2004-2005 FUKUCHI Kentaro
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

#define OPMAP_MAX 4

typedef struct
  {
  char *opmap[OPMAP_MAX];
  int stat;
  unsigned char phase;
  int mode;
  int speed;
  RGB32 palette[256];
  } op_t;

#define OP_SPIRAL1  0
#define OP_SPIRAL2  1
#define OP_PARABOLA 2
#define OP_HSTRIPE  3


static void initPalette(op_t* priv)
{
	int i;
	unsigned char v;

	for(i=0; i<112; i++) {
		priv->palette[i] = 0;
		priv->palette[i+128] = 0xffffff;
	}
	for(i=0; i<16; i++) {
		v = 16 * (i + 1) - 1;
		priv->palette[i+112] = (v<<16) | (v<<8) | v;
		v = 255 - v;
		priv->palette[i+240] = (v<<16) | (v<<8) | v;
	}
}

static void setOpmap(effect * e)
{
	int i, j, x, y;
#ifndef PS2
	double xx, yy, r, at, rr;
#else
	float xx, yy, r, at, rr;
#endif
	int sci;

        op_t * priv = e->priv;
	sci = 640 / e->video_width;
	i = 0;
	for(y=0; y<e->video_height; y++) {
		yy = (double)(y - e->video_height/2) / e->video_width;
		for(x=0; x<e->video_width; x++) {
			xx = (double)x / e->video_width - 0.5;
#ifndef PS2
			r = sqrt(xx * xx + yy * yy);
			at = atan2(xx, yy);
#else
			r = sqrtf(xx * xx + yy * yy);
			at = atan2f(xx, yy);
#endif

			priv->opmap[OP_SPIRAL1][i] = ((unsigned int)
				((at / M_PI * 256) + (r * 4000))) & 255;

			j = r * 300 / 32;
			rr = r * 300 - j * 32;
			j *= 64;
			j += (rr > 28) ? (rr - 28) * 16 : 0;
			priv->opmap[OP_SPIRAL2][i] = ((unsigned int)
				((at / M_PI * 4096) + (r * 1600) - j )) & 255;

			priv->opmap[OP_PARABOLA][i] = ((unsigned int)(yy/(xx*xx*0.3+0.1)*400))&255;
			priv->opmap[OP_HSTRIPE][i] = x*8*sci;
			i++;
		}
	}
}

static effect *opRegister(void)
{
	effect *entry;
        op_t * priv;
        
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
        op_t * priv = e->priv;
	int i;
        
        for(i=0; i<OPMAP_MAX; i++)
          {
          priv->opmap[i] = malloc(e->video_area);
          if(priv->opmap[i] == NULL)
            {
            return -1;
            }
          }
        
	initPalette(priv);
	setOpmap(e);
        image_init(e);
        priv->phase = 0;
	image_set_threshold_y(e, 50);

	priv->stat = 1;
	return 0;
}

static int stop(effect * e)
  {
  int i;
  op_t * priv = e->priv;
  priv->stat = 0;
  for(i=0; i<OPMAP_MAX; i++)
    free(priv->opmap[i]);
  return 1;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
{
	int x, y;
	char *p;
	unsigned char *diff;
        op_t * priv = e->priv;

	switch(priv->mode) {
		default:
		case 0:
			p = priv->opmap[OP_SPIRAL1];
			break;
		case 1:
			p = priv->opmap[OP_SPIRAL2];
			break;
		case 2:
			p = priv->opmap[OP_PARABOLA];
			break;
		case 3:
			p = priv->opmap[OP_HSTRIPE];
			break;
	}

	priv->phase -= priv->speed;

	diff = image_y_over(e, src);
	for(y=0; y<e->video_height; y++) {
		for(x=0; x<e->video_width; x++) {
			*dest++ = priv->palette[(((char)(*p+priv->phase))^*diff++)&255];
			p++;
		}
	}

	return 0;
}


static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "maelstrom" },
      .multi_names = (char const *[]){ "maelstrom", "radiation", "perspective",
                                       "vertical", NULL },
      .multi_labels = (char const *[]){ TRS("Maelstrom"), TRS("Radiation"),
                                        TRS("Perspective horizontal stripes"), 
                                        TRS("Vertical stripes"), NULL },
    },
    {
      .name =      "speed",
      .long_name = TRS("Speed"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = -64  },
      .val_max =     { .val_i = 64 },
      .val_default = { .val_i = 16  },
    },
    { },
  };

static const bg_parameter_info_t * get_parameters(void * data)
  {
  return parameters;
  }

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t *val)
  {
  int changed;
  bg_effectv_plugin_t * vp = data;
  op_t * priv = vp->e->priv;
  
  if(!name)
    return;
  EFFECTV_SET_PARAM_INT(speed);
  if(!strcmp(name, "mode"))
    {
    // "maelstrom", "radiation", "perspective", "vertical"
    
    if(!strcmp(val->val_str, "maelstrom"))
      priv->mode = 0;
    else if(!strcmp(val->val_str, "radiation"))
      priv->mode = 1;
    else if(!strcmp(val->val_str, "perspective"))
      priv->mode = 2;
    else if(!strcmp(val->val_str, "vertical"))
      priv->mode = 3;
    else
      priv->mode = 0;
    }
  }

static void * create_optv()
  {
  return bg_effectv_create(opRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_optv",
      .long_name = TRS("OpTV"),
      .description = TRS("Traditional black-white optical animation is now resurrected as a real-time video effect. Input images are binarized and combined with various optical pattern. You can change its animation speed and direction with a keyboard. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_optv,
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
