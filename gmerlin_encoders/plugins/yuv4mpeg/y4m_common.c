/*****************************************************************

  y4m_common.c

  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <string.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <yuv4mpeg.h>
#include "y4m_common.h"

void bg_y4m_set_pixelformat(bg_y4m_common_t * com)
  {
  switch(com->chroma_mode)
    {
    case Y4M_CHROMA_420JPEG:
      com->format.pixelformat = GAVL_YUV_420_P;
      com->format.chroma_placement = GAVL_CHROMA_PLACEMENT_DEFAULT;
      break;
    case Y4M_CHROMA_420MPEG2:
      com->format.pixelformat = GAVL_YUV_420_P;
      com->format.chroma_placement = GAVL_CHROMA_PLACEMENT_MPEG2;
      break;
    case Y4M_CHROMA_420PALDV:
      com->format.pixelformat = GAVL_YUV_420_P;
      com->format.chroma_placement = GAVL_CHROMA_PLACEMENT_DVPAL;
      break;
    case Y4M_CHROMA_444:
      com->format.pixelformat = GAVL_YUV_444_P;
      break;
    case Y4M_CHROMA_422:
      com->format.pixelformat = GAVL_YUV_422_P;
      break;
    case Y4M_CHROMA_411:
      com->format.pixelformat = GAVL_YUV_411_P;
      break;
    case Y4M_CHROMA_MONO:
      /* Monochrome isn't supported by gavl, we choose the format with the
         smallest chroma planes to save memory */
      com->format.pixelformat = GAVL_YUV_410_P;
      break;
    case Y4M_CHROMA_444ALPHA:
      /* Must be converted to packed */
      com->format.pixelformat = GAVL_YUVA_32;
      com->tmp_planes[0] = malloc(com->format.image_width *
                                    com->format.image_height * 4);
        
      com->tmp_planes[1] = com->tmp_planes[0] +
        com->format.image_width * com->format.image_height;
      com->tmp_planes[2] = com->tmp_planes[1] +
        com->format.image_width * com->format.image_height;
      com->tmp_planes[3] = com->tmp_planes[2] +
        com->format.image_width * com->format.image_height;
      break;
    }
  
  }

int bg_y4m_write_header(bg_y4m_common_t * com)
  {
  int i;
  y4m_ratio_t r;
  
  /* Set up the stream- and frame header */
  y4m_init_stream_info(&(com->si));
  y4m_init_frame_info(&(com->fi));

  y4m_si_set_width(&(com->si), com->format.image_width);
  y4m_si_set_height(&(com->si), com->format.image_height);

  switch(com->format.interlace_mode)
    {
    case GAVL_INTERLACE_TOP_FIRST:
      i = Y4M_ILACE_TOP_FIRST;
      break;
    case GAVL_INTERLACE_BOTTOM_FIRST:
      i = Y4M_ILACE_BOTTOM_FIRST;
      break;
    default:
      i = Y4M_ILACE_NONE;
      break;
    }
  y4m_si_set_interlace(&(com->si), i);

  r.n = com->format.timescale;
  r.d = com->format.frame_duration;
  
  y4m_si_set_framerate(&(com->si), r);

  r.n = com->format.pixel_width;
  r.d = com->format.pixel_height;

  y4m_si_set_sampleaspect(&(com->si), r);
  y4m_si_set_chroma(&(com->si), com->chroma_mode);

  /* Now, it's time to write the stream header */

  fprintf(stderr, "Writing stream header....");
  if(y4m_write_stream_header_cb(&(com->writer), &(com->si)) != Y4M_OK)
    {
    fprintf(stderr, "Writing stream header failed\n");
    return 0;
    }
  fprintf(stderr, "done\n");
  return 1;

  }

static void convert_yuva4444(uint8_t ** dst, uint8_t ** src, int width, int height, int stride)
  {
  int i, j;
  uint8_t *y, *u, *v, *a, *s;
  
  y = dst[0];
  u = dst[1];
  v = dst[2];
  a = dst[3];

  s = src[0];
  
  for(i = 0; i < height; i++)
    {
    s = src[0] + i * stride;
    for(j = 0; j < width; j++)
      {
      *(y++) = *(s++);
      *(u++) = *(s++);
      *(v++) = *(s++);
      *(a++) = *(s++);
      }
    }
  }


void bg_y4m_write_frame(bg_y4m_common_t * com, gavl_video_frame_t * frame)
  {
  /* Check for YUVA4444 */
  if(com->format.pixelformat == GAVL_YUVA_32)
    {
    convert_yuva4444(com->tmp_planes, frame->planes,
                     com->format.image_width,
                     com->format.image_height,
                     frame->strides[0]);
    
    }
  else
    {
    if((frame->strides[0] == com->strides[0]) &&
       (frame->strides[1] == com->strides[1]) &&
       (frame->strides[2] == com->strides[2]) &&
       (frame->strides[3] == com->strides[3]))
      y4m_write_frame_cb(&(com->writer), &(com->si), &(com->fi), frame->planes);
    else
      {
      if(!com->frame)
        com->frame = gavl_video_frame_create_nopadd(&(com->format));
      gavl_video_frame_copy(&(com->format), com->frame, frame);
      y4m_write_frame_cb(&(com->writer), &(com->si), &(com->fi), com->frame->planes);
      }
    }

  }

void bg_y4m_cleanup(bg_y4m_common_t * com)
  {
  y4m_fini_stream_info(&(com->si));
  y4m_fini_frame_info(&(com->fi));
  
  if(com->tmp_planes[0])
    free(com->tmp_planes[0]);
  if(com->frame)
    gavl_video_frame_destroy(com->frame);
  if(com->filename)
    free(com->filename);

  }
