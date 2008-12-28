/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

/*
 * Port of yadif filter from Avisynth:
 *
 * Yadif C-plugin for Avisynth 2.5 - Yet Another DeInterlacing Filter
 *      Copyright (C)2007 Alexander G. Balakhnin aka Fizick
 *      http://avisynth.org.ru
 * Port of YADIF filter from MPlayer
 *      Copyright (C) 2006 Michael Niedermayer <michaelni@gmx.at>
 */


#include <gmerlin/plugin.h>
#include <bgyadif.h>
#include <gavl/gavldsp.h>

#include <string.h>

struct bg_yadif_s
  {
  gavl_dsp_context_t * dsp_ctx;
  gavl_dsp_funcs_t   * dsp_funcs;
  
  int parity;
  
  /* Where to get data */
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;

  gavl_video_format_t in_format;
  gavl_video_format_t out_format;
  
  void (*filter_line)(int mode, uint8_t *dst, const uint8_t *prev,
                      const uint8_t *cur, const uint8_t *next,
                      int w, int src_stride, int parity, int advance);
  void (*interpolate)(const uint8_t *src_1, const uint8_t *src_2, uint8_t *dst, int num);

  struct
    {
    int w;
    int h;
    int plane;
    int offset;
    int advance;
    } components[4];
  
  int num_components;

  gavl_video_frame_t * cur;
  gavl_video_frame_t * prev;
  gavl_video_frame_t * next;
  
  int64_t frame;
  int64_t field;
  int eof;
  
  int mode;
  };

/* Line filter functions */

static void filter_line_c(int mode, uint8_t *dst, const uint8_t *prev,
                          const uint8_t *cur, const uint8_t *next, int w, int src_stride, int parity,
                          int advance);


bg_yadif_t * bg_yadif_create()
  {
  bg_yadif_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->dsp_ctx = gavl_dsp_context_create();
  ret->dsp_funcs = gavl_dsp_context_get_funcs(ret->dsp_ctx);
  return ret;
  }

#define FREE_FRAME(f) if(f) { gavl_video_frame_destroy(f); f = NULL; }

void bg_yadif_destroy(bg_yadif_t * d)
  {
  gavl_dsp_context_destroy(d->dsp_ctx);

  FREE_FRAME(d->cur);
  FREE_FRAME(d->prev);
  FREE_FRAME(d->next);
  
  free(d);
  }

static const gavl_pixelformat_t pixelformats[] =
  {
    GAVL_YUV_420_P,
    GAVL_YUV_422_P,
    GAVL_YUV_444_P,
    GAVL_YUV_411_P,
    GAVL_YUV_410_P,
    GAVL_YUVJ_420_P,
    GAVL_YUVJ_422_P,
    GAVL_YUVJ_444_P,
    //    GAVL_GRAY_8,
    GAVL_PIXELFORMAT_NONE,
  };

void bg_yadif_init(bg_yadif_t * di,
                   gavl_video_format_t * format,
                   gavl_video_options_t * opt, int mode)
  {
  int sub_h, sub_v;

  FREE_FRAME(di->cur);
  FREE_FRAME(di->prev);
  FREE_FRAME(di->next);
  
  format->pixelformat = gavl_pixelformat_get_best(format->pixelformat,
                                                  pixelformats,
                                                  (int*)0);
  switch(format->pixelformat)
    {
    case GAVL_YUV_420_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_444_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_410_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      gavl_pixelformat_chroma_sub(format->pixelformat, &sub_h, &sub_v);
      
      di->filter_line = filter_line_c;
      di->interpolate = di->dsp_funcs->average_8;

      di->components[0].w = format->image_width;
      di->components[0].h = format->image_height;
      di->components[0].offset  = 0;
      di->components[0].plane   = 0;
      di->components[0].advance = 1;

      di->components[1].w = format->image_width / sub_h;
      di->components[1].h = format->image_height / sub_v;
      di->components[1].offset  = 0;
      di->components[1].plane   = 1;
      di->components[1].advance = 1;

      di->components[2].w = format->image_width / sub_h;
      di->components[2].h = format->image_height / sub_v;
      di->components[2].offset  = 0;
      di->components[2].plane   = 2;
      di->components[2].advance = 1;
      di->num_components = 3;
      
      break;
    default:
      break;
    }
  gavl_video_format_copy(&di->in_format, format);
  gavl_video_format_copy(&di->out_format, format);
  di->out_format.interlace_mode = GAVL_INTERLACE_NONE;

  if(mode & 1)
    di->out_format.timescale *= 2;
  
  di->mode = mode;
  }

