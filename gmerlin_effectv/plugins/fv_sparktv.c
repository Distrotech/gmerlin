/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * SparkTV - spark effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gmerlin_effectv.h>
#include "utils.h"

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);
// static int event(SDL_Event *event);

static char *effectname = "SparkTV";

#define SPARK_MAX 10
#define POINT_MAX 100

struct shortvec {
	int x1;
	int y1;
	int x2;
	int y2;
};


typedef struct
  {
  int stat;
  int bgIsSet;
  int mode;
  struct shortvec sparks[SPARK_MAX];
  int sparks_life[SPARK_MAX];
  int sparks_head;
  int px[POINT_MAX];
  int py[POINT_MAX];
  int pp[POINT_MAX];
  } spark_t;

#define SPARK_BLUE 0x80
#define SPARK_CYAN 0x6080
#define SPARK_WHITE 0x808080

static int shortvec_length2(struct shortvec sv)
{
	int dx, dy;

	dx = sv.x2 - sv.x1;
	dy = sv.y2 - sv.y1;

	return (dx*dx+dy*dy);
}

static void draw_sparkline_dx
(int x, int y, int dx, int dy, RGB32 *dest, int width, int height)
{
	int t, i, ady;
	RGB32 a, b;
	RGB32 *p;

	p = &dest[y*width+x];
	t = dx;
	ady = abs(dy);
	for(i=0; i<dx; i++) {
		if(y>2 && y<height-2) {
			a = (*(p-width*2) & 0xfffeff) + SPARK_BLUE;
			b = a & 0x100;
			*(p-width*2) = a | (b - (b >> 8));

			a = (*(p-width) & 0xfefeff) + SPARK_CYAN;
			b = a & 0x10100;
			*(p-width) = a | (b - (b >> 8));

			a = (*p & 0xfefeff) + SPARK_WHITE;
			b = a & 0x1010100;
			*p = a | (b - (b >> 8));

			a = (*(p+width) & 0xfefeff) + SPARK_CYAN;
			b = a & 0x10100;
			*(p+width) = a | (b - (b >> 8));

			a = (*(p+width*2) & 0xfffeff) + SPARK_BLUE;
			b = a & 0x100;
			*(p+width*2) = a | (b - (b >> 8));
		}
		p++;
		t -= ady;
		if(t<0) {
			t += dx;
			if(dy<0) {
				y--;
				p -= width;
			} else {
				y++;
				p += width;
			}
		}
	}
}

static void draw_sparkline_dy
(int x, int y, int dx, int dy, RGB32 *dest, int width, int height)
{
	int t, i, adx;
	RGB32 a, b;
	RGB32 *p;

	p = &dest[y*width+x];
	t = dy;
	adx = abs(dx);
	for(i=0; i<dy; i++) {
		if(x>2 && x<width-2) {
			a = (*(p-2) & 0xfffeff) + SPARK_BLUE;
			b = a & 0x100;
			*(p-2) = a | (b - (b >> 8));

			a = (*(p-1) & 0xfefeff) + SPARK_CYAN;
			b = a & 0x10100;
			*(p-1) = a | (b - (b >> 8));

			a = (*p & 0xfefeff) + SPARK_WHITE;
			b = a & 0x1010100;
			*p = a | (b - (b >> 8));

			a = (*(p+1) & 0xfefeff) + SPARK_CYAN;
			b = a & 0x10100;
			*(p+1) = a | (b - (b >> 8));

			a = (*(p+2) & 0xfffeff) + SPARK_BLUE;
			b = a & 0x100;
			*(p+2) = a | (b - (b >> 8));
		}
		p += width;
		t -= adx;
		if(t<0) {
			t += dy;
			if(dx<0) {
				x--;
				p--;
			} else {
				x++;
				p++;
			}
		}
	}
}

static void draw_sparkline
(int x1, int y1, int x2, int y2, RGB32 *dest, int width, int height)
{
	int dx, dy;

	dx = x2 - x1;
	dy = y2 - y1;

	if(abs(dx)>abs(dy)) {
		if(dx<0) {
			draw_sparkline_dx(x2, y2, -dx, -dy, dest, width, height);
		} else {
			draw_sparkline_dx(x1, y1, dx, dy, dest, width, height);
		}
	} else {
		if(dy<0) {
			draw_sparkline_dy(x2, y2, -dx, -dy, dest, width, height);
		} else {
			draw_sparkline_dy(x1, y1, dx, dy, dest, width, height);
		}
	}
}

