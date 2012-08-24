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

#define LOG_DOMAIN "fv_interlace"

typedef struct
  {
  gavl_interlace_mode_t out_interlace;
    
  gavl_video_format_t format;
  gavl_video_format_t field_format[2];
  
  gavl_video_frame_t * frame1;
  
  gavl_video_frame_t * src_field;
  gavl_video_frame_t * dst_field;

  int need_restart;
  
  int num_frames; // 0, 1 or 2

  gavl_video_source_t * in_src;
  } interlace_priv_t;

static void * create_interlace()
  {
  interlace_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->src_field = gavl_video_frame_create(NULL);
  ret->dst_field = gavl_video_frame_create(NULL);
  return ret;
  }

static void destroy_interlace(void * priv)
  {
  interlace_priv_t * vp;
  vp = priv;
  if(vp->frame1)
    gavl_video_frame_destroy(vp->frame1);

  gavl_video_frame_null(vp->src_field);
  gavl_video_frame_null(vp->dst_field);

  gavl_video_frame_destroy(vp->src_field);
  gavl_video_frame_destroy(vp->dst_field);
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "field_order",
      .long_name = TRS("Output field order"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .multi_names  = (char const*[]){ "top", "bottom", NULL },
      .multi_labels = (char const*[]){ TRS("Top first"),
                                       TRS("Bottom first"), NULL },
      .val_default = { .val_str = "top" },
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_interlace(void * priv)
  {
  return parameters;
  }

static int need_restart_interlace(void * priv)
  {
  interlace_priv_t * vp;
  vp = priv;
  return vp->need_restart;
  }


static void set_parameter_interlace(void * priv, const char * name,
                               const bg_parameter_value_t * val)
  {
  interlace_priv_t * vp;
  gavl_interlace_mode_t new_interlace;
  vp = priv;
  if(!name)
    return;

  if(!strcmp(name, "field_order"))
    {
    if(!strcmp(val->val_str, "bottom"))
      new_interlace = GAVL_INTERLACE_BOTTOM_FIRST;
    else
      new_interlace = GAVL_INTERLACE_TOP_FIRST;
    if(new_interlace != vp->out_interlace)
      {
      vp->out_interlace = new_interlace;
      vp->need_restart = 1;
      }
    }
  }


static void copy_field(interlace_priv_t * vp,
                       gavl_video_frame_t * src,
                       gavl_video_frame_t * dst,
                       int field)
  {
  gavl_video_frame_get_field(vp->format.pixelformat,
                             src, vp->src_field, field);
  
  gavl_video_frame_get_field(vp->format.pixelformat,
                             dst, vp->dst_field, field);

  gavl_video_frame_copy(&vp->field_format[field], vp->dst_field, vp->src_field);
  }
                       

static gavl_source_status_t
read_func(void * priv, gavl_video_frame_t ** frame)
  {
  int field;
  gavl_source_status_t st;
  gavl_video_frame_t * frame2 = NULL;
  interlace_priv_t * vp = priv;

  field = vp->out_interlace == GAVL_INTERLACE_TOP_FIRST ? 0 : 1;
  
  if(!vp->frame1)
    {
    vp->frame1 = gavl_video_frame_create(&vp->format);
    gavl_video_frame_clear(vp->frame1, &vp->format);
    }

  if(vp->num_frames == 0)
    {
    if((st = gavl_video_source_read_frame(vp->in_src, &vp->frame1)) !=
       GAVL_SOURCE_OK)
      return st;
    vp->num_frames++;
    }
  if(vp->num_frames == 1)
    {
    if((st = gavl_video_source_read_frame(vp->in_src, &frame2)) !=
       GAVL_SOURCE_OK)
      return st;
    vp->num_frames++;
    }

  /* Copy first field */
  copy_field(vp, vp->frame1, *frame, field);
  
  gavl_video_frame_copy_metadata(*frame, vp->frame1);
  
  field = 1 - field;
  
  /* Copy second field */
  
  copy_field(vp, frame2, *frame, field);
  (*frame)->duration += frame2->duration;
  vp->num_frames = 0;
  return GAVL_SOURCE_OK;
  }


static gavl_video_source_t * connect_interlace(void * priv,
                                               gavl_video_source_t * src,
                                               const gavl_video_options_t * opt)
  {
  const gavl_video_format_t * in_format;

  interlace_priv_t * vp = priv;

  
  vp->in_src = src;
  in_format = gavl_video_source_get_src_format(vp->in_src);
  gavl_video_format_copy(&vp->format, in_format);
  
  vp->format.frame_duration *= 2;
  vp->format.interlace_mode = vp->out_interlace;
  gavl_get_field_format(&vp->format, &vp->field_format[0], 0);
  gavl_get_field_format(&vp->format, &vp->field_format[1], 1);

  if(vp->frame1)
    {
    gavl_video_frame_destroy(vp->frame1);
    vp->frame1 = NULL;
    }
  
  gavl_video_source_set_dst(vp->in_src, 0, in_format);
  vp->need_restart = 0;
  vp->num_frames = 0;
  
  return gavl_video_source_create(read_func,
                                  vp, 0,
                                  &vp->format);
  }

static void reset_interlace(void * priv)
  {
  interlace_priv_t * vp = priv;
  vp->num_frames = 0;
  }


const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_interlace",
      .long_name = TRS("Interlace"),
      .description = TRS("Interlace video images. Output has half the input framerate."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_interlace,
      .destroy =   destroy_interlace,
      .get_parameters =   get_parameters_interlace,
      .set_parameter =    set_parameter_interlace,
      .priority =         1,
    },
    
    .connect = connect_interlace,
    .need_restart = need_restart_interlace,
    .reset = reset_interlace,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
