/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * DisplayWall
 * Copyright (C) 2005-2006 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include <utils.h>

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);


typedef struct
  {
  int state;

  int *vecx;
  int *vecy;
  int scale;
  int bx, by;
  int speedx;
  int speedy;
  int cx, cy;
  } displaywalltv_t;

static void initVec(effect * e);

static effect *displayWallRegister(void)
  {
  effect *entry;
  displaywalltv_t * priv;
  
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

static void initVec(effect * e)
  {
  int x, y, i;
  double vx, vy;
  displaywalltv_t * priv = e->priv;
  i = 0;
  for(y=0; y<e->video_height; y++)
    {
    for(x=0; x<e->video_width; x++)
      {
      vx = (double)(x - priv->cx) / e->video_width;
      vy = (double)(y - priv->cy) / e->video_width;
      
      vx *= 1.0 - vx * vx * 0.4;
      vy *= 1.0 - vx * vx * 0.8;
      vx *= 1.0 - (double)y / e->video_height * 0.15;
      priv->vecx[i] = vx * e->video_width;
      priv->vecy[i] = vy * e->video_width;
      i++;
      }
    }
  }

static int start(effect * e)
  {
  displaywalltv_t * priv = e->priv;
  priv->state = 1;
  
  priv->cx = e->video_width / 2;
  priv->cy = e->video_height / 2;
  
  priv->vecx = malloc(sizeof(int) * e->video_area);
  priv->vecy = malloc(sizeof(int) * e->video_area);

  if(priv->vecx == NULL || priv->vecy == NULL)
    return -1;

  initVec(e);

  return 0;
  }

static int stop(effect * e)
  {
  displaywalltv_t * priv = e->priv;
  priv->state = 0;
  free(priv->vecx);
  free(priv->vecy);
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y, i;
  int px, py;
  displaywalltv_t * priv = e->priv;
  
  priv->bx += priv->speedx;
  priv->by += priv->speedy;
  while(priv->bx < 0) priv->bx += e->video_width;
  while(priv->bx >= e->video_width) priv->bx -= e->video_width;
  while(priv->by < 0) priv->by += e->video_height;
  while(priv->by >= e->video_height) priv->by -= e->video_height;

  if(priv->scale == 1)
    {
    priv->bx = priv->cx;
    priv->by = priv->cy;
    }
  
  i = 0;
  for(y=0; y<e->video_height; y++)
    {
    for(x=0; x<e->video_width; x++)
      {
      px = priv->bx + priv->vecx[i] * priv->scale;
      py = priv->by + priv->vecy[i] * priv->scale;
      while(px < 0) px += e->video_width;
      while(px >= e->video_width) px -= e->video_width;
      while(py < 0) py += e->video_height;
      while(py >= e->video_height) py -= e->video_height;
      dest[i++] = src[py * e->video_width + px];
      }
    }
  
  return 0;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "scale",
      .long_name = TRS("Scale"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 9 },
      .val_default = { .val_i = 3 },
    },
    {
      .name =      "speedx",
      .long_name = TRS("Horizontal scroll speed"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = -100 },
      .val_max =     { .val_i = 100 },
      .val_default = { .val_i = 10 },
    },
    {
      .name =      "speedy",
      .long_name = TRS("Vertical scroll speed"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = -100 },
      .val_max =     { .val_i = 100 },
      .val_default = { .val_i = 10 },
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
  displaywalltv_t * priv = vp->e->priv;
  int changed = 0;
  if(!name)
    return;

  EFFECTV_SET_PARAM_INT(speedx);
  EFFECTV_SET_PARAM_INT(speedy);
  EFFECTV_SET_PARAM_INT(scale);
  }

static void * create_displaywalltv()
  {
  return bg_effectv_create(displayWallRegister, BG_EFFECTV_COLOR_AGNOSTIC);
  }



const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_displaywalltv",
      .long_name = TRS("DisplaywallTV"),
      .description = TRS("Display the tiled video images. You can scroll the image or change the scale. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_displaywalltv,
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



