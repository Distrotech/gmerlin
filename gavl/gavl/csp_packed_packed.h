/* Packed -> Packed conversion */

/*
 *  Needs the following macros:
 *  IN_TYPE:     Type of the input pointer
 *  OUT_TYPE:    Type of the output pointer
 *  IN_ADVANCE:  Input advance
 *  OUT_ADVANCE: Output advance
 *  FUNC_NAME:   Name of the function
 *  NUM_PIXELS:  The number of pixels the conversion processes at once
 *  CONVERT:     This macro takes no arguments and makes the appropriate conversion
 *               from <src> to <dst>
 *  INIT:        Variable declarations and initialization (Optional)
 *  CLEANUP:     Stuff at the end (Optional)
 */

static void (FUNC_NAME)(gavl_video_convert_context_t * ctx)
  {
  int i;
  int j, jmax;
  IN_TYPE  * src;
  OUT_TYPE * dst;
  uint8_t * src_save;
  uint8_t * dst_save;
#ifdef INIT
  INIT
#endif
  src_save = ctx->input_frame->planes[0];
  dst_save = ctx->output_frame->planes[0];
  jmax = ctx->input_format.image_width / NUM_PIXELS;
  for(i = 0; i < ctx->input_format.image_height; i++)
    {
    src = (IN_TYPE*)src_save;
    dst = (OUT_TYPE*)dst_save;

    for(j = 0; j < jmax; j++)
      {
      CONVERT
      src += IN_ADVANCE;
      dst += OUT_ADVANCE;
      }
    src_save += ctx->input_frame->strides[0];
    dst_save += ctx->output_frame->strides[0];
    }

#ifdef CLEANUP
  CLEANUP
#endif

  }

#undef IN_TYPE 
#undef OUT_TYPE
#undef IN_ADVANCE
#undef OUT_ADVANCE
#undef NUM_PIXELS
#undef FUNC_NAME
#undef CONVERT

#ifdef INIT
#undef INIT
#endif

#ifdef CLEANUP
#undef CLEANUP
#endif
