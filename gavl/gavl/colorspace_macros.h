/*****************************************************************

  colorspace_macros.h

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
 *  These define all colorspace conversion loops
 *  They are used by the C and mmx(ext) versions
 *
 *  The following macros are defined:
 *
 *  CONVERSION_FUNC_START_PLANAR_PACKED(advance_x, advance_y, input_type, output_type)
 *  CONVERSION_FUNC_START_PACKED_PLANAR(advance_x, advance_y, input_type, output_type)
 *  CONVERSION_FUNC_START_PLANAR_PLANAR(advance_x, advance_y, input_type, output_type)
 *  CONVERSION_FUNC_START_PACKED_PACKED(advance_x, advance_y, input_type, output_type)
 *
 *  The advance_x and advance_y values inicate, how many pixels (advance_x) or scanlines
 *  (advance_y) your routines process at once. input_type and output_type are the datatypes
 *  you want to have for your input and output pixels. For RGB16 formats, this is
 *  e.g. uint16_t.
 *
 *  Their counterparts are:
 *
 *  CONVERSION_FUNC_END_PLANAR_PACKED
 *  CONVERSION_FUNC_END_PACKED_PLANAR
 *  CONVERSION_FUNC_END_PACKED_PACKED
 *  CONVERSION_FUNC_END_PLANAR_PLANAR
 *
 *  
 *
 */

#ifdef SCANLINE

#define CONVERSION_FUNC_START_PLANAR_PACKED(input_type, output_type, advance_x, advance_y) \
input_type * end_ptr;\
int units_per_line = ctx->input_frame->strides[0]/sizeof(input_type);\
input_type  * src_y = (input_type*)ctx->input_frame->planes[0];\
input_type  * src_u = (input_type*)ctx->input_frame->planes[1];\
input_type  * src_v = (input_type*)ctx->input_frame->planes[2];\
output_type * dst = (output_type*)ctx->input_frame->planes[0];

#define CONVERSION_FUNC_END_PLANAR_PACKED 

#define CONVERSION_LOOP_START_PLANAR_PACKED(input_type, output_type) 

#define CONVERSION_FUNC_START_PACKED_PACKED(input_type, output_type, advance_x, advance_y) \
input_type * end_ptr;\
int units_per_line = ctx->input_frame->strides[0]/sizeof(input_type);\
input_type  * src = (input_type*)ctx->input_frame->planes[0];\
output_type * dst = (output_type*)ctx->output_frame->planes[0];

#define CONVERSION_FUNC_END_PACKED_PACKED

#define CONVERSION_LOOP_START_PACKED_PACKED(input_type, output_type)

#define CONVERSION_FUNC_START_PACKED_PLANAR(input_type, output_type, advance_x, advance_y) \
input_type * end_ptr;\
int units_per_line = ctx->input_frame->strides[0]/sizeof(input_type);\
input_type * src = (input_type*)ctx->input_frame->planes[0];\
output_type * dst_y = (output_type*)ctx->output_frame->planes[0];\
output_type * dst_u = (output_type*)ctx->output_frame->planes[1];\
output_type * dst_v = (output_type*)ctx->output_frame->planes[2];

#define CONVERSION_FUNC_END_PACKED_PLANAR

#define CONVERSION_LOOP_START_PACKED_PLANAR(input_type, output_type)

#define CONVERSION_FUNC_START_PLANAR_PLANAR(input_type, output_type, advance_x, advance_y) \
input_type * end_ptr;\
int units_per_line = ctx->input_frame->planes[0]_stride/sizeof(input_type);\
input_type * src_y = (input_type*)ctx->input_frame->planes[0];\
input_type * src_u = (input_type*)ctx->input_frame->planes[1];\
input_type * src_v = (input_type*)ctx->input_frame->planes[2];\
output_type * dst_y = (output_type*)ctx->output_frame->planes[0];\
output_type * dst_u = (output_type*)ctx->output_frame->planes[1];\
output_type * dst_v = (output_type*)ctx->output_frame->planes[2];

