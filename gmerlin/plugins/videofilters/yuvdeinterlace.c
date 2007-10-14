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

/*
 *  Original file yuvdeinterlace.cc from mjpegtools (http://mjpeg.sourceforge.net),
 *  adapted to gmerlin
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

struct yuvdeinterlacer_s
  {
  int width;
  int height;
  int cwidth;
  int cheight;
  int field_order;
  int both_fields;
  
  int current_field;
  
  int verbose;
  int input_chroma_subsampling;
  int output_chroma_subsampling;
  int vertical_overshot_luma;
  int vertical_overshot_chroma;
  int mark_moving_blocks;
  int just_anti_alias;
  
  gavl_video_frame_t * inframe;
  gavl_video_frame_t * inframe0;
  gavl_video_frame_t * inframe1;
  gavl_video_frame_t * outframe;

  gavl_video_frame_t * inframe_real;
  gavl_video_frame_t * inframe0_real;
  gavl_video_frame_t * inframe1_real;
  gavl_video_frame_t * outframe_real;
  
  uint8_t *scratch;
  uint8_t *mmap;

  int scratch_offset; // Added to the adress after malloc, substracted before free()
  
  gavl_video_format_t format;
  
  /* Where to get data */
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;

  int need_restart;
  
  };

void yuvdeinterlacer_connect_input(void * priv,
                                   bg_read_video_func_t func,
                                   void * data, int stream)
  {
  yuvdeinterlacer_t * di = (yuvdeinterlacer_t *)priv;
  di->read_func = func;
  di->read_data = data;
  di->read_stream = stream;
  }

static void antialias_plane (yuvdeinterlacer_t * di, uint8_t * out, int w, int h);

#define OVERSHOOT 32

static void initialize_memory(yuvdeinterlacer_t * d)
  {
  int luma_size;
  int chroma_size;
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
  
  d->scratch = (uint8_t *) malloc (luma_size + vertical_overshot_luma);
  memset(d->scratch, 0, luma_size + vertical_overshot_luma);
  
  d->mmap = (uint8_t *) malloc (luma_size + vertical_overshot_luma);
  memset(d->mmap, 0, luma_size + vertical_overshot_luma);
  
  d->scratch_offset = vertical_overshot_luma;
  d->scratch += d->scratch_offset;
  d->width = w;
  d->height = h;
  d->cwidth = cw;
  d->cheight = ch;
  }

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
  }

