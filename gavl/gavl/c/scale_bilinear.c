/*****************************************************************
 * gavl - a general purpose audio/video processing library
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
#include <stdio.h>
#include <gavl/gavl.h>
#include <video.h>
#include <scale.h>

static void scale_rgb_15_y_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  
  }

static void scale_rgb_16_y_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  
  }
  
static void scale_uint8_x_1_y_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  
  }

static void scale_uint8_x_3_y_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  
  }

static void scale_uint8_x_4_y_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  
  }

static void scale_uint16_x_1_y_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  
  }

static void scale_uint16_x_3_y_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  
  }

static void scale_uint16_x_4_y_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  
  }

static void scale_float_x_3_y_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  
  }

static void scale_float_x_4_y_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  
  }


void gavl_init_scale_funcs_bilinear_c(gavl_scale_funcs_t * tab, int scale_x, int scale_y)
  {
  if(scale_x && scale_y)
    {
    tab->scale_rgb_15 =     scale_rgb_15_xy_bilinear_c;
    tab->scale_rgb_16 =     scale_rgb_16_xy_bilinear_c;
    tab->scale_uint8_x_1 =  scale_uint8_x_1_xy_bilinear_c;
    tab->scale_uint8_x_3 =  scale_uint8_x_3_xy_bilinear_c;
    tab->scale_uint8_x_4 =  scale_uint8_x_4_xy_bilinear_c;
    tab->scale_uint16_x_1 = scale_uint16_x_1_xy_bilinear_c;
    tab->scale_uint16_x_3 = scale_uint16_x_3_xy_bilinear_c;
    tab->scale_uint16_x_4 = scale_uint16_x_4_xy_bilinear_c;
    tab->scale_float_x_3 =  scale_float_x_3_xy_bilinear_c;
    tab->scale_float_x_4 =  scale_float_x_4_xy_bilinear_c;
    }
  if(scale_x)
    {
    tab->scale_rgb_15 =     scale_rgb_15_x_bilinear_c;
    tab->scale_rgb_16 =     scale_rgb_16_x_bilinear_c;
    tab->scale_uint8_x_1 =  scale_uint8_x_1_x_bilinear_c;
    tab->scale_uint8_x_3 =  scale_uint8_x_3_x_bilinear_c;
    tab->scale_uint8_x_4 =  scale_uint8_x_4_x_bilinear_c;
    tab->scale_uint16_x_1 = scale_uint16_x_1_x_bilinear_c;
    tab->scale_uint16_x_3 = scale_uint16_x_3_x_bilinear_c;
    tab->scale_uint16_x_4 = scale_uint16_x_4_x_bilinear_c;
    tab->scale_float_x_3 =  scale_float_x_3_x_bilinear_c;
    tab->scale_float_x_4 =  scale_float_x_4_x_bilinear_c;
    }
  if(scale_y)
    {
    tab->scale_rgb_15 =     scale_rgb_15_y_bilinear_c;
    tab->scale_rgb_16 =     scale_rgb_16_y_bilinear_c;
    tab->scale_uint8_x_1 =  scale_uint8_x_1_y_bilinear_c;
    tab->scale_uint8_x_3 =  scale_uint8_x_3_y_bilinear_c;
    tab->scale_uint8_x_4 =  scale_uint8_x_4_y_bilinear_c;
    tab->scale_uint16_x_1 = scale_uint16_x_1_y_bilinear_c;
    tab->scale_uint16_x_3 = scale_uint16_x_3_y_bilinear_c;
    tab->scale_uint16_x_4 = scale_uint16_x_4_y_bilinear_c;
    tab->scale_float_x_3 =  scale_float_x_3_y_bilinear_c;
    tab->scale_float_x_4 =  scale_float_x_4_y_bilinear_c;
    }
  tab->bits_rgb_15 = 8;
  tab->bits_rgb_16 = 8;
  tab->bits_uint8  = 8;
  tab->bits_uint16 = 16;

  }
