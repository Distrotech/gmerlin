/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <string.h>
#include <math.h>
#include <mjpeg_types.h>
#include <yuv4mpeg.h>
#include <mjpeg_logging.h>
#include "motionsearch.h"

#include <gavl/gavl.h>

#include <yuvdeinterlace.h>


#ifdef __GNUC__
#define RESTRICT __restrict__
#else
#define RESTRICT
#endif

struct yuvdeinterlacer_s
  {
  int width;
  int height;
  int cwidth;
  int cheight;
  int field_order;
  int both_fields;
  int verbose;
  int input_chroma_subsampling;
  int output_chroma_subsampling;
  int vertical_overshot_luma;
  int vertical_overshot_chroma;
  int just_anti_alias;

  //  y4mstream Y4MStream;

  //  uint8_t *inframe[3];
  //  uint8_t *inframe0[3];
  //  uint8_t *inframe1[3];

  //  uint8_t *outframe[3];

  uint8_t * RESTRICT scratch;
  uint8_t * RESTRICT mmap;

  int (* RESTRICT motion_1[2])[2];
  int (* RESTRICT motion_2[2])[2];

  /* Added for gmerlin */
  gavl_video_frame_t * inframe;
  gavl_video_frame_t * inframe0;
  gavl_video_frame_t * inframe1;
  gavl_video_frame_t * outframe;

  gavl_video_frame_t * inframe_real;
  gavl_video_frame_t * inframe0_real;
  gavl_video_frame_t * inframe1_real;
  gavl_video_frame_t * outframe_real;

  gavl_video_format_t format;

  int scratch_offset; // Added to the adress after malloc, substracted before free()
  
  /* Where to get data */
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;

  int need_restart;
  int current_field;
  int64_t frame;
  int eof;
  
  /* Multithreading stuff */

  gavl_video_run_func  run_func;
  gavl_video_stop_func stop_func;
  void * run_data;
  void * stop_data;
  int num_threads;
  
  struct
    {
    int w, h;
    uint8_t * out;
    const uint8_t * in;
    const uint8_t * in0;
    const uint8_t * in1;
    yuvdeinterlacer_t * di;
    int (* RESTRICT lvxy_1)[2];
    int (* RESTRICT lvxy_2)[2];
    int field;
    } cur;
  
  };

void antialias_plane (yuvdeinterlacer_t * di, uint8_t * RESTRICT out, int w, int h);


void yuvdeinterlacer_connect_input(void * priv,
                                   bg_read_video_func_t func,
                                   void * data, int stream)
  {
  yuvdeinterlacer_t * di = (yuvdeinterlacer_t *)priv;
  di->read_func = func;
  di->read_data = data;
  di->read_stream = stream;
  }

#if 0 // Original 
void initialize_memory (yuvdeinterlacer_t * di, int w, int h, int cw, int ch)
  {
    int luma_size;
    int chroma_size;
    int lmotion_size;
    int cmotion_size;

    // Some functions need some vertical overshoot area
    // above and below the image. So we make the buffer
    // a little bigger...
      vertical_overshot_luma = 32 * w;
      vertical_overshot_chroma = 32 * cw;
      luma_size = (w * h) + 2 * vertical_overshot_luma;
      chroma_size = (cw * ch) + 2 * vertical_overshot_chroma;
      lmotion_size = ((w + 7) / 8) * ((h + 7) / 8);
      cmotion_size = ((cw + 7) / 8) * ((ch + 7) / 8);

      inframe[0] = (uint8_t *) calloc (luma_size, 1) + vertical_overshot_luma;
      inframe[1] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;
      inframe[2] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;

      inframe0[0] = (uint8_t *) calloc (luma_size, 1) + vertical_overshot_luma;
      inframe0[1] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;
      inframe0[2] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;

      inframe1[0] = (uint8_t *) calloc (luma_size, 1) + vertical_overshot_luma;
      inframe1[1] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;
      inframe1[2] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;

      outframe[0] = (uint8_t *) calloc (luma_size, 1) + vertical_overshot_luma;
      outframe[1] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;
      outframe[2] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;

      scratch = (uint8_t *) malloc (luma_size) + vertical_overshot_luma;
      mmap = (uint8_t *) malloc (w * h);

      motion[0] = calloc (lmotion_size, sizeof(motion[0][0]));
      motion[1] = calloc (cmotion_size, sizeof(motion[0][0]));

      width = w;
      height = h;
      cwidth = cw;
      cheight = ch;
  }
#else
#define OVERSHOOT 32

static void initialize_memory(yuvdeinterlacer_t * d)
  {
  int luma_size;
  int chroma_size;
  int lmotion_size;
  int cmotion_size;
  
  gavl_rectangle_i_t rect;
  int w, h, cw, ch, sub_h, sub_v;

  int vertical_overshot_luma;
  int vertical_overshot_chroma;
  
  gavl_video_format_t frame_format;
  
  gavl_pixelformat_chroma_sub(d->format.pixelformat, &sub_h, &sub_v);

  d->need_restart = 0;
  
  w = d->format.image_width;
  h = d->format.image_height;
  
  cw = w / sub_h;
  ch = h / sub_v;

  gavl_video_format_copy(&frame_format, &d->format);

  frame_format.frame_height = frame_format.image_height + 2*OVERSHOOT;
  frame_format.frame_width  = frame_format.image_width;
  
  // Some functions need some vertical overshoot area
  // above and below the image. So we make the buffer
  // a little bigger...
  vertical_overshot_luma = OVERSHOOT * w;
  vertical_overshot_chroma = OVERSHOOT * cw;
  luma_size = (w * h) + 2 * vertical_overshot_luma;
  chroma_size = (cw * ch) + 2 * vertical_overshot_chroma;
  
  lmotion_size = ((w + 7) / 8) * ((h + 7) / 8);
  cmotion_size = ((cw + 7) / 8) * ((ch + 7) / 8);
  
  d->inframe_real = gavl_video_frame_create_nopad(&frame_format);
  d->inframe0_real = gavl_video_frame_create_nopad(&frame_format);
  d->inframe1_real = gavl_video_frame_create_nopad(&frame_format);
  d->outframe_real = gavl_video_frame_create_nopad(&frame_format);

  gavl_video_frame_clear(d->inframe_real, &frame_format);
  gavl_video_frame_clear(d->inframe0_real, &frame_format);
  gavl_video_frame_clear(d->inframe1_real, &frame_format);
  gavl_video_frame_clear(d->outframe_real, &frame_format);
  
  d->inframe = gavl_video_frame_create((gavl_video_format_t*)0);
  d->inframe0 = gavl_video_frame_create((gavl_video_format_t*)0);
  d->inframe1 = gavl_video_frame_create((gavl_video_format_t*)0);
  d->outframe = gavl_video_frame_create((gavl_video_format_t*)0);

  rect.x = 0;
  rect.y = OVERSHOOT;
  rect.w = frame_format.image_width;
  rect.h = frame_format.image_height;

  gavl_video_frame_get_subframe(d->format.pixelformat, d->inframe_real, d->inframe, &rect);
  gavl_video_frame_get_subframe(d->format.pixelformat, d->inframe0_real, d->inframe0, &rect);
  gavl_video_frame_get_subframe(d->format.pixelformat, d->inframe1_real, d->inframe1, &rect);
  gavl_video_frame_get_subframe(d->format.pixelformat, d->outframe_real, d->outframe, &rect);
  
  d->scratch = (uint8_t *) calloc (1, luma_size + vertical_overshot_luma);
  d->mmap = (uint8_t *) calloc (1, luma_size);

  d->motion_1[0] = calloc (lmotion_size, sizeof(d->motion_1[0][0]));
  d->motion_1[1] = calloc (cmotion_size, sizeof(d->motion_1[0][0]));
  d->motion_2[0] = calloc (lmotion_size, sizeof(d->motion_1[0][0]));
  d->motion_2[1] = calloc (cmotion_size, sizeof(d->motion_1[0][0]));
  
  d->scratch_offset = vertical_overshot_luma;
  d->scratch += d->scratch_offset;
  d->width = w;
  d->height = h;
  d->cwidth = cw;
  d->cheight = ch;

  
  }
