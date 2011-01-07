/*****************************************************************
 * gavl - a general purpose audio/video processing library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

/* Packed -> Planar conversion */

/*
 *  Needs the following macros:
 *  IN_TYPE:        Type of the input pointers
 *  OUT_TYPE:       Type of the output pointers
 *  IN_ADVANCE:     Input advance
 *  OUT_ADVANCE_Y:  Output advance
 *  OUT_ADVANCE_UV: Output advance
 *  FUNC_NAME:      Name of the function
 *  NUM_PIXELS:     The number of pixels the conversion processes at once
 *  CONVERT_YUV:    Makes the appropriate conversion
 *                  from <src> to <dst> for luma and chroma
 *  CONVERT_Y:      Makes the appropriate conversion
 *                  from <src> to <dst> for luma only (only if CHROMA_SUB > 1)
 *  CHROMA_SUB:     Vertical chroma subsampling factor
 *  INIT:        Variable declarations and initialization (Optional)
 *  CLEANUP:     Stuff at the end (Optional)
 */

static void (FUNC_NAME)(gavl_video_convert_context_t * ctx)
  {
#ifndef SCANLINE
  int i, imax;
  uint8_t * src_save;
  uint8_t * dst_save_y;
  uint8_t * dst_save_u;
  uint8_t * dst_save_v;
#endif
  int j, jmax;
  IN_TYPE  * src;
  OUT_TYPE * dst_y;
  OUT_TYPE * dst_u;
  OUT_TYPE * dst_v;

#ifdef INIT
  INIT
#endif

#ifndef SCANLINE
  src_save = ctx->input_frame->planes[0];
  dst_save_y = ctx->output_frame->planes[0];
  dst_save_u = ctx->output_frame->planes[1];
  dst_save_v = ctx->output_frame->planes[2];
#else
  src   = (IN_TYPE*)ctx->input_frame->planes[0];
  dst_y = (OUT_TYPE*)ctx->output_frame->planes[0];
  dst_u = (OUT_TYPE*)ctx->output_frame->planes[1];
  dst_v = (OUT_TYPE*)ctx->output_frame->planes[2];
#endif
  
  jmax = ctx->input_format.image_width  / NUM_PIXELS;
#ifndef SCANLINE
  imax = ctx->input_format.image_height / CHROMA_SUB;
  for(i = 0; i < imax; i++)
    {
    src =    (IN_TYPE*)src_save;
    dst_y = (OUT_TYPE*)dst_save_y;
    dst_u = (OUT_TYPE*)dst_save_u;
    dst_v = (OUT_TYPE*)dst_save_v;
#endif /* !SCANLINE */
    
GAVL_LOOP_HEAD(j, jmax)
      CONVERT_YUV
      src += IN_ADVANCE;
      dst_y += OUT_ADVANCE_Y;
      dst_u += OUT_ADVANCE_UV;
      dst_v += OUT_ADVANCE_UV;
GAVL_LOOP_TAIL

#ifndef SCANLINE
    dst_save_y += ctx->output_frame->strides[0];
    dst_save_u += ctx->output_frame->strides[1];
    dst_save_v += ctx->output_frame->strides[2];
    src_save += ctx->input_frame->strides[0];
    
#if CHROMA_SUB > 1
    src =    (IN_TYPE*)src_save;
    dst_y = (OUT_TYPE*)dst_save_y;
    
GAVL_LOOP_HEAD(j, jmax)
      CONVERT_Y
      src += IN_ADVANCE;
      dst_y += OUT_ADVANCE_Y;
GAVL_LOOP_TAIL

    dst_save_y += ctx->output_frame->strides[0];
    src_save += ctx->input_frame->strides[0];

#endif
    
#if CHROMA_SUB > 2
    src =    (IN_TYPE*)src_save;
    dst_y = (OUT_TYPE*)dst_save_y;
    
GAVL_LOOP_HEAD(j, jmax)
      CONVERT_Y
      src += IN_ADVANCE;
      dst_y += OUT_ADVANCE_Y;
GAVL_LOOP_TAIL

    dst_save_y += ctx->output_frame->strides[0];
    src_save += ctx->input_frame->strides[0];
#endif

#if CHROMA_SUB > 3
    src =    (IN_TYPE*)src_save;
    dst_y = (OUT_TYPE*)dst_save_y;
    
GAVL_LOOP_HEAD(j, jmax)
      CONVERT_Y
      src += IN_ADVANCE;
      dst_y += OUT_ADVANCE_Y;
GAVL_LOOP_TAIL

  dst_save_y += ctx->output_frame->strides[0];
  src_save += ctx->input_frame->strides[0];
#endif
  }
#endif /* !SCANLINE */

#ifdef CLEANUP
  CLEANUP
#endif
  
  }

#undef FUNC_NAME      // Name of the function
#undef IN_TYPE        // Type of the input pointers
#undef OUT_TYPE       // Type of the output pointers
#undef IN_ADVANCE     // Input advance
#undef OUT_ADVANCE_Y  // Output advance
#undef OUT_ADVANCE_UV // Output advance
#undef NUM_PIXELS     // The number of pixels the conversion processes at once
#undef CONVERT_YUV    // Makes the appropriate conversion
                      // from <src> to <dst> for luma and chroma
#ifdef CONVERT_Y
#undef CONVERT_Y      // Makes the appropriate conversion
                      // from <src> to <dst> for luma only
#endif
#undef CHROMA_SUB     // Vertical chroma subsampling factor

#ifdef INIT
#undef INIT
#endif

#ifdef CLEANUP
#undef CLEANUP
#endif
