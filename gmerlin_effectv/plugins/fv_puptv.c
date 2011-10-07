/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * PUPTV - comes from "Partial UPdate", certain part of image is updated at a
 *         frame. This techniques is also used by 1DTV, but the result is
 *         (slightly) different.
 * Copyright (C) 2003 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

static int start(effect *);
static int stop(effect *);
static int draw(effect *, RGB32 *src, RGB32 *dest);


typedef struct
  {
  int state;
  RGB32 *buffer;
  int bgIsSet;
  int mode;
  
  float strength;
  int phase;
  } pup_t;

static void randomPup(effect *, RGB32 *);
static void diagonalPup(effect *, RGB32 *);
static void dissolutionPup(effect *, RGB32 *);
static void verticalPup(effect *, RGB32 *);
static void horizontalPup(effect *, RGB32 *);
static void rasterPup(effect *, RGB32 *);

static int resetBuffer(effect * e, RGB32 *src)
  {
  pup_t * priv = e->priv;
  memcpy(priv->buffer, src, e->video_area * PIXEL_SIZE);
  priv->bgIsSet = 1;
  
  return 0;
  }

static effect *pupRegister()
  {
  effect *entry;
  pup_t * priv = calloc(1, sizeof(*priv));
        
  entry = calloc(1, sizeof(effect));
  if(entry == NULL)
    return NULL;
  entry->priv = priv;
  entry->start = start;
  entry->stop = stop;
  entry->draw = draw;

  return entry;
  }

static int start(effect * e)
  {
  pup_t * priv = e->priv;
  priv->bgIsSet = 0;
  priv->state = 1;
  priv->buffer = malloc(e->video_area * PIXEL_SIZE);
  if(priv->buffer == NULL)
    return -1;
        
  return 0;
  }

static int stop(effect * e)
  {
  pup_t * priv = e->priv;
  priv->state = 0;
  free(priv->buffer);
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  pup_t * priv = e->priv;
  if(!priv->bgIsSet) {
  resetBuffer(e, src);
  }

  switch(priv->mode) {
  case 0:
    verticalPup(e, src);
    break;
  case 1:
    horizontalPup(e, src);
    break;
  case 2:
    diagonalPup(e, src);
    break;
  case 3:
    dissolutionPup(e, src);
    break;
  case 4:
    randomPup(e, src);
    break;
  case 5:
    rasterPup(e, src);
    break;
  default:
    break;
  }

  memcpy(dest, priv->buffer, e->video_area * PIXEL_SIZE);

  return 0;
  }


static void randomPup(effect * e, RGB32 *src)
  {
  int i, x;
  pup_t * priv = e->priv;
  int pixNum = 100 + (int)(priv->strength * (10000 - 100) + 0.5);
  
  for(i=pixNum; i>0; i--) {
  x = inline_fastrand(e) % e->video_area;
  priv->buffer[x] = src[x];
  }
  }

static void diagonalPup(effect * e, RGB32 *src)
  {
  int x, y, s;
  RGB32 *p;
  pup_t * priv = e->priv;

  int step = -100 + (int)(priv->strength * (200) + 0.5);;
  
  if(step == 0) {
  memcpy(priv->buffer, src, e->video_area * PIXEL_SIZE);
  return;
  }

  s = abs(step);

  p = priv->buffer;
  for(y=0; y<e->video_height; y++) {
  if(step > 0) {
  x = (priv->phase + y) % s;
  } else {
  x = (priv->phase - y) % s;
  }
  for(; x<e->video_width; x+=s) {
  p[x] = src[x];
  }
  src += e->video_width;
  p += e->video_width;
  }

  priv->phase++;
  if(priv->phase >= s)
    priv->phase = 0;
  }

static void dissolutionPup(effect * e, RGB32 *src)
  {
  int i;
  pup_t * priv = e->priv;
  
  int step = 1 + (int)(priv->strength * 99 + 0.5);;

  for(i=priv->phase; i<e->video_area; i+=step) {
  priv->buffer[i] = src[i];
  }

  priv->phase++;
  if(priv->phase >= step)
    priv->phase = 0;
  }