#endif 

#if 0 // Original
  deinterlacer ()
  {
    both_fields = 0;
    just_anti_alias = 0;
  }
#else
yuvdeinterlacer_t * yuvdeinterlacer_create()
  {
  yuvdeinterlacer_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  // Make mjpeg logging quiet (it will only confuse users)
  mjpeg_default_handler_verbosity(0);
  
  // initialize motionsearch-library
  init_motion_search ();

#ifdef HAVE_ALTIVEC
  reset_motion_simd ("sad_00");
#endif

  return ret;
  }
#endif

#if 0
  ~deinterlacer ()
  {
    free (inframe[0] - vertical_overshot_luma);
    free (inframe[1] - vertical_overshot_chroma);
    free (inframe[2] - vertical_overshot_chroma);

    free (inframe0[0] - vertical_overshot_luma);
    free (inframe0[1] - vertical_overshot_chroma);
    free (inframe0[2] - vertical_overshot_chroma);

    free (inframe1[0] - vertical_overshot_luma);
    free (inframe1[1] - vertical_overshot_chroma);
    free (inframe1[2] - vertical_overshot_chroma);

    free (outframe[0] - vertical_overshot_luma);
    free (outframe[1] - vertical_overshot_chroma);
    free (outframe[2] - vertical_overshot_chroma);

    free (scratch - vertical_overshot_luma);
    free (mmap);

    free (motion[0]);
    free (motion[1]);
    
  }
#endif

#define FREE_FRAME(f) \
if(f) \
  { \
  gavl_video_frame_destroy(f); \
  f = (gavl_video_frame_t*)0; \
  }

#define FREE_FRAME_NULL(f) \
if(f) \
  { \
  gavl_video_frame_null(f); \
  gavl_video_frame_destroy(f); \
  f = (gavl_video_frame_t*)0; \
  }

static void free_memory(yuvdeinterlacer_t * d)
  {
  FREE_FRAME_NULL(d->inframe);
  FREE_FRAME_NULL(d->inframe0);
  FREE_FRAME_NULL(d->inframe1);
  FREE_FRAME_NULL(d->outframe);
  
  FREE_FRAME(d->inframe_real);
  FREE_FRAME(d->inframe0_real);
  FREE_FRAME(d->inframe1_real);
  FREE_FRAME(d->outframe_real);

  if(d->scratch)
    {
    d->scratch -= d->scratch_offset;
    free (d->scratch);
    d->scratch = NULL;
    }
  if(d->mmap)
    {
    free (d->mmap);
    d->mmap = NULL;
    }
  if(d->motion_1[0])
    {
    free (d->motion_1[0]);
    d->motion_1[0] = NULL;
    }
  if(d->motion_1[1])
    {
    free (d->motion_1[1]);
    d->motion_1[1] = NULL;
    }
  if(d->motion_2[0])
    {
    free (d->motion_2[0]);
    d->motion_2[0] = NULL;
    }
  if(d->motion_2[1])
    {
    free (d->motion_2[1]);
    d->motion_2[1] = NULL;
    }
  }

static void temporal_reconstruct_slice_1(void * p, int start, int end)
  {
  int x, y;
  yuvdeinterlacer_t * di = p;
  const uint8_t * const in = di->cur.in;
  const uint8_t * RESTRICT in0  = di->cur.in0;
  const uint8_t * const in1 = di->cur.in1;
  int w = di->cur.w;
  int_fast16_t a, b, c;
  
  for(y = start; y < end; y++)
    {
    for (x = 0; x < w; x++)
      {
      a  = abs( *(in +x+(y-1)*w)-*(in0+x+(y-1)*w) );
      a += abs( *(in1+x+(y-1)*w)-*(in0+x+(y-1)*w) );

      b  = abs( *(in +x+(y  )*w)-*(in0+x+(y  )*w) );
      b += abs( *(in1+x+(y  )*w)-*(in0+x+(y  )*w) );

      c  = abs( *(in +x+(y+1)*w)-*(in0+x+(y+1)*w) );
      c += abs( *(in1+x+(y+1)*w)-*(in0+x+(y+1)*w) );

      //      *(di->scratch+x+(y-1)*w) = *(in0+x+(y-1)*w);

      *(di->scratch+x+(y)*w) = *(in0+x+(y)*w);
      
      if( (a<16 || c<16) && b<16 ) // Pixel is static?
	{
	// Yes...
        //	*(di->scratch+x+(y  )*w) = *(in0+x+(y  )*w);
	*(di->mmap+x+y*w) = 255; // mark pixel as static in motion-map...
	}
      else
	{
	*(di->mmap+x+y*w) = 0; // mark pixel as moving in motion-map...
	}
      }
    }
  
  }

#define GET_SAD \
  sad = psad_00 (scratch_ptr, out_ptr + dx + dy * w, w, 16, min); \
  if (sad < min) \
    { \
    min = sad; \
    vx = dx; \
    vy = dy; \
    } \


