/*****************************************************************

  _video_copy.c 

  Copyright (c) 2001-2002 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/


/*
 *  The conversion routines YUV 422 Planar <-> YUV 420 Planar
 *  and YUV 411 Planar <-> YUV 410 Planar
 *  differ only by their memcpy implementations 
 */

static void yuv_420_p_to_yuv_422_p_generic(gavl_video_convert_context_t * ctx)
  {
  int i;
  int y_size =
    ctx->input_frame->strides[0] < ctx->output_frame->strides[0] ?
    ctx->input_frame->strides[0] : ctx->output_frame->strides[0];

  int uv_size =
    ctx->input_frame->strides[1] < ctx->output_frame->strides[1] ?
    ctx->input_frame->strides[1] : ctx->output_frame->strides[1];
  int imax = ctx->input_format.image_height/2;
  
  uint8_t * src_y = ctx->input_frame->planes[0];
  uint8_t * src_u = ctx->input_frame->planes[1];
  uint8_t * src_v = ctx->input_frame->planes[2];
  uint8_t * dst_y = ctx->output_frame->planes[0];
  uint8_t * dst_u = ctx->output_frame->planes[1];
  uint8_t * dst_v = ctx->output_frame->planes[2];

  for(i = 0; i < imax; i++)
    {
    GAVL_MEMCPY(dst_y, src_y, y_size);
    GAVL_MEMCPY(dst_u, src_u, uv_size);
    GAVL_MEMCPY(dst_v, src_v, uv_size);
    
    dst_y += ctx->output_frame->strides[0];
    dst_u += ctx->output_frame->strides[1];
    dst_v += ctx->output_frame->strides[2];
    
    src_y += ctx->input_frame->strides[0];

    GAVL_MEMCPY(dst_y, src_y, y_size);
    GAVL_MEMCPY(dst_u, src_u, uv_size);
    GAVL_MEMCPY(dst_v, src_v, uv_size);
    
    dst_y += ctx->output_frame->strides[0];
    dst_u += ctx->output_frame->strides[1];
    dst_v += ctx->output_frame->strides[2];
    
    src_y += ctx->input_frame->strides[0];
    src_u += ctx->input_frame->strides[1];
    src_v += ctx->input_frame->strides[2];
    
    }
  }

static void yuv_422_p_to_yuv_420_p_generic(gavl_video_convert_context_t * ctx)
  {
  int i;
  int y_size =
    ctx->input_frame->strides[0] < ctx->output_frame->strides[0] ?
    ctx->input_frame->strides[0] : ctx->output_frame->strides[0];

  int uv_size =
    ctx->input_frame->strides[1] < ctx->output_frame->strides[1] ?
    ctx->input_frame->strides[1] : ctx->output_frame->strides[1];
  int imax = ctx->input_format.image_height/2;
  
  uint8_t * src_y = ctx->input_frame->planes[0];
  uint8_t * src_u = ctx->input_frame->planes[1];
  uint8_t * src_v = ctx->input_frame->planes[2];
  uint8_t * dst_y = ctx->output_frame->planes[0];
  uint8_t * dst_u = ctx->output_frame->planes[1];
  uint8_t * dst_v = ctx->output_frame->planes[2];

  for(i = 0; i < imax; i++)
    {
    
    GAVL_MEMCPY(dst_y, src_y, y_size);
    GAVL_MEMCPY(dst_u, src_u, uv_size);
    GAVL_MEMCPY(dst_v, src_v, uv_size);
    
    dst_y += ctx->output_frame->strides[0];
    
    src_y += ctx->input_frame->strides[0];
    src_u += ctx->input_frame->strides[1];
    src_v += ctx->input_frame->strides[2];

    GAVL_MEMCPY(dst_y, src_y, y_size);
    
    dst_y += ctx->output_frame->strides[0];
    dst_u += ctx->output_frame->strides[1];
    dst_v += ctx->output_frame->strides[2];
    
    src_y += ctx->input_frame->strides[0];
    src_u += ctx->input_frame->strides[1];
    src_v += ctx->input_frame->strides[2];
    
    }
  

  }

