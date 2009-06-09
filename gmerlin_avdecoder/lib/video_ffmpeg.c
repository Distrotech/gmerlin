/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <bswap.h>
#include <avdec_private.h>
#include <codecs.h>
#include <ptscache.h>

#include <stdio.h>

#include AVCODEC_HEADER

#include <dvframe.h>

#ifdef HAVE_LIBPOSTPROC
#include POSTPROC_HEADER
#endif

#ifdef HAVE_LIBSWSCALE
#include SWSCALE_HEADER
#endif

#define LOG_DOMAIN "ffmpeg_video"

/* We support either pts based or constant framerate. No support for libmpeg2 style
   variable framerates.
   
   When we use a file index and extract extradata from the stream, we assume, that
   the extradata is in the same packet as the beginning of the first keyframe.
   This seems always be true for H.264.
*/

// #define DUMP_DECODE
// #define DUMP_EXTRADATA

/* Map of ffmpeg codecs to fourccs (from ffmpeg's avienc.c) */

#define HAS_DELAY      (1<<0)
#define HAS_GET_BUFFER (1<<1)

typedef struct
  {
  const char * decoder_name;
  const char * format_name;
  enum CodecID ffmpeg_id;
  uint32_t * fourccs;
  } codec_info_t;

typedef struct
  {
  AVCodecContext * ctx;
  AVFrame * frame;
  //  enum CodecID ffmpeg_id;
  codec_info_t * info;
  gavl_video_frame_t * gavl_frame;
    
  /* Pixelformat */
  int do_convert;
  enum PixelFormat dst_format;

  /* Real video ugliness */
  uint32_t rv_extradata[2+FF_INPUT_BUFFER_PADDING_SIZE/4];
  AVPaletteControl palette;

  /* State variables */
  int need_format;

  uint8_t * extradata;
  int extradata_size;
  
  int demuxer_eof;
  int codec_eof;
  
  //  packet_info_t packets[FF_MAX_B_FRAMES+1];
  bgav_packet_t * packet;
  
  int flags;

#ifdef HAVE_LIBPOSTPROC
  int do_pp;
  pp_context_t *pp_context;
  pp_mode_t    *pp_mode;
#endif

#ifdef HAVE_LIBSWSCALE
  struct SwsContext *swsContext;
#endif
  
  gavl_video_frame_t * flip_frame; /* Only used if we flip AND do postprocessing */
  
  /* Swap fields for MJPEG-A/B bottom first */
  int swap_fields;
  gavl_video_frame_t  * src_field;
  gavl_video_frame_t  * dst_field;
  gavl_video_format_t field_format;
  
  /* */
  //  AVCodecParserContext * parser;
  //  int parser_started;
    
  bgav_dv_dec_t * dvdec;
  
  gavl_timecode_t last_dv_timecode;

  uint8_t * frame_buffer;
  int frame_buffer_len;

  bgav_pts_cache_t pts_cache;

  int64_t picture_timestamp;
  int     picture_duration;
  
  int64_t skip_time;

#if LIBAVCODEC_BUILD >= ((52<<16)+(26<<8)+0)
  AVPacket pkt;
#endif
  } ffmpeg_video_priv;

static int my_get_buffer(struct AVCodecContext *c, AVFrame *pic)
  {
  int ret;
  ffmpeg_video_priv * priv;
  bgav_pts_cache_entry_t * e;
  
  bgav_stream_t * s = (bgav_stream_t *)c->opaque;
  priv = s->data.video.decoder->priv;
  ret = avcodec_default_get_buffer(c, pic);

  //  fprintf(stderr, "Got packet 2 %ld %ld\n",
  //          priv->packet->position, priv->packet->pts);

  bgav_pts_cache_push(&priv->pts_cache,
                      priv->packet->pts,
                      priv->packet->duration,
                      (int*)0, &e);

  pic->opaque= e;
  return ret;
  }

static codec_info_t * lookup_codec(bgav_stream_t * s);

/* This MUST match demux_rm.c!! */

typedef struct dp_hdr_s {
    uint32_t chunks;    // number of chunks
    uint32_t timestamp; // timestamp from packet header
    uint32_t len;       // length of actual data
    uint32_t chunktab;  // offset to chunk offset array
} dp_hdr_t;

static int get_data(bgav_stream_t * s)
  {
  ffmpeg_video_priv * priv = s->data.video.decoder->priv;
  
  if(priv->packet)
    {
    bgav_demuxer_done_packet_read(s->demuxer, priv->packet);
    priv->packet = NULL;
    }

  priv->packet = bgav_demuxer_get_packet_read(s->demuxer, s);

  //  fprintf(stderr, "Got packet %d\n", PACKET_GET_KEYFRAME(priv->packet));
  
  if(!priv->packet)
    return 0;
  return 1;
  }

static void get_format(AVCodecContext * ctx, gavl_video_format_t * format);
static void put_frame(bgav_stream_t * s, gavl_video_frame_t * f);

#ifdef HAVE_LIBPOSTPROC
static void init_pp(bgav_stream_t * s);
#endif

/* Codec specific hacks */

static void handle_dv(bgav_stream_t * s);

#if 0
static int frame_dumped = 0;
static void dump_frame(uint8_t * data, int len)
  {
  FILE * out;
  if(frame_dumped)
    return;
  frame_dumped = 1;
  out = fopen("frame.dat", "wb");
  fwrite(data, 1, len, out);
  fclose(out);
  }
#endif

static int decode_picture(bgav_stream_t * s)
  {
  int i, imax;
  int bytes_used;
  dp_hdr_t *hdr;
  ffmpeg_video_priv * priv;
  bgav_pts_cache_entry_t * e;
  int have_picture = 0;
  
  priv = s->data.video.decoder->priv;
  
  while(1)
    {
    /* Read data if necessary */
    if(!get_data(s))
      {
      if(priv->demuxer_eof)
        {
        if(priv->flags & HAS_DELAY)
          {
          priv->frame_buffer_len = 0;
          priv->frame_buffer = (uint8_t*)0;
          }
        else
          {
          priv->codec_eof = 1;
          return 0;
          }
        }
      else /* Just parsing */
        return 0;
      }

    /* Skip non-reference frames */

    if(priv->packet->pts == BGAV_TIMESTAMP_UNDEFINED)
      {
      priv->ctx->skip_frame = AVDISCARD_NONREF;
      }
    else if(priv->skip_time != BGAV_TIMESTAMP_UNDEFINED)
      {
      if(priv->packet->pts + priv->packet->duration < priv->skip_time)
        {
        priv->ctx->skip_frame = AVDISCARD_NONREF;
        // fprintf(stderr, "Skip frame %c\n", priv->packet->flags & 0xff);
        }
      else
        {
        priv->ctx->skip_frame = AVDISCARD_DEFAULT;
        if(!(priv->flags & HAS_GET_BUFFER))
          bgav_pts_cache_push(&priv->pts_cache,
                              priv->packet->pts,
                              priv->packet->duration,
                            (int*)0, &e);
        }
      }
    else
      {
      priv->ctx->skip_frame = AVDISCARD_DEFAULT;
      if(!(priv->flags & HAS_GET_BUFFER))
        bgav_pts_cache_push(&priv->pts_cache,
                            priv->packet->pts,
                            priv->packet->duration,
                            (int*)0, &e);
      
      }
    
    priv->frame_buffer = priv->packet->data;

    if(priv->packet->field2_offset)
      priv->frame_buffer_len = priv->packet->field2_offset;
    else
      priv->frame_buffer_len = priv->packet->data_size;
    
    /* Other Real Video oddities */
    if((s->fourcc == BGAV_MK_FOURCC('R', 'V', '1', '0')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '1', '3')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '2', '0')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '3', '0')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '4', '0')))
      {
      if(priv->ctx->extradata_size == 8)
        {
        hdr= (dp_hdr_t*)(priv->frame_buffer);
        if(priv->ctx->slice_offset==NULL)
          priv->ctx->slice_offset= malloc(sizeof(int)*1000);
        priv->ctx->slice_count= hdr->chunks+1;
        for(i=0; i<priv->ctx->slice_count; i++)
          priv->ctx->slice_offset[i]=
            ((uint32_t*)(priv->frame_buffer+hdr->chunktab))[2*i+1];
        priv->frame_buffer_len=hdr->len;
        priv->frame_buffer += sizeof(dp_hdr_t);
        }
      }
    
    /* DV Video ugliness */
    
    if(priv->info->ffmpeg_id == CODEC_ID_DVVIDEO)
      {
      handle_dv(s);
      }
    
    /* Palette terror */
    if(s->data.video.palette_changed)
      {
      priv->palette.palette_changed = 1;
      imax =
        (s->data.video.palette_size > AVPALETTE_COUNT)
        ? AVPALETTE_COUNT : s->data.video.palette_size;
      for(i = 0; i < imax; i++)
        {
        priv->palette.palette[i] =
          ((s->data.video.palette[i].a >> 8) << 24) |
          ((s->data.video.palette[i].r >> 8) << 16) |
          ((s->data.video.palette[i].g >> 8) << 8) |
          ((s->data.video.palette[i].b >> 8));
        }
      s->data.video.palette_changed = 0;
      }
    
    /* Decode one frame */
    
#ifdef DUMP_DECODE
    bgav_dprintf("Decode: out_time: %" PRId64 " len: %d\n", s->out_time,
                 priv->frame_buffer_len);
    if(priv->frame_buffer)
      bgav_hexdump(priv->frame_buffer, 64, 16);
#endif
    
    //    dump_frame(priv->frame_buffer, priv->frame_buffer_len);

#if LIBAVCODEC_BUILD >= ((52<<16)+(26<<8)+0)
    priv->pkt.data = priv->frame_buffer;
    priv->pkt.size = priv->frame_buffer_len;
    bytes_used = avcodec_decode_video2(priv->ctx,
                                       priv->frame,
                                       &have_picture,
                                       &priv->pkt);
#else
    bytes_used = avcodec_decode_video(priv->ctx,
                                      priv->frame,
                                      &have_picture,
                                      priv->frame_buffer,
                                      priv->frame_buffer_len);
#endif
    
#ifdef DUMP_DECODE
    bgav_dprintf("Used %d/%d bytes, got picture: %d\n",
                 bytes_used, priv->frame_buffer_len, have_picture);
