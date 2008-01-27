/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * EdgeBlurTV - Get difference, and make blur.
 * Copyright (C) 2005-2006 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#define MAX_BLUR 31
// #define COLORS 4
#define MAGIC_THRESHOLD 16

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);
// static int event(SDL_Event *event);


typedef struct
  {
  int state;
  int *blur[2];
  int blurFrame;
  RGB32 palette[MAX_BLUR + 1];
  RGB32 color;
  } edgeblur_t;

static void makePalette(edgeblur_t * priv)
  {
  int i, v;
  int r, g, b;

  r = RED(priv->color);
  g = GREEN(priv->color);
  b = BLUE(priv->color);
  
  for(i=0; i<=MAX_BLUR; i++)
    {
    v = (255 * i) / MAX_BLUR;
    priv->palette[i] =
      RGB((v * r)>>8,(v * g)>>8,(v * b)>>8);
    }
  }

static effect *edgeBlurRegister(void)
  {
  effect *entry;
  edgeblur_t * priv;
  
  entry = (effect *)calloc(1, sizeof(effect));
  if(entry == NULL)
    {
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
  edgeblur_t * priv = (edgeblur_t *)e->priv;
  
  priv->blur[0] = (int *)malloc(e->video_area * sizeof(int));
  priv->blur[1] = (int *)malloc(e->video_area * sizeof(int));
  if(priv->blur[0] == NULL || priv->blur[1] == NULL)
    {
    return -1;
    }
  
  memset(priv->blur[0], 0, e->video_area * sizeof(int));
  memset(priv->blur[1], 0, e->video_area * sizeof(int));
  priv->blurFrame = 0;

  image_init(e);
  image_set_threshold_y(e, MAGIC_THRESHOLD);

  priv->state = 1;
  return 0;
  }

static int stop(effect * e)
  {
  edgeblur_t * priv = (edgeblur_t *)e->priv;
  if(priv->state)
    {
    priv->state = 0;
    free(priv->blur[0]);
    free(priv->blur[1]);
    }
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  unsigned char *diff;
  int v;
  int *p, *q;
  edgeblur_t * priv = (edgeblur_t *)e->priv;

  diff = image_edge(e, src);

  p = priv->blur[priv->blurFrame    ] + e->video_width + 1;
  q = priv->blur[priv->blurFrame ^ 1] + e->video_width + 1;
  diff += e->video_width + 1;
  dest += e->video_width + 1;
  for(y=e->video_height - 2; y>0; y--) {
  for(x=e->video_width - 2; x>0; x--) {
  if(*diff++ > 64) {
  v = MAX_BLUR;
  } else {
  v = *(p - e->video_width) + *(p - 1) + *(p + 1) + *(p + e->video_width);
  if(v > 3) v -= 3;
  v = v >> 2;
  }
  *dest++ = priv->palette[v];
  *q++ = v;
  p++;

  }
  p += 2;
  q += 2;
  diff += 2;
  dest += 2;
  }

  priv->blurFrame ^= 1;

  return 0;
  }

#if 0

static int event(SDL_Event *event)
  {
  if(event->type == SDL_KEYDOWN) {
  switch(event->key.keysym.sym) {
  case SDLK_r:
    palette = &palettes[(MAX_BLUR + 1)*2];
    break;
  case SDLK_g:
    palette = &palettes[(MAX_BLUR + 1)];
    break;
  case SDLK_b:
    palette = &palettes[0];
    break;
  case SDLK_w:
    palette = &palettes[(MAX_BLUR + 1)*3];
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
      .name =      "color",
      .long_name = TRS("Color"),
      .type = BG_PARAMETER_COLOR_RGB,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_color = { 1.0, 1.0, 1.0, 1.0 } },
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
  bg_effectv_plugin_t * vp = (bg_effectv_plugin_t *)data;
  edgeblur_t * priv = (edgeblur_t*)vp->e->priv;
  
  if(!name)
    return;
  EFFECTV_SET_PARAM_COLOR(color);
  makePalette(priv);
  }

static void * create_edgeblurtv()
  {
  return bg_effectv_create(edgeBlurRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_edgeblurtv",
      .long_name = TRS("EdgeblurTV"),
      .description = TRS("Detects edge and display it with motion blur effect. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_edgeblurtv,
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
