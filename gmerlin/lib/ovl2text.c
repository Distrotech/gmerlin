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

#include <config.h>

#include <string.h>


#include <gmerlin/translation.h>
#include <gmerlin/pluginregistry.h>
#include <gavl/metatags.h>

#include <ovl2text.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "ovl2text"

#include <gmerlin/ocr.h>


typedef struct
  {
  bg_encoder_callbacks_t * cb;
  bg_plugin_registry_t * plugin_reg;
  gavl_video_format_t format;
  bg_ocr_t * ocr;

  bg_parameter_info_t * parameters;

  bg_plugin_handle_t * enc_handle;
  bg_encoder_plugin_t * enc_plugin;

  } ovl2text_t;

void * bg_ovl2text_create(bg_plugin_registry_t * plugin_reg)
  {
  ovl2text_t * ret = calloc(1, sizeof(*ret));
  ret->plugin_reg = plugin_reg;

  ret->ocr = bg_ocr_create(ret->plugin_reg);
  return ret;
  }

static void set_callbacks_ovl2text(void * data, bg_encoder_callbacks_t * cb)
  {
  ovl2text_t * e = data;
  e->cb = cb;
  }

static int open_ovl2text(void * data, const char * filename,
                         const gavl_metadata_t * metadata,
                         const gavl_chapter_list_t * chapter_list)
  {
  ovl2text_t * e = data;
  
  /* Open text subtitle encoder */
  if(!e->enc_plugin || !e->enc_plugin->open(e->enc_handle->priv, filename,
                                            metadata, chapter_list))
    return 0;
  return 1;
  }

static int
add_subtitle_overlay_stream_ovl2text(void * priv,
                                     const gavl_metadata_t * m,
                                     const gavl_video_format_t * format)
  {
  const char * language;
  ovl2text_t * e = priv;

  language = gavl_metadata_get(m, GAVL_META_LANGUAGE);
  
  if(!bg_ocr_init(e->ocr, format, language))
    return -1;

  gavl_video_format_copy(&e->format, format);
  
  if(!e->enc_plugin ||
     (e->enc_plugin->add_subtitle_text_stream(e->enc_handle->priv, m,
                                              &e->format.timescale) < 0))
    return -1;
  return 0;
  }

static int start_ovl2text(void * priv)
  {
  ovl2text_t * e = priv;
  if(e->enc_plugin->start && !e->enc_plugin->start(e->enc_handle->priv))
    return 0;
  return 1;
  }

static void get_subtitle_overlay_format_ovl2text(void * priv, int stream,
                                               gavl_video_format_t*ret)
  {
  ovl2text_t * e = priv;
  gavl_video_format_copy(ret, &e->format);
  }

static int write_subtitle_overlay_ovl2text(void * priv, gavl_overlay_t * ovl, int stream)
  {
  int ret = 0;
  char * str = NULL;
  ovl2text_t * e = priv;
  
  bg_ocr_run(e->ocr, &e->format, ovl, &str);
  
  if(str)
    ret = e->enc_plugin->write_subtitle_text(e->enc_handle->priv,
                                             str, ovl->timestamp,
                                             ovl->duration, stream);
  return ret;
  }

static int close_ovl2text(void * priv, int do_delete)
  {
  ovl2text_t * e = priv;
  e->enc_plugin->close(e->enc_handle->priv, do_delete);
  return 1;
  }

static void destroy_ovl2text(void * priv)
  {
  ovl2text_t * e = priv;
  bg_ocr_destroy(e->ocr);

  if(e->parameters)
    bg_parameter_info_destroy_array(e->parameters);

  if(e->enc_handle)
    bg_plugin_unref(e->enc_handle);
  
  free(e);
  }

static const bg_parameter_info_t enc_section[] =
  {
    {
      .name =        "plugin",
      .long_name =   TRS("Plugin"),
      .type =      BG_PARAMETER_MULTI_MENU,
      .help_string = TRS("Plugin for writing the text subtitles"),
    },
    { /* End */ }
  };

static bg_parameter_info_t * create_parameters(bg_plugin_registry_t * plugin_reg)
  {
  const bg_parameter_info_t * info[3];
  bg_parameter_info_t * enc;
  bg_parameter_info_t * ret;
  enc = bg_parameter_info_copy_array(enc_section);
  bg_plugin_registry_set_parameter_info(plugin_reg,
                                        BG_PLUGIN_ENCODER_SUBTITLE_TEXT,
                                        BG_PLUGIN_FILE, &enc[0]);
  info[0] = bg_ocr_get_parameters();
  info[1] = enc;
  info[2] = NULL;
  ret = bg_parameter_info_concat_arrays(info);
  
  bg_parameter_info_destroy_array(enc);
  return ret;
  }


static const bg_parameter_info_t * get_parameters_ovl2text(void * priv)
  {
  ovl2text_t * e = priv;
  
  if(e->parameters)
    return e->parameters;
  
  /* Create parameters */
  e->parameters = create_parameters(e->plugin_reg);
  return e->parameters;
  }

static void set_parameter_ovl2text(void * priv, const char * name,
                                   const bg_parameter_value_t * val)
  {
  ovl2text_t * e = priv;
  if(!name)
    {
    bg_ocr_set_parameter(e->ocr, NULL, NULL);
    return;
    }
  else if(bg_ocr_set_parameter(e->ocr, name, val))
    return;
  else if(!strcmp(name, "plugin"))
    {
    const bg_plugin_info_t * info;

    if(e->enc_handle)
      {
      bg_plugin_unref(e->enc_handle);
      e->enc_handle = NULL;
      }
    info = bg_plugin_find_by_name(e->plugin_reg, val->val_str);
    
    e->enc_handle = bg_plugin_load(e->plugin_reg, info);
    e->enc_plugin = (bg_encoder_plugin_t*)(e->enc_handle->plugin);
    
    if(e->enc_plugin->set_callbacks && e->cb)
      e->enc_plugin->set_callbacks(e->enc_handle->priv, e->cb);
    }
  else
    e->enc_plugin->common.set_parameter(e->enc_handle->priv, name, val);
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
  ret->parameters = create_parameters(reg);
  return ret;
  }
