/*****************************************************************
 
  textrenderer.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/* System includes */

#include <stdlib.h>

/* Freetype */

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

/* Fontconfig */

#include <fontconfig/fontconfig.h>

/* Gmerlin */

#include <parameter.h>
#include <textrenderer.h>
#include <utils.h>
#include <charset.h>

/* Text alignment */

#define JUSTIFY_CENTER 0
#define JUSTIFY_LEFT   1
#define JUSTIFY_RIGHT  2

#define JUSTIFY_TOP    1
#define JUSTIFY_BOTTOM 2

bg_parameter_info_t parameters[] =
  {
    {
      name:       "color",
      long_name:  "Text color",
      type:       BG_PARAMETER_COLOR_RGBA,
      val_default: { val_color: (float[]){ 1.0, 1.0, 1.0, 1.0 } },
    },
    {
      name:       "font",
      long_name:  "Font",
      type:       BG_PARAMETER_FONT,
      val_default: { val_str: "Sans-20" }
    },
    {
      name:       "justify_h",
      long_name:  "Horizontal justify",
      type:       BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "center" },
      multi_names:  (char*[]){ "center", "left", "right", (char*)0 },
      multi_labels: (char*[]){ "Center", "Left", "Right", (char*)0  },
            
    },
    {
      name:       "justify_v",
      long_name:  "Vertical justify",
      type:       BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "center" },
      multi_names:  (char*[]){ "center", "top", "bottom", (char*)0  },
      multi_labels: (char*[]){ "Center", "Top", "Bottom", (char*)0 },
    },
    {
      name:        "cache_size",
      long_name:   "Cache size",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 1     },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 255   },
      
      help_string: "Specify, how many different characters are cached for faster rendering. For European languages, this never needs to be larger than 255",
    },
    {
      name:        "border_left",
      long_name:   "Left border",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0     },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 10    },
      help_string: "Distance from the left text border to the image border",
    },
    {
      name:        "border_right",
      long_name:   "Left border",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0     },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 10    },
      help_string: "Distance from the right text border to the image border",
    },
    {
      name:        "border_top",
      long_name:   "Top border",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0     },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 10    },
      help_string: "Distance from the top text border to the image border",
    },
    {
      name:        "border_bottom",
      long_name:   "Bottom border",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0     },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 10    },
      help_string: "Distance from the bottom text border to the image border",
    },
    {
      name:        "ignore_linebreaks",
      long_name:   "Ignore linebreaks",
      type:        BG_PARAMETER_CHECKBUTTON,
      help_string: "Ignore linebreaks in subtitles."
    },
    { /* End of parameters */ },
  };

typedef struct
  {
  int xmin, xmax, ymin, ymax;
  } bbox_t;

struct bg_text_renderer_s
  {
  gavl_rectangle_i_t max_bbox; /* Maximum bounding box the text may have */
  
  FT_Library library;
  FT_Face    face;
  
  int font_loaded;
  
  /* Configuration stuff */

  char * font;
  char * last_font;
  double font_size;
  
  float color[4];

  float alpha_f;
  int   alpha_i;
  
  /* Charset converter */

  bg_charset_converter_t * cnv;

  /* Glyph cache */

  struct
    {
    uint32_t unicode;
    FT_Glyph glyph;
    } * cache;

  FT_Matrix matrix; /* For scaling to pixel aspect ratio */
  FT_Vector delta;
  
  int cache_size;
  int cache_alloc;
  gavl_video_format_t overlay_format;

  int justify_h;
  int justify_v;
  int border_left, border_right, border_top, border_bottom;
  
  bbox_t bbox;
  int ignore_linebreaks;
  
  void (*render_func)(bg_text_renderer_t * r, FT_BitmapGlyph glyph,
                      gavl_video_frame_t * frame,
                      int * dst_x, int * dst_y);
  };

static void adjust_bbox(FT_BitmapGlyph glyph, int dst_x, int dst_y, bbox_t * ret)
  {
  int x, y;
  
  x = dst_x + glyph->left;
  y = dst_y - glyph->top;
  
  if(ret->xmin > x)
    ret->xmin = x;

  if(ret->ymin > y)
    ret->ymin = y;

  if(ret->xmax < x + glyph->bitmap.width)
    ret->xmax = x + glyph->bitmap.width;

  if(ret->ymax < y + glyph->bitmap.rows)
    ret->ymax = y + glyph->bitmap.rows;
  }

