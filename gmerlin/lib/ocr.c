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
#include <ctype.h>

#include <gmerlin/ocr.h>
#include <gmerlin/subprocess.h>
#include <gmerlin/utils.h>
#include <gmerlin/bggavl.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "ocr"

typedef struct
  {
  const char * name;
  int (*supported)(bg_plugin_registry_t * plugin_reg);
  int (*init)(bg_ocr_t * ocr, gavl_video_format_t*);
  int (*run)(bg_ocr_t * ocr, const gavl_video_format_t*,gavl_video_frame_t*,char ** ret);
  } ocr_funcs_t;

static int supported_tesseract(bg_plugin_registry_t * plugin_reg);
static int init_tesseract(bg_ocr_t *, gavl_video_format_t *);
static int run_tesseract(bg_ocr_t *, const gavl_video_format_t *, gavl_video_frame_t*,char ** ret);

static ocr_funcs_t ocr_funcs[] =
  {
    {
      .name      = "tesseract",
      .supported = supported_tesseract,
      .init      = init_tesseract,
      .run       = run_tesseract,
    },
    { /* */ }
  };

struct bg_ocr_s
  {
  gavl_video_converter_t * cnv;
  gavl_video_options_t * opt;
  
  bg_plugin_registry_t * plugin_reg;

  int do_convert;
  char lang[4];

  gavl_video_format_t in_format;
  gavl_video_format_t out_format;
  ocr_funcs_t * funcs;

  bg_plugin_handle_t * iw_handle;
  bg_image_writer_plugin_t * iw_plugin;

  gavl_video_frame_t * out_frame;
  
  bg_iw_callbacks_t cb;
  char * image_file;

  char * tmpdir;
  
  };

static int create_output_file(void * priv, const char * name)
  {
  bg_ocr_t * ocr = priv;
  ocr->image_file = gavl_strrep(ocr->image_file, name);
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Writing image file %s", name);
  return 1;
  }

static int load_image_writer(bg_ocr_t * ocr, const char * plugin)
  {
  const bg_plugin_info_t * info;

  if(ocr->iw_handle)
    {
    bg_plugin_unref(ocr->iw_handle);
    ocr->iw_handle = NULL;
    }
  
  info = bg_plugin_find_by_name(ocr->plugin_reg, plugin);
  ocr->iw_handle = bg_plugin_load(ocr->plugin_reg, info);
  if(!ocr->iw_handle)
    return 0;
  ocr->iw_plugin = (bg_image_writer_plugin_t*)ocr->iw_handle->plugin;

  if(ocr->iw_plugin->set_callbacks)
    ocr->iw_plugin->set_callbacks(ocr->iw_handle->priv, &ocr->cb);
  return 1;
  }

bg_ocr_t * bg_ocr_create(bg_plugin_registry_t * plugin_reg)
  {
  int i;
  bg_ocr_t * ret;
  ocr_funcs_t * funcs = NULL;

  i = 0;
  while(ocr_funcs[i].name)
    {
    if(ocr_funcs[i].supported(plugin_reg))
      funcs = &ocr_funcs[i];
    i++;
    }

  if(!funcs)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No engine found");
    return NULL;
    }
  
  ret = calloc(1, sizeof(*ret));

  ret->cb.data = ret;
  ret->cb.create_output_file = create_output_file;
  
  ret->cnv = gavl_video_converter_create();
  ret->opt = gavl_video_converter_get_options(ret->cnv);
  gavl_video_options_set_alpha_mode(ret->opt, GAVL_ALPHA_BLEND_COLOR);
  
  ret->plugin_reg = plugin_reg;
  ret->funcs = funcs;
    
  return ret;
  }

const bg_parameter_info_t parameters[] =
  {
    {                                    \
      .name =        "background_color",      \
      .long_name =   TRS("Background color"), \
      .type =      BG_PARAMETER_COLOR_RGB, \
      .val_default = { .val_color = { 0.0, 0.0, 0.0 } }, \
      .help_string = TRS("Background color to use, when converting formats with transparency to grayscale"), \
    },
    {                                    \
      .name =        "tmpdir",      \
      .long_name =   TRS("Temporary directory"), \
      .type =      BG_PARAMETER_DIRECTORY, \
      .val_default = { .val_str = "/tmp" }, \
      .help_string = TRS("Temporary directory for image files"), \
    },
    { /* End */ }
    
  };

const bg_parameter_info_t * bg_ocr_get_parameters()
  {
  return parameters;
  }

int bg_ocr_set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val)
  {
  bg_ocr_t * ocr = data;

  if(!name)
    return 1;
  else if(!strcmp(name, "background_color"))
    {
    gavl_video_options_set_background_color(ocr->opt, val->val_color);
    return 1;
    }
  else if(!strcmp(name, "tmpdir"))
    {
    ocr->tmpdir = gavl_strrep(ocr->tmpdir, val->val_str);
    return 1;
    }
  
  return 0;
  }


