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


/* System includes */

#include <stdlib.h>
#include <pthread.h>

/* Fontconfig */

#include <fontconfig/fontconfig.h>

/* Gmerlin */

#include <config.h>
#include <translation.h>


#include <bgfreetype.h>

#include <parameter.h>
#include <textrenderer.h>
#include <utils.h>
#include <charset.h>

// #undef FT_STROKER_H

/* Text alignment */

#define JUSTIFY_CENTER 0
#define JUSTIFY_LEFT   1
#define JUSTIFY_RIGHT  2

#define JUSTIFY_TOP    1
#define JUSTIFY_BOTTOM 2

static const bg_parameter_info_t parameters[] =
  {
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
      .val_default = { .val_str = "Sans-20" }
    },
    {
      .name =       "justify_h",
      .long_name =  TRS("Horizontal justify"),
      .type =       BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "center" },
      .multi_names =  (char const *[]){ "center", "left", "right", (char*)0 },
      .multi_labels = (char const *[]){ TRS("Center"), TRS("Left"), TRS("Right"), (char*)0  },
            
    },
    {
      .name =       "justify_v",
      .long_name =  TRS("Vertical justify"),
      .type =       BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "bottom" },
      .multi_names =  (char const *[]){ "center", "top", "bottom", (char*)0  },
      .multi_labels = (char const *[]){ TRS("Center"), TRS("Top"), TRS("Bottom"), (char*)0 },
    },
    {
      .name =        "cache_size",
      .long_name =   TRS("Cache size"),
      .type =        BG_PARAMETER_INT,
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
      .long_name =   TRS("Left border"),
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
      .help_string = TRS("Ignore linebreaks")
    },
    {
      .name =       "default_format",
      .long_name =  TRS("Default format"),
      .type =       BG_PARAMETER_SECTION,
    },
    {
      .name =       "default_width",
      .long_name =  TRS("Default width"),
      .type =       BG_PARAMETER_INT,
      .val_min =     { .val_i = 0     },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 640   },
    },
    {
      .name =       "default_height",
      .long_name =  TRS("Default height"),
      .type =       BG_PARAMETER_INT,
      .val_min =     { .val_i = 0     },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 480   },
    },
    {
      .name =       "default_csp",
      .long_name =  TRS("Default Colorspace"),
      .type =       BG_PARAMETER_STRINGLIST,
      .val_default =  { .val_str = "yuv" },
      .multi_names =  (char const *[]){ "yuv", "rgb", (char*)0 },
      .multi_labels = (char const *[]){ TRS("YCrCb"), TRS("RGB"), (char*)0 },
    },
    {
      .name =       "default_framerate",
      .long_name =  TRS("Default Framerate"),
      .type =       BG_PARAMETER_FLOAT,
      .val_default =  { .val_f = 10.0 },
      .num_digits = 3,
    },
    { /* End of parameters */ },
  };

typedef struct
  {
  int xmin, xmax, ymin, ymax;
  } bbox_t;

typedef struct
  {
  uint32_t unicode;
  FT_Glyph glyph;
#ifdef FT_STROKER_H
  FT_Glyph glyph_stroke;
#endif
  int advance_x, advance_y; /* Advance in integer pixels */
  bbox_t bbox;
  } cache_entry_t;

struct bg_text_renderer_s
  {
  gavl_rectangle_i_t max_bbox; /* Maximum bounding box the text may have */
  
  FT_Library library;
  FT_Face    face;
  
  int font_loaded;
  int font_changed;
  
  /* Configuration stuff */

  char * font;
  char * font_file;
  
  double font_size;
  
  float color[4];
  float color_config[4];
  float alpha_f;
  int   alpha_i;

#ifdef FT_STROKER_H
  float color_stroke[4];
  int color_i[4];
  FT_Stroker stroker;
  float border_width;
#endif  
  
  /* Charset converter */

  bg_charset_converter_t * cnv;

  /* Glyph cache */

  cache_entry_t * cache;
  
  int cache_size;
  int cache_alloc;
  gavl_video_format_t overlay_format;
  gavl_video_format_t frame_format;
  gavl_video_format_t last_frame_format;

  int justify_h;
  int justify_v;
  int border_left, border_right, border_top, border_bottom;
  
  bbox_t bbox; /* Actual bounding box of the text */
  int ignore_linebreaks;

  int sub_h, sub_v; /* Chroma subsampling of the final destination frame */
  
  void (*render_func)(bg_text_renderer_t * r, cache_entry_t * glyph,
                      gavl_video_frame_t * frame,
                      int * dst_x, int * dst_y);
  pthread_mutex_t config_mutex;

  int config_changed;

  int fixed_width;
  
  int default_width, default_height;
  gavl_pixelformat_t default_csp;
  float default_framerate;
  };

static void adjust_bbox(cache_entry_t * glyph, int dst_x, int dst_y, bbox_t * ret)
  {
  if(ret->xmin > dst_x + glyph->bbox.xmin)
    ret->xmin = dst_x + glyph->bbox.xmin;

  if(ret->ymin > dst_y + glyph->bbox.ymin)
    ret->ymin = dst_y + glyph->bbox.ymin;

  if(ret->xmax < dst_x + glyph->bbox.xmax)
    ret->xmax = dst_x + glyph->bbox.xmax;

  if(ret->ymax < dst_y + glyph->bbox.ymax)
    ret->ymax = dst_y + glyph->bbox.ymax;
  }

#define MY_MIN(x, y) (x < y ? x : y)

static void render_rgba_32(bg_text_renderer_t * r, cache_entry_t * glyph,
                           gavl_video_frame_t * frame,
                           int * dst_x, int * dst_y)
  {
  FT_BitmapGlyph bitmap_glyph;
  uint8_t * src_ptr, * dst_ptr, * src_ptr_start, * dst_ptr_start;
  int i, j, i_tmp, jmax;
#ifdef FT_STROKER_H
  int alpha_i;
#endif
 

#ifdef FT_STROKER_H
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph_stroke);
#else
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);
#endif
  
  if(!bitmap_glyph->bitmap.buffer)
    {
    *dst_x += glyph->advance_x;
    *dst_y += glyph->advance_y;
    return;
    }

  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 4;

  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);
    
  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      i_tmp = ((int)*src_ptr * (int)r->alpha_i) >> 8;
      if(i_tmp > dst_ptr[3])
        dst_ptr[3] = i_tmp;
      src_ptr++;
      dst_ptr += 4;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  // #if 0
