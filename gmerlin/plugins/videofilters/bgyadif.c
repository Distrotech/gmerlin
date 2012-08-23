/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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


#include <config.h>
#include <gmerlin/plugin.h>
#include <bgyadif.h>
#include <gavl/gavldsp.h>

#include <string.h>

//#undef HAVE_MMX

typedef struct
  {
  int w;
  int h;
  int plane;
  int offset;
  int advance;
  } component_t;

struct bg_yadif_s
  {
  gavl_dsp_context_t * dsp_ctx;
  gavl_dsp_funcs_t   * dsp_funcs;
  
  int parity;
  int accel_flags;
  
  gavl_video_format_t in_format;
  gavl_video_format_t out_format;
  
  void (*filter_line)(int mode, uint8_t *dst, const uint8_t *prev,
                      const uint8_t *cur, const uint8_t *next,
                      int w, int src_stride, int parity, int advance);

  component_t components[4];
  component_t * comp;
  int current_parity;
  int tff;
  
  int num_components;

  gavl_video_frame_t * cur;
  gavl_video_frame_t * prev;
  gavl_video_frame_t * next;

  gavl_video_frame_t * dst;
  
  int64_t frame;
  int64_t field;
  int eof;
  
  int mode;

  int luma_shift;
  int chroma_shift;
#ifdef HAVE_MMX
  int mmx;
#endif

  /* Multithreading stuff */

  gavl_video_run_func run_func;
  void * run_data;
  gavl_video_stop_func stop_func;
  void * stop_data;
  int num_threads;
  };

/* Line filter functions */

static void filter_line_c(int mode, uint8_t *dst, const uint8_t *prev,
                          const uint8_t *cur, const uint8_t *next, int w, int src_stride, int parity,
                          int advance);

#ifdef HAVE_MMX
static void filter_line_mmx2(int mode, uint8_t *dst, const uint8_t *prev,
                             const uint8_t *cur, const uint8_t *next, int w, int src_stride, int parity,
                             int advance);
#endif

bg_yadif_t * bg_yadif_create()
  {
  bg_yadif_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->dsp_ctx = gavl_dsp_context_create();
  ret->dsp_funcs = gavl_dsp_context_get_funcs(ret->dsp_ctx);
  ret->accel_flags = gavl_accel_supported();
  return ret;
  }

#define SHIFT_PLANES(f) \
  { \
  if(f->planes[0]) \
    f->planes[0] += di->luma_shift; \
  if(f->planes[1]) \
    f->planes[1] += di->chroma_shift; \
  if(f->planes[2]) \
    f->planes[2] += di->chroma_shift; \
  }

#define FREE_FRAME(f) \
if(f) \
  { \
  if(f->planes[0]) \
    f->planes[0] -= di->luma_shift; \
  if(f->planes[1]) \
    f->planes[1] -= di->chroma_shift; \
  if(f->planes[2]) \
    f->planes[2] -= di->chroma_shift; \
  gavl_video_frame_destroy(f); \
  f = NULL; \
  }

