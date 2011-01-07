/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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


#include <avdec_private.h>

#include <string.h>
#include <stdio.h>

#include <yuv4mpeg.h>

#define LOG_DOMAIN "demux_y4m"

typedef struct
  {
  y4m_stream_info_t si;
  y4m_frame_info_t fi;

  y4m_cb_reader_t reader;
  uint8_t * tmp_planes[4]; /* For YUVA4444 */
  int64_t pts;
  } y4m_t;

/* Read function to pass to the mjpegutils */

static ssize_t read_func(void * data, void * buf, size_t len)
  {
  int result;
  bgav_input_context_t * inp = (bgav_input_context_t *)data;
  result = bgav_input_read_data(inp, (uint8_t*)buf, len);

  if(result < len)
    return len - result;
  return 0;
  }

static int probe_y4m(bgav_input_context_t * input)
  {
  uint8_t probe_data[9];

  if(bgav_input_get_data(input, probe_data, 9) < 9)
    return 0;

  if(!strncmp((char*)probe_data, "YUV4MPEG2", 9))
    return 1;
  
  return 0;
  }

#if 0

/* For debugging only (incomplete) */
static void dump_stream_header(y4m_stream_info_t * si)
  {
  bgav_dprintf( "YUV4MPEG2 stream header\n");
  bgav_dprintf( "  Image size: %dx%d\n", y4m_si_get_width(si), y4m_si_get_height(si));
  bgav_dprintf( "  Interlace mode: %d\n", y4m_si_get_interlace(si));
  bgav_dprintf( "  Chroma: %d\n", y4m_si_get_chroma(si));
  }
#endif

static int open_y4m(bgav_demuxer_context_t * ctx)
  {
  int result;
  y4m_t * priv;
  bgav_stream_t * s;
  y4m_ratio_t r;
  
  /* Tell the lib to accept new streams */

  y4m_accept_extensions(1);
  
  /* Allocate private data */
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  y4m_init_stream_info(&priv->si);

  /* Set up the reader */

  priv->reader.read = read_func;
  priv->reader.data = ctx->input;

  /* Read the stream header */

  if((result = y4m_read_stream_header_cb(&priv->reader,  &priv->si)) != Y4M_OK)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Reading stream header failed %d", result);
    return 0;
    }

  /* Create track table */

  ctx->tt = bgav_track_table_create(1);
  
  /* Set up the stream */
  s = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
  s->data.video.format.image_width  = y4m_si_get_width(&priv->si);
  s->data.video.format.image_height = y4m_si_get_height(&priv->si);
  
  s->data.video.format.frame_width  = s->data.video.format.image_width;
  s->data.video.format.frame_height = s->data.video.format.image_height;

  r = y4m_si_get_sampleaspect(&priv->si);
  s->data.video.format.pixel_width  = r.n;
  s->data.video.format.pixel_height = r.d;
  
  r = y4m_si_get_framerate(&priv->si);
  s->data.video.format.timescale      = r.n;
  s->data.video.format.frame_duration = r.d;
  
  s->flags |= STREAM_INTRA_ONLY;
  
  result = y4m_si_get_interlace(&priv->si);
  switch(result)
    {
    case Y4M_ILACE_NONE:
      s->data.video.format.interlace_mode = GAVL_INTERLACE_NONE;
      break;
    case Y4M_ILACE_TOP_FIRST:
      s->data.video.format.interlace_mode = GAVL_INTERLACE_TOP_FIRST;
      break;
    case Y4M_ILACE_BOTTOM_FIRST:
      s->data.video.format.interlace_mode = GAVL_INTERLACE_BOTTOM_FIRST;
      break;
    case Y4M_ILACE_MIXED:
      s->data.video.format.interlace_mode = GAVL_INTERLACE_MIXED;
      s->data.video.format.timescale *= 2;
      s->data.video.format.frame_duration *= 2;
      s->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
      s->data.video.frametime_mode = BGAV_FRAMETIME_PACKET;
      break;
    }
  
  result = y4m_si_get_chroma(&priv->si);

  switch(result)
    {
    case Y4M_CHROMA_420JPEG:
      s->data.video.format.pixelformat = GAVL_YUV_420_P;
      s->data.video.format.chroma_placement = GAVL_CHROMA_PLACEMENT_DEFAULT;
      break;
    case Y4M_CHROMA_420MPEG2:
      s->data.video.format.pixelformat = GAVL_YUV_420_P;
      s->data.video.format.chroma_placement = GAVL_CHROMA_PLACEMENT_MPEG2;
      break;
    case Y4M_CHROMA_420PALDV:
      s->data.video.format.pixelformat = GAVL_YUV_420_P;
      s->data.video.format.chroma_placement = GAVL_CHROMA_PLACEMENT_DVPAL;
      break;
    case Y4M_CHROMA_444:
      s->data.video.format.pixelformat = GAVL_YUV_444_P;
      break;
    case Y4M_CHROMA_422:
      s->data.video.format.pixelformat = GAVL_YUV_422_P;
      break;
    case Y4M_CHROMA_411:
      s->data.video.format.pixelformat = GAVL_YUV_411_P;
      break;
    case Y4M_CHROMA_MONO:
      /* Monochrome isn't supported by gavl, we choose the format with the
         smallest chroma planes to save memory */
      s->data.video.format.pixelformat = GAVL_YUV_410_P;
      break;
    case Y4M_CHROMA_444ALPHA:
      /* Must be converted to packed */
      s->data.video.format.pixelformat = GAVL_YUVA_32;
      priv->tmp_planes[0] = malloc(s->data.video.format.image_width *
                                 s->data.video.format.image_height * 4);
      
      priv->tmp_planes[1] = priv->tmp_planes[0] +
        s->data.video.format.image_width * s->data.video.format.image_height;
      priv->tmp_planes[2] = priv->tmp_planes[1] +
        s->data.video.format.image_width * s->data.video.format.image_height;
      priv->tmp_planes[3] = priv->tmp_planes[2] +
        s->data.video.format.image_width * s->data.video.format.image_height;
      break;
    }

  s->fourcc = BGAV_MK_FOURCC('g','a','v','l');

  /* Initialize frame info (will be needed for reading later on) */
  y4m_init_frame_info(&priv->fi);

  ctx->stream_description = bgav_sprintf("yuv4mpeg");
  ctx->index_mode = INDEX_MODE_SIMPLE;
  return 1;
  }