#ifdef FT_STROKER_H
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);

  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 4;

  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);
  
  
  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      if(*src_ptr)
        {
        alpha_i = *src_ptr;
        
        dst_ptr[0] = (int)dst_ptr[0] +
          ((alpha_i * ((int)(r->color_i[0]) - (int)dst_ptr[0])) >> 8);

        dst_ptr[1] = (int)dst_ptr[1] +
          ((alpha_i * ((int)(r->color_i[1]) - (int)dst_ptr[1])) >> 8);

        dst_ptr[2] = (int)dst_ptr[2] +
          ((alpha_i * ((int)(r->color_i[2]) - (int)dst_ptr[2])) >> 8);

        }

      src_ptr++;
      dst_ptr += 4;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  
#endif
  
  *dst_x += glyph->advance_x;
  *dst_y += glyph->advance_y;
  }

static void render_graya_16(bg_text_renderer_t * r, cache_entry_t * glyph,
                            gavl_video_frame_t * frame,
                            int * dst_x, int * dst_y)
  {
  FT_BitmapGlyph bitmap_glyph;
  uint8_t * src_ptr, * dst_ptr, * src_ptr_start, * dst_ptr_start;
  int i, j, i_tmp, jmax;
#ifdef FT_STROKER_H
  int alpha_i;
#endif
 

#ifdef FT_STROKER_H
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph_stroke);
#else
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);
#endif
  
  if(!bitmap_glyph->bitmap.buffer)
    {
    *dst_x += glyph->advance_x;
    *dst_y += glyph->advance_y;
    return;
    }

  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 2;
  
  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);
    
  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      i_tmp = ((int)*src_ptr * (int)r->alpha_i) >> 8;
      if(i_tmp > dst_ptr[1])
        dst_ptr[1] = i_tmp;
      src_ptr++;
      dst_ptr += 2;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  // #if 0
#ifdef FT_STROKER_H
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);

  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 2;

  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);
  
  
  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      if(*src_ptr)
        {
        alpha_i = *src_ptr;
        
        dst_ptr[0] = (int)dst_ptr[0] +
          ((alpha_i * ((int)(r->color_i[0]) - (int)dst_ptr[0])) >> 8);
        }
      src_ptr++;
      dst_ptr += 2;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  
#endif
  
  *dst_x += glyph->advance_x;
  *dst_y += glyph->advance_y;
  }


static void render_rgba_64(bg_text_renderer_t * r, cache_entry_t * glyph,
                           gavl_video_frame_t * frame,
                           int * dst_x, int * dst_y)
  {
  FT_BitmapGlyph bitmap_glyph;
  uint8_t * src_ptr, * src_ptr_start, * dst_ptr_start;
  uint16_t * dst_ptr;
  int i, j, i_tmp, jmax;
#ifdef FT_STROKER_H
  int alpha_i;
#endif

#ifdef FT_STROKER_H
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph_stroke);
#else
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);
#endif
  
  if(!bitmap_glyph->bitmap.buffer)
    {
    *dst_x += glyph->advance_x;
    *dst_y += glyph->advance_y;
    return;
    }

  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 8;

  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);
    
  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = (uint16_t*)dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      i_tmp = ((int)*src_ptr * (int)r->alpha_i) >> 8;
      if(i_tmp > dst_ptr[3])
        dst_ptr[3] = i_tmp;
      
      src_ptr++;
      dst_ptr += 4;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  // #if 0
#ifdef FT_STROKER_H
  /* Render border */
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);

  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 8;

  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);
  
  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = (uint16_t*)dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      if(*src_ptr)
        {
        //        alpha_i = ((int)*src_ptr * (int)r->color_i[3]) >> 8;
        alpha_i = *src_ptr;
        
        dst_ptr[0] = (int)dst_ptr[0] +
          ((alpha_i * ((int64_t)(r->color_i[0]) - (int64_t)dst_ptr[0])) >> 8);

        dst_ptr[1] = (int)dst_ptr[1] +
          ((alpha_i * ((int64_t)(r->color_i[1]) - (int64_t)dst_ptr[1])) >> 8);

        dst_ptr[2] = (int)dst_ptr[2] +
          ((alpha_i * ((int64_t)(r->color_i[2]) - (int64_t)dst_ptr[2])) >> 8);

        }
      src_ptr++;
      dst_ptr += 4;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  
#endif
  
  *dst_x += glyph->advance_x;
  *dst_y += glyph->advance_y;

  }

static void render_graya_32(bg_text_renderer_t * r, cache_entry_t * glyph,
                            gavl_video_frame_t * frame,
                            int * dst_x, int * dst_y)
  {
  FT_BitmapGlyph bitmap_glyph;
  uint8_t * src_ptr, * src_ptr_start, * dst_ptr_start;
  uint16_t * dst_ptr;
  int i, j, i_tmp, jmax;
#ifdef FT_STROKER_H
  int alpha_i;
#endif

#ifdef FT_STROKER_H
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph_stroke);
#else
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);
#endif
  
  if(!bitmap_glyph->bitmap.buffer)
    {
    *dst_x += glyph->advance_x;
    *dst_y += glyph->advance_y;
    return;
    }

  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 4;

  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);
    
  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = (uint16_t*)dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      i_tmp = ((int)*src_ptr * (int)r->alpha_i) >> 8;
      if(i_tmp > dst_ptr[1])
        dst_ptr[1] = i_tmp;
      
      src_ptr++;
      dst_ptr += 2;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  // #if 0
#ifdef FT_STROKER_H
  /* Render border */
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);

  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 4;

  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);
  
  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = (uint16_t*)dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      if(*src_ptr)
        {
        //        alpha_i = ((int)*src_ptr * (int)r->color_i[3]) >> 8;
        alpha_i = *src_ptr;
        
        dst_ptr[0] = (int)dst_ptr[0] +
          ((alpha_i * ((int64_t)(r->color_i[0]) - (int64_t)dst_ptr[0])) >> 8);
        }
      src_ptr++;
      dst_ptr += 2;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  
