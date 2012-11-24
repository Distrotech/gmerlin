/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

#include "ffmpeg_common.h"

#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "ffmpeg_encoder"


/*
 *  Standalone codecs
 */

struct bg_ffmpeg_codec_context_s
  {
  AVCodecContext * avctx_priv;
  AVCodecContext * avctx;
  
  gavl_packet_sink_t * psink;
  gavl_audio_sink_t  * asink;
  gavl_video_sink_t  * vsink;
#if LIBAVCODEC_VERSION_MAJOR >= 54
  AVDictionary * options;
#endif

  gavl_packet_t gp;

  /* Multipass stuff */

  char * stats_filename;
  int pass;
  int total_passes;
  FILE * stats_file;
  
  /* Only non-null within the format writer */
  const ffmpeg_format_info_t * format;

  enum CodecID id;

  int got_error;
  
  gavl_audio_format_t afmt;
  gavl_video_format_t vfmt;

  /* Video frame to encode */
  AVFrame * frame;
  };

/*
 *  Create a codec context.
 *  If avctx is NULL, it will be created and destroyed.
 *  If id is CODEC_ID_NONE, a "codec" parameter will be supported
 */

bg_ffmpeg_codec_context_t * bg_ffmpeg_codec_create(int type,
                                                   AVCodecContext * avctx,
                                                   enum CodecID id,
                                                   const ffmpeg_format_info_t * format)
  {
  bg_ffmpeg_codec_context_t * ret = calloc(1, sizeof(*ret));
  ret->format = format;
  ret->id = id;

  /* Get codec context from format context */
  if(avctx)
    ret->avctx = avctx;
  
  /* Create private codec context */
  else
    {
#if LIBAVCODEC_VERSION_INT < ((53<<16)|(8<<8)|0)
    codec->avctx_priv = avcodec_alloc_context();
#else
    AVCodec * codec = avcodec_find_encoder(ret->id);
    
    if(!codec)
      {
      av_free(ret->avctx_priv);
      free(ret);
      return NULL;
      }
    ret->avctx_priv = avcodec_alloc_context3(codec);
#endif
    ret->avctx = avctx;
    ret->avctx->codec_id = ret->id;
    }

  ret->avctx->codec_type = type;
  
  return ret;
  }

const bg_parameter_info_t * bg_ffmpeg_codec_get_parameters(bg_ffmpeg_codec_context_t * ctx)
  {
  return bg_ffmpeg_get_codec_parameters(ctx->id, ctx->avctx->codec_type);  
  }

void bg_ffmpeg_codec_set_parameter(bg_ffmpeg_codec_context_t * ctx,
                                   const char * name,
                                   const bg_parameter_value_t * val)
  {
  
  }

/*
 *  Audio
 */



gavl_audio_sink_t * bg_ffmpeg_codec_open_audio(bg_ffmpeg_codec_context_t * ctx,
                                               gavl_compression_info_t * ci,
                                               gavl_audio_format_t * fmt,
                                               gavl_metadata_t * m)
  {
  /* Adjust format */
  fmt->interleave_mode = GAVL_INTERLEAVE_ALL;
  fmt->sample_format   = GAVL_SAMPLE_S16;

  /* Set format for codec */

  /* Will be cleared later if we don't write compressed
     packets */
  ctx->avctx->sample_rate = fmt->samplerate;

  /* TODO: Channel setup */
  ctx->avctx->channels    = fmt->num_channels;

  /* Copy format for later use */
  gavl_audio_format_copy(&ctx->afmt, fmt);
  return ctx->asink;
  }

/*
 *  Video
 */

int bg_ffmpeg_codec_set_video_pass(bg_ffmpeg_codec_context_t * ctx,
                                   int pass,
                                   int total_passes,
                                   const char * stats_filename)
  {
  ctx->pass           = pass;
  ctx->total_passes   = total_passes;
  ctx->stats_filename = bg_strdup(ctx->stats_filename, stats_filename);
  return 1;
  }


