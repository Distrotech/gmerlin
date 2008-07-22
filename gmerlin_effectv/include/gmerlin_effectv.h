/*****************************************************************
 * gmerlin_effectv - EffecTV plugins for gmerlin
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <config.h>
#include "EffecTV.h"
#include <gavl/gavl.h>
#include <gavl/gavldsp.h>
#include <gmerlin/plugin.h>
#include <gmerlin/translation.h>

#define BG_EFFECTV_REUSE_OUTPUT   (1<<0)
#define BG_EFFECTV_COLOR_AGNOSTIC (1<<1)

typedef struct
  {
  effect * e;
  gavl_video_frame_t * in_frame;  /* Not padded */
  gavl_video_frame_t * out_frame; /* Not padded */
  gavl_video_format_t format;

  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;

  int started;

  int flags;

#ifdef WORDS_BIGENDIAN
  gavl_dsp_context_t * dsp_ctx;
#endif
  } bg_effectv_plugin_t;

void bg_effectv_destroy(void * priv);

void bg_effectv_connect_input_port(void * priv,
                                   bg_read_video_func_t func,
                                   void * data, int stream, int port);

void bg_effectv_set_input_format(void * priv, gavl_video_format_t * format, int port);

void * bg_effectv_create(effectRegisterFunc * f, int flags);

void bg_effectv_destroy(void*priv);

void bg_effectv_get_output_format(void * priv, gavl_video_format_t * format);

int bg_effectv_read_video(void * priv, gavl_video_frame_t * frame, int stream);

#define EFFECTV_SET_PARAM_INT(n) \
  if(!strcmp(#n, name)) \
    { \
    if(priv->n != val->val_i) \
      { \
      priv->n = val->val_i; \
      changed = 1; \
      } \
    }
     
#define EFFECTV_SET_PARAM_FLOAT(n) \
  if(!strcmp(#n, name)) \
    { \
    if(priv->n != val->val_f) \
      { \
      priv->n = val->val_f; \
      changed = 1; \
      } \
    }

#define EFFECTV_SET_PARAM_COLOR(n) \
  if(!strcmp(#n, name)) \
    { \
      int r, g, b; \
      r = (int)(val->val_color[0]*255.0+0.5); \
      if(r & ~0xFF) r = (-r) >> 31; \
      g = (int)(val->val_color[1]*255.0+0.5); \
      if(g & ~0xFF) g = (-g) >> 31; \
      b = (int)(val->val_color[2]*255.0+0.5); \
      if(b & ~0xFF) b = (-b) >> 31; \
      priv->n = ((0<<24) + (r<<16) + (g <<8) + (b)); \
    }