void yuvdeinterlacer_init(yuvdeinterlacer_t * ret,
                          gavl_video_format_t * format)
  {
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

void yuvdeinterlacer_reset(yuvdeinterlacer_t * di)
  {
  di->current_field = 0;

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

static void
temporal_reconstruct_plane(yuvdeinterlacer_t * di, uint8_t * out, uint8_t * in,
                           uint8_t * in0, uint8_t * in1, int w, int h, int field)
  {

  int x, y;
  int vx, vy, dx, dy, px, py;
  uint32_t min, sad;
  int a, b, c, d, e, f, g, m, i;
  static int lvx[256][256];	// this is sufficient for HDTV-signals or 2K cinema-resolution...
  static int lvy[256][256];	// dito...

  memset(lvx, 0, sizeof(lvx));
  memset(lvy, 0, sizeof(lvy));
  
  // the ELA-algorithm overshots by one line above and below the
  // frame-size, so fill the ELA-overshot-area in the inframe to
  // ensure that no green or purple lines are generated...
  //#ifdef notnow
  // do NOT do this - the "in0 -w" computes an address 'w' bytes BEFORE the 
  // start of the buffer.  On some systems this corrupts the malloc arena but
  // the program keeps running.  On other systems an immediate segfault happens
  // when the first memcpy is done.

  memcpy (in0 - w, in + w, w);
  memcpy (in0 + (w * h), in + (w * h) - 2 * w, w);
  // #endif

  // create deinterlaced frame of the reference-field in scratch
  for (y = (1 - field); y <= h; y += 2)
    {
    for (x = 0; x < w; x++)
      {

      a  = abs( *(in +x+(y-1)*w)-*(in0+x+(y-1)*w) );
      a += abs( *(in1+x+(y-1)*w)-*(in0+x+(y-1)*w) );
      a /= 2;

      b  = abs( *(in +x+(y  )*w)-*(in0+x+(y  )*w) );
      b += abs( *(in1+x+(y  )*w)-*(in0+x+(y  )*w) );
      b /= 2;

      c  = abs( *(in +x+(y+1)*w)-*(in0+x+(y+1)*w) );
      c += abs( *(in1+x+(y+1)*w)-*(in0+x+(y+1)*w) );
      c /= 2;

      if( (a<8 || c<8) && b<8 ) // Pixel is static?
	{
	// Yes...
	*(di->scratch+x+(y-1)*w) = *(in0+x+(y-1)*w);
	*(di->scratch+x+(y  )*w) = *(in0+x+(y  )*w);
	*(di->mmap+x+y*w) = 255; // mark pixel as static in motion-map...
	}
      else
	{
	// No...
	// Do an edge-directed-interpolation
	*(di->scratch+x+(y-1)*w) = *(in0+x+(y-1)*w);

	m  = *(in0+(x-3)+(y-2)*w);
	m += *(in0+(x-2)+(y-2)*w);
	m += *(in0+(x-1)+(y-2)*w);
	m += *(in0+(x-0)+(y-2)*w);
	m += *(in0+(x+1)+(y-2)*w);
	m += *(in0+(x+2)+(y-2)*w);
	m += *(in0+(x+3)+(y-2)*w);
	m += *(in0+(x-3)+(y-1)*w);
	m += *(in0+(x-2)+(y-1)*w);
	m += *(in0+(x-1)+(y-1)*w);
	m += *(in0+(x-0)+(y-1)*w);
	m += *(in0+(x+1)+(y-1)*w);
	m += *(in0+(x+2)+(y-1)*w);
	m += *(in0+(x+3)+(y-1)*w);
	m += *(in0+(x-3)+(y+1)*w);
	m += *(in0+(x-2)+(y+1)*w);
	m += *(in0+(x-1)+(y+1)*w);
	m += *(in0+(x-0)+(y+1)*w);
	m += *(in0+(x+1)+(y+1)*w);
	m += *(in0+(x+2)+(y+1)*w);
	m += *(in0+(x+3)+(y+1)*w);
	m += *(in0+(x-3)+(y+3)*w);
	m += *(in0+(x-2)+(y+3)*w);
	m += *(in0+(x-1)+(y+3)*w);
	m += *(in0+(x-0)+(y+3)*w);
	m += *(in0+(x+1)+(y+3)*w);
	m += *(in0+(x+2)+(y+3)*w);
	m += *(in0+(x+3)+(y+3)*w);
	m /= 28;

	a  = abs(  *(in0+(x-3)+(y-1)*w) - *(in0+(x+3)+(y+1)*w) );
	i = ( *(in0+(x-3)+(y-1)*w) + *(in0+(x+3)+(y+1)*w) )/2;
	a += 255-abs(m-i);

	b  = abs(  *(in0+(x-2)+(y-1)*w) - *(in0+(x+2)+(y+1)*w) );
	i = ( *(in0+(x-2)+(y-1)*w) + *(in0+(x+2)+(y+1)*w) )/2;
	b += 255-abs(m-i);

	c  = abs(  *(in0+(x-1)+(y-1)*w) - *(in0+(x+1)+(y+1)*w) );
	i = ( *(in0+(x-1)+(y-1)*w) + *(in0+(x+1)+(y+1)*w) )/2;
	c += 255-abs(m-i);

	d  = abs(  *(in0+(x  )+(y-1)*w) - *(in0+(x  )+(y+1)*w) );
	i = ( *(in0+(x  )+(y-1)*w) + *(in0+(x  )+(y+1)*w) )/2;
	d += 255-abs(m-i);

	e  = abs(  *(in0+(x+1)+(y-1)*w) - *(in0+(x-1)+(y+1)*w) );
	i = ( *(in0+(x+1)+(y-1)*w) + *(in0+(x+1)+(y-1)*w) )/2;
	e += 255-abs(m-i);

	f  = abs(  *(in0+(x+2)+(y-1)*w) - *(in0+(x-2)+(y+1)*w) );
	i = ( *(in0+(x+2)+(y-1)*w) + *(in0+(x+2)+(y-1)*w) )/2;
	f += 255-abs(m-i);

	g  = abs(  *(in0+(x+3)+(y-1)*w) - *(in0+(x-3)+(y+1)*w) );
	i = ( *(in0+(x+3)+(y-1)*w) + *(in0+(x+1)+(y-3)*w) )/2;
	g += 255-abs(m-i);

        i = ( *(in0+(x  )+(y-1)*w) + *(in0+(x  )+(y+1)*w) )/2;

	if (a<b && a<c && a<d && a<e && a<f && a<g )
          i = ( *(in0+(x-3)+(y-1)*w) + *(in0+(x+3)+(y+1)*w) )/2;
	else if (b<a && b<c && b<d && b<e && b<f && b<g )
          i = ( *(in0+(x-2)+(y-1)*w) + *(in0+(x+2)+(y+1)*w) )/2;
        else if (c<a && c<b && c<d && c<e && c<f && c<g )
          i = ( *(in0+(x-1)+(y-1)*w) + *(in0+(x+1)+(y+1)*w) )/2;
        else if (d<a && d<b && d<c && d<e && d<f && d<g )
          i = ( *(in0+(x  )+(y-1)*w) + *(in0+(x  )+(y+1)*w) )/2;
        else if (e<a && e<b && e<c && e<d && e<f && e<g )
          i = ( *(in0+(x+1)+(y-1)*w) + *(in0+(x-1)+(y+1)*w) )/2;
        else if (f<a && f<b && f<c && f<d && f<e && f<g )
          i = ( *(in0+(x+2)+(y-1)*w) + *(in0+(x-2)+(y+1)*w) )/2;
        else if (g<a && g<b && g<c && g<d && g<e && g<f )
          i = ( *(in0+(x+3)+(y-1)*w) + *(in0+(x-3)+(y+1)*w) )/2;
        
	*(di->scratch+x+(y  )*w) = i;
	*(di->mmap+x+y*w) = 0; // mark pixel as moving in motion-map...
	}

      }
    }
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

  memcpy (di->scratch + (w * h), di->scratch + (w * h) - w, w);
  memcpy (di->scratch + (w * h) + w, di->scratch + (w * h) - w, w);
  memcpy (di->scratch + (w * h) + w * 2, di->scratch + (w * h) - w, w);
  memcpy (di->scratch + (w * h) + w * 3, di->scratch + (w * h) - w, w);
  if(1)
    for (y = 0; y < h; y += 8)
      for (x = 0; x < w; x += 8)
	{
        // offset x+y so we get an overlapped search
        x -= 4;
        y -= 4;

        // reset motion-search with the zero-motion-vector (0;0)
        min = psad_00 (di->scratch + x + y * w, out + x + y * w, w, 16, 0x00ffffff);
        vx = 0;
        vy = 0;

        // check some "hot" candidates first...

        // if possible check all-neighbors-interpolation-vector
        if (min > 512)
          if (x >= 8 && y >= 8)
            {
            dx  = lvx[x / 8 - 1][y / 8 - 1];
            dx += lvx[x / 8    ][y / 8 - 1];
            dx += lvx[x / 8 + 1][y / 8 - 1];
            dx += lvx[x / 8 - 1][y / 8    ];
            dx += lvx[x / 8    ][y / 8    ];
            dx /= 5;
		
            dy  = lvy[x / 8 - 1][y / 8 - 1];
            dy += lvy[x / 8    ][y / 8 - 1];
            dy += lvy[x / 8 + 1][y / 8 - 1];
            dy += lvy[x / 8 - 1][y / 8    ];
            dy += lvy[x / 8    ][y / 8    ];
            dy /= 10;
            dy *= 2;

            sad =
              psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
            if (sad < min)
              {
              min = sad;
              vx = dx;
              vy = dy;
              }
            }

        // if possible check top-left-neighbor-vector
        if (min > 512)
          if (x >= 8 && y >= 8)
            {
            dx = lvx[x / 8 - 1][y / 8 - 1];
            dy = lvy[x / 8 - 1][y / 8 - 1];
            sad =
              psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
            if (sad < min)
              {
              min = sad;
              vx = dx;
              vy = dy;
              }
            }

        // if possible check top-neighbor-vector
        if (min > 512)
          if (y >= 8)
            {
            dx = lvx[x / 8][y / 8 - 1];
            dy = lvy[x / 8][y / 8 - 1];
            sad =
              psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
            if (sad < min)
              {
              min = sad;
              vx = dx;
              vy = dy;
              }
            }

        // if possible check top-right-neighbor-vector
        if (min > 512)
          if (x <= (w - 8) && y >= 8)
            {
            dx = lvx[x / 8 + 1][y / 8 - 1];
            dy = lvy[x / 8 + 1][y / 8 - 1];
            sad =
              psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
            if (sad < min)
              {
              min = sad;
              vx = dx;
              vy = dy;
              }
            }

        // if possible check right-neighbor-vector
        if (min > 512)
          if (x >= 8)
            {
            dx = lvx[x / 8 - 1][y / 8];
            dy = lvy[x / 8 - 1][y / 8];
            sad =
              psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
            if (sad < min)
              {
              min = sad;
              vx = dx;
              vy = dy;
              }
            }

        // check temporal-neighbor-vector
        if (min > 512)
          {
          dx = lvx[x / 8][y / 8];
          dy = lvy[x / 8][y / 8];
          sad = psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }
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
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }

          dx = px+2;
          dy = py;
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }

          dx = px-4;
          dy = py;
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }

          dx = px+4;
          dy = py;
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }

          dx = px;
          dy = py-4;
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }

          dx = px;
          dy = py+4;
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }
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
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }

          dx = px+1;
          dy = py;
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }

          dx = px-2;
          dy = py;
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }

          dx = px+2;
          dy = py;
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }

          dx = px;
          dy = py-2;
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }

          dx = px;
          dy = py+2;
          sad =
            psad_00 (di->scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, 0x00ffffff);
          if (sad < min)
            {
            min = sad;
            vx = dx;
            vy = dy;
            }
          }

	  // store the found vector, so we can do a candidates-check...
	  lvx[x / 8][y / 8] = vx;
	  lvy[x / 8][y / 8] = vy;

	  // remove x+y offset...
	  x += 4;
	  y += 4;

	  // reconstruct that block in di->scratch
	  // do so by using the lowpass (and alias-term) from the current field
	  // and the highpass (and phase-inverted alias-term) from the previous frame(!)
