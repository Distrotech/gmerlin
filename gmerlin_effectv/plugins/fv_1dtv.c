/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * 1DTV - scans line by line and generates amazing still image.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);


typedef struct
  {
  int state;
  int line;
  int prevline;
  RGB32 *linebuf;
  } onedtv_t;

static effect *onedRegister(void)
  {
  effect *entry;
  onedtv_t * priv;
  priv = calloc(1, sizeof(*priv));
  
  entry = calloc(1, sizeof(effect));
  entry->priv = priv;
  if(entry == NULL)
    return NULL;
	
  entry->start = start;
  entry->stop = stop;
  entry->draw = draw;

  return entry;
  }

static int start(effect * e)
  {
  onedtv_t * priv = e->priv;
 
  priv->linebuf = malloc(e->video_width * PIXEL_SIZE);
  if(priv->linebuf == NULL)
    return -1;
  memset(priv->linebuf, 0, e->video_width * PIXEL_SIZE);
  
  
  
  priv->line = 0;
  priv->prevline = 0;
  
  priv->state = 1;
  
  return 0;
  }

static int stop(effect * e)
  {
  onedtv_t * priv = e->priv;
  priv->state = 0;
  if(priv->linebuf) free(priv->linebuf);
  return 0;
  }


static void blitline_buf(effect * e, RGB32 *src, RGB32 *dest)
  {
  onedtv_t * priv = e->priv;
  memcpy(dest + e->video_width * priv->prevline,
         priv->linebuf, e->video_width * PIXEL_SIZE);

  src += e->video_width * priv->line;
  dest += e->video_width * priv->line;
  memcpy(dest, src, e->video_width * PIXEL_SIZE);
  memcpy(priv->linebuf, src, e->video_width * PIXEL_SIZE);

  priv->prevline = priv->line;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  int i;
  onedtv_t * priv = e->priv;

  blitline_buf(e, src, dest);

  priv->line++;
  if(priv->line >= e->video_height)
    priv->line = 0;

  dest += e->video_width * priv->line;
  for(i=0; i<e->video_width; i++) {
  dest[i] = 0xff00;
  }

  return 0;
  }

static void * create_1dtv()
  {
  return bg_effectv_create(onedRegister, BG_EFFECTV_REUSE_OUTPUT);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_onedtv",
      .long_name = TRS("1DTV"),
      .description = TRS("1DTV is one of the most amazing effect, but that algorithm is very easy. The horizontal green line is the current scanning position and it moves down every frame. So only moving objects is distorted. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_1dtv,
      .destroy =   bg_effectv_destroy,
      //      .get_parameters =   get_parameters_invert,
      //      .set_parameter =    set_parameter_invert,
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