static const uint8_t y_8_to_yj_8[256] = 
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07, 0x08, 
  0x09, 0x0a, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 
  0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x1a, 0x1b, 
  0x1c, 0x1d, 0x1e, 0x1f, 0x21, 0x22, 0x23, 0x24, 
  0x25, 0x26, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 
  0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x36, 0x37, 
  0x38, 0x39, 0x3a, 0x3b, 0x3d, 0x3e, 0x3f, 0x40, 
  0x41, 0x42, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 
  0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x52, 0x53, 
  0x54, 0x55, 0x56, 0x57, 0x58, 0x5a, 0x5b, 0x5c, 
  0x5d, 0x5e, 0x5f, 0x61, 0x62, 0x63, 0x64, 0x65, 
  0x66, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6f, 
  0x70, 0x71, 0x72, 0x73, 0x74, 0x76, 0x77, 0x78, 
  0x79, 0x7a, 0x7b, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 
  0x82, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8b, 
  0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x92, 0x93, 0x94, 
  0x95, 0x96, 0x97, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 
  0x9e, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa7, 
  0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xaf, 0xb0, 
  0xb1, 0xb2, 0xb3, 0xb4, 0xb6, 0xb7, 0xb8, 0xb9, 
  0xba, 0xbb, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 
  0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xcb, 0xcc, 
  0xcd, 0xce, 0xcf, 0xd0, 0xd2, 0xd3, 0xd4, 0xd5, 
  0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe7, 0xe8, 
  0xe9, 0xea, 0xeb, 0xec, 0xee, 0xef, 0xf0, 0xf1, 
  0xf2, 0xf3, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 
  0xfc, 0xfd, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
};


static void convert_yuva4444(uint8_t ** dst, uint8_t ** src, int size)
  {
  int i;
  uint8_t *y, *u, *v, *a, *d;
  
  y = src[0];
  u = src[1];
  v = src[2];
  a = src[3];

  d = dst[0];
  
  for(i = 0; i < size; i++)
    {
    *(d++) = *(y++);
    *(d++) = *(u++);
    *(d++) = *(v++);
    *(d++) = y_8_to_yj_8[*(a++)];
    }
  }

static int select_track_y4m(bgav_demuxer_context_t * ctx, int track)
  {
  y4m_t * priv;
  priv = (y4m_t*)(ctx->priv);
  priv->pts = 0;
  return 1;
  }


