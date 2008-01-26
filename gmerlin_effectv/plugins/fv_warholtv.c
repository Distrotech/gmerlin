/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * WarholTV - Hommage aux Andy Warhol
 * Copyright (C) 2002 Jun IIO
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include <utils.h>

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);

static char *effectname = "WarholTV";

typedef struct
  {
  int state;
  } warholtv_t;

static const RGB32 colortable[26] = {
	0x000080, 0x008000, 0x800000,
	0x00e000, 0x808000, 0x800080, 
	0x808080, 0x008080, 0xe0e000, 
};

static effect *warholRegister(void)
{
	effect *entry;
        warholtv_t * priv;
          
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

static int start(effect * e)
  {
  warholtv_t * priv = (warholtv_t*)e->priv;
  
  priv->state = 1;
  return 0;
  }

static int stop(effect * e)
  {
  warholtv_t * priv = (warholtv_t*)e->priv;
  priv->state = 0;
  return 0;
  }

#define DIVIDER		3
static int draw(effect * e, RGB32 *src, RGB32 *dst)
{
	int p, q, x, y, i;

	for (y = 0; y < e->video_height; y++)
	  for (x = 0; x < e->video_width; x++)
	    {
	      p = (x * DIVIDER) % e->video_width;
	      q = (y * DIVIDER) % e->video_height;
	      i = ((y * DIVIDER) / e->video_height) * DIVIDER 
		+ ((x * DIVIDER) / e->video_width);
	      *dst++ = src[q * e->video_width + p] ^ colortable[i];
	    }
        
	return 0;
}

static void * create_warholtv()
  {
  return bg_effectv_create(warholRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_warholtv",
      .long_name = TRS("WarholTV"),
      .description = TRS("WarholTV offers some effects like Andy Warhol's series of paintings; 'Marilyn', 'Marilyn Three Times', 'Four Marilyns' etc. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_warholtv,
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