void bg_yadif_get_output_format(bg_yadif_t * di,
                                gavl_video_format_t * format)
  {
  gavl_video_format_copy(format, &di->out_format);

  }

void bg_yadif_connect_input(void * priv,
                            bg_read_video_func_t func,
                            void * data, int stream)
  {
  bg_yadif_t * di = priv;
  di->read_func = func;
  di->read_data = data;
  di->read_stream = stream;
  }

static void filter_plane(bg_yadif_t * di,
                         int mode, uint8_t *dst, int dst_stride,
                         const uint8_t *prev0, const uint8_t *cur0,
                         const uint8_t *next0, int src_stride, int w, int h,
                         int parity, int tff, int mmx, int advance)
  {
  int y;
#if 0
  filter_line = filter_line_c;
#ifdef __GNUC__
  if (mmx)
    filter_line = filter_line_mmx2;
#endif
#endif
  y=0;
  if(((y ^ parity) & 1))
    {
    memcpy(dst, cur0 + src_stride, w);// duplicate 1
    }
  else
    {
    memcpy(dst, cur0, w);
    }
  y=1;
  if(((y ^ parity) & 1))
    {
    di->interpolate(cur0, cur0 + src_stride*2, dst + dst_stride, w);   // interpolate 0 and 2
    }
  else
    {
    memcpy(dst + dst_stride, cur0 + src_stride, w); // copy original
    }
  for(y=2; y<h-2; y++)
    {
    if(((y ^ parity) & 1))
      {
      const uint8_t *prev= prev0 + y*src_stride;
      const uint8_t *cur = cur0 + y*src_stride;
      const uint8_t *next= next0 + y*src_stride;
      uint8_t *dst2= dst + y*dst_stride;
      di->filter_line(mode, dst2, prev, cur, next, w, src_stride, (parity ^ tff), advance);
      }
    else
      {
      memcpy(dst + y*dst_stride, cur0 + y*src_stride, w); // copy original
      }
    }
  y=h-2;
  if(((y ^ parity) & 1))
    {
    di->interpolate(cur0 + (h-3)*src_stride, cur0 + (h-1)*src_stride, dst + (h-2)*dst_stride, w);   // interpolate h-3 and h-1
    }
  else
    {
    memcpy(dst + (h-2)*dst_stride, cur0 + (h-2)*src_stride, w); // copy original
    }
  y=h-1;
  if(((y ^ parity) & 1))
    {
    memcpy(dst + (h-1)*dst_stride, cur0 + (h-2)*src_stride, w); // duplicate h-2
    }
  else
    {
    memcpy(dst + (h-1)*dst_stride, cur0 + (h-1)*src_stride, w); // copy original
    }
  
#ifdef __GNUC__
  if (mmx)
    asm volatile("emms");
#endif
  }

static void filter_frame(bg_yadif_t * di,
                         int mode, int parity, int mmx,
                         gavl_video_frame_t * out)
  {
  int i;
  int tff;

  switch(di->in_format.interlace_mode)
    {
    case GAVL_INTERLACE_NONE:
    case GAVL_INTERLACE_TOP_FIRST:
      tff = 1;
      break;
    case GAVL_INTERLACE_BOTTOM_FIRST:
      tff = 0;
      break;
    case GAVL_INTERLACE_MIXED:

      switch(di->cur->interlace_mode)
        {
        case GAVL_INTERLACE_NONE:
          gavl_video_frame_copy(&di->out_format, out, di->cur);
          return;
          break;
        case GAVL_INTERLACE_TOP_FIRST:
          tff = 1;
          break;
        case GAVL_INTERLACE_BOTTOM_FIRST:
          tff = 0;
          break;
        default:
          return;
        }
      break;
    }
  
  for(i = 0; i < di->num_components; i++)
    {
    filter_plane(di, mode,
                 out->planes[di->components[i].plane] + di->components[i].offset,
                 out->strides[di->components[i].plane],
                 di->prev->planes[di->components[i].plane] + di->components[i].offset,
                 di->cur->planes[di->components[i].plane] + di->components[i].offset,
                 di->next->planes[di->components[i].plane] + di->components[i].offset,
                 di->prev->strides[di->components[i].plane],
                 di->components[i].w, di->components[i].h, parity, tff, mmx,
                 di->components[i].advance);
    }
  }

static int read_frame(bg_yadif_t * di)
  {
  gavl_video_frame_t * tmp;
  tmp = di->prev;

  di->prev = di->cur;
  di->cur = di->next;
  di->next = tmp;
  return di->read_func(di->read_data, di->next, di->read_stream);
  }

