/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * Plugin for EffecTV by Fukuchi Kentarou
 * Written by clifford smith <nullset@dookie.net>
 * 
 * TransForm.c: Performs positinal translations on images
 * 
 * Space selects b/w the different transforms
 *
 * basicaly TransformList contains an array of 
 * values indicating where pixels go.
 * This value could be stored here or generated. The ones i use
 * here are generated.
 * Special value: -1 = black, -2 = get values from mapFromT(x,y,t)
 * ToDo: support multiple functions ( -3 to -10 or something?)
 * Note: the functions CAN return -1 to mean black....
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#define TableMax  6 /* # of transforms */

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dest);

typedef struct
  {
  int state;
  int *TableList[TableMax];
  int transform; /* Which transform to use */
  int t;
  } transform_t;

static int mapFromT(effect * e, int x,int y, int t) {
  int xd,yd;
  yd = y + (fastrand(e) >> 30)-2;
  xd = x + (fastrand(e) >> 30)-2;
  if (xd > e->video_width) {
    xd-=1;
  }
  return xd + yd*e->video_width;
}

static effect *transformRegister(void)
  {
  effect *entry;
  transform_t * priv;
  entry = calloc(1, sizeof(effect));
  if(entry == NULL) return NULL;

  priv = calloc(1, sizeof(*priv));
  entry->priv = priv;
  entry->start = start;
  entry->stop = stop;
  entry->draw = draw;
  return entry;
  }

static void SquareTableInit(effect * e)
{
	const int size = 16;
	int x, y, tx, ty;
        transform_t * priv = e->priv;
	for(y=0; y<e->video_height; y++) {
		ty = y % size - size / 2;
		if((y/size)%2)
			ty = y - ty;
		else
			ty = y + ty;
		if(ty<0) ty = 0;
		if(ty>=e->video_height) ty = e->video_height - 1;
		for(x=0; x<e->video_width; x++) {
			tx = x % size - size / 2;
			if((x/size)%2)
				tx = x - tx;
			else
				tx = x + tx;
			if(tx<0) tx = 0;
			if(tx>=e->video_width) tx = e->video_width - 1;
			priv->TableList[5][x+y*e->video_width] = ty*e->video_width+tx;
		}
	}
}

static int start(effect * e)
{
        transform_t * priv = e->priv;
  int x,y,i;
//   int xdest,ydest;

  for (i=0;i < TableMax ; i++) {
    priv->TableList[i]= malloc(sizeof(int) * e->video_width * e->video_height);
  }

  for (y=0;y < e->video_height;y++) {
    for (x=0;x < e->video_width;x++) {

      priv->TableList[0][x+y*e->video_width]=     
	priv->TableList[1][(e->video_width-1-x)+y*e->video_width] = 
	priv->TableList[2][x+(e->video_height-1-y)*e->video_width] = 
	priv->TableList[3][(e->video_width-1-x)+(e->video_height-1-y)*e->video_width] = 
	x+y*e->video_width;
      priv->TableList[4][x+y*e->video_width] = -2; /* Function */
	}

  }
  SquareTableInit(e);
  priv->t = 0;
  priv->state = 1;
  return 0;
}

static int stop(effect * e)
{
  int i;
        transform_t * priv = e->priv;

  if(priv->state) {
	priv->state = 0;
	for (i=0; i < TableMax ; i++) {
	  free(priv->TableList[i]);
	}

  }
	return 0;
}

static int draw(effect * e, RGB32 *src, RGB32 *dst)
{
  int x,y;
  transform_t * priv = e->priv;
  priv->t++;
  
  for (y=0;y < e->video_height;y++)
    for (x=0;x < e->video_width;x++)
      {
      int dest,value;
      // printf("%d,%d\n",x,y);
      dest = priv->TableList[priv->transform][x + y*e->video_width];
      if (dest == -2)
        {
        dest = mapFromT(e, x,y,priv->t);
        }
      if (dest == -1)
        {
        value = 0;
        }
      else
        {
        value = *(RGB32 *)(src + dest);	 
        }
      *(RGB32 *)(dst+x+y*e->video_width) = value;
      }
  return 0;
}


static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "transform",
      .long_name = TRS("Transformation"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 0   },
      .val_max =     { .val_i = TableMax-1 },
      .val_default = { .val_i = 0 },
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
  int changed;
  bg_effectv_plugin_t * vp = data;
  transform_t * priv = vp->e->priv;
  
  if(!name)
    return;
  EFFECTV_SET_PARAM_INT(transform);
  }

static void * create_transformtv()
  {
  return bg_effectv_create(transformRegister, BG_EFFECTV_COLOR_AGNOSTIC);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_transformtv",
      .long_name = TRS("TransformTV"),
      .description = TRS("TransformTV does transform mark effect on the video input. The transform is caused by a motion or random rain drops. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_transformtv,
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
