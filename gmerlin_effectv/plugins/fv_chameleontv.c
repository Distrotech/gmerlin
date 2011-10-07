/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * ChameleonTV - Vanishing into the wall!!
 * Copyright (C) 2003 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#define PLANES_DEPTH 6
#define PLANES (1<<PLANES_DEPTH) 

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);


typedef struct
  {
  int state;
  int mode;
  RGB32 *bgimage;
  int bgIsSet;
  unsigned int *sum;
  unsigned char *timebuffer;
  int plane;
  } chameleon_t;

static void setBackground(effect * e, RGB32 *src);
static void drawDisappearing(effect * e, RGB32 *src, RGB32 *dest);
static void drawAppearing(effect * e, RGB32 *src, RGB32 *dest);

static effect *chameleonRegister(void)
  {
  effect *entry;
  chameleon_t * priv;
  
  entry = calloc(1, sizeof(effect));
  
  if(entry == NULL)
    return NULL;

  priv = calloc(1, sizeof(*priv));
  entry->priv = priv;

  entry->start = start;
  entry->stop = stop;
  entry->draw = draw;
  
  return entry;
  }

static int start(effect * e)
  {
  chameleon_t * priv = e->priv;
  
  priv->sum = malloc(e->video_area * sizeof(unsigned int));
  priv->bgimage = malloc(e->video_area * PIXEL_SIZE);
  if(priv->sum == NULL || priv->bgimage == NULL)
    return -1;
  
  priv->timebuffer = malloc(e->video_area * PLANES);
  if(priv->timebuffer == NULL)
    return -1;
  
  memset(priv->timebuffer, 0, e->video_area * PLANES);
  memset(priv->sum, 0, e->video_area * sizeof(unsigned int));
  priv->bgIsSet = 0;
  priv->plane = 0;
  
  priv->state = 1;
  return 0;
  }

static int stop(effect * e)
  {
  chameleon_t * priv = e->priv;
  if(priv->state)
    {
    if(priv->timebuffer)
      {
      free(priv->timebuffer);
      priv->timebuffer = NULL;
      }
    if(priv->sum)
      {
      free(priv->sum);
      priv->sum = NULL;
      }
    if(priv->bgimage)
      {
      free(priv->bgimage);
      priv->bgimage = NULL;
      }
    priv->state = 0;
    }
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  chameleon_t * priv = e->priv;
  if(!priv->bgIsSet)
    {
    setBackground(e, src);
    }
  
  if(priv->mode == 0)
    {
    drawDisappearing(e, src, dest);
    }
  else
    {
    drawAppearing(e, src, dest);
    }
  
  return 0;
  }

static void drawDisappearing(effect * e, RGB32 *src, RGB32 *dest)
{
	int i;
	unsigned int Y;
	int r, g, b;
	int R, G, B;
	unsigned char *p;
	RGB32 *q;
	unsigned int *s;
  chameleon_t * priv = e->priv;

	p = priv->timebuffer + priv->plane * e->video_area;
	q = priv->bgimage;
	s = priv->sum;
	for(i=0; i<e->video_area; i++) {
		Y = *src++;

		r = (Y>>16) & 0xff;
		g = (Y>>8) & 0xff;
		b = Y & 0xff;

		R = (*q>>16) & 0xff;
		G = (*q>>8) & 0xff;
		B = *q & 0xff;

		Y = (r + g * 2 + b) >> 2;
		*s -= *p;
		*s += Y;
		*p = Y;
		Y = (abs(((int)Y<<PLANES_DEPTH) - (int)(*s)) * 8)>>PLANES_DEPTH;
		if(Y>255) Y = 255;

		R += ((r - R) * Y) >> 8;
		G += ((g - G) * Y) >> 8;
		B += ((b - B) * Y) >> 8;
		*dest++ = (R<<16)|(G<<8)|B;

		p++;
		q++;
		s++;
	}
	priv->plane++;
	priv->plane = priv->plane & (PLANES-1);
}

static void drawAppearing(effect * e, RGB32 *src, RGB32 *dest)
{
	int i;
	unsigned int Y;
	int r, g, b;
	int R, G, B;
	unsigned char *p;
	RGB32 *q;
	unsigned int *s;
  chameleon_t * priv = e->priv;

	p = priv->timebuffer + priv->plane * e->video_area;
	q = priv->bgimage;
	s = priv->sum;
	for(i=0; i<e->video_area; i++) {
		Y = *src++;

		r = (Y>>16) & 0xff;
		g = (Y>>8) & 0xff;
		b = Y & 0xff;

		R = (*q>>16) & 0xff;
		G = (*q>>8) & 0xff;
		B = *q & 0xff;

		Y = (r + g * 2 + b) >> 2;
		*s -= *p;
		*s += Y;
		*p = Y;
		Y = (abs(((int)Y<<PLANES_DEPTH) - (int)(*s)) * 8)>>PLANES_DEPTH;
		if(Y>255) Y = 255;

		r += ((R - r) * Y) >> 8;
		g += ((G - g) * Y) >> 8;
		b += ((B - b) * Y) >> 8;
		*dest++ = (r<<16)|(g<<8)|b;

		p++;
		q++;
		s++;
	}
	priv->plane++;
        priv->plane = priv->plane & (PLANES-1);
}

static void setBackground(effect * e, RGB32 *src)
  {
  chameleon_t * priv = e->priv;
  memcpy(priv->bgimage, src, e->video_area * PIXEL_SIZE);
  priv->bgIsSet = 1;
  }


static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "disapp" },
      .multi_names = (char const *[]){ "app", "disapp",
                                       NULL },
      .multi_labels = (char const *[]){ TRS("Appear"),
                                        TRS("Disappear"),
                                        NULL },
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
  chameleon_t * priv = vp->e->priv;
  
  if(!name)
    return;
  if(!strcmp(name, "mode"))
    {
    if(!strcmp(val->val_str, "app"))
      priv->mode = 1;
    else
      priv->mode = 0;
    }
  }

static void * create_chameleontv()
  {
  return bg_effectv_create(chameleonRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_chameleontv",
      .long_name = TRS("ChameleonTV"),
      .description = TRS("When you are still in the sight of the camera for a second, you will be vanishing into the background, and disappear. When you move again, you will appear normally. By contrast, when you switch from \"disappearing mode\" to \"appearing mode\", moving objects are not shown, and a still object appears after seconds. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_chameleontv,
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
