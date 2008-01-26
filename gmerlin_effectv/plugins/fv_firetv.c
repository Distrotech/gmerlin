/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * FireTV - clips incoming objects and burn them.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * Fire routine is taken from Frank Jan Sorensen's demo program.
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);
// static int event(SDL_Event *);

#define MaxColor 120
#define Decay 15
#define MAGIC_THRESHOLD 50

static char *effectname = "FireTV";

typedef struct
  {
  int state;
  unsigned char *buffer;
  RGB32 palette[256];
  int mode;
  int bgIsSet;
  } fire_t;

static int setBackground(effect * e, RGB32 *src)
  {
  fire_t * priv = (fire_t *)(e->priv);
  
  image_bgset_y(e, src);
  priv->bgIsSet = 1;
  
  return 0;
  }

static void makePalette(fire_t * priv)
  {
  int i, r, g, b;
  
  for(i=0; i<MaxColor; i++)
    {
    HSItoRGB(4.6-1.5*i/MaxColor, (double)i/MaxColor, (double)i/MaxColor, &r, &g, &b);
    priv->palette[i] = (r<<16)|(g<<8)|b;
    }
  for(i=MaxColor; i<256; i++)
    {
    if(r<255)r++;if(r<255)r++;if(r<255)r++;
    if(g<255)g++;
    if(g<255)g++;
    if(b<255)b++;
    if(b<255)b++;
    priv->palette[i] = (r<<16)|(g<<8)|b;
    }
  }

static effect *fireRegister(void)
  {
  effect *entry;
  fire_t * priv;

  entry = (effect *)calloc(1, sizeof(effect));
  if(entry == NULL)
    {
    return NULL;
    }
  priv = calloc(1, sizeof(*priv));
  entry->priv = priv;
  
  entry->name = effectname;
  entry->start = start;
  entry->stop = stop;
  entry->draw = draw;
  
  makePalette(priv);
  
  return entry;
  }

static int start(effect * e)
  {
  fire_t * priv = (fire_t *)(e->priv);
  priv->buffer = (unsigned char *)malloc(e->video_area);
  if(priv->buffer == NULL)
    {
    return -1;
    }
  image_init(e);
  image_set_threshold_y(e, MAGIC_THRESHOLD);
  memset(priv->buffer, 0, e->video_area);
  priv->bgIsSet = 0;
  
  priv->state = 1;
  return 0;
  }

static int stop(effect * e)
  {
  fire_t * priv = (fire_t *)(e->priv);
  priv->state = 0;
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  int i, x, y;
  unsigned char v;
  unsigned char *diff;
  fire_t * priv = (fire_t *)(e->priv);
  
  if(!priv->bgIsSet)
    {
    setBackground(e, src);
    }
  
  switch(priv->mode)
    {
    case 0:
    default:
      diff = image_bgsubtract_y(e, src);
      for(i=0; i<e->video_area-e->video_width; i++)
        {
        priv->buffer[i] |= diff[i];
        }
      break;
    case 1:
      for(i=0; i<e->video_area-e->video_width; i++)
        {
        v = (src[i]>>16) & 0xff;
        if(v > 150)
          {
          priv->buffer[i] |= v;
          }
        }
      break;
    case 2:
      for(i=0; i<e->video_area-e->video_width; i++)
        {
        v = src[i] & 0xff;
        if(v < 60)
          {
          priv->buffer[i] |= 0xff - v;
          }
        }
      break;
    }
  
  for(x=1; x<e->video_width-1; x++)
    {
    i = e->video_width + x;
    for(y=1; y<e->video_height; y++)
      {
      v = priv->buffer[i];
      if(v<Decay)
        priv->buffer[i-e->video_width] = 0;
      else
        priv->buffer[i-e->video_width+fastrand(e)%3-1] = v - (fastrand(e)&Decay);
      i += e->video_width;
      }
    }
  for(y=0; y<e->video_height; y++)
    {
    for(x=1; x<e->video_width-1; x++)
      {
      dest[y*e->video_width+x] = priv->palette[priv->buffer[y*e->video_width+x]];
      }
    }
  
  return 0;
  }

#if 0
static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			if(mode == 0) {
				bgIsSet = 0;
			}
			break;
		case SDLK_1:
		case SDLK_KP1:
			mode = 0;
			break;
		case SDLK_2:
		case SDLK_KP2:
			mode = 1;
			break;
		case SDLK_3:
		case SDLK_KP3:
			mode = 2;
			break;
		default:
			break;
		}
	}
	return 0;
}
#endif

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "all" },
      .multi_names = (char const *[]){ "fg", "light", "dark", (char*)0 },
      .multi_labels = (char const *[]){ TRS("Foreground"), TRS("Light parts"),TRS("Dark parts"), (char*)0 },
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
  bg_effectv_plugin_t * vp = (bg_effectv_plugin_t *)data;
  fire_t * priv = (fire_t*)vp->e->priv;
  
  if(!name)
    return;
  if(!strcmp(name, "mode"))
    {
    if(!strcmp(val->val_str, "fg"))
      priv->mode = 0;
    else if(!strcmp(val->val_str, "light"))
      priv->mode = 1;
    else if(!strcmp(val->val_str, "dark"))
      priv->mode = 2;
    }
  }

static void * create_firetv()
  {
  return bg_effectv_create(fireRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_firetv",
      .long_name = TRS("FireTV"),
      .description = TRS("FireTV clips moving objects and burns it. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_firetv,
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