void bg_yadif_destroy(bg_yadif_t * di)
  {
  gavl_dsp_context_destroy(di->dsp_ctx);

  FREE_FRAME(di->cur);
  FREE_FRAME(di->prev);
  FREE_FRAME(di->next);
  
  free(di);
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
                   gavl_video_format_t * out_format,
                   gavl_video_options_t * opt, int mode)
  {
  int sub_h = 1, sub_v = 1;
  gavl_video_format_t frame_format;
  
  di->frame = 0;
  di->field = 0;
  di->eof = 0;

  di->run_func  = gavl_video_options_get_run_func(opt, &di->run_data);
  di->stop_func = gavl_video_options_get_stop_func(opt, &di->stop_data);
  di->num_threads = gavl_video_options_get_num_threads(opt);
  
  FREE_FRAME(di->cur);
  FREE_FRAME(di->prev);
  FREE_FRAME(di->next);
  
  format->pixelformat = gavl_pixelformat_get_best(format->pixelformat,
                                                  pixelformats,
                                                  NULL);

#ifdef HAVE_MMX
  di->mmx = 0;
#endif
 
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
#ifdef HAVE_MMX
      if((di->accel_flags & GAVL_ACCEL_MMXEXT) &&
         ((format->image_width / sub_h) % 4 == 0))
        {
        di->filter_line = filter_line_mmx2;
        // fprintf(stderr, "Using mmxext\n");
        di->mmx = 1;
        }
      else
        {
#endif
        di->filter_line = filter_line_c;
#ifdef HAVE_MMX
        }
#endif
      
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

  gavl_video_format_copy(&frame_format, &di->in_format);
  
  frame_format.frame_height = frame_format.image_height + 4 * sub_v;
  
  di->cur = gavl_video_frame_create(&frame_format);
  di->prev = gavl_video_frame_create(&frame_format);
  di->next = gavl_video_frame_create(&frame_format);
  gavl_video_frame_clear(di->cur, &frame_format);
  gavl_video_frame_clear(di->prev, &frame_format);
  gavl_video_frame_clear(di->next, &frame_format);
  
  di->luma_shift   = di->cur->strides[0] * 2;
  di->chroma_shift = di->cur->strides[1] * 2;
  
  SHIFT_PLANES(di->cur);  
  SHIFT_PLANES(di->prev);  
  SHIFT_PLANES(di->next);
  
  di->parity =
    (di->in_format.interlace_mode == GAVL_INTERLACE_BOTTOM_FIRST) ? 1 : 0;
  
  di->mode = mode;
  gavl_video_format_copy(out_format, &di->out_format);
  }


static void filter_plane(void * priv, int start, int end)
  {
  int y;

  uint8_t *dst;
  int dst_stride;
  const uint8_t *prev0;
  const uint8_t *cur0;
  const uint8_t *next0;
  int src_stride;
  int w;
  bg_yadif_t * di = priv;
  dst_stride = di->dst->strides[di->comp->plane];
  src_stride = di->prev->strides[di->comp->plane];
    
  dst        = di->dst->planes[di->comp->plane] + di->comp->offset;
  prev0 = di->prev->planes[di->comp->plane] + di->comp->offset;
  cur0 = di->cur->planes[di->comp->plane] + di->comp->offset;
  next0 = di->next->planes[di->comp->plane] + di->comp->offset;
  
  w = di->comp->w;
  
  for(y=start; y<end; y++)
    {
    if(((y ^ di->current_parity) & 1))
      {
      const uint8_t *prev= prev0 + y*src_stride;
      const uint8_t *cur = cur0 + y*src_stride;
      const uint8_t *next= next0 + y*src_stride;
      uint8_t *dst2= dst + y*dst_stride;
      di->filter_line(di->mode, dst2, prev, cur, next, w,
                      src_stride, (di->current_parity ^ di->tff),
                      di->comp->advance);
      }
    else
      {
      memcpy(dst + y*dst_stride, cur0 + y*src_stride, w); // copy original
      }
    }
  
#ifdef HAVE_MMX
  if (di->mmx)
    asm volatile("emms");
#endif
  }