#endif
    
    /* Decode 2nd field for field pictures */
    if(priv->packet->field2_offset && (bytes_used > 0))
      {
      priv->frame_buffer = priv->packet->data + priv->packet->field2_offset;
      priv->frame_buffer_len = priv->packet->data_size - priv->packet->field2_offset;

#ifdef DUMP_DECODE
      bgav_dprintf("Decode (f2): out_time: %" PRId64 " len: %d\n", s->out_time,
                   priv->frame_buffer_len);
      if(priv->frame_buffer)
        bgav_hexdump(priv->frame_buffer, 16, 16);
#endif

#if LIBAVCODEC_BUILD >= ((52<<16)+(26<<8)+0)
      priv->pkt.data = priv->frame_buffer;
      priv->pkt.size = priv->frame_buffer_len;
      bytes_used = avcodec_decode_video2(priv->ctx,
                                         priv->frame,
                                         &have_picture,
                                         &priv->pkt);
#else
      
      bytes_used = avcodec_decode_video(priv->ctx,
                                        priv->frame,
                                        &have_picture,
                                        priv->frame_buffer,
                                        priv->frame_buffer_len);
#endif
      }
    
    /* If we passed no data and got no picture, we are done here */
    if(!priv->frame_buffer_len && !have_picture)
      {
      priv->codec_eof = 1;
      return 0;
      }
#ifdef DUMP_DECODE
    bgav_dprintf("Used %d/%d bytes, got picture: %d ",
                 bytes_used, priv->frame_buffer_len, have_picture);
    if(!have_picture)
      bgav_dprintf("\n");
    else
      {
      if(priv->frame->pict_type == FF_I_TYPE)
        bgav_dprintf("I-Frame ");
      else if(priv->frame->pict_type == FF_B_TYPE)
        bgav_dprintf("B-Frame ");
      else if(priv->frame->pict_type == FF_P_TYPE)
        bgav_dprintf("P-Frame ");
      else if(priv->frame->pict_type == FF_S_TYPE)
        bgav_dprintf("S-Frame ");
      else if(priv->frame->pict_type == FF_SI_TYPE)
        bgav_dprintf("SI-Frame ");
      else if(priv->frame->pict_type == FF_SP_TYPE)
        bgav_dprintf("SP-Frame ");

      bgav_dprintf("Interlaced: %d TFF: %d Repeat: %d, framerate: %f",
                   priv->frame->interlaced_frame,
                   priv->frame->top_field_first,
                   priv->frame->repeat_pict,
                   (float)(priv->ctx->time_base.den) / (float)(priv->ctx->time_base.num)
                   );
      
      bgav_dprintf("\n");
      }
#endif
    
    if(have_picture)
      {
      s->flags |= STREAM_HAVE_PICTURE; 
      break;
      }
    }

  if(priv->flags & HAS_GET_BUFFER)
    {
    e = priv->frame->opaque;
    priv->picture_timestamp = e->pts;
    priv->picture_duration  = e->duration;
    e->used = 0;
    }
  else
    {
    priv->picture_timestamp =
      bgav_pts_cache_get_first(&priv->pts_cache,
                               &priv->picture_duration);
    }
  return 1;
  }

static int skipto_ffmpeg(bgav_stream_t * s, int64_t time)
  {
  ffmpeg_video_priv * priv;
  priv = s->data.video.decoder->priv;
  priv->skip_time = time;
  while(1)
    {
    /* TODO: Skip B-frames */
#if 0
  if(!priv->do_timing && !f)
    {
    priv->ctx->skip_idct        = AVDISCARD_NONREF;
    priv->ctx->skip_loop_filter = AVDISCARD_NONREF;
    }
  else
    {
    priv->ctx->skip_idct        = AVDISCARD_DEFAULT;
    priv->ctx->skip_loop_filter = AVDISCARD_DEFAULT;
    }
#endif
   
    if(!decode_picture(s))
      return 0;

    //    fprintf(stderr, "Skipto ffmpeg %ld %ld\n",
    //            priv->picture_timestamp, time);
    
    if(priv->picture_timestamp + priv->picture_duration > time)
      break;

    }
#if 0
  fprintf(stderr, "Skipto ffmpeg %ld %ld\n",
          priv->picture_timestamp, time);
#endif
  priv->skip_time = BGAV_TIMESTAMP_UNDEFINED;
  s->out_time = priv->picture_timestamp;
  return 1;
  }

static int decode_ffmpeg(bgav_stream_t * s, gavl_video_frame_t * f)
  {
  ffmpeg_video_priv * priv;
  /* We get the DV format info ourselfes, since the values
     ffmpeg returns are not reliable */
  priv = s->data.video.decoder->priv;
  
  if(!(s->flags & STREAM_HAVE_PICTURE))
    decode_picture(s);
  
  if(s->flags & STREAM_HAVE_PICTURE)
    {
    if(f)
      put_frame(s, f);
    }
  else if(!priv->need_format)
    return 0; /* EOF */

  /* If we do the timing ourselves, we must decode one frame in advance */
  
  //  if(priv->do_timing)
  //    decode_picture(s);
  
  return 1;
  }

static int init_ffmpeg(bgav_stream_t * s)
  {
  AVCodec * codec;
  
  ffmpeg_video_priv * priv;

  //  av_log_set_level(AV_LOG_DEBUG);
  
  if((s->action == BGAV_STREAM_PARSE) &&
     (s->demuxer->index_mode != INDEX_MODE_MPEG) &&
     (s->index_mode != INDEX_MODE_MPEG))
    return 1;

  priv = calloc(1, sizeof(*priv));
  priv->skip_time = BGAV_TIMESTAMP_UNDEFINED;
  
  s->data.video.decoder->priv = priv;
  
  /* Set up coded specific details */
  
  if(s->fourcc == BGAV_MK_FOURCC('W','V','1','F'))
    s->data.video.flip_y = 1;
  
  priv->info = lookup_codec(s);
  codec = avcodec_find_decoder(priv->info->ffmpeg_id);
  priv->ctx = avcodec_alloc_context();
  priv->ctx->width = s->data.video.format.frame_width;
  priv->ctx->height = s->data.video.format.frame_height;
#if LIBAVCODEC_VERSION_INT < ((52<<16)+(0<<8)+0)
  priv->ctx->bits_per_sample = s->data.video.depth;
#else
  priv->ctx->bits_per_coded_sample = s->data.video.depth;
#endif  

  /* Setting codec tag with Nuppelvideo crashes */
  //  if(s->fourcc != BGAV_MK_FOURCC('R', 'J', 'P', 'G'))
#if 1
    {
    priv->ctx->codec_tag   =
      ((s->fourcc & 0x000000ff) << 24) |
      ((s->fourcc & 0x0000ff00) << 8) |
      ((s->fourcc & 0x00ff0000) >> 8) |
      ((s->fourcc & 0xff000000) >> 24);
    }
#endif
  priv->ctx->codec_id = codec->id;
    
  /*
   *  We always assume, that we have complete frames only.
   *  For streams, where the packets are not aligned with frames,
   *  we need an AVParser
   */
  priv->ctx->flags &= ~CODEC_FLAG_TRUNCATED;
  priv->ctx->flags |=  CODEC_FLAG_BITEXACT;

  /* Set get_buffer and release_buffer */
  
  if(codec->capabilities & CODEC_CAP_DELAY)
    {
    priv->flags |= HAS_DELAY;
#if 1
    if(!(s->flags & STREAM_WRONG_B_TIMESTAMPS))
      {
      priv->ctx->get_buffer = my_get_buffer;
      //    priv->ctx->release_buffer = my_release_buffer;
      priv->ctx->opaque = s;
      priv->flags |= HAS_GET_BUFFER;
      }
#endif
    }
  
  if(s->ext_data)
    {
    priv->extradata = calloc(s->ext_size + FF_INPUT_BUFFER_PADDING_SIZE, 1);
    memcpy(priv->extradata, s->ext_data, s->ext_size);
    priv->extradata_size = s->ext_size;

    priv->ctx->extradata      = priv->extradata;
    priv->ctx->extradata_size = priv->extradata_size;
    
#ifdef DUMP_EXTRADATA
    bgav_dprintf("video_ffmpeg: Adding extradata %d bytes\n",
                 priv->ctx->extradata_size);
    bgav_hexdump(priv->ctx->extradata, priv->ctx->extradata_size, 16);
#endif
    }
  
  if(s->action == BGAV_STREAM_PARSE)
    return 1;
  
  priv->ctx->codec_type = CODEC_TYPE_VIDEO;
  
  priv->ctx->bit_rate = 0;

  priv->ctx->time_base.den = s->data.video.format.timescale;
  priv->ctx->time_base.num = s->data.video.format.frame_duration;

  /* Build the palette from the stream info */
  
  if(s->data.video.palette_size)
    priv->ctx->palctrl = &(priv->palette);
  
  //  bgav_hexdump(s->ext_data, s->ext_size, 16);
  
  priv->frame = avcodec_alloc_frame();
  priv->gavl_frame = gavl_video_frame_create(NULL);
  
  /* Some codecs need extra stuff */

  /* Swap fields for Quicktime Motion JPEG */
  if((s->fourcc == BGAV_MK_FOURCC('m','j','p','a')) ||
     (s->fourcc == BGAV_MK_FOURCC('m','j','p','b')))
    {
    if(s->data.video.format.interlace_mode == GAVL_INTERLACE_BOTTOM_FIRST)
      {
      priv->swap_fields = 1;
      priv->src_field = gavl_video_frame_create(NULL);
      priv->dst_field = gavl_video_frame_create(NULL);
      }
    }

  
  /* Huffman tables for Motion jpeg */

  if(((s->fourcc == BGAV_MK_FOURCC('A','V','R','n')) ||
      (s->fourcc == BGAV_MK_FOURCC('M','J','P','G'))) &&
     priv->ctx->extradata_size)
    priv->ctx->flags |= CODEC_FLAG_EXTERN_HUFF;
  
  
  /* RealVideo oddities */

  if((s->fourcc == BGAV_MK_FOURCC('R','V','1','0')) ||
     (s->fourcc == BGAV_MK_FOURCC('R','V','1','3')) ||
     (s->fourcc == BGAV_MK_FOURCC('R','V','2','0')))
    {
    priv->ctx->extradata_size= 8;
    priv->ctx->extradata = (uint8_t*)priv->rv_extradata;
    
    if(priv->ctx->extradata_size != 8)
      {
      /* only 1 packet per frame & sub_id from fourcc */
      priv->rv_extradata[0] = 0;
      priv->rv_extradata[1] =
        (s->fourcc == BGAV_MK_FOURCC('R','V','1','3')) ? 0x10003001 :
        0x10000000;
      priv->ctx->sub_id = priv->rv_extradata[1];
      }
    else
      {
      /* has extra slice header (demux_rm or rm->avi streamcopy) */
      unsigned int* extrahdr=(unsigned int*)(s->ext_data);
      priv->rv_extradata[0] = be2me_32(extrahdr[0]);

      priv->ctx->sub_id = extrahdr[1];

      
      priv->rv_extradata[1] = be2me_32(extrahdr[1]);
      }

    }
  
  priv->ctx->workaround_bugs = FF_BUG_AUTODETECT;
  priv->ctx->error_concealment = 3;
  
  //  priv->ctx->error_resilience = 3;
  
  /* Open this thing */
  
  if(avcodec_open(priv->ctx, codec) != 0)
    return 0;

  //  priv->ctx->skip_frame = AVDISCARD_NONREF;
  //  priv->ctx->skip_loop_filter = AVDISCARD_ALL;
  //  priv->ctx->skip_idct = AVDISCARD_ALL;
  
  /* Set missing format values */
  priv->last_dv_timecode = GAVL_TIMECODE_UNDEFINED;
  
  priv->need_format = 1;
  
  if(!decode_picture(s))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Could not get initial frame");
    return 0;
    }
  
  get_format(priv->ctx, &s->data.video.format);
  priv->need_format = 0;
    