static void render_rgba_32(bg_text_renderer_t * r, FT_BitmapGlyph glyph,
                           gavl_video_frame_t * frame,
                           int * dst_x, int * dst_y)
  {
  uint8_t * src_ptr, * dst_ptr, * src_ptr_start, * dst_ptr_start;
  int i, j;
#if 0
  fprintf(stderr, "render_rgba32, alpha: %d\n", r->alpha_i);
  //  fprintf(stderr, "Left: %d Top: %d\n", glyph->left, glyph->top);

  //  fprintf(stderr, "Bitmap: Rows: %d, width: %d, pitch: %d, num_grays: %d\n",
  //          glyph->bitmap.rows, glyph->bitmap.width, glyph->bitmap.pitch,
  //          glyph->bitmap.num_grays);
#endif

  if(!glyph->bitmap.buffer)
    {
    // fprintf(stderr, "Bitmap missing\n");
    *dst_x += glyph->root.advance.x>>16;
    *dst_y += glyph->root.advance.y>>16;
    return;
    }
  
  src_ptr_start = glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - glyph->top) *
    frame->strides[0] + (*dst_x + glyph->left) * 4;
  
  for(i = 0; i < glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < glyph->bitmap.width; j++)
      {
      dst_ptr[3] = ((int)*src_ptr * (int)r->alpha_i) >> 8;
      //      fprintf(stderr, "color: %02x %02x %02x %02x\n",
      //              dst_ptr[0], dst_ptr[1], dst_ptr[2], dst_ptr[3]);
      src_ptr++;
      dst_ptr += 4;
      }
    src_ptr_start += glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  *dst_x += glyph->root.advance.x>>16;
  *dst_y += glyph->root.advance.y>>16;
  }

static void render_rgba_64(bg_text_renderer_t * r, FT_BitmapGlyph glyph,
                           gavl_video_frame_t * frame,
                           int * dst_x, int * dst_y)
  {
  uint8_t * src_ptr;
  uint16_t * dst_ptr;
  uint8_t * src_ptr_start, * dst_ptr_start;
  int i, j;

  if(!glyph->bitmap.buffer)
    {
    // fprintf(stderr, "Bitmap missing\n");
    *dst_x += glyph->root.advance.x>>16;
    *dst_y += glyph->root.advance.y>>16;
    return;
    }

  src_ptr_start = glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - glyph->top) *
    frame->strides[0] + (*dst_x + glyph->left) * 8;
  
  for(i = 0; i < glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = (uint16_t*)dst_ptr_start;
    
    for(j = 0; j < glyph->bitmap.width; j++)
      {
      dst_ptr[3] = ((int)*src_ptr * (int)r->alpha_i) >> 8;
      src_ptr++;
      dst_ptr += 4;
      }
    src_ptr_start += glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  *dst_x += glyph->root.advance.x>>16;
  *dst_y += glyph->root.advance.y>>16;
  }

static void render_rgba_float(bg_text_renderer_t * r, FT_BitmapGlyph glyph,
                              gavl_video_frame_t * frame,
                              int * dst_x, int * dst_y)
  {
  uint8_t * src_ptr;
  float * dst_ptr;
  uint8_t * src_ptr_start, * dst_ptr_start;
  int i, j;

  if(!glyph->bitmap.buffer)
    {
    // fprintf(stderr, "Bitmap missing\n");
    *dst_x += glyph->root.advance.x>>16;
    *dst_y += glyph->root.advance.y>>16;
    return;
    }

  src_ptr_start = glyph->bitmap.buffer;
  dst_ptr_start = frame->planes[0] + (*dst_y - glyph->top) *
    frame->strides[0] + (*dst_x + glyph->left) * 4 * sizeof(float);
  
  for(i = 0; i < glyph->bitmap.rows; i++)
    {
    src_ptr = src_ptr_start;
    dst_ptr = (float*)dst_ptr_start;
    
    for(j = 0; j < glyph->bitmap.width; j++)
      {
      dst_ptr[3] = (float)(*src_ptr)/255.0 * r->alpha_f;
      src_ptr++;
      dst_ptr += 4;
      }
    src_ptr_start += glyph->bitmap.pitch;
    dst_ptr_start += frame->strides[0];
    }
  *dst_x += glyph->root.advance.x>>16;
  *dst_y += glyph->root.advance.y>>16;
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
    FT_Done_Glyph(r->cache[i].glyph);
  r->cache_size = 0;
  }