#if 1
	  for (dy = (1 - field); dy < 8; dy += 2)
	    for (dx = 0; dx < 8; dx++)
	      {
              a =
                *(di->scratch + (x + dx) + (y + dy - 3) * w) * -26 +
                *(di->scratch + (x + dx) + (y + dy - 1) * w) * +526 +
                *(di->scratch + (x + dx) + (y + dy + 1) * w) * +526 +
                *(di->scratch + (x + dx) + (y + dy + 3) * w) * -26;
              a /= 1000;

              b =
                *(out + (x + dx + vx) + (y + dy + vy - 3) * w) * +26 +
                *(out + (x + dx + vx) + (y + dy + vy - 1) * w) * -526 +
                *(out + (x + dx + vx) + (y + dy + vy + 0) * w) * +1000 +
                *(out + (x + dx + vx) + (y + dy + vy + 1) * w) * -526 +
                *(out + (x + dx + vx) + (y + dy + vy + 3) * w) * +26;
              b /= 1000;

              a = a + b;
              a = a < 0 ? 0 : a;
              a = a > 255 ? 255 : a;

              *(di->scratch + (x + dx) + (y + dy) * w) = a;
	      }
#else
	  for (dy = (1 - field); dy < 8; dy += 2)
	    for (dx = 0; dx < 8; dx++)
	      {
              *(di->scratch + (x + dx) + (y + dy) * w) =
                (
                 //*(di->scratch + (x + dx) + (y + dy) * w) +
                 *(out + (x + dx + vx) + (y + dy +vy) * w) 
                 )/1;
	      }