int bg_yadif_read(void * priv, gavl_video_frame_t * frame, int stream)
  {
  bg_yadif_t * di = priv;
  int tff;
  
  if(!di->field)
    {
    if(!di->frame) /* Fill buffer */
      {
      di->read_func(di->read_data, di->cur, di->read_stream);
      di->frame++;
      di->read_func(di->read_data, di->next, di->read_stream);
      gavl_video_frame_copy(&di->in_format, di->prev, di->next);
      }
    else if(!read_frame(di))
      di->eof = 1;
    
    di->frame++;
    
    /* Output first field */
    filter_frame(di, di->mode, di->parity, 0, // int mmx,
                 frame);

    if(di->mode & 1)
      di->field++;
    else
      di->field = 0;

    frame->timestamp = gavl_time_rescale(di->in_format.timescale,
                                         di->out_format.timescale,
                                         di->cur->timestamp);
    frame->timecode = di->cur->timecode;
    frame->duration = di->cur->duration;
    }
  else
    {
    /* Output second field */
    filter_frame(di, di->mode, 1 - di->parity, 0, // int mmx,
                 frame);

    frame->timestamp = gavl_time_rescale(di->in_format.timescale,
                                         di->out_format.timescale,
                                         di->cur->timestamp) + di->cur->duration;
    frame->timecode = di->cur->timecode;
    frame->duration = di->cur->duration;
    
    di->field = 0;
    }
  
  return 0;
  }

void bg_yadif_reset(bg_yadif_t * di)
  {
  di->frame = 0;
  di->field = 0;
  }


/* Scanline interpolation functions */

#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define ABS(a) ((a) > 0 ? (a) : (-(a)))

#define MIN3(a,b,c) MIN(MIN(a,b),c)
#define MAX3(a,b,c) MAX(MAX(a,b),c)

static void filter_line_c(int mode, uint8_t *dst, const uint8_t *prev,
                          const uint8_t *cur, const uint8_t *next, int w,
                          int src_stride, int parity, int advance)
  {
  int x;
  const uint8_t *prev2= parity ? prev : cur ;
  const uint8_t *next2= parity ? cur  : next;
  for(x=0; x<w; x++){
  int c= cur[-src_stride];
  int d= (prev2[0] + next2[0])>>1;
  int e= cur[+src_stride];
  int temporal_diff0= ABS(prev2[0] - next2[0]);
  int temporal_diff1=( ABS(prev[-src_stride] - c) + ABS(prev[+src_stride] - e) )>>1;
  int temporal_diff2=( ABS(next[-src_stride] - c) + ABS(next[+src_stride] - e) )>>1;
  int diff= MAX3(temporal_diff0>>1, temporal_diff1, temporal_diff2);
  int spatial_pred= (c+e)>>1;
  int spatial_score= ABS(cur[-src_stride-1] - cur[+src_stride-1]) + ABS(c-e)
    + ABS(cur[-src_stride+1] - cur[+src_stride+1]) - 1;

#define CHECK(j)\
    {   int score= ABS(cur[-src_stride-1+ j] - cur[+src_stride-1- j])\
                 + ABS(cur[-src_stride  + j] - cur[+src_stride  - j])\
                 + ABS(cur[-src_stride+1+ j] - cur[+src_stride+1- j]);\
        if(score < spatial_score){\
            spatial_score= score;\
            spatial_pred= (cur[-src_stride  + j] + cur[+src_stride  - j])>>1;\

  CHECK(-1) CHECK(-2) }} }}
  CHECK( 1) CHECK( 2) }} }}

  if(mode<2)
    {
      int b= (prev2[-2*src_stride] + next2[-2*src_stride])>>1;
      int f= (prev2[+2*src_stride] + next2[+2*src_stride])>>1;
#if 0
      int a= cur[-3*src_stride];
      int g= cur[+3*src_stride];
      int max= MAX3(d-e, d-c, MIN3(MAX(b-c,f-e),MAX(b-c,b-a),MAX(f-g,f-e)) );
      int min= MIN3(d-e, d-c, MAX3(MIN(b-c,f-e),MIN(b-c,b-a),MIN(f-g,f-e)) );
#else
      int max= MAX3(d-e, d-c, MIN(b-c, f-e));
      int min= MIN3(d-e, d-c, MAX(b-c, f-e));
#endif

      diff= MAX3(diff, min, -max);
      }

    if(spatial_pred > d + diff)
      spatial_pred = d + diff;
    else if(spatial_pred < d - diff)
      spatial_pred = d - diff;

    dst[0] = spatial_pred;
    dst++;
    cur++;
    prev++;
    next++;
    prev2++;
    next2++;
    }
  }
