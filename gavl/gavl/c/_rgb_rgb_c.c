/*****************************************************************

  _rgb_rgb_c.c

  Copyright (c) 2001 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include "../colorspace_macros.h"
#include <stdio.h>

/*
 * Macros for pixel conversion
 */

// Masks for BGR16 and RGB16 formats

#define RGB16_LOWER_MASK  0x001f
#define RGB16_MIDDLE_MASK 0x07e0
#define RGB16_UPPER_MASK  0xf800

// Extract unsigned char RGB values from 15 bit pixels

#define RGB16_TO_R(pixel) ((pixel & RGB16_UPPER_MASK)>>8)
#define RGB16_TO_G(pixel) ((pixel & RGB16_MIDDLE_MASK)>>3) 
#define RGB16_TO_B(pixel) ((pixel & RGB16_LOWER_MASK)<<3)

#define BGR16_TO_B(pixel) RGB16_TO_R(pixel)
#define BGR16_TO_G(pixel) RGB16_TO_G(pixel)
#define BGR16_TO_R(pixel) RGB16_TO_B(pixel)

// Masks for BGR16 and RGB16 formats

#define RGB15_LOWER_MASK  0x001f
#define RGB15_MIDDLE_MASK 0x03e0
#define RGB15_UPPER_MASK  0x7C00

// Extract unsigned char RGB values from 16 bit pixels

#define RGB15_TO_R(pixel) ((pixel & RGB15_UPPER_MASK)>>7)
#define RGB15_TO_G(pixel) ((pixel & RGB15_MIDDLE_MASK)>>2) 
#define RGB15_TO_B(pixel) ((pixel & RGB15_LOWER_MASK)<<3)

#define BGR15_TO_B(pixel) RGB15_TO_R(pixel)
#define BGR15_TO_G(pixel) RGB15_TO_G(pixel) 
#define BGR15_TO_R(pixel) RGB15_TO_B(pixel)

/* Combine r, g and b values to a 16 bit rgb pixel (taken from avifile) */

#define PACK_RGB16(r,g,b,pixel) pixel=((((((r<<5)&0xff00)|g)<<6)&0xfff00)|b)>>3;

#define PACK_BGR16(r,g,b,pixel) pixel=((((((b<<5)&0xff00)|g)<<6)&0xfff00)|r)>>3;

#define PACK_RGB15(r,g,b,pixel) pixel=((((((r<<5)&0xff00)|g)<<5)&0xfff00)|b)>>3;
#define PACK_BGR15(r,g,b,pixel) pixel=((((((b<<5)&0xff00)|g)<<5)&0xfff00)|r)>>3;

#define COPY_24 dst[0]=src[0];dst[1]=src[1];dst[2]=src[2];

#define SWAP_24 dst[2]=src[0];dst[1]=src[1];dst[0]=src[2];

#define INIT_RGBA32 int anti_alpha;\
                    int background_r=ctx->options->background_red>>8;\
                    int background_g=ctx->options->background_green>>8;\
                    int background_b=ctx->options->background_blue>>8;

#define RGBA_2_RGB(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b) \
anti_alpha = 0xFF - src_a;\
dst_r=((src_a*src_r)+(anti_alpha*background_r))>>8;\
dst_g=((src_a*src_g)+(anti_alpha*background_g))>>8;\
dst_b=((src_a*src_b)+(anti_alpha*background_b))>>8;