#endif
  
  *dst_x += glyph->advance_x;
  *dst_y += glyph->advance_y;
  }


static void render_rgba_float(bg_text_renderer_t * r, cache_entry_t * glyph,
                              gavl_video_frame_t * frame,
                              int * dst_x, int * dst_y)
  {
  FT_BitmapGlyph bitmap_glyph;
  uint8_t * src_ptr, * src_ptr_start, * dst_ptr_start;
  float * dst_ptr;
  int i, j, jmax;
  float f_tmp;
#ifdef FT_STROKER_H
  float alpha_f;
#endif

#ifdef FT_STROKER_H
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph_stroke);
#else
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);
#endif
  
  if(!bitmap_glyph->bitmap.buffer)
    {
    *dst_x += glyph->advance_x;
    *dst_y += glyph->advance_y;
    return;
    }

  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 4 * sizeof(float);


  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);

  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = (float*)dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      f_tmp = ((float)*src_ptr * r->alpha_f) / 255.0;
      if(f_tmp > dst_ptr[3])
        dst_ptr[3] = f_tmp;
      
      src_ptr++;
      dst_ptr += 4;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  // #if 0
#ifdef FT_STROKER_H
  /* Render border */
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);
  
  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 4 * sizeof(float);

  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);
  
  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = (float*)dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      if(*src_ptr)
        {
        alpha_f = (float)(*src_ptr) / 255.0;
        
        dst_ptr[0] = dst_ptr[0] + alpha_f * (r->color[0] - dst_ptr[0]);
        dst_ptr[1] = dst_ptr[1] + alpha_f * (r->color[1] - dst_ptr[1]);
        dst_ptr[2] = dst_ptr[2] + alpha_f * (r->color[2] - dst_ptr[2]);
        }
      src_ptr++;
      dst_ptr += 4;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  
#endif
  
  *dst_x += glyph->advance_x;
  *dst_y += glyph->advance_y;

  }

static void render_graya_float(bg_text_renderer_t * r, cache_entry_t * glyph,
                               gavl_video_frame_t * frame,
                               int * dst_x, int * dst_y)
  {
  FT_BitmapGlyph bitmap_glyph;
  uint8_t * src_ptr, * src_ptr_start, * dst_ptr_start;
  float * dst_ptr;
  int i, j, jmax;
  float f_tmp;
#ifdef FT_STROKER_H
  float alpha_f;
#endif

#ifdef FT_STROKER_H
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph_stroke);
#else
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);
#endif
  
  if(!bitmap_glyph->bitmap.buffer)
    {
    *dst_x += glyph->advance_x;
    *dst_y += glyph->advance_y;
    return;
    }

  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 2 * sizeof(float);


  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);

  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = (float*)dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      f_tmp = ((float)*src_ptr * r->alpha_f) / 255.0;
      if(f_tmp > dst_ptr[1])
        dst_ptr[1] = f_tmp;
      
      src_ptr++;
      dst_ptr += 2;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  // #if 0
#ifdef FT_STROKER_H
  /* Render border */
  bitmap_glyph = (FT_BitmapGlyph)(glyph->glyph);
  
  src_ptr_start = bitmap_glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - bitmap_glyph->top) *
    frame->strides[0] + (*dst_x + bitmap_glyph->left) * 2 * sizeof(float);

  jmax = MY_MIN(bitmap_glyph->bitmap.width, 
                bitmap_glyph->bitmap.pitch);
  
  for(i = 0; i < bitmap_glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = (float*)dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {
      if(*src_ptr)
        {
        alpha_f = (float)(*src_ptr) / 255.0;
        
        dst_ptr[0] = dst_ptr[0] + alpha_f * (r->color[0] - dst_ptr[0]);
        }
      src_ptr++;
      dst_ptr += 2;
      }
    src_ptr_start += bitmap_glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  
#endif
  
  *dst_x += glyph->advance_x;
  *dst_y += glyph->advance_y;

  }


static void alloc_glyph_cache(bg_text_renderer_t * r, int size)
  {
  int i;
  if(size > r->cache_alloc)
    {
    /* Enlarge */
    r->cache_alloc = size;
    r->cache = realloc(r->cache, r->cache_alloc * sizeof(*(r->cache)));
    }
  else if(size < r->cache_alloc)
    {
    /* Shrink */
    if(size < r->cache_size)
      {
      for(i = size; i < r->cache_size; i++)
        FT_Done_Glyph(r->cache[i].glyph);
      r->cache_size = size;
      }
    r->cache_alloc = size;
    r->cache = realloc(r->cache, r->cache_alloc * sizeof(*(r->cache)));
    }
  else
    return;
  }

static void clear_glyph_cache(bg_text_renderer_t * r)
  {
  int i;
  for(i = 0; i < r->cache_size; i++)
    {
    FT_Done_Glyph(r->cache[i].glyph);
#ifdef FT_STROKER_H
    FT_Done_Glyph(r->cache[i].glyph_stroke);
#endif
    }
  r->cache_size = 0;
  }

