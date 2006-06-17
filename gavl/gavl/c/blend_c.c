/*****************************************************************

  blend_c.c

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

#include <gavl/gavl.h>
#include <video.h>
#include <blend.h>

/* Switch on individual items in the colorspace tables / macros */
#define HAVE_YUVJ_TO_YUV_8
#define HAVE_YUVJ_TO_YUV_16
#define HAVE_YUV_8_TO_YUVJ
#define HAVE_RGB_16_TO_RGB_24

#include "colorspace_macros.h"
#include "colorspace_tables.h"

/* The naive blend equation
 *
 * d = a * s + (1-a) * d
 *
 * can be simplified to
 *
 * d = (s - d) * a + d
 *
 * but intermediate results can be negative!
 */

#define BLEND_8(s, d, a) \
  d = ((s - d) * a)/256 + d;

#define BLEND_16(s, d, a)                        \
  d = ((s - d) * a)/65536 + d;

#define BLEND_FLOAT(s, d, a)                       \
  d = (s - d) * a + d;

/* ovl: GAVL_RGBA_32 */

static void blend_rgb_15(gavl_overlay_blend_context_t * ctx,
                         gavl_video_frame_t * frame,
                         gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint16_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int r_tmp, g_tmp, b_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = (uint16_t*)dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      r_tmp = RGB15_TO_R_8(*dst_ptr);
      g_tmp = RGB15_TO_G_8(*dst_ptr);
      b_tmp = RGB15_TO_B_8(*dst_ptr);

      BLEND_8(ovl_ptr[0], r_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[1], g_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[2], b_tmp, ovl_ptr[3]);

      PACK_8_TO_RGB15(r_tmp,g_tmp,b_tmp,*dst_ptr);

      ovl_ptr+=4;
      dst_ptr++;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  
  }

/* ovl: GAVL_RGBA_32 */

