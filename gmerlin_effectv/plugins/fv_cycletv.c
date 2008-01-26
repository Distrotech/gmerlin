/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * cycleTV - no effect.
 * Written by clifford smith <nullset@dookie.net>
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dst);

static char *effectname = "cycleTV";

typedef struct
  {
  int state;
  int roff,goff,boff; /* Offset values */
  } cycle_t;

static effect *cycleRegister(void)
{
	effect *entry;
        cycle_t * priv;

	entry = (effect *)calloc(1, sizeof(effect));
	if(entry == NULL) return NULL;
        priv = calloc(1, sizeof(*priv));
        entry->priv = priv;
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;

	return entry;
}

static int start(effect* e)
  {
  cycle_t * priv = (cycle_t*)e->priv;
  priv->roff = priv->goff = priv->boff = 0;
  priv->state = 1;
  return 0;
  }

static int stop(effect*e)
  {
  cycle_t * priv = (cycle_t*)e->priv;
  priv->state = 0;
  return 0;
  }

#define NEWCOLOR(c,o) ((c+o)%230)
static int draw(effect* e, RGB32 *src, RGB32 *dst)
  {
  int i;
  cycle_t * priv = (cycle_t*)e->priv;
  
  priv->roff += 1;
  priv->goff += 3;        
  
  priv->boff += 7;
  for (i=0 ; i < e->video_area ; i++) {
    RGB32 t;
    t = *src++;
    *dst++ = RGB(NEWCOLOR(RED(t),priv->roff),
	       NEWCOLOR(GREEN(t),priv->goff),
	       NEWCOLOR(BLUE(t),priv->boff));
  }

  return 0;
}

static void * create_cycletv()
  {
  return bg_effectv_create(cycleRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_cycletv",
      .long_name = TRS("CycleTV"),
      .description = TRS("CycleTV randomly cycles the color palette. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_cycletv,
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
