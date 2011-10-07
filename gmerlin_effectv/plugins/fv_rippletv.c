/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * RippleTV - Water ripple effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */


#include <stdlib.h>
#include <string.h>
#include "gmerlin_effectv.h"
#include "utils.h"

#define MAGIC_THRESHOLD 70

/* Might be set from the commandline, but we don't want to debug this */
#ifdef DEBUG
#undef DEBUG
#endif

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dest);


/* At least some of these could be parameters */

//static const int point = 16;
static const int impact = 2;

typedef struct
  {
  int mode; // 0 = motion detection / 1 = rain
  int stat;
  signed char *vtable;
  int *map;
  int *map1, *map2, *map3;
  int map_h, map_w;
  int sqrtable[256];
  int bgIsSet;
  int loopnum;
  int point;
  int period;
  int rain_stat;
  unsigned int drop_prob;
  int drop_prob_increment;
  int drops_per_frame_max;
  int drops_per_frame;
  int drop_power;

  int strength;
  int decay;
  int decay_cfg;
  } ripple_t;

static void setTable(effect * e)
  {
  int i;
  ripple_t * priv = e->priv;
  for(i=0; i<128; i++)
    {
    priv->sqrtable[i] = i*i;
    }
  for(i=1; i<=128; i++)
    {
    priv->sqrtable[256-i] = -i*i;
    }
  }

static int setBackground(effect * e, RGB32 *src)
  {
  ripple_t * priv = e->priv;
  
  image_bgset_y(e, src);
  priv->bgIsSet = 1;
  
  return 0;
  }