static FT_Glyph get_glyph(bg_text_renderer_t * r, uint32_t unicode)
  {
  int i, index;
  for(i = 0; i < r->cache_size; i++)
    {
    if(r->cache[i].unicode == unicode)
      return r->cache[i].glyph;
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
  
  /* Load the glyph */
  if(FT_Load_Char(r->face, unicode, FT_LOAD_DEFAULT))
    {
    fprintf(stderr, "Cannot load character for code %d\n", unicode);
    return (FT_Glyph)0;
    }
  /* extract glyph image */
  if(FT_Get_Glyph(r->face->glyph, &(r->cache[index].glyph)))
    {
    fprintf(stderr, "Copying glyph failed\n");
    return (FT_Glyph)0;
    }
  /* Render glyph (we'll need it anyway) */
    
  if(FT_Glyph_To_Bitmap( &(r->cache[index].glyph),
                         FT_RENDER_MODE_NORMAL,
                         (FT_Vector*)0, 1 ))
    return (FT_Glyph)0;
  r->cache[index].unicode = unicode;
  return r->cache[index].glyph;
  }

/* fontconfig stuff inspired from MPlayer code */

static int load_font(bg_text_renderer_t * r, const char * font_name)
  {
  int err;
  FcPattern *fc_pattern, *fc_pattern_1;
  FcChar8 *filename;
  FcBool scalable;
    
  if(r->font_loaded && !strcmp(r->font, font_name))
    return 1;

  clear_glyph_cache(r);
  
  /* Get font file */
  FcInit();
  fc_pattern = FcNameParse(font_name);

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
    fprintf(stderr, "Font %s not scalable, using built in default\n",
            font_name);
    FcPatternDestroy(fc_pattern);
    FcPatternDestroy(fc_pattern_1);
    
    fc_pattern = FcNameParse("Sans-20");
    FcConfigSubstitute(0, fc_pattern, FcMatchPattern);
    FcDefaultSubstitute(fc_pattern);
    fc_pattern_1 = FcFontMatch(0, fc_pattern, 0);
    }
  // s doesn't need to be freed according to fontconfig docs
  FcPatternGetString(fc_pattern_1, FC_FILE, 0, &filename);
  FcPatternGetDouble(fc_pattern_1, FC_SIZE, 0, &(r->font_size));
  
  fprintf(stderr, "File: %s, Size: %f\n", filename, r->font_size);
  
  /* Load face */
  
  err = FT_New_Face(r->library, filename, 0, &r->face);
  if(err)
    {
    FcPatternDestroy(fc_pattern);
    FcPatternDestroy(fc_pattern_1);
    return 0;
    }
  FcPatternDestroy(fc_pattern);
  FcPatternDestroy(fc_pattern_1);

  /* Select Unicode */

  FT_Select_Charmap(r->face, FT_ENCODING_UNICODE );
  
  return 1;
  }

bg_text_renderer_t * bg_text_renderer_create()
  {
  bg_text_renderer_t * ret = calloc(1, sizeof(*ret));
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
  ret->cnv = bg_charset_converter_create("UTF-8", "UCS-4LE");
#else
  ret->cnv = bg_charset_converter_create("UTF-8", "UCS-4BE");
#endif
  
  /* Initialize freetype */
  FT_Init_FreeType(&ret->library);

  return ret;
  }

void bg_text_renderer_destroy(bg_text_renderer_t * r)
  {
  bg_charset_converter_destroy(r->cnv);

  if(r->cache)
    {
    clear_glyph_cache(r);
    free(r->cache);
    }
  FT_Done_FreeType(r->library);

  free(r);
  }