static cache_entry_t * get_glyph(bg_text_renderer_t * r, uint32_t unicode)
  {
  int i, index;
  cache_entry_t * entry;
  
  FT_BitmapGlyph bitmap_glyph;
      
  for(i = 0; i < r->cache_size; i++)
    {
    if(r->cache[i].unicode == unicode)
      return &r->cache[i];
    }

  /* No glyph found, try to load a new one into the cache */
  if(r->cache_size >= r->cache_alloc)
    {
    FT_Done_Glyph(r->cache[0].glyph);
    index = 0;
    }
  else
    {
    index = r->cache_size;
    r->cache_size++;
    }
  entry = &(r->cache[index]);
  
  /* Load the glyph */
  if(FT_Load_Char(r->face, unicode, FT_LOAD_DEFAULT))
    {
    return (cache_entry_t*)0;
    }
  /* extract glyph image */
  if(FT_Get_Glyph(r->face->glyph, &(entry->glyph)))
    {
    return (cache_entry_t*)0;
    }
#ifdef FT_STROKER_H
  /* Stroke glyph */
  entry->glyph_stroke = entry->glyph;
  FT_Glyph_StrokeBorder(&(entry->glyph_stroke), r->stroker, 0, 0);
  //  FT_Glyph_StrokeBorder(&(entry->glyph_stroke), r->stroker, 1, 0);
#endif
  
  /* Render glyph */
  if(FT_Glyph_To_Bitmap( &(entry->glyph),
                         FT_RENDER_MODE_NORMAL,
                         (FT_Vector*)0, 1 ))
    return (cache_entry_t*)0;

#ifdef FT_STROKER_H
  if(FT_Glyph_To_Bitmap( &(entry->glyph_stroke),
                         FT_RENDER_MODE_NORMAL,
                         (FT_Vector*)0, 1 ))
    return (cache_entry_t*)0;
#endif

  /* Get bounding box and advances */

#ifdef FT_STROKER_H
  bitmap_glyph = (FT_BitmapGlyph)(entry->glyph_stroke);
#else
  bitmap_glyph = (FT_BitmapGlyph)(entry->glyph);
#endif
  
  entry->advance_x = entry->glyph->advance.x>>16;
  entry->advance_y = entry->glyph->advance.y>>16;

  if(r->fixed_width)
    {
    entry->bbox.xmin = 0;
    entry->bbox.xmax = entry->advance_x;
    }
  else
    {
    entry->bbox.xmin = bitmap_glyph->left;
    entry->bbox.xmax = entry->bbox.xmin + bitmap_glyph->bitmap.width;
    }
  entry->bbox.ymin = -bitmap_glyph->top;
  entry->bbox.ymax = entry->bbox.ymin + bitmap_glyph->bitmap.rows;

    
  entry->unicode = unicode;
  return entry;
  }

static void unload_font(bg_text_renderer_t * r)
  {
  if(!r->font_loaded)
    return;

#ifdef FT_STROKER_H
  FT_Stroker_Done(r->stroker);
#endif

  clear_glyph_cache(r);
  FT_Done_Face(r->face);
  r->face = (FT_Face)0;
  r->font_loaded = 0;
  }

/* fontconfig stuff inspired from MPlayer code */

static int load_font(bg_text_renderer_t * r)
  {
  int err;
  FcPattern *fc_pattern = (FcPattern *)0;
  FcPattern *fc_pattern_1 = (FcPattern *)0;
  FcChar8 *filename;
  FcBool scalable;
  float sar, font_size_scaled;
  
  unload_font(r);

  if(r->font)
    {
    /* Get font file */
    FcInit();
    fc_pattern = FcNameParse((const FcChar8*)r->font);
    
    //  FcPatternPrint(fc_pattern);
    
    FcConfigSubstitute(0, fc_pattern, FcMatchPattern);
    
    //  FcPatternPrint(fc_pattern);
    
    FcDefaultSubstitute(fc_pattern);
    
    //  FcPatternPrint(fc_pattern);
    
    fc_pattern_1 = FcFontMatch((FcConfig*)0, fc_pattern, 0);
    
    //  FcPatternPrint(fc_pattern);
    
    FcPatternGetBool(fc_pattern_1, FC_SCALABLE, 0, &scalable);
    if(scalable != FcTrue)
      {
      FcPatternDestroy(fc_pattern);
      FcPatternDestroy(fc_pattern_1);
      
      fc_pattern = FcNameParse((const FcChar8*)"Sans-20");
      FcConfigSubstitute(0, fc_pattern, FcMatchPattern);
      FcDefaultSubstitute(fc_pattern);
      fc_pattern_1 = FcFontMatch(0, fc_pattern, 0);
      }
    // s doesn't need to be freed according to fontconfig docs
    
    FcPatternGetString(fc_pattern_1, FC_FILE, 0, &filename);
    FcPatternGetDouble(fc_pattern_1, FC_SIZE, 0, &(r->font_size));
    }
  else
    {
    filename = (FcChar8 *)r->font_file;
    }
  
  
  /* Load face */
  
  err = FT_New_Face(r->library, (char*)filename, 0, &r->face);

  
  if(err)
    {
    if(r->font)
      {
      FcPatternDestroy(fc_pattern);
      FcPatternDestroy(fc_pattern_1);
      }
    return 0;
    }

  r->fixed_width = FT_IS_FIXED_WIDTH(r->face);
  
  if(r->font)
    {
    FcPatternDestroy(fc_pattern);
    FcPatternDestroy(fc_pattern_1);
    }
  
  /* Select Unicode */

  FT_Select_Charmap(r->face, FT_ENCODING_UNICODE );

#ifdef FT_STROKER_H
  /* Create stroker */

#if (FREETYPE_MAJOR > 2) || ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR > 1))  
  FT_Stroker_New(r->library, &(r->stroker));
#else
  FT_Stroker_New(r->face->memory, &(r->stroker));
#endif

  FT_Stroker_Set(r->stroker, (int)(r->border_width * 32.0 + 0.5), 
                 FT_STROKER_LINECAP_ROUND, 
                 FT_STROKER_LINEJOIN_ROUND, 0); 
#endif
  
  sar = (float)(r->frame_format.pixel_width) /
    (float)(r->frame_format.pixel_height);

  font_size_scaled = r->font_size * sar * (float)(r->frame_format.image_width) / 640.0;
  
  
  err = FT_Set_Char_Size(r->face,                     /* handle to face object           */
                         0,                           /* char_width in 1/64th of points  */
                         (int)(font_size_scaled*64.0+0.5),/* char_height in 1/64th of points */
                         (int)(72.0/sar+0.5),         /* horizontal device resolution    */
                         72 );                        /* vertical device resolution      */
  
  clear_glyph_cache(r);
  r->font_loaded = 1;
  return 1;
  }