#define CONVERSION_FUNC_END_PLANAR_PLANAR

#define CONVERSION_LOOP_START_PLANAR_PLANAR(input_type, output_type)

#else /* !SCANLINE */

/* Planar -> Packed conversion */

#define CONVERSION_FUNC_START_PLANAR_PACKED(input_type, output_type, advance_x, advance_y) \
int i;\
input_type * end_ptr;\
int imax = ctx->input_format.image_height/advance_y;\
int units_per_line = ctx->input_frame->strides[0]/sizeof(input_type);\
input_type * src_y;\
input_type * src_u;\
input_type * src_v;\
output_type * dst;\
uint8_t * src_y_save = ctx->input_frame->planes[0];\
uint8_t * src_u_save = ctx->input_frame->planes[1];\
uint8_t * src_v_save = ctx->input_frame->planes[2];\
uint8_t * dst_save =   ctx->output_frame->planes[0];

#define CONVERSION_LOOP_START_PLANAR_PACKED(input_type, output_type) \
for(i = 0; i < imax; i++){\
src_y = src_y_save;\
src_u = src_u_save;\
src_v = src_v_save;\
dst =   (output_type*)dst_save;
                                            
#define CONVERSION_FUNC_END_PLANAR_PACKED \
src_y_save += ctx->input_frame->strides[0];\
src_u_save += ctx->input_frame->strides[1];\
src_v_save += ctx->input_frame->strides[2];\
dst_save += ctx->output_frame->strides[0];}

/* Planar -> Planar conversion */

#define CONVERSION_FUNC_START_PLANAR_PLANAR(input_type, output_type, advance_x, advance_y) \
int i;\
input_type * end_ptr;\
int imax = ctx->input_format.image_height/advance_y;\
int units_per_line = ctx->input_frame->strides[0]/sizeof(input_type);\
input_type * src_y;\
input_type * src_u;\
input_type * src_v;\
output_type * dst_y;\
output_type * dst_u;\
output_type * dst_v;\
uint8_t * src_y_save = (input_type*)ctx->input_frame->planes[0];\
uint8_t * src_u_save = (input_type*)ctx->input_frame->planes[1];\
uint8_t * src_v_save = (input_type*)ctx->input_frame->planes[2];\
uint8_t * dst_y_save = (output_type*)ctx->output_frame->planes[0];\
uint8_t * dst_u_save = (output_type*)ctx->output_frame->planes[1];\
uint8_t * dst_v_save = (output_type*)ctx->output_frame->planes[2];

#define CONVERSION_LOOP_START_PLANAR_PLANAR(input_type, output_type) \
for(i = 0; i < imax; i++){\
src_y = (input_type*)src_y_save;\
src_u = (input_type*)src_u_save;\
src_v = (input_type*)src_v_save;\
dst_y = (output_type*)dst_y_save;\
dst_u = (output_type*)dst_u_save;\
dst_v = (output_type*)dst_v_save;

#define CONVERSION_FUNC_END_PLANAR_PLANAR \
src_y_save += ctx->input_frame->strides[0];\
src_u_save += ctx->input_frame->strides[1];\
src_v_save += ctx->input_frame->strides[2];\
dst_y_save += ctx->output_frame->strides[0];\
dst_u_save += ctx->output_frame->strides[1];\
dst_v_save += ctx->output_frame->strides[2];}

/* Packed -> Planar conversion */

#define CONVERSION_FUNC_START_PACKED_PLANAR(input_type, output_type, advance_x, advance_y) \
int i;\
input_type * end_ptr;\
input_type * src;\
output_type * dst_y;\
output_type * dst_u;\
output_type * dst_v;\
int imax = ctx->input_format.image_height/advance_y;\
int units_per_line = ctx->input_frame->strides[0]/sizeof(input_type);\
uint8_t * src_save = ctx->input_frame->planes[0];\
uint8_t * dst_y_save = ctx->output_frame->planes[0];\
uint8_t * dst_u_save = ctx->output_frame->planes[1];\
uint8_t * dst_v_save = ctx->output_frame->planes[2];