bg_parameter_info_t * bg_text_renderer_get_parameters(bg_text_renderer_t * r)
  {
  return parameters;
  }

void bg_text_renderer_set_parameter(void * data, char * name,
                                    bg_parameter_value_t * val)
  {
  bg_text_renderer_t * r;
  r = (bg_text_renderer_t *)data;

  if(!name)
    return;

  if(!strcmp(name, "font"))
    {
    if(!load_font(r, val->val_str))
      fprintf(stderr, "load_font failed\n");
    //    fprintf(stderr, "Loaded font %s\n", val->val_str);
    }
  else if(!strcmp(name, "color"))
    {
    r->color[0] = val->val_color[0];
    r->color[1] = val->val_color[1];
    r->color[2] = val->val_color[2];
    r->color[3] = 0.0;
    r->alpha_f  = val->val_color[3];
    }
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
  }

void bg_text_renderer_init(bg_text_renderer_t * r,
                           const gavl_video_format_t * frame_format,
                           gavl_video_format_t * overlay_format)
  {
  float sar;
  int bits, err;
  
  sar = (float)(frame_format->pixel_width) / (float)(frame_format->pixel_height);
  /* Copy formats */
  
  gavl_video_format_copy(overlay_format, frame_format);
  
  /* Decide about overlay format */

  if(gavl_pixelformat_is_rgb(frame_format->pixelformat))
    {
    bits = 8*gavl_pixelformat_bytes_per_pixel(frame_format->pixelformat);
    if(bits <= 32)
      {
      overlay_format->pixelformat = GAVL_RGBA_32;
      r->alpha_i = (int)(r->alpha_f*255.0+0.5);
      r->render_func = render_rgba_32;
      }
    else if(bits <= 64)
      {
      overlay_format->pixelformat = GAVL_RGBA_64;
      r->alpha_i = (int)(r->alpha_f*65535.0+0.5);
      r->render_func = render_rgba_64;
      }
    else
      {
      overlay_format->pixelformat = GAVL_RGBA_FLOAT;
      r->render_func = render_rgba_float;
      }
    }
  else
    {
    overlay_format->pixelformat = GAVL_YUVA_32;
    r->alpha_i = (int)(r->alpha_f*255.0+0.5);
    r->render_func = render_rgba_32;
    }

  /* */
  
  gavl_video_format_copy(&(r->overlay_format), overlay_format);
  gavl_rectangle_i_set_all(&(r->max_bbox), &(r->overlay_format));

  gavl_rectangle_i_crop_left(&(r->max_bbox), r->border_left);
  gavl_rectangle_i_crop_right(&(r->max_bbox), r->border_right);
  gavl_rectangle_i_crop_top(&(r->max_bbox), r->border_top);
  gavl_rectangle_i_crop_bottom(&(r->max_bbox), r->border_bottom);
  
  
  /* We assume, that the font is already loaded!! */
  clear_glyph_cache(r);
  err = FT_Set_Char_Size(r->face,                     /* handle to face object           */
                         0,                           /* char_width in 1/64th of points  */
                         (int)(r->font_size*64.0+0.5),/* char_height in 1/64th of points */
                         (int)(72.0/sar+0.5),         /* horizontal device resolution    */
                         72 );                        /* vertical device resolution      */
  if(err)
    return;
  }

static void flush_line(bg_text_renderer_t * r, gavl_video_frame_t * f,
                       FT_BitmapGlyph * glyphs, int len, int line_y)
  {
  int line_x, j;
  int line_width;
  line_width = 0;
  line_width = -glyphs[0]->left;
  //  fprintf(stderr, "Line width: %d\n", line_width);
  /* Compute the length of the line */
  for(j = 0; j < len-1; j++)
    {
    line_width += glyphs[j]->root.advance.x>>16;
    }
  line_width += glyphs[len - 1]->left + glyphs[len - 1]->bitmap.width;
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

  line_x -= glyphs[0]->left;
  //  fprintf(stderr, "Flush line %d %d\n", line_x, line_y);
  
  for(j = 0; j < len; j++)
    {
    adjust_bbox(glyphs[j], line_x, line_y, &r->bbox);
    r->render_func(r, glyphs[j], f, &line_x, &line_y);
    }
  }