bg_text_renderer_t * bg_text_renderer_create()
  {
  bg_text_renderer_t * ret = calloc(1, sizeof(*ret));
#ifndef WORDS_BIGENDIAN
  ret->cnv = bg_charset_converter_create("UTF-8", "UCS-4LE");
#else
  ret->cnv = bg_charset_converter_create("UTF-8", "UCS-4BE");
#endif
  pthread_mutex_init(&(ret->config_mutex),(pthread_mutexattr_t *)0);
  /* Initialize freetype */
  FT_Init_FreeType(&ret->library);

  return ret;
  }

void bg_text_renderer_destroy(bg_text_renderer_t * r)
  {
  bg_charset_converter_destroy(r->cnv);

  unload_font(r);
    
  if(r->cache)
    free(r->cache);

  if(r->font)
    free(r->font);
  if(r->font_file)
    free(r->font_file);
  
  FT_Done_FreeType(r->library);
  pthread_mutex_destroy(&(r->config_mutex));
  free(r);
  }

const bg_parameter_info_t * bg_text_renderer_get_parameters()
  {
  return parameters;
  }

void bg_text_renderer_set_parameter(void * data, const char * name,
                                    const bg_parameter_value_t * val)
  {
  bg_text_renderer_t * r;
  r = (bg_text_renderer_t *)data;

  if(!name)
    return;

  pthread_mutex_lock(&(r->config_mutex));

  /* General text renderer */
  if(!strcmp(name, "font"))
    {
    if(!r->font || strcmp(val->val_str, r->font))
      {
      r->font = bg_strdup(r->font, val->val_str);
      r->font_changed = 1;
      }
    }
  /* OSD */
  else if(!strcmp(name, "font_file"))
    {
    if(!r->font_file || strcmp(val->val_str, r->font_file))
      {
      r->font_file = bg_strdup(r->font_file, val->val_str);
      r->font_changed = 1;
      }
    }
  else if(!strcmp(name, "font_size"))
    {
    if(r->font_size != val->val_f)
      {
      r->font_size = val->val_f;
      r->font_changed = 1;
      }
    }
  
  /* */
  else if(!strcmp(name, "color"))
    {
    r->color_config[0] = val->val_color[0];
    r->color_config[1] = val->val_color[1];
    r->color_config[2] = val->val_color[2];
    r->color_config[3] = 0.0;
    r->alpha_f  = val->val_color[3];
    }
#ifdef FT_STROKER_H
  else if(!strcmp(name, "border_color"))
    {
    r->color_stroke[0] = val->val_color[0];
    r->color_stroke[1] = val->val_color[1];
    r->color_stroke[2] = val->val_color[2];
    r->color_stroke[3] = 0.0;
    }
  else if(!strcmp(name, "border_width"))
    {
    r->border_width = val->val_f;
    }
#endif
  else if(!strcmp(name, "cache_size"))
    {
    alloc_glyph_cache(r, val->val_i);
    }
  else if(!strcmp(name, "justify_h"))
    {
    if(!strcmp(val->val_str, "left"))
      r->justify_h = JUSTIFY_LEFT;
    else if(!strcmp(val->val_str, "right"))
      r->justify_h = JUSTIFY_RIGHT;
    else if(!strcmp(val->val_str, "center"))
      r->justify_h = JUSTIFY_CENTER;
    }
  else if(!strcmp(name, "justify_v"))
    {
    if(!strcmp(val->val_str, "top"))
      r->justify_v = JUSTIFY_TOP;
    else if(!strcmp(val->val_str, "bottom"))
      r->justify_v = JUSTIFY_BOTTOM;
    else if(!strcmp(val->val_str, "center"))
      r->justify_v = JUSTIFY_CENTER;
    }
  else if(!strcmp(name, "border_left"))
    {
    r->border_left = val->val_i;
    }
  else if(!strcmp(name, "border_right"))
    {
    r->border_right = val->val_i;
    }
  else if(!strcmp(name, "border_top"))
    {
    r->border_top = val->val_i;
    }
  else if(!strcmp(name, "border_bottom"))
    {
    r->border_bottom = val->val_i;
    }
  else if(!strcmp(name, "ignore_linebreaks"))
    {
    r->ignore_linebreaks = val->val_i;
    }
  else if(!strcmp(name, "default_width"))
    {
    r->default_width = val->val_i;
    }
  else if(!strcmp(name, "default_height"))
    {
    r->default_height = val->val_i;
    }
  else if(!strcmp(name, "default_framerate"))
    {
    r->default_framerate = val->val_f;
    }
  else if(!strcmp(name, "default_csp"))
    {
    if(!strcmp(val->val_str, "rgb"))
      r->default_csp = GAVL_RGB_24;
    else
      r->default_csp = GAVL_YUV_444_P;
    }
  r->config_changed = 1;
  pthread_mutex_unlock(&(r->config_mutex));
  }

/* Copied from gavl */

#define r_float_to_y  0.29900
#define g_float_to_y  0.58700
#define b_float_to_y  0.11400

#define r_float_to_u  (-0.16874)
#define g_float_to_u  (-0.33126)
#define b_float_to_u   0.50000

#define r_float_to_v   0.50000
#define g_float_to_v  (-0.41869)
#define b_float_to_v  (-0.08131)

#define Y_FLOAT_TO_8(val) (int)(val * 219.0+0.5) + 16;
#define UV_FLOAT_TO_8(val) (int)(val * 224.0+0.5) + 128;

#define GRAY_FLOAT_TO_8(val) (int)(val * 255.0+0.5);
#define GRAY_FLOAT_TO_16(val) (int)(val * 65535.0+0.5);

#define Y_FLOAT_TO_16(val,dst)  dst=(int)(val * 219.0 * (float)0x100+0.5) + 0x1000;
#define UV_FLOAT_TO_16(val,dst) dst=(int)(val * 224.0 * (float)0x100+0.5) + 0x8000;

#define RGB_FLOAT_TO_Y_8(r, g, b, y)                              \
  y_tmp = r_float_to_y * r + g_float_to_y * g + b_float_to_y * b; \
  y = Y_FLOAT_TO_8(y_tmp);