static void break_line(effect * e, int a, int b, int width, int height)
{
	int dx, dy;
	int x, y;
	int c;
	int len;
        spark_t * priv = (spark_t *)e->priv;
	dx = priv->px[b] - priv->px[a];
	dy = priv->py[b] - priv->py[a];
	if((dx*dx+dy*dy)<100 || (b-a)<3) {
		priv->pp[a] = b;
		return;
	}
	len = (abs(dx)+abs(dy))/4;
	x = priv->px[a] + dx/2 - len/2 + len*(int)((inline_fastrand(e))&255)/256;
	y = priv->py[a] + dy/2 - len/2 + len*(int)((inline_fastrand(e))&255)/256;
	if(x<0) x = 0;
	if(y<0) y = 0;
	if(x>=width) x = width - 1;
	if(y>=height) y = height - 1;
	c = (a+b)/2;
	priv->px[c] = x;
	priv->py[c] = y;
	break_line(e, a, c, width, height);
	break_line(e, c, b, width, height);
}

static void draw_spark(effect * e, struct shortvec sv, RGB32 *dest, int width, int height)
{
	int i;
        spark_t * priv = (spark_t *)e->priv;

	priv->px[0] = sv.x1;
	priv->py[0] = sv.y1;
	priv->px[POINT_MAX-1] = sv.x2;
	priv->py[POINT_MAX-1] = sv.y2;
	break_line(e, 0, POINT_MAX-1, width, height);
	for(i=0; priv->pp[i]>0; i=priv->pp[i]) {
		draw_sparkline(priv->px[i], priv->py[i], priv->px[priv->pp[i]], priv->py[priv->pp[i]], dest, width, height);
	}

}

static struct shortvec scanline_dx(effect * e, int dir, int y1, int y2, unsigned char *diff)
{
	int i, x, y;
	int dy;
	int width = e->video_width;
	struct shortvec sv = {0, 0};
	int start = 0;
        //        spark_t * priv = (spark_t *)e->priv;

	dy = 256 * (y2 - y1) / width;
	y = y1 * 256;
	if(dir == 1) {
		x = 0;
	} else {
		x = width - 1;
	}
	for(i=0; i<width; i++) {
		if(start == 0) {
			if(diff[(y>>8)*width+x]) {
				sv.x1 = x;
				sv.y1 = y>>8;
				start = 1;
			}
		} else {
			if(diff[(y>>8)*width+x] == 0) {
				sv.x2 = x;
				sv.y2 = y>>8;
				start = 2;
				break;
			}
		}
		y += dy;
		x += dir;
	}
	if(start == 0) {
		sv.x1 = sv.x2 = sv.y1 = sv.y2 = 0;
	}
	if(start == 1) {
		sv.x2 = x - dir;
		sv.y2 = (y - dy)>>8;
	}
	return sv;
}

static struct shortvec scanline_dy(effect * e, int dir, int x1, int x2, unsigned char *diff)
{
	int i, x, y;
	int dx;
	int width = e->video_width;
	int height = e->video_height;
	struct shortvec sv = { 0, 0 };
	int start = 0;
        //        spark_t * priv = (spark_t *)e->priv;

	dx = 256 * (x2 - x1) / height;
	x = x1 * 256;
	if(dir == 1) {
		y = 0;
	} else {
		y = height - 1;
	}
	for(i=0; i<height; i++) {
		if(start == 0) {
			if(diff[y*width+(x>>8)]) {
				sv.x1 = x>>8;
				sv.y1 = y;
				start = 1;
			}
		} else {
			if(diff[y*width+(x>>8)] == 0) {
				sv.x2 = x>>8;
				sv.y2 = y;
				start = 2;
				break;
			}
		}
		x += dx;
		y += dir;
	}
	if(start == 0) {
		sv.x1 = sv.x2 = sv.y1 = sv.y2 = 0;
	}
	if(start == 1) {
		sv.x2 = (x - dx)>>8;
		sv.y2 = y - dir;
	}
	return sv;
}

#define MARGINE 20
static struct shortvec detectEdgePoints(effect * e, unsigned char *diff)
{
	int p1, p2;
	int d;
        //        spark_t * priv = (spark_t *)e->priv;