static void temporal_reconstruct_plane(yuvdeinterlacer_t * di,
                                       uint8_t * RESTRICT out,
                                       const uint8_t * const in,
                                       uint8_t * RESTRICT in0,
                                       const uint8_t * const in1,
                                       int w, int h, int field,
                                       int (* RESTRICT lvxy)[2],
                                       int (* RESTRICT lvxy_dst)[2])
  {
  int_fast16_t x, y;
  int_fast16_t vx, vy, dx, dy, px, py;
  uint_fast16_t min, sad;
  int_fast16_t a, b;
  const int iw = (w + 7) / 8;
  int i, nt, delta, scanline;

  uint8_t * scratch_ptr, * out_ptr;
  
  // the ELA-algorithm overshots by one line above and below the
  // frame-size, so fill the ELA-overshot-area in the inframe to
  // ensure that no green or purple lines are generated...
  memcpy (in0 - w, in + w, w);
  memcpy (in0 + (w * h), in + (w * h) - 2 * w, w);
#if 0
  int w, h;
  uint8_t * out;
  uint8_t * in;
  uint8_t * in0;
  uint8_t * in1;
  yuvdeinterlacer_t * di;
  int (* RESTRICT lvxy)[2];
  int field;
#endif
  
  di->cur.w = w;
  di->cur.h = h;
  di->cur.in = in;
  di->cur.in0 = in0;
  di->cur.in1 = in1;
  di->cur.out = out;
  di->cur.field = field;
  
  nt = di->num_threads;
  if(nt > h/2)
    nt = h/2;

  //  nt = 1;
  
  delta = (h - (1 - field)) / nt;
  if(delta & 1)
    delta++;
  scanline = 1 - field;
  for(i = 0; i < nt - 1; i++)
    {
    di->run_func(temporal_reconstruct_slice_1,
                       di, scanline, scanline+delta, di->run_data, i);
    
    //        fprintf(stderr, "scanline: %d (%d)\n", ctx->scanline, ctx->dst_rect.h);
    scanline += delta;
    }
  di->run_func(temporal_reconstruct_slice_1,
               di, scanline, h, di->run_data, nt - 1);
      
  for(i = 0; i < nt; i++)
    di->stop_func(di->stop_data, i);
  
  //  temporal_reconstruct_slice_1(di, (1 - field), h);
  
  //  if(y == h)
  //    memcpy (di->scratch + w * (h - 1), in0 + w * (h - 1), w);

  // As we now have a rather good interpolation of how the reference frame
  // might have been looking for when it had been scanned progressively,
  // we try to motion-compensate the former reconstructed frame (the reconstruction
  // of the previous field) to the new frame
  //
  // The block-size may not be too large but to get halfway decent motion-vectors
  // we need to have a matching-size of 16x16 at least as most of the material will
  // be noisy...

  // we need some overshot areas, again
  memcpy (out - w, out, w);
  memcpy (out - w * 2, out, w);
  memcpy (out - w * 3, out, w);

  memcpy (out + (w * h), out + (w * h) - w, w);
  memcpy (out + (w * h) + w, out + (w * h) - w, w);
  memcpy (out + (w * h) + w * 2, out + (w * h) - w, w);
  memcpy (out + (w * h) + w * 3, out + (w * h) - w, w);

  memcpy (di->scratch - w, di->scratch, w);
  memcpy (di->scratch - w * 2, di->scratch, w);
  memcpy (di->scratch - w * 3, di->scratch, w);
  memcpy (di->scratch - w * 4, di->scratch, w);
  memset (di->scratch - w * 4 - 4, di->scratch[0], 4);

  memcpy (di->scratch + (w * h), di->scratch + (w * h) - w, w);
  memcpy (di->scratch + (w * h) + w, di->scratch + (w * h) - w, w);
  memcpy (di->scratch + (w * h) + w * 2, di->scratch + (w * h) - w, w);
  memcpy (di->scratch + (w * h) + w * 3, di->scratch + (w * h) - w, w);
  memset (di->scratch + (w * h) + w * 4, di->scratch[w * h - 1], 11);

  
  for (y = 0; y < h; y += 8)
    {
    for (x = 0; x < w; x += 8)
      {
      uint_fast16_t ix = (unsigned)x / 8;
      uint_fast16_t iy = (unsigned)y / 8;
      
      // offset x+y so we get an overlapped search
      x -= 4;
      y -= 4;

      out_ptr = out + w * y + x;
      scratch_ptr = di->scratch + w * y + x;
      
      // reset motion-search with the zero-motion-vector (0;0)
      min = psad_00 (di->scratch + x + y * w, out + x + y * w, w, 16, 16*16*255);
      vx = 0;
      vy = 0;

      // check some "hot" candidates first...

      // if possible check all-neighbors-interpolation-vector
      if (min > 512)
        if (iy && ix && ix < (iw - 1))
          {
          dx  = lvxy[ix - 1 + (iy - 1) * iw][0];
          dx += lvxy[ix     + (iy - 1) * iw][0];
          dx += lvxy[ix + 1 + (iy - 1) * iw][0];
          dx += lvxy[ix - 1 + (iy    ) * iw][0];
          dx += lvxy[ix     + (iy    ) * iw][0];
          dx /= 5;
		
          dy  = lvxy[ix - 1 + (iy - 1) * iw][1];
          dy += lvxy[ix     + (iy - 1) * iw][1];
          dy += lvxy[ix + 1 + (iy - 1) * iw][1];
          dy += lvxy[ix - 1 + (iy    ) * iw][1];
          dy += lvxy[ix     + (iy    ) * iw][1];
          dy /= 10;
          dy *= 2;

          GET_SAD;
          }

      // if possible check top-left-neighbor-vector
      if (min > 512)
        if (iy && ix)
          {
          dx = lvxy[ix - 1 + (iy - 1) * iw][0];
          dy = lvxy[ix - 1 + (iy - 1) * iw][1];
          GET_SAD;
          }

      // if possible check top-neighbor-vector
      if (min > 512)
        if (iy)
          {
          dx = lvxy[ix + (iy - 1) * iw][0];
          dy = lvxy[ix + (iy - 1) * iw][1];
          GET_SAD;
          }

      // if possible check top-right-neighbor-vector
      if (min > 512)
        if (iy && ix < (iw - 1))
          {
          dx = lvxy[ix + 1 + (iy - 1) * iw][0];
          dy = lvxy[ix + 1 + (iy - 1) * iw][1];
          GET_SAD;
          }

      // if possible check left-neighbor-vector
      if (min > 512)
        if (ix)
          {
          dx = lvxy[ix - 1 + iy * iw][0];
          dy = lvxy[ix - 1 + iy * iw][1];
          GET_SAD;
          }

      // check temporal-neighbor-vector
      if (min > 512)
        {
        dx = lvxy[ix + iy * iw][0];
        dy = lvxy[ix + iy * iw][1];
        GET_SAD;
        }

      // search for a better one...
      px = vx;
      py = vy;

      if( min>1024 ) // the found predicted location [px;py] is a bad match...
        {
        // Do a large but coarse diamond search arround the prediction-vector
        //
        //         X
        //        ---
        //       X-X-X
        //      -------
        //     -X-X-X-X-
        //    -----------
        //   X-X-X-O-X-X-X
        //    -----------
        //     -X-X-X-X-
        //      -------
        //       X-X-X
        //        ---
        //         X
        // 
        // The locations marked with an X are checked, only. The location marked with
        // an O was already checked before...
        //
        dx = px-2;
        dy = py;
        GET_SAD;

        dx = px+2;
        dy = py;

        GET_SAD;
        
        dx = px-4;
        dy = py;
        GET_SAD;

        dx = px+4;
        dy = py;
        GET_SAD;

        dx = px;
        dy = py-4;
        GET_SAD;

        dx = px;
        dy = py+4;
        GET_SAD;
        }
      // update prediction-vector
      px = vx;
      py = vy;

        {
        // only do a small diamond search here... Either we have passed the large
        // diamond search or the predicted vector was good enough
        //
        //     X
        //    ---
        //   XXOXX
        //    ---
        //     X
        // 
        // The locations marked with an X are checked, only. The location marked with
        // an O was already checked before...
        //

        dx = px-1;
        dy = py;
        GET_SAD;

        dx = px+1;
        dy = py;
        GET_SAD;

        dx = px-2;
        dy = py;
        GET_SAD;

        dx = px+2;
        dy = py;
        GET_SAD;

        dx = px;
        dy = py-2;
        GET_SAD;

        dx = px;
        dy = py+2;
        GET_SAD;
        }

        // store the found vector, so we can do a candidates-check...
        lvxy_dst[ix + iy * iw][0] = vx;
        lvxy_dst[ix + iy * iw][1] = vy;
        
        // remove x+y offset...
        x += 4;
        y += 4;
        
        // reconstruct that block in scratch
        // do so by using the lowpass (and alias-term) from the current field
        // and the highpass (and phase-inverted alias-term) from the previous frame(!)
        for (dy = (1 - field); dy < 8; dy += 2)
          {
          uint8_t * RESTRICT dest = di->scratch + x + (y + dy) * w;
          uint8_t * RESTRICT src1 = di->scratch + x + (y + dy) * w;
          uint8_t * RESTRICT src2 = out + (x + vx) + (y + dy + vy) * w;

          for (dx = 0; dx < 8; dx++)
            {
            a =
              src1[dx - 3 * w] * -1 +
              src1[dx - 1 * w] * +17 +
              src1[dx + 1 * w] * +17 +
              src1[dx + 3 * w] * -1;

            b =
              src2[dx - 3 * w] * +1 +
              src2[dx - 1 * w] * -17 +
              src2[dx + 0 * w] * +32 +
              src2[dx + 1 * w] * -17 +
              src2[dx + 3 * w] * +1;

            a = a + b;
            a = a < 0 ? 0 : (a > 8160 ? 255 : (unsigned)a / 32);

            dest[dx] = a;
            }
          }
      
      }
    }
    
  for (y = (1 - field); y < h; y += 2)
    for (x = 0; x < w; x++)
      {
      if ( *(di->mmap+x+y*w)==255 ) // if pixel is static
        {
        *(di->scratch + x + (y) * w) = *(in0+x+(y  )*w);
        }
      }
  
  memcpy (out, di->scratch, w * h);
  antialias_plane (di, out, w, h );
  }

