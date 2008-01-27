/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * dice.c: a 'dicing' effect
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Public License.
 *
 * I suppose this looks similar to PuzzleTV, but it's not. The screen is
 * divided into small squares, each of which is rotated either 0, 90, 180 or
 * 270 degrees.  The amount of rotation for each square is chosen at random.
 *
 * Controls:
 *      c   -   shrink the size of the squares, down to 1x1.
 *      v   -   enlarge the size of the squares, up to 32x32.
 *      space - generate a new random rotation map.
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

#define DEFAULT_CUBE_BITS   4
#define MAX_CUBE_BITS       5
#define MIN_CUBE_BITS       0


typedef enum _dice_dir {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3
} DiceDir;

static int start(effect *);
static int stop(effect *);
static int draw(effect *, RGB32 *src, RGB32 *dest);
static void diceCreateMap(effect *);


typedef struct
  {
  int state;

  char* dicemap;

  int g_cube_bits;
  int g_cube_size;
  int g_map_height;
  int g_map_width;
  } dice_t;


static effect *diceRegister(void)
  {
  effect *entry;
  dice_t * priv;
  priv = calloc(1, sizeof(*priv));

  entry = (effect *)calloc(1, sizeof(effect));
  if(entry == NULL)
    {
    return NULL;
    }

  entry->priv = priv;
  entry->start = start;
  entry->stop = stop;
  entry->draw = draw;
  
  return entry;
  }

static int start(effect * e)
  {
  dice_t * priv = (dice_t*)(e->priv);
  priv->dicemap = (char*)malloc(e->video_height * e->video_width * sizeof(char));
  if(priv->dicemap == NULL)
    {
    return -1;
    }
  
  diceCreateMap(e);
    
  
  priv->state = 1;
  return 0;
  }

static int stop(effect * e)
  {
  dice_t * priv = (dice_t*)(e->priv);
  
  priv->state = 0;
  free(priv->dicemap);
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  int i;
  int map_x, map_y, map_i;
  int base;
  int dx, dy, di;
  dice_t * priv = (dice_t*)(e->priv);
  
  map_i = 0;
  for(map_y = 0; map_y < priv->g_map_height; map_y++)
    {
    for(map_x = 0; map_x < priv->g_map_width; map_x++)
      {
      base = (map_y << priv->g_cube_bits) * e->video_width + (map_x << priv->g_cube_bits);
      switch (priv->dicemap[map_i])
        {
        case Up:
          // fprintf(stderr, "U");
          for (dy = 0; dy < priv->g_cube_size; dy++)
            {
            i = base + dy * e->video_width;
            for (dx = 0; dx < priv->g_cube_size; dx++)
              {
              dest[i] = src[i];
              i++;
              }
            }
          break;
        case Left:
          // fprintf(stderr, "L");
          for (dy = 0; dy < priv->g_cube_size; dy++)
            {
            i = base + dy * e->video_width;
            for (dx = 0; dx < priv->g_cube_size; dx++)
              {
              di = base + (dx * e->video_width) + (priv->g_cube_size - dy - 1);
              dest[di] = src[i];
              i++;
              }
            }
          break;
        case Down:
          // fprintf(stderr, "D");
          for (dy = 0; dy < priv->g_cube_size; dy++)
            {
            di = base + dy * e->video_width;
            i = base + (priv->g_cube_size - dy - 1) * e->video_width + priv->g_cube_size;
            for (dx = 0; dx < priv->g_cube_size; dx++)
              {
              i--;
              dest[di] = src[i];
              di++;
              }
            }
          break;
        case Right:
          // fprintf(stderr, "R");
          for (dy = 0; dy < priv->g_cube_size; dy++)
            {
            i = base + (dy * e->video_width);
            for (dx = 0; dx < priv->g_cube_size; dx++)
              {
              di = base + dy + (priv->g_cube_size - dx - 1) * e->video_width;
              dest[di] = src[i];
              i++;
              }
            }
          break;
        default:
          // fprintf(stderr, "E");
          // This should never occur
          break;
        }
      map_i++;
      }
    // fprintf(stderr,"\n");
    }

  return 0;
  }


static void diceCreateMap(effect * e)
  {
  int x;
  int y;
  int i;
  dice_t * priv = (dice_t*)(e->priv);
  
  priv->g_map_height = e->video_height >> priv->g_cube_bits;
  priv->g_map_width = e->video_width >> priv->g_cube_bits;
  priv->g_cube_size = 1 << priv->g_cube_bits;
  
  i = 0;
  for (y=0; y<priv->g_map_height; y++)
    {
    for(x=0; x<priv->g_map_width; x++)
      {
      // dicemap[i] = ((i + y) & 0x3); /* Up, Down, Left or Right */
      priv->dicemap[i] = (inline_fastrand(e) >> 24) & 0x03;
      i++;
      }
    }
  
  return;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "g_cube_bits",
      .long_name = TRS("Square Size"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_i =  DEFAULT_CUBE_BITS },
      .val_min = { .val_i =  MIN_CUBE_BITS },
      .val_max = { .val_i =  MAX_CUBE_BITS },
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
  bg_effectv_plugin_t * vp = (bg_effectv_plugin_t *)data;
  dice_t * priv = (dice_t*)vp->e->priv;
  
  if(!name)
    return;

  EFFECTV_SET_PARAM_INT(g_cube_bits);

  if(changed)
    diceCreateMap(vp->e);

  }

static void * create_dicetv()
  {
  return bg_effectv_create(diceRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_dicetv",
      .long_name = TRS("DiceTV"),
      .description = TRS("DiceTV 'dices' the screen up into many small squares, each defaulting to a size of 16 pixels by 16 pixels.. Each square is rotated randomly in one of four directions: up (no change), down (180 degrees, or upside down), right (90 degrees clockwise), or left (90 degrees counterclockwise). The direction of each square normally remains consistent between each frame. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_dicetv,
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
