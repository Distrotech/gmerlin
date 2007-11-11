/*****************************************************************
 
  video_ffmpeg.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ffmpeg/avcodec.h>

#include <config.h>
#include <bswap.h>
#include <avdec_private.h>
#include <codecs.h>

#include <stdio.h>

#include <dvframe.h>

#ifdef HAVE_LIBPOSTPROC
#include <postprocess.h>
#endif

#ifdef HAVE_LIBSWSCALE
#include <swscale.h>
#endif

#define LOG_DOMAIN "ffmpeg_video"

// #define DUMP_DECODE
// #define DUMP_EXTRADATA
// #define DUMP_PARSER
/* Map of ffmpeg codecs to fourccs (from ffmpeg's avienc.c) */

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

  /* For buffering and parsing */
  
  bgav_bytebuffer_t buf;

  /* Updated by get_data() */
  
  uint8_t * parsed_frame;
  int parsed_frame_size;
  int parsed_bytes_used;

  /* Pixelformat */
  int do_convert;
  enum PixelFormat dst_format;

  /* Real video ugliness */
  uint32_t rv_extradata[2+FF_INPUT_BUFFER_PADDING_SIZE/4];
  AVPaletteControl palette;

  /* State variables */
  int need_format;
  int have_picture;

  uint8_t * extradata;
  int extradata_size;
  
  int need_extradata; /* Get extradata from parser */
      
  int64_t last_pts;
  int64_t last_dts;
  int eof;

  
  
  struct
    {
    int64_t pts[FF_MAX_B_FRAMES+1];
    int num;
    } pts_cache;
  
  int delay, has_delay;

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
  AVCodecParserContext * parser;
  int parser_started;
  
  int64_t frame_pts;
  } ffmpeg_video_priv;

static codec_info_t * lookup_codec(bgav_stream_t * s);

/* This MUST match demux_rm.c!! */

typedef struct dp_hdr_s {
    uint32_t chunks;    // number of chunks
    uint32_t timestamp; // timestamp from packet header
    uint32_t len;       // length of actual data
    uint32_t chunktab;  // offset to chunk offset array
} dp_hdr_t;

// static int data_dumped = 0;

#define TIME_BGAV_2_FFMPEG(t) ((t == BGAV_TIMESTAMP_UNDEFINED) ? AV_NOPTS_VALUE : t)
#define TIME_FFMPEG_2_BGAV(t) ((t == AV_NOPTS_VALUE) ? BGAV_TIMESTAMP_UNDEFINED : t)

static int64_t get_pts(ffmpeg_video_priv * priv)
  {
  int64_t ret;
  if(!priv->pts_cache.num)
    return BGAV_TIMESTAMP_UNDEFINED;

  ret = priv->pts_cache.pts[0];
  
  priv->pts_cache.num--;
  if(priv->pts_cache.num)
    memmove(&(priv->pts_cache.pts[0]), &(priv->pts_cache.pts[1]),
            sizeof(int64_t) * priv->pts_cache.num);
  return ret;
  }

static int put_pts(ffmpeg_video_priv * priv, int64_t pts)
  {
  if(priv->pts_cache.num >= sizeof(priv->pts_cache.pts)/sizeof(priv->pts_cache.pts[0]))
    get_pts(priv);
  
  priv->pts_cache.pts[priv->pts_cache.num] = pts;
  priv->pts_cache.num++;
  return 1;
  }


