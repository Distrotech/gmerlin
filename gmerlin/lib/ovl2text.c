/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
#include <gmerlin/translation.h>
#include <gmerlin/pluginregistry.h>

#include <ovl2text.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "ovl2text"

#include <gmerlin/ocr.h>


typedef struct
  {
  bg_encoder_callbacks_t * cb;
  bg_plugin_registry_t * reg;
  gavl_video_format_t format;
  bg_ocr_t * ocr;

  bg_parameter_info_t * parameters;
  } ovl2text_t;

void * bg_ovl2text_create(bg_plugin_registry_t * plugin_reg)
  {
  ovl2text_t * ret = calloc(1, sizeof(*ret));
  ret->reg = plugin_reg;

  ret->ocr = bg_ocr_create(ret->reg);
  return ret;
  }

static void set_callbacks_ovl2text(void * data, bg_encoder_callbacks_t * cb)
  {
  ovl2text_t * e = data;
  e->cb = cb;
  }

static int open_ovl2text(void * data, const char * filename,
                         const bg_metadata_t * metadata,
                         const bg_chapter_list_t * chapter_list)
  {
  ovl2text_t * e = data;
  
  return 0;
  }

static int add_subtitle_overlay_stream_ovl2text(void * priv, const char * language,
                                              const gavl_video_format_t * format)
  {
  return 0;
  }

static int start_ovl2text(void * priv)
  {
  return 1;
  }

static void get_subtitle_overlay_format_ovl2text(void * priv, int stream,
                                               gavl_video_format_t*ret)
  {
  ovl2text_t * ovl2text = priv;
  gavl_video_format_copy(ret, &ovl2text->format);
  }

static int write_subtitle_overlay_ovl2text(void * priv, gavl_overlay_t * ovl, int stream)
  {
  ovl2text_t * ovl2text = priv;
  return 0;
  }

static int close_ovl2text(void * priv, int do_delete)
  {
  ovl2text_t * ovl2text = priv;
  return 0;
  }

static void destroy_ovl2text(void * priv)
  {
  ovl2text_t * ovl2text = priv;
  
  }

static const bg_parameter_info_t * get_parameters_ovl2text(void * priv)
  {
  ovl2text_t * ovl2text = priv;
  return ovl2text->parameters;
  }

static void set_parameter_ovl2text(void * priv, const char * name, const bg_parameter_value_t * val)
  {
  
  }

static const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "e_ovl2text",       /* Unique short name */
      .long_name =      TRS("Overlay to text converter"),
      .description =    TRS("Exports overlay subtitles to a text format by performing an OCR."),
      .type =           BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY,
      .flags =          BG_PLUGIN_FILE,
      .priority =       BG_PLUGIN_PRIORITY_MAX,
      .destroy =        destroy_ovl2text,
      .get_parameters = get_parameters_ovl2text,
      .set_parameter =  set_parameter_ovl2text,
    },

    .max_subtitle_overlay_streams = 1,
    
    .set_callbacks =        set_callbacks_ovl2text,
    
    .open =                 open_ovl2text,

    .add_subtitle_overlay_stream =     add_subtitle_overlay_stream_ovl2text,
    
    .start =                start_ovl2text,

    .get_subtitle_overlay_format = get_subtitle_overlay_format_ovl2text,
    
    .write_subtitle_overlay = write_subtitle_overlay_ovl2text,
    .close =             close_ovl2text,
  };

const bg_plugin_common_t * bg_ovl2text_get()
  {
  return (bg_plugin_common_t *)&the_plugin;
  }

bg_plugin_info_t * bg_ovl2text_info(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * ret;
  ret = bg_plugin_info_create(&the_plugin.common);
  return ret;
  }