static void scale_motion_vectors (yuvdeinterlacer_t * di, int nom, int rshift)
  {
  int i;

  for (i = (di->width / 8) * (di->height / 8); i--; )
    {
    di->motion_1[0][i][0] = (di->motion_1[0][i][0] * nom) >> rshift;
    di->motion_1[0][i][1] = (di->motion_1[0][i][1] * nom) >> rshift;
    }
	
  for (i = (di->cwidth / 8) * (di->cheight / 8); i--; )
    {
    di->motion_1[1][i][0] = (di->motion_1[1][i][0] * nom) >> rshift;
    di->motion_1[1][i][1] = (di->motion_1[1][i][1] * nom) >> rshift;
    }
  }

static void temporal_reconstruct_frame(yuvdeinterlacer_t * di,
                                       gavl_video_frame_t * in,
                                       gavl_video_frame_t * in0,
                                       gavl_video_frame_t * in1,
                                       int field)
  {
  int (* RESTRICT swp)[2];

  //  int * swp;
  temporal_reconstruct_plane(di,
                             di->outframe->planes[0],
                             in->planes[0],
                             in0->planes[0],
                             in1->planes[0],
                             di->width, di->height, field, di->motion_1[0], di->motion_2[0]);
  temporal_reconstruct_plane(di,
                             di->outframe->planes[1],
                             in->planes[1],
                             in0->planes[1],
                             in1->planes[1],
                             di->cwidth, di->cheight, field, di->motion_1[1], di->motion_2[1]);
  temporal_reconstruct_plane(di,
                             di->outframe->planes[2],
                             in->planes[2],
                             in0->planes[2],
                             in1->planes[2],
                             di->cwidth, di->cheight, field, di->motion_1[1], di->motion_2[1]);
  swp = di->motion_1[0];
  di->motion_1[0] = di->motion_2[0];
  di->motion_2[0] = swp;

  swp = di->motion_1[1];
  di->motion_1[1] = di->motion_2[1];
  di->motion_2[1] = swp;
  }