static effect *rippleRegister(void)
{
	effect *entry;
        ripple_t * priv;

        entry = calloc(1, sizeof(effect));
	if(entry == NULL) {
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
  ripple_t * priv = e->priv;
  image_init(e);
  priv->map_h = e->video_height / 2 + 1;
  priv->map_w = e->video_width / 2 + 1;
  priv->map = malloc(priv->map_h*priv->map_w*3*sizeof(int));
  priv->vtable = malloc(priv->map_h*priv->map_w*2*sizeof(signed char));
  if(priv->map == NULL || priv->vtable == NULL)
    {
    return 0;
    }
  
  priv->map3 = priv->map + priv->map_w * priv->map_h * 2;

  //  priv->mode = 1;
  
  setTable(e);
        
  memset(priv->map, 0, priv->map_h*priv->map_w*3*sizeof(int));
  memset(priv->vtable, 0, priv->map_h*priv->map_w*2*sizeof(signed char));
  priv->map1 = priv->map;
  priv->map2 = priv->map + priv->map_h*priv->map_w;
  image_set_threshold_y(e, MAGIC_THRESHOLD);
  //  priv->bgIsSet = 1;

  priv->period = 0;
  priv->rain_stat = 0;
  priv->drop_prob = 0;
  priv->drop_prob_increment = 0;
  priv->drops_per_frame_max = 0;
  priv->drops_per_frame = 0;
  priv->drop_power = 0;

  
  priv->stat = 1;
  return 1;
  }

static int stop(effect * e)
  {
  ripple_t * priv = e->priv;

  free(priv->map);
  free(priv->vtable);
  
  priv->stat = 0;
  return 0;
  }

static void motiondetect(effect * e, RGB32 *src)
  {
  unsigned char *diff;
  int width;
  int *p, *q;
  int x, y, h;
  ripple_t * priv = e->priv;

  if(!priv->bgIsSet)
    {
    setBackground(e, src);
    priv->bgIsSet = 1;
    }
  
  diff = image_bgsubtract_update_y(e, src);
  width = e->video_width;
  p = priv->map1+priv->map_w+1;
  q = priv->map2+priv->map_w+1;
  diff += width+2;

  for(y=priv->map_h-2; y>0; y--)
    {
    for(x=priv->map_w-2; x>0; x--)
      {
      h = (int)*diff + (int)*(diff+1) + (int)*(diff+width) + (int)*(diff+width+1);
      if(h>0)
        {
        *p = h<<(priv->point + impact - 8);
        *q = *p;
        }
      p++;
      q++;
      diff += 2;
      }
    diff += width+2;
    p+=2;
    q+=2;
    }
  }

static inline void drop(effect * e, int power)
  {
  int x, y;
  int *p, *q;
  ripple_t * priv = e->priv;
  
  x = fastrand(e)%(priv->map_w-4)+2;
  y = fastrand(e)%(priv->map_h-4)+2;
  p = priv->map1 + y*priv->map_w + x;
  q = priv->map2 + y*priv->map_w + x;
  *p = power;
  *q = power;
  *(p-priv->map_w) = *(p-1) = *(p+1) = *(p+priv->map_w) = power/2;
  *(p-priv->map_w-1) = *(p-priv->map_w+1) = *(p+priv->map_w-1) = *(p+priv->map_w+1) = power/4;
  *(q-priv->map_w) = *(q-1) = *(q+1) = *(q+priv->map_w) = power/2;
  *(q-priv->map_w-1) = *(q-priv->map_w+1) = *(q+priv->map_w-1) = *(p+priv->map_w+1) = power/4;
  }

static void raindrop(effect * e)
  {

  int i;
  ripple_t * priv = e->priv;

  if(priv->period == 0)
    {
    switch(priv->rain_stat)
      {
      case 0:
        priv->period = (fastrand(e)>>23)+100;
        priv->drop_prob = 0;
        priv->drop_prob_increment = 0x00ffffff/priv->period;
        priv->drop_power = (-(fastrand(e)>>28)-2)<<priv->point;
        priv->drops_per_frame_max = 2<<(fastrand(e)>>30); // 2,4,8 or 16
        priv->rain_stat = 1;
        break;
      case 1:
        priv->drop_prob = 0x00ffffff;
        priv->drops_per_frame = 1;
        priv->drop_prob_increment = 1;
        priv->period = (priv->drops_per_frame_max - 1) * 16;
        priv->rain_stat = 2;
        break;
      case 2:
        priv->period = (fastrand(e)>>22)+1000;
        priv->drop_prob_increment = 0;
        priv->rain_stat = 3;
        break;
      case 3:
        priv->period = (priv->drops_per_frame_max - 1) * 16;
        priv->drop_prob_increment = -1;
        priv->rain_stat = 4;
        break;
      case 4:
        priv->period = (fastrand(e)>>24)+60;
        priv->drop_prob_increment = -(priv->drop_prob/priv->period);
        priv->rain_stat = 5;
        break;
      case 5:
      default:
        priv->period = (fastrand(e)>>23)+500;
        priv->drop_prob = 0;
        priv->rain_stat = 0;
        break;
      }
    }
  switch(priv->rain_stat)
    {
    default:
    case 0:
      break;
    case 1:
    case 5:
      if((fastrand(e)>>8)<priv->drop_prob) {
      drop(e, priv->drop_power);
      }
      priv->drop_prob += priv->drop_prob_increment;
      break;
    case 2:
    case 3:
    case 4:
      for(i=priv->drops_per_frame/16; i>0; i--) {
      drop(e, priv->drop_power);
      }
      priv->drops_per_frame += priv->drop_prob_increment;
      break;
    }
  priv->period--;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y, i;
  int dx, dy;
  int h, v;
  int width, height;
  int *p, *q, *r;
  signed char *vp;
#ifdef DEBUG
  RGB32 *dest2;
#endif
  ripple_t * priv = e->priv;

  /* impact from the motion or rain drop */
  if(priv->mode)
    {
    raindrop(e);
    }
  else
    {
    motiondetect(e, src);
    }
  
  /* simulate surface wave */
  width = priv->map_w;
  height = priv->map_h;
        
  /* This function is called only 30 times per second. To increase a speed
   * of wave, iterates this loop several times. */
  for(i=priv->loopnum; i>0; i--)
    {
    /* wave simulation */
    p = priv->map1 + width + 1;
    q = priv->map2 + width + 1;
    r = priv->map3 + width + 1;
    for(y=height-2; y>0; y--)
      {
      for(x=width-2; x>0; x--)
        {
        h = *(p-width-1) + *(p-width+1) + *(p+width-1) + *(p+width+1)
          + *(p-width) + *(p-1) + *(p+1) + *(p+width) - (*p)*9;
        h = h >> 3;
        v = *p - *q;
        v += h - (v >> priv->decay);
        *r = v + *p;
        p++;
        q++;
        r++;
        }
      p += 2;
      q += 2;
      r += 2;
      }

    /* low pass filter */
    p = priv->map3 + width + 1;
    q = priv->map2 + width + 1;
    for(y=height-2; y>0; y--)
      {
      for(x=width-2; x>0; x--)
        {
        h = *(p-width) + *(p-1) + *(p+1) + *(p+width) + (*p)*60;
        *q = h >> 6;
        p++;
        q++;
        }
      p+=2;
      q+=2;
      }

    p = priv->map1;
    priv->map1 = priv->map2;
    priv->map2 = p;
    }

  vp = priv->vtable;
  p = priv->map1;
  for(y=height-1; y>0; y--)
    {
    for(x=width-1; x>0; x--)
      {
      /* difference of the height between two voxel. They are twiced to
       * emphasise the wave. */
      vp[0] = priv->sqrtable[((p[0] - p[1])>>(priv->point-1))&0xff]; 
      vp[1] = priv->sqrtable[((p[0] - p[width])>>(priv->point-1))&0xff]; 
      p++;
      vp+=2;
      }
    p++;
    vp+=2;
    }

  height = e->video_height;
  width = e->video_width;
  vp = priv->vtable;

#ifndef DEBUG
  /* draw refracted image. The vector table is stretched. */
  for(y=0; y<height; y+=2)
    {
    for(x=0; x<width; x+=2)
      {
      h = (int)vp[0];
      v = (int)vp[1];
      dx = x + h;
      dy = y + v;
      if(dx<0) dx=0;
      if(dy<0) dy=0;
      if(dx>=width) dx=width-1;
      if(dy>=height) dy=height-1;
      dest[0] = src[dy*width+dx];

      i = dx;

      dx = x + 1 + (h+(int)vp[2])/2;
      if(dx<0) dx=0;
      if(dx>=width) dx=width-1;
      dest[1] = src[dy*width+dx];

      dy = y + 1 + (v+(int)vp[priv->map_w*2+1])/2;
      if(dy<0) dy=0;
      if(dy>=height) dy=height-1;
      dest[width] = src[dy*width+i];

      dest[width+1] = src[dy*width+dx];
      dest+=2;
      vp+=2;
      }
    dest += e->video_width;
    vp += 2;
    }
#else
  dest2 = dest;
  p = priv->map1;
  for(y=0; y<height; y+=2)
    {
    for(x=0; x<width; x+=2)
      {
      h = ((p[0]>>(priv->point-5))+128)<<8;
      //                      h = ((int)vp[1]+128)<<8;
      dest[0] = h;
      dest[1] = h;
      dest[width] = h;
      dest[width+1] = h;
      p++;
      dest+=2;
      vp+=2;
      }
    dest += e->video_width;
    vp += 2;
    p++;
    }
  h = priv->map_h/2;
  dest2 += e->video_height/2*e->video_width;
  p = priv->map1 + h * priv->map_w;
  for(x=0; x<priv->map_w-1; x++)
    {
    y = p[x]>>(priv->point-2);
    if(y>=h) y = h - 1;
    if(y<=-h) y = -h + 1;
    dest2[y*e->video_width+x*2] = 0xffffff;
    dest2[y*e->video_width+x*2+1] = 0xffffff;
    }
#endif

  return 0;
  }


static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "motion" },
      .multi_names = (char const *[]){ "motion", "rain", NULL },
      .multi_labels = (char const *[]){ TRS("Motion"), TRS("Rain"), NULL },
    },
    {
      .name =      "loopnum",
      .long_name = TRS("Speed"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1   },
      .val_max =     { .val_i = 5 },
      .val_default = { .val_i = 2  },
    },
    {
      .name =      "strength",
      .long_name = TRS("Strength"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1  },
      .val_max =     { .val_i = 10 },
      .val_default = { .val_i = 8  },
    },
    {
      .name =      "decay_cfg",
      .long_name = TRS("Decay"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1  },
      .val_max =     { .val_i = 15 },
      .val_default = { .val_i = 8  },
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
  ripple_t * priv = vp->e->priv;
  
  if(!name)
    return;
  EFFECTV_SET_PARAM_INT(loopnum);
  EFFECTV_SET_PARAM_INT(strength);
  EFFECTV_SET_PARAM_INT(decay_cfg);
  if(!strcmp(name, "mode"))
    {
    if(!strcmp(val->val_str, "rain"))
      priv->mode = 1;
    else
      priv->mode = 0;
    }
  
  priv->decay = 16 - priv->decay_cfg;
  priv->point = 20 - priv->strength;
  }

static void * create_rippletv()
  {
  return bg_effectv_create(rippleRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_rippletv",
      .long_name = TRS("RippleTV"),
      .description = TRS("RippleTV does ripple mark effect on the video input. The ripple is caused by a motion or random rain drops. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_rippletv,
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