#ifdef HAVE_LIBPOSTPROC
  init_pp(s);
#endif
  
  /* Handle unsupported colormodels */
  if(s->data.video.format.pixelformat == GAVL_PIXELFORMAT_NONE)
    {
    s->data.video.format.pixelformat = GAVL_YUV_420_P;
    priv->do_convert = 1;
    priv->dst_format = PIX_FMT_YUV420P;

#ifdef HAVE_LIBSWSCALE
    priv->swsContext =
      sws_getContext(s->data.video.format.image_width,
                     s->data.video.format.image_height,
                     priv->ctx->pix_fmt,
                     s->data.video.format.image_width,
                     s->data.video.format.image_height,
                     priv->dst_format,
                     0, (SwsFilter*)0,
                     (SwsFilter*)0,
                     (double*)0);
#endif
    }

  if(priv->swap_fields)
    {
    gavl_video_format_copy(&(priv->field_format),
                           &(s->data.video.format));
    priv->field_format.frame_height /= 2;
    priv->field_format.image_height /= 2;
    }
  
  s->description = bgav_sprintf("%s", priv->info->format_name);
  return 1;
  }



static void resync_ffmpeg(bgav_stream_t * s)
  {
  bgav_packet_t * p;
  ffmpeg_video_priv * priv;
  priv = s->data.video.decoder->priv;
  avcodec_flush_buffers(priv->ctx);

  priv->packet = NULL;

  priv->demuxer_eof = 0;
  priv->codec_eof = 0;
  priv->last_dv_timecode = GAVL_TIMECODE_UNDEFINED;

  bgav_pts_cache_clear(&priv->pts_cache);

  /* get pictures until we have a keyframe */

  while(1)
    {
    /* Skip pictures until we have the next keyframe */
    p = bgav_demuxer_peek_packet_read(s->demuxer, s, 1);

    if(!p)
      return;

    if(PACKET_GET_KEYFRAME(p))
      {
      s->out_time = p->pts;
      break;
      }
    /* Skip this packet */
    bgav_log(s->opt, BGAV_LOG_DEBUG, LOG_DOMAIN, "Skipping packet %c", PACKET_GET_CODING_TYPE(p));
    p = bgav_demuxer_get_packet_read(s->demuxer, s);
    bgav_demuxer_done_packet_read(s->demuxer, p);
    }
  // decode_picture(s);
  }

static void close_ffmpeg(bgav_stream_t * s)
  {
  ffmpeg_video_priv * priv;
  priv= (s->data.video.decoder->priv);

  if(!priv)
    return;
  
  if(priv->ctx)
    {
    avcodec_close(priv->ctx);
    free(priv->ctx);
    }
  if(priv->gavl_frame)
    {
    gavl_video_frame_null(priv->gavl_frame);
    gavl_video_frame_destroy(priv->gavl_frame);
    }

  if(priv->src_field)
    {
    gavl_video_frame_null(priv->src_field);
    gavl_video_frame_destroy(priv->src_field);
    }
  
  if(priv->dst_field)
    {
    gavl_video_frame_null(priv->dst_field);
    gavl_video_frame_destroy(priv->dst_field);
    }

  if(priv->dvdec)
    {
    bgav_dv_dec_destroy(priv->dvdec);
    }
  
  if(priv->extradata)
    free(priv->extradata);
#ifdef HAVE_LIBPOSTPROC
  if(priv->pp_mode)
    pp_free_mode(priv->pp_mode);
  if(priv->pp_context)
    pp_free_context(priv->pp_context);
#endif
  
#ifdef HAVE_LIBSWSCALE
  if(priv->swsContext)
    sws_freeContext(priv->swsContext);
#endif
  free(priv->frame);
  free(priv);
  }

static codec_info_t codec_infos[] =
  {

/*     CODEC_ID_MPEG1VIDEO */
#if 0        
    { "FFmpeg Mpeg1 decoder", "MPEG-1", CODEC_ID_MPEG1VIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('m', 'p', 'g', '1'), 
               BGAV_MK_FOURCC('m', 'p', 'g', '2'),
               BGAV_MK_FOURCC('P', 'I', 'M', '1'), 
               BGAV_MK_FOURCC('V', 'C', 'R', '2'),
               0x00 } }, 
#endif    
/*     CODEC_ID_MPEG2VIDEO, /\* preferred ID for MPEG-1/2 video decoding *\/ */

/*     CODEC_ID_MPEG2VIDEO_XVMC, */

/*     CODEC_ID_H261, */
#if 0 // http://samples.mplayerhq.hu/V-codecs/h261/h261test.avi: Grey image
      // http://samples.mplayerhq.hu/V-codecs/h261/lotr.mov: Messed up image then crash
      // MPlayer can't play these either
    /************************************************************
     * H261 Variants
     ************************************************************/
    
    { "FFmpeg H261 decoder", "H261", CODEC_ID_H261,
      (uint32_t[]){ BGAV_MK_FOURCC('H', '2', '6', '1'),
               BGAV_MK_FOURCC('h', '2', '6', '1'),
               0x00 } },
#endif    
    
    /*     CODEC_ID_H263, */
    { "FFmpeg H263 decoder", "H263", CODEC_ID_H263,
      (uint32_t[]){ BGAV_MK_FOURCC('H', '2', '6', '3'),
                    BGAV_MK_FOURCC('h', '2', '6', '3'),
                    BGAV_MK_FOURCC('s', '2', '6', '3'),
                    BGAV_MK_FOURCC('u', '2', '6', '3'),
                    BGAV_MK_FOURCC('U', '2', '6', '3'),
                    BGAV_MK_FOURCC('v', 'i', 'v', '1'),
                    0x00 } },

    /*     CODEC_ID_RV10, */
    { "FFmpeg Real Video 1.0 decoder", "Real Video 1.0", CODEC_ID_RV10,
      (uint32_t[]){ BGAV_MK_FOURCC('R', 'V', '1', '0'),
               BGAV_MK_FOURCC('R', 'V', '1', '3'),
               0x00 } },

    /*     CODEC_ID_RV20, */
    { "FFmpeg Real Video 2.0 decoder", "Real Video 2.0", CODEC_ID_RV20,
      (uint32_t[]){ BGAV_MK_FOURCC('R', 'V', '2', '0'),
               0x00 } },

    /*     CODEC_ID_MJPEG, */
    { "FFmpeg motion JPEG decoder", "Motion JPEG", CODEC_ID_MJPEG,
      (uint32_t[]){ BGAV_MK_FOURCC('L', 'J', 'P', 'G'),
                    BGAV_MK_FOURCC('A', 'V', 'R', 'n'),
                    BGAV_MK_FOURCC('j', 'p', 'e', 'g'),
                    BGAV_MK_FOURCC('m', 'j', 'p', 'a'),
                    BGAV_MK_FOURCC('A', 'V', 'D', 'J'),
                    BGAV_MK_FOURCC('M', 'J', 'P', 'G'),
                    BGAV_MK_FOURCC('I', 'J', 'P', 'G'),
                    BGAV_MK_FOURCC('J', 'P', 'G', 'L'),
                    BGAV_MK_FOURCC('L', 'J', 'P', 'G'),
                    BGAV_MK_FOURCC('M', 'J', 'L', 'S'),
                    BGAV_MK_FOURCC('d', 'm', 'b', '1'),
                    BGAV_MK_FOURCC('J', 'F', 'I', 'F'), // SMJPEG
                    0x00 } },

    /*     CODEC_ID_MJPEGB, */
    { "FFmpeg motion Jpeg-B decoder", "Motion Jpeg B", CODEC_ID_MJPEGB,
      (uint32_t[]){ BGAV_MK_FOURCC('m', 'j', 'p', 'b'),
                    0x00 } },
    
/*     CODEC_ID_LJPEG, */

/*     CODEC_ID_SP5X, */
    { "FFmpeg SP5X decoder", "SP5X Motion JPEG", CODEC_ID_SP5X,
      (uint32_t[]){ BGAV_MK_FOURCC('S', 'P', '5', '4'),
               0x00 } },

/*     CODEC_ID_JPEGLS, */