static void antialias_slice_1(void * p, int start, int end)
  {
  int x, y;
  int vx;
  uint_fast16_t sad;
  uint_fast16_t min;
  int dx;
  yuvdeinterlacer_t * di = p;
  
  int w = di->cur.w;

  uint8_t * scratch_ptr = di->scratch + start * w;

  uint8_t * out = di->cur.out + start * w;
  uint8_t * out_p = out + w; /* out + width */
  uint8_t * out_m = out - w; /* out - width */
  
  for (y = start; y < end; y++)
    {
    out = di->cur.out + y * w + 2;
    out_p = out + w; /* out + width */
    out_m = out - w; /* out - width */
    scratch_ptr = di->scratch + y * w + 2;
    
    for (x = 2; x < (w - 2); x++)
      {
      min = ~0;
      vx = 0;
      for (dx = -3; dx <= 3; dx++)
        {
        sad = 0;
        //      sad  = abs( *(out+(x+dx-3)+(y-1)*w) - *(out+(x-3)+(y+0)*w) );
        //      sad += abs( *(out+(x+dx-2)+(y-1)*w) - *(out+(x-2)+(y+0)*w) );
        sad += abs (*(out_m + (dx - 1)) - *(out + (- 1) ));
        sad += abs (*(out_m + (dx + 0)) - *(out + (+ 0) ));
        sad += abs (*(out_m + (dx + 1)) - *(out + (+ 1) ));
        //      sad += abs( *(out+(x+dx+2)+(y-1)*w) - *(out+(x+2)+(y+0)*w) );
        //      sad += abs( *(out+(x+dx+3)+(y-1)*w) - *(out+(x+3)+(y+0)*w) );

        //      sad += abs( *(out+(x-dx-3)+(y+1)*w) - *(out+(x-3)+(y+0)*w) );
        //      sad += abs( *(out+(x-dx-2)+(y+1)*w) - *(out+(x-2)+(y+0)*w) );
        sad += abs (*(out_p + (- dx - 1)) - *(out + (- 1)));
        sad += abs (*(out_p + (- dx + 0)) - *(out + (+ 0)));
        sad += abs (*(out_p + (- dx + 1)) - *(out + (+ 1)));
        //      sad += abs( *(out+(x-dx+2)+(y+1)*w) - *(out+(x+2)+(y+0)*w) );
        //      sad += abs( *(out+(x-dx+3)+(y+1)*w) - *(out+(x+3)+(y+0)*w) );

        if (sad < min)
          {
          min = sad;
          vx = dx;
          }
        }

      if (abs (vx) <= 1)
        *scratch_ptr =
          ((2 * *(out)) + *(out_m + (vx)) +  *(out_p + (- vx))) / 4;
      else if (abs (vx) <= 3)
        *scratch_ptr =
          (2 * *(out + (-1)) +
           4 * *(out + ( 0)) +
           2 * *(out + ( 1)) +
           1 * *(out_m + (+ vx - 1)) +
           2 * *(out_m + (+ vx + 0)) +
           1 * *(out_m + (+ vx + 1)) +
           1 * *(out_p + (- vx - 1)) +
           2 * *(out_p + (- vx + 0)) +
           1 * *(out_p + (- vx + 1))) / 16;
      else
        *scratch_ptr =
          (2 * *(out + (- 1)) +
           4 * *(out + (- 1)) +
           8 * *(out + (+ 0)) +
           4 * *(out + (+ 1)) +
           2 * *(out + (+ 1)) +
           1 * *(out_m + (+ vx - 1)) +
           2 * *(out_m + (+ vx - 1)) +
           4 * *(out_m + (+ vx + 0)) +
           2 * *(out_m + (+ vx + 1)) +
           1 * *(out_m + (+ vx + 1)) +
           1 * *(out_p + (- vx - 1)) +
           2 * *(out_p + (- vx - 1)) +
           4 * *(out_p + (- vx + 0)) +
           2 * *(out_p + (- vx + 1)) +
           1 * *(out_p + (- vx + 1))) / 40;
      
      scratch_ptr++;
      out++;
      out_p++;
      out_m++;
      }
    }
  }

static void antialias_slice_2(void * p, int start, int end)
  {
  int x, y;
  yuvdeinterlacer_t * di = p;
  
  int w = di->cur.w;

  uint8_t * scratch_ptr;
  uint8_t * out_ptr;
  
  for (y = start; y < end; y++)
    {
    out_ptr = di->cur.out + y * w + 2;
    scratch_ptr = di->scratch + y * w + 2;
    
    for (x = 2; x < (w - 2); x++)
      {
      *(out_ptr) = (*(out_ptr) + *(scratch_ptr)) >> 1;
      out_ptr++;
      scratch_ptr++;
      }
    }
  }

void antialias_plane (yuvdeinterlacer_t * di,
                      uint8_t * RESTRICT out, int w, int h)
  {
  int nt, scanline, delta;
  int i;
  
  di->cur.w = w;
  di->cur.h = h;
  di->cur.out = out;
  
  nt = di->num_threads;
  if(nt > h/2)
    nt = h/2;

  //  nt = 1;
  
  delta = (h - 4) / nt;

  scanline = 2;

  for(i = 0; i < nt - 1; i++)
    {
    di->run_func(antialias_slice_1,
                 di, scanline, scanline+delta, di->run_data, i);
    
    //        fprintf(stderr, "scanline: %d (%d)\n", ctx->scanline, ctx->dst_rect.h);
    scanline += delta;
    }
  di->run_func(antialias_slice_1,
               di, scanline, h-2, di->run_data, nt - 1);
      
  for(i = 0; i < nt; i++)
    di->stop_func(di->stop_data, i);


  scanline = 2;

  for(i = 0; i < nt - 1; i++)
    {
    di->run_func(antialias_slice_2,
                 di, scanline, scanline+delta, di->run_data, i);
    
    //        fprintf(stderr, "scanline: %d (%d)\n", ctx->scanline, ctx->dst_rect.h);
    scanline += delta;
    }
  di->run_func(antialias_slice_2,
               di, scanline, h-2, di->run_data, nt - 1);
      
  for(i = 0; i < nt; i++)
    di->stop_func(di->stop_data, i);
  }

static int antialias_frame (yuvdeinterlacer_t * di, gavl_video_frame_t * frame)
  {
  if(!di->read_func(di->read_data, di->inframe, di->read_stream))
    return 0;
  
  antialias_plane (di, di->inframe->planes[0], di->width, di->height);
  antialias_plane (di, di->inframe->planes[1], di->cwidth, di->cheight);
  antialias_plane (di, di->inframe->planes[2], di->cwidth, di->cheight);
  
  gavl_video_frame_copy(&di->format, frame, di->inframe);
  gavl_video_frame_copy_metadata(frame, di->inframe);
  
  return 1;

  //  y4m_write_frame (Y4MStream.fd_out, &Y4MStream.ostreaminfo, &Y4MStream.oframeinfo, inframe);
  }

