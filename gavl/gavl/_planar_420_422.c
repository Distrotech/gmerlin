/*****************************************************************

  _planar_420_422.c 

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
 *  The conversion routines between YUV 420 Planar and YUV 420 Planar
 *  differ only by their memcpy implementations 
 */

static void yuv_420_p_to_yuv_422_p_c(gavl_video_convert_context_t * ctx)
  {
  int i;
  int y_size =
    ctx->input_frame->y_stride < ctx->output_frame->y_stride ?
    ctx->input_frame->y_stride : ctx->output_frame->y_stride;

  int uv_size =
    ctx->input_frame->u_stride < ctx->output_frame->u_stride ?
    ctx->input_frame->u_stride : ctx->output_frame->u_stride;
  int imax = ctx->input_format.height/2;
  
  uint8_t * src_y = ctx->input_frame->y;
  uint8_t * src_u = ctx->input_frame->u;
  uint8_t * src_v = ctx->input_frame->v;
  uint8_t * dst_y = ctx->output_frame->y;
  uint8_t * dst_u = ctx->output_frame->u;
  uint8_t * dst_v = ctx->output_frame->v;

  for(i = 0; i < imax; i++)
    {
    GAVL_MEMCPY(dst_y, src_y, y_size);
    GAVL_MEMCPY(dst_u, src_u, uv_size);
    GAVL_MEMCPY(dst_v, src_v, uv_size);
    
    dst_y += ctx->output_frame->y_stride;
    dst_u += ctx->output_frame->u_stride;
    dst_v += ctx->output_frame->v_stride;
    
    src_y += ctx->input_frame->y_stride;

    GAVL_MEMCPY(dst_y, src_y, y_size);
    GAVL_MEMCPY(dst_u, src_u, uv_size);
    GAVL_MEMCPY(dst_v, src_v, uv_size);
    
    dst_y += ctx->output_frame->y_stride;
    dst_u += ctx->output_frame->u_stride;
    dst_v += ctx->output_frame->v_stride;
    
    src_y += ctx->input_frame->y_stride;
    src_u += ctx->input_frame->u_stride;
    src_v += ctx->input_frame->v_stride;
    
    }
  }

static void yuv_422_p_to_yuv_420_p_c(gavl_video_convert_context_t * ctx)
  {
  int i;
  int y_size =
    ctx->input_frame->y_stride < ctx->output_frame->y_stride ?
    ctx->input_frame->y_stride : ctx->output_frame->y_stride;

  int uv_size =
    ctx->input_frame->u_stride < ctx->output_frame->u_stride ?
    ctx->input_frame->u_stride : ctx->output_frame->u_stride;
  int imax = ctx->input_format.height/2;
  
  uint8_t * src_y = ctx->input_frame->y;
  uint8_t * src_u = ctx->input_frame->u;
  uint8_t * src_v = ctx->input_frame->v;
  uint8_t * dst_y = ctx->output_frame->y;
  uint8_t * dst_u = ctx->output_frame->u;
  uint8_t * dst_v = ctx->output_frame->v;

  for(i = 0; i < imax; i++)
    {
    
    GAVL_MEMCPY(dst_y, src_y, y_size);
    GAVL_MEMCPY(dst_u, src_u, uv_size);
    GAVL_MEMCPY(dst_v, src_v, uv_size);
    
    dst_y += ctx->output_frame->y_stride;
    
    src_y += ctx->input_frame->y_stride;
    src_u += ctx->input_frame->u_stride;
    src_v += ctx->input_frame->v_stride;

    GAVL_MEMCPY(dst_y, src_y, y_size);
    
    dst_y += ctx->output_frame->y_stride;
    dst_u += ctx->output_frame->u_stride;
    dst_v += ctx->output_frame->v_stride;
    
    src_y += ctx->input_frame->y_stride;
    src_u += ctx->input_frame->u_stride;
    src_v += ctx->input_frame->v_stride;
    
    }
  

  }