#endif
	}

  // copy a gauss-filtered variant of the reconstructed image to the out-buffer
  // the reason why this must be filtered is not so easy to understand, so I leave
  // some room for anyone who might try without... :-)
  //
  // If you want a starting point: remember an interlaced camera *must* *not* have
  // more vertical resolution than approximatly 0.6 times the nominal vertical
  // resolution... (So a progressive-scan camera will allways be better in this
  // regard but will introduce severe defects when the movie is displayed on an
  // interlaced screen...) Eitherways, who cares: this is better than a frame-mode
  // were the missing information is generated via a pixel-shift... :-)


  for (y = (1 - field); y <= h; y += 2)
    for (x = 0; x < w; x++)
      {
      if ( *(di->mmap+x+y*w)==255 ) // if pixel is static
        {
        *(di->scratch + x + (y) * w) = *(in0+x+(y  )*w);
        }
      }

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
      {
      //*(out + x + y * w) = (4 * *(di->scratch + x + (y) * w)+*(di->scratch + x + (y-1) * w)+*(di->scratch + x + (y+1) * w))/6;
      *(out + x + y * w) = *(di->scratch + x + (y) * w);
      }

  antialias_plane (di, out, w, h );
  }

static void temporal_reconstruct_frame(yuvdeinterlacer_t * di, int field)
  {
  temporal_reconstruct_plane(di,
                             di->outframe->planes[0],
                             di->inframe->planes[0],
                             di->inframe0->planes[0],
                             di->inframe1->planes[0],
                             di->width, di->height, field);
  temporal_reconstruct_plane(di,
                             di->outframe->planes[1],
                             di->inframe->planes[1],
                             di->inframe0->planes[1],
                             di->inframe1->planes[1],
                             di->cwidth, di->cheight, field);
  temporal_reconstruct_plane(di,
                             di->outframe->planes[2],
                             di->inframe->planes[2],
                             di->inframe0->planes[2],
                             di->inframe1->planes[2],
                             di->cwidth, di->cheight, field);
  }