#if 0 // Original
  void deinterlace_motion_compensated (int frame)
  {
    uint8_t *tmpptr;

    if(frame)
      {
      	uint8_t *saveptr[3] = {0, 0, 0};
    
        if(frame == 1)
          {
	    temporal_reconstruct_frame (outframe[0], inframe0[0], inframe[0],  inframe0[0], width, height, field_order, motion[0]);
	    temporal_reconstruct_frame (outframe[1], inframe0[1], inframe[1],  inframe0[1], cwidth, cheight, field_order, motion[1]);
	    temporal_reconstruct_frame (outframe[2], inframe0[2], inframe[2],  inframe0[2], cwidth, cheight, field_order, motion[1]);

	    temporal_reconstruct_frame (outframe[0], inframe0[0], inframe[0],  inframe0[0], width, height, 1 - field_order, motion[0]);
	    temporal_reconstruct_frame (outframe[1], inframe0[1], inframe[1],  inframe0[1], cwidth, cheight, 1 - field_order, motion[1]);
	    temporal_reconstruct_frame (outframe[2], inframe0[2], inframe[2],  inframe0[2], cwidth, cheight, 1 - field_order, motion[1]);

	    scale_motion_vectors (2, 0);

	    saveptr[0] = inframe1[0];
	    saveptr[1] = inframe1[1];
	    saveptr[2] = inframe1[2];
	    inframe1[0] = inframe[0];
	    inframe1[1] = inframe[1];
	    inframe1[2] = inframe[2];
          }
	else if(frame < 0)
	  {
	    saveptr[0] = inframe[0];
	    saveptr[1] = inframe[1];
	    saveptr[2] = inframe[2];
	    inframe[0] = inframe1[0];
	    inframe[1] = inframe1[1];
	    inframe[2] = inframe1[2];
	  }

	if (field_order == 0)
	  {
	    temporal_reconstruct_frame (outframe[0], inframe[0], inframe0[0],  inframe1[0], width, height, 1, motion[0]);
	    temporal_reconstruct_frame (outframe[1], inframe[1], inframe0[1],  inframe1[1], cwidth, cheight, 1, motion[1]);
	    temporal_reconstruct_frame (outframe[2], inframe[2], inframe0[2],  inframe1[2], cwidth, cheight, 1, motion[1]);

	    y4m_write_frame (Y4MStream.fd_out, &Y4MStream.ostreaminfo,
	                     &Y4MStream.oframeinfo, outframe);

	    if (frame == 1)
	      scale_motion_vectors (-1, both_fields);

	    if (both_fields == 1)
	      {
		temporal_reconstruct_frame (outframe[0], inframe[0], inframe0[0],  inframe1[0], width, height, 0, motion[0]);
		temporal_reconstruct_frame (outframe[1], inframe[1], inframe0[1],  inframe1[1], cwidth, cheight, 0, motion[1]);
		temporal_reconstruct_frame (outframe[2], inframe[2], inframe0[2],  inframe1[2], cwidth, cheight, 0, motion[1]);

		y4m_write_frame (Y4MStream.fd_out, &Y4MStream.ostreaminfo,
		                 &Y4MStream.oframeinfo, outframe);
	      }
	  }
	else
	  {
	    temporal_reconstruct_frame (outframe[0], inframe[0], inframe0[0],  inframe1[0], width, height, 0, motion[0]);
	    temporal_reconstruct_frame (outframe[1], inframe[1], inframe0[1],  inframe1[1], cwidth, cheight, 0, motion[1]);
	    temporal_reconstruct_frame (outframe[2], inframe[2], inframe0[2],  inframe1[2], cwidth, cheight, 0, motion[1]);

	    y4m_write_frame (Y4MStream.fd_out, &Y4MStream.ostreaminfo,
	                     &Y4MStream.oframeinfo, outframe);

	    if (frame == 1)
	      scale_motion_vectors (-1, both_fields);

	    if (both_fields == 1)
	      {
		temporal_reconstruct_frame (outframe[0], inframe[0], inframe0[0],  inframe1[0], width, height, 1, motion[0]);
		temporal_reconstruct_frame (outframe[1], inframe[1], inframe0[1],  inframe1[1], cwidth, cheight, 1, motion[1]);
		temporal_reconstruct_frame (outframe[2], inframe[2], inframe0[2],  inframe1[2], cwidth, cheight, 1, motion[1]);

		y4m_write_frame (Y4MStream.fd_out, &Y4MStream.ostreaminfo,
		                 &Y4MStream.oframeinfo, outframe);
	      }
	  }

	if (frame < 2)
	  {
	    inframe1[0] = saveptr[0];
	    inframe1[1] = saveptr[1];
	    inframe1[2] = saveptr[2];
	  }
      }

    tmpptr = inframe1[0];
    inframe1[0] = inframe0[0];
    inframe0[0] = inframe[0];
    inframe[0] = tmpptr;

    tmpptr = inframe1[1];
    inframe1[1] = inframe0[1];
    inframe0[1] = inframe[1];
    inframe[1] = tmpptr;

    tmpptr = inframe1[2];
    inframe1[2] = inframe0[2];
    inframe0[2] = inframe[2];
    inframe[2] = tmpptr;
  }


#endif

static int deinterlace_motion_compensated (yuvdeinterlacer_t * di, gavl_video_frame_t * frame)
  {
  gavl_video_frame_t *tmpptr;
  int read_frame = 0;
  gavl_video_frame_t *saveptr = (gavl_video_frame_t *)0;
  
  if(di->both_fields)
    {
    if(di->current_field >= 2)
      di->current_field = 0;
    }
  else
    di->current_field = 0;
  
  if(!di->current_field)
    {
    if(di->eof)
      return 1;
    
    if(!di->read_func(di->read_data, di->inframe, di->read_stream))
      {
      saveptr = di->inframe;
      di->inframe = di->inframe1;
      di->eof = 1;
      }
    else
      {
      /* If we have no frame yet, we must read the second one and initialize */

      if(!di->frame)
        {
        tmpptr = di->inframe1;
        di->inframe1 = di->inframe0;
        di->inframe0 = di->inframe;
        di->inframe = tmpptr;
        di->frame++;
      
        if(!di->read_func(di->read_data, di->inframe, di->read_stream))
          return 0;

        temporal_reconstruct_frame(di, di->inframe0, di->inframe, di->inframe0, di->field_order);
        temporal_reconstruct_frame(di, di->inframe0, di->inframe, di->inframe0, 1 - di->field_order);
      
        scale_motion_vectors (di, 2, 0);

        saveptr = di->inframe1;
        di->inframe1 = di->inframe;
        }
      read_frame = 1;
      }
    }
  
  if(!di->current_field)
    {
    if(di->field_order == 0)
      {
      temporal_reconstruct_frame(di, di->inframe, di->inframe0, di->inframe1, 1);
      if (di->frame == 1)
        scale_motion_vectors(di, -1, di->both_fields);
      }
    else
      {
      temporal_reconstruct_frame(di, di->inframe, di->inframe0, di->inframe1, 0);
      if (di->frame == 1)
        scale_motion_vectors(di, -1, di->both_fields);
      }
    }
  else
    {
    if(di->field_order == 0)
      temporal_reconstruct_frame(di, di->inframe, di->inframe0, di->inframe1, 0);
    else
      temporal_reconstruct_frame(di, di->inframe, di->inframe0, di->inframe1, 1);
    }
  
  if(saveptr)
    di->inframe1 = saveptr;
  
  if(di->current_field || !di->both_fields)
    {
    tmpptr = di->inframe1;
    di->inframe1 = di->inframe0;
    di->inframe0 = di->inframe;
    di->inframe = tmpptr;
    di->frame++;
    }
  gavl_video_frame_copy(&di->format, frame, di->outframe);
  
  frame->timestamp = di->inframe->timestamp + di->current_field * di->format.frame_duration;
  frame->duration  = di->inframe->duration;

  di->current_field++;

  return 1;
  }


