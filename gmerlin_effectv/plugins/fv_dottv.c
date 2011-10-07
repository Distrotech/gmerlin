/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * DotTV: convert gray scale image into a set of dots
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include <utils.h>
#include "heart.inc"

static int start(effect *);
static int stop(effect *);
static int draw(effect *, RGB32 *src, RGB32 *dest);

#define DOTDEPTH 5
#define DOTMAX (1<<DOTDEPTH)


typedef struct
  {
  int state;
  RGB32 *pattern;
  RGB32 *heart_pattern;
  int dots_width;
  int dots_height;
  int dot_size;
  int dot_hsize;
  int *sampx, *sampy;
  int mode;
  } dottv_t;

inline static unsigned char inline_RGBtoY(effect * e, int rgb)
{
	int i;

	i = e->rgb2yuv->RtoY[(rgb>>16)&0xff];
	i += e->rgb2yuv->GtoY[(rgb>>8)&0xff];
	i += e->rgb2yuv->BtoY[rgb&0xff];
	return i;
}

static void init_sampxy_table(effect * e)
{
	int i, j;
        dottv_t * priv = e->priv;
	j = priv->dot_hsize;
	for(i=0; i<priv->dots_width; i++) {
		priv->sampx[i] = j;
		j += priv->dot_size;
	}
	j = priv->dot_hsize;
	for(i=0; i<priv->dots_height; i++) {
        priv->sampy[i] = j;
		j += priv->dot_size;
	}
}

static void makePattern(effect * e)
{
	int i, x, y, c;
	int u, v;
	double p, q, r;
	RGB32 *pat;
        dottv_t * priv = e->priv;

	for(i=0; i<DOTMAX; i++) {
/* Generated pattern is a quadrant of a disk. */
		pat = priv->pattern + (i+1) * priv->dot_hsize * priv->dot_hsize - 1;
		r = (0.2 * i / DOTMAX + 0.8) * priv->dot_hsize;
		r = r*r;
		for(y=0; y<priv->dot_hsize; y++) {
			for(x=0; x<priv->dot_hsize; x++) {
				c = 0;
				for(u=0; u<4; u++) {
					p = (double)u/4.0 + y;
					p = p*p;
					for(v=0; v<4; v++) {
						q = (double)v/4.0 + x;
						if(p+q*q<r) {
							c++;
						}
					}
				}
				c = (c>15)?15:c;
				*pat-- = c<<20 | c<<12 | c<<4;
/* The upper left part of a disk is needed, but generated pattern is a bottom
 * right part. So I spin the pattern. */
			}
		}
	}
}

static void makeOneHeart(effect * e, int val, unsigned char *bigheart)
{
	int x, y;
	int xx, yy;
	int f1x, f1y;
	int f2x, f2y;
	double s1x, s1y;
	double s2x, s2y;
	double d1x, d1y;
	double d2x, d2y;
	double sum, hsum;
	double w, h;
	RGB32 *pat;
	RGB32 c;
        dottv_t * priv = e->priv;
#define SFACT 4

	pat = priv->heart_pattern + val * priv->dot_size * priv->dot_hsize;
	s2y = (double)(-priv->dot_hsize) / priv->dot_size * (31.9 + (double)(DOTMAX-val)/SFACT)
		+ 31.9;
	f2y = (int)s2y;
	for(y=0; y<priv->dot_size; y++) {
		s1y = s2y;
		f1y = f2y;

		s2y = (double)(y+1-priv->dot_hsize) / priv->dot_size
			* (31.9 + (double)(DOTMAX-val)/SFACT) + 31.9;
		f2y = (int)s2y;
		d1y = 1.0 - (s1y - (double)f1y);
		d2y = s2y - (double)f2y;
		h = s2y - s1y;

		s2x = (double)(-priv->dot_hsize) / priv->dot_size
			* (31.9 + (double)(DOTMAX-val)/SFACT) + 31.9;
		f2x = (int)s2x;
		for(x=0; x<priv->dot_hsize; x++) {
			s1x = s2x;
			f1x = f2x;
			s2x = (double)(x+1-priv->dot_hsize) / priv->dot_size
				* (31.9 + (double)(DOTMAX-val)/SFACT) + 31.9;
			f2x = (int)s2x;
			d1x = 1.0 - (s1x - (double)f1x);
			d2x = s2x - (double)f2x;
			w = s2x - s1x;

			sum = 0.0;
			for(yy = f1y; yy <= f2y; yy++) {
				hsum = d1x * bigheart[yy*32+f1x];
				for(xx = f1x+1; xx < f2x; xx++) {
					hsum += bigheart[yy*32+xx];
				}
				hsum += d2x * bigheart[yy*32+f2x];

				if(yy == f1y) {
					sum += hsum * d1y;
				} else if(yy == f2y) {
					sum += hsum * d2y;
				} else {
					sum += hsum;
				}
			}
			c = (RGB32)(sum / w / h);
			if(c<0) c = 0;
			if(c>255) c = 255;
			*pat++ = c<<16;
		}
	}
}

static void makeHeartPattern(effect * e)
{
	int i, x, y;
	unsigned char *bigheart;

	bigheart = malloc(sizeof(unsigned char) * 64 * 32);
	memset(bigheart, 0, 64 * 32 * sizeof(unsigned char));
	for(y=0; y<32; y++) {
		for(x=0; x<16;x++) {
			bigheart[(y+16)*32+x+16] = half_heart[y*16+x];
		}
	}

	for(i=0; i<DOTMAX; i++) {
		makeOneHeart(e, i, bigheart);
	}

	free(bigheart);
}