static int deinterlace_motion_compensated(yuvdeinterlacer_t * di, gavl_video_frame_t * frame)
  {
  gavl_video_frame_t * swap;


  if(di->both_fields)
    {
    if(di->current_field >= 2)
      di->current_field = 0;
    }
  else
    di->current_field = 0;
  
  if(!di->current_field)
    {
    if(!di->read_func(di->read_data, di->inframe, di->read_stream))
      return 0;
    }
  
  if (di->field_order == 0)
    {
    if(!di->current_field)
      temporal_reconstruct_frame(di, 1);
    else
      temporal_reconstruct_frame(di, 0);
    }
  else
    {
    if(!di->current_field)
      temporal_reconstruct_frame(di, 0);
    else
      temporal_reconstruct_frame(di, 1);
    }

  /* Save older frame */

  if(!di->both_fields || di->current_field)
    {
    swap = di->inframe1;
    di->inframe1 = di->inframe0;
    di->inframe0 = swap;
    gavl_video_frame_copy(&di->format, di->inframe0, di->inframe);
    }
  
  gavl_video_frame_copy(&di->format, frame, di->outframe);
  
  frame->timestamp = di->inframe->timestamp + di->current_field * di->format.frame_duration;
  frame->duration  = di->inframe->duration;
  
  di->current_field++;
  return 1;
  }

