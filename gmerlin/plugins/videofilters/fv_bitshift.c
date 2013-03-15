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

#define LOG_DOMAIN "fv_bitshift"

typedef struct
  {
  int shift;
  
  gavl_video_format_t format;
  
  int samples_per_line;

  gavl_video_source_t * in_src;
  gavl_video_source_t * out_src;

  } shift_priv_t;

static void * create_shift()
  {
  shift_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_shift(void * priv)
  {
  shift_priv_t * vp;
  vp = priv;

  if(vp->out_src)
    gavl_video_source_destroy(vp->out_src);
  
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "shift",
      .long_name = TRS("Bits to shift"),
      .type = BG_PARAMETER_SLIDER_INT,
      .val_min = { .val_i = 0 },
      .val_max = { .val_i = 8 },
      .flags = BG_PARAMETER_SYNC,
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_shift(void * priv)
  {
  return parameters;
  }

static void set_parameter_shift(void * priv, const char * name,
                               const bg_parameter_value_t * val)
  {
  shift_priv_t * vp;
  vp = priv;

  if(!name)
    return;

  if(!strcmp(name, "shift"))
    vp->shift = val->val_i;
  }


/* We support all 16 bit formats */
static const gavl_pixelformat_t pixelformats[] =
  {
    GAVL_GRAY_16,
    GAVL_GRAYA_32,
    GAVL_RGB_48,
    GAVL_RGBA_64,
    GAVL_PIXELFORMAT_NONE,
  };

static gavl_source_status_t read_func(void * priv,
                                      gavl_video_frame_t ** frame)
  {
  gavl_source_status_t st;
  shift_priv_t * vp;
  int i, j;
  uint16_t * ptr;
  gavl_video_frame_t * f;

  
  vp = priv;
  
  if((st = gavl_video_source_read_frame(vp->in_src, frame)) != GAVL_SOURCE_OK)
    return st;

  f = *frame;
  
  if(vp->shift)
    {
    for(i = 0; i < vp->format.image_height; i++)
      {
      ptr = (uint16_t*)(f->planes[0] + i * f->strides[0]);

      for(j = 0; j < vp->samples_per_line; j++)
        {
        (*ptr) <<= vp->shift;
        ptr++;
        }
      }
    }
  return GAVL_SOURCE_OK;
  }

static gavl_video_source_t *
connect_shift(void * priv, gavl_video_source_t * src,
              const gavl_video_options_t * opt)
  {
  shift_priv_t * vp;
  int width_mult;

  vp = priv;

  if(vp->out_src)
    gavl_video_source_destroy(vp->out_src);
  
  vp->in_src = src;
  gavl_video_format_copy(&vp->format,
                         gavl_video_source_get_src_format(vp->in_src));

  width_mult = 3;
  vp->format.pixelformat =
    gavl_pixelformat_get_best(vp->format.pixelformat,
                              pixelformats,
                              NULL);
  //  gavl_video_format_copy(&vp->format, format);
  
  if(gavl_pixelformat_is_gray(vp->format.pixelformat))
    width_mult = 1;
  if(gavl_pixelformat_has_alpha(vp->format.pixelformat))
    width_mult++;
  vp->samples_per_line = vp->format.image_width * width_mult;
  
  gavl_video_source_set_dst(vp->in_src, 0, &vp->format);

  vp->out_src =
    gavl_video_source_create_source(read_func,
                                    vp, 0,
                                    vp->in_src);
  return vp->out_src;
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_shift",
      .long_name = TRS("Shift image"),
      .description = TRS("Upshift 16 bit images, where only some lower bits are used"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_shift,
      .destroy =   destroy_shift,
      .get_parameters =   get_parameters_shift,
      .set_parameter =    set_parameter_shift,
      .priority =         1,
    },
    
    .connect = connect_shift,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
