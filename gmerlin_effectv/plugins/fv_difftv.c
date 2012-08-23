/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * diff.c: color independant differencing.  Just a little test.
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Public License.
 *
 * Controls:
 *      c   -   lower tolerance (threshhold)
 *      v   -   increase tolerance (threshhold)
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#define TOLERANCE_STEP  4

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);
//static void diffUpdate();
//static void diffSave();

typedef struct
  {
  int state;
  RGB32* prevbuf;
  int tolerance;
  } diff_t;


static effect *diffRegister(void)
{
	effect *entry;
        diff_t * priv;
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
  diff_t * priv = e->priv;
  priv->prevbuf = malloc(e->video_area * PIXEL_SIZE);
  if(priv->prevbuf == NULL)
    {
    return -1;
    }
  memset(priv->prevbuf, 0, e->video_area * PIXEL_SIZE);
  priv->state = 1;
  return 0;
  }

static int stop(effect * e)
  {
  diff_t * priv = e->priv;
  priv->state = 0;
  free(priv->prevbuf);
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  int i;
  int x,y;
  unsigned int src_red, src_grn, src_blu;
  unsigned int old_red, old_grn, old_blu;
  unsigned int red_val, red_diff, grn_val, grn_diff, blu_val, blu_diff;
  diff_t * priv = e->priv;
    
  i = 0;
  for (y = 0; y < e->video_height; y++)
    {
    for (x = 0; x < e->video_width; x++)
      {
      // MMX would just eat this algorithm up
      src_red = (src[i] & 0x00FF0000) >> 16;
      src_grn = (src[i] & 0x0000FF00) >> 8;
      src_blu = (src[i] & 0x000000FF);

      old_red = (priv->prevbuf[i] & 0x00FF0000) >> 16;
      old_grn = (priv->prevbuf[i] & 0x0000FF00) >> 8;
      old_blu = (priv->prevbuf[i] & 0x000000FF);

      // RED
      if (src_red > old_red)
        {
        red_val = 0xFF;
        red_diff = src_red - old_red;
        }
      else
        {
        red_val = 0x7F;
        red_diff = old_red - src_red;
        }
      red_val = (red_diff >= priv->tolerance) ? red_val : 0x00;

      // GREEN
      if (src_grn > old_grn)
        {
        grn_val = 0xFF;
        grn_diff = src_grn - old_grn;
        }
      else
        {
        grn_val = 0x7F;
        grn_diff = old_grn - src_grn;
        }
      grn_val = (grn_diff >= priv->tolerance) ? grn_val : 0x00;
            
      // BLUE
      if (src_blu > old_blu)
        {
        blu_val = 0xFF;
        blu_diff = src_blu - old_blu;
        }
      else
        {
        blu_val = 0x7F;
        blu_diff = old_blu - src_blu;
        }
      blu_val = (blu_diff >= priv->tolerance) ? blu_val : 0x00;

      priv->prevbuf[i] = (( ((src_red + old_red) >> 1) << 16) +
                    ( ((src_grn + old_grn) >> 1) << 8) +
                    ( ((src_blu + old_blu) >> 1) ) );

#if 0            
      if ((0x00 != blu_val) && (0x00 != grn_val) && (0x00 != red_val))
        {
        dest[i] = src[i];
        }
      else
        {
        dest[i] = priv->prevbuf[i];
        }
#else            
      dest[i] = (red_val << 16) + (grn_val << 8) + blu_val;
#endif
            
      //x            dest[i] = (red_diff << 17) + (grn_diff << 9) + (blu_diff << 1);

            
      //            priv->prevbuf[i] = src[i];    // Since we're already iterating through both of them...

      i++;
      }
    }

  return 0;
  }


static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "tolerance",
      .long_name = TRS("Tolerance"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_i = 10 },
      .val_min     = { .val_i =  0 },
      .val_max     = { .val_i = 255 },
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
  int changed;
  bg_effectv_plugin_t * vp = data;
  diff_t * priv = vp->e->priv;
  
  if(!name)
    return;
  EFFECTV_SET_PARAM_INT(tolerance);
  }

static void * create_difftv()
  {
  return bg_effectv_create(diffRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_difftv",
      .long_name = TRS("DiffTV"),
      .description = TRS("DiffTV highlights interframe differences. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_difftv,
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
