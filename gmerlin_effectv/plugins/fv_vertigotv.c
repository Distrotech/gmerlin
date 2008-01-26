/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * VertigoTV - Alpha blending with zoomed and rotated images.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gmerlin_effectv.h>
#include <utils.h>

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);

// static int event(SDL_Event *event);

static char *effectname = "VertigoTV";

typedef struct
  {
  int state;
  RGB32 *buffer;
  RGB32 *current_buffer, *alt_buffer;
  int dx, dy;
  int sx, sy;
  double phase;
  double phase_increment;
  double zoomrate;
  
  } vertigotv_t;


static void setParams(effect * e)
{
	double vx, vy;
	double t;
	double x, y;
	double dizz;
        vertigotv_t * priv = (vertigotv_t *)e->priv;
	dizz = sin(priv->phase) * 10 + sin(priv->phase*1.9+5) * 5;

	x = e->video_width / 2;
	y = e->video_height / 2;
	t = (x*x + y*y) * priv->zoomrate;
	if(e->video_width > e->video_height) {
		if(dizz >= 0) {
			if(dizz > x) dizz = x;
			vx = (x*(x-dizz) + y*y) / t;
		} else {
			if(dizz < -x) dizz = -x;
			vx = (x*(x+dizz) + y*y) / t;
		}
		vy = (dizz*y) / t;
	} else {
		if(dizz >= 0) {
			if(dizz > y) dizz = y;
			vx = (x*x + y*(y-dizz)) / t;
		} else {
			if(dizz < -y) dizz = -y;
			vx = (x*x + y*(y+dizz)) / t;
		}
		vy = (dizz*x) / t;
	}
	priv->dx = vx * 65536;
	priv->dy = vy * 65536;
	priv->sx = (-vx * x + vy * y + x + cos(priv->phase*5) * 2) * 65536;
	priv->sy = (-vx * y - vy * x + y + sin(priv->phase*6) * 2) * 65536;

	priv->phase += priv->phase_increment;
	if(priv->phase > 5700000) priv->phase = 0;
}

static effect *dizzyRegister(void)
{
	effect *entry;
        vertigotv_t * priv;
	
	entry = (effect *)calloc(1, sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
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
  vertigotv_t * priv = (vertigotv_t *)e->priv;
        
  priv->buffer = (RGB32 *)malloc(e->video_area*2*PIXEL_SIZE);
  memset(priv->buffer, 0, e->video_area * 2 * PIXEL_SIZE);
  
  if(priv->buffer == NULL)
    {
    return -1;
    }
  
  priv->current_buffer = priv->buffer;
  priv->alt_buffer = priv->buffer + e->video_area;
  priv->phase = 0;

  priv->state = 1;
  return 0;
  }

static int stop(effect * e)
{
        vertigotv_t * priv = (vertigotv_t *)e->priv;
	priv->state = 0;
        if(priv->buffer)
          {
          free(priv->buffer);
          priv->buffer = NULL;
          }
        return 0;
}

static int draw(effect * e, RGB32 *src, RGB32 *dest)
{
        vertigotv_t * priv = (vertigotv_t *)e->priv;
	RGB32 *p;
	RGB32 v;
	int x, y;
	int ox, oy;
	int i;

	setParams(e);
	p = priv->alt_buffer;
	for(y=e->video_height; y>0; y--) {
		ox = priv->sx;
		oy = priv->sy;
		for(x=e->video_width; x>0; x--) {
			i = (oy>>16)*e->video_width + (ox>>16);
			if(i<0) i = 0;
			if(i>=e->video_area) i = e->video_area;
			v = priv->current_buffer[i] & 0xfcfcff;
			v = (v * 3) + ((*src++) & 0xfcfcff);
			*p++ = (v>>2);
			ox += priv->dx;
			oy += priv->dy;
		}
		priv->sx -= priv->dy;
		priv->sy += priv->dx;
	}

	memcpy(dest, priv->alt_buffer, e->video_area*sizeof(RGB32));

	p = priv->current_buffer;
	priv->current_buffer = priv->alt_buffer;
	priv->alt_buffer = p;

	return 0;
}

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "phase_increment",
      .long_name = TRS("Phase increment"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.01 },
      .val_max =     { .val_f = 1.00 },
      .val_default = { .val_f = 0.02 },
      .num_digits = 2,
    },
    {
      .name =      "zoomrate",
      .long_name = TRS("Zoom rate"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 1.01 },
      .val_max =     { .val_f = 1.1 },
      .val_default = { .val_f = 1.01 },
      .num_digits = 2,
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
  bg_effectv_plugin_t * vp = (bg_effectv_plugin_t *)data;
  vertigotv_t * priv = (vertigotv_t*)vp->e->priv;
  int changed = 0;
  if(!name)
    return;
  EFFECTV_SET_PARAM_FLOAT(zoomrate);
  EFFECTV_SET_PARAM_FLOAT(phase_increment);
  }

static void * create_vertigotv()
  {
  return bg_effectv_create(dizzyRegister, 0);
  }



const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_vertigotv",
      .long_name = TRS("VertigoTV"),
      .description = TRS("VertigoTV is a loopback alpha blending effector with rotating and scaling. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_vertigotv,
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


#if 0
static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			phase = 0.0;
			phase_increment = 0.02;
			zoomrate = 1.01;
			break;

		case SDLK_INSERT:
			phase_increment += 0.01;
			break;

		case SDLK_DELETE:
			phase_increment -= 0.01;
			if(phase_increment < 0.01) {
				phase_increment = 0.01;
			}
			break;

		case SDLK_PAGEUP:
			zoomrate += 0.01;
			if(zoomrate > 1.1) {
				zoomrate = 1.1;
			}
			break;

		case SDLK_PAGEDOWN:
			zoomrate -= 0.01;
			if(zoomrate < 1.01) {
				zoomrate = 1.01;
			}
			break;

		default:
			break;
		}
	}

	return 0;
}
#endif