static void antialias_plane (yuvdeinterlacer_t * di, uint8_t * out, int w, int h)
  {
  int x, y;
  int vx;
  uint32_t sad;
  uint32_t min;
  int dx;

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
      {
      min = 0x00ffffff;
      vx = 0;
      for (dx = -3; dx <= 3; dx++)
        {
        sad = 0;
        //      sad  = abs( *(out+(x+dx-3)+(y-1)*w) - *(out+(x-3)+(y+0)*w) );
        //      sad += abs( *(out+(x+dx-2)+(y-1)*w) - *(out+(x-2)+(y+0)*w) );
        sad += abs (*(out + (x + dx - 1) + (y - 1) * w) - *(out + (x - 1) + (y + 0) * w));
        sad += abs (*(out + (x + dx + 0) + (y - 1) * w) - *(out + (x + 0) + (y + 0) * w));
        sad += abs (*(out + (x + dx + 1) + (y - 1) * w) - *(out + (x + 1) + (y + 0) * w));
        //      sad += abs( *(out+(x+dx+2)+(y-1)*w) - *(out+(x+2)+(y+0)*w) );
        //      sad += abs( *(out+(x+dx+3)+(y-1)*w) - *(out+(x+3)+(y+0)*w) );

        //      sad += abs( *(out+(x-dx-3)+(y+1)*w) - *(out+(x-3)+(y+0)*w) );
        //      sad += abs( *(out+(x-dx-2)+(y+1)*w) - *(out+(x-2)+(y+0)*w) );
        sad += abs (*(out + (x - dx - 1) + (y + 1) * w) - *(out + (x - 1) + (y + 0) * w));
        sad += abs (*(out + (x - dx + 0) + (y + 1) * w) - *(out + (x + 0) + (y + 0) * w));
        sad += abs (*(out + (x - dx + 1) + (y + 1) * w) - *(out + (x + 1) + (y + 0) * w));
        //      sad += abs( *(out+(x-dx+2)+(y+1)*w) - *(out+(x+2)+(y+0)*w) );
        //      sad += abs( *(out+(x-dx+3)+(y+1)*w) - *(out+(x+3)+(y+0)*w) );

        if (sad < min)
          {
          min = sad;
          vx = dx;
          }
        }

      if (abs (vx) <= 1)
        *(di->scratch + x + y * w) =
          (2**(out + (x) + (y) * w) + *(out + (x + vx) + (y - 1) * w) +
           *(out + (x - vx) + (y + 1) * w)) / 4;
      else if (abs (vx) <= 3)
        *(di->scratch + x + y * w) =
          (2 * *(out + (x - 1) + (y) * w) +
           4 * *(out + (x + 0) + (y) * w) +
           2 * *(out + (x + 1) + (y) * w) +
           1 * *(out + (x + vx - 1) + (y - 1) * w) +
           2 * *(out + (x + vx + 0) + (y - 1) * w) +
           1 * *(out + (x + vx + 1) + (y - 1) * w) +
           1 * *(out + (x - vx - 1) + (y + 1) * w) +
           2 * *(out + (x - vx + 0) + (y + 1) * w) +
           1 * *(out + (x - vx + 1) + (y + 1) * w)) / 16;
      else
        *(di->scratch + x + y * w) =
          (2 * *(out + (x - 1) + (y) * w) +
           4 * *(out + (x - 1) + (y) * w) +
           8 * *(out + (x + 0) + (y) * w) +
           4 * *(out + (x + 1) + (y) * w) +
           2 * *(out + (x + 1) + (y) * w) +
           1 * *(out + (x + vx - 1) + (y - 1) * w) +
           2 * *(out + (x + vx - 1) + (y - 1) * w) +
           4 * *(out + (x + vx + 0) + (y - 1) * w) +
           2 * *(out + (x + vx + 1) + (y - 1) * w) +
           1 * *(out + (x + vx + 1) + (y - 1) * w) +
           1 * *(out + (x - vx - 1) + (y + 1) * w) +
           2 * *(out + (x - vx - 1) + (y + 1) * w) +
           4 * *(out + (x - vx + 0) + (y + 1) * w) +
           2 * *(out + (x - vx + 1) + (y + 1) * w) +
           1 * *(out + (x - vx + 1) + (y + 1) * w)) / 40;

      }

  for (y = 2; y < (h - 2); y++)
    for (x = 2; x < (w - 2); x++)
      {
      *(out + (x) + (y) * w) = (*(out + (x) + (y) * w) + *(di->scratch + (x) + (y + 0) * w)) / 2;
      }

  }