static void filter_frame(bg_yadif_t * di, int parity, 
                         gavl_video_frame_t * out)
  {
  int i;
  
  di->current_parity = parity;
  
  switch(di->in_format.interlace_mode)
    {
    case GAVL_INTERLACE_NONE:
    case GAVL_INTERLACE_UNKNOWN:
    case GAVL_INTERLACE_TOP_FIRST:
      di->tff = 1;
      break;
    case GAVL_INTERLACE_BOTTOM_FIRST:
      di->tff = 0;
      break;
    case GAVL_INTERLACE_MIXED:
    case GAVL_INTERLACE_MIXED_TOP:
    case GAVL_INTERLACE_MIXED_BOTTOM:

      switch(di->cur->interlace_mode)
        {
        case GAVL_INTERLACE_NONE:
          //          fprintf(stderr, "Progressive frame in mixed sequence\n");
          gavl_video_frame_copy(&di->out_format, out, di->cur);
          return;
          break;
        case GAVL_INTERLACE_TOP_FIRST:
          //          fprintf(stderr, "Top first\n");
          di->tff = 1;
          break;
        case GAVL_INTERLACE_BOTTOM_FIRST:
          //          fprintf(stderr, "Bottom first\n");
          di->tff = 0;
          break;
        default:
          return;
        }
      break;
    }
  
  di->dst = out;

  if(di->num_threads < 2)
    {
    for(i = 0; i < di->num_components; i++)
      {
      di->comp = &di->components[i];
      filter_plane(di, 0, di->comp->h);
      }
    }
  else
    {
    int j, nt, scanline, delta;
    for(i = 0; i < di->num_components; i++)
      {
      di->comp = &di->components[i];
      nt = di->num_threads;
      if(nt > di->comp->h)
        nt = di->comp->h;

      delta = di->comp->h / nt;
      scanline = 0;
      for(j = 0; j < nt - 1; j++)
        {
        di->run_func(filter_plane, di, scanline, scanline+delta, di->run_data, j);
        
        //        fprintf(stderr, "scanline: %d (%d)\n", ctx->scanline, ctx->dst_rect.h);
        scanline += delta;
        }
      di->run_func(filter_plane, di, scanline, di->comp->h, di->run_data, nt - 1);
      
      for(j = 0; j < nt; j++)
        di->stop_func(di->stop_data, j);
      }
    
    }
  }

gavl_source_status_t
bg_yadif_read(void * priv, gavl_video_frame_t ** frame, gavl_video_source_t * src)
  {
  gavl_source_status_t st;
  bg_yadif_t * di = priv;
  
  if(!di->field)
    {
    if(di->eof)
      return 0;

    /* Fill buffer */
    if(!di->frame)
      {
      st = gavl_video_source_read_frame(src, &di->cur);
      if(st != GAVL_SOURCE_OK)
        return st;
      di->frame++;
      }
    if(di->frame == 1)
      {
      st = gavl_video_source_read_frame(src, &di->next);
      if(st != GAVL_SOURCE_OK)
        return st;
      gavl_video_frame_copy(&di->in_format, di->prev, di->next);
      di->frame++;
      }
    else
      {
      gavl_video_frame_t * tmp;
      tmp = di->prev;

      di->prev = di->cur;
      di->cur = di->next;
      di->next = tmp;

      st = gavl_video_source_read_frame(src, &di->next);
      if(st != GAVL_SOURCE_OK)
        {
        if(st == GAVL_SOURCE_EOF)
          {
          di->eof = 1;
          gavl_video_frame_copy(&di->in_format, di->next, di->prev);
          }
        else
          return st;
        }
      }
    
    di->frame++;
    
    /* Output first field */
    filter_frame(di, di->parity, *frame);

    if(di->mode & 1)
      di->field++;
    else
      di->field = 0;

    (*frame)->timestamp = gavl_time_rescale(di->in_format.timescale,
                                         di->out_format.timescale,
                                         di->cur->timestamp);
    (*frame)->timecode = di->cur->timecode;
    (*frame)->duration = di->cur->duration;
    }
  else
    {
    /* Output second field */
    filter_frame(di, 1 - di->parity, *frame);
    
    (*frame)->timestamp = gavl_time_rescale(di->in_format.timescale,
                                         di->out_format.timescale,
                                         di->cur->timestamp) + di->cur->duration;
    (*frame)->timecode = di->cur->timecode;
    (*frame)->duration = di->cur->duration;
    
    di->field = 0;
    }

  //  fprintf(stderr, "PTS: %ld (%f)\n", frame->timestamp, gavl_time_to_seconds(gavl_time_unscale(di->out_format.timescale,
  //                                                                                          frame->timestamp)));
  
  return GAVL_SOURCE_OK;
  }

