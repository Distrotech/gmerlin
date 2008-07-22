/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * From main.c of warp-1.1:
 *
 *      Simple DirectMedia Layer demo
 *      Realtime picture 'gooing'
 *      Released under GPL
 *      by sam lantinga slouken@devolution.com
 */

#include <stdlib.h>
#include <string.h>
#include "gmerlin_effectv.h"
#include "utils.h"
#include <math.h>

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

static void initWarp(effect *e);
static void disposeWarp (effect *e);
static void doWarp (effect *e, int xw, int yw, int cw,RGB32 *src,RGB32 *dst);

typedef struct
  {
  int *offstable;
  int32_t *disttable;
  int32_t ctable[1024];
  int32_t sintable[1024+256];
  int state;
  int tval;
  } warptv_t;

static int start(effect *e);
static int stop(effect *e);
static int draw(effect *e, RGB32 *src, RGB32 *dest);


static effect *warpRegister(void)
  {
  effect *entry;
  warptv_t * priv = calloc(1, sizeof(*priv));
  
  entry = (effect *)calloc(1, sizeof(effect));
  if(entry == NULL) return NULL;
	
  entry->start = start;
  entry->stop = stop;
  entry->draw = draw;
  entry->priv = priv;
  return entry;
  }

static int start(effect *e)
  {
  warptv_t * priv = (warptv_t*)e->priv;
  initWarp(e);
  priv->state = 1;
  return 0;
  }

static int stop(effect *e)
  {
  warptv_t * priv = (warptv_t*)e->priv;
  if(priv->state)
    {
    priv->state = 0;
    disposeWarp(e);
    }
  return 0;
  }

static void initSinTable (effect *e)
  {
  int32_t	*tptr, *tsinptr;
  double	i;
  warptv_t * priv = (warptv_t*)e->priv;
  
  tsinptr = tptr = priv->sintable;
  
  for (i = 0; i < 1024; i++)
    *tptr++ = (int) (sin (i*M_PI/512) * 32767);
  
  for (i = 0; i < 256; i++)
    *tptr++ = *tsinptr++;
  }

static void initOffsTable (effect *e)
  {
  int y;
  warptv_t * priv = (warptv_t*)e->priv;
  
  for (y = 0; y < e->video_height; y++)
    {
    priv->offstable[y] = y * e->video_width;
    }
  }

static void initDistTable (effect *e)
  {
  int32_t	halfw, halfh, *distptr;
#ifdef PS2
  float	x,y,m;
#else
  double	x,y,m;
#endif
  warptv_t * priv = (warptv_t*)e->priv;

  halfw = e->video_width>> 1;
  halfh = e->video_height >> 1;

  distptr = priv->disttable;

  m = sqrt ((double)(halfw*halfw + halfh*halfh));

  for (y = -halfh; y < halfh; y++)
    for (x= -halfw; x < halfw; x++)
#ifdef PS2
      *distptr++ = ((int)
                    ( (sqrtf (x*x+y*y) * 511.9999) / m)) << 1;
#else
  *distptr++ = ((int)
                ( (sqrt (x*x+y*y) * 511.9999) / m)) << 1;
#endif
  }

static void initWarp (effect *e)
  {
  warptv_t * priv = (warptv_t*)e->priv;

  priv->offstable = (int *)malloc (e->video_height * sizeof (int));      
  priv->disttable = malloc (e->video_width * e->video_height * sizeof (int));
  initSinTable (e);
  initOffsTable (e);
  initDistTable (e);
  }

static void disposeWarp (effect *e)
  {
  warptv_t * priv = (warptv_t*)e->priv;
  if(priv->disttable)
    free (priv->disttable);
  if(priv->offstable)
    free (priv->offstable);
  }

static int draw(effect *e, RGB32 *src, RGB32 *dst)
  {
  int xw,yw,cw;
  warptv_t * priv = (warptv_t*)e->priv;
  
  xw  = (int) (sin((priv->tval+100)*M_PI/128) * 30);
  yw  = (int) (sin((priv->tval)*M_PI/256) * -35);
  cw  = (int) (sin((priv->tval-70)*M_PI/64) * 50);
  xw += (int) (sin((priv->tval-10)*M_PI/512) * 40);
  yw += (int) (sin((priv->tval+30)*M_PI/512) * 40);	  

  doWarp(e, xw,yw,cw,src,dst);
  priv->tval = (priv->tval+1) &511;
  
  return 0;
  }

static void doWarp (effect *e, int xw, int yw, int cw,RGB32 *src,RGB32 *dst)
  {
  warptv_t * priv = (warptv_t*)e->priv;
  
  int32_t c,i, x,y, dx,dy, maxx, maxy;
  int32_t width, height, skip, *ctptr, *distptr;
  uint32_t *destptr;
  //	Uint32 **offsptr;

  ctptr = priv->ctable;
  distptr = priv->disttable;
  width = e->video_width;
  height = e->video_height;
  destptr = dst;
  skip = 0 ; /* video_width*sizeof(RGB32)/4 - video_width;; */
  c = 0;
  for (x = 0; x < 512; x++)
    {
    i = (c >> 3) & 0x3FE;
    *ctptr++ = ((priv->sintable[i] * yw) >> 15);
    *ctptr++ = ((priv->sintable[i+256] * xw) >> 15);
    c += cw;
    }
  maxx = width - 2; maxy = height - 2;
  /*	printf("Forring\n"); */
  for (y = 0; y < height-1; y++)
    {
    for (x = 0; x < width; x++)
      {
      i = *distptr++; 
      dx = priv->ctable [i+1] + x; 
      dy = priv->ctable [i] + y;	 
      
      
      if (dx < 0) dx = 0; 
      else if (dx > maxx) dx = maxx; 
      
      if (dy < 0) dy = 0; 
      else if (dy > maxy) dy = maxy; 
      /* 	   printf("f:%d\n",dy); */
      /*	   printf("%d\t%d\n",dx,dy); */
      *destptr++ = src[priv->offstable[dy]+dx]; 
      }
    destptr += skip;
    }
  }

static void * create_warptv()
  {
  return bg_effectv_create(warpRegister, BG_EFFECTV_COLOR_AGNOSTIC);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_warptv",
      .long_name = TRS("WarpTV"),
      .description = TRS("WarpTV does realtime goo'ing of the video input. based on warp-1.1 SDL demo by Sam Latinga (http://libSDL.org). Original version by Emmanuel Marty <core at ggi-project dawt org>. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_warptv,
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