static void yuv_410_p_to_yuv_411_p_generic(gavl_video_convert_context_t * ctx)
  {
  int i;
  int y_size =
    ctx->input_frame->strides[0] < ctx->output_frame->strides[0] ?
    ctx->input_frame->strides[0] : ctx->output_frame->strides[0];

  int uv_size =
    ctx->input_frame->strides[1] < ctx->output_frame->strides[1] ?
    ctx->input_frame->strides[1] : ctx->output_frame->strides[1];
  int imax = ctx->input_format.image_height/4;
  
  uint8_t * src_y = ctx->input_frame->planes[0];
  uint8_t * src_u = ctx->input_frame->planes[1];
  uint8_t * src_v = ctx->input_frame->planes[2];
  uint8_t * dst_y = ctx->output_frame->planes[0];
  uint8_t * dst_u = ctx->output_frame->planes[1];
  uint8_t * dst_v = ctx->output_frame->planes[2];

  for(i = 0; i < imax; i++)
    {
    /* 1 */
    GAVL_MEMCPY(dst_y, src_y, y_size);
    GAVL_MEMCPY(dst_u, src_u, uv_size);
    GAVL_MEMCPY(dst_v, src_v, uv_size);
    
    dst_y += ctx->output_frame->strides[0];
    dst_u += ctx->output_frame->strides[1];
    dst_v += ctx->output_frame->strides[2];
    
    src_y += ctx->input_frame->strides[0];

    /* 2 */
    GAVL_MEMCPY(dst_y, src_y, y_size);
    GAVL_MEMCPY(dst_u, src_u, uv_size);
    GAVL_MEMCPY(dst_v, src_v, uv_size);
    
    dst_y += ctx->output_frame->strides[0];
    dst_u += ctx->output_frame->strides[1];
    dst_v += ctx->output_frame->strides[2];
    
    src_y += ctx->input_frame->strides[0];

    /* 3 */
    GAVL_MEMCPY(dst_y, src_y, y_size);
    GAVL_MEMCPY(dst_u, src_u, uv_size);
    GAVL_MEMCPY(dst_v, src_v, uv_size);
    
    dst_y += ctx->output_frame->strides[0];
    dst_u += ctx->output_frame->strides[1];
    dst_v += ctx->output_frame->strides[2];
    
    src_y += ctx->input_frame->strides[0];

    /* 4 */
    GAVL_MEMCPY(dst_y, src_y, y_size);
    GAVL_MEMCPY(dst_u, src_u, uv_size);
    GAVL_MEMCPY(dst_v, src_v, uv_size);
    
    dst_y += ctx->output_frame->strides[0];
    dst_u += ctx->output_frame->strides[1];
    dst_v += ctx->output_frame->strides[2];
    
    src_y += ctx->input_frame->strides[0];
    src_u += ctx->input_frame->strides[1];
    src_v += ctx->input_frame->strides[2];
    
    }
  }

static void yuv_411_p_to_yuv_410_p_generic(gavl_video_convert_context_t * ctx)
  {
  int i;
  int y_size =
    ctx->input_frame->strides[0] < ctx->output_frame->strides[0] ?
    ctx->input_frame->strides[0] : ctx->output_frame->strides[0];

  int uv_size =
    ctx->input_frame->strides[1] < ctx->output_frame->strides[1] ?
    ctx->input_frame->strides[1] : ctx->output_frame->strides[1];
  int imax = ctx->input_format.image_height/4;
  
  uint8_t * src_y = ctx->input_frame->planes[0];
  uint8_t * src_u = ctx->input_frame->planes[1];
  uint8_t * src_v = ctx->input_frame->planes[2];
  uint8_t * dst_y = ctx->output_frame->planes[0];
  uint8_t * dst_u = ctx->output_frame->planes[1];
  uint8_t * dst_v = ctx->output_frame->planes[2];

  for(i = 0; i < imax; i++)
    {
    /* 1 */
    GAVL_MEMCPY(dst_y, src_y, y_size);
    GAVL_MEMCPY(dst_u, src_u, uv_size);
    GAVL_MEMCPY(dst_v, src_v, uv_size);
    
    dst_y += ctx->output_frame->strides[0];
    
    src_y += ctx->input_frame->strides[0];
    src_u += ctx->input_frame->strides[1];
    src_v += ctx->input_frame->strides[2];
    /* 2 */
    GAVL_MEMCPY(dst_y, src_y, y_size);
    
    dst_y += ctx->output_frame->strides[0];
    
    src_y += ctx->input_frame->strides[0];
    src_u += ctx->input_frame->strides[1];
    src_v += ctx->input_frame->strides[2];

    /* 3 */
    GAVL_MEMCPY(dst_y, src_y, y_size);
    
    dst_y += ctx->output_frame->strides[0];
    
    src_y += ctx->input_frame->strides[0];
    src_u += ctx->input_frame->strides[1];
    src_v += ctx->input_frame->strides[2];
    /* 4 */
    GAVL_MEMCPY(dst_y, src_y, y_size);
    
    dst_y += ctx->output_frame->strides[0];
    
    src_y += ctx->input_frame->strides[0];
    src_u += ctx->input_frame->strides[1];
    src_v += ctx->input_frame->strides[2];

    /* Increment dst chroma */
    
    dst_u += ctx->output_frame->strides[1];
    dst_v += ctx->output_frame->strides[2];
    }
  }
