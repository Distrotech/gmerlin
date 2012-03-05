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

#include <gmerlin/ocr.h>
#include <gmerlin/subprocess.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>

struct bg_ocr_s
  {
  gavl_video_converter_t * cnv;
  bg_plugin_registry_t * plugin_reg;

  int do_convert;
  char lang[4];

  gavl_video_format_t in_format;
  gavl_video_format_t out_format;
  };

bg_ocr_t * bg_ocr_create(bg_plugin_registry_t * plugin_reg)
  {
  bg_ocr_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->cnv = gavl_video_converter_create();
  ret->plugin_reg = plugin_reg;
  
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
    {
      /* End */
    }
    
  };

const bg_parameter_info_t * bg_ocr_get_parameters(bg_ocr_t * ocr)
  {
  return parameters;
  }

void bg_ocr_set_parameter(void * ocr, const char * name,
                          const bg_parameter_value_t * val)
  {

  }


int bg_ocr_init(bg_ocr_t * ocr,
                const gavl_video_format_t * format,
                const char * language)
  {
  gavl_video_format_copy(&ocr->in_format, format);
  gavl_video_format_copy(&ocr->out_format, format);

  /* Get pixelformat for conversion */
    
  ocr->do_convert = gavl_video_converter_init(ocr->cnv,
                                              &ocr->in_format,
                                              &ocr->out_format);
  return 0;
  }


int bg_ocr_run(bg_ocr_t * ocr,
               const gavl_video_format_t * format,
               gavl_video_frame_t * frame,
               char ** ret)
  {
  return 0;
  }

void bg_ocr_destroy(bg_ocr_t * ocr)
  {
  if(ocr->cnv)
    gavl_video_converter_destroy(ocr->cnv);
  free(ocr);
  }
