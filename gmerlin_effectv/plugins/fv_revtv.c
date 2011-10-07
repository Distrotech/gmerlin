/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * revTV based on Rutt-Etra Video Synthesizer 1974?
 *
 * (c)2002 Ed Tannenbaum
 *
 * This effect acts like a waveform monitor on each line.
 * It was originally done by deflecting the electron beam on a monitor using
 * additional electromagnets on the yoke of a b/w CRT. Here it is emulated digitally.

 * Experimental tapes were made with this system by Bill and Louise Etra and Woody and Steina Vasulka

 * The line spacing can be controlled using the 1 and 2 Keys.
 * The gain is controlled using the 3 and 4 keys.
 * The update rate is controlled using the 0 and - keys.
 */

#include <stdlib.h>
#include <string.h>
#include "gmerlin_effectv.h"
#include "utils.h"

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dest);


typedef struct
  {
  RGB32 fg_color;
  RGB32 bg_color;
  int vgrabtime;
  int vgrab;
  
  float spacing;
  float amplitude;
  
  int state;
  
  } rev_t;


static void vasulka(effect * e,
                    RGB32 *src, RGB32 *dst, int srcx, int srcy, int dstx, int dsty, int w, int h);

static effect *revRegister(void)
{
	effect *entry;
        rev_t * priv;

        entry = calloc(1, sizeof(effect));
	if(entry == NULL) return NULL;

        priv = calloc(1, sizeof(*priv));
        entry->priv = priv;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
        
	return entry;
}

static int start(effect * e)
  {
  rev_t * priv = e->priv;
  priv->state = 1;
  return 0;
  }

static int stop(effect * e)
  {
  rev_t * priv = e->priv;
  priv->state = 0;
  return 0;
  }


static int draw(effect * e, RGB32 *src, RGB32 *dst)
  {
  int i;
  RGB32 * ptr;
  rev_t * priv = e->priv;
  priv->vgrab++;
  if (priv->vgrab>=priv->vgrabtime)
    {
    priv->vgrab=0;
    ptr = dst;
    for( i = 0; i < e->video_area; i++)
      {
      *(ptr++) = priv->bg_color;
      }
    //    memset(dst, 0, e->video_area*PIXEL_SIZE); // clear the screen
    vasulka(e, src, dst, 0, 0, 0, 0, e->video_width, e->video_height);
    }
  
  return 0;
  }

static void vasulka(effect * e,
                    RGB32 *src, RGB32 *dst,
                    int srcx, int srcy, int dstx, int dsty, int w, int h)
  {
  rev_t * priv = e->priv;
  RGB32 *cdst=dst+((dsty*e->video_width)+dstx);
  RGB32 *nsrc;
  int y,x,R,G,B,yval;
  int offset;
  int linespace = (int)(priv->spacing * e->video_height + 1.0);
  int vscale = (int)(priv->amplitude * e->video_height + 0.5);
  // draw the offset lines
  for (y=srcy; y<h+srcy; y+=linespace)
    {
    for(x=srcx; x<=w+srcx; x++)
      {
      nsrc=src+(y*e->video_width)+x;
      // Calc Y Value for curpix
      R = ((*nsrc)&0xff0000)>>(16-1);
      G = ((*nsrc)&0xff00)>>(8-2);
      B = (*nsrc)&0xff;
      yval = y-(int)(((R + G + B) * vscale) / (255 + 255*2 + 255*4)) ;
      offset = x + yval * e->video_width;
      if(offset >= 0 && offset < e->video_area)
        {
        cdst[offset]=priv->fg_color;
        }
      }
    }
  }


static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "fg_color",
      .long_name = TRS("Line color"),
      .type = BG_PARAMETER_COLOR_RGB,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_color = { 1.0, 1.0, 1.0, 1.0 } },
    },
    {
      .name =      "bg_color",
      .long_name = TRS("Background"),
      .type = BG_PARAMETER_COLOR_RGB,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_color = { 0.0, 0.0, 0.0, 1.0 } },
    },
    {
      .name =      "spacing",
      .long_name = TRS("Spacing"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0  },
      .val_max =     { .val_f = 1.0 },
      .val_default = { .val_f = 0.1  },
      .num_digits = 3,
    },
    {
      .name =      "amplitude",
      .long_name = TRS("Amplitude"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 1.0 },
      .val_default = { .val_f = 0.1 },
      .num_digits = 2,
    },
    {
      .name =      "vgrabtime",
      .long_name = TRS("Grab interval"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 10 },
      .val_default = { .val_i = 1 },
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
  rev_t * priv = vp->e->priv;
  
  if(!name)
    return;
  EFFECTV_SET_PARAM_FLOAT(spacing);
  EFFECTV_SET_PARAM_FLOAT(amplitude);
  EFFECTV_SET_PARAM_COLOR(fg_color);
  EFFECTV_SET_PARAM_INT(vgrabtime);
  EFFECTV_SET_PARAM_COLOR(bg_color);
  }

static void * create_revtv()
  {
  return bg_effectv_create(revRegister, BG_EFFECTV_REUSE_OUTPUT);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_revtv",
      .long_name = TRS("RevTV"),
      .description = TRS("RevTV acts like a video waveform monitor for each line of video processed. This creates a pseudo 3D effect based on the brightness of the video along each line. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_revtv,
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
