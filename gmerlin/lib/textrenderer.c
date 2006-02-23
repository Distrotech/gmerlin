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
      name:        "cache_size",
      long_name:   "Cache size",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 1     },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 255   },
      
      help_string: "Specify, how many different characters are cached for faster rendering. For European languages, this never needs to be larger than 255",
    },
    { /* End of parameters */ },
  };

struct bg_text_renderer_s
  {
  FT_Library library;
  FT_Face    face;

  int font_loaded;
  
  /* Configuration stuff */

  char * font;
  char * last_font;
  double font_size;
  
  float color[4];

  /* Charset converter */

  bg_charset_converter_t * cnv;

  /* Glyph cache */

  struct
    {
    uint32_t charcode; /* Unicode */
    FT_Glyph glyph;
    } * cache;

  FT_Matrix matrix; /* For scaling to pixel aspect ratio */
  FT_Vector delta;
  
  int cache_size;
  int cache_alloc;
  };

static void alloc_glyph_cache(bg_text_renderer_t * r, int size)
  {
  int i;
  if(size > r->cache_alloc)
    {
    /* Enlarge */
    r->cache_alloc = size;
    r->cache = realloc(r->cache, r->cache_alloc * sizeof(*(r->cache)));
    }
  else if(size > r->cache_alloc)
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

static FT_Glyph get_glyph(bg_text_renderer_t * r, uint32_t charcode)
  {
  int i, index;
  for(i = 0; i < r->cache_size; i++)
    {
    if(r->cache[i].charcode == charcode)
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
  if(FT_Load_Char(r->face, charcode, FT_LOAD_DEFAULT))
    {
    fprintf(stderr, "Cannot load character for charcode %d\n", charcode);
    return (FT_Glyph)0;
    }
  /* extract glyph image */
  if(FT_Get_Glyph(r->face->glyph, &(r->cache[index].glyph)))
    {
    fprintf(stderr, "Copying glyph failed\n");
    return (FT_Glyph)0;
    }
  /* Render glyph (we'll need it anyway) */

  
  
  return r->cache[index].glyph;
  }

/* fontconfig stuff inspired from MPlayer code */

static int load_font(bg_text_renderer_t * r, const char * font_name)
  {
  int err;
  FcPattern *fc_pattern;
  FcChar8 *filename;
  FcBool scalable;

  clear_glyph_cache(r);
  
  if(r->font_loaded && !strcmp(r->font, font_name))
    return 1;
  
  /* Get font file */
  FcInit();
  fc_pattern = FcNameParse(font_name);

  FcPatternPrint(fc_pattern);
  
  FcConfigSubstitute(0, fc_pattern, FcMatchPattern);

  FcPatternPrint(fc_pattern);

  FcDefaultSubstitute(fc_pattern);

  FcPatternPrint(fc_pattern);

  
  fc_pattern = FcFontMatch((FcConfig*)0, fc_pattern, 0);

  FcPatternPrint(fc_pattern);

  FcPatternGetBool(fc_pattern, FC_SCALABLE, 0, &scalable);
  if(scalable != FcTrue)
    {
    fprintf(stderr, "Font %s not scalable, using built in default\n",
            font_name);
    fc_pattern = FcNameParse("Sans-20");
    FcConfigSubstitute(0, fc_pattern, FcMatchPattern);
    FcDefaultSubstitute(fc_pattern);
    fc_pattern = FcFontMatch(0, fc_pattern, 0);
    }
  // s doesn't need to be freed according to fontconfig docs
  FcPatternGetString(fc_pattern, FC_FILE, 0, &filename);
  FcPatternGetDouble(fc_pattern, FC_SIZE, 0, &(r->font_size));
  
  fprintf(stderr, "File: %s, Size: %f\n", filename, r->font_size);
  
  /* Load face */
  
  err = FT_New_Face(r->library, filename, 0, &r->face);
  if(err)
    {
    FcPatternDestroy(fc_pattern);
    return 0;
    }
  FcPatternDestroy(fc_pattern);

  /* Select UCS-4 */

  FT_Select_Charmap(r->face, FT_ENCODING_UNICODE );
  
  return 1;
  }

bg_text_renderer_t * bg_text_renderer_create()
  {
  bg_text_renderer_t * ret = calloc(1, sizeof(*ret));

  ret->cnv = bg_charset_converter_create("UTF-8", "UCS-4");
  
  /* Initialize freetype */
  FT_Init_FreeType(&ret->library);

  return ret;
  }

void bg_text_renderer_destroy(bg_text_renderer_t * r)
  {
  FT_Done_FreeType(r->library);
  bg_charset_converter_destroy(r->cnv);
  free(r);
  }

bg_parameter_info_t * bg_text_renderer_get_parameters(bg_text_renderer_t * r)
  {
  return parameters;
  }

void bg_text_renderer_set_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  bg_text_renderer_t * r;
  r = (bg_text_renderer_t *)data;

  if(!name)
    return;

  if(!strcmp(name, "font"))
    {
    if(!load_font(r, val->val_str))
      fprintf(stderr, "load_font failed\n");
    fprintf(stderr, "Loaded font %s\n", val->val_str);
    }
  else if(!strcmp(name, "color"))
    {
    
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
      overlay_format->pixelformat = GAVL_RGBA_32;
    else if(bits <= 64)
      overlay_format->pixelformat = GAVL_RGBA_64;
    else
      overlay_format->pixelformat = GAVL_RGBA_FLOAT;
    }
  else
    overlay_format->pixelformat = GAVL_YUVA_32;
  
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

void bg_text_renderer_render(bg_text_renderer_t * r, const char * string,
                             gavl_overlay_t * ovl)
  {
  uint32_t * string_unicode;
  int len, i;

  gavl_video_frame_fill(ovl->frame, &(r->overlay_format), r->color);
  
  /* Convert string */

  string_unicode = (uint32_t*)bg_convert_string(r->cnv, string, -1, &len);
#if 0
  while(1)
    {
    /* Get next character */

    
    
    }
#endif
  }