static int get_data(bgav_stream_t * s)
  {
  int ext_len;
  int bytes_to_parse;
  int done = 0;
  ffmpeg_video_priv * priv;
  bgav_packet_t * p = (bgav_packet_t*)0;
  
  priv = (ffmpeg_video_priv*)(s->data.video.decoder->priv);

  if(priv->eof)
    {
    priv->parsed_frame = (uint8_t*)0;
    priv->parsed_frame_size = 0;
    return 1;
    }
  
  while(!done)
    {
    /* Try to parse the stream */
    if(priv->parser && priv->parser_started)
      {
#ifdef DUMP_PARSER
      bgav_dprintf("Parsing %d bytes ", priv->buf.size);
      if(priv->last_pts != BGAV_TIMESTAMP_UNDEFINED)
        bgav_dprintf("PTS: %" PRId64 " ", priv->last_pts);
      if(priv->last_dts != BGAV_TIMESTAMP_UNDEFINED)
        bgav_dprintf("DTS: %" PRId64 " ", priv->last_dts);
#endif

      if(priv->eof)
        bytes_to_parse = 0;
      else
        bytes_to_parse = priv->buf.size - priv->parsed_bytes_used;
      
      priv->parsed_bytes_used +=
        av_parser_parse(priv->parser,
                        priv->ctx,
                        &priv->parsed_frame,
                        &priv->parsed_frame_size,
                        priv->buf.buffer + priv->parsed_bytes_used,
                        bytes_to_parse,
                        TIME_BGAV_2_FFMPEG(priv->last_pts),
                        TIME_BGAV_2_FFMPEG(priv->last_dts));
#ifdef DUMP_PARSER
      bgav_dprintf("done: bytes_used: %d frame_size: %d ",
                   priv->parsed_bytes_used, priv->parsed_frame_size);

      if(priv->parser->pts != AV_NOPTS_VALUE)
        bgav_dprintf("PTS: %" PRId64 " ", priv->parser->pts);
      if(priv->parser->dts != AV_NOPTS_VALUE)
        bgav_dprintf("DTS: %" PRId64 " ", priv->parser->dts);
      bgav_dprintf("\n");
#endif
      if(priv->parsed_frame_size)
        {
        if(!put_pts(priv, priv->parser->pts))
          return 0;
        done = 1;
        break;
        }
      else if(priv->eof) /* Sometimes, we don't get the last frame */
        {
        done = 1;
        break;
        }
      }

    /* Get packet from demuxer */
    p = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!p)
      {
      if(priv->parser)
        {
        priv->eof = 1;
        /* Parse once more to get the last frame */
        continue;
        }
      else
        {
        priv->parsed_frame = (uint8_t*)0;
        priv->parsed_frame_size = 0;
        break;
        }
      }
    
    priv->last_pts = p->pts;
    priv->last_dts = p->dts;
    bgav_bytebuffer_append(&priv->buf, p, FF_INPUT_BUFFER_PADDING_SIZE);
    if(!priv->parser)
      {
      priv->parsed_frame = priv->buf.buffer;
      priv->parsed_frame_size = priv->buf.size;
#ifdef DUMP_PARSER
      bgav_dprintf("Got complete packet %d bytes ", priv->buf.size);
#endif      
      if(!put_pts(priv, p->pts))
        return 0;
      done = 1;
      }
    else
      {
      /* Extract extradata */
      if(priv->need_extradata)
        {
        ext_len = priv->parser->parser->split(priv->ctx, priv->buf.buffer,
                                              priv->buf.size);
        if(ext_len)
          {
#ifdef DUMP_PARSER
          bgav_dprintf("Got extradata from parser %d bytes\n", ext_len);
          bgav_hexdump(priv->buf.buffer, ext_len, 16);
#endif      
          priv->extradata = calloc(1, ext_len + FF_INPUT_BUFFER_PADDING_SIZE);
          memcpy(priv->extradata, priv->buf.buffer, ext_len);
          priv->extradata_size = ext_len;
          priv->need_extradata = 0;
          
          // ffplay doesn't remove these either
          // bgav_bytebuffer_remove(&priv->buf, ext_len);
          done = 1;
          }
        }
          
      priv->parser_started = 1;
      }
    bgav_demuxer_done_packet_read(s->demuxer, p);
    }
  return 1;
  }

static void get_format(AVCodecContext * ctx, gavl_video_format_t * format);
static void put_frame(bgav_stream_t * s, gavl_video_frame_t * f);

#ifdef HAVE_LIBPOSTPROC
static void init_pp(bgav_stream_t * s);
#endif