#define RGB_FLOAT_TO_YUV_8(r, g, b, y, u, v)                      \
  RGB_FLOAT_TO_Y_8(r, g, b, y)                                    \
  u_tmp = r_float_to_u * r + g_float_to_u * g + b_float_to_u * b; \
  v_tmp = r_float_to_v * r + g_float_to_v * g + b_float_to_v * b; \
  u = UV_FLOAT_TO_8(u_tmp);                                        \
  v = UV_FLOAT_TO_8(v_tmp);

#define RGB_FLOAT_TO_Y_16(r, g, b, y)                              \
  y_tmp = r_float_to_y * r + g_float_to_y * g + b_float_to_y * b; \
  Y_FLOAT_TO_16(y_tmp, y);

#define RGB_FLOAT_TO_YUV_16(r, g, b, y, u, v)                      \
  RGB_FLOAT_TO_Y_16(r, g, b, y)                                    \
  u_tmp = r_float_to_u * r + g_float_to_u * g + b_float_to_u * b; \
  v_tmp = r_float_to_v * r + g_float_to_v * g + b_float_to_v * b; \
  UV_FLOAT_TO_16(u_tmp, u);                                        \
  UV_FLOAT_TO_16(v_tmp, v);

#define RGB_FLOAT_TO_YUV_FLOAT(r, g, b, y, u, v)                      \
  y = r_float_to_y * r + g_float_to_y * g + b_float_to_y * b; \
  u = r_float_to_u * r + g_float_to_u * g + b_float_to_u * b; \
  v = r_float_to_v * r + g_float_to_v * g + b_float_to_v * b;

#define RGB_FLOAT_TO_Y_FLOAT(r, g, b, y, u, v)                      \
  y = r_float_to_y * r + g_float_to_y * g + b_float_to_y * b;

#define RGB_FLOAT_TO_GRAY_16(r, g, b, y)                              \
  y_tmp = r_float_to_y * r + g_float_to_y * g + b_float_to_y * b; \
  y = GRAY_FLOAT_TO_16(y_tmp);

#define RGB_FLOAT_TO_GRAY_8(r, g, b, y)                              \
  y_tmp = r_float_to_y * r + g_float_to_y * g + b_float_to_y * b; \
  y = GRAY_FLOAT_TO_8(y_tmp);

#define RGB_FLOAT_TO_GRAY_FLOAT(r, g, b, y)                              \
  y = r_float_to_y * r + g_float_to_y * g + b_float_to_y * b;