static void swap_rgb_24_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  SWAP_24
    
  SCANLINE_LOOP_END_PACKED_PACKED(3,3)
     
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void swap_rgb_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  SWAP_24

  SCANLINE_LOOP_END_PACKED_PACKED(4,4)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void swap_rgb_16_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  *dst = (((*src & RGB16_UPPER_MASK) >> 11)|\
           (*src & RGB16_MIDDLE_MASK)|\
          ((*src & RGB16_LOWER_MASK) << 11));
    
  SCANLINE_LOOP_END_PACKED_PACKED(1,1)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void swap_rgb_15_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  *dst = (((*src & RGB15_UPPER_MASK) >> 10)|\
          (*src & RGB15_MIDDLE_MASK)|\
         ((*src & RGB15_LOWER_MASK) << 10));

  SCANLINE_LOOP_END_PACKED_PACKED(1,1)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_15_to_16_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  *dst = *src + (*src & 0xFFE0);

  SCANLINE_LOOP_END_PACKED_PACKED(1,1)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_15_to_24_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  dst[0] = RGB15_TO_R(*src);
  dst[1] = RGB15_TO_G(*src);
  dst[2] = RGB15_TO_B(*src);
      
  SCANLINE_LOOP_END_PACKED_PACKED(1,3)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_15_to_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  dst[0] = RGB15_TO_R(*src);
  dst[1] = RGB15_TO_G(*src);
  dst[2] = RGB15_TO_B(*src);

  SCANLINE_LOOP_END_PACKED_PACKED(1,4)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_16_to_15_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint16_t, 1, 1) 
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

    *dst = (*src & RGB16_LOWER_MASK) |
    (((*src & (RGB16_MIDDLE_MASK|RGB16_UPPER_MASK)) >> 1) &
    (RGB15_UPPER_MASK | RGB15_MIDDLE_MASK));

  SCANLINE_LOOP_END_PACKED_PACKED(1,1)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_16_to_24_c(gavl_video_convert_context_t * ctx)
  {

  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  dst[0] = RGB16_TO_R(*src);
  dst[1] = RGB16_TO_G(*src);
  dst[2] = RGB16_TO_B(*src);
    
  SCANLINE_LOOP_END_PACKED_PACKED(1,3)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_16_to_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  dst[0] = RGB16_TO_R(*src);
  dst[1] = RGB16_TO_G(*src);
  dst[2] = RGB16_TO_B(*src);

  SCANLINE_LOOP_END_PACKED_PACKED(1,4)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }
  
static void rgb_24_to_15_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  PACK_RGB15(src[0],src[1],src[2],*dst)

  SCANLINE_LOOP_END_PACKED_PACKED(3,1)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_24_to_16_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  PACK_RGB16(src[0],src[1],src[2],*dst)

  SCANLINE_LOOP_END_PACKED_PACKED(3,1)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_24_to_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  COPY_24

  SCANLINE_LOOP_END_PACKED_PACKED(3,4)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }
  
static void rgb_32_to_15_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  PACK_RGB15(src[0],src[1],src[2],*dst)

  SCANLINE_LOOP_END_PACKED_PACKED(4,1)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_32_to_16_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  PACK_RGB16(src[0],src[1],src[2],*dst)
  
  SCANLINE_LOOP_END_PACKED_PACKED(4,1)
 
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_32_to_24_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  COPY_24  
    
  SCANLINE_LOOP_END_PACKED_PACKED(4,3)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_15_to_16_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  *dst = ((*src & RGB15_UPPER_MASK) >> 10) |
    ((*src & RGB15_MIDDLE_MASK) << 1) | 
    ((*src & RGB15_LOWER_MASK) << 11);
    
  SCANLINE_LOOP_END_PACKED_PACKED(1,1)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_15_to_24_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  dst[2] = RGB15_TO_R(*src);
  dst[1] = RGB15_TO_G(*src);
  dst[0] = RGB15_TO_B(*src);

  SCANLINE_LOOP_END_PACKED_PACKED(1,3)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_15_to_32_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  dst[2] = RGB15_TO_R(*src);
  dst[1] = RGB15_TO_G(*src);
  dst[0] = RGB15_TO_B(*src);

  SCANLINE_LOOP_END_PACKED_PACKED(1,4)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_16_to_15_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  *dst = ((*src & RGB16_UPPER_MASK) >> 11) |
    (((*src & RGB16_MIDDLE_MASK) >> 1) & RGB15_MIDDLE_MASK) | 
    ((*src & RGB16_LOWER_MASK) << 10);
  
  SCANLINE_LOOP_END_PACKED_PACKED(1,1)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_16_to_24_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  dst[2]=RGB16_TO_R(*src);
  dst[1]=RGB16_TO_G(*src);
  dst[0]=RGB16_TO_B(*src);
      
  SCANLINE_LOOP_END_PACKED_PACKED(1,3)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_16_to_32_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  dst[2]=RGB16_TO_R(*src);
  dst[1]=RGB16_TO_G(*src);
  dst[0]=RGB16_TO_B(*src);

  SCANLINE_LOOP_END_PACKED_PACKED(1,4)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_24_to_15_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  PACK_BGR15(src[0],src[1],src[2],*dst)

  SCANLINE_LOOP_END_PACKED_PACKED(3,1)

  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_24_to_16_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  PACK_BGR16(src[0],src[1],src[2],*dst)
    
  SCANLINE_LOOP_END_PACKED_PACKED(3,1)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_24_to_32_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  SWAP_24

  SCANLINE_LOOP_END_PACKED_PACKED(3,4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }
  