static int decode(bgav_stream_t * s, gavl_video_frame_t * f)
  {
  int i, imax;
  int bytes_used;
  int len;
  dp_hdr_t *hdr;
  ffmpeg_video_priv * priv;
  /* We get the DV format info ourselfes, since the values
     ffmpeg returns are not reliable */
  bgav_dv_dec_t * dvdec;
  int done = 0;
  
  priv = (ffmpeg_video_priv*)(s->data.video.decoder->priv);
  
  if(priv->have_picture)
    {
    done = 1;
    }
  while(!done)
    {
    /* Read data if necessary */
    if(!get_data(s))
      {
      return 0;
      }
    len = priv->parsed_frame_size;
    
    /* If we are out of data and the codec has no B-Frames, we are done */
    if(!priv->delay && !priv->parsed_frame)
      {
      done = 1;
      break;
      }
    /* Other Real Video oddities */
    if((s->fourcc == BGAV_MK_FOURCC('R', 'V', '1', '0')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '1', '3')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '2', '0')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '3', '0')) ||
       (s->fourcc == BGAV_MK_FOURCC('R', 'V', '4', '0')))
      {
      if(priv->ctx->extradata_size == 8)
        {
        hdr= (dp_hdr_t*)(priv->buf.buffer);
        if(priv->ctx->slice_offset==NULL)
          priv->ctx->slice_offset= malloc(sizeof(int)*1000);
        priv->ctx->slice_count= hdr->chunks+1;
        for(i=0; i<priv->ctx->slice_count; i++)
          priv->ctx->slice_offset[i]=
            ((uint32_t*)(priv->buf.buffer+hdr->chunktab))[2*i+1];
        len=hdr->len;
        bgav_bytebuffer_remove(&priv->buf, sizeof(dp_hdr_t));
        }
      }

    /* Set skip flags */
    if(!f && !priv->need_format)
      priv->ctx->hurry_up = 1;
    else
      priv->ctx->hurry_up = 0;

    /* DV Video ugliness */
    if(priv->need_format && (priv->info->ffmpeg_id == CODEC_ID_DVVIDEO))
      {
      dvdec = bgav_dv_dec_create();
      bgav_dv_dec_set_header(dvdec, priv->buf.buffer);
      bgav_dv_dec_set_frame(dvdec, priv->buf.buffer);

      bgav_dv_dec_get_pixel_aspect(dvdec, &s->data.video.format.pixel_width,
                                   &s->data.video.format.pixel_height);
      
      bgav_dv_dec_destroy(dvdec);
      }

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
    bgav_dprintf("Decode: position: %" PRId64 " len: %d\n", s->out_position, len);
    if(priv->parsed_frame)
      bgav_hexdump(priv->parsed_frame, 16, 16);
#endif
    
    if(priv->has_delay && len)
      priv->delay++;
    
    bytes_used = avcodec_decode_video(priv->ctx,
                                      priv->frame,
                                      &priv->have_picture,
                                      priv->parsed_frame,
                                      len);

    /* If we passed no data and got no picture, we are done here */
    if(!len && !priv->have_picture)
      return 0;
    
    /* Update decoding delay */
    if(priv->has_delay && priv->have_picture)
      priv->delay--;
    
#ifdef DUMP_DECODE
    bgav_dprintf("Used %d/%d bytes, got picture: %d, delay: %d, num_old_pts: %d ",
                 bytes_used, len, priv->have_picture, priv->delay, priv->pts_cache.num);
    if(!priv->have_picture)
      bgav_dprintf("\n");
    else if(priv->frame->pict_type == FF_I_TYPE)
      bgav_dprintf("I-Frame\n");
    else if(priv->frame->pict_type == FF_B_TYPE)
      bgav_dprintf("B-Frame\n");
    else if(priv->frame->pict_type == FF_P_TYPE)
      bgav_dprintf("P-Frame\n");
    else if(priv->frame->pict_type == FF_S_TYPE)
      bgav_dprintf("S-Frame\n");
    else if(priv->frame->pict_type == FF_SI_TYPE)
      bgav_dprintf("SI-Frame\n");
    else if(priv->frame->pict_type == FF_SP_TYPE)
      bgav_dprintf("SP-Frame\n");