static int next_packet_y4m(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * s;
  y4m_t * priv;
    
  priv = (y4m_t*)(ctx->priv);
  
  s = ctx->tt->cur->video_streams;
  
  p = bgav_stream_get_packet_write(s);
  p->position = ctx->input->position;
  if(!p->video_frame)
    {
    p->video_frame = gavl_video_frame_create_nopad(&s->data.video.format);
    
    /* For monochrome format also need to clear the chroma planes */
    gavl_video_frame_clear(p->video_frame, &s->data.video.format);
    }
  
  if(s->data.video.format.pixelformat == GAVL_YUVA_32)
    {
    if(y4m_read_frame_cb(&priv->reader, &priv->si,
                         &priv->fi, priv->tmp_planes) != Y4M_OK)
      return 0;
    convert_yuva4444(p->video_frame->planes, priv->tmp_planes,
                     s->data.video.format.image_width * 
                     s->data.video.format.image_height);
    }
  else
    {
    if(y4m_read_frame_cb(&priv->reader, &priv->si,
                         &priv->fi, p->video_frame->planes) != Y4M_OK)
      return 0;
    }
  
  p->pts = priv->pts;
  p->video_frame->timestamp = p->pts;

  PACKET_SET_KEYFRAME(p);

  if(s->data.video.format.interlace_mode == GAVL_INTERLACE_MIXED)
    {
    switch(y4m_fi_get_presentation(&priv->fi))
      {
      case Y4M_PRESENT_TOP_FIRST:  /* top-field-first                 */
        p->video_frame->interlace_mode = GAVL_INTERLACE_TOP_FIRST;
        p->duration = s->data.video.format.frame_duration;
        break;
      case Y4M_PRESENT_TOP_FIRST_RPT:  /* top-first, repeat top           */
        p->video_frame->interlace_mode = GAVL_INTERLACE_TOP_FIRST;
        p->duration = (s->data.video.format.frame_duration * 3)/2;
        break;
      case Y4M_PRESENT_BOTTOM_FIRST:  /* bottom-field-first              */
        p->video_frame->interlace_mode = GAVL_INTERLACE_BOTTOM_FIRST;
        p->duration = s->data.video.format.frame_duration;
        break;
      case Y4M_PRESENT_BOTTOM_FIRST_RPT:  /* bottom-first, repeat bottom     */
        p->video_frame->interlace_mode = GAVL_INTERLACE_BOTTOM_FIRST;
        p->duration = (s->data.video.format.frame_duration * 3)/2;
        break;
      case Y4M_PRESENT_PROG_SINGLE:  /* single progressive frame        */
        p->video_frame->interlace_mode = GAVL_INTERLACE_NONE;
        p->duration = s->data.video.format.frame_duration;
        break;
      case Y4M_PRESENT_PROG_DOUBLE:  /* progressive frame, repeat once  */
        p->video_frame->interlace_mode = GAVL_INTERLACE_NONE;
        p->duration = 2 * s->data.video.format.frame_duration;
        break;
      case Y4M_PRESENT_PROG_TRIPLE:  /* progressive frame, repeat twice */
        p->video_frame->interlace_mode = GAVL_INTERLACE_NONE;
        p->duration = 3 * s->data.video.format.frame_duration;
        break;
      }
    }
  else
    {
    p->duration = s->data.video.format.frame_duration;
    }
  p->video_frame->duration = p->duration;

  priv->pts += p->duration;
  
  bgav_stream_done_packet_write(s, p);
  
  return 1;
  }

static void resync_y4m(bgav_demuxer_context_t * ctx, bgav_stream_t * s)
  {
  y4m_t * priv;
  priv = (y4m_t *)(ctx->priv);
  priv->pts = STREAM_GET_SYNC(s);
  }

static void close_y4m(bgav_demuxer_context_t * ctx)
  {
  y4m_t * priv;
  priv = (y4m_t *)(ctx->priv);
  y4m_fini_stream_info(&priv->si);
  y4m_fini_frame_info(&priv->fi);
  if(priv->tmp_planes[0])
    free(priv->tmp_planes[0]);
  free(priv);
  }

const bgav_demuxer_t bgav_demuxer_y4m =
  {
    .probe        = probe_y4m,
    .open         = open_y4m,
    .select_track = select_track_y4m,
    .next_packet = next_packet_y4m,
    .resync      = resync_y4m,
    .close =       close_y4m
  };