/*     CODEC_ID_MPEG4, */
    { "FFmpeg MPEG-4 decoder", "MPEG-4", CODEC_ID_MPEG4,
      (uint32_t[]){ BGAV_MK_FOURCC('D', 'I', 'V', 'X'),
               BGAV_MK_FOURCC('d', 'i', 'v', 'x'),
               BGAV_MK_FOURCC('D', 'X', '5', '0'),
               BGAV_MK_FOURCC('X', 'V', 'I', 'D'),
               BGAV_MK_FOURCC('X', 'V', 'I', 'D'),
               BGAV_MK_FOURCC('M', 'P', '4', 'S'),
               BGAV_MK_FOURCC('M', '4', 'S', '2'),
               BGAV_MK_FOURCC(0x04, 0, 0, 0), /* some broken avi use this */
               BGAV_MK_FOURCC('D', 'I', 'V', '1'),
               BGAV_MK_FOURCC('B', 'L', 'Z', '0'),
               BGAV_MK_FOURCC('m', 'p', '4', 'v'),
               BGAV_MK_FOURCC('U', 'M', 'P', '4'),
               BGAV_MK_FOURCC('3', 'I', 'V', '2'),
               BGAV_MK_FOURCC('W', 'V', '1', 'F'),
               BGAV_MK_FOURCC('R', 'M', 'P', '4'),
               BGAV_MK_FOURCC('S', 'E', 'D', 'G'),
               BGAV_MK_FOURCC('S', 'M', 'P', '4'),
               BGAV_MK_FOURCC('3', 'I', 'V', 'D'),
               BGAV_MK_FOURCC('F', 'M', 'P', '4'),
               0x00 } },

    /*     CODEC_ID_RAWVIDEO, */
    { "FFmpeg Raw decoder", "Uncompressed", CODEC_ID_RAWVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('Y', '4', '2', '2'),
                    BGAV_MK_FOURCC('I', '4', '2', '0'),
                    0x00 } },
    
    /*     CODEC_ID_MSMPEG4V1, */
    { "FFmpeg MSMPEG4V1 decoder", "Microsoft MPEG-4 V1", CODEC_ID_MSMPEG4V1,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'P', 'G', '4'),
               BGAV_MK_FOURCC('D', 'I', 'V', '4'),
               0x00 } },

    /*     CODEC_ID_MSMPEG4V2, */
    { "FFmpeg MSMPEG4V2 decoder", "Microsoft MPEG-4 V2", CODEC_ID_MSMPEG4V2,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'P', '4', '2'),
               BGAV_MK_FOURCC('D', 'I', 'V', '2'),
               0x00 } },

    /*     CODEC_ID_MSMPEG4V3, */
    { "FFmpeg MSMPEG4V3 decoder", "Microsoft MPEG-4 V3", CODEC_ID_MSMPEG4V3,
      (uint32_t[]){ BGAV_MK_FOURCC('D', 'I', 'V', '3'),
               BGAV_MK_FOURCC('M', 'P', '4', '3'), 
               BGAV_MK_FOURCC('M', 'P', 'G', '3'), 
               BGAV_MK_FOURCC('D', 'I', 'V', '5'), 
               BGAV_MK_FOURCC('D', 'I', 'V', '6'), 
               BGAV_MK_FOURCC('D', 'I', 'V', '4'), 
               BGAV_MK_FOURCC('A', 'P', '4', '1'),
               BGAV_MK_FOURCC('C', 'O', 'L', '1'),
               BGAV_MK_FOURCC('C', 'O', 'L', '0'),
               0x00 } },
    
    /*     CODEC_ID_WMV1, */
    { "FFmpeg WMV1 decoder", "Window Media Video 7", CODEC_ID_WMV1,
      (uint32_t[]){ BGAV_MK_FOURCC('W', 'M', 'V', '1'),
               0x00 } }, 

    /*     CODEC_ID_WMV2, */
    { "FFmpeg WMV2 decoder", "Window Media Video 8", CODEC_ID_WMV2,
      (uint32_t[]){ BGAV_MK_FOURCC('W', 'M', 'V', '2'),
               0x00 } }, 
    
    /*     CODEC_ID_H263P, */

    /*     CODEC_ID_H263I, */
    { "FFmpeg H263I decoder", "I263", CODEC_ID_H263I,
      (uint32_t[]){ BGAV_MK_FOURCC('I', '2', '6', '3'), /* intel h263 */
               0x00 } },
    
    /*     CODEC_ID_FLV1, */
    { "FFmpeg Flash video decoder", "Flash Video 1", CODEC_ID_FLV1,
      (uint32_t[]){ BGAV_MK_FOURCC('F', 'L', 'V', '1'),
                    0x00 } },

    /*     CODEC_ID_SVQ1, */
    { "FFmpeg Sorenson 1 decoder", "Sorenson Video 1", CODEC_ID_SVQ1,
      (uint32_t[]){ BGAV_MK_FOURCC('S', 'V', 'Q', '1'),
               BGAV_MK_FOURCC('s', 'v', 'q', '1'),
               BGAV_MK_FOURCC('s', 'v', 'q', 'i'),
               0x00 } },

    /*     CODEC_ID_SVQ3, */
    { "FFmpeg Sorenson 3 decoder", "Sorenson Video 3", CODEC_ID_SVQ3,
      (uint32_t[]){ BGAV_MK_FOURCC('S', 'V', 'Q', '3'),
                    0x00 } },
    
    /*     CODEC_ID_DVVIDEO, */
    { "FFmpeg DV decoder", "DV Video", CODEC_ID_DVVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('d', 'v', 's', 'd'), 
                    BGAV_MK_FOURCC('D', 'V', 'S', 'D'), 
                    BGAV_MK_FOURCC('d', 'v', 'h', 'd'), 
                    BGAV_MK_FOURCC('d', 'v', 's', 'l'), 
                    BGAV_MK_FOURCC('d', 'v', '2', '5'),
                    /* Generic DV */
                    BGAV_MK_FOURCC('D', 'V', ' ', ' '),

                    BGAV_MK_FOURCC('d', 'v', 'c', 'p') , /* DV PAL */
                    BGAV_MK_FOURCC('d', 'v', 'c', ' ') , /* DV NTSC */
                    BGAV_MK_FOURCC('d', 'v', 'p', 'p') , /* DVCPRO PAL produced by FCP */
                    BGAV_MK_FOURCC('d', 'v', '5', 'p') , /* DVCPRO50 PAL produced by FCP */
                    BGAV_MK_FOURCC('d', 'v', '5', 'n') , /* DVCPRO50 NTSC produced by FCP */
                    BGAV_MK_FOURCC('A', 'V', 'd', 'v') , /* AVID DV */
                    BGAV_MK_FOURCC('A', 'V', 'd', '1') , /* AVID DV */
                    BGAV_MK_FOURCC('d', 'v', 'h', 'q') , /* DVCPRO HD 720p50 */
                    BGAV_MK_FOURCC('d', 'v', 'h', 'p') , /* DVCPRO HD 720p60 */
                    BGAV_MK_FOURCC('d', 'v', 'h', '5') , /* DVCPRO HD 50i produced by FCP */
                    BGAV_MK_FOURCC('d', 'v', 'h', '6') , /* DVCPRO HD 60i produced by FCP */
                    BGAV_MK_FOURCC('d', 'v', 'h', '3') , /* DVCPRO HD 30p produced by FCP */
                    
               0x00 } },
    
    { "FFmpeg DVCPRO50 decoder", "DVCPRO50 Video", CODEC_ID_DVVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('d', 'v', '5', 'n'),
               BGAV_MK_FOURCC('d', 'v', '5', 'p'),
               0x00 } },

    /*     CODEC_ID_HUFFYUV, */
    { "FFmpeg Hufyuv decoder", "Huff YUV", CODEC_ID_HUFFYUV,
      (uint32_t[]){ BGAV_MK_FOURCC('H', 'F', 'Y', 'U'),
                    0x00 } },

    /*     CODEC_ID_CYUV, */
    { "FFmpeg Creative YUV decoder", "Creative YUV", CODEC_ID_CYUV,
      (uint32_t[]){ BGAV_MK_FOURCC('C', 'Y', 'U', 'V'),
                    BGAV_MK_FOURCC('c', 'y', 'u', 'v'),
               0x00 } },

    /*     CODEC_ID_H264, */
    { "FFmpeg H264 decoder", "H264", CODEC_ID_H264,
      (uint32_t[]){ BGAV_MK_FOURCC('a', 'v', 'c', '1'),
               BGAV_MK_FOURCC('H', '2', '6', '4'),
               BGAV_MK_FOURCC('h', '2', '6', '4'),
               0x00 } },

    /*     CODEC_ID_INDEO3, */
    { "FFmpeg Inteo 3 decoder", "Intel Indeo 3", CODEC_ID_INDEO3,
      (uint32_t[]){ BGAV_MK_FOURCC('I', 'V', '3', '1'),
                    BGAV_MK_FOURCC('I', 'V', '3', '2'),
                    BGAV_MK_FOURCC('i', 'v', '3', '1'),
                    BGAV_MK_FOURCC('i', 'v', '3', '2'),
                    0x00 } },

    /*     CODEC_ID_VP3, */
    { "FFmpeg VP3 decoder", "On2 VP3", CODEC_ID_VP3,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '3', '1'),
                    BGAV_MK_FOURCC('V', 'P', '3', ' '),
               0x00 } },

    /*     CODEC_ID_THEORA, */

    /*     CODEC_ID_ASV1, */
    { "FFmpeg ASV1 decoder", "Asus v1", CODEC_ID_ASV1,
      (uint32_t[]){ BGAV_MK_FOURCC('A', 'S', 'V', '1'),
               0x00 } },
    
    /*     CODEC_ID_ASV2, */
    { "FFmpeg ASV2 decoder", "Asus v2", CODEC_ID_ASV2,
      (uint32_t[]){ BGAV_MK_FOURCC('A', 'S', 'V', '2'),
                    0x00 } },
    
    /*     CODEC_ID_FFV1, */
    
    { "FFmpeg Video 1 (FFV1) decoder", "FFV1", CODEC_ID_FFV1,
      (uint32_t[]){ BGAV_MK_FOURCC('F', 'F', 'V', '1'),
               0x00 } },
    
    /*     CODEC_ID_4XM, */
    { "FFmpeg 4XM video decoder", "4XM Video", CODEC_ID_4XM,
      (uint32_t[]){ BGAV_MK_FOURCC('4', 'X', 'M', 'V'),

                    0x00 } },

    /*     CODEC_ID_VCR1, */
    { "FFmpeg VCR1 decoder", "ATI VCR1", CODEC_ID_VCR1,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'C', 'R', '1'),
               0x00 } },

    /*     CODEC_ID_CLJR, */
    { "FFmpeg CLJR decoder", "Cirrus Logic AccuPak", CODEC_ID_CLJR,
      (uint32_t[]){ BGAV_MK_FOURCC('C', 'L', 'J', 'R'),
               0x00 } },

    /*     CODEC_ID_MDEC, */
    { "FFmpeg MPEC video decoder", "Playstation MDEC", CODEC_ID_MDEC,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'D', 'E', 'C'),
                    0x00 } },
    
    /*     CODEC_ID_ROQ, */
    { "FFmpeg ID Roq Video Decoder", "ID Roq Video", CODEC_ID_ROQ,
      (uint32_t[]){ BGAV_MK_FOURCC('R', 'O', 'Q', 'V'),
                    0x00 } },

    /*     CODEC_ID_INTERPLAY_VIDEO, */
    { "FFmpeg Interplay Video Decoder", "Interplay Video", CODEC_ID_INTERPLAY_VIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('I', 'P', 'V', 'D'),
                    0x00 } },
    
    /*     CODEC_ID_XAN_WC3, */
    
    /*     CODEC_ID_XAN_WC4, */