#endif
    
    /* Advance packet buffer */
    
    if(priv->parsed_frame)
      {
      /* Check for error */
      if(bytes_used < 0)
        {
        if(priv->parser)
          {
          bgav_bytebuffer_remove(&priv->buf, priv->parsed_bytes_used);
          priv->parsed_bytes_used = 0;
          }
        else
          bgav_bytebuffer_flush(&priv->buf);
        get_pts(priv);
        }
      else
        {
        if(priv->parser)
          {
          bgav_bytebuffer_remove(&priv->buf, priv->parsed_bytes_used);
          priv->parsed_bytes_used = 0;
          }
        else if(!s->not_aligned)
          {
          bgav_bytebuffer_flush(&priv->buf);
          }
        else
          bgav_bytebuffer_remove(&priv->buf, bytes_used);
        
        if(priv->need_format)
          {
          done = 1;
          return 1;
          }
        }
      }
    if(priv->have_picture)
      done = 1;
    }
  
  if(priv->have_picture)
    {
    if(f)
      put_frame(s, f);

    /* Throw away the picture even if we skip a frame */
    priv->have_picture = 0;
    }
  else /* !got_picture */
    {
    if(!priv->parsed_frame)
      return 0; /* EOF */
    }

  /*
   * Update timestamp
   */

  if(s->vfr_timestamps && (priv->pts_cache.num > 1))
    s->data.video.last_frame_duration =
      gavl_time_rescale(s->timescale, s->data.video.format.timescale,
                        priv->pts_cache.pts[1] - priv->pts_cache.pts[0]);
  else
    s->data.video.last_frame_duration = s->data.video.format.frame_duration;
  
  s->time_scaled = get_pts(priv);
  
  
  if(s->time_scaled != BGAV_TIMESTAMP_UNDEFINED)
    {
    priv->frame_pts = gavl_time_rescale(s->timescale, s->data.video.format.timescale,
                                        s->time_scaled);
    }
  else
    {
    priv->frame_pts += s->data.video.format.frame_duration;
    }
  s->data.video.last_frame_time = priv->frame_pts;
  
  return 1;
  }

static int init(bgav_stream_t * s)
  {
  AVCodec * codec;
  
  ffmpeg_video_priv * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;

  /* Set up coded specific details */
  
  if(s->fourcc == BGAV_MK_FOURCC('W','V','1','F'))
    s->data.video.flip_y = 1;
  
  priv->info = lookup_codec(s);
  codec = avcodec_find_decoder(priv->info->ffmpeg_id);
  priv->ctx = avcodec_alloc_context();
  priv->ctx->width = s->data.video.format.frame_width;
  priv->ctx->height = s->data.video.format.frame_height;
  priv->ctx->bits_per_sample = s->data.video.depth;
#if 1
  priv->ctx->codec_tag   =
    ((s->fourcc & 0x000000ff) << 24) |
    ((s->fourcc & 0x0000ff00) << 8) |
    ((s->fourcc & 0x00ff0000) >> 8) |
    ((s->fourcc & 0xff000000) >> 24);
#endif
  priv->ctx->codec_id = codec->id;
  
  /* Initialize parser */
  if(s->not_aligned)
    {
    priv->parser = av_parser_init(priv->ctx->codec_id);
    //    if(priv->parser)
    //      priv->parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
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
  else if(priv->parser && priv->parser->parser->split)
    {
    priv->need_extradata = 1;
    while(priv->need_extradata)
      {
      if(!get_data(s))
        {
        bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Ran out of data during extradata scanning");
        return 0;
        }
      }
    priv->ctx->extradata      = priv->extradata;
    priv->ctx->extradata_size = priv->extradata_size;
    }
  
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

  /*
   *  We always assume, that we have complete frames only.
   *  For streams, where the packets are not aligned with frames,
   *  we need an AVParser
   */
  priv->ctx->flags &= ~CODEC_FLAG_TRUNCATED;
  
  if(codec->capabilities & CODEC_CAP_DELAY)
    priv->has_delay = 1;
  
  priv->ctx->workaround_bugs = FF_BUG_AUTODETECT;
  priv->ctx->error_concealment = 3;
  
  //  priv->ctx->error_resilience = 3;
  
  /* Open this thing */
  
  if(avcodec_open(priv->ctx, codec) != 0)
    return 0;
  
  /* Set missing format values */

  
  priv->need_format = 1;

  if(!decode(s, NULL))
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
  ffmpeg_video_priv * priv;
  priv = (ffmpeg_video_priv*)(s->data.video.decoder->priv);
  avcodec_flush_buffers(priv->ctx);
  bgav_bytebuffer_flush(&priv->buf);
  priv->delay = 0;
  priv->pts_cache.num = 0;
  priv->have_picture = 0;
  
  if(priv->parser)
    {
    av_parser_close(priv->parser);
    priv->parser = av_parser_init(priv->ctx->codec_id);
    priv->last_pts = BGAV_TIMESTAMP_UNDEFINED;
    priv->last_dts = BGAV_TIMESTAMP_UNDEFINED;
    priv->parser_started = 0;
    }
  
  }

static void close_ffmpeg(bgav_stream_t * s)
  {
  ffmpeg_video_priv * priv;
  priv= (ffmpeg_video_priv*)(s->data.video.decoder->priv);

  if(!priv)
    return;
  
  bgav_bytebuffer_free(&priv->buf);
  
  if(priv->parser)
    av_parser_close(priv->parser);
  
  
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
               BGAV_MK_FOURCC('d', 'v', 'c', ' '), 
               BGAV_MK_FOURCC('d', 'v', 'c', 'p'), 
               BGAV_MK_FOURCC('d', 'v', 'p', 'p'), 
               BGAV_MK_FOURCC('d', 'v', 's', 'l'), 
               BGAV_MK_FOURCC('d', 'v', '2', '5'),
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
#ifndef HAVE_W32DLL
    
    // [wmv3 @ 0xb63fe128]Old WMV3 version detected, only I-frames will be decoded
    { "FFmpeg WMV3 decoder", "Window Media Video 9", CODEC_ID_WMV3,
      (uint32_t[]){ BGAV_MK_FOURCC('W', 'M', 'V', '3'),
               0x00 } }, 
#endif

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
#if LIBAVCODEC_BUILD >= 3345152
    { "FFmpeg Chinese AVS decoder", "Chinese AVS", CODEC_ID_CAVS,
      (uint32_t[]){ BGAV_MK_FOURCC('C', 'A', 'V', 'S'),
                    0x00 } },
#endif
    /*     CODEC_ID_JPEG2000, */

    /*     CODEC_ID_VMNC, */
#if LIBAVCODEC_BUILD >= 3345664
    { "FFmpeg VMware video decoder", "VMware video", CODEC_ID_VMNC,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'M', 'n', 'c'),
                    0x00 } },