static void  rgb_32_to_15_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  PACK_BGR15(src[0],src[1],src[2],*dst)
    
  SCANLINE_LOOP_END_PACKED_PACKED(4,1)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_32_to_16_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  PACK_BGR16(src[0],src[1],src[2],*dst)
  
  SCANLINE_LOOP_END_PACKED_PACKED(4,1)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_32_to_24_swap_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)

  SCANLINE_LOOP_START_PACKED
  SWAP_24
  SCANLINE_LOOP_END_PACKED_PACKED(4,3)

  CONVERSION_FUNC_END_PACKED_PACKED
  }

/* Conversion from RGBA to RGB formats */

static void  rgba_32_to_rgb_15_c(gavl_video_convert_context_t * ctx)
  {
  uint8_t r_tmp;
  uint8_t g_tmp;
  uint8_t b_tmp;
  
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  INIT_RGBA32
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED
  RGBA_2_RGB(src[0], src[1], src[2], src[3], r_tmp, g_tmp, b_tmp)
  PACK_RGB15(r_tmp, g_tmp, b_tmp, *dst)
  SCANLINE_LOOP_END_PACKED_PACKED(4,1)

  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgba_32_to_bgr_15_c(gavl_video_convert_context_t * ctx)
  {
  uint8_t r_tmp;
  uint8_t g_tmp;
  uint8_t b_tmp;

  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  INIT_RGBA32
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED
  RGBA_2_RGB(src[0], src[1], src[2], src[3], r_tmp, g_tmp, b_tmp)
  PACK_BGR15(r_tmp, g_tmp, b_tmp, *dst)
  SCANLINE_LOOP_END_PACKED_PACKED(4,1)

  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgba_32_to_rgb_16_c(gavl_video_convert_context_t * ctx)
  {
  uint8_t r_tmp;
  uint8_t g_tmp;
  uint8_t b_tmp;

  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  INIT_RGBA32
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED
  RGBA_2_RGB(src[0], src[1], src[2], src[3], r_tmp, g_tmp, b_tmp)
  PACK_RGB16(r_tmp, g_tmp, b_tmp, *dst)

  SCANLINE_LOOP_END_PACKED_PACKED(4,1)

  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgba_32_to_bgr_16_c(gavl_video_convert_context_t * ctx)
  {
  uint8_t r_tmp;
  uint8_t g_tmp;
  uint8_t b_tmp;

  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 1, 1)
  INIT_RGBA32
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED
  RGBA_2_RGB(src[0], src[1], src[2], src[3], r_tmp, g_tmp, b_tmp)
  PACK_BGR16(r_tmp, g_tmp, b_tmp, *dst)

  SCANLINE_LOOP_END_PACKED_PACKED(4,1)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgba_32_to_rgb_24_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  INIT_RGBA32
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED
  RGBA_2_RGB(src[0], src[1], src[2], src[3], dst[0], dst[1], dst[2])

  SCANLINE_LOOP_END_PACKED_PACKED(4,3)
  CONVERSION_FUNC_END_PACKED_PACKED
  }
                                 
static void  rgba_32_to_bgr_24_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  INIT_RGBA32
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED
  RGBA_2_RGB(src[0], src[1], src[2], src[3], dst[2], dst[1], dst[0])

  SCANLINE_LOOP_END_PACKED_PACKED(4,3)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgba_32_to_rgb_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  INIT_RGBA32
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED
  RGBA_2_RGB(src[0], src[1], src[2], src[3], dst[0], dst[1], dst[2])

  SCANLINE_LOOP_END_PACKED_PACKED(4,4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgba_32_to_bgr_32_c(gavl_video_convert_context_t * ctx)
  {
  //  fprintf(stderr, "rgba_32_to_bgr_32_c\n");
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  INIT_RGBA32
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED
  RGBA_2_RGB(src[0], src[1], src[2], src[3], dst[2], dst[1], dst[0])

  SCANLINE_LOOP_END_PACKED_PACKED(4,4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

  /* Conversion from RGB formats to RGBA */

static void  rgb_15_to_rgba_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)
  SCANLINE_LOOP_START_PACKED

  dst[0] = RGB15_TO_R(*src);
  dst[1] = RGB15_TO_G(*src);
  dst[2] = RGB15_TO_B(*src);
  dst[3] = 0xff;
    
  SCANLINE_LOOP_END_PACKED_PACKED(1,4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  bgr_15_to_rgba_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)
  SCANLINE_LOOP_START_PACKED

  dst[0] = BGR15_TO_R(*src);
  dst[1] = BGR15_TO_G(*src);
  dst[2] = BGR15_TO_B(*src);
  dst[3] = 0xff;
    
  SCANLINE_LOOP_END_PACKED_PACKED(1,4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_16_to_rgba_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)

  SCANLINE_LOOP_START_PACKED

  dst[0] = RGB16_TO_R(*src);
  dst[1] = RGB16_TO_G(*src);
  dst[2] = RGB16_TO_B(*src);
  dst[3] = 0xff;
    
  SCANLINE_LOOP_END_PACKED_PACKED(1,4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  bgr_16_to_rgba_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)
  SCANLINE_LOOP_START_PACKED

  dst[0] = BGR16_TO_R(*src);
  dst[1] = BGR16_TO_G(*src);
  dst[2] = BGR16_TO_B(*src);
  dst[3] = 0xff;
    
  SCANLINE_LOOP_END_PACKED_PACKED(1,4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_24_to_rgba_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED

  COPY_24
  dst[3] = 0xff;
    
  SCANLINE_LOOP_END_PACKED_PACKED(3,4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  bgr_24_to_rgba_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED

  SWAP_24
  dst[3] = 0xff;

  SCANLINE_LOOP_END_PACKED_PACKED(3,4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  rgb_32_to_rgba_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED

  COPY_24
  dst[3] = 0xff;

  SCANLINE_LOOP_END_PACKED_PACKED(4,4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void  bgr_32_to_rgba_32_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 1, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED

  SWAP_24
  dst[3] = 0xff;
    
  SCANLINE_LOOP_END_PACKED_PACKED(4,4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

#ifdef SCANLINE
void gavl_init_rgb_rgb_scanline_funcs_c(gavl_colorspace_function_table_t * tab)
#else     
void gavl_init_rgb_rgb_funcs_c(gavl_colorspace_function_table_t * tab)
#endif
  {
  tab->swap_rgb_24 = swap_rgb_24_c;
  tab->swap_rgb_32 = swap_rgb_32_c;
  tab->swap_rgb_16 = swap_rgb_16_c;
  tab->swap_rgb_15 = swap_rgb_15_c;

  tab->rgb_15_to_16 = rgb_15_to_16_c;
  tab->rgb_15_to_24 = rgb_15_to_24_c;
  tab->rgb_15_to_32 = rgb_15_to_32_c;

  tab->rgb_16_to_15 = rgb_16_to_15_c;
  tab->rgb_16_to_24 = rgb_16_to_24_c;
  tab->rgb_16_to_32 = rgb_16_to_32_c;
  
  tab->rgb_24_to_15 = rgb_24_to_15_c;
  tab->rgb_24_to_16 = rgb_24_to_16_c;
  tab->rgb_24_to_32 = rgb_24_to_32_c;
  
  tab->rgb_32_to_15 = rgb_32_to_15_c;
  tab->rgb_32_to_16 = rgb_32_to_16_c;
  tab->rgb_32_to_24 = rgb_32_to_24_c;

  tab->rgb_15_to_16_swap = rgb_15_to_16_swap_c;
  tab->rgb_15_to_24_swap = rgb_15_to_24_swap_c;
  tab->rgb_15_to_32_swap = rgb_15_to_32_swap_c;

  tab->rgb_16_to_15_swap = rgb_16_to_15_swap_c;
  tab->rgb_16_to_24_swap = rgb_16_to_24_swap_c;
  tab->rgb_16_to_32_swap = rgb_16_to_32_swap_c;
  
  tab->rgb_24_to_15_swap = rgb_24_to_15_swap_c;
  tab->rgb_24_to_16_swap = rgb_24_to_16_swap_c;
  tab->rgb_24_to_32_swap = rgb_24_to_32_swap_c;
  
  tab->rgb_32_to_15_swap = rgb_32_to_15_swap_c;
  tab->rgb_32_to_16_swap = rgb_32_to_16_swap_c;
  tab->rgb_32_to_24_swap = rgb_32_to_24_swap_c;

  /* Conversion from RGBA to RGB formats */

  tab->rgba_32_to_rgb_15 = rgba_32_to_rgb_15_c;
  tab->rgba_32_to_bgr_15 = rgba_32_to_bgr_15_c;
  tab->rgba_32_to_rgb_16 = rgba_32_to_rgb_16_c;
  tab->rgba_32_to_bgr_16 = rgba_32_to_bgr_16_c;
  tab->rgba_32_to_rgb_24 = rgba_32_to_rgb_24_c;
  tab->rgba_32_to_bgr_24 = rgba_32_to_bgr_24_c;
  tab->rgba_32_to_rgb_32 = rgba_32_to_rgb_32_c;
  tab->rgba_32_to_bgr_32 = rgba_32_to_bgr_32_c;

  /* Conversion from RGB formats to RGBA */

  tab->rgb_15_to_rgba_32 = rgb_15_to_rgba_32_c;
  tab->bgr_15_to_rgba_32 = bgr_15_to_rgba_32_c;
  tab->rgb_16_to_rgba_32 = rgb_16_to_rgba_32_c;
  tab->bgr_16_to_rgba_32 = bgr_16_to_rgba_32_c;
  tab->rgb_24_to_rgba_32 = rgb_24_to_rgba_32_c;
  tab->bgr_24_to_rgba_32 = bgr_24_to_rgba_32_c;
  tab->rgb_32_to_rgba_32 = rgb_32_to_rgba_32_c;
  tab->bgr_32_to_rgba_32 = bgr_32_to_rgba_32_c;
  }


#undef RGB16_LOWER_MASK
#undef RGB16_MIDDLE_MASK
#undef RGB16_UPPER_MASK

// Extract unsigned char RGB values from 15 bit pixels

#undef RGB16_TO_R
#undef RGB16_TO_G 
#undef RGB16_TO_B

#undef BGR16_TO_B
#undef BGR16_TO_G
#undef BGR16_TO_R

// Masks for BGR16 and RGB16 formats

#undef RGB15_LOWER_MASK
#undef RGB15_MIDDLE_MASK
#undef RGB15_UPPER_MASK

// Extract unsigned char RGB values from 16 bit pixels

#undef RGB15_TO_R
#undef RGB15_TO_G 
#undef RGB15_TO_B

#undef BGR15_TO_B
#undef BGR15_TO_G 
#undef BGR15_TO_R

/* Combine r, g and b values to a 16 bit rgb pixel (taken from avifile) */

#undef PACK_RGB16

#undef PACK_BGR16

#undef PACK_RGB15
#undef PACK_BGR15
#undef COPY_24
#undef SWAP_24 
#undef INIT_RGBA32
#undef RGBA_2_RGB
