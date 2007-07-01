/*****************************************************************

  deinterlace_blend.c

  Copyright (c) 2007 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <stdio.h>
#include <string.h>

#include <config.h>

#include <gavl/gavl.h>
#include <video.h>
#include <deinterlace.h>

static void deinterlace_blend(gavl_video_deinterlacer_t * d,
                              gavl_video_frame_t * input_frame,
                              gavl_video_frame_t * output_frame)
  {
  int i, j;
  int width, height;
  uint8_t * b, *m, *t, *dst;
  
  width = d->line_width;
  height = d->format.image_height;
  
  for(i = 0; i < d->num_planes; i++)
    {
    if(i == 1)
      {
      width  /= d->sub_h;
      height /= d->sub_v;
      }

    /* Top line */
    
    t = input_frame->planes[i];
    m = t;
    b = m + input_frame->strides[i];
    dst = output_frame->planes[i];

    d->blend_func(t, m, b, dst, width);

    /* Middle lines */
    
    dst += output_frame->strides[i];

    m += input_frame->strides[i];
    b += input_frame->strides[i];
    
    for(j = 1; j < height-1; j++)
      {
      d->blend_func(t, m, b, dst, width);

      t += input_frame->strides[i];
      m += input_frame->strides[i];
      b += input_frame->strides[i];
      
      dst += output_frame->strides[i];
      }
    /* Bottom line */
    b -= input_frame->strides[i];
    d->blend_func(t, m, b, dst, width);
    }
  
  }

void gavl_deinterlacer_init_blend(gavl_video_deinterlacer_t * d,
                                  const gavl_video_format_t * src_format)
  {
  /* Get functions */
  gavl_video_deinterlace_blend_func_table_t tab;
  memset(&tab, 0, sizeof(tab));

  gavl_find_deinterlacer_blend_funcs_c(&tab, &d->opt, src_format);

#if HAVE_MMX  
  if(d->opt.accel_flags & GAVL_ACCEL_MMX)
    gavl_find_deinterlacer_blend_funcs_mmx(&tab, &d->opt, src_format);
  if((d->opt.accel_flags & GAVL_ACCEL_MMXEXT) && (d->opt.quality < 3))
    gavl_find_deinterlacer_blend_funcs_mmxext(&tab, &d->opt, src_format);
#endif


#if 0 // TDOD: Test 3dnow
  // #ifdef HAVE_3DNOW
  if((d->opt.accel_flags & GAVL_ACCEL_3DNOW) && (d->opt.quality < 3))
    gavl_find_deinterlacer_blend_funcs_3dnow(&tab, &d->opt, src_format);
#endif
  
  d->num_planes = gavl_pixelformat_num_planes(src_format->pixelformat);
  gavl_pixelformat_chroma_sub(src_format->pixelformat, &d->sub_h, &d->sub_v);
  
  switch(src_format->pixelformat)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
      d->line_width = src_format->image_width;
      d->blend_func = tab.func_packed_15;
      break;
    case GAVL_RGB_16:
    case GAVL_BGR_16:
      d->line_width = src_format->image_width;
      d->blend_func = tab.func_packed_16;
      break;
    case GAVL_RGB_24:
    case GAVL_BGR_24:
      d->line_width = src_format->image_width * 3;
      d->blend_func = tab.func_8;
      break;
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGBA_32:
    case GAVL_YUVA_32:
      d->line_width = src_format->image_width * 4;
      d->blend_func = tab.func_8;
      break;
    case GAVL_RGB_48:
      d->line_width = src_format->image_width * 3;
      d->blend_func = tab.func_16;
      break;
    case GAVL_RGB_FLOAT:
      d->line_width = src_format->image_width * 3;
      d->blend_func = tab.func_float;
      break;
    case GAVL_RGBA_64:
      d->line_width = src_format->image_width * 4;
      d->blend_func = tab.func_16;
      break;
    case GAVL_RGBA_FLOAT:
      d->line_width = src_format->image_width * 4;
      d->blend_func = tab.func_float;
      break;
    case GAVL_YUY2:
    case GAVL_UYVY:
      d->line_width = src_format->image_width * 2;
      d->blend_func = tab.func_8;
      break;
    case GAVL_YUV_444_P_16:
    case GAVL_YUV_422_P_16:
      d->line_width = src_format->image_width;
      d->blend_func = tab.func_16;
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      d->line_width = src_format->image_width;
      d->blend_func = tab.func_8;
      break;
    case GAVL_PIXELFORMAT_NONE:
      break;
      
    }
  d->func = deinterlace_blend;
  }