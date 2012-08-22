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
#include <gmerlin/bggavl.h>
#include <bgfreetype.h>

#include <gmerlin/textrenderer.h>

#define LOG_DOMAIN "fv_tcdisplay"


typedef struct
  {
  gavl_video_format_t format;
  gavl_video_format_t ovl_format;
  
  gavl_overlay_t ovl;
  
  bg_text_renderer_t * renderer;
  gavl_overlay_blend_context_t * blender;
  
  int interpolate;
  gavl_timecode_t last_timecode;

  gavl_video_source_t * in_src;
  gavl_video_source_t * out_src;

  } tc_priv_t;

static void * create_tcdisplay()
  {
  tc_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->renderer = bg_text_renderer_create();
  ret->blender = gavl_overlay_blend_context_create();
  return ret;
  }

static void destroy_tcdisplay(void * priv)
  {
  tc_priv_t * vp;
  vp = priv;
  bg_text_renderer_destroy(vp->renderer);
  gavl_overlay_blend_context_destroy(vp->blender);

  if(vp->out_src)
    gavl_video_source_destroy(vp->out_src);

  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name =       "general",
      .long_name =  TRS("General"),
      .type =       BG_PARAMETER_SECTION,
    },
    {
      .name = "interpolate",
      .long_name = TRS("Interpolate missing"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Interpolate missing timecodes"),
    },
    {
      .name =       "render_options",
      .long_name =  TRS("Render options"),
      .type =       BG_PARAMETER_SECTION,
    },
    {
      .name =       "color",
      .long_name =  TRS("Text color"),
      .type =       BG_PARAMETER_COLOR_RGBA,
      .val_default = { .val_color = { 1.0, 1.0, 1.0, 1.0 } },
    },
#ifdef FT_STROKER_H
    {
      .name =       "border_color",
      .long_name =  TRS("Border color"),
      .type =       BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 0.0, 0.0, 0.0, 1.0 } },
    },
    {
      .name =       "border_width",
      .long_name =  TRS("Border width"),
      .type =       BG_PARAMETER_FLOAT,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 10.0 },
      .val_default = { .val_f = 2.0 },
      .num_digits =  2,
    },