#if 0 /* Commented out in libavcodec as well */
    { "FFmpeg Xxan decoder", "Xan/WC3", CODEC_ID_XAN_WC4,
      (uint32_t[]){ BGAV_MK_FOURCC('X', 'x', 'a', 'n'),
               0x00 } },
#endif

    /*     CODEC_ID_RPZA, */
    
    { "FFmpeg rpza decoder", "Apple Video", CODEC_ID_RPZA,
      (uint32_t[]){ BGAV_MK_FOURCC('r', 'p', 'z', 'a'),
               BGAV_MK_FOURCC('a', 'z', 'p', 'r'),
               0x00 } },

    /*     CODEC_ID_CINEPAK, */
    { "FFmpeg cinepak decoder", "Cinepak", CODEC_ID_CINEPAK,
      (uint32_t[]){ BGAV_MK_FOURCC('c', 'v', 'i', 'd'),
               0x00 } },

    /*     CODEC_ID_WS_VQA, */
    { "FFmpeg Westwood VQA decoder", "Westwood VQA", CODEC_ID_WS_VQA,
      (uint32_t[]){ BGAV_MK_FOURCC('W', 'V', 'Q', 'A'),
                    0x00 } },
    
    /*     CODEC_ID_MSRLE, */
    { "FFmpeg MSRLE Decoder", "Microsoft RLE", CODEC_ID_MSRLE,
      (uint32_t[]){ BGAV_MK_FOURCC('W', 'R', 'L', 'E'),
               BGAV_MK_FOURCC('m', 'r', 'l', 'e'),
               BGAV_MK_FOURCC(0x1, 0x0, 0x0, 0x0),
               0x00 } },

    /*     CODEC_ID_MSVIDEO1, */
    { "FFmpeg MSVideo 1 decoder", "Microsoft Video 1", CODEC_ID_MSVIDEO1,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'S', 'V', 'C'),
               BGAV_MK_FOURCC('m', 's', 'v', 'c'),
               BGAV_MK_FOURCC('C', 'R', 'A', 'M'),
               BGAV_MK_FOURCC('c', 'r', 'a', 'm'),
               BGAV_MK_FOURCC('W', 'H', 'A', 'M'),
               BGAV_MK_FOURCC('w', 'h', 'a', 'm'),
               0x00 } },

    /*     CODEC_ID_IDCIN, */
#if 1 // Crashes
    { "FFmpeg ID CIN decoder", "ID CIN", CODEC_ID_IDCIN,
      (uint32_t[]){ BGAV_MK_FOURCC('I', 'D', 'C', 'I'),
               0x00 } },
#endif
    /*     CODEC_ID_8BPS, */
    { "FFmpeg 8BPS decoder", "Quicktime Planar RGB (8BPS)", CODEC_ID_8BPS,
      (uint32_t[]){ BGAV_MK_FOURCC('8', 'B', 'P', 'S'),
               0x00 } },
    /*     CODEC_ID_SMC, */
    { "FFmpeg SMC decoder", "Apple Graphics", CODEC_ID_SMC,
      (uint32_t[]){ BGAV_MK_FOURCC('s', 'm', 'c', ' '),
               0x00 } },


    /*     CODEC_ID_FLIC, */
    { "FFmpeg FLI/FLC Decoder", "FLI/FLC Animation", CODEC_ID_FLIC,
      (uint32_t[]){ BGAV_MK_FOURCC('F', 'L', 'I', 'C'),
               0x00 } },

    /*     CODEC_ID_TRUEMOTION1, */
    { "FFmpeg DUCK TrueMotion 1 decoder", "Duck TrueMotion 1", CODEC_ID_TRUEMOTION1,
      (uint32_t[]){ BGAV_MK_FOURCC('D', 'U', 'C', 'K'),
               0x00 } },

    /*     CODEC_ID_VMDVIDEO, */
    { "FFmpeg Sierra VMD video decoder", "Sierra VMD video",
      CODEC_ID_VMDVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'M', 'D', 'V'),
                    0x00 } },
    
    /*     CODEC_ID_MSZH, */
    { "FFmpeg MSZH decoder", "LCL MSZH", CODEC_ID_MSZH,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'S', 'Z', 'H'),
               0x00 } },

    /*     CODEC_ID_ZLIB, */
    { "FFmpeg ZLIB decoder", "LCL ZLIB", CODEC_ID_ZLIB,
      (uint32_t[]){ BGAV_MK_FOURCC('Z', 'L', 'I', 'B'),
               0x00 } },
    /*     CODEC_ID_QTRLE, */
    { "FFmpeg QT rle Decoder", "Quicktime RLE", CODEC_ID_QTRLE,
      (uint32_t[]){ BGAV_MK_FOURCC('r', 'l', 'e', ' '),
               0x00 } },

    /*     CODEC_ID_SNOW, */
    { "FFmpeg Snow decoder", "Snow", CODEC_ID_SNOW,
      (uint32_t[]){ BGAV_MK_FOURCC('S', 'N', 'O', 'W'),
                    0x00 } },
    
    /*     CODEC_ID_TSCC, */
    { "FFmpeg TSCC decoder", "TechSmith Camtasia", CODEC_ID_TSCC,
      (uint32_t[]){ BGAV_MK_FOURCC('T', 'S', 'C', 'C'),
               BGAV_MK_FOURCC('t', 's', 'c', 'c'),
               0x00 } },
    /*     CODEC_ID_ULTI, */
    { "FFmpeg ULTI decoder", "IBM Ultimotion", CODEC_ID_ULTI,
      (uint32_t[]){ BGAV_MK_FOURCC('U', 'L', 'T', 'I'),
               0x00 } },

    /*     CODEC_ID_QDRAW, */
    { "FFmpeg QDraw decoder", "Apple QuickDraw", CODEC_ID_QDRAW,
      (uint32_t[]){ BGAV_MK_FOURCC('q', 'd', 'r', 'w'),
               0x00 } },
    /*     CODEC_ID_VIXL, */
    { "FFmpeg Video XL Decoder", "Video XL", CODEC_ID_VIXL,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'I', 'X', 'L'),
               0x00 } },
    /*     CODEC_ID_QPEG, */
    { "FFmpeg QPEG decoder", "QPEG", CODEC_ID_QPEG,
      (uint32_t[]){ BGAV_MK_FOURCC('Q', '1', '.', '0'),
                    BGAV_MK_FOURCC('Q', '1', '.', '1'),
                    0x00 } },

    /*     CODEC_ID_XVID, */

    /*     CODEC_ID_PNG, */

    /*     CODEC_ID_PPM, */

    /*     CODEC_ID_PBM, */

    /*     CODEC_ID_PGM, */

    /*     CODEC_ID_PGMYUV, */

    /*     CODEC_ID_PAM, */

    /*     CODEC_ID_FFVHUFF, */
    { "FFmpeg FFVHUFF decoder", "FFmpeg Huffman", CODEC_ID_FFVHUFF,
      (uint32_t[]){ BGAV_MK_FOURCC('F', 'F', 'V', 'H'),
               0x00 } },
    
    /*     CODEC_ID_RV30, */
    /*     CODEC_ID_RV40, */

    /*     CODEC_ID_VC1, */
    { "FFmpeg VC1 decoder", "VC1", CODEC_ID_VC1,
      (uint32_t[]){ BGAV_MK_FOURCC('W', 'V', 'C', '1'),
                    BGAV_MK_FOURCC('V', 'C', '-', '1'),
                    0x00 } },
    
    /*     CODEC_ID_WMV3, */
    // #ifndef HAVE_W32DLL
    
    // [wmv3 @ 0xb63fe128]Old WMV3 version detected, only I-frames will be decoded
    { "FFmpeg WMV3 decoder", "Window Media Video 9", CODEC_ID_WMV3,
      (uint32_t[]){ BGAV_MK_FOURCC('W', 'M', 'V', '3'),
               0x00 } }, 
    // #endif

    /*     CODEC_ID_LOCO, */
    { "FFmpeg LOCO decoder", "LOCO", CODEC_ID_LOCO,
      (uint32_t[]){ BGAV_MK_FOURCC('L', 'O', 'C', 'O'),
               0x00 } },
    
    /*     CODEC_ID_WNV1, */
    { "FFmpeg WNV1 decoder", "Winnow Video 1", CODEC_ID_WNV1,
      (uint32_t[]){ BGAV_MK_FOURCC('W', 'N', 'V', '1'),
               0x00 } },

    /*     CODEC_ID_AASC, */
    { "FFmpeg AASC decoder", "Autodesk Animator Studio Codec", CODEC_ID_AASC,
      (uint32_t[]){ BGAV_MK_FOURCC('A', 'A', 'S', 'C'),
               0x00 } },

    /*     CODEC_ID_INDEO2, */
    { "FFmpeg Inteo 2 decoder", "Intel Indeo 2", CODEC_ID_INDEO2,
      (uint32_t[]){ BGAV_MK_FOURCC('R', 'T', '2', '1'),
                    0x00 } },
    /*     CODEC_ID_FRAPS, */
    { "FFmpeg Fraps 1 decoder", "Fraps 1", CODEC_ID_FRAPS,
      (uint32_t[]){ BGAV_MK_FOURCC('F', 'P', 'S', '1'),
                    0x00 } },

    /*     CODEC_ID_TRUEMOTION2, */
    { "FFmpeg DUCK TrueMotion 2 decoder", "Duck TrueMotion 2", CODEC_ID_TRUEMOTION2,
      (uint32_t[]){ BGAV_MK_FOURCC('T', 'M', '2', '0'),
               0x00 } },
    /*     CODEC_ID_BMP, */
    /*     CODEC_ID_CSCD, */
    { "FFmpeg CSCD decoder", "CamStudio Screen Codec", CODEC_ID_CSCD,
      (uint32_t[]){ BGAV_MK_FOURCC('C', 'S', 'C', 'D'),
               0x00 } },
    
    /*     CODEC_ID_MMVIDEO, */
    { "FFmpeg MM video decoder", "American Laser Games MM", CODEC_ID_MMVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'M', 'V', 'D'),
                    0x00 } },
    /*     CODEC_ID_ZMBV, */
    { "FFmpeg ZMBV decoder", "Zip Motion Blocks Video", CODEC_ID_ZMBV,
      (uint32_t[]){ BGAV_MK_FOURCC('Z', 'M', 'B', 'V'),
               0x00 } },

    /*     CODEC_ID_AVS, */
    { "FFmpeg AVS Video Decoder", "AVS Video", CODEC_ID_AVS,
      (uint32_t[]){ BGAV_MK_FOURCC('A', 'V', 'S', ' '),
                    0x00 } },
    /*     CODEC_ID_SMACKVIDEO, */
    { "FFmpeg Smacker Video Decoder", "Smacker Video", CODEC_ID_SMACKVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('S', 'M', 'K', '2'),
                    BGAV_MK_FOURCC('S', 'M', 'K', '4'),
                    0x00 } },
    /*     CODEC_ID_NUV, */
    { "FFmpeg NuppelVideo decoder", "NuppelVideo (rtjpeg)", CODEC_ID_NUV,
      (uint32_t[]){ BGAV_MK_FOURCC('R', 'J', 'P', 'G'),
                    BGAV_MK_FOURCC('N', 'U', 'V', ' '),
               0x00 } },
    /*     CODEC_ID_KMVC, */
    { "FFmpeg KMVC decoder", "Karl Morton's video codec", CODEC_ID_KMVC,
      (uint32_t[]){ BGAV_MK_FOURCC('K', 'M', 'V', 'C'),
               0x00 } },
    /*     CODEC_ID_FLASHSV, */
    { "FFmpeg Flash screen video decoder", "Flash Screen Video", CODEC_ID_FLASHSV,
      (uint32_t[]){ BGAV_MK_FOURCC('F', 'L', 'V', 'S'),
                    0x00 } },
    /*     CODEC_ID_CAVS, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(11<<8)+0)
    { "FFmpeg Chinese AVS decoder", "Chinese AVS", CODEC_ID_CAVS,
      (uint32_t[]){ BGAV_MK_FOURCC('C', 'A', 'V', 'S'),
                    0x00 } },
#endif
    /*     CODEC_ID_JPEG2000, */

    /*     CODEC_ID_VMNC, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(13<<8)+0)
    { "FFmpeg VMware video decoder", "VMware video", CODEC_ID_VMNC,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'M', 'n', 'c'),
                    0x00 } },
#endif      
    /*     CODEC_ID_VP5, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(14<<8)+0)    
    { "FFmpeg VP5 decoder", "On2 VP5", CODEC_ID_VP5,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '5', '0'),
                    0x00 } },
#endif
    /*     CODEC_ID_VP6, */

