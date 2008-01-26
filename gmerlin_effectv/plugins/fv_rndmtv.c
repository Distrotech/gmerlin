/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * rndmTV Random noise based on video signal
 * (c)2002 Ed Tannenbaum <et@et-arts.com>
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gmerlin_effectv.h>
#include "utils.h"

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dest);
//static int event(SDL_Event *event);

static const int rgrabtime=1;
static const int rthecolor=0xffffffff;

typedef struct
  {
  int state;
  int rgrab;
  int rmode; // =1;
  } rndm_t;

static char *effectname = "RndmTV";

static effect *rndmRegister(void)
  {
  effect *entry;
  rndm_t * priv;
        
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
  rndm_t * priv = (rndm_t*)e->priv;
  priv->state = 1;
  return 0;
  }

static int stop(effect * e)
  {
  rndm_t * priv = (rndm_t*)e->priv;
  priv->state = 0;
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dst)
  {
  int i, tmp, rtmp;
  rndm_t * priv = (rndm_t*)e->priv;
  
  priv->rgrab++;
  if (priv->rgrab>=rgrabtime)
    {
    priv->rgrab=0;

    //bzero(dst, video_area*sizeof(RGB32)); // clear the screen
    if (priv->rmode==0)
      {
      //static

      for (i=0; i<e->video_height*e->video_width; i++)
        {
        if((inline_fastrand(e)>>24)<((*src)&0xff00)>>8)
          {

          *dst=rthecolor;
          }
        else{
        *dst=0x00;
        }
        src++ ; dst++;
        }
      } else {
      for (i=0; i<e->video_height*e->video_width; i++)
        {
        /*
          tmp=0;
          if((inline_fastrand()>>24)<((*src)&0xff00)>>8){
          tmp=0xff00;
          }
          if((inline_fastrand()>>24)<((*src)&0xff0000)>>16){
          tmp=tmp |0xff0000;
          }
          if((inline_fastrand()>>24)<((*src)&0xff)){
          tmp=tmp |0xff;
          }
          *dst=tmp;
          */
        tmp=0;
        rtmp=inline_fastrand(e)>>24;
        if (rtmp < ((*src)&0xff00)>>8)
          {
          tmp=0xff00;
          }
        if(rtmp < ((*src)&0xff0000)>>16)
          {
          tmp=tmp |0xff0000;
          }
        if(rtmp < ((*src)&0xff))
          {
          tmp=tmp |0xff;
          }
        *dst=tmp;
        src++ ; dst++;
        }
      }
    }

  return 0;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "rmode",
      .long_name = TRS("Color"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_SYNC,
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
  bg_effectv_plugin_t * vp = (bg_effectv_plugin_t *)data;
  rndm_t * priv = (rndm_t*)vp->e->priv;
  
  if(!name)
    return;

  EFFECTV_SET_PARAM_INT(rmode);

  }


#if 0

static int event(SDL_Event *event)
{

	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {



				case SDLK_0:
					rgrabtime++;
                        if (rgrabtime==0)rgrabtime =1;

                        fprintf(stdout,"rgrabtime=%d\n",rgrabtime);
                        break;


				case SDLK_MINUS:
                        rgrabtime--;
                        if (rgrabtime==0)rgrabtime =1;

                        fprintf(stdout,"rgrabtime=%d\n",rgrabtime);
                        break;
				
				case SDLK_SPACE:
					rmode++;
                        if (rmode==2)rmode =0;

                        fprintf(stdout,"rmode=%d\n",rmode);
                        break;



		default:
			break;
		}
	}

	return 0;
}
#endif

static void * create_rndmtv()
  {
  return bg_effectv_create(rndmRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_rndmtv",
      .long_name = TRS("RndmTV"),
      .description = TRS("RndmTV give you a noisy picture in color or B/W. Inspired by the old days when reception was poor. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_rndmtv,
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