int bg_ocr_init(bg_ocr_t * ocr,
                const gavl_video_format_t * format,
                const char * language)
  {
  if(ocr->out_frame)
    {
    gavl_video_frame_destroy(ocr->out_frame);
    ocr->out_frame = NULL;
    }
  
  gavl_video_format_copy(&ocr->in_format, format);
  gavl_video_format_copy(&ocr->out_format, format);

  /* Get pixelformat for conversion */

  if(language && (language[0] != '\0'))
    strncpy(ocr->lang, language, 3);
  
  if(!ocr->funcs->init(ocr, &ocr->out_format))
    return 0;
  
  /* Initialize converter */
  ocr->do_convert = gavl_video_converter_init(ocr->cnv,
                                              &ocr->in_format,
                                              &ocr->out_format);

  if(ocr->do_convert)
    ocr->out_frame = gavl_video_frame_create(&ocr->out_format);
  
  return 1;
  }


int bg_ocr_run(bg_ocr_t * ocr,
               const gavl_video_format_t * format,
               gavl_video_frame_t * frame,
               char ** ret)
  {
  int result;
  gavl_video_format_t tmp_format;
  
  if(ocr->do_convert)
    {
    gavl_video_format_copy(&tmp_format, format);
    tmp_format.pixelformat = ocr->out_format.pixelformat;
    
    gavl_video_converter_init(ocr->cnv,
                              &ocr->in_format,
                              &tmp_format);
    gavl_video_convert(ocr->cnv, frame, ocr->out_frame);
    
    result = ocr->funcs->run(ocr, &tmp_format, ocr->out_frame, ret);
    }
  else
    result = ocr->funcs->run(ocr, format, frame, ret);

  if(!result || (**ret == '\0'))
    {
    if(*ret)
      free(*ret);
    
    bg_log(BG_LOG_WARNING, LOG_DOMAIN,
           "OCR failed, keeping %s", ocr->image_file);
    *ret = ocr->image_file;
    ocr->image_file = NULL;
    }
  else
    {
    if(ocr->image_file)
      {
      bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Removing %s", ocr->image_file);
      remove(ocr->image_file);
      free(ocr->image_file);
      ocr->image_file = NULL;
      }
    }
  return result;
  }

void bg_ocr_destroy(bg_ocr_t * ocr)
  {
  if(ocr->cnv)
    gavl_video_converter_destroy(ocr->cnv);
  if(ocr->out_frame)
    gavl_video_frame_destroy(ocr->out_frame);
  if(ocr->iw_handle)
    bg_plugin_unref(ocr->iw_handle);

  if(ocr->image_file)
    free(ocr->image_file);
  if(ocr->tmpdir)
    free(ocr->tmpdir);
  
  free(ocr);
  }

/* Tesseract */

static int supported_tesseract(bg_plugin_registry_t * plugin_reg)
  {
  if(!bg_search_file_exec("tesseract", NULL))
    return 0;
  if(!bg_plugin_find_by_name(plugin_reg, "iw_tiff"))
    return 0;
  return 1;
  }

static int init_tesseract(bg_ocr_t * ocr, gavl_video_format_t * format)
  {
  format->pixelformat = GAVL_GRAY_8;

  if(!ocr->iw_handle)
    {
    if(!load_image_writer(ocr, "iw_tiff"))
      return 0;
    }
  
  return 1;
  }

static int run_tesseract(bg_ocr_t * ocr, const gavl_video_format_t * format,
                         gavl_video_frame_t * frame, char ** ret)
  {
  char * pos;
  char * commandline = NULL;
  gavl_video_format_t tmp_format;
  char * tiff_file = NULL;
  char * text_file = NULL;
  char * base = NULL;
  int result = 0;

  char * template = bg_sprintf("%s/gmerlin_ocr_%%05d.tif", ocr->tmpdir);
  
  /* Create name for tiff file */
  tiff_file = bg_create_unique_filename(template);

  free(template);
  
  if(!tiff_file)
    return 0;

  
  base = gavl_strdup(tiff_file);
  pos = strrchr(base, '.');
  if(!pos)
    return 0;
  
  *pos = '\0';

  /* Create name for text file */
  text_file = bg_sprintf("%s.txt", base);
  
  /* Save image */
  
  gavl_video_format_copy(&tmp_format, format);

  if(!ocr->iw_plugin->write_header(ocr->iw_handle->priv, base, &tmp_format, NULL))
    goto fail;
  if(!ocr->iw_plugin->write_image(ocr->iw_handle->priv, frame))
    goto fail;

  commandline = bg_sprintf("tesseract %s %s", ocr->image_file, base);

  if(ocr->lang[0] != '\0')
    {
    commandline = gavl_strcat(commandline, " -l ");
    commandline = gavl_strcat(commandline, bg_iso639_b_to_t(ocr->lang));
    }
  
  if(bg_system(commandline))
    goto fail;
  
  *ret = bg_read_file(text_file, NULL);
  
  if(!(*ret))
    goto fail;
  
  pos = (*ret) + (strlen(*ret)-1);

  while(isspace(*pos) && (pos >= *ret))
    {
    *pos = '\0';
    pos--;
    }

  result = 1;

  fail:

  if(tiff_file)
    free(tiff_file);
  
  if(base)
    free(base);
  if(text_file)
    {
    remove(text_file);
    free(text_file);
    }
  if(commandline)
    free(commandline);
  return result;
  }