#if LIBAVCODEC_BUILD >= ((51<<16)+(14<<8)+0) 
    { "FFmpeg VP6.2 decoder", "On2 VP6.2", CODEC_ID_VP6,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '6', '2'),
                    0x00 } },
#endif

#if LIBAVCODEC_BUILD >= ((51<<16)+(27<<8)+0)
    { "FFmpeg VP6.0 decoder", "On2 VP6.0", CODEC_ID_VP6,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '6', '0'),
                    0x00 } },
    { "FFmpeg VP6.1 decoder", "On2 VP6.1", CODEC_ID_VP6,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '6', '1'),
                    0x00 } },
    { "FFmpeg VP6.2 decoder (flash variant)", "On2 VP6.2 (flash variant)", CODEC_ID_VP6F,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '6', 'F'),
                    0x00 } },
#endif
    /*     CODEC_ID_TARGA, */
    /*     CODEC_ID_DSICINVIDEO, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(18<<8)+0)
    { "FFmpeg Delphine CIN video decoder", "Delphine CIN Video", CODEC_ID_DSICINVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('d', 'c', 'i', 'n'),
               0x00 } },
#endif      
    /*     CODEC_ID_TIERTEXSEQVIDEO, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(19<<8)+0)
    { "FFmpeg Tiertex Video Decoder", "Tiertex Video", CODEC_ID_TIERTEXSEQVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('T', 'I', 'T', 'X'),
                    0x00 } },
#endif
    /*     CODEC_ID_TIFF, */
    /*     CODEC_ID_GIF, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(21<<8)+0)
    { "FFmpeg GIF Video Decoder", "GIF", CODEC_ID_GIF,
      (uint32_t[]){ BGAV_MK_FOURCC('g', 'i', 'f', ' '),
                    0x00 } },
#endif
    /*     CODEC_ID_FFH264, */
    /*     CODEC_ID_DXA, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(39<<8)+0)
    { "FFmpeg DXA decoder", "DXA", CODEC_ID_DXA,
      (uint32_t[]){ BGAV_MK_FOURCC('D', 'X', 'A', ' '),
                    0x00 } },
#endif
    /*     CODEC_ID_DNXHD, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(40<<8)+0)
    { "FFmpeg DNxHD Video decoder", "DNxHD", CODEC_ID_DNXHD,
      (uint32_t[]){ BGAV_MK_FOURCC('A', 'V', 'd', 'n'),
               0x00 } },
#endif
    /*     CODEC_ID_THP, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(40<<8)+4)
    { "FFmpeg THP Video decoder", "THP Video", CODEC_ID_THP,
      (uint32_t[]){ BGAV_MK_FOURCC('T', 'H', 'P', 'V'),
               0x00 } },
#endif
    /*     CODEC_ID_SGI, */
    /*     CODEC_ID_C93, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(40<<8)+3)
    { "FFmpeg C93 decoder", "C93", CODEC_ID_C93,
      (uint32_t[]){ BGAV_MK_FOURCC('C', '9', '3', 'V'),
                    0x00 } },
#endif
    /*     CODEC_ID_BETHSOFTVID, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(40<<8)+3)
    { "FFmpeg Bethsoft VID decoder", "Bethsoft VID",
      CODEC_ID_BETHSOFTVID,
      (uint32_t[]){ BGAV_MK_FOURCC('B','S','D','V'), 0x00 } },
#endif

    /*     CODEC_ID_PTX, */
    
    /*     CODEC_ID_TXD, */
    
    /*     CODEC_ID_VP6A, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(45<<8)+0)
    { "FFmpeg VP6 yuva decoder", "On2 VP6.0 with alpha", CODEC_ID_VP6A,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '6', 'A'),
                    0x00 } },
#endif
    /*     CODEC_ID_AMV, */
    /*     CODEC_ID_VB, */
#if LIBAVCODEC_BUILD >= ((51<<16)+(47<<8)+0)
    { "FFmpeg Beam Software VB decoder", "Beam Software VB",
      CODEC_ID_VB,
      (uint32_t[]){ BGAV_MK_FOURCC('V','B','V','1'), 0x00 } },
#endif
    
  };

#define NUM_CODECS sizeof(codec_infos)/sizeof(codec_infos[0])

static int real_num_codecs;

static struct
  {
  codec_info_t * info;
  bgav_video_decoder_t decoder;
  } codecs[NUM_CODECS];

static codec_info_t * lookup_codec(bgav_stream_t * s)
  {
  int i;
  for(i = 0; i < real_num_codecs; i++)
    {
    if(s->data.video.decoder->decoder == &(codecs[i].decoder))
      return codecs[i].info;
    }
  return (codec_info_t *)0;
  }

void bgav_init_video_decoders_ffmpeg(bgav_options_t * opt)
  {
  int i;
  AVCodec * c;
  
  real_num_codecs = 0;
  for(i = 0; i < NUM_CODECS; i++)
    {
    if((c = avcodec_find_decoder(codec_infos[i].ffmpeg_id)))
      {
      codecs[real_num_codecs].info = &(codec_infos[i]);
      codecs[real_num_codecs].decoder.name = codecs[real_num_codecs].info->decoder_name;
      
      if(c->capabilities & CODEC_CAP_DELAY) 
        {
        codecs[real_num_codecs].decoder.flags |= VCODEC_FLAG_DELAY;
        codecs[real_num_codecs].decoder.skipto = skipto_ffmpeg;
        }
      codecs[real_num_codecs].decoder.fourccs = codecs[real_num_codecs].info->fourccs;
      codecs[real_num_codecs].decoder.init = init_ffmpeg;
      codecs[real_num_codecs].decoder.decode = decode_ffmpeg;
      //      codecs[real_num_codecs].decoder.parse = parse_ffmpeg;
      codecs[real_num_codecs].decoder.close = close_ffmpeg;
      codecs[real_num_codecs].decoder.resync = resync_ffmpeg;
      bgav_video_decoder_register(&codecs[real_num_codecs].decoder);
      real_num_codecs++;
      }
    else
      {
      bgav_log(opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "Cannot find %s", codec_infos[i].decoder_name);
      }
    }
  }

static void pal8_to_rgb24(gavl_video_frame_t * dst, AVFrame * src,
                          int width, int height, int flip_y)
  {
  int i, j;
  uint32_t pixel;
  uint8_t * dst_ptr;
  uint8_t * dst_save;

  uint8_t * src_ptr;
  uint8_t * src_save;

  uint32_t * palette;

  int dst_stride;
    
  palette = (uint32_t*)(src->data[1]);
  
  if(flip_y)
    {
    dst_save = dst->planes[0] + (height - 1) * dst->strides[0];
    dst_stride = - dst->strides[0];
    }
  else
    {
    dst_save = dst->planes[0];
    dst_stride = dst->strides[0];
    }
  
  src_save = src->data[0];
  for(i = 0; i < height; i++)
    {
    src_ptr = src_save;
    dst_ptr = dst_save;
    
    for(j = 0; j < width; j++)
      {
      pixel = palette[*src_ptr];
      dst_ptr[0] = (pixel & 0x00FF0000) >> 16;
      dst_ptr[1] = (pixel & 0x0000FF00) >> 8;
      dst_ptr[2] = (pixel & 0x000000FF);
      
      dst_ptr+=3;
      src_ptr++;
      }

    src_save += src->linesize[0];
    dst_save += dst_stride;
    }
  }

static void pal8_to_rgba32(gavl_video_frame_t * dst, AVFrame * src,
                           int width, int height, int flip_y)
  {
  int i, j;
  uint32_t pixel;
  uint8_t * dst_ptr;
  uint8_t * dst_save;

  uint8_t * src_ptr;
  uint8_t * src_save;

  uint32_t * palette;

  int dst_stride;
    
  palette = (uint32_t*)(src->data[1]);
  
  if(flip_y)
    {
    dst_save = dst->planes[0] + (height - 1) * dst->strides[0];
    dst_stride = - dst->strides[0];
    }
  else
    {
    dst_save = dst->planes[0];
    dst_stride = dst->strides[0];
    }
  
  src_save = src->data[0];
  for(i = 0; i < height; i++)
    {
    src_ptr = src_save;
    dst_ptr = dst_save;
    
    for(j = 0; j < width; j++)
      {
      pixel = palette[*src_ptr];
      dst_ptr[0] = (pixel & 0x00FF0000) >> 16;
      dst_ptr[1] = (pixel & 0x0000FF00) >> 8;
      dst_ptr[2] = (pixel & 0x000000FF);
      dst_ptr[3] = (pixel & 0xFF000000) >> 24;
      
      dst_ptr+=4;
      src_ptr++;
      }

    src_save += src->linesize[0];
    dst_save += dst_stride;
    }
  }