	d = fastrand(e)>>30;
	switch(d) {
	case 0:
		p1 = fastrand(e)%(e->video_width - MARGINE*2);
		p2 = fastrand(e)%(e->video_width - MARGINE*2);
		return scanline_dy(e, 1, p1, p2, diff);
	case 1:
		p1 = fastrand(e)%(e->video_width - MARGINE*2);
		p2 = fastrand(e)%(e->video_width - MARGINE*2);
		return scanline_dy(e, -1, p1, p2, diff);
	case 2:
		p1 = fastrand(e)%(e->video_height - MARGINE*2);
		p2 = fastrand(e)%(e->video_height - MARGINE*2);
		return scanline_dx(e, 1, p1, p2, diff);
	default:
	case 3:
		p1 = fastrand(e)%(e->video_height - MARGINE*2);
		p2 = fastrand(e)%(e->video_height - MARGINE*2);
		return scanline_dx(e, -1, p1, p2, diff);
	}
}

static int setBackground(effect * e, RGB32 *src)
{
        spark_t * priv = (spark_t *)e->priv;
	image_bgset_y(e, src);
	priv->bgIsSet = 1;

	return 0;
}

static effect *sparkRegister(void)
{
	effect *entry;
        spark_t * priv;
        
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
	int i;
        spark_t * priv = (spark_t *)e->priv;
	
	for(i=0; i<POINT_MAX; i++) {
		priv->pp[i] = 0;
	}
	for(i=0; i<SPARK_MAX; i++) {
        priv->sparks_life[i] = 0;
	}
	priv->sparks_head = 0;
        image_init(e);
        image_set_threshold_y(e, 40);
	priv->bgIsSet = 0;

	priv->stat = 1;
	return 0;
}

static int stop(effect * e)
{
        spark_t * priv = (spark_t *)e->priv;
	priv->stat = 0;
	return 0;
}

static int draw(effect * e, RGB32 *src, RGB32 *dest)
{
	int i;
	unsigned char *diff;
	struct shortvec sv;
        spark_t * priv = (spark_t *)e->priv;

	if(!priv->bgIsSet) {
		setBackground(e, src);
	}

	switch(priv->mode) {
		default:
		case 0:
			diff = image_diff_filter(e, image_bgsubtract_y(e, src));
			break;
		case 1:
			diff = image_diff_filter(e, image_y_over(e, src));
			break;
		case 2:
			diff = image_diff_filter(e, image_y_under(e, src));
			break;
	}

	memcpy(dest, src, e->video_area * sizeof(RGB32));

	sv = detectEdgePoints(e, diff);
	if((inline_fastrand(e)&0x10000000) == 0) {
		if(shortvec_length2(sv)>400) {
			priv->sparks[priv->sparks_head] = sv;
			priv->sparks_life[priv->sparks_head] = (inline_fastrand(e)>>29) + 2;
			priv->sparks_head = (priv->sparks_head+1) % SPARK_MAX;
		}
	}
	for(i=0; i<SPARK_MAX; i++) {
		if(priv->sparks_life[i]) {
			draw_spark(e, priv->sparks[i], dest, e->video_width, e->video_height);
			priv->sparks_life[i]--;
		}
	}

	return 0;
}

#if 0

static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
			bgIsSet = 0;
			break;
		case SDLK_1:
		case SDLK_KP1:
			mode = 0;
			break;
		case SDLK_2:
		case SDLK_KP2:
			mode = 1;
			break;
		case SDLK_3:
		case SDLK_KP3:
			mode = 2;
			break;
		default:
			break;
		}
	}
	return 0;
}
#endif

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "all" },
      .multi_names = (char const *[]){ "fg",
                                       "light",
                                       "dark",
                                       (char*)0 },
      
      .multi_labels = (char const *[]){ TRS("Foreground"),
                                        TRS("Light parts"),
                                        TRS("Dark parts"),
                                        (char*)0 },
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
  spark_t * priv = (spark_t*)vp->e->priv;
  
  if(!name)
    return;
  if(!strcmp(name, "mode"))
    {
    if(!strcmp(val->val_str, "fg"))
      priv->mode = 0;
    else if(!strcmp(val->val_str, "light"))
      priv->mode = 1;
    else if(!strcmp(val->val_str, "dark"))
      priv->mode = 2;
    }
  }

static void * create_sparktv()
  {
  return bg_effectv_create(sparkRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_sparktv",
      .long_name = TRS("SparkTV"),
      .description = TRS("Bright sparks run on incoming objects. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_sparktv,
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