void yuvdeinterlacer_set_mode(yuvdeinterlacer_t * di, int mode)
  {
  switch(mode)
    {
    case YUVD_MODE_ANTIALIAS:
      if(!di->just_anti_alias || di->both_fields)
        di->need_restart = 1;
      
      di->just_anti_alias = 1;
      di->both_fields     = 0;
      break;
    case YUVD_MODE_DEINT_1:
      if(di->just_anti_alias || di->both_fields)
        di->need_restart = 1;
      di->just_anti_alias = 0;
      di->both_fields     = 0;
      break;
    case YUVD_MODE_DEINT_2:
      if(di->just_anti_alias || !di->both_fields)
        di->need_restart = 1;
      di->just_anti_alias = 0;
      di->both_fields     = 1;
      break;
    }
  }

void yuvdeinterlacer_init(yuvdeinterlacer_t * ret,
                          gavl_video_format_t * format,
                          gavl_video_options_t * opt)
  {
  ret->run_func =
    gavl_video_options_get_run_func(opt, &ret->run_data);
  ret->stop_func =
    gavl_video_options_get_stop_func(opt, &ret->stop_data);
                                                  
  ret->num_threads = 
    gavl_video_options_get_num_threads(opt);
  
  free_memory(ret);
  switch(format->pixelformat)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
    case GAVL_RGB_24:
    case GAVL_BGR_24:
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_YUV_444_P:
    case GAVL_YUV_444_P_16:
    case GAVL_RGB_48:
    case GAVL_RGB_FLOAT:
    case GAVL_RGBA_32:
    case GAVL_YUVA_32:
    case GAVL_RGBA_64:
    case GAVL_GRAY_8: 
    case GAVL_GRAY_16:
    case GAVL_GRAY_FLOAT:
    case GAVL_GRAYA_16:
    case GAVL_GRAYA_32:
    case GAVL_GRAYA_FLOAT:
    case GAVL_YUV_FLOAT:
    case GAVL_YUVA_FLOAT:
    case GAVL_YUVA_64:
    case GAVL_RGBA_FLOAT:
      format->pixelformat = GAVL_YUV_444_P;
      break;
    case GAVL_YUY2:
    case GAVL_UYVY:
    case GAVL_YUV_422_P_16:
      format->pixelformat = GAVL_YUV_422_P;
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_411_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
    case GAVL_YUV_422_P:
      break;
    case GAVL_PIXELFORMAT_NONE:
      break;
      
    }

  gavl_video_format_copy(&ret->format, format);

  switch(format->interlace_mode)
    {
    case GAVL_INTERLACE_NONE:
      ret->field_order = 1;
      break;
    case GAVL_INTERLACE_TOP_FIRST:
      ret->field_order = 1;
      break;
    case GAVL_INTERLACE_BOTTOM_FIRST:
      ret->field_order = 0;
      break;
    case GAVL_INTERLACE_MIXED:
      ret->field_order = 1;
      break;
    }
  if(ret->both_fields)
    ret->format.timescale *= 2;
  
  ret->format.interlace_mode = GAVL_INTERLACE_NONE;
  initialize_memory(ret);
  }

void yuvdeinterlacer_get_output_format(yuvdeinterlacer_t * di,
                                       gavl_video_format_t * format)
  {
  gavl_video_format_copy(format, &di->format);
  }

int yuvdeinterlacer_read(void * data, gavl_video_frame_t * frame, int stream)
  {
  yuvdeinterlacer_t * di = (yuvdeinterlacer_t *)data;
  if(di->just_anti_alias)
    return antialias_frame(di, frame);
  else
    return deinterlace_motion_compensated(di, frame);
  }

void yuvdeinterlacer_reset(yuvdeinterlacer_t * di)
  {
  di->current_field = 0;
  di->frame = 0;
  di->eof = 0;
  
  gavl_video_frame_clear(di->inframe_real, &di->format);
  gavl_video_frame_clear(di->inframe0_real, &di->format);
  gavl_video_frame_clear(di->inframe1_real, &di->format);
  gavl_video_frame_clear(di->outframe_real, &di->format);
  }

void yuvdeinterlacer_destroy(yuvdeinterlacer_t * d)
  {
  free_memory(d);
  free(d);
  }


#if 0