void bg_yadif_reset(bg_yadif_t * di)
  {
  di->frame = 0;
  di->field = 0;
  di->eof = 0;
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
#undef CHECK

#ifdef HAVE_MMX

#define LOAD4(mem,dst) \
            "movd      "mem", "#dst" \n\t"\
            "punpcklbw %%mm7, "#dst" \n\t"

#define PABS(tmp,dst) \
            "pxor     "#tmp", "#tmp" \n\t"\
            "psubw    "#dst", "#tmp" \n\t"\
            "pmaxsw   "#tmp", "#dst" \n\t"

#define CHECK(pj,mj) \
            "movq "#pj"(%[cur],%[mrefs]), %%mm2 \n\t" /* cur[x-refs-1+j] */\
            "movq "#mj"(%[cur],%[prefs]), %%mm3 \n\t" /* cur[x+refs-1-j] */\
            "movq      %%mm2, %%mm4 \n\t"\
            "movq      %%mm2, %%mm5 \n\t"\
            "pxor      %%mm3, %%mm4 \n\t"\
            "pavgb     %%mm3, %%mm5 \n\t"\
            "pand     %[pb1], %%mm4 \n\t"\
            "psubusb   %%mm4, %%mm5 \n\t"\
            "psrlq     $8,    %%mm5 \n\t"\
            "punpcklbw %%mm7, %%mm5 \n\t" /* (cur[x-refs+j] + cur[x+refs-j])>>1 */\
            "movq      %%mm2, %%mm4 \n\t"\
            "psubusb   %%mm3, %%mm2 \n\t"\
            "psubusb   %%mm4, %%mm3 \n\t"\
            "pmaxub    %%mm3, %%mm2 \n\t"\
            "movq      %%mm2, %%mm3 \n\t"\
            "movq      %%mm2, %%mm4 \n\t" /* ABS(cur[x-refs-1+j] - cur[x+refs-1-j]) */\
            "psrlq      $8,   %%mm3 \n\t" /* ABS(cur[x-refs  +j] - cur[x+refs  -j]) */\
            "psrlq     $16,   %%mm4 \n\t" /* ABS(cur[x-refs+1+j] - cur[x+refs+1-j]) */\
            "punpcklbw %%mm7, %%mm2 \n\t"\
            "punpcklbw %%mm7, %%mm3 \n\t"\
            "punpcklbw %%mm7, %%mm4 \n\t"\
            "paddw     %%mm3, %%mm2 \n\t"\
            "paddw     %%mm4, %%mm2 \n\t" /* score */

#define CHECK1 \
            "movq      %%mm0, %%mm3 \n\t"\
            "pcmpgtw   %%mm2, %%mm3 \n\t" /* if(score < spatial_score) */\
            "pminsw    %%mm2, %%mm0 \n\t" /* spatial_score= score; */\
            "movq      %%mm3, %%mm6 \n\t"\
            "pand      %%mm3, %%mm5 \n\t"\
            "pandn     %%mm1, %%mm3 \n\t"\
            "por       %%mm5, %%mm3 \n\t"\
            "movq      %%mm3, %%mm1 \n\t" /* spatial_pred= (cur[x-refs+j] + cur[x+refs-j])>>1; */

#define CHECK2 /* pretend not to have checked dir=2 if dir=1 was bad.\
                  hurts both quality and speed, but matches the C version. */\
            "paddw    %[pw1], %%mm6 \n\t"\
            "psllw     $14,   %%mm6 \n\t"\
            "paddsw    %%mm6, %%mm2 \n\t"\
            "movq      %%mm0, %%mm3 \n\t"\
            "pcmpgtw   %%mm2, %%mm3 \n\t"\
            "pminsw    %%mm2, %%mm0 \n\t"\
            "pand      %%mm3, %%mm5 \n\t"\
            "pandn     %%mm1, %%mm3 \n\t"\
            "por       %%mm5, %%mm3 \n\t"\
            "movq      %%mm3, %%mm1 \n\t"

static void filter_line_mmx2(int mode, uint8_t *dst, const uint8_t *prev, const uint8_t *cur, const uint8_t *next, int w, int refs, int parity, int advance){
    static const uint64_t pw_1 = 0x0001000100010001ULL;
    static const uint64_t pb_1 = 0x0101010101010101ULL;
//    const int mode = p->mode;
    uint64_t tmp0, tmp1, tmp2, tmp3;
    int x;

#define FILTER\
    for(x=0; x<w; x+=4){\
        asm volatile(\
            "pxor      %%mm7, %%mm7 \n\t"\
            LOAD4("(%[cur],%[mrefs])", %%mm0) /* c = cur[x-refs] */\
            LOAD4("(%[cur],%[prefs])", %%mm1) /* e = cur[x+refs] */\
            LOAD4("(%["prev2"])", %%mm2) /* prev2[x] */\
            LOAD4("(%["next2"])", %%mm3) /* next2[x] */\
            "movq      %%mm3, %%mm4 \n\t"\
            "paddw     %%mm2, %%mm3 \n\t"\
            "psraw     $1,    %%mm3 \n\t" /* d = (prev2[x] + next2[x])>>1 */\
            "movq      %%mm0, %[tmp0] \n\t" /* c */\
            "movq      %%mm3, %[tmp1] \n\t" /* d */\
            "movq      %%mm1, %[tmp2] \n\t" /* e */\
            "psubw     %%mm4, %%mm2 \n\t"\
            PABS(      %%mm4, %%mm2) /* temporal_diff0 */\
            LOAD4("(%[prev],%[mrefs])", %%mm3) /* prev[x-refs] */\
            LOAD4("(%[prev],%[prefs])", %%mm4) /* prev[x+refs] */\
            "psubw     %%mm0, %%mm3 \n\t"\
            "psubw     %%mm1, %%mm4 \n\t"\
            PABS(      %%mm5, %%mm3)\
            PABS(      %%mm5, %%mm4)\
            "paddw     %%mm4, %%mm3 \n\t" /* temporal_diff1 */\
            "psrlw     $1,    %%mm2 \n\t"\
            "psrlw     $1,    %%mm3 \n\t"\
            "pmaxsw    %%mm3, %%mm2 \n\t"\
            LOAD4("(%[next],%[mrefs])", %%mm3) /* next[x-refs] */\
            LOAD4("(%[next],%[prefs])", %%mm4) /* next[x+refs] */\
            "psubw     %%mm0, %%mm3 \n\t"\
            "psubw     %%mm1, %%mm4 \n\t"\
            PABS(      %%mm5, %%mm3)\
            PABS(      %%mm5, %%mm4)\
            "paddw     %%mm4, %%mm3 \n\t" /* temporal_diff2 */\
            "psrlw     $1,    %%mm3 \n\t"\
            "pmaxsw    %%mm3, %%mm2 \n\t"\
            "movq      %%mm2, %[tmp3] \n\t" /* diff */\
\
            "paddw     %%mm0, %%mm1 \n\t"\
            "paddw     %%mm0, %%mm0 \n\t"\
            "psubw     %%mm1, %%mm0 \n\t"\
            "psrlw     $1,    %%mm1 \n\t" /* spatial_pred */\
            PABS(      %%mm2, %%mm0)      /* ABS(c-e) */\
\
            "movq -1(%[cur],%[mrefs]), %%mm2 \n\t" /* cur[x-refs-1] */\
            "movq -1(%[cur],%[prefs]), %%mm3 \n\t" /* cur[x+refs-1] */\
            "movq      %%mm2, %%mm4 \n\t"\
            "psubusb   %%mm3, %%mm2 \n\t"\
            "psubusb   %%mm4, %%mm3 \n\t"\
            "pmaxub    %%mm3, %%mm2 \n\t"\
            "pshufw $9,%%mm2, %%mm3 \n\t"\
            "punpcklbw %%mm7, %%mm2 \n\t" /* ABS(cur[x-refs-1] - cur[x+refs-1]) */\
            "punpcklbw %%mm7, %%mm3 \n\t" /* ABS(cur[x-refs+1] - cur[x+refs+1]) */\
            "paddw     %%mm2, %%mm0 \n\t"\
            "paddw     %%mm3, %%mm0 \n\t"\
            "psubw    %[pw1], %%mm0 \n\t" /* spatial_score */\
\
            CHECK(-2,0)\
            CHECK1\
            CHECK(-3,1)\
            CHECK2\
            CHECK(0,-2)\
            CHECK1\
            CHECK(1,-3)\
            CHECK2\
\
            /* if(p->mode<2) ... */\
            "movq    %[tmp3], %%mm6 \n\t" /* diff */\
            "cmp       $2, %[mode] \n\t"\
            "jge       1f \n\t"\
            LOAD4("(%["prev2"],%[mrefs],2)", %%mm2) /* prev2[x-2*refs] */\
            LOAD4("(%["next2"],%[mrefs],2)", %%mm4) /* next2[x-2*refs] */\
            LOAD4("(%["prev2"],%[prefs],2)", %%mm3) /* prev2[x+2*refs] */\
            LOAD4("(%["next2"],%[prefs],2)", %%mm5) /* next2[x+2*refs] */\
            "paddw     %%mm4, %%mm2 \n\t"\
            "paddw     %%mm5, %%mm3 \n\t"\
            "psrlw     $1,    %%mm2 \n\t" /* b */\
            "psrlw     $1,    %%mm3 \n\t" /* f */\
            "movq    %[tmp0], %%mm4 \n\t" /* c */\
            "movq    %[tmp1], %%mm5 \n\t" /* d */\
            "movq    %[tmp2], %%mm7 \n\t" /* e */\
            "psubw     %%mm4, %%mm2 \n\t" /* b-c */\
            "psubw     %%mm7, %%mm3 \n\t" /* f-e */\
            "movq      %%mm5, %%mm0 \n\t"\
            "psubw     %%mm4, %%mm5 \n\t" /* d-c */\
            "psubw     %%mm7, %%mm0 \n\t" /* d-e */\
            "movq      %%mm2, %%mm4 \n\t"\
            "pminsw    %%mm3, %%mm2 \n\t"\
            "pmaxsw    %%mm4, %%mm3 \n\t"\
            "pmaxsw    %%mm5, %%mm2 \n\t"\
            "pminsw    %%mm5, %%mm3 \n\t"\
            "pmaxsw    %%mm0, %%mm2 \n\t" /* max */\
            "pminsw    %%mm0, %%mm3 \n\t" /* min */\
            "pxor      %%mm4, %%mm4 \n\t"\
            "pmaxsw    %%mm3, %%mm6 \n\t"\
            "psubw     %%mm2, %%mm4 \n\t" /* -max */\
            "pmaxsw    %%mm4, %%mm6 \n\t" /* diff= MAX3(diff, min, -max); */\
            "1: \n\t"\
\
            "movq    %[tmp1], %%mm2 \n\t" /* d */\
            "movq      %%mm2, %%mm3 \n\t"\
            "psubw     %%mm6, %%mm2 \n\t" /* d-diff */\
            "paddw     %%mm6, %%mm3 \n\t" /* d+diff */\
            "pmaxsw    %%mm2, %%mm1 \n\t"\
            "pminsw    %%mm3, %%mm1 \n\t" /* d = clip(spatial_pred, d-diff, d+diff); */\
            "packuswb  %%mm1, %%mm1 \n\t"\
\
            :[tmp0]"=m"(tmp0),\
             [tmp1]"=m"(tmp1),\
             [tmp2]"=m"(tmp2),\
             [tmp3]"=m"(tmp3)\
            :[prev] "r"(prev),\
             [cur]  "r"(cur),\
             [next] "r"(next),\
             [prefs]"r"((long)refs),\
             [mrefs]"r"((long)-refs),\
             [pw1]  "m"(pw_1),\
             [pb1]  "m"(pb_1),\
             [mode] "g"(mode)\
        );\
        asm volatile("movd %%mm1, %0" :"=m"(*dst));\
        dst += 4;\
        prev+= 4;\
        cur += 4;\
        next+= 4;\
    }

    if(parity){
#define prev2 "prev"
#define next2 "cur"
        FILTER
#undef prev2
#undef next2
    }else{
#define prev2 "cur"
#define next2 "next"
        FILTER
#undef prev2
#undef next2
    }
}
#undef LOAD4
#undef PABS
#undef CHECK
#undef CHECK1
#undef CHECK2
#undef FILTER
#endif