#define CONVERSION_LOOP_START_PACKED_PLANAR(input_type, output_type) \
for(i = 0; i < imax; i++){\
src =   (input_type*)src_save;\
dst_y = (output_type*)dst_y_save;\
dst_u = (output_type*)dst_u_save;\
dst_v = (output_type*)dst_v_save;

#define CONVERSION_FUNC_END_PACKED_PLANAR \
dst_y_save += ctx->output_frame->strides[0];\
dst_u_save += ctx->output_frame->strides[1];\
dst_v_save += ctx->output_frame->strides[2];\
src_save += ctx->input_frame->strides[0];}

/* Packed -> Packed conversion */

#define CONVERSION_FUNC_START_PACKED_PACKED(input_type, output_type, advance_x, advance_y) \
int i;\
input_type * end_ptr;\
input_type * src;\
output_type * dst;\
int imax = ctx->input_format.image_height/advance_y;\
int units_per_line = ctx->input_frame->strides[0]/sizeof(input_type);\
uint8_t * src_save = ctx->input_frame->planes[0];\
uint8_t * dst_save = ctx->output_frame->planes[0];

#define CONVERSION_LOOP_START_PACKED_PACKED(input_type, output_type) \
for(i = 0; i < imax; i++){\
src = (input_type*)src_save;\
dst = (output_type*)dst_save;

#define CONVERSION_FUNC_END_PACKED_PACKED \
dst_save += ctx->output_frame->strides[0];\
src_save += ctx->input_frame->strides[0];}

#define CONVERSION_FUNC_MIDDLE_PACKED_TO_420(input_type, output_type) \
dst_y_save += ctx->output_frame->strides[0];\
src_save += ctx->input_frame->strides[0];\
src =   (input_type*)src_save;\
dst_y = (output_type*)dst_y_save;

#define CONVERSION_FUNC_MIDDLE_420_TO_PACKED(input_type, output_type) \
src_y_save += ctx->input_frame->strides[0];\
dst_save += ctx->output_frame->strides[0];\
src_y = (input_type*)src_y_save;\
src_u = (input_type*)src_u_save;\
src_v = (input_type*)src_v_save;\
dst = (output_type*)dst_save;

#endif /* SCANLINE */

#define SCANLINE_LOOP_START_PACKED \
end_ptr = src + units_per_line;\
while(src < end_ptr){

#define SCANLINE_LOOP_START_PLANAR \
end_ptr = src_y + units_per_line;\
while(src_y < end_ptr){

#define SCANLINE_LOOP_END_PACKED_PACKED(src_advance, dst_advance) \
src+=src_advance;\
dst+=dst_advance;\
}

#define SCANLINE_LOOP_END_PACKED_PLANAR(src_advance, dst_y_advance, dst_uv_advance) \
src+=src_advance;\
dst_y+=dst_y_advance;\
dst_u+=dst_uv_advance;\
dst_v+=dst_uv_advance;\
}

#define SCANLINE_LOOP_END_PACKED_TO_420_Y(src_advance, dst_y_advance) \
src+=src_advance;\
dst_y+=dst_y_advance;\
}


#define SCANLINE_LOOP_END_PLANAR_PACKED(src_y_advance, src_uv_advance, dst_advance) \
dst+=dst_advance;\
src_y+=src_y_advance;\
src_u+=src_uv_advance;\
src_v+=src_uv_advance;\
}

#define SCANLINE_LOOP_END_PLANAR_PLANAR(src_y_advance, src_uv_advance, dst_y_advance, dst_uv_advance) \
src_y+=src_y_advance;\
src_u+=src_uv_advance;\
src_v+=src_uv_advance;\
dst_y+=dst_y_advance;\
dst_u+=dst_uv_advance;\
dst_v+=dst_uv_advance;\
}

