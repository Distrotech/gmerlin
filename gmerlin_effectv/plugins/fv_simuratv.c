/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * SimuraTV - color distortion and mirrored image effector
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include <utils.h>

static int start(effect*);
static int stop(effect*);
static int draw(effect*,RGB32 *src, RGB32 *dest);
// static int event(SDL_Event *event);


typedef struct
  {
  int stat;
  RGB32 color;
  int mirror;
  int hwidth;
  int hheight;
  } simuratv_t;

#if 0
static RGB32 colortable[26] = {
	0x000080, 0x0000e0, 0x0000ff,
	0x008000, 0x00e000, 0x00ff00,
	0x008080, 0x00e0e0, 0x00ffff,
	0x800000, 0xe00000, 0xff0000,
	0x800080, 0xe000e0, 0xff00ff,
	0x808000, 0xe0e000, 0xffff00,
	0x808080, 0xe0e0e0, 0xffffff,
	0x76ca0a, 0x3cafaa, 0x60a848, 0x504858, 0x89ba43
};
static const char keytable[26] = {
	'q', 'a', 'z',
	'w', 's', 'x',
	'e', 'd', 'c',
	'r', 'f', 'v',
	't', 'g', 'b',
	'y', 'h', 'n',
	'u', 'j', 'm',
	'i', 'k', 'o', 'l', 'p'
};
#endif
static void mirror_no(effect * e, RGB32 *, RGB32 *);
static void mirror_u(effect * e, RGB32 *, RGB32 *);
static void mirror_d(effect * e, RGB32 *, RGB32 *);
static void mirror_r(effect * e, RGB32 *, RGB32 *);
static void mirror_l(effect * e, RGB32 *, RGB32 *);
static void mirror_ul(effect * e, RGB32 *, RGB32 *);
static void mirror_ur(effect * e, RGB32 *, RGB32 *);
static void mirror_dl(effect * e, RGB32 *, RGB32 *);
static void mirror_dr(effect * e, RGB32 *, RGB32 *);

static effect *simuraRegister(void)
{
	effect *entry;
        simuratv_t * priv;
#if 0
        int i;
	RGB32 tmp[26];
	
	for(i=0; i<26; i++) {
		tmp[keytable[i] - 'a'] = colortable[i];
	}
	for(i=0; i<26; i++) {
		colortable[i] = tmp[i];
	}
#endif

	entry = (effect *)calloc(1, sizeof(effect));
	if(entry == NULL) {
		return NULL;
	}
        priv = calloc(1, sizeof(*priv));
        entry->priv = priv;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
        //	entry->event = event;

	return entry;
}

static int start(effect * e)
  {
  simuratv_t * priv = (simuratv_t *)e->priv;
  
  priv->hwidth = e->video_width/2;
  priv->hheight = e->video_height/2;
  
  priv->stat = 1;
  return 0;
  }

static int stop(effect * e)
  {
  simuratv_t * priv = (simuratv_t *)e->priv;
  priv->stat = 0;
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  simuratv_t * priv = (simuratv_t *)e->priv;
  switch(priv->mirror)
    {
    case 1:
      mirror_l(e, src, dest);
      break;
    case 2:
      mirror_r(e, src, dest);
      break;
    case 3:
      mirror_d(e, src, dest);
      break;
    case 4:
      mirror_dl(e, src, dest);
      break;
    case 5:
      mirror_dr(e, src, dest);
      break;
    case 6:
      mirror_u(e, src, dest);
      break;
    case 7:
      mirror_ul(e, src, dest);
      break;
    case 8:
      mirror_ur(e, src, dest);
      break;
    case 0:
    default:
      mirror_no(e, src, dest);
      break;
  }

  return 0;
  }

#if 0
static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_a:
		case SDLK_b:
		case SDLK_c:
		case SDLK_d:
		case SDLK_e:
		case SDLK_f:
		case SDLK_g:
		case SDLK_h:
		case SDLK_i:
		case SDLK_j:
		case SDLK_k:
		case SDLK_l:
		case SDLK_m:
		case SDLK_n:
		case SDLK_o:
		case SDLK_p:
		case SDLK_q:
		case SDLK_r:
		case SDLK_s:
		case SDLK_t:
		case SDLK_u:
		case SDLK_v:
		case SDLK_w:
		case SDLK_x:
		case SDLK_y:
		case SDLK_z:
			color = colortable[event->key.keysym.sym - SDLK_a];
			break;
		case SDLK_1:
		case SDLK_2:
		case SDLK_3:
		case SDLK_4:
		case SDLK_5:
		case SDLK_6:
		case SDLK_7:
		case SDLK_8:
		case SDLK_9:
			mirror = event->key.keysym.sym - SDLK_1;
			break;
		case SDLK_KP1:
		case SDLK_KP2:
		case SDLK_KP3:
		case SDLK_KP4:
		case SDLK_KP5:
		case SDLK_KP6:
		case SDLK_KP7:
		case SDLK_KP8:
		case SDLK_KP9:
			mirror = event->key.keysym.sym - SDLK_KP1;
			break;
		case SDLK_SPACE:
			color = 0;
			break;
		default:
			break;
		}
	}
	return 0;
}
#endif

static void mirror_no(effect * e, RGB32 *src, RGB32 *dest)
  {
  int i;
  simuratv_t * priv = (simuratv_t *)e->priv;

  for(i=0; i<e->video_area; i++)
    {
    dest[i] = src[i] ^ priv->color;
    }
  }