#if LIBAVCODEC_BUILD >= ((51<<16)+(45<<8)+0)

static void yuva420_to_yuva32(gavl_video_frame_t * dst, AVFrame * src,
                              int width, int height, int flip_y)
  {
  int i, j;
  uint8_t * dst_ptr;
  uint8_t * dst_save;

  uint8_t * src_ptr_y;
  uint8_t * src_save_y;

  uint8_t * src_ptr_u;
  uint8_t * src_save_u;

  uint8_t * src_ptr_v;
  uint8_t * src_save_v;

  uint8_t * src_ptr_a;
  uint8_t * src_save_a;

  int dst_stride;
  
  if(flip_y)
    {
    dst_save = dst->planes[0] + (height - 1) * dst->strides[0];
    dst_stride = - dst->strides[0];
    }
  else
    {
    dst_save = dst->planes[0];
    dst_stride = dst->strides[0];
    }
  
  src_save_y = src->data[0];
  src_save_u = src->data[1];
  src_save_v = src->data[2];
  src_save_a = src->data[3];

  for(i = 0; i < height/2; i++)
    {
    src_ptr_y = src_save_y;
    src_ptr_u = src_save_u;
    src_ptr_v = src_save_v;
    src_ptr_a = src_save_a;
    dst_ptr = dst_save;
    
    for(j = 0; j < width/2; j++)
      {
      dst_ptr[0] = *src_ptr_y;
      dst_ptr[1] = *src_ptr_u;
      dst_ptr[2] = *src_ptr_v;
      dst_ptr[3] = *src_ptr_a;
      
      dst_ptr+=4;
      src_ptr_y++;
      src_ptr_a++;

      dst_ptr[0] = *src_ptr_y;
      dst_ptr[1] = *src_ptr_u;
      dst_ptr[2] = *src_ptr_v;
      dst_ptr[3] = *src_ptr_a;
      
      dst_ptr+=4;
      src_ptr_y++;
      src_ptr_a++;

      src_ptr_u++;
      src_ptr_v++;
      
      }

    src_save_y += src->linesize[0];
    dst_save += dst_stride;

    src_ptr_y = src_save_y;
    src_ptr_u = src_save_u;
    src_ptr_v = src_save_v;
    src_ptr_a = src_save_a;
    dst_ptr = dst_save;
    
    for(j = 0; j < width/2; j++)
      {
      dst_ptr[0] = *src_ptr_y;
      dst_ptr[1] = *src_ptr_u;
      dst_ptr[2] = *src_ptr_v;
      dst_ptr[3] = *src_ptr_a;
      
      dst_ptr+=4;
      src_ptr_y++;
      src_ptr_a++;

      dst_ptr[0] = *src_ptr_y;
      dst_ptr[1] = *src_ptr_u;
      dst_ptr[2] = *src_ptr_v;
      dst_ptr[3] = *src_ptr_a;
      
      dst_ptr+=4;
      src_ptr_y++;
      src_ptr_a++;

      src_ptr_u++;
      src_ptr_v++;
      
      }
    
    src_save_y += src->linesize[0];
    src_save_u += src->linesize[1];
    src_save_v += src->linesize[2];
    src_save_a += src->linesize[3];
    
    dst_save += dst_stride;
    }
  }
#endif


/* Real stupid rgba format conversion */

static void rgba32_to_rgba32(gavl_video_frame_t * dst, AVFrame * src,
                             int width, int height, int flip_y)
  {
  int i, j;
  uint32_t r, g, b, a;
  uint8_t * dst_ptr;
  uint8_t * dst_save;
  int dst_stride;
  
  uint32_t * src_ptr;
  uint8_t  * src_save;

  if(flip_y)
    {
    dst_save = dst->planes[0] + (height - 1) * dst->strides[0];
    dst_stride = - dst->strides[0];
    }
  else
    {
    dst_save = dst->planes[0];
    dst_stride = dst->strides[0];
    }
    
  src_save = src->data[0];
  for(i = 0; i < height; i++)
    {
    src_ptr = (uint32_t*)src_save;
    dst_ptr = dst_save;
    
    for(j = 0; j < width; j++)
      {
      a = ((*src_ptr) & 0xff000000) >> 24;
      r = ((*src_ptr) & 0x00ff0000) >> 16;
      g = ((*src_ptr) & 0x0000ff00) >> 8;
      b = ((*src_ptr) & 0x000000ff);
      dst_ptr[0] = r;
      dst_ptr[1] = g;
      dst_ptr[2] = b;
      dst_ptr[3] = a;
      
      dst_ptr+=4;
      src_ptr++;
      }
    src_save += src->linesize[0];
    dst_save += dst_stride;
    }
  }

/* Pixel formats */

static const struct
  {
  enum PixelFormat  ffmpeg_csp;
  gavl_pixelformat_t gavl_csp;
  } pixelformats[] =
  {
    { PIX_FMT_YUV420P,       GAVL_YUV_420_P },  ///< Planar YUV 4:2:0 (1 Cr & Cb sample per 2x2 Y samples)
#if LIBAVUTIL_VERSION_INT < (50<<16)
    { PIX_FMT_YUV422,        GAVL_YUY2      },
#else
    { PIX_FMT_YUYV422,       GAVL_YUY2      },
#endif
    { PIX_FMT_RGB24,         GAVL_RGB_24    },  ///< Packed pixel, 3 bytes per pixel, RGBRGB...
    { PIX_FMT_BGR24,         GAVL_BGR_24    },  ///< Packed pixel, 3 bytes per pixel, BGRBGR...
    { PIX_FMT_YUV422P,       GAVL_YUV_422_P },  ///< Planar YUV 4:2:2 (1 Cr & Cb sample per 2x1 Y samples)
    { PIX_FMT_YUV444P,       GAVL_YUV_444_P }, ///< Planar YUV 4:4:4 (1 Cr & Cb sample per 1x1 Y samples)
#if LIBAVUTIL_VERSION_INT < (50<<16)
    { PIX_FMT_RGBA32,        GAVL_RGBA_32   },  ///< Packed pixel, 4 bytes per pixel, BGRABGRA..., stored in cpu endianness
#else
    { PIX_FMT_RGB32,         GAVL_RGBA_32   },  ///< Packed pixel, 4 bytes per pixel, BGRABGRA..., stored in cpu endianness
#endif
    { PIX_FMT_YUV410P,       GAVL_YUV_410_P }, ///< Planar YUV 4:1:0 (1 Cr & Cb sample per 4x4 Y samples)
    { PIX_FMT_YUV411P,       GAVL_YUV_411_P }, ///< Planar YUV 4:1:1 (1 Cr & Cb sample per 4x1 Y samples)
    { PIX_FMT_RGB565,        GAVL_RGB_16 }, ///< always stored in cpu endianness
    { PIX_FMT_RGB555,        GAVL_RGB_15 }, ///< always stored in cpu endianness, most significant bit to 1
    { PIX_FMT_GRAY8,         GAVL_PIXELFORMAT_NONE },
    { PIX_FMT_MONOWHITE,     GAVL_PIXELFORMAT_NONE }, ///< 0 is white
    { PIX_FMT_MONOBLACK,     GAVL_PIXELFORMAT_NONE }, ///< 0 is black
    // { PIX_FMT_PAL8,          GAVL_RGB_24     }, ///< 8 bit with RGBA palette
    { PIX_FMT_YUVJ420P,      GAVL_YUVJ_420_P }, ///< Planar YUV 4:2:0 full scale (jpeg)
    { PIX_FMT_YUVJ422P,      GAVL_YUVJ_422_P }, ///< Planar YUV 4:2:2 full scale (jpeg)
    { PIX_FMT_YUVJ444P,      GAVL_YUVJ_444_P }, ///< Planar YUV 4:4:4 full scale (jpeg)
    { PIX_FMT_XVMC_MPEG2_MC, GAVL_PIXELFORMAT_NONE }, ///< XVideo Motion Acceleration via common packet passing(xvmc_render.h)
    { PIX_FMT_XVMC_MPEG2_IDCT, GAVL_PIXELFORMAT_NONE },
#if LIBAVCODEC_BUILD >= ((51<<16)+(45<<8)+0)
    { PIX_FMT_YUVA420P,      GAVL_YUVA_32 },
#endif
    { PIX_FMT_NB, GAVL_PIXELFORMAT_NONE }
};


static gavl_pixelformat_t get_pixelformat(enum PixelFormat p,
                                          gavl_pixelformat_t pixelformat)
  {
  int i;
  if(p == PIX_FMT_PAL8)
    {
    if(pixelformat == GAVL_RGBA_32)
      return GAVL_RGBA_32;
    else
      return GAVL_RGB_24;
    }
  
  for(i = 0; i < sizeof(pixelformats)/sizeof(pixelformats[0]); i++)
    {
    if(pixelformats[i].ffmpeg_csp == p)
      return pixelformats[i].gavl_csp;
    }
  return GAVL_PIXELFORMAT_NONE;
  }