#endif    
    {
      .name =       "font",
      .long_name =  TRS("Font"),
      .type =       BG_PARAMETER_FONT,
      .val_default = { .val_str = "Courier-20" }
    },
    {
      .name =       "justify_h",
      .long_name =  TRS("Horizontal justify"),
      .type =       BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "right" },
      .multi_names =  (char const *[]){ "center", "left", "right", NULL },
      .multi_labels = (char const *[]){ TRS("Center"), TRS("Left"), TRS("Right"), NULL  },
            
    },
    {
      .name =       "justify_v",
      .long_name =  TRS("Vertical justify"),
      .type =       BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "top" },
      .multi_names =  (char const *[]){ "center", "top", "bottom", NULL  },
      .multi_labels = (char const *[]){ TRS("Center"), TRS("Top"), TRS("Bottom"), NULL },
    },
    {
      .name =        "cache_size",
      .long_name =   TRS("Cache size"),
      .type =        BG_PARAMETER_INT,
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .val_min =     { .val_i = 1     },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 255   },
      
      .help_string = TRS("Specify, how many different characters are cached for faster rendering. For European languages, this never needs to be larger than 255."),
    },
    {
      .name =        "border_left",
      .long_name =   TRS("Left border"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0     },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 10    },
      .help_string = TRS("Distance from the left text border to the image border"),
    },
    {
      .name =        "border_right",
      .long_name =   TRS("Right border"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0     },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 10    },
      .help_string = TRS("Distance from the right text border to the image border"),
    },
    {
      .name =        "border_top",
      .long_name =   TRS("Top border"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0     },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 10    },
      .help_string = TRS("Distance from the top text border to the image border"),
    },
    {
      .name =        "border_bottom",
      .long_name =   "Bottom border",
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0     },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 10    },
      .help_string = TRS("Distance from the bottom text border to the image border"),
    },
    {
      .name =        "ignore_linebreaks",
      .long_name =   TRS("Ignore linebreaks"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .help_string = TRS("Ignore linebreaks")
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_tcdisplay(void * priv)
  {
  return parameters;
  }

static void
set_parameter_tcdisplay(void * priv, const char * name,
                      const bg_parameter_value_t * val)
  {
  tc_priv_t * vp;
  vp = priv;

  if(!name)
    {
    bg_text_renderer_set_parameter(vp->renderer,
                                   NULL, NULL);
    }
  else if(!strcmp(name, "interpolate"))
    {
    vp->interpolate = val->val_i;
    }
  else
    bg_text_renderer_set_parameter(vp->renderer,
                                   name, val);

    }

static void reset_tcdisplay(void * priv)
  {
  tc_priv_t * vp;
  vp = priv;
  vp->last_timecode = GAVL_TIMECODE_UNDEFINED;
  }

static void set_format(void * priv,
                       const gavl_video_format_t * format)
  {
  tc_priv_t * vp;
  vp = priv;
  
  gavl_video_format_copy(&vp->format, format);

  bg_text_renderer_init(vp->renderer,
                        &vp->format,
                        &vp->ovl_format);
  
  gavl_overlay_blend_context_init(vp->blender,
                                  &vp->format,
                                  &vp->ovl_format);

  if(vp->ovl.frame)
    gavl_video_frame_destroy(vp->ovl.frame);
  vp->ovl.frame = gavl_video_frame_create(&vp->ovl_format);
  vp->last_timecode = GAVL_TIMECODE_UNDEFINED;
  }

static gavl_source_status_t
read_func(void * priv, gavl_video_frame_t ** frame)
  {
  gavl_source_status_t st;
  tc_priv_t * vp;
  char str[GAVL_TIMECODE_STRING_LEN];
  char * pos;
  vp = priv;
  
  if((st = gavl_video_source_read_frame(vp->in_src, frame)) !=
     GAVL_SOURCE_OK)
    return st;
  
  if((*frame)->timecode == GAVL_TIMECODE_UNDEFINED)
    {
    if(vp->interpolate && (vp->last_timecode != GAVL_TIMECODE_UNDEFINED))
      {
      int64_t framecount;
      framecount = gavl_timecode_to_framecount(&vp->format.timecode_format,
                                               vp->last_timecode);
      
      framecount++;
      
      vp->last_timecode =
        gavl_timecode_from_framecount(&vp->format.timecode_format,
                                      framecount);
      }
    else
      return 1;
    }
  else
    {
    vp->last_timecode = (*frame)->timecode;
    }
  
  gavl_timecode_prettyprint(&vp->format.timecode_format,
                            vp->last_timecode, str);

  pos = strchr(str, ' ');
  if(pos)
    *pos = '\n';
  
  //  fprintf(stderr, "Got timecode: %s\n", str);
  
  bg_text_renderer_render(vp->renderer, str, &vp->ovl);
  // bg_text_renderer_render(vp->renderer, "Blah", &vp->ovl);
  gavl_overlay_blend_context_set_overlay(vp->blender, &vp->ovl);
  
  gavl_overlay_blend(vp->blender, *frame);
  
  return GAVL_SOURCE_OK;
  }

static gavl_video_source_t *
connect_tcdisplay(void * priv, gavl_video_source_t * src,
                  const gavl_video_options_t * opt)
  {
  tc_priv_t * vp = priv;
  
  if(vp->out_src)
    {
    gavl_video_source_destroy(vp->out_src);
    vp->out_src = NULL;
    }
  vp->in_src = src;
  set_format(vp, gavl_video_source_get_src_format(vp->in_src));

  if(opt)
    gavl_video_options_copy(gavl_video_source_get_options(vp->in_src), opt);
  
  gavl_video_source_set_dst(vp->in_src, 0, &vp->format);
  vp->out_src = gavl_video_source_create(read_func, vp, 0, &vp->format);
  return vp->out_src;
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_tcdisplay",
      .long_name = TRS("Display timecodes"),
      .description = TRS("Burn timecodes into video frames"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_tcdisplay,
      .destroy =   destroy_tcdisplay,
      .get_parameters =   get_parameters_tcdisplay,
      .set_parameter =    set_parameter_tcdisplay,
      .priority =         1,
    },
    
    .connect = connect_tcdisplay,
    
    .reset      = reset_tcdisplay,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