static int antialias_frame (yuvdeinterlacer_t * di, gavl_video_frame_t * frame)
  {
  if(!di->read_func(di->read_data, di->inframe, di->read_stream))
    return 0;

  antialias_plane (di, di->inframe->planes[0], di->width, di->height);
  antialias_plane (di, di->inframe->planes[1], di->cwidth, di->cheight);
  antialias_plane (di, di->inframe->planes[2], di->cwidth, di->cheight);

  gavl_video_frame_copy(&di->format, frame, di->inframe);
  frame->timestamp = di->inframe->timestamp;
  frame->duration  = di->inframe->duration;
  return 1;
  //  y4m_write_frame (Y4MStream.fd_out, &Y4MStream.ostreaminfo, &Y4MStream.oframeinfo, inframe);
  }

#if 0
bg_parameter_info_t yuvdeinterlacer_parameters[] =
  {
    {
      name: "yuvdeinterlace_mode",
      long_name: "Mode",
      type: BG_PARAMETER_MULTI_LIST,
    },
    {
      name: "just_anti_alias",
      long_name: "Do only antialiasing",
      type: BG_PARAMETER_CHECKBUTTON,
    },
    { /* End */ }
  };


void yuvdeinterlacer_set_parameter(void * data, const char * name, const bg_parameter_value_t * val)
  {
  yuvdeinterlacer_t * di = (yuvdeinterlacer_t *)data;
  if(!name)
    return;
  else if(!strcmp(name, "both_fields"))
    {
    if(di->both_fields != val->val_i)
      {
      di->need_restart = 1;
      di->both_fields = val->val_i;
      }
    }
  }
#endif

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


int yuvdeinterlacer_read(void * data, gavl_video_frame_t * frame, int stream)
  {
  yuvdeinterlacer_t * di = (yuvdeinterlacer_t *)data;
  if(di->just_anti_alias)
    return antialias_frame(di, frame);
  else
    return deinterlace_motion_compensated(di, frame);
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

  while ((c = getopt (argc, argv, "hvds:t:ma")) != -1)
    {
      switch (c)
	{
	case 'h':
	  {
	    mjpeg_info(" Usage of the deinterlacer");
	    mjpeg_info(" -------------------------");
	    mjpeg_info(" -v be verbose");
	    mjpeg_info(" -d output both fields");
	    mjpeg_info(" -m mark moving blocks");
	    mjpeg_info(" -t [nr] (default 4) motion threshold.");
	    mjpeg_info("         0 -> every block is moving.");
	    mjpeg_info(" -a just antialias the frames! This will");
	    mjpeg_info("    assume progressive but aliased input.");
	    mjpeg_info("    you can use this to improve badly deinterlaced");
	    mjpeg_info("    footage. EG: deinterlaced with cubic-interpolation");
	    mjpeg_info("    or worse...");

	    mjpeg_info(" -s [n=0/1] forces field-order in case of misflagged streams");
	    mjpeg_info("    -s0 is top-field-first");
	    mjpeg_info("    -s1 is bottom-field-first");
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
	case 'm':
	  {
	    YUVdeint.mark_moving_blocks = 1;
	    mjpeg_info("I will mark detected moving blocks for you, so you can");
	    mjpeg_info("fine-tune the motion-threshold (-t)...");
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
	    YUVdeint.motion_threshold = atoi (optarg);
	    mjpeg_info("motion-threshold set to : %i", YUVdeint.motion_threshold);
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
	YUVdeint.deinterlace_motion_compensated ();
      else
	YUVdeint.antialias_frame ();
      frame++;
    }
  return 0;
}

#endif
