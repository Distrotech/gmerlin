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
      val_default: { val_str: "Sans-10" }
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
  float color[4];

  /* Charset converter */

  bg_charset_converter_t * cnv;
  
  };

/* Inspired from MPlayer */

static int load_font(bg_text_renderer_t * r, const char * font_name)
  {
  int err;
  FcPattern *fc_pattern;
  FcChar8 *filename;
  FcBool scalable;

  if(r->font_loaded && !strcmp(r->font, font_name))
    return 1;
  
  /* Get font file */
  FcInit();
  fc_pattern = FcNameParse(font_name);
  FcConfigSubstitute(0, fc_pattern, FcMatchPattern);
  FcDefaultSubstitute(fc_pattern);
  fc_pattern = FcFontMatch(0, fc_pattern, 0);
  FcPatternGetBool(fc_pattern, FC_SCALABLE, 0, &scalable);
  if(scalable != FcTrue)
    {
    fc_pattern = FcNameParse("Sans-serif");
    FcConfigSubstitute(0, fc_pattern, FcMatchPattern);
    FcDefaultSubstitute(fc_pattern);
    fc_pattern = FcFontMatch(0, fc_pattern, 0);
    }
  // s doesn't need to be freed according to fontconfig docs
  FcPatternGetString(fc_pattern, FC_FILE, 0, &filename);
  free(fc_pattern);
  
  /* Load face */
  
  err = FT_New_Face(r->library, filename, 0, &r->face);
  if(err)
    return 0;

  
  
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
    }
  else if(!strcmp(name, "color"))
    {
    
    }
  
  }

void bg_text_renderer_init(bg_text_renderer_t * r,
                           const gavl_video_format_t * frame_format,
                           gavl_video_format_t * overlay_format)
  {
  int bits;
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
  
  
  }

void bg_text_renderer_render(bg_text_renderer_t * r, const char * string, gavl_overlay_t * ovl)
  {
  uint32_t * string_unicode;
  int len;
    
  /* Convert string */

  string_unicode = (uint32_t*)bg_convert_string(r->cnv,
                                                string, -1,
                                                &len);
  
  }