/* Static functions (moved here, to make the above mess more readable) */
static void get_format(AVCodecContext * ctx, gavl_video_format_t * format)
  {
  format->pixelformat =
    get_pixelformat(ctx->pix_fmt, format->pixelformat);
  
  if(ctx->codec_id == CODEC_ID_DVVIDEO)
    {
    if(format->pixelformat == GAVL_YUV_420_P)
      format->chroma_placement = GAVL_CHROMA_PLACEMENT_DVPAL;

    if(!format->interlace_mode)
      format->interlace_mode = GAVL_INTERLACE_BOTTOM_FIRST;

    /* We completely ignore the frame size of the container */
    format->image_width = ctx->width;
    format->frame_width = ctx->width;
        
    format->image_height = ctx->height;
    format->frame_height = ctx->height;
    }
  else
    {
    if((ctx->sample_aspect_ratio.num > 1) ||
       (ctx->sample_aspect_ratio.den > 1))
      {
      format->pixel_width  = ctx->sample_aspect_ratio.num;
      format->pixel_height = ctx->sample_aspect_ratio.den;
      }
    /* Some demuxers don't know the frame dimensions */
    if(!format->image_width)
      {
      format->image_width = ctx->width;
      format->frame_width = ctx->width;

      format->image_height = ctx->height;
      format->frame_height = ctx->height;
      }
    
    if((format->pixel_width <= 0) || (format->pixel_height <= 0))
      {
      format->pixel_width  = 1;
      format->pixel_height = 1;
      }
    
    /* Sometimes, the size encoded in some mp4 (vol?) headers is different from
       what is found in the container. In this case, the image must be scaled. */
    else if(format->image_width &&
            (ctx->width < format->image_width))
      {
      format->pixel_width  = format->image_width;
      format->pixel_height = ctx->width;
      format->image_width = ctx->width;
      }
    if(((ctx->codec_id == CODEC_ID_MPEG4) ||
        (ctx->codec_id == CODEC_ID_H264)) &&
       (format->pixelformat == GAVL_YUV_420_P))
      format->chroma_placement = GAVL_CHROMA_PLACEMENT_MPEG2;
    }
  
  if(!format->timescale)
    {
    format->timescale = ctx->time_base.den;
    format->frame_duration = ctx->time_base.num;
    }
  }

#ifdef HAVE_LIBPOSTPROC
static void init_pp(bgav_stream_t * s)
  {
  int accel_flags;
  int pp_flags;
  ffmpeg_video_priv * priv;
  priv = s->data.video.decoder->priv;

  
  /* Initialize postprocessing */
  if(s->opt->pp_level > 0)
    {
    switch(priv->info->ffmpeg_id)
      {
      case CODEC_ID_MPEG4:
      case CODEC_ID_MSMPEG4V1:
      case CODEC_ID_MSMPEG4V2:
      case CODEC_ID_MSMPEG4V3:
      case CODEC_ID_WMV1:
      case CODEC_ID_WMV2:
      case CODEC_ID_WMV3:
        priv->do_pp = 1;
        accel_flags = gavl_accel_supported();

        if(s->data.video.format.pixelformat != GAVL_YUV_420_P)
          {
          bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                   "Unsupported pixelformat for postprocessing");
          priv->do_pp = 0;
          break;
          }
        pp_flags = PP_FORMAT_420;
            
        if(accel_flags & GAVL_ACCEL_MMX)
          pp_flags |= PP_CPU_CAPS_MMX;
        if(accel_flags & GAVL_ACCEL_MMXEXT)
          pp_flags |= PP_CPU_CAPS_MMX2;
        if(accel_flags & GAVL_ACCEL_3DNOW)
          pp_flags |= PP_CPU_CAPS_3DNOW;

        priv->pp_context =
          pp_get_context(priv->ctx->width, priv->ctx->height,
                         pp_flags);
        priv->pp_mode = pp_get_mode_by_name_and_quality("hb:a,vb:a,dr:a",
                                                        s->opt->pp_level);

        if(s->data.video.flip_y)
          priv->flip_frame = gavl_video_frame_create(&s->data.video.format);
            
        break;
      default:
        priv->do_pp = 0;
        break;
      }
        
    }
  }
#endif

/* Copy/postprocess/flip internal frame to output */
static void put_frame(bgav_stream_t * s, gavl_video_frame_t * f)
  {
#ifndef HAVE_LIBSWSCALE
  AVPicture ffmpeg_frame;
#endif
  
  ffmpeg_video_priv * priv;
  priv = s->data.video.decoder->priv;
  if(priv->ctx->pix_fmt == PIX_FMT_PAL8)
    {
    if(s->data.video.format.pixelformat == GAVL_RGBA_32)
      pal8_to_rgba32(f, priv->frame,
                     s->data.video.format.image_width,
                     s->data.video.format.image_height, s->data.video.flip_y);
    else
      pal8_to_rgb24(f, priv->frame,
                    s->data.video.format.image_width,
                    s->data.video.format.image_height, s->data.video.flip_y);
    }
#if LIBAVUTIL_VERSION_INT < (50<<16)
  else if(priv->ctx->pix_fmt == PIX_FMT_RGBA32)
#else
  else if(priv->ctx->pix_fmt == PIX_FMT_RGB32)
#endif
    {
    rgba32_to_rgba32(f, priv->frame,
                     s->data.video.format.image_width,
                     s->data.video.format.image_height, s->data.video.flip_y);
    }
#if LIBAVCODEC_BUILD >= ((51<<16)+(45<<8)+0)
  else if(priv->ctx->pix_fmt == PIX_FMT_YUVA420P)
    {
    yuva420_to_yuva32(f, priv->frame,
                      s->data.video.format.image_width,
                      s->data.video.format.image_height, s->data.video.flip_y);
    }
#endif
  else if(!priv->do_convert)
    {
#ifdef HAVE_LIBPOSTPROC
    if(priv->do_pp)
      {
      if(s->data.video.flip_y)
        {
        pp_postprocess((const uint8_t**)priv->frame->data, priv->frame->linesize,
                       priv->flip_frame->planes, priv->flip_frame->strides,
                       priv->ctx->width, priv->ctx->height,
                       priv->frame->qscale_table, priv->frame->qstride,
                       priv->pp_mode, priv->pp_context,
                       priv->frame->pict_type);
        gavl_video_frame_copy_flip_y(&(s->data.video.format), f, priv->flip_frame);
        }
      else
        pp_postprocess((const uint8_t**)priv->frame->data, priv->frame->linesize,
                       f->planes, f->strides,
                       priv->ctx->width, priv->ctx->height,
                       priv->frame->qscale_table, priv->frame->qstride,
                       priv->pp_mode, priv->pp_context,
                       priv->frame->pict_type);
            
      }
    else
      {
#endif
      priv->gavl_frame->planes[0]  = priv->frame->data[0];
      priv->gavl_frame->planes[1]  = priv->frame->data[1];
      priv->gavl_frame->planes[2]  = priv->frame->data[2];
          
      priv->gavl_frame->strides[0] = priv->frame->linesize[0];
      priv->gavl_frame->strides[1] = priv->frame->linesize[1];
      priv->gavl_frame->strides[2] = priv->frame->linesize[2];

      if(s->data.video.flip_y)
        gavl_video_frame_copy_flip_y(&(s->data.video.format), f, priv->gavl_frame);
      else if(priv->swap_fields)
        {
        /* src field (top) -> dst field (bottom) */
        gavl_video_frame_get_field(s->data.video.format.pixelformat,
                                   priv->gavl_frame,
                                   priv->src_field,
                                   0);

        gavl_video_frame_get_field(s->data.video.format.pixelformat,
                                   f,
                                   priv->dst_field,
                                   1);
        
        gavl_video_frame_copy(&priv->field_format, priv->dst_field, priv->src_field);

        /* src field (bottom) -> dst field (top) */
        gavl_video_frame_get_field(s->data.video.format.pixelformat,
                                   priv->gavl_frame,
                                   priv->src_field,
                                   1);

        gavl_video_frame_get_field(s->data.video.format.pixelformat,
                                   f,
                                   priv->dst_field,
                                   0);
        gavl_video_frame_copy(&priv->field_format, priv->dst_field, priv->src_field);
        }
      else
        gavl_video_frame_copy(&(s->data.video.format), f, priv->gavl_frame);
#ifdef HAVE_LIBPOSTPROC
      }
#endif
    }
  else
    {
    // TODO: Enable postprocessing for non-gavl pixelformats
    // (but not as long as it makes no sense)
#ifdef HAVE_LIBSWSCALE
    sws_scale(priv->swsContext,
              priv->frame->data, priv->frame->linesize,
              0, s->data.video.format.image_height,
              f->planes, f->strides);

#else
    ffmpeg_frame.data[0]     = f->planes[0];
    ffmpeg_frame.data[1]     = f->planes[1];
    ffmpeg_frame.data[2]     = f->planes[2];
    ffmpeg_frame.linesize[0] = f->strides[0];
    ffmpeg_frame.linesize[1] = f->strides[1];
    ffmpeg_frame.linesize[2] = f->strides[2];
    img_convert(&ffmpeg_frame, priv->dst_format,
                (AVPicture*)(priv->frame), priv->ctx->pix_fmt,
                s->data.video.format.image_width,
                s->data.video.format.image_height);
#endif
    }
  f->timestamp = priv->picture_timestamp;
  f->duration = priv->picture_duration;
  
  }

/* Extract format and get timecode */

/* We get the DV format info ourselfes, since the values
   ffmpeg returns are not reliable */

static void handle_dv(bgav_stream_t * s)
  {
  ffmpeg_video_priv * priv = s->data.video.decoder->priv;
  
  if(priv->need_format)
    {
    priv->dvdec = bgav_dv_dec_create();
    bgav_dv_dec_set_header(priv->dvdec, priv->frame_buffer);
    }
      
  bgav_dv_dec_set_frame(priv->dvdec, priv->frame_buffer);
  
  if(priv->need_format)
    {
    bgav_dv_dec_get_pixel_aspect(priv->dvdec,
                                 &s->data.video.format.pixel_width,
                                 &s->data.video.format.pixel_height);
        
    if(!s->data.video.format.timecode_format.int_framerate)
      {
      bgav_dv_dec_get_timecode_format(priv->dvdec,
                                      &s->data.video.format.timecode_format, s->opt);
      }
    }
  /* Extract timecodes */
  if(s->opt->dv_datetime)
    {
    int year, month, day;
    int hour, minute, second;

    gavl_timecode_t tc = 0;
        
    if(bgav_dv_dec_get_date(priv->dvdec, &year, &month, &day) &&
       bgav_dv_dec_get_time(priv->dvdec, &hour, &minute, &second))
      {
      gavl_timecode_from_ymd(&tc,
                             year, month, day);
      gavl_timecode_from_hmsf(&tc,
                              hour, minute, second, 0);
          
      if((priv->last_dv_timecode != GAVL_TIMECODE_UNDEFINED) &&
         (tc != priv->last_dv_timecode))
        {
        if(s->data.video.format.timecode_format.flags & GAVL_TIMECODE_DROP_FRAME)
          {
          if(!second && (minute % 10))
            gavl_timecode_from_hmsf(&tc,
                                    hour, minute, second, 2);
          }
        s->codec_timecode = tc;
        s->has_codec_timecode = 1;
        }
      priv->last_dv_timecode = tc;
      }
    }
  else
    {
    if(bgav_dv_dec_get_timecode(priv->dvdec,
                                &s->codec_timecode))
      s->has_codec_timecode = 1;
    }
  
  }