#endif      
    /*     CODEC_ID_VP5, */
#if LIBAVCODEC_BUILD >= 3345920    
    { "FFmpeg VP5 decoder", "On2 VP5", CODEC_ID_VP5,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '5', '0'),
                    0x00 } },
#endif
    /*     CODEC_ID_VP6, */

#if LIBAVCODEC_BUILD >= 3345920    
    { "FFmpeg VP6.2 decoder", "On2 VP6.2", CODEC_ID_VP6,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '6', '2'),
                    0x00 } },
#endif

#if LIBAVCODEC_BUILD >= 3349248
    { "FFmpeg VP6.0 decoder", "On2 VP6.0", CODEC_ID_VP6,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '6', '0'),
                    0x00 } },
    { "FFmpeg VP6.1 decoder", "On2 VP6.1", CODEC_ID_VP6,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '6', '1'),
                    0x00 } },
#endif
    /*     CODEC_ID_VP6F, */
    /*     CODEC_ID_TARGA, */
    /*     CODEC_ID_DSICINVIDEO, */
#if LIBAVCODEC_BUILD >= 3346944    
    { "FFmpeg Delphine CIN video decoder", "Delphine CIN Video", CODEC_ID_DSICINVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('d', 'c', 'i', 'n'),
               0x00 } },
#endif      
    /*     CODEC_ID_TIERTEXSEQVIDEO, */
#if LIBAVCODEC_BUILD >= 3347200
    { "FFmpeg Tiertex Video Decoder", "Tiertex Video", CODEC_ID_TIERTEXSEQVIDEO,
      (uint32_t[]){ BGAV_MK_FOURCC('T', 'I', 'T', 'X'),
                    0x00 } },
#endif
    /*     CODEC_ID_TIFF, */
    /*     CODEC_ID_GIF, */
#if LIBAVCODEC_BUILD >= 3347712
    { "FFmpeg GIF Video Decoder", "GIF", CODEC_ID_GIF,
      (uint32_t[]){ BGAV_MK_FOURCC('g', 'i', 'f', ' '),
                    0x00 } },
