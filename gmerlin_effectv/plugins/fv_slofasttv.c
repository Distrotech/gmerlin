/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * slofastTV - nonlinear time TV
 * Copyright (C) 2005 SCHUBERT Erich
 * based on slofastTV Copyright (C) 2002 TANNENBAUM Edo
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#define PLANES 8


static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);


typedef struct
  {
  int state;
  RGB32 *buffer;
  RGB32 *planetable[PLANES];
  int mode;
  int head;
  int tail;
  int count;
  } slofast_t;


#define STATE_STOP  0
#define STATE_FILL  1
#define STATE_FLUSH 2

static effect *slofastRegister(void)
  {
  effect *entry;
  slofast_t * priv;
  entry = calloc(1, sizeof(effect));
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
  int i;
  slofast_t * priv = e->priv;
  priv->buffer = malloc(e->video_area*PIXEL_SIZE*PLANES);
  if(priv->buffer == NULL)
    return -1;
  memset(priv->buffer, 0, e->video_area*PIXEL_SIZE*PLANES);
  for(i=0;i<PLANES;i++)
    priv->planetable[i] = &priv->buffer[e->video_area*i];
  
  priv->head  = 0;
  priv->tail  = 0;
  priv->count = 0;
  
  priv->mode = STATE_FILL;
  return 0;
  }

static int stop(effect * e)
  {
  slofast_t * priv = e->priv;
  if(priv->state)
    {
    if(priv->buffer)
      free(priv->buffer);
    priv->state = 0;
    }
  
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  slofast_t * priv = e->priv;
  /* store new frame */
  if ((priv->mode == STATE_FILL) || (priv->count & 0x1) == 1)
    {
    memcpy(priv->planetable[priv->head], src, e->video_area * PIXEL_SIZE);
    priv->head++;
    while (priv->head >= PLANES) priv->head=0;
    
    /* switch mode when head catches tail */
    if (priv->head == priv->tail) priv->mode = STATE_FLUSH;
    }
  
  /* copy current tail image */
  if ((priv->mode == STATE_FLUSH) || (priv->count & 0x1) == 1)
    {
    memcpy(dest, priv->planetable[priv->tail], e->video_area * PIXEL_SIZE);
    priv->tail++;
    while (priv->tail >= PLANES) priv->tail -= PLANES;
    
    /* switch mode when tail reaches head */
    if (priv->head == priv->tail) priv->mode = STATE_FILL;
    }
  
  priv->count++;
  
  return 0;
  }

static void * create_slofasttv()
  {
  return bg_effectv_create(slofastRegister, BG_EFFECTV_COLOR_AGNOSTIC);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_slofasttv",
      .long_name = TRS("SlofastTV"),
      .description = TRS("SloFastTV plays back the current video input at non-constant speed: while the buffer fills the video is played back at half the frame rate, when the buffer is full it plays back at the double rate until it has caught up with the live video again. This causes the actual image to be delayed from 0 to about half a second. Movements that previously had a constant speed will appear very slow and then very fast. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_slofasttv,
      .destroy =   bg_effectv_destroy,
      //      .get_parameters =   get_parameters,
      //      .set_parameter =    set_parameter,
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