static void mirror_u(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  simuratv_t * priv = (simuratv_t *)e->priv;

  for(y=0; y<priv->hheight; y++) {
  for(x=0; x<e->video_width; x++) {
  dest[y*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[(e->video_height-y-1)*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  }
  }
  }

static void mirror_d(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  simuratv_t * priv = (simuratv_t *)e->priv;

  for(y=priv->hheight; y<e->video_height; y++) {
  for(x=0; x<e->video_width; x++) {
  dest[y*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[(e->video_height-y-1)*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  }
  }
  }

static void mirror_l(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  simuratv_t * priv = (simuratv_t *)e->priv;

  for(y=0; y<e->video_height; y++) {
  for(x=0; x<priv->hwidth; x++) {
  dest[y*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[y*e->video_width+(e->video_width-x-1)] = src[y*e->video_width+x] ^ priv->color;
  }
  }
  }

static void mirror_r(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  simuratv_t * priv = (simuratv_t *)e->priv;

  for(y=0; y<e->video_height; y++) {
  for(x=priv->hwidth; x<e->video_width; x++) {
  dest[y*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[y*e->video_width+(e->video_width-x-1)] = src[y*e->video_width+x] ^ priv->color;
  }
  }
  }

static void mirror_ul(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  simuratv_t * priv = (simuratv_t *)e->priv;

  for(y=0; y<priv->hheight; y++) {
  for(x=0; x<priv->hwidth; x++) {
  dest[y*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[y*e->video_width+(e->video_width-x-1)] = src[y*e->video_width+x] ^ priv->color;
  dest[(e->video_height-y-1)*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[(e->video_height-y-1)*e->video_width+(e->video_width-x-1)] = src[y*e->video_width+x] ^ priv->color;
  }
  }
  }

static void mirror_ur(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  simuratv_t * priv = (simuratv_t *)e->priv;

  for(y=0; y<priv->hheight; y++) {
  for(x=priv->hwidth; x<e->video_width; x++) {
  dest[y*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[y*e->video_width+(e->video_width-x-1)] = src[y*e->video_width+x] ^ priv->color;
  dest[(e->video_height-y-1)*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[(e->video_height-y-1)*e->video_width+(e->video_width-x-1)] = src[y*e->video_width+x] ^ priv->color;
  }
  }
  }

static void mirror_dl(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  simuratv_t * priv = (simuratv_t *)e->priv;

  for(y=priv->hheight; y<e->video_height; y++) {
  for(x=0; x<priv->hwidth; x++) {
  dest[y*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[y*e->video_width+(e->video_width-x-1)] = src[y*e->video_width+x] ^ priv->color;
  dest[(e->video_height-y-1)*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[(e->video_height-y-1)*e->video_width+(e->video_width-x-1)] = src[y*e->video_width+x] ^ priv->color;
  }
  }
  }

static void mirror_dr(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  simuratv_t * priv = (simuratv_t *)e->priv;

  for(y=priv->hheight; y<e->video_height; y++) {
  for(x=priv->hwidth; x<e->video_width; x++) {
  dest[y*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[y*e->video_width+(e->video_width-x-1)] = src[y*e->video_width+x] ^ priv->color;
  dest[(e->video_height-y-1)*e->video_width+x] = src[y*e->video_width+x] ^ priv->color;
  dest[(e->video_height-y-1)*e->video_width+(e->video_width-x-1)] = src[y*e->video_width+x] ^ priv->color;
  }
  }
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "color",
      .long_name = TRS("Color"),
      .type = BG_PARAMETER_COLOR_RGB,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_color = { 0.0, 0.0, 0.0, 0.0 } },
    },
    {
      .name =      "mirror_mode",
      .long_name = TRS("Mirror mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "no" },
      .multi_names = (char const *[]){ "no", "l", "r", "d", "dl", "dr", "u", "ul", "ur", (char*)0 },
      .multi_labels = (char const *[]){ TRS("None"), TRS("Left"), TRS("Right"), TRS("Down"), TRS("Down left"), TRS("Down right"), TRS("Upper"), TRS("Upper left"), TRS("Upper right"), (char*)0 },
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
  bg_effectv_plugin_t * vp = (bg_effectv_plugin_t *)data;
  simuratv_t * priv = (simuratv_t*)vp->e->priv;

  if(!name)
    return;
  EFFECTV_SET_PARAM_COLOR(color);

  if(!strcmp(name, "mirror_mode"))
    {
    if(!strcmp(val->val_str, "no"))
      priv->mirror = 0;
    if(!strcmp(val->val_str, "l"))
      priv->mirror = 1;
    if(!strcmp(val->val_str, "r"))
      priv->mirror = 2;
    if(!strcmp(val->val_str, "d"))
      priv->mirror = 3;
    if(!strcmp(val->val_str, "dl"))
      priv->mirror = 4;
    if(!strcmp(val->val_str, "dr"))
      priv->mirror = 5;
    if(!strcmp(val->val_str, "u"))
      priv->mirror = 6;
    if(!strcmp(val->val_str, "ul"))
      priv->mirror = 7;
    if(!strcmp(val->val_str, "ur"))
      priv->mirror = 8;
    }
  
  }
static void * create_simuratv()
  {
  return bg_effectv_create(simuraRegister, 0);
  }



const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_simuratv",
      .long_name = TRS("SimuraTV"),
      .description = TRS("The origin of SimuraTV is \"SimuraEffect\", a VJ (Video Jockey) tool I made in 1995. Color effect and image mirroring are all of SimuraTV. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_simuratv,
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