static
void init_nolock(bg_text_renderer_t * r)
  {
  int bits;
#ifdef FT_STROKER_H 
  float y_tmp, u_tmp, v_tmp;
#endif  

  if((r->frame_format.image_width  != r->last_frame_format.image_width) ||
     (r->frame_format.image_height != r->last_frame_format.image_height) ||
     !r->last_frame_format.pixel_width || !r->frame_format.pixel_height ||
     (r->frame_format.pixel_width  * r->last_frame_format.pixel_height !=
      r->frame_format.pixel_height * r->last_frame_format.pixel_width))
    r->font_changed = 1;

  gavl_video_format_copy(&r->last_frame_format, &r->frame_format);
  
  
  /* Load font if necessary */
  if(r->font_changed || !r->face)
    load_font(r);
  
  /* Copy formats */

  gavl_video_format_copy(&r->overlay_format, &r->frame_format);
  
  /* Decide about overlay format */

  gavl_pixelformat_chroma_sub(r->frame_format.pixelformat,
                              &r->sub_h, &r->sub_v);
  
  if(gavl_pixelformat_is_rgb(r->frame_format.pixelformat))
    {
    bits = 8*gavl_pixelformat_bytes_per_pixel(r->frame_format.pixelformat);
    if(bits <= 32)
      {
      r->overlay_format.pixelformat = GAVL_RGBA_32;
      r->alpha_i = (int)(r->alpha_f*255.0+0.5);
#ifdef FT_STROKER_H 
      r->color_i[0] = (int)(r->color_config[0]*255.0+0.5);
      r->color_i[1] = (int)(r->color_config[1]*255.0+0.5);
      r->color_i[2] = (int)(r->color_config[2]*255.0+0.5);
      r->color_i[3] = (int)(r->color_config[3]*255.0+0.5);
#endif
      r->render_func = render_rgba_32;
      }
    else if(bits <= 64)
      {
      r->overlay_format.pixelformat = GAVL_RGBA_64;
      r->alpha_i = (int)(r->alpha_f*65535.0+0.5);
#ifdef FT_STROKER_H 
      r->color_i[0] = (int)(r->color_config[0]*65535.0+0.5);
      r->color_i[1] = (int)(r->color_config[1]*65535.0+0.5);
      r->color_i[2] = (int)(r->color_config[2]*65535.0+0.5);
      r->color_i[3] = (int)(r->color_config[3]*65535.0+0.5);
#endif
      r->render_func = render_rgba_64;
      }
    else
      {
      r->overlay_format.pixelformat = GAVL_RGBA_FLOAT;
      r->render_func = render_rgba_float;
      r->color[0] = r->color_config[0];
      r->color[1] = r->color_config[1];
      r->color[2] = r->color_config[2];
      }
    }
  else if(gavl_pixelformat_is_yuv(r->frame_format.pixelformat))
    {
    if(gavl_pixelformat_is_planar(r->frame_format.pixelformat))
      {
      switch(gavl_pixelformat_bytes_per_component(r->frame_format.pixelformat))
        {
        case 1:
          r->overlay_format.pixelformat = GAVL_YUVA_32;
          r->alpha_i = (int)(r->alpha_f*255.0+0.5);
#ifdef FT_STROKER_H 
          RGB_FLOAT_TO_YUV_8(r->color_config[0],
                             r->color_config[1],
                             r->color_config[2],
                             r->color_i[0],
                             r->color_i[1],
                             r->color_i[2]);
          r->color_i[3] = (int)(r->color[3]*255.0+0.5);
#endif
          r->render_func = render_rgba_32;
          break; 
        case 2:
          r->overlay_format.pixelformat = GAVL_YUVA_64;
          r->alpha_i = (int)(r->alpha_f*65535.0+0.5);
#ifdef FT_STROKER_H 
          RGB_FLOAT_TO_YUV_16(r->color_config[0],
                              r->color_config[1],
                              r->color_config[2],
                              r->color_i[0],
                              r->color_i[1],
                              r->color_i[2]);
          r->color_i[3] = (int)(r->color[3]*65535.0+0.5);
#endif
          r->render_func = render_rgba_64;
          break; 
        }
      }
    else /* Packed */
      {
      switch(gavl_pixelformat_bytes_per_pixel(r->frame_format.pixelformat))
        {
        case 2: /* YUY2, UYVY */
        case 4: /* YUVA32 */
          r->overlay_format.pixelformat = GAVL_YUVA_32;
          r->alpha_i = (int)(r->alpha_f*255.0+0.5);
#ifdef FT_STROKER_H 
          RGB_FLOAT_TO_YUV_8(r->color_config[0],
                             r->color_config[1],
                             r->color_config[2],
                             r->color_i[0],
                             r->color_i[1],
                             r->color_i[2]);
          r->color_i[3] = (int)(r->color[3]*255.0+0.5);
#endif
          r->render_func = render_rgba_32;
          break; 
        case 8: /* YUVA 64 */
          r->overlay_format.pixelformat = GAVL_YUVA_64;
          r->alpha_i = (int)(r->alpha_f*65535.0+0.5);
#ifdef FT_STROKER_H 
          RGB_FLOAT_TO_YUV_16(r->color_config[0],
                              r->color_config[1],
                              r->color_config[2],
                              r->color_i[0],
                              r->color_i[1],
                              r->color_i[2]);
          r->color_i[3] = (int)(r->color[3]*65535.0+0.5);
#endif
          r->render_func = render_rgba_64;
          break; 
        case 3*sizeof(float): /* YUV float */
        case 4*sizeof(float): /* YUVA float */
          r->overlay_format.pixelformat = GAVL_YUVA_FLOAT;
#ifdef FT_STROKER_H 
          RGB_FLOAT_TO_YUV_FLOAT(r->color_config[0],
                                 r->color_config[1],
                                 r->color_config[2],
                                 r->color[0],
                                 r->color[1],
                                 r->color[2]);
#endif
          r->render_func = render_rgba_float;
          break; 
        }
      }
    }
  else // Gray
    {
    switch(r->frame_format.pixelformat)
      {
      case GAVL_GRAY_8:
      case GAVL_GRAYA_16:
        r->overlay_format.pixelformat = GAVL_GRAYA_16;
        r->alpha_i = (int)(r->alpha_f*255.0+0.5);
#ifdef FT_STROKER_H 
        RGB_FLOAT_TO_GRAY_8(r->color_config[0],
                            r->color_config[1],
                            r->color_config[2],
                            r->color_i[0]);
#endif
        r->render_func = render_graya_16;
        break;
      case GAVL_GRAY_16:
      case GAVL_GRAYA_32:
        r->overlay_format.pixelformat = GAVL_GRAYA_32;
        r->alpha_i = (int)(r->alpha_f*65535.0+0.5);
#ifdef FT_STROKER_H 
        RGB_FLOAT_TO_GRAY_16(r->color_config[0],
                             r->color_config[1],
                             r->color_config[2],
                             r->color_i[0]);
#endif
        r->render_func = render_graya_32;
        break;
      case GAVL_GRAY_FLOAT:
      case GAVL_GRAYA_FLOAT:
        r->overlay_format.pixelformat = GAVL_GRAYA_FLOAT;
        r->render_func = render_graya_float;
        RGB_FLOAT_TO_GRAY_FLOAT(r->color_config[0],
                                r->color_config[1],
                                r->color_config[2],
                                r->color[0]);
        break;
      default:
        break;
      }
    }
  
  /* */
  
  gavl_rectangle_i_set_all(&(r->max_bbox), &(r->overlay_format));

  gavl_rectangle_i_crop_left(&(r->max_bbox), r->border_left);
  gavl_rectangle_i_crop_right(&(r->max_bbox), r->border_right);
  gavl_rectangle_i_crop_top(&(r->max_bbox), r->border_top);
  gavl_rectangle_i_crop_bottom(&(r->max_bbox), r->border_bottom);
  
  }

void bg_text_renderer_init(bg_text_renderer_t * r,
                           const gavl_video_format_t * frame_format,
                           gavl_video_format_t * overlay_format)
  {
  int timescale, frame_duration;
  
  pthread_mutex_lock(&r->config_mutex);

  timescale      = overlay_format->timescale;
  frame_duration = overlay_format->frame_duration;
    
  if(frame_format)
    {
    gavl_video_format_copy(&(r->frame_format), frame_format);
    }
  else
    {
    memset(&(r->frame_format), 0, sizeof(r->frame_format));
    r->frame_format.image_width  = r->default_width;
    r->frame_format.image_height = r->default_height;

    r->frame_format.frame_width  = r->default_width;
    r->frame_format.frame_height = r->default_height;

    r->frame_format.pixel_width = 1;
    r->frame_format.pixel_height = 1;
    r->frame_format.pixelformat = r->default_csp;
    r->frame_format.timescale = (int)(r->default_framerate * 1000 + 0.5);
    r->frame_format.frame_duration = 1000;
    
    }
  
  init_nolock(r);

  r->overlay_format.timescale      = timescale;
  r->overlay_format.frame_duration = frame_duration;

  gavl_video_format_copy(overlay_format, &(r->overlay_format));

  r->config_changed = 0;

  
  pthread_mutex_unlock(&r->config_mutex);
  }

void bg_text_renderer_get_frame_format(bg_text_renderer_t * r,
                                       gavl_video_format_t * frame_format)
  {
  gavl_video_format_copy(frame_format, &r->frame_format);
  }