static void verticalPup(effect * e, RGB32 *src)
  {
  int x, y;
  RGB32 *dest;
  pup_t * priv = e->priv;
  int step = 2 + (int)(priv->strength * (e->video_width-1) + 0.5);;
  
  dest = priv->buffer;
  for(y=0; y<e->video_height; y++)
    {
    for(x=priv->phase; x<e->video_width; x+=step)
      {
      dest[x] = src[x];
      }
    src += e->video_width;
    dest += e->video_width;
    }
  
  priv->phase++;
  while(priv->phase >= step)
    {
    priv->phase -= step;
    }
  }

static void horizontalPup(effect * e, RGB32 *src)
  {
  int y;
  RGB32 *dest;
  pup_t * priv = e->priv;
  int step = 2 + (int)(priv->strength * (e->video_height-2) + 0.5);;
  
  src += e->video_width * priv->phase;
  dest = priv->buffer + e->video_width * priv->phase;
  for(y=priv->phase; y<e->video_height; y+=step) {
  memcpy(dest, src, e->video_width * PIXEL_SIZE);
  src += e->video_width * step;
  dest += e->video_width * step;
  }

  priv->phase++;
  while(priv->phase >= step)
    {
    priv->phase -= step;
    }
  }

static void rasterPup(effect * e, RGB32 *src)
  {
  int x, y;
  unsigned int offset;
  RGB32 *dest;
  pup_t * priv = e->priv;
  int step = 2 + (int)(priv->strength * (e->video_height-2) + 0.5);;

  offset = 0;

  dest = priv->buffer;
  for(y=0; y<e->video_height; y++)
    {
    if(y&1)
      {
      for(x=priv->phase; x<e->video_width; x+=step)
        {
        dest[x] = src[x];
        }
      }
    else
      {
      for(x=e->video_width-1-priv->phase;x>=0; x-=step)
        {
        dest[x] = src[x];
        }
      }
    src += e->video_width;
    dest += e->video_width;
    }
  
  priv->phase++;
  while(priv->phase >= step)
    {
    priv->phase -= step;
    }
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "horizontal" },
      .multi_names = (char const *[]){ "horizontal",
                                       "vertical",
                                       "diagonal",
                                       "dissolution",
                                       "random",
                                       "raster", NULL },
      
      .multi_labels = (char const *[]){ TRS("Horizontal"),
                                        TRS("Vertical"),
                                        TRS("Diagonal"),
                                        TRS("Dissolution"),
                                        TRS("Random"),
                                        TRS("Raster"), NULL },
    },
    {
      .name =      "strength",
      .long_name = TRS("Strength"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 1.0 },
      .val_default = { .val_f = 0.2 },
      .num_digits = 2,
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
  int changed = 0;
  bg_effectv_plugin_t * vp = data;
  pup_t * priv = vp->e->priv;
  
  if(!name)
    return;
  if(!strcmp(name, "mode"))
    {
    if(!strcmp(val->val_str, "horizontal"))
      priv->mode = 1;
    else if(!strcmp(val->val_str, "vertical"))
      priv->mode = 0;
    else if(!strcmp(val->val_str, "diagonal"))
      priv->mode = 2;
    else if(!strcmp(val->val_str, "dissolution"))
      priv->mode = 3;
    else if(!strcmp(val->val_str, "random"))
      priv->mode = 4;
    else if(!strcmp(val->val_str, "raster"))
      priv->mode = 5;
    priv->phase = 0;
    }
  EFFECTV_SET_PARAM_FLOAT(strength);
  }

static void * create_puptv()
  {
  return bg_effectv_create(pupRegister, BG_EFFECTV_COLOR_AGNOSTIC);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_puptv",
      .long_name = TRS("PupTV"),
      .description = TRS("PupTV does pup mark effect on the video input. The pup is caused by a motion or random rain drops. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_puptv,
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