static void blend_bgr_15(gavl_overlay_blend_context_t * ctx,
                         gavl_video_frame_t * frame,
                         gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint16_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int r_tmp, g_tmp, b_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = (uint16_t*)dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      r_tmp = BGR15_TO_R_8(*dst_ptr);
      g_tmp = BGR15_TO_G_8(*dst_ptr);
      b_tmp = BGR15_TO_B_8(*dst_ptr);

      BLEND_8(ovl_ptr[0], r_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[1], g_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[2], b_tmp, ovl_ptr[3]);

      PACK_8_TO_BGR15(r_tmp,g_tmp,b_tmp,*dst_ptr);

      ovl_ptr+=4;
      dst_ptr++;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_RGBA_32 */

static void blend_rgb_16(gavl_overlay_blend_context_t * ctx,
                         gavl_video_frame_t * frame,
                         gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint16_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int r_tmp, g_tmp, b_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = (uint16_t*)dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      r_tmp = RGB16_TO_R_8(*dst_ptr);
      g_tmp = RGB16_TO_G_8(*dst_ptr);
      b_tmp = RGB16_TO_B_8(*dst_ptr);

      BLEND_8(ovl_ptr[0], r_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[1], g_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[2], b_tmp, ovl_ptr[3]);

      PACK_8_TO_RGB16(r_tmp,g_tmp,b_tmp,*dst_ptr);

      ovl_ptr+=4;
      dst_ptr++;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_RGBA_32 */

static void blend_bgr_16(gavl_overlay_blend_context_t * ctx,
                         gavl_video_frame_t * frame,
                         gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint16_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int r_tmp, g_tmp, b_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = (uint16_t*)dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      r_tmp = BGR16_TO_R_8(*dst_ptr);
      g_tmp = BGR16_TO_G_8(*dst_ptr);
      b_tmp = BGR16_TO_B_8(*dst_ptr);

      BLEND_8(ovl_ptr[0], r_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[1], g_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[2], b_tmp, ovl_ptr[3]);

      PACK_8_TO_BGR16(r_tmp,g_tmp,b_tmp,*dst_ptr);

      ovl_ptr+=4;
      dst_ptr++;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_RGBA_32 */

static void blend_rgb_24(gavl_overlay_blend_context_t * ctx,
                         gavl_video_frame_t * frame,
                         gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int r_tmp, g_tmp, b_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      r_tmp = dst_ptr[0];
      g_tmp = dst_ptr[1];
      b_tmp = dst_ptr[2];

      BLEND_8(ovl_ptr[0], r_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[1], g_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[2], b_tmp, ovl_ptr[3]);
      
      dst_ptr[0] = r_tmp;
      dst_ptr[1] = g_tmp;
      dst_ptr[2] = b_tmp;
      
      ovl_ptr+=4;
      dst_ptr+=3;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_RGBA_32 */

static void blend_bgr_24(gavl_overlay_blend_context_t * ctx,
                         gavl_video_frame_t * frame,
                         gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int r_tmp, g_tmp, b_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      r_tmp = dst_ptr[2];
      g_tmp = dst_ptr[1];
      b_tmp = dst_ptr[0];

      BLEND_8(ovl_ptr[0], r_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[1], g_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[2], b_tmp, ovl_ptr[3]);
      
      dst_ptr[2] = r_tmp;
      dst_ptr[1] = g_tmp;
      dst_ptr[0] = b_tmp;
      
      ovl_ptr+=4;
      dst_ptr+=3;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_RGBA_32 */

static void blend_rgb_32(gavl_overlay_blend_context_t * ctx,
                         gavl_video_frame_t * frame,
                         gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int r_tmp, g_tmp, b_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      r_tmp = dst_ptr[0];
      g_tmp = dst_ptr[1];
      b_tmp = dst_ptr[2];

      BLEND_8(ovl_ptr[0], r_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[1], g_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[2], b_tmp, ovl_ptr[3]);
      
      dst_ptr[0] = r_tmp;
      dst_ptr[1] = g_tmp;
      dst_ptr[2] = b_tmp;
      
      ovl_ptr+=4;
      dst_ptr+=4;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_RGBA_32 */

static void blend_bgr_32(gavl_overlay_blend_context_t * ctx,
                         gavl_video_frame_t * frame,
                         gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int r_tmp, g_tmp, b_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      r_tmp = dst_ptr[2];
      g_tmp = dst_ptr[1];
      b_tmp = dst_ptr[0];

      BLEND_8(ovl_ptr[0], r_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[1], g_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[2], b_tmp, ovl_ptr[3]);
      
      dst_ptr[2] = r_tmp;
      dst_ptr[1] = g_tmp;
      dst_ptr[0] = b_tmp;
      
      ovl_ptr+=4;
      dst_ptr+=4;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_RGBA_32 */

static void blend_rgba_32(gavl_overlay_blend_context_t * ctx,
                          gavl_video_frame_t * frame,
                          gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int32_t r_tmp, g_tmp, b_tmp, a_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      r_tmp = dst_ptr[0];
      g_tmp = dst_ptr[1];
      b_tmp = dst_ptr[2];
      a_tmp = dst_ptr[3];

      BLEND_8(ovl_ptr[0], r_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[1], g_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[2], b_tmp, ovl_ptr[3]);

      // BLEND_8(ovl_ptr[3], a_tmp, ovl_ptr[3]);

      a_tmp = ovl_ptr[3] + a_tmp - ((ovl_ptr[3] * a_tmp) >> 8);
      
      dst_ptr[0] = r_tmp;
      dst_ptr[1] = g_tmp;
      dst_ptr[2] = b_tmp;
      dst_ptr[3] = RECLIP_32_TO_8(a_tmp);
      
      ovl_ptr+=4;
      dst_ptr+=4;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_RGBA_64 */

static void blend_rgb_48(gavl_overlay_blend_context_t * ctx,
                         gavl_video_frame_t * frame,
                         gavl_video_frame_t * overlay)
  {
  int i, j;
  uint16_t * ovl_ptr;
  uint16_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int64_t r_tmp, g_tmp, b_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = (uint16_t*)ovl_ptr_start;
    dst_ptr = (uint16_t*)dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      r_tmp = dst_ptr[0];
      g_tmp = dst_ptr[1];
      b_tmp = dst_ptr[2];

      BLEND_16(ovl_ptr[0], r_tmp, ovl_ptr[3]);
      BLEND_16(ovl_ptr[1], g_tmp, ovl_ptr[3]);
      BLEND_16(ovl_ptr[2], b_tmp, ovl_ptr[3]);
      
      dst_ptr[0] = r_tmp;
      dst_ptr[1] = g_tmp;
      dst_ptr[2] = b_tmp;
      
      ovl_ptr+=4;
      dst_ptr+=3;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_RGBA_64 */

static void blend_rgba_64(gavl_overlay_blend_context_t * ctx,
                         gavl_video_frame_t * frame,
                         gavl_video_frame_t * overlay)
  {
  int i, j;
  uint16_t * ovl_ptr;
  uint16_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int64_t r_tmp, g_tmp, b_tmp, a_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = (uint16_t*)ovl_ptr_start;
    dst_ptr = (uint16_t*)dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      r_tmp = dst_ptr[0];
      g_tmp = dst_ptr[1];
      b_tmp = dst_ptr[2];
      a_tmp = dst_ptr[3];

      BLEND_16(ovl_ptr[0], r_tmp, ovl_ptr[3]);
      BLEND_16(ovl_ptr[1], g_tmp, ovl_ptr[3]);
      BLEND_16(ovl_ptr[2], b_tmp, ovl_ptr[3]);

      // BLEND_8(ovl_ptr[3], a_tmp, ovl_ptr[3]);

      a_tmp = (int64_t)ovl_ptr[3] + a_tmp - ((ovl_ptr[3] * a_tmp) >> 16);
      
      dst_ptr[0] = r_tmp;
      dst_ptr[1] = g_tmp;
      dst_ptr[2] = b_tmp;
      dst_ptr[3] = RECLIP_64_TO_16(a_tmp);
      
      ovl_ptr+=4;
      dst_ptr+=4;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_RGBA_FLOAT */

static void blend_rgb_float(gavl_overlay_blend_context_t * ctx,
                            gavl_video_frame_t * frame,
                            gavl_video_frame_t * overlay)
  {
  int i, j;
  float * ovl_ptr;
  float * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = (float*)ovl_ptr_start;
    dst_ptr = (float*)dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      BLEND_FLOAT(ovl_ptr[0], dst_ptr[0], ovl_ptr[3]);
      BLEND_FLOAT(ovl_ptr[1], dst_ptr[1], ovl_ptr[3]);
      BLEND_FLOAT(ovl_ptr[2], dst_ptr[2], ovl_ptr[3]);
      
      ovl_ptr+=4;
      dst_ptr+=3;
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_RGBA_FLOAT */

static void blend_rgba_float(gavl_overlay_blend_context_t * ctx,
                             gavl_video_frame_t * frame,
                             gavl_video_frame_t * overlay)
  {
  int i, j;
  float * ovl_ptr;
  float * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = (float*)ovl_ptr_start;
    dst_ptr = (float*)dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      BLEND_FLOAT(ovl_ptr[0], dst_ptr[0], ovl_ptr[3]);
      BLEND_FLOAT(ovl_ptr[1], dst_ptr[1], ovl_ptr[3]);
      BLEND_FLOAT(ovl_ptr[2], dst_ptr[2], ovl_ptr[3]);
      dst_ptr[3] = dst_ptr[3] + ovl_ptr[3] - dst_ptr[3]*ovl_ptr[3];
      ovl_ptr+=4;
      dst_ptr+=4;
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuy2(gavl_overlay_blend_context_t * ctx,
                       gavl_video_frame_t * frame,
                       gavl_video_frame_t * overlay)
  {
  int i, j, jmax;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;
  
  int tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];

  jmax = ctx->ovl.ovl_rect.w / 2;
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {

      /* Y0 */
      tmp = dst_ptr[0];
      BLEND_8(ovl_ptr[0], tmp, ovl_ptr[3]);
      dst_ptr[0] = tmp;

      /* U0 */
      tmp = dst_ptr[1];
      BLEND_8(ovl_ptr[1], tmp, ovl_ptr[3]);
      dst_ptr[1] = tmp;

      /* V0 */
      tmp = dst_ptr[3];
      BLEND_8(ovl_ptr[2], tmp, ovl_ptr[3]);
      dst_ptr[3] = tmp;

      /* Y1 */
      tmp = dst_ptr[2];
      BLEND_8(ovl_ptr[4], tmp, ovl_ptr[7]);
      dst_ptr[2] = tmp;
      
      ovl_ptr+=8;
      dst_ptr+=4;
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }


  
  }

/* ovl: GAVL_YUVA_32 */

static void blend_uyvy(gavl_overlay_blend_context_t * ctx,
                       gavl_video_frame_t * frame,
                       gavl_video_frame_t * overlay)
  {
  int i, j, jmax;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;
  
  int tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];

  jmax = ctx->ovl.ovl_rect.w / 2;
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < jmax; j++)
      {

      /* Y0 */
      tmp = dst_ptr[1];
      BLEND_8(ovl_ptr[0], tmp, ovl_ptr[3]);
      dst_ptr[1] = tmp;

      /* U0 */
      tmp = dst_ptr[0];
      BLEND_8(ovl_ptr[1], tmp, ovl_ptr[3]);
      dst_ptr[0] = tmp;

      /* V0 */
      tmp = dst_ptr[2];
      BLEND_8(ovl_ptr[2], tmp, ovl_ptr[3]);
      dst_ptr[2] = tmp;

      /* Y1 */
      tmp = dst_ptr[3];
      BLEND_8(ovl_ptr[4], tmp, ovl_ptr[7]);
      dst_ptr[3] = tmp;
      
      ovl_ptr+=8;
      dst_ptr+=4;
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuva_32(gavl_overlay_blend_context_t * ctx,
                          gavl_video_frame_t * frame,
                          gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_start;

  int32_t y_tmp, u_tmp, v_tmp, a_tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_start = frame->planes[0];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr = dst_ptr_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      y_tmp = dst_ptr[0];
      u_tmp = dst_ptr[1];
      v_tmp = dst_ptr[2];
      a_tmp = dst_ptr[3];

      BLEND_8(ovl_ptr[0], y_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[1], u_tmp, ovl_ptr[3]);
      BLEND_8(ovl_ptr[2], v_tmp, ovl_ptr[3]);

      // BLEND_8(ovl_ptr[3], a_tmp, ovl_ptr[3]);

      a_tmp = ovl_ptr[3] + a_tmp - ((ovl_ptr[3] * a_tmp) >> 8);
      
      dst_ptr[0] = y_tmp;
      dst_ptr[1] = u_tmp;
      dst_ptr[2] = v_tmp;
      dst_ptr[3] = RECLIP_32_TO_8(a_tmp);
      
      ovl_ptr+=4;
      dst_ptr+=4;
      
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_start += frame->strides[0];
    }

  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuv_420_p(gavl_overlay_blend_context_t * ctx,
                            gavl_video_frame_t * frame,
                            gavl_video_frame_t * overlay)
  {
  int i, j, imax, jmax;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr_y;
  uint8_t * dst_ptr_u;
  uint8_t * dst_ptr_v;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_y_start;
  uint8_t * dst_ptr_u_start;
  uint8_t * dst_ptr_v_start;
  
  int tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_y_start = frame->planes[0];
  dst_ptr_u_start = frame->planes[1];
  dst_ptr_v_start = frame->planes[2];

  imax = ctx->ovl.ovl_rect.h / 2;
  jmax = ctx->ovl.ovl_rect.w / 2;
  
  for(i = 0; i < imax; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    dst_ptr_u = dst_ptr_u_start;
    dst_ptr_v = dst_ptr_v_start;
    
    for(j = 0; j < jmax; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[0], tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;

      /* U0 */
      tmp = *dst_ptr_u;
      BLEND_8(ovl_ptr[1], tmp, ovl_ptr[3]);
      *(dst_ptr_u++) = tmp;

      /* V0 */
      tmp = *dst_ptr_v;
      BLEND_8(ovl_ptr[2], tmp, ovl_ptr[3]);
      *(dst_ptr_v++) = tmp;

      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[4], tmp, ovl_ptr[7]);
      *(dst_ptr_y++) = tmp;
      
      ovl_ptr+=8;
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_y_start += frame->strides[0];
    dst_ptr_u_start += frame->strides[1];
    dst_ptr_v_start += frame->strides[2];


    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    dst_ptr_u = dst_ptr_u_start;
    dst_ptr_v = dst_ptr_v_start;
    
    for(j = 0; j < jmax; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[0], tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;
      
      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[4], tmp, ovl_ptr[7]);
      *(dst_ptr_y++) = tmp;
      
      ovl_ptr+=8;
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_y_start += frame->strides[0];
    }
  
  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuv_422_p(gavl_overlay_blend_context_t * ctx,
                            gavl_video_frame_t * frame,
                            gavl_video_frame_t * overlay)
  {
  int i, j, jmax;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr_y;
  uint8_t * dst_ptr_u;
  uint8_t * dst_ptr_v;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_y_start;
  uint8_t * dst_ptr_u_start;
  uint8_t * dst_ptr_v_start;
  
  int tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_y_start = frame->planes[0];
  dst_ptr_u_start = frame->planes[1];
  dst_ptr_v_start = frame->planes[2];

  jmax = ctx->ovl.ovl_rect.w / 2;
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    dst_ptr_u = dst_ptr_u_start;
    dst_ptr_v = dst_ptr_v_start;
    
    for(j = 0; j < jmax; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[0], tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;

      /* U0 */
      tmp = *dst_ptr_u;
      BLEND_8(ovl_ptr[1], tmp, ovl_ptr[3]);
      *(dst_ptr_u++) = tmp;

      /* V0 */
      tmp = *dst_ptr_v;
      BLEND_8(ovl_ptr[2], tmp, ovl_ptr[3]);
      *(dst_ptr_v++) = tmp;

      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[4], tmp, ovl_ptr[7]);
      *(dst_ptr_y++) = tmp;
      
      ovl_ptr+=8;
      }
    ovl_ptr_start += overlay->strides[0];

    dst_ptr_y_start += frame->strides[0];
    dst_ptr_u_start += frame->strides[1];
    dst_ptr_v_start += frame->strides[2];
    }
  
  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuv_444_p(gavl_overlay_blend_context_t * ctx,
                            gavl_video_frame_t * frame,
                            gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr_y;
  uint8_t * dst_ptr_u;
  uint8_t * dst_ptr_v;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_y_start;
  uint8_t * dst_ptr_u_start;
  uint8_t * dst_ptr_v_start;
  
  int tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_y_start = frame->planes[0];
  dst_ptr_u_start = frame->planes[1];
  dst_ptr_v_start = frame->planes[2];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    dst_ptr_u = dst_ptr_u_start;
    dst_ptr_v = dst_ptr_v_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[0], tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;

      /* U0 */
      tmp = *dst_ptr_u;
      BLEND_8(ovl_ptr[1], tmp, ovl_ptr[3]);
      *(dst_ptr_u++) = tmp;

      /* V0 */
      tmp = *dst_ptr_v;
      BLEND_8(ovl_ptr[2], tmp, ovl_ptr[3]);
      *(dst_ptr_v++) = tmp;
      
      ovl_ptr+=4;
      }
    ovl_ptr_start += overlay->strides[0];

    dst_ptr_y_start += frame->strides[0];
    dst_ptr_u_start += frame->strides[1];
    dst_ptr_v_start += frame->strides[2];
    }
  
  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuv_411_p(gavl_overlay_blend_context_t * ctx,
                            gavl_video_frame_t * frame,
                            gavl_video_frame_t * overlay)
  {
  int i, j, jmax;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr_y;
  uint8_t * dst_ptr_u;
  uint8_t * dst_ptr_v;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_y_start;
  uint8_t * dst_ptr_u_start;
  uint8_t * dst_ptr_v_start;
  
  int tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_y_start = frame->planes[0];
  dst_ptr_u_start = frame->planes[1];
  dst_ptr_v_start = frame->planes[2];

  jmax = ctx->ovl.ovl_rect.w / 4;
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    dst_ptr_u = dst_ptr_u_start;
    dst_ptr_v = dst_ptr_v_start;
    
    for(j = 0; j < jmax; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[0], tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;

      /* U0 */
      tmp = *dst_ptr_u;
      BLEND_8(ovl_ptr[1], tmp, ovl_ptr[3]);
      *(dst_ptr_u++) = tmp;

      /* V0 */
      tmp = *dst_ptr_v;
      BLEND_8(ovl_ptr[2], tmp, ovl_ptr[3]);
      *(dst_ptr_v++) = tmp;

      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[4], tmp, ovl_ptr[7]);
      *(dst_ptr_y++) = tmp;

      /* Y2 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[8], tmp, ovl_ptr[11]);
      *(dst_ptr_y++) = tmp;

      /* Y3 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[12], tmp, ovl_ptr[15]);
      *(dst_ptr_y++) = tmp;
      
      ovl_ptr+=16;
      }
    ovl_ptr_start += overlay->strides[0];

    dst_ptr_y_start += frame->strides[0];
    dst_ptr_u_start += frame->strides[1];
    dst_ptr_v_start += frame->strides[2];
    }
  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuv_410_p(gavl_overlay_blend_context_t * ctx,
                            gavl_video_frame_t * frame,
                            gavl_video_frame_t * overlay)
  {
    int i, j, imax, jmax;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr_y;
  uint8_t * dst_ptr_u;
  uint8_t * dst_ptr_v;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_y_start;
  uint8_t * dst_ptr_u_start;
  uint8_t * dst_ptr_v_start;
  
  int tmp;
  
  ovl_ptr_start = overlay->planes[0];

  dst_ptr_y_start = frame->planes[0];
  dst_ptr_u_start = frame->planes[1];
  dst_ptr_v_start = frame->planes[2];

  imax = ctx->ovl.ovl_rect.h / 4;
  jmax = ctx->ovl.ovl_rect.w / 4;
  
  for(i = 0; i < imax; i++)
    {
    /* Line 0 */

    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    dst_ptr_u = dst_ptr_u_start;
    dst_ptr_v = dst_ptr_v_start;
    
    for(j = 0; j < jmax; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[0], tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;

      /* U0 */
      tmp = *dst_ptr_u;
      BLEND_8(ovl_ptr[1], tmp, ovl_ptr[3]);
      *(dst_ptr_u++) = tmp;

      /* V0 */
      tmp = *dst_ptr_v;
      BLEND_8(ovl_ptr[2], tmp, ovl_ptr[3]);
      *(dst_ptr_v++) = tmp;

      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[4], tmp, ovl_ptr[7]);
      *(dst_ptr_y++) = tmp;
      
      /* Y2 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[8], tmp, ovl_ptr[11]);
      *(dst_ptr_y++) = tmp;
      
      /* Y3 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[12], tmp, ovl_ptr[15]);
      *(dst_ptr_y++) = tmp;
      
      ovl_ptr+=16;
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_y_start += frame->strides[0];
    dst_ptr_u_start += frame->strides[1];
    dst_ptr_v_start += frame->strides[2];

    /* Line 1 */
    
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    
    for(j = 0; j < jmax; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[0], tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;
      
      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[4], tmp, ovl_ptr[7]);
      *(dst_ptr_y++) = tmp;
      
      /* Y2 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[8], tmp, ovl_ptr[11]);
      *(dst_ptr_y++) = tmp;

      /* Y3 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[12], tmp, ovl_ptr[15]);
      *(dst_ptr_y++) = tmp;
            
      ovl_ptr+=16;
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_y_start += frame->strides[0];

    /* Line 2 */
    
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    
    for(j = 0; j < jmax; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[0], tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;
      
      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[4], tmp, ovl_ptr[7]);
      *(dst_ptr_y++) = tmp;
            
      /* Y2 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[8], tmp, ovl_ptr[11]);
      *(dst_ptr_y++) = tmp;

      /* Y3 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[12], tmp, ovl_ptr[15]);
      *(dst_ptr_y++) = tmp;
            
      ovl_ptr+=16;
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_y_start += frame->strides[0];

    /* Line 3 */
    
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    
    for(j = 0; j < jmax; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[0], tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;
      
      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[4], tmp, ovl_ptr[7]);
      *(dst_ptr_y++) = tmp;
            
      /* Y2 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[8], tmp, ovl_ptr[11]);
      *(dst_ptr_y++) = tmp;

      /* Y3 */
      tmp = *dst_ptr_y;
      BLEND_8(ovl_ptr[12], tmp, ovl_ptr[16]);
      *(dst_ptr_y++) = tmp;
            
      ovl_ptr+=16;
      }
    ovl_ptr_start += overlay->strides[0];
    dst_ptr_y_start += frame->strides[0];

    
    }

  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuvj_420_p(gavl_overlay_blend_context_t * ctx,
                            gavl_video_frame_t * frame,
                            gavl_video_frame_t * overlay)
  {
  int i, j, imax, jmax;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr_y;
  uint8_t * dst_ptr_u;
  uint8_t * dst_ptr_v;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_y_start;
  uint8_t * dst_ptr_u_start;
  uint8_t * dst_ptr_v_start;
  
  int tmp;

  imax = ctx->ovl.ovl_rect.h/2;
  jmax = ctx->ovl.ovl_rect.w/2;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_y_start = frame->planes[0];
  dst_ptr_u_start = frame->planes[1];
  dst_ptr_v_start = frame->planes[2];
  
  for(i = 0; i < imax; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    dst_ptr_u = dst_ptr_u_start;
    dst_ptr_v = dst_ptr_v_start;
    
    for(j = 0; j < jmax; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(Y_8_TO_YJ_8(ovl_ptr[0]), tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;

      /* U0 */
      tmp = *dst_ptr_u;
      BLEND_8(UV_8_TO_UVJ_8(ovl_ptr[1]), tmp, ovl_ptr[3]);
      *(dst_ptr_u++) = tmp;

      /* V0 */
      tmp = *dst_ptr_v;
      BLEND_8(UV_8_TO_UVJ_8(ovl_ptr[2]), tmp, ovl_ptr[3]);
      *(dst_ptr_v++) = tmp;

      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_8(Y_8_TO_YJ_8(ovl_ptr[4]), tmp, ovl_ptr[7]);
      *(dst_ptr_y++) = tmp;

      ovl_ptr+=8;
      }
    ovl_ptr_start += overlay->strides[0];

    dst_ptr_y_start += frame->strides[0];
    dst_ptr_u_start += frame->strides[1];
    dst_ptr_v_start += frame->strides[2];


    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    
    for(j = 0; j < jmax; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(Y_8_TO_YJ_8(ovl_ptr[0]), tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;
      
      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_8(Y_8_TO_YJ_8(ovl_ptr[4]), tmp, ovl_ptr[7]);
      *(dst_ptr_y++) = tmp;
      
      ovl_ptr+=8;
      }
    ovl_ptr_start += overlay->strides[0];

    dst_ptr_y_start += frame->strides[0];


    }
  
  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuvj_422_p(gavl_overlay_blend_context_t * ctx,
                            gavl_video_frame_t * frame,
                            gavl_video_frame_t * overlay)
  {
  int i, j, jmax;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr_y;
  uint8_t * dst_ptr_u;
  uint8_t * dst_ptr_v;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_y_start;
  uint8_t * dst_ptr_u_start;
  uint8_t * dst_ptr_v_start;
  
  int tmp;

  jmax = ctx->ovl.ovl_rect.w/2;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_y_start = frame->planes[0];
  dst_ptr_u_start = frame->planes[1];
  dst_ptr_v_start = frame->planes[2];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    dst_ptr_u = dst_ptr_u_start;
    dst_ptr_v = dst_ptr_v_start;
    
    for(j = 0; j < jmax; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(Y_8_TO_YJ_8(ovl_ptr[0]), tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;

      /* U0 */
      tmp = *dst_ptr_u;
      BLEND_8(UV_8_TO_UVJ_8(ovl_ptr[1]), tmp, ovl_ptr[3]);
      *(dst_ptr_u++) = tmp;

      /* V0 */
      tmp = *dst_ptr_v;
      BLEND_8(UV_8_TO_UVJ_8(ovl_ptr[2]), tmp, ovl_ptr[3]);
      *(dst_ptr_v++) = tmp;

      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_8(Y_8_TO_YJ_8(ovl_ptr[4]), tmp, ovl_ptr[7]);
      *(dst_ptr_y++) = tmp;

      
      ovl_ptr+=8;
      }
    ovl_ptr_start += overlay->strides[0];

    dst_ptr_y_start += frame->strides[0];
    dst_ptr_u_start += frame->strides[1];
    dst_ptr_v_start += frame->strides[2];
    }
  
  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuvj_444_p(gavl_overlay_blend_context_t * ctx,
                            gavl_video_frame_t * frame,
                            gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint8_t * dst_ptr_y;
  uint8_t * dst_ptr_u;
  uint8_t * dst_ptr_v;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_y_start;
  uint8_t * dst_ptr_u_start;
  uint8_t * dst_ptr_v_start;
  
  int tmp;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_y_start = frame->planes[0];
  dst_ptr_u_start = frame->planes[1];
  dst_ptr_v_start = frame->planes[2];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = dst_ptr_y_start;
    dst_ptr_u = dst_ptr_u_start;
    dst_ptr_v = dst_ptr_v_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_8(Y_8_TO_YJ_8(ovl_ptr[0]), tmp, ovl_ptr[3]);
      *(dst_ptr_y++) = tmp;

      /* U0 */
      tmp = *dst_ptr_u;
      BLEND_8(UV_8_TO_UVJ_8(ovl_ptr[1]), tmp, ovl_ptr[3]);
      *(dst_ptr_u++) = tmp;

      /* V0 */
      tmp = *dst_ptr_v;
      BLEND_8(UV_8_TO_UVJ_8(ovl_ptr[2]), tmp, ovl_ptr[3]);
      *(dst_ptr_v++) = tmp;
      
      ovl_ptr+=4;
      }
    ovl_ptr_start += overlay->strides[0];

    dst_ptr_y_start += frame->strides[0];
    dst_ptr_u_start += frame->strides[1];
    dst_ptr_v_start += frame->strides[2];
    }
  
  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuv_422_p_16(gavl_overlay_blend_context_t * ctx,
                               gavl_video_frame_t * frame,
                               gavl_video_frame_t * overlay)
  {
  int i, j, jmax;
  uint8_t * ovl_ptr;
  uint16_t * dst_ptr_y;
  uint16_t * dst_ptr_u;
  uint16_t * dst_ptr_v;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_y_start;
  uint8_t * dst_ptr_u_start;
  uint8_t * dst_ptr_v_start;
  
  int64_t tmp, alpha;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_y_start = frame->planes[0];
  dst_ptr_u_start = frame->planes[1];
  dst_ptr_v_start = frame->planes[2];

  jmax = ctx->ovl.ovl_rect.w / 2;
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = (uint16_t*)dst_ptr_y_start;
    dst_ptr_u = (uint16_t*)dst_ptr_u_start;
    dst_ptr_v = (uint16_t*)dst_ptr_v_start;
    
    for(j = 0; j < jmax; j++)
      {
      alpha = RGB_8_TO_16(ovl_ptr[3]);
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_16(Y_8_TO_16(ovl_ptr[0]), tmp, alpha);
      *(dst_ptr_y++) = tmp;

      /* U0 */
      tmp = *dst_ptr_u;
      BLEND_16(UV_8_TO_16(ovl_ptr[1]), tmp, alpha);
      *(dst_ptr_u++) = tmp;

      /* V0 */
      tmp = *dst_ptr_v;
      BLEND_16(UV_8_TO_16(ovl_ptr[2]), tmp, alpha);
      *(dst_ptr_v++) = tmp;

      /* Y1 */
      tmp = *dst_ptr_y;
      BLEND_16(Y_8_TO_16(ovl_ptr[0]), tmp, alpha);
      *(dst_ptr_y++) = tmp;

      
      ovl_ptr+=8;
      }
    ovl_ptr_start += overlay->strides[0];

    dst_ptr_y_start += frame->strides[0];
    dst_ptr_u_start += frame->strides[1];
    dst_ptr_v_start += frame->strides[2];
    }
  
  }

/* ovl: GAVL_YUVA_32 */

static void blend_yuv_444_p_16(gavl_overlay_blend_context_t * ctx,
                               gavl_video_frame_t * frame,
                               gavl_video_frame_t * overlay)
  {
  int i, j;
  uint8_t * ovl_ptr;
  uint16_t * dst_ptr_y;
  uint16_t * dst_ptr_u;
  uint16_t * dst_ptr_v;
  
  uint8_t * ovl_ptr_start;
  uint8_t * dst_ptr_y_start;
  uint8_t * dst_ptr_u_start;
  uint8_t * dst_ptr_v_start;
  
  int64_t tmp, alpha;
  
  ovl_ptr_start = overlay->planes[0];
  dst_ptr_y_start = frame->planes[0];
  dst_ptr_u_start = frame->planes[1];
  dst_ptr_v_start = frame->planes[2];
  
  for(i = 0; i < ctx->ovl.ovl_rect.h; i++)
    {
    ovl_ptr = ovl_ptr_start;
    dst_ptr_y = (uint16_t*)dst_ptr_y_start;
    dst_ptr_u = (uint16_t*)dst_ptr_u_start;
    dst_ptr_v = (uint16_t*)dst_ptr_v_start;
    
    for(j = 0; j < ctx->ovl.ovl_rect.w; j++)
      {
      alpha = RGB_8_TO_16(ovl_ptr[3]);
      /* Y0 */
      tmp = *dst_ptr_y;
      BLEND_16(Y_8_TO_16(ovl_ptr[0]), tmp, alpha);
      *(dst_ptr_y++) = tmp;

      /* U0 */
      tmp = *dst_ptr_u;
      BLEND_16(UV_8_TO_16(ovl_ptr[1]), tmp, alpha);
      *(dst_ptr_u++) = tmp;

      /* V0 */
      tmp = *dst_ptr_v;
      BLEND_16(UV_8_TO_16(ovl_ptr[2]), tmp, alpha);
      *(dst_ptr_v++) = tmp;
      
      ovl_ptr+=4;
      }
    ovl_ptr_start += overlay->strides[0];

    dst_ptr_y_start += frame->strides[0];
    dst_ptr_u_start += frame->strides[1];
    dst_ptr_v_start += frame->strides[2];
    }
  
  }

gavl_blend_func_t
gavl_find_blend_func_c(gavl_overlay_blend_context_t * ctx,
                       gavl_pixelformat_t frame_format,
                       gavl_pixelformat_t * overlay_format)
  {
  switch(frame_format)
    {
    case GAVL_RGB_15:
      *overlay_format = GAVL_RGBA_32;
      return blend_rgb_15;
      break;
    case GAVL_BGR_15:
      *overlay_format = GAVL_RGBA_32;
      return blend_bgr_15;
      break;
    case GAVL_RGB_16:
      *overlay_format = GAVL_RGBA_32;
      return blend_rgb_16;
      break;
    case GAVL_BGR_16:
      *overlay_format = GAVL_RGBA_32;
      return blend_bgr_16;
      break;
    case GAVL_RGB_24:
      *overlay_format = GAVL_RGBA_32;
      return blend_rgb_24;
      break;
    case GAVL_BGR_24:
      *overlay_format = GAVL_RGBA_32;
      return blend_bgr_24;
      break;
    case GAVL_RGB_32:
      *overlay_format = GAVL_RGBA_32;
      return blend_rgb_32;
      break;
    case GAVL_BGR_32:
      *overlay_format = GAVL_RGBA_32;
      return blend_bgr_32;
      break;
    case GAVL_RGBA_32:
      *overlay_format = GAVL_RGBA_32;
      return blend_rgba_32;
      break;
    case GAVL_YUY2:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuy2;
      break;
    case GAVL_UYVY:
      *overlay_format = GAVL_YUVA_32;
      return blend_uyvy;
      break;
    case GAVL_YUVA_32:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuva_32;
      break;
    case GAVL_YUV_420_P:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuv_420_p;
      break;
    case GAVL_YUV_422_P:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuv_422_p;
      break;
    case GAVL_YUV_444_P:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuv_444_p;
      break;
    case GAVL_YUV_411_P:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuv_411_p;
      break;
    case GAVL_YUV_410_P:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuv_410_p;
      break;
    case GAVL_YUVJ_420_P:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuvj_420_p;
      break;
    case GAVL_YUVJ_422_P:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuvj_422_p;
      break;
    case GAVL_YUVJ_444_P:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuvj_444_p;
      break;
    case GAVL_YUV_444_P_16:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuv_444_p_16;
      break;
    case GAVL_YUV_422_P_16:
      *overlay_format = GAVL_YUVA_32;
      return blend_yuv_422_p_16;
      break;
    case GAVL_RGB_48:
      *overlay_format = GAVL_RGBA_64;
      return blend_rgb_48;
      break;
    case GAVL_RGBA_64:
      *overlay_format = GAVL_RGBA_64;
      return blend_rgba_64;
      break;
    case GAVL_RGB_FLOAT:
      *overlay_format = GAVL_RGBA_FLOAT;
      return blend_rgb_float;
      break;
    case GAVL_RGBA_FLOAT:
      *overlay_format = GAVL_RGBA_FLOAT;
      return blend_rgba_float;
      break;
    case GAVL_PIXELFORMAT_NONE:
      return (gavl_blend_func_t)0;
    }
  return (gavl_blend_func_t)0;
  }