static int flush_video(bg_ffmpeg_codec_context_t * ctx,
                       AVFrame * frame)
  {
  AVPacket pkt;

  int bytes_encoded = 0;
  int got_packet = 0;

  gavl_packet_reset(&ctx->gp);

  av_init_packet(&pkt);

#if ENCODE_VIDEO2
  pkt.data = ctx->gp.data;
  pkt.size = ctx->gp.data_alloc;
  
  if(avcodec_encode_video2(ctx->avctx, &pkt, frame, &got_packet) < 0)
    return -1;
  if(got_packet)
    bytes_encoded = pkt.size;
#else
  bytes_encoded = avcodec_encode_video(ctx->avctx,
                                       ctx->gp.data, ctx->gp.data_alloc,
                                       frame);
  if(bytes_encoded < 0)
    return;
  else if(bytes_encoded > 0)
    got_packet = 1;
#endif

  if(got_packet)
    {
    ctx->gp.duration = av_frame_get_pkt_duration(ctx->avctx->coded_frame);
    
#if ENCODE_VIDEO // Old
    ctx->gp.pts = ctx->avctx->coded_frame->pts;
    
    if(ctx->avctx->coded_frame->key_frame)
      ctc->gp.flags |= GAVL_PACKET_KEYFRAME;
    pkt.data = st->buffer;
    pkt.size = bytes_encoded;
#else // New
    ctx->gp.pts = pkt.pts;
#endif
    /* Write frame */
    
    if(gavl_packet_sink_put_packet(ctx->psink, &ctx->gp) != GAVL_SINK_OK)
      ctx->got_error = 1;
    
    /* Write stats */
    
    if((ctx->pass == 1) && ctx->avctx->stats_out && ctx->stats_file)
      fprintf(ctx->stats_file, "%s", ctx->avctx->stats_out);
    }
  return bytes_encoded;
  }

static gavl_sink_status_t
write_video_func(void * data, gavl_video_frame_t * frame)
  {
  bg_ffmpeg_codec_context_t * ctx = data;
  
  ctx->frame->pts = frame->timestamp;
  av_frame_set_pkt_duration(ctx->frame, frame->duration);
  
  ctx->frame->data[0]     = frame->planes[0];
  ctx->frame->data[1]     = frame->planes[1];
  ctx->frame->data[2]     = frame->planes[2];
  ctx->frame->linesize[0] = frame->strides[0];
  ctx->frame->linesize[1] = frame->strides[1];
  ctx->frame->linesize[2] = frame->strides[2];
  
  flush_video(ctx, ctx->frame);

  if(ctx->got_error)
    return GAVL_SINK_ERROR;
  
  return GAVL_SINK_OK;
  }

gavl_video_sink_t * bg_ffmpeg_codec_open_video(bg_ffmpeg_codec_context_t * ctx,
                                               gavl_compression_info_t * ci,
                                               gavl_video_format_t * fmt,
                                               gavl_metadata_t * m)
  {
  AVCodec * codec = avcodec_find_encoder(ctx->id);
  
  /* Set format for codec */
  ctx->avctx->width  = fmt->image_width;
  ctx->avctx->height = fmt->image_height;
  
  ctx->avctx->sample_aspect_ratio.num = fmt->pixel_width;
  ctx->avctx->sample_aspect_ratio.den = fmt->pixel_height;

  
  
#if LIBAVCODEC_VERSION_MAJOR < 54
  if(avcodec_open(ctx->avctx, codec) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "avcodec_open failed for video");
    return 0;
    }
#else
  if(avcodec_open2(ctx->avctx, codec, &ctx->options) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "avcodec_open2 failed for video");
    return 0;
    }
#endif

  
  ctx->frame = avcodec_alloc_frame();
  
  gavl_packet_alloc(&ctx->gp, fmt->image_width * fmt->image_width * 4);
  
  gavl_video_format_copy(&ctx->vfmt, fmt);
  ctx->vsink = gavl_video_sink_create(NULL, write_video_func, ctx, &ctx->vfmt);
  
  return ctx->vsink;
  }

void bg_ffmpeg_codec_set_packet_sink(bg_ffmpeg_codec_context_t * ctx,
                                     gavl_packet_sink_t * psink)
  {
  ctx->psink = psink;
  }

void bg_ffmpeg_codec_destroy(bg_ffmpeg_codec_context_t * ctx)
  {
  /* Flush */

  /* Close */
  
  /* Destroy */
  
  gavl_packet_free(&ctx->gp);
  free(ctx);
  }