#endif
    /*     CODEC_ID_FFH264, */
    /*     CODEC_ID_DXA, */
#if LIBAVCODEC_BUILD >= 3352320
    { "FFmpeg DXA decoder", "DXA", CODEC_ID_DXA,
      (uint32_t[]){ BGAV_MK_FOURCC('D', 'X', 'A', ' '),
                    0x00 } },
#endif
    /*     CODEC_ID_DNXHD, */
    /*     CODEC_ID_THP, */
#if LIBAVCODEC_BUILD >= 3352580
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
  real_num_codecs = 0;
  for(i = 0; i < NUM_CODECS; i++)
    {
    if(avcodec_find_decoder(codec_infos[i].ffmpeg_id))
      {
      codecs[real_num_codecs].info = &(codec_infos[i]);
      codecs[real_num_codecs].decoder.name = codecs[real_num_codecs].info->decoder_name;
      codecs[real_num_codecs].decoder.fourccs = codecs[real_num_codecs].info->fourccs;
      codecs[real_num_codecs].decoder.init = init;
      codecs[real_num_codecs].decoder.decode = decode;
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

static struct
  {
  enum PixelFormat  ffmpeg_csp;
  gavl_pixelformat_t gavl_csp;
  } pixelformats[] =
  {
    { PIX_FMT_YUV420P,       GAVL_YUV_420_P },  ///< Planar YUV 4:2:0 (1 Cr & Cb sample per 2x2 Y samples)
    { PIX_FMT_YUV422,        GAVL_YUY2      },
    { PIX_FMT_RGB24,         GAVL_RGB_24    },  ///< Packed pixel, 3 bytes per pixel, RGBRGB...
    { PIX_FMT_BGR24,         GAVL_BGR_24    },  ///< Packed pixel, 3 bytes per pixel, BGRBGR...
    { PIX_FMT_YUV422P,       GAVL_YUV_422_P },  ///< Planar YUV 4:2:2 (1 Cr & Cb sample per 2x1 Y samples)
    { PIX_FMT_YUV444P,       GAVL_YUV_444_P }, ///< Planar YUV 4:4:4 (1 Cr & Cb sample per 1x1 Y samples)
    { PIX_FMT_RGBA32,        GAVL_RGBA_32   },  ///< Packed pixel, 4 bytes per pixel, BGRABGRA..., stored in cpu endianness
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
    if((ctx->codec_id == CODEC_ID_MPEG4) && (format->pixelformat == GAVL_YUV_420_P))
      format->chroma_placement = GAVL_CHROMA_PLACEMENT_MPEG2;
    }
  
  if(!format->timescale)
    {
    format->timescale      = ctx->time_base.den;
    format->frame_duration = ctx->time_base.num;
    }
  
  }

#ifdef HAVE_LIBPOSTPROC
static void init_pp(bgav_stream_t * s)
  {
  int accel_flags;
  int pp_flags;
  ffmpeg_video_priv * priv;
  priv = (ffmpeg_video_priv*)(s->data.video.decoder->priv);

  
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
  priv = (ffmpeg_video_priv*)(s->data.video.decoder->priv);
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
  else if(priv->ctx->pix_fmt == PIX_FMT_RGBA32)
    {
    rgba32_to_rgba32(f, priv->frame,
                     s->data.video.format.image_width,
                     s->data.video.format.image_height, s->data.video.flip_y);
    }
  else if(!priv->do_convert)
    {
#ifdef HAVE_LIBPOSTPROC
    if(priv->do_pp)
      {
      if(s->data.video.flip_y)
        {
        pp_postprocess(priv->frame->data, priv->frame->linesize,
                       priv->flip_frame->planes, priv->flip_frame->strides,
                       priv->ctx->width, priv->ctx->height,
                       priv->frame->qscale_table, priv->frame->qstride,
                       priv->pp_mode, priv->pp_context,
                       priv->frame->pict_type);
        gavl_video_frame_copy_flip_y(&(s->data.video.format), f, priv->flip_frame);
        }
      else
        pp_postprocess(priv->frame->data, priv->frame->linesize,
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
  
  }
