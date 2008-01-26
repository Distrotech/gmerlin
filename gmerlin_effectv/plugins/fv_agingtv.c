/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * AgingTV - film-aging effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dest);


typedef struct _scratch
{
	int life;
	int x;
	int dx;
	int init;
} scratch;

#define SCRATCH_MAX 20

typedef struct
  {
  int state;
  int area_scale;
  int aging_mode;

  scratch scratches[SCRATCH_MAX];
  int scratch_lines;
  int dust_interval;
  int pits_interval;
  
  } agingtv_t;

static void coloraging(effect * e, RGB32 *src, RGB32 *dest)
{
	static int c = 0x18;
	RGB32 a, b;
	int i;

	c -= (int)(inline_fastrand(e))>>28;
	if(c < 0) c = 0;
	if(c > 0x18) c = 0x18;
	for(i=0; i<e->video_area; i++) {
		a = *src++;
		b = (a & 0xfcfcfc)>>2;
		*dest++ = a - b + (c|(c<<8)|(c<<16)) + ((inline_fastrand(e)>>8)&0x101010);
	}
}



static void scratching(effect * e, RGB32 *dest)
{
	int i, y, y1, y2;
	RGB32 *p, a, b;
	const int width = e->video_width;
	const int height = e->video_height;
        agingtv_t * priv = (agingtv_t*)e->priv;
	for(i=0; i<priv->scratch_lines; i++) {
		if(priv->scratches[i].life) {
			priv->scratches[i].x = priv->scratches[i].x + priv->scratches[i].dx;
			if(priv->scratches[i].x < 0 || priv->scratches[i].x > width*256) {
				priv->scratches[i].life = 0;
				break;
			}
			p = dest + (priv->scratches[i].x>>8);
			if(priv->scratches[i].init) {
				y1 = priv->scratches[i].init;
				priv->scratches[i].init = 0;
			} else {
				y1 = 0;
			}
			priv->scratches[i].life--;
			if(priv->scratches[i].life) {
				y2 = height;
			} else {
				y2 = fastrand(e) % height;
			}
			for(y=y1; y<y2; y++) {
				a = *p & 0xfefeff;
				a += 0x202020;
				b = a & 0x1010100;
				*p = a | (b - (b>>8));
				p += width;
			}
		} else {
			if((fastrand(e)&0xf0000000) == 0) {
				priv->scratches[i].life = 2 + (fastrand(e)>>27);
				priv->scratches[i].x = fastrand(e) % (width * 256);
				priv->scratches[i].dx = ((int)fastrand(e))>>23;
				priv->scratches[i].init = (fastrand(e) % (height-1))+1;
			}
		}
	}
}

static int dx[8] = { 1, 1, 0, -1, -1, -1,  0, 1};
static int dy[8] = { 0, -1, -1, -1, 0, 1, 1, 1};

static void dusts(effect * e, RGB32 *dest)
{
	int i, j;
	int dnum;
	int d, len;
	int x, y;
	const int width = e->video_width;
	const int height = e->video_height;
        agingtv_t * priv = (agingtv_t*)e->priv;

	if(priv->dust_interval == 0) {
		if((fastrand(e)&0xf0000000) == 0) {
			priv->dust_interval = fastrand(e)>>29;
		}
		return;
	}

	dnum = priv->area_scale*4 + (fastrand(e)>>27);
	for(i=0; i<dnum; i++) {
		x = fastrand(e)%width;
		y = fastrand(e)%height;
		d = fastrand(e)>>29;
		len = fastrand(e)%priv->area_scale + 5;
		for(j=0; j<len; j++) {
			dest[y*width + x] = 0x101010;
			y += dy[d];
			x += dx[d];
			if(x<0 || x>=width) break;
			if(y<0 || y>=height) break;
			d = (d + fastrand(e)%3 - 1) & 7;
		}
	}
	priv->dust_interval--;
}


static void pits(effect * e, RGB32 *dest)
{
	int i, j;
	int pnum, size, pnumscale;
	int x, y;
	const int width = e->video_width;
	const int height = e->video_height;
        agingtv_t * priv = (agingtv_t*)e->priv;

	pnumscale = priv->area_scale * 2;
	if(priv->pits_interval) {
		pnum = pnumscale + (fastrand(e)%pnumscale);
		priv->pits_interval--;
	} else {
		pnum = fastrand(e)%pnumscale;
		if((fastrand(e)&0xf8000000) == 0) {
			priv->pits_interval = (fastrand(e)>>28) + 20;
		}
	}
	for(i=0; i<pnum; i++) {
		x = fastrand(e)%(width-1);
		y = fastrand(e)%(height-1);
		size = fastrand(e)>>28;
		for(j=0; j<size; j++) {
			x = x + fastrand(e)%3-1;
			y = y + fastrand(e)%3-1;
			if(x<0 || x>=width) break;
			if(y<0 || y>=height) break;
			dest[y*width + x] = 0xc0c0c0;
		}
	}
}

static effect *agingRegister(void)
{
	effect *entry;
        agingtv_t * priv;
	entry = (effect *)calloc(1, sizeof(effect));
	if(entry == NULL) return NULL;
	
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
        //	entry->event = NULL;
        priv = calloc(1, sizeof(*priv));
        entry->priv = priv;
	return entry;
}

static void aging_mode_switch(effect * e)
{
        agingtv_t * priv = (agingtv_t*)e->priv;
	switch(priv->aging_mode) {
		default:
		case 0:
			priv->scratch_lines = 7;
	/* Most of the parameters are tuned for 640x480 mode */
	/* area_scale is set to 10 when screen size is 640x480. */
			priv->area_scale = e->video_width * e->video_height / 64 / 480;
	}
	if(priv->area_scale <= 0)
		priv->area_scale = 1;
}

static int start(effect * e)
{
        agingtv_t * priv = (agingtv_t*)e->priv;
	priv->aging_mode = 0;
	aging_mode_switch(e);

	priv->state = 1;
	return 0;
}

static int stop(effect * e)
{
        agingtv_t * priv = (agingtv_t*)e->priv;
	priv->state = 0;

	return 0;
}

static int draw(effect * e, RGB32 *src, RGB32 *dest)
{
        agingtv_t * priv = (agingtv_t*)e->priv;
	coloraging(e, src, dest);

	scratching(e, dest);
	pits(e, dest);
	if(priv->area_scale > 1)
		dusts(e, dest);

	return 0;
}

static void * create_agingtv()
  {
  return bg_effectv_create(agingRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_agingtv",
      .long_name = TRS("AgingTV"),
      .description = TRS("AgingTV ages video input stream in realtime. Discolors, scratches, puts dust. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_agingtv,
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
