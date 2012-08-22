/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "fv_tlp"

#include <gavl/gavldsp.h>

typedef struct tlp_priv_s
  {
  float factor;
  
  gavl_video_format_t format;
  
  gavl_video_frame_t * frames[2];

  gavl_video_frame_t * this_frame;
  gavl_video_frame_t * last_frame;
  
  gavl_dsp_context_t * ctx;
  int init;

  gavl_video_source_t * in_src;
  gavl_video_source_t * out_src;

  } tlp_priv_t;

static void * create_tlp()
  {
  tlp_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->ctx = gavl_dsp_context_create();
  return ret;
  }

static void destroy_tlp(void * priv)
  {
  tlp_priv_t * vp;
  vp = priv;
  if(vp->frames[0])
    gavl_video_frame_destroy(vp->frames[0]);
  if(vp->frames[1])
    gavl_video_frame_destroy(vp->frames[1]);
  if(vp->ctx)
    gavl_dsp_context_destroy(vp->ctx);

  if(vp->out_src)
    gavl_video_source_destroy(vp->out_src);

  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "factor",
      .long_name = TRS("Strength"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .num_digits = 2,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 1.0 },
      .val_default = { .val_f = 0.5 },
      .help_string = TRS("0 means no effect, 1 means maximum (= still image)"),
    },
    {
      .name = "quality",
      .long_name = TRS("Quality"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = GAVL_QUALITY_FASTEST },
      .val_max =     { .val_i = GAVL_QUALITY_BEST },
      .val_default = { .val_i = GAVL_QUALITY_DEFAULT },
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_tlp(void * priv)
  {
  return parameters;
  }

static void set_parameter_tlp(void * priv, const char * name, const bg_parameter_value_t * val)
  {
  tlp_priv_t * vp;
  vp = priv;

  if(!name)
    return;
  if(!strcmp(name, "factor"))
    vp->factor = val->val_f;
  else if(!strcmp(name, "quality"))
    gavl_dsp_context_set_quality(vp->ctx, val->val_i);
  }

static void reset_tlp(void * priv)
  {
  tlp_priv_t * vp;
  vp = priv;
  vp->init = 1;
  }

static gavl_source_status_t
read_func(void * priv, gavl_video_frame_t ** frame)
  {
  gavl_source_status_t st;
  
  tlp_priv_t * vp;
  vp = priv;
  
  if(!vp->frames[0])
    vp->frames[0] = gavl_video_frame_create(&vp->format);
  if(!vp->frames[1])
    vp->frames[1] = gavl_video_frame_create(&vp->format);
  
  if(vp->init)
    {
    vp->this_frame = vp->frames[0];
    vp->last_frame = vp->frames[1];

    if((st = gavl_video_source_read_frame(vp->in_src, &vp->last_frame)) !=
       GAVL_SOURCE_OK)
      return st;
    *frame = vp->last_frame;
    vp->init = 0;
    }
  else
    {
    gavl_video_frame_t * in_frame = NULL;
    gavl_video_frame_t * tmp;
    
    if((st = gavl_video_source_read_frame(vp->in_src, &in_frame)) !=
       GAVL_SOURCE_OK)
      return st;
    
    gavl_dsp_interpolate_video_frame(vp->ctx, &vp->format, in_frame, vp->last_frame,
                                     vp->this_frame, vp->factor);
    *frame = vp->this_frame;
    gavl_video_frame_copy_metadata(*frame, in_frame);

    /* Swap */
    tmp = vp->this_frame;
    vp->this_frame = vp->last_frame;
    vp->last_frame = tmp;
    }
  
  return GAVL_SOURCE_OK;
  }

static gavl_video_source_t *
connect_tlp(void * priv, gavl_video_source_t * src,
                 const gavl_video_options_t * opt)
  {
  tlp_priv_t * vp = priv;
  
  if(vp->out_src)
    {
    gavl_video_source_destroy(vp->out_src);
    vp->out_src = NULL;
    }
  vp->in_src = src;

  /* */
  gavl_video_format_copy(&vp->format,
                         gavl_video_source_get_src_format(vp->in_src));
  
  if(vp->frames[0])
    {
    gavl_video_frame_destroy(vp->frames[0]);
    vp->frames[0] = NULL;
    }
  if(vp->frames[1])
    {
    gavl_video_frame_destroy(vp->frames[1]);
    vp->frames[1] = NULL;
    }
  vp->init = 1;
  
  if(opt)
    gavl_video_options_copy(gavl_video_source_get_options(vp->in_src), opt);
  
  gavl_video_source_set_dst(vp->in_src, 0, &vp->format);
  vp->out_src =
    gavl_video_source_create(read_func, vp,
                             GAVL_SOURCE_SRC_ALLOC |
                             GAVL_SOURCE_SRC_REF, &vp->format);
  return vp->out_src;
  }


const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_tlp",
      .long_name = TRS("Temporal lowpass"),
      .description = TRS("Simple temporal lowpass"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_tlp,
      .destroy =   destroy_tlp,
      .get_parameters =   get_parameters_tlp,
      .set_parameter =    set_parameter_tlp,
      .priority =         1,
    },
    
    .connect = connect_tlp,
    .reset = reset_tlp,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