int
main (int argc, char *argv[])
{
  int frame = 0;
  int errno = 0;
  int ss_h, ss_v;

  deinterlacer YUVdeint;

  char c;

  YUVdeint.field_order = -1;

  mjpeg_info("-------------------------------------------------");
  mjpeg_info( "       Motion-Compensating-Deinterlacer");
  mjpeg_info("-------------------------------------------------");

  while ((c = getopt (argc, argv, "hvds:t:a")) != -1)
    {
      switch (c)
	{
	case 'h':
	  {
	    mjpeg_info(" Usage of the deinterlacer");
	    mjpeg_info(" -------------------------");
	    mjpeg_info(" -v be verbose");
	    mjpeg_info(" -d output both fields");
	    mjpeg_info(" -a just antialias the frames! This will");
	    mjpeg_info("    assume progressive but aliased input.");
	    mjpeg_info("    you can use this to improve badly deinterlaced");
	    mjpeg_info("    footage. EG: deinterlaced with cubic-interpolation");
	    mjpeg_info("    or worse...");

	    mjpeg_info(" -s [n=0/1] forces field-order in case of misflagged streams");
	    mjpeg_info("    -s0 is bottom-field-first");
	    mjpeg_info("    -s1 is top-field-first");
	    exit (0);
	    break;
	  }
	case 'v':
	  {
	    YUVdeint.verbose = 1;
	    break;
	  }
	case 'd':
	  {
	    YUVdeint.both_fields = 1;
	    mjpeg_info("Regenerating both fields. Please fix the Framerate.");
	    break;
	  }
	case 'a':
	  {
	    YUVdeint.just_anti_alias = 1;
	    YUVdeint.field_order = 0;	// just to prevent the program to barf in this case
	    mjpeg_info("I will just anti-alias the frames. make sure they are progressive!");
	    break;
	  }
	case 't':
	  {
	    mjpeg_info("motion-threshold not used");
	    break;
	  }
	case 's':
	  {
	    YUVdeint.field_order = atoi (optarg);
	    if (YUVdeint.field_order != 0)
	      {
		mjpeg_info("forced top-field-first!");
		YUVdeint.field_order = 1;
	      }
	    else
	      {
		mjpeg_info("forced bottom-field-first!");
		YUVdeint.field_order = 0;
	      }
	    break;
	  }
	}
    }

  // initialize motionsearch-library      
  init_motion_search ();

#ifdef HAVE_ALTIVEC
  reset_motion_simd ("sad_00");
#endif

  // initialize stream-information 
  y4m_accept_extensions (1);
  y4m_init_stream_info (&YUVdeint.Y4MStream.istreaminfo);
  y4m_init_frame_info (&YUVdeint.Y4MStream.iframeinfo);
  y4m_init_stream_info (&YUVdeint.Y4MStream.ostreaminfo);
  y4m_init_frame_info (&YUVdeint.Y4MStream.oframeinfo);

/* open input stream */
  if ((errno = y4m_read_stream_header (YUVdeint.Y4MStream.fd_in,
				       &YUVdeint.Y4MStream.istreaminfo)) != Y4M_OK)
    {
      mjpeg_error_exit1 ("Couldn't read YUV4MPEG header: %s!", y4m_strerr (errno));
    }

  /* get format information */
  YUVdeint.width = y4m_si_get_width (&YUVdeint.Y4MStream.istreaminfo);
  YUVdeint.height = y4m_si_get_height (&YUVdeint.Y4MStream.istreaminfo);
  YUVdeint.input_chroma_subsampling = y4m_si_get_chroma (&YUVdeint.Y4MStream.istreaminfo);
  mjpeg_info("Y4M-Stream is %ix%i(%s)", YUVdeint.width,
	     YUVdeint.height, y4m_chroma_keyword (YUVdeint.input_chroma_subsampling));

  /* if chroma-subsampling isn't supported bail out ... */
  switch (YUVdeint.input_chroma_subsampling)
    {
    case Y4M_CHROMA_420JPEG:
    case Y4M_CHROMA_420MPEG2:
    case Y4M_CHROMA_420PALDV:
    case Y4M_CHROMA_444:
    case Y4M_CHROMA_422:
    case Y4M_CHROMA_411:
      ss_h = y4m_chroma_ss_x_ratio (YUVdeint.input_chroma_subsampling).d;
      ss_v = y4m_chroma_ss_y_ratio (YUVdeint.input_chroma_subsampling).d;
      YUVdeint.cwidth = YUVdeint.width / ss_h;
      YUVdeint.cheight = YUVdeint.height / ss_v;
      break;
    default:
      mjpeg_error_exit1 ("%s is not in supported chroma-format. Sorry.",
			 y4m_chroma_keyword (YUVdeint.input_chroma_subsampling));
    }

  /* the output is progressive 4:2:0 MPEG 1 */
  y4m_si_set_interlace (&YUVdeint.Y4MStream.ostreaminfo, Y4M_ILACE_NONE);
  y4m_si_set_chroma (&YUVdeint.Y4MStream.ostreaminfo, YUVdeint.input_chroma_subsampling);
  y4m_si_set_width (&YUVdeint.Y4MStream.ostreaminfo, YUVdeint.width);
  y4m_si_set_height (&YUVdeint.Y4MStream.ostreaminfo, YUVdeint.height);
  y4m_si_set_framerate (&YUVdeint.Y4MStream.ostreaminfo,
			y4m_si_get_framerate (&YUVdeint.Y4MStream.istreaminfo));
  y4m_si_set_sampleaspect (&YUVdeint.Y4MStream.ostreaminfo,
			   y4m_si_get_sampleaspect (&YUVdeint.Y4MStream.istreaminfo));

/* check for field dominance */

  if (YUVdeint.field_order == -1)
    {
      /* field-order was not specified on commandline. So we try to
       * get it from the stream itself...
       */

      if (y4m_si_get_interlace (&YUVdeint.Y4MStream.istreaminfo) == Y4M_ILACE_TOP_FIRST)
	{
	  /* got it: Top-field-first... */
	  mjpeg_info(" Stream is interlaced, top-field-first.");
	  YUVdeint.field_order = 1;
	}
      else if (y4m_si_get_interlace (&YUVdeint.Y4MStream.istreaminfo) == Y4M_ILACE_BOTTOM_FIRST)
	{
	  /* got it: Bottom-field-first... */
	  mjpeg_info(" Stream is interlaced, bottom-field-first.");
	  YUVdeint.field_order = 0;
	}
      else
	{
	  mjpeg_error("Unable to determine field-order from input-stream.");
	  mjpeg_error("This is most likely the case when using mplayer to produce the input-stream.");
	  mjpeg_error("Either the stream is misflagged or progressive...");
	  mjpeg_error("I will stop here, sorry. Please choose a field-order");
	  mjpeg_error("with -s0 or -s1. Otherwise I can't do anything for you. TERMINATED. Thanks...");
	  exit (-1);
	}
    }

  // initialize deinterlacer internals
  YUVdeint.initialize_memory (YUVdeint.width, YUVdeint.height, YUVdeint.cwidth, YUVdeint.cheight);

  /* write the outstream header */
  y4m_write_stream_header (YUVdeint.Y4MStream.fd_out, &YUVdeint.Y4MStream.ostreaminfo);

  /* read every frame until the end of the input stream and process it */
  while (Y4M_OK == (errno = y4m_read_frame (YUVdeint.Y4MStream.fd_in,
					    &YUVdeint.Y4MStream.istreaminfo,
					    &YUVdeint.Y4MStream.iframeinfo, YUVdeint.inframe)))
    {
      if (!YUVdeint.just_anti_alias)
	YUVdeint.deinterlace_motion_compensated (frame);
      else
	YUVdeint.antialias_frame ();
      frame++;
    }

  if (!YUVdeint.just_anti_alias)
    YUVdeint.deinterlace_motion_compensated (-frame);

  return 0;
}

#endif
