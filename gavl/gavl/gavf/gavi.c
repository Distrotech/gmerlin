#include <stdlib.h>
#include <string.h>


#include <gavfprivate.h>

/*
 *  Layout of a gavi file:
 *
 *  GAVLIMAG: Signature
 *  metadata
 *  format
 *  frame
 */

static const uint8_t sig[8] = "GAVLIMAG";

int gavl_image_write_header(gavf_io_t * io,
                            const gavl_metadata_t * m,
                            const gavl_video_format_t * v)
  {
  if((gavf_io_write_data(io, sig, 8) < 8) ||
     !gavf_write_metadata(io, m) ||
     !gavf_write_video_format(io, v))
    return 0;
  return 1;
  }

int gavl_image_write_image(gavf_io_t * io,
                           const gavl_video_format_t * v,
                           gavl_video_frame_t * f)
  {
  int len = gavl_video_format_get_image_size(v);
  if((gavf_io_write_data(io, f->planes[0], len) < len))
    return 0;
  return 1;
  }

int gavl_image_read_header(gavf_io_t * io,
                           gavl_metadata_t * m,
                           gavl_video_format_t * v)
  {
  uint8_t sig_test[8];
  if((gavf_io_read_data(io, sig_test, 8) < 8) ||
     memcmp(sig_test, sig, 8) ||
     !gavf_read_metadata(io, m) ||
     !gavf_read_video_format(io, v))
    return 0;
  return 1;
  }

int gavl_image_read_image(gavf_io_t * io,
                          gavl_video_format_t * v,
                          gavl_video_frame_t * f)
  {
  int len = gavl_video_format_get_image_size(v);
  if(gavf_io_read_data(io, f->planes[0], len) < len)
    return 0;
  return 1;
  }
