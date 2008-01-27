/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * LifeTV - Play John Horton Conway's `Life' game with video input.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * This idea is stolen from Nobuyuki Matsushita's installation program of
 * ``HoloWall''. (See http://www.csl.sony.co.jp/person/matsu/index.html)
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dest);

typedef struct
  {
  int stat;
  unsigned char *field, *field1, *field2;
  } life_t;

static void clear_field(effect * e)
  {
  life_t * priv;
  priv = (life_t *)e->priv;
  memset(priv->field1, 0, e->video_area);
  }

static effect *lifeRegister(void)
  {
  effect *entry;
  life_t * priv;
  
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
  life_t * priv = (life_t *)e->priv;
  image_init(e);
  image_set_threshold_y(e, 40);

  priv->field = (unsigned char *)malloc(e->video_area*2);
  if(priv->field == NULL)
    return -1;
  
  priv->field1 = priv->field;
  priv->field2 = priv->field + e->video_area;
  clear_field(e);
  
  priv->stat = 1;
  return 0;
  }

static int stop(effect * e)
  {
  life_t * priv = (life_t *)e->priv;
  free(priv->field);
  priv->stat = 0;
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  unsigned char *p, *q, v;
  unsigned char sum, sum1, sum2, sum3;
  int width;
  RGB32 pix;
  life_t * priv = (life_t *)e->priv;
  
  width = e->video_width;
  
  p = image_diff_filter(e, image_bgsubtract_update_y(e, src));
  for(x=0; x<e->video_area; x++)
    {
    priv->field1[x] |= p[x];
    }
  
  p = priv->field1 + 1;
  q = priv->field2 + width + 1;
  dest += width + 1;
  src += width + 1;
/* each value of cell is 0 or 0xff. 0xff can be treated as -1, so
 * following equations treat each value as negative number. */
  for(y=1; y<e->video_height-1; y++)
    {
    sum1 = 0;
    sum2 = p[0] + p[width] + p[width*2];
    for(x=1; x<width-1; x++)
      {
      sum3 = p[1] + p[width+1] + p[width*2+1];
      sum = sum1 + sum2 + sum3;
      v = 0 - ((sum==0xfd)|((p[width]!=0)&(sum==0xfc)));
      *q++ = v;
      pix = (signed char)v;
      //			pix = pix >> 8;
      *dest++ = pix | *src++;
      sum1 = sum2;
      sum2 = sum3;
      p++;
      }
    p += 2;
    q += 2;
    src += 2;
    dest += 2;
    }
  p = priv->field1;
  priv->field1 = priv->field2;
  priv->field2 = p;
  
  return 0;
  }

static void * create_lifetv()
  {
  return bg_effectv_create(lifeRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_lifetv",
      .long_name = TRS("LifeTV"),
      .description = TRS("You can play John Horton Conway's Life Game with video input. Moving objects drop seeds on the game field. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_lifetv,
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
