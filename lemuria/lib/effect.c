/*    
 *   Lemuria, an OpenGL music visualization
 *   Copyright (C) 2002 - 2007 Burkhard Plaum
 *
 *   Lemuria is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <GL/gl.h>

#include <inttypes.h>
#include <stdio.h>

#include <lemuria_private.h>
#include <effect.h>
#include <utils.h>

#define MIN_TEXTURE_FRAMES     100
#define MIN_FOREGROUND_FRAMES 2000
//#define MIN_FOREGROUND_FRAMES 500
#define MIN_BACKGROUND_FRAMES 2000

#define FLAG_NEXT             (1<<0)

// #define TEST_MODE

/* Texture effects */

extern effect_plugin_t oscilloscope_effect;
extern effect_plugin_t blur_effect;
extern effect_plugin_t vectorscope_effect;

/* Foreground effects */

extern effect_plugin_t cube_effect;
extern effect_plugin_t tube_effect;
extern effect_plugin_t bubbles_effect;
extern effect_plugin_t icosphere_effect;
extern effect_plugin_t squares_effect;
extern effect_plugin_t fountain_effect;

extern effect_plugin_t oszi3d_effect;
extern effect_plugin_t superellipse_effect;
extern effect_plugin_t hyperbubble_effect;

extern effect_plugin_t swarm_effect;

extern effect_plugin_t tentacle_effect;

/* Background effects */

extern effect_plugin_t psycho_effect;
extern effect_plugin_t boioioing_effect;
extern effect_plugin_t starfield_effect;
extern effect_plugin_t lines_effect;
extern effect_plugin_t monolith_effect;
extern effect_plugin_t archaic_effect;
extern effect_plugin_t tunnel_effect;
extern effect_plugin_t mountains_effect;
extern effect_plugin_t drive_effect;

extern effect_plugin_t deepsea_effect;

extern effect_plugin_t xaos_effect;
extern effect_plugin_t lineobjects_effect;


// extern effect_plugin_t v4l_effect;


//extern effect_plugin_t test_effect;

typedef struct
  {
  const char * name;
  const char * label;
  effect_plugin_t * effect;
  } effect_info;

static effect_plugin_t * foreground_effects[] = 
  {
    &bubbles_effect,
    &cube_effect,
    &fountain_effect,
//    &tentacle_effect,
    &oszi3d_effect,
    &swarm_effect,
    &hyperbubble_effect,
    &tube_effect,
    &icosphere_effect,
    &squares_effect,
    &superellipse_effect,
  };

static effect_plugin_t * background_effects[] = 
  {
    &mountains_effect,
    &drive_effect,
//     &xaos_effect,
    &deepsea_effect,
    &lineobjects_effect,
    //    &v4l_effect,
    &boioioing_effect,
    &psycho_effect,
    &starfield_effect,

    &monolith_effect,

#if 1
//    &drive_effect,
    &tunnel_effect,
    &lines_effect,
    &archaic_effect,
#endif
  };

static effect_plugin_t * texture_effects[] = 
  {
    &oscilloscope_effect,
    &blur_effect,
    &vectorscope_effect
  };

static int num_foreground_effects =
sizeof(foreground_effects)/sizeof(foreground_effects[0]);

static int num_background_effects =
sizeof(background_effects)/sizeof(background_effects[0]);

static int num_texture_effects =
sizeof(texture_effects)/sizeof(texture_effects[0]);

static void manage_effect(lemuria_engine_t * e,
                          lemuria_effect_t * effect,
                          effect_plugin_t ** plugins,
                          int num_plugins,
                          int min_frames,
                          float probability)
  {
  int new_index;
  
  /* Load effect if there wasn't one */
  //  fprintf(stderr, 
  
  if(!effect->effect)
    {
#ifndef TEST_MODE
    effect->index = lemuria_random_int(e, 0, num_plugins-1);
#else
    effect->index = 0;
#endif
    effect->effect = plugins[effect->index];
    effect->mode = EFFECT_RUNNING;
    effect->flags = 0;
    effect->frame_counter = 0;
    if(effect->effect->init)
      effect->data = effect->effect->init(e);
    else
      fprintf(stderr, "FATAL: Plugin has no init function\n");
    return;
    }
  
  /* Kick out obsolete effects */

  else if((effect->mode == EFFECT_DONE) || (effect->mode == EFFECT_FINISH))
    {
    effect->effect->cleanup(effect->data);
    if(effect->flags & FLAG_NEXT)
      {
      effect->index++;
      if(effect->index >= num_plugins)
        effect->index = 0;
      }
    else
      {
      new_index = lemuria_random_int(e, 0, num_plugins-2);
      if(new_index >= effect->index)
        new_index++;
      effect->index = new_index;
      }
    effect->effect = plugins[effect->index];
    effect->mode = EFFECT_RUNNING;
    effect->flags = 0;
    if(effect->frame_counter >= 0)
      effect->frame_counter = 0;
    effect->data = effect->effect->init(e);
    }

  /* Check wether we should change the effects */

  else if((effect->mode == EFFECT_RUNNING) &&
          e->beat_detected &&
          (effect->frame_counter > min_frames) &&
          (lemuria_decide(e, probability)))
    {
    effect->mode = EFFECT_FINISH;
    }
  else if(effect->frame_counter >= 0)
    effect->frame_counter++;
  }

void lemuria_manage_effects(lemuria_engine_t * e)
  {
  manage_effect(e, &(e->background),
                background_effects,
                num_background_effects,
                MIN_BACKGROUND_FRAMES,
                0.02);
  
  manage_effect(e, &(e->foreground),
                foreground_effects,
                num_foreground_effects,
                MIN_FOREGROUND_FRAMES,
                0.02);
  
  manage_effect(e, &(e->texture),
                texture_effects,
                num_texture_effects,
                MIN_TEXTURE_FRAMES,
                0.4);
  }

void lemuria_set_background(lemuria_engine_t * e)
  {
  if(e->background.mode == EFFECT_RUNNING)
    e->background.mode = EFFECT_FINISH;
  }

void lemuria_next_background(lemuria_engine_t * e)
  {
  if(e->background.mode == EFFECT_RUNNING)
    {
    e->background.mode = EFFECT_FINISH;
    e->background.flags |= FLAG_NEXT;
    }
  }

void lemuria_set_foreground(lemuria_engine_t * e)
  {
  if(e->foreground.mode == EFFECT_RUNNING)
    e->foreground.mode = EFFECT_FINISH;
  }

void lemuria_next_foreground(lemuria_engine_t * e)
  {
  if(e->foreground.mode == EFFECT_RUNNING)
    {
    e->foreground.mode = EFFECT_FINISH;
    e->foreground.flags |= FLAG_NEXT;
    }
  }

void lemuria_set_texture(lemuria_engine_t * e)
  {
  if(e->texture.mode == EFFECT_RUNNING)
    e->texture.mode = EFFECT_FINISH;
  }


void lemuria_next_texture(lemuria_engine_t * e)
  {
  if(e->texture.mode == EFFECT_RUNNING)
    {
    e->texture.mode = EFFECT_FINISH;
    e->texture.flags |= FLAG_NEXT;
    }
  }

