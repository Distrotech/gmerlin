/*****************************************************************

  blend.h

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

typedef void (*gavl_blend_func_t)(gavl_overlay_blend_context_t * ctx,
                                  gavl_video_frame_t * frame,
                                  gavl_video_frame_t * overlay);

struct gavl_overlay_blend_context_s
  {
  gavl_video_frame_t * cnv_in_frame;
  
  gavl_video_format_t dst_format;
  gavl_video_format_t ovl_format;
  gavl_blend_func_t func;

  gavl_overlay_t ovl;

  int has_overlay;
  int do_convert;
  
  gavl_video_frame_t * ovl_win;
  gavl_video_frame_t * dst_win;

  gavl_rectangle_i_t dst_rect;
    
  gavl_video_options_t opt;
  gavl_video_convert_context_t * cnv;

  /* Chroma subsampling of the destination format */  
  int dst_sub_h, dst_sub_v;
  
  };

gavl_blend_func_t
gavl_find_blend_func_c(gavl_overlay_blend_context_t * ctx,
                       gavl_pixelformat_t frame_format,
                       gavl_pixelformat_t * overlay_format);

                       