void bg_text_renderer_render(bg_text_renderer_t * r, const char * string,
                             gavl_overlay_t * ovl)
  {
  FT_BitmapGlyph * glyphs = (FT_BitmapGlyph*)0;
  uint32_t * string_unicode = (uint32_t *)0;
  int len, i;
  int pos_x, pos_y;
  int line_start, line_end;
  int line_width, line_end_y;
  int line_offset;
  int newline;
  
  fprintf(stderr, "bg_text_renderer_render: string:\n\n%s\n\n", string);
    
  r->bbox.xmin = r->overlay_format.image_width;
  r->bbox.ymin = r->overlay_format.image_height;

  r->bbox.xmax = 0;
  r->bbox.ymax = 0;
  
  
  gavl_video_frame_fill(ovl->frame, &(r->overlay_format), r->color);

  
  /* Convert string */

  string_unicode = (uint32_t*)bg_convert_string(r->cnv, string, -1, &len);
  len /= 4;
  
  //  fprintf(stderr, "Len: %d\n", len);
  
  line_start = 0;
  line_end   = 0;

  line_offset = r->face->size->metrics.height >> 6;
  
  pos_x = 0;
  pos_y = r->face->size->metrics.ascender >> 6;

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
    glyphs[i] = (FT_BitmapGlyph)get_glyph(r, string_unicode[i]);
    }
  //  fprintf(stderr, "pos_y: %d\n", pos_y);

  for(i = 0; i < len; i++)
    {
    if((string_unicode[i] == ' ') ||
       (string_unicode[i] == '\n') ||
       (i == len))
      {
      line_end = i;
      line_width = pos_x;
      line_end_y = pos_y;
      }
    
    // fprintf(stderr, "Checking '%c', x: %d\n", string_unicode[i], pos_x);
    
    /* Linebreak */
    if((pos_x + (glyphs[i]->root.advance.x>>16) > r->max_bbox.w) ||
       (string_unicode[i] == '\n'))
      {
      //      fprintf(stderr, "Linebreak\n");
      
      /* Render the line */
      
      flush_line(r, ovl->frame,
                 &glyphs[line_start], line_end - line_start,
                 pos_y);
      
      pos_x -= line_width;
      
      pos_y += line_offset;
      
      if((pos_y + (r->face->size->metrics.descender >> 6)) > r->max_bbox.h)
        break;
      line_start = line_end+1;
      
      }
    
    pos_x += glyphs[i]->root.advance.x>>16;

    }
  if(len - line_start)
    {
    flush_line(r, ovl->frame,
               &glyphs[line_start], len - line_start,
               pos_y);
    }

  fprintf(stderr, "bounding_box: %d,%d -> %d,%d\n",
          r->bbox.xmin, r->bbox.ymin, r->bbox.xmax, r->bbox.ymax);
  
  ovl->ovl_rect.x = r->bbox.xmin;
  ovl->ovl_rect.y = r->bbox.ymin;

  ovl->ovl_rect.w = r->bbox.xmax - r->bbox.xmin;
  ovl->ovl_rect.h = r->bbox.ymax - r->bbox.ymin;

  switch(r->justify_h)
    {
    case JUSTIFY_LEFT:
    case JUSTIFY_CENTER:
      ovl->dst_x = ovl->ovl_rect.x + r->border_left;
      break;
    case JUSTIFY_RIGHT:
      ovl->dst_x = r->overlay_format.image_width - r->border_right - ovl->ovl_rect.w;
      break;
    }

  switch(r->justify_v)
    {
    case JUSTIFY_TOP:
      ovl->dst_y = r->border_top;
      break;
    case JUSTIFY_CENTER:
      ovl->dst_y = r->border_top +
        ((r->overlay_format.image_height - ovl->ovl_rect.h)>>1);
      break;
    case JUSTIFY_BOTTOM:
      ovl->dst_y = r->overlay_format.image_height - r->border_bottom - ovl->ovl_rect.h;
      break;
    }
  if(glyphs)
    free(glyphs);
  if(string_unicode)
    free(string_unicode);
  }