static void flush_line(bg_text_renderer_t * r, gavl_video_frame_t * f,
                       cache_entry_t ** glyphs, int len, int * line_y)
  {
  int line_x, j;
  int line_width;

  if(!len)
    return;
  
  line_width = 0;
  line_width = -glyphs[0]->bbox.xmin;
  /* Compute the length of the line */
  for(j = 0; j < len-1; j++)
    {
    line_width += glyphs[j]->advance_x;

    /* Adjust the y-position, if the glyph extends beyond the top
       border of the frame. This should happen only for the first
       line */

    if(*line_y < -glyphs[j]->bbox.ymin)
      *line_y = -glyphs[j]->bbox.ymin;
    }
  line_width += glyphs[len - 1]->bbox.xmax;

  if(*line_y < -glyphs[len - 1]->bbox.ymin)
    *line_y = -glyphs[len - 1]->bbox.ymin;
  
  switch(r->justify_h)
    {
    case JUSTIFY_CENTER:
      line_x = (r->max_bbox.w - line_width) >> 1;
      break;
    case JUSTIFY_LEFT:
      line_x = 0;
      break;
    case JUSTIFY_RIGHT:
      line_x = r->max_bbox.w - line_width;
      break;
    }

  line_x -= glyphs[0]->bbox.xmin;
  
  for(j = 0; j < len; j++)
    {
    adjust_bbox(glyphs[j], line_x, *line_y, &r->bbox);
    r->render_func(r, glyphs[j], f, &line_x, line_y);
    }
  }

void bg_text_renderer_render(bg_text_renderer_t * r, const char * string,
                             gavl_overlay_t * ovl)
  {
  cache_entry_t ** glyphs = (cache_entry_t **)0;
  uint32_t * string_unicode = (uint32_t *)0;
  int len, i;
  int pos_x, pos_y;
  int line_start, line_end;
  int line_width, line_end_y;
  int line_offset;
  int break_word; /* Linebreak within a (long) word */
  pthread_mutex_lock(&r->config_mutex);
  
  if(r->config_changed)
    init_nolock(r);
  
    
  r->bbox.xmin = r->overlay_format.image_width;
  r->bbox.ymin = r->overlay_format.image_height;

  r->bbox.xmax = 0;
  r->bbox.ymax = 0;

#ifdef FT_STROKER_H
  gavl_video_frame_fill(ovl->frame, &(r->overlay_format), r->color_stroke);
#else
  gavl_video_frame_fill(ovl->frame, &(r->overlay_format), r->color);
#endif
  
  /* Convert string */

  string_unicode = (uint32_t*)bg_convert_string(r->cnv, string, -1, &len);
  len /= 4;
  
  line_offset = r->face->size->metrics.height >> 6;
  
  glyphs = malloc(len * sizeof(*glyphs));

  if(r->ignore_linebreaks)
    {
    for(i = 0; i < len; i++)
      {
      if(string_unicode[i] == '\n')
        string_unicode[i] = ' ';
      }
    }
  for(i = 0; i < len; i++)
    {
    glyphs[i] = get_glyph(r, string_unicode[i]);
    if(!glyphs[i])
      glyphs[i] = get_glyph(r, '?');
    }

  line_start = 0;
  line_end   = -1;
  break_word = 0;
  
  pos_x = 0;
  pos_y = r->face->size->metrics.ascender >> 6;
  line_width = 0;
  for(i = 0; i < len; i++)
    {
    if((string_unicode[i] == ' ') ||
       (string_unicode[i] == '\n') ||
       (i == len))
      {
      line_end = i;
      line_width = pos_x;
      line_end_y = pos_y;
      break_word = 0;
      }
    
    
    /* Linebreak */
    if((pos_x + (glyphs[i]->advance_x) > r->max_bbox.w) ||
       (string_unicode[i] == '\n'))
      {
      
      if(line_end < 0)
        {
        line_width = pos_x;
        line_end_y = pos_y;
        line_end = i;
        break_word = 1;
        }
      /* Render the line */
      
      flush_line(r, ovl->frame,
                 &glyphs[line_start], line_end - line_start,
                 &pos_y);
      
      pos_x -= line_width;
      
      pos_y += line_offset;
      
      if((pos_y + (r->face->size->metrics.descender >> 6)) > r->max_bbox.h)
        break;
      
      line_start = line_end;
      if(!break_word)
        line_start++;
      line_end = -1;
      }
    pos_x += glyphs[i]->advance_x;

    }
  if(len - line_start)
    {
    flush_line(r, ovl->frame,
               &glyphs[line_start], len - line_start,
               &pos_y);
    }
  
  ovl->ovl_rect.x = r->bbox.xmin;
  ovl->ovl_rect.y = r->bbox.ymin;

  ovl->ovl_rect.w = r->bbox.xmax - r->bbox.xmin;
  ovl->ovl_rect.h = r->bbox.ymax - r->bbox.ymin;

  /* Align to subsampling */
  ovl->ovl_rect.w += r->sub_h - (ovl->ovl_rect.w % r->sub_h);
  ovl->ovl_rect.h += r->sub_v - (ovl->ovl_rect.h % r->sub_v);
  
  switch(r->justify_h)
    {
    case JUSTIFY_LEFT:
    case JUSTIFY_CENTER:
      ovl->dst_x = ovl->ovl_rect.x + r->border_left;
      
      if(ovl->dst_x % r->sub_h)
        ovl->dst_x += r->sub_h - (ovl->dst_x % r->sub_h);
      
      break;
    case JUSTIFY_RIGHT:
      ovl->dst_x = r->overlay_format.image_width - r->border_right - ovl->ovl_rect.w;
      ovl->dst_x -= (ovl->dst_x % r->sub_h);
      break;
    }

  switch(r->justify_v)
    {
    case JUSTIFY_TOP:
      ovl->dst_y = r->border_top;
      ovl->dst_y += r->sub_v - (ovl->dst_y % r->sub_v);

      break;
    case JUSTIFY_CENTER:
      ovl->dst_y = r->border_top +
        ((r->overlay_format.image_height - ovl->ovl_rect.h)>>1);

      if(ovl->dst_y % r->sub_v)
        ovl->dst_y += r->sub_v - (ovl->dst_y % r->sub_v);

      break;
    case JUSTIFY_BOTTOM:
      ovl->dst_y = r->overlay_format.image_height - r->border_bottom - ovl->ovl_rect.h;
      ovl->dst_y -= (ovl->dst_y % r->sub_v);
      break;
    }
  if(glyphs)
    free(glyphs);
  if(string_unicode)
    free(string_unicode);

  pthread_mutex_unlock(&r->config_mutex);
  }