static effect *dotRegister()
{
	effect *entry;
        dottv_t * priv;
        
	entry = calloc(1, sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
        entry->rgb2yuv = rgb2yuv_init();
        priv = calloc(1, sizeof(*priv));
        entry->priv = priv;
        
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;


	return entry;
}

static int start(effect * e)
  {
  
  double scale = 1.0;
  dottv_t * priv = e->priv;

#if 0  
  if(screen_scale > 0)
    {
    scale = screen_scale;
    }
  else
    {
    scale = (double)screen_width / video_width;
    if(scale > (double)screen_height / video_height)
      {
      scale = (double)screen_height / video_height;
      }
    }
#endif
  priv->dot_size = 8 * scale;
  priv->dot_size = priv->dot_size & 0xfe;
  priv->dot_hsize = priv->dot_size / 2;
  priv->dots_width = e->video_width / priv->dot_size;
  priv->dots_height = e->video_height / priv->dot_size;
	
  priv->pattern = malloc(DOTMAX * priv->dot_hsize * priv->dot_hsize * sizeof(RGB32));
  if(priv->pattern == NULL) {
  return -1;
  }
  priv->heart_pattern = malloc(DOTMAX * priv->dot_hsize * priv->dot_size * PIXEL_SIZE);
  if(priv->heart_pattern == NULL) {
  free(priv->pattern);
  return -1;
  }
  
  priv->sampx = malloc(e->video_width*sizeof(int));
  priv->sampy = malloc(e->video_height*sizeof(int));
  if(priv->sampx == NULL || priv->sampy == NULL)
    {
    return -1;
    }
  
  init_sampxy_table(e);

  makePattern(e);
  makeHeartPattern(e);
    
  priv->state = 1;
  return 0;
  }

static int stop(effect * e)
{
  dottv_t * priv = e->priv;
  priv->state = 0;

  if(priv->pattern) free(priv->pattern);
  if(priv->heart_pattern) free(priv->heart_pattern);
  if(priv->sampx) free(priv->sampx);
  if(priv->sampy) free(priv->sampy);
  
  return 0;
}

static void drawDot(effect * e, int xx, int yy, unsigned char c, RGB32 *dest)
{
  dottv_t * priv = e->priv;
	int x, y;
	RGB32 *pat;

	c = (c>>(8-DOTDEPTH));
	pat = priv->pattern + c * priv->dot_hsize * priv->dot_hsize;
	dest = dest + yy * priv->dot_size * e->video_width + xx * priv->dot_size;
	for(y=0; y<priv->dot_hsize; y++) {
		for(x=0; x<priv->dot_hsize; x++) {
			*dest++ = *pat++;
		}
		pat -= 2;
		for(x=0; x<priv->dot_hsize-1; x++) {
			*dest++ = *pat--;
		}
		dest += e->video_width - priv->dot_size + 1;
		pat += priv->dot_hsize + 1;
	}
	pat -= priv->dot_hsize*2;
	for(y=0; y<priv->dot_hsize-1; y++) {
		for(x=0; x<priv->dot_hsize; x++) {
			*dest++ = *pat++;
		}
		pat -= 2;
		for(x=0; x<priv->dot_hsize-1; x++) {
			*dest++ = *pat--;
		}
		dest += e->video_width - priv->dot_size + 1;
		pat += -priv->dot_hsize + 1;
	}
}

static void drawHeart(effect * e, int xx, int yy, unsigned char c, RGB32 *dest)
{
  dottv_t * priv = e->priv;
	int x, y;
	RGB32 *pat;

	c = (c>>(8-DOTDEPTH));
	pat = priv->heart_pattern + c * priv->dot_size * priv->dot_hsize;
	dest = dest + yy * priv->dot_size * e->video_width + xx * priv->dot_size;
	for(y=0; y<priv->dot_size; y++) {
		for(x=0; x<priv->dot_hsize; x++) {
			*dest++ = *pat++;
		}
		pat--;
		for(x=0; x<priv->dot_hsize; x++) {
			*dest++ = *pat--;
		}
		dest += e->video_width - priv->dot_size;
		pat += priv->dot_hsize + 1;
	}
}

static int draw(effect * e, RGB32 *src, RGB32 *dest)
{
  dottv_t * priv = e->priv;
	int x, y;
	int sx, sy;

        //	dest = (RGB32 *)screen_getaddress(); // cheater! cheater!

	if(priv->mode) {
		for(y=0; y<priv->dots_height; y++) {
			sy = priv->sampy[y];
			for(x=0; x<priv->dots_width; x++) {
				sx = priv->sampx[x];
				drawHeart(e, x, y, inline_RGBtoY(e, src[sy*e->video_width+sx]), dest);
			}
		}
	} else {
		for(y=0; y<priv->dots_height; y++) {
			sy = priv->sampy[y];
			for(x=0; x<priv->dots_width; x++) {
				sx = priv->sampx[x];
				drawDot(e, x, y, inline_RGBtoY(e, src[sy*e->video_width+sx]), dest);
			}
		}
	}

	return 1; // undocumented feature ;-)
}

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "dots" },
      .multi_names = (char const *[]){ "dots", "hearts", NULL },
      .multi_labels = (char const *[]){ TRS("Dots"), TRS("hearts"), NULL },
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters(void * data)
  {
  return parameters;
  }

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t *val)
  {
  bg_effectv_plugin_t * vp = data;
  dottv_t * priv = vp->e->priv;

  if(!name)
    return;

  if(!strcmp(name, "mode"))
    {
    if(!strcmp(val->val_str, "dots"))
      priv->mode = 0;
    else
      priv->mode = 1;
    }
  }


static void * create_dottv()
  {
  return bg_effectv_create(dotRegister, 0);
  }



const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_dottv",
      .long_name = TRS("DotTV"),
      .description = TRS("DotTV converts gray scale images to set of dots. It is hard to recognize what is shown when your eyes are close to the monitor. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_dottv,
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
