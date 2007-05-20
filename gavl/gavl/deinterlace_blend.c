/*****************************************************************

  deinterlace_copy.c

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

#include <gavl/gavl.h>
#include <video.h>
#include <deinterlace.h>

static void deinterlace_blend(gavl_video_deinterlacer_t * d,
                              gavl_video_frame_t * input_frame,
                              gavl_video_frame_t * output_frame)
  {
  int i, j;
  int width, height;
  
  width = d->line_width;
  height = d->format.image_height;
  
  for(i = 0; i < d->num_planes; i++)
    {
    if(i == 1)
      {
      width  /= d->sub_h;
      height /= d->sub_v;
      }
    
    for(j = 0; j < height; j++)
      {
      
      }
    }
  
  }

void gavl_deinterlacer_init_blend(gavl_video_deinterlacer_t * d,
                                  const gavl_video_format_t * src_format)
  {
  d->num_planes = gavl_pixelformat_num_planes(src_format->pixelformat);
  gavl_pixelformat_chroma_sub(src_format->pixelformat, &d->sub_h, &d->sub_v);
  
  switch(src_format->pixelformat)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
      d->line_width = src_format->image_width;
      break;
    case GAVL_RGB_24:
    case GAVL_BGR_24:
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGB_48:
    case GAVL_RGB_FLOAT:
    case GAVL_RGBA_32:
    case GAVL_YUVA_32:
    case GAVL_RGBA_64:
    case GAVL_RGBA_FLOAT:
    case GAVL_YUY2:
    case GAVL_UYVY:
    case GAVL_YUV_444_P_16:
    case GAVL_YUV_422_P_16:
    case GAVL_YUV_420_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
    case GAVL_PIXELFORMAT_NONE:
      break;
      
    }
  d->func = deinterlace_blend;
  }
