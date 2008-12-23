/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;
  gavl_video_format_t field_format;

  gavl_video_frame_t * frame;

  gavl_video_frame_t * src_field;
  gavl_video_frame_t * dst_field;

  int need_restart;
  
  
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
  vp = (interlace_priv_t *)priv;
  if(vp->frame)
    gavl_video_frame_destroy(vp->frame);
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
  vp = (interlace_priv_t *)priv;
  return vp->need_restart;
  }


static void set_parameter_interlace(void * priv, const char * name,
                               const bg_parameter_value_t * val)
  {
  interlace_priv_t * vp;
  gavl_interlace_mode_t new_interlace;
  vp = (interlace_priv_t *)priv;
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

static void connect_input_port_interlace(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  interlace_priv_t * vp;
  vp = (interlace_priv_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

static void set_input_format_interlace(void * priv,
                                       gavl_video_format_t * format, int port)
  {
  interlace_priv_t * vp;
  vp = (interlace_priv_t *)priv;

  if(!port)
    {
    gavl_video_format_copy(&vp->format, format);
    gavl_video_format_copy(&vp->field_format, format);
    vp->field_format.image_height /= 2;
    vp->format.frame_duration *= 2;
    vp->format.interlace_mode = vp->out_interlace;
    }
  if(vp->frame)
    {
    gavl_video_frame_destroy(vp->frame);
    vp->frame = (gavl_video_frame_t*)0;
    }
  vp->need_restart = 0;
  }

static void
get_output_format_interlace(void * priv, gavl_video_format_t * format)
  {
  interlace_priv_t * vp;
  vp = (interlace_priv_t *)priv;
  
  gavl_video_format_copy(format, &vp->format);
  }

static int read_video_interlace(void * priv,
                                gavl_video_frame_t * frame,
                                int stream)
  {
  interlace_priv_t * vp;
  int field;
  vp = (interlace_priv_t *)priv;

  field = vp->out_interlace == GAVL_INTERLACE_TOP_FIRST ? 0 : 1;

  if(!vp->frame)
    {
    vp->frame = gavl_video_frame_create(&vp->format);
    gavl_video_frame_clear(vp->frame, &vp->format);
    }
  
  if(!vp->read_func(vp->read_data, vp->frame, vp->read_stream))
    return 0;
  
  /* Copy first field */
  gavl_video_frame_get_field(vp->format.pixelformat,
                             vp->frame,
                             vp->src_field, field);
  
  gavl_video_frame_get_field(vp->format.pixelformat,
                             frame,
                             vp->dst_field, field);
  gavl_video_frame_copy(&vp->field_format, vp->dst_field, vp->src_field);

  gavl_video_frame_copy_metadata(frame, vp->frame);

  field = 1 - field;
  
  if(!vp->read_func(vp->read_data, vp->frame, vp->read_stream))
    {
    /* First field is available, but not the second one: make it black */
    gavl_video_frame_clear(vp->frame, &vp->format);
    return 0;
    }
  else
    {
    frame->duration += vp->frame->duration;
    }
  /* Copy second field */
  
  gavl_video_frame_get_field(vp->format.pixelformat,
                             vp->frame,
                             vp->src_field, field);
  
  gavl_video_frame_get_field(vp->format.pixelformat,
                             frame,
                             vp->dst_field, field);
  gavl_video_frame_copy(&vp->field_format, vp->dst_field, vp->src_field);

  
  
  return 1;
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
    
    .connect_input_port = connect_input_port_interlace,
    
    .set_input_format = set_input_format_interlace,
    .get_output_format = get_output_format_interlace,

    .read_video = read_video_interlace,
    .need_restart = need_restart_interlace,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
