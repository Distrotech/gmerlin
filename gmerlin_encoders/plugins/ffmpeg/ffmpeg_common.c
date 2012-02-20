/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

#include <stdlib.h>
#include <string.h>

#include <config.h>

#include "ffmpeg_common.h"
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "ffmpeg"

#if LIBAVCODEC_VERSION_MAJOR >= 53
#define CodecType AVMediaType
#define CODEC_TYPE_UNKNOWN    AVMEDIA_TYPE_UNKNOWN
#define CODEC_TYPE_VIDEO      AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_AUDIO      AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_DATA       AVMEDIA_TYPE_DATA
#define CODEC_TYPE_SUBTITLE   AVMEDIA_TYPE_SUBTITLE
#define CODEC_TYPE_ATTACHMENT AVMEDIA_TYPE_ATTACHMENT
#define CODEC_TYPE_NB         AVMEDIA_TYPE_NB
#endif


#if LIBAVCODEC_VERSION_MAJOR >= 53
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif

#if LIBAVCODEC_VERSION_MAJOR >= 53
#define guess_format(a, b, c) av_guess_format(a, b, c)
#endif

#if LIBAVFORMAT_VERSION_MAJOR >= 53
#define NEW_METADATA
#endif

#ifdef NEW_METADATA
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51,5,0)
/* metadata -> dictionary */
#define av_metadata_set2(a,b,c,d) av_dict_set(a,b,c,d)
#endif
#endif

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 96, 0)  
#define AVFORMAT_FREE_CONTEXT 1
#endif


static bg_parameter_info_t *
create_format_parameters(const ffmpeg_format_info_t * formats)
  {
  int num_formats, i;
  
  bg_parameter_info_t * ret;
  ret = calloc(2, sizeof(*ret));

  ret[0].name = bg_strdup(ret[0].name, "format");
  ret[0].long_name = bg_strdup(ret[0].long_name, TRS("Format"));
  ret[0].type = BG_PARAMETER_STRINGLIST;

  num_formats = 0;
  while(formats[num_formats].name)
    num_formats++;

  ret[0].multi_names_nc =
    calloc(num_formats+1, sizeof(*ret[0].multi_names_nc));
  ret[0].multi_labels_nc =
    calloc(num_formats+1, sizeof(*ret[0].multi_labels_nc));

  for(i = 0; i < num_formats; i++)
    {
    ret[0].multi_names_nc[i] =
      bg_strdup(ret[0].multi_names_nc[i], formats[i].short_name);
    ret[0].multi_labels_nc[i] =
      bg_strdup(ret[0].multi_labels_nc[i], formats[i].name);
    }
  bg_parameter_info_set_const_ptrs(&ret[0]);
  ret[0].val_default.val_str =
    bg_strdup(ret[0].val_default.val_str, formats[0].short_name);
  return ret;
  }

void * bg_ffmpeg_create(const ffmpeg_format_info_t * formats)
  {
  ffmpeg_priv_t * ret;
  av_register_all();

  ret = calloc(1, sizeof(*ret));
  
  ret->formats = formats;

  ret->audio_parameters =
    bg_ffmpeg_create_audio_parameters(formats);
  ret->video_parameters =
    bg_ffmpeg_create_video_parameters(formats);
  ret->parameters = create_format_parameters(formats);
  
  return ret;
  }

void bg_ffmpeg_set_callbacks(void * data,
                             bg_encoder_callbacks_t * cb)
  {
  ffmpeg_priv_t * f = data;
  f->cb = cb;
  }
                             

void bg_ffmpeg_destroy(void * data)
  {
  ffmpeg_priv_t * priv;
  priv = data;

  if(priv->parameters)
    bg_parameter_info_destroy_array(priv->parameters);
  if(priv->audio_parameters)
    bg_parameter_info_destroy_array(priv->audio_parameters);
  if(priv->video_parameters)
    bg_parameter_info_destroy_array(priv->video_parameters);

  if(priv->audio_streams)
    free(priv->audio_streams);
  if(priv->video_streams)
    free(priv->video_streams);
  free(priv);
  }

const bg_parameter_info_t * bg_ffmpeg_get_parameters(void * data)
  {
  ffmpeg_priv_t * priv;
  priv = data;
  return priv->parameters;
  }

const bg_parameter_info_t * bg_ffmpeg_get_audio_parameters(void * data)
  {
  ffmpeg_priv_t * priv;
  priv = data;
  return priv->audio_parameters;
  }

const bg_parameter_info_t * bg_ffmpeg_get_video_parameters(void * data)
  {
  ffmpeg_priv_t * priv;
  priv = data;
  return priv->video_parameters;
  }

void bg_ffmpeg_set_parameter(void * data, const char * name,
                             const bg_parameter_value_t * v)
  {
  int i;
  ffmpeg_priv_t * priv;
  priv = data;
  if(!name)
    {
    return;
    }
  /* Get format to encode */
  if(!strcmp(name, "format"))
    {
    i = 0;
    while(priv->formats[i].name)
      {
      if(!strcmp(priv->formats[i].short_name, v->val_str))
        {
        priv->format = &priv->formats[i];
        break;
        }
      i++;
      }
    }
  
  }

#ifdef NEW_METADATA
static void set_metadata(ffmpeg_priv_t * priv,
                         const bg_metadata_t * m)
  {
  if(m->title)
    av_metadata_set2(&priv->ctx->metadata, "title", m->title, 0);
  
  if(m->author)
    av_metadata_set2(&priv->ctx->metadata, "composer", m->author, 0);
  
  if(m->album)
    av_metadata_set2(&priv->ctx->metadata, "album", m->album, 0);

  if(m->copyright)
    av_metadata_set2(&priv->ctx->metadata, "copyright", m->copyright, 0);

  if(m->comment)
    av_metadata_set2(&priv->ctx->metadata, "comment", m->comment, 0);
  
  if(m->genre)
    av_metadata_set2(&priv->ctx->metadata, "genre", m->genre, 0);
    
  if(m->date)
    av_metadata_set2(&priv->ctx->metadata, "date", m->date, 0);

  if(m->track)
    {
    char * str = bg_sprintf("%d", m->track);
    av_metadata_set2(&priv->ctx->metadata, "track", str, 0);
    free(str);
    }
  }
#else
static void set_metadata(ffmpeg_priv_t * priv,
                         const bg_metadata_t * metadata)
  {
  if(metadata->title)
    strncpy(priv->ctx->title, metadata->title,
            sizeof(priv->ctx->title)-1);

  if(metadata->author)
    strncpy(priv->ctx->author, metadata->author,
            sizeof(priv->ctx->author)-1);

  if(metadata->album)
    strncpy(priv->ctx->album, metadata->album,
            sizeof(priv->ctx->album)-1);

  if(metadata->copyright)
    strncpy(priv->ctx->copyright, metadata->copyright,
            sizeof(priv->ctx->copyright)-1);

  if(metadata->comment)
    strncpy(priv->ctx->comment, metadata->comment,
            sizeof(priv->ctx->comment)-1);
    
  if(metadata->genre)
    strncpy(priv->ctx->genre, metadata->genre,
            sizeof(priv->ctx->genre)-1);

  if(metadata->date)
    priv->ctx->year = bg_metadata_get_year(metadata);
    
  priv->ctx->track = metadata->track;
  
  }
#endif

int bg_ffmpeg_open(void * data, const char * filename,
                   const bg_metadata_t * metadata,
                   const bg_chapter_list_t * chapter_list)
  {
  ffmpeg_priv_t * priv;
  AVOutputFormat *fmt;
  char * tmp_string;
  
  priv = data;
  if(!priv->format)
    return 0;

  /* Initialize format context */
  fmt = guess_format(priv->format->short_name, NULL, NULL);
  if(!fmt)
    return 0;
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 26, 0)
  priv->ctx = av_alloc_format_context();
#else
  priv->ctx = avformat_alloc_context();
#endif
  tmp_string =
    bg_filename_ensure_extension(filename,
                                 priv->format->extension);

  if(!bg_encoder_cb_create_output_file(priv->cb, tmp_string))
    {
    free(tmp_string);
    return 0;
    }
  
  snprintf(priv->ctx->filename,
           sizeof(priv->ctx->filename), "%s", tmp_string);
  
  free(tmp_string);
  
  priv->ctx->oformat = fmt;
    
  /* Add metadata */

  if(metadata)
    {
    set_metadata(priv, metadata);
    }
  
  return 1;
  }

int bg_ffmpeg_add_audio_stream(void * data, const char * language,
                               const gavl_audio_format_t * format)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_audio_stream_t * st;
  
  priv = data;
  
  priv->audio_streams =
    realloc(priv->audio_streams,
            (priv->num_audio_streams+1)*sizeof(*priv->audio_streams));
  
  st = priv->audio_streams + priv->num_audio_streams;
  memset(st, 0, sizeof(*st));
  
  gavl_audio_format_copy(&st->format, format);

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,10,0)
  st->stream = avformat_new_stream(priv->ctx, NULL);
#else
  st->stream = av_new_stream(priv->ctx,
                             priv->num_audio_streams +
                             priv->num_video_streams);
#endif 
  
  /* Adjust format */
  st->format.interleave_mode = GAVL_INTERLEAVE_ALL;
  st->format.sample_format   = GAVL_SAMPLE_S16;

  /* Set format for codec */
  st->stream->codec->sample_rate = st->format.samplerate;
  st->stream->codec->channels    = st->format.num_channels;
  st->stream->codec->codec_type        = CODEC_TYPE_AUDIO;
  
  priv->num_audio_streams++;
  return priv->num_audio_streams-1;
  }

int bg_ffmpeg_add_video_stream(void * data, const gavl_video_format_t * format)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_video_stream_t * st;
  priv = data;

  priv->video_streams =
    realloc(priv->video_streams,
            (priv->num_video_streams+1)*sizeof(*priv->video_streams));
  
  st = priv->video_streams + priv->num_video_streams;
  memset(st, 0, sizeof(*st));
  
  gavl_video_format_copy(&st->format, format);

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,10,0)
  st->stream = avformat_new_stream(priv->ctx, NULL);
#else
  st->stream = av_new_stream(priv->ctx,
                             priv->num_audio_streams +
                             priv->num_video_streams);
#endif 
  
  st->stream->codec->codec_type = CODEC_TYPE_VIDEO;
  
  /* Set format for codec */
  st->stream->codec->width  = st->format.image_width;
  st->stream->codec->height = st->format.image_height;

  st->stream->codec->sample_aspect_ratio.num = st->format.pixel_width;
  st->stream->codec->sample_aspect_ratio.den = st->format.pixel_height;
  st->stream->sample_aspect_ratio.num = st->format.pixel_width;
  st->stream->sample_aspect_ratio.den = st->format.pixel_height;
  
  priv->num_video_streams++;
  return priv->num_video_streams-1;
  }

void bg_ffmpeg_set_audio_parameter(void * data, int stream, const char * name,
                                   const bg_parameter_value_t * v)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_audio_stream_t * st;
  priv = data;

  st = &priv->audio_streams[stream];
  
  if(!name)
    {
    /* Set the bitrate for PCM codecs */
    switch(st->stream->codec->codec_id)
      {
      case CODEC_ID_PCM_S16BE:
      case CODEC_ID_PCM_S16LE:
        st->stream->codec->bit_rate = st->format.samplerate * st->format.num_channels * 16;
        break;
      case CODEC_ID_PCM_S8:
      case CODEC_ID_PCM_U8:
      case CODEC_ID_PCM_ALAW:
      case CODEC_ID_PCM_MULAW:
        st->stream->codec->bit_rate = st->format.samplerate * st->format.num_channels * 8;
        break;
      default:
        break;
      }
    
    return;
    }
  
  
  if(!strcmp(name, "codec"))
    st->stream->codec->codec_id = bg_ffmpeg_find_audio_encoder(priv->format, v->val_str);
  else
    bg_ffmpeg_set_codec_parameter(st->stream->codec,
#if LIBAVCODEC_VERSION_MAJOR >= 54
                                  &st->options,
#endif
                                  name, v);
  
  }


void bg_ffmpeg_set_video_parameter(void * data, int stream, const char * name,
                                   const bg_parameter_value_t * v)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_video_stream_t * st;
  priv = data;

  if(!name)
    {
    return;
    }
  st = &priv->video_streams[stream];
  
  if(!strcmp(name, "codec"))
    st->stream->codec->codec_id =
      bg_ffmpeg_find_video_encoder(priv->format, v->val_str);
  else if(bg_encoder_set_framerate_parameter(&st->fr, name, v))
    {
    return;
    }
  else
    bg_ffmpeg_set_codec_parameter(st->stream->codec,
#if LIBAVCODEC_VERSION_MAJOR >= 54
                                  &st->options,
#endif
                                  name, v);
  }


int bg_ffmpeg_set_video_pass(void * data, int stream, int pass,
                             int total_passes,
                             const char * stats_filename)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_video_stream_t * st;
  priv = data;

  st = &priv->video_streams[stream];
  st->pass           = pass;
  st->total_passes   = total_passes;
  st->stats_filename = bg_strdup(st->stats_filename, stats_filename);

  return 1;
  }

static int open_audio_encoder(ffmpeg_priv_t * priv,
                              ffmpeg_audio_stream_t * st)
  {
  AVCodec * codec;

  if(st->stream->codec->codec_id == CODEC_ID_NONE)
    return 0;
  
  codec = avcodec_find_encoder(st->stream->codec->codec_id);
  if(!codec)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Audio codec not available in your libavcodec installation");
    return 0;
    }

  st->stream->codec->sample_fmt = codec->sample_fmts[0];
  st->format.sample_format = bg_sample_format_ffmpeg_2_gavl(codec->sample_fmts[0]);

#if LIBAVCODEC_VERSION_MAJOR < 54
  if(avcodec_open(st->stream->codec, codec) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "avcodec_open failed for audio");
    return 0;
    }
#else
  if(avcodec_open2(st->stream->codec, codec, &st->options) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "avcodec_open2 failed for audio");
    return 0;
    }
#endif
  
  if(st->stream->codec->frame_size <= 1)
    st->format.samples_per_frame = 1024; // Frame size for uncompressed codecs
  else
    st->format.samples_per_frame = st->stream->codec->frame_size;
  
  st->frame = gavl_audio_frame_create(&st->format);
  /* Mute frame */
  gavl_audio_frame_mute(st->frame, &st->format);
  
  st->buffer_alloc = 32768;
  st->buffer = malloc(st->buffer_alloc); /* Hopefully enough */

  st->initialized = 1;
  
  return 1;
  }


static int open_video_encoder(ffmpeg_priv_t * priv,
                              ffmpeg_video_stream_t * st)
  {
  int stats_len;
  AVCodec * codec;

  if(st->stream->codec->codec_id == CODEC_ID_NONE)
    return 0;

  codec = avcodec_find_encoder(st->stream->codec->codec_id);
  if(!codec)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Video codec not available in your libavcodec installation");
    return 0;
    }

  /* Set up multipass encoding */
  
  if(st->total_passes)
    {
    if(st->pass == 1)
      {
      st->stats_file = fopen(st->stats_filename, "w");
      st->stream->codec->flags |= CODEC_FLAG_PASS1;
      }
    else if(st->pass == st->total_passes)
      {
      st->stats_file = fopen(st->stats_filename, "r");
      fseek(st->stats_file, 0, SEEK_END);
      stats_len = ftell(st->stats_file);
      fseek(st->stats_file, 0, SEEK_SET);
      
      st->stream->codec->stats_in = av_malloc(stats_len + 1);
      if(fread(st->stream->codec->stats_in, 1, stats_len, st->stats_file) < stats_len)
        {
        av_free(st->stream->codec->stats_in);
        st->stream->codec->stats_in = NULL;
        }
      else
        st->stream->codec->stats_in[stats_len] = '\0';
      
      fclose(st->stats_file);
      st->stats_file = NULL;
      
      st->stream->codec->flags |= CODEC_FLAG_PASS2;
      }
    }

  /* Set up pixelformat */
  
  st->stream->codec->pix_fmt = codec->pix_fmts[0];
  st->format.pixelformat = bg_pixelformat_ffmpeg_2_gavl(st->stream->codec->pix_fmt);
  
  /* Set up framerate */

  if(priv->format->flags & FLAG_CONSTANT_FRAMERATE)
    {
    if(priv->format->framerates)
      bg_encoder_set_framerate_nearest(&st->fr,
                                       priv->format->framerates,
                                       &st->format);
    else
      bg_encoder_set_framerate(&st->fr, &st->format);
    }
  
  if(st->format.framerate_mode == GAVL_FRAMERATE_CONSTANT)
    {
    st->stream->codec->time_base.den = st->format.timescale;
    st->stream->codec->time_base.num = st->format.frame_duration;
    }
  else
    {
    st->stream->codec->time_base.den = st->format.timescale;
    st->stream->codec->time_base.num = 1;
    }

#if LIBAVCODEC_VERSION_MAJOR < 54
  if(avcodec_open(st->stream->codec, codec) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "avcodec_open failed for video");
    return 0;
    }
#else
  if(avcodec_open2(st->stream->codec, codec, &st->options) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "avcodec_open2 failed for video");
    return 0;
    }
#endif

  st->buffer_alloc = st->format.image_width * st->format.image_width * 4;
  st->buffer = malloc(st->buffer_alloc);
  
  st->frame = avcodec_alloc_frame();
  st->initialized = 1;
  
  return 1;
  }

int bg_ffmpeg_start(void * data)
  {
  ffmpeg_priv_t * priv;
  int i;
  priv = data;
  
#if LIBAVFORMAT_VERSION_MAJOR < 54
  /* set the output parameters (must be done even if no
     parameters). */
  if(av_set_parameters(priv->ctx, NULL) < 0)
    {
    return 0;
    }
#endif
  
  /* Open encoders */
  for(i = 0; i < priv->num_audio_streams; i++)
    {
    if(!open_audio_encoder(priv, &priv->audio_streams[i]))
      return 0;
    }

  for(i = 0; i < priv->num_video_streams; i++)
    {
    if(!open_video_encoder(priv, &priv->video_streams[i]))
      return 0;
    }

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 105, 0)  
  /* Open file */
  if(url_fopen(&priv->ctx->pb, priv->ctx->filename, URL_WRONLY) < 0)
    return 0;
#else
  if(avio_open(&priv->ctx->pb, priv->ctx->filename, AVIO_FLAG_WRITE) < 0)
    return 0;
#endif
  
#if LIBAVFORMAT_VERSION_MAJOR < 54
  if(av_write_header(priv->ctx))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "av_write_header failed");
    return 0;
    }
#else
  if(avformat_write_header(priv->ctx, NULL))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "avformat_write_header failed");
    return 0;
    }
#endif
  
  priv->initialized = 1;
  return 1;
  }

void bg_ffmpeg_get_audio_format(void * data, int stream,
                                gavl_audio_format_t*ret)
  {
  ffmpeg_priv_t * priv;
  priv = data;

  gavl_audio_format_copy(ret, &priv->audio_streams[stream].format);
                         
  }

void bg_ffmpeg_get_video_format(void * data, int stream,
                                gavl_video_format_t*ret)
  {
  ffmpeg_priv_t * priv;
  priv = data;

  gavl_video_format_copy(ret, &priv->video_streams[stream].format);
  
  }

#if ENCODE_AUDIO2
static int flush_audio(ffmpeg_priv_t * priv,
                       ffmpeg_audio_stream_t * st)
  {
  AVPacket pkt;
  AVFrame f;
  int got_packet;
  
  av_init_packet(&pkt);

  pkt.data = st->buffer;
  pkt.size = st->buffer_alloc;
  
  avcodec_get_frame_defaults(&f);
  f.nb_samples = st->frame->valid_samples;

  f.pts = st->samples_written;
  
  avcodec_fill_audio_frame(&f, st->format.num_channels, st->stream->codec->sample_fmt,
                           st->frame->samples.u_8,
                           st->stream->codec->frame_size * st->format.num_channels * 2, 1);
    
  if(avcodec_encode_audio2(st->stream->codec, &pkt,
                           &f, &got_packet) < 0)
    return 0;
  
  if(got_packet && pkt.size)
    {
    if(pkt.pts != AV_NOPTS_VALUE)
      pkt.pts= av_rescale_q(pkt.pts,
                            st->stream->codec->time_base,
                            st->stream->time_base);
    
    pkt.flags |= PKT_FLAG_KEY;
    pkt.stream_index= st->stream->index;
    
    /* write the compressed frame in the media file */
    if(av_interleaved_write_frame(priv->ctx, &pkt) != 0)
      //    if(av_write_frame(priv->ctx, &pkt) != 0)
      {
      priv->got_error = 1;
      return 0;
      }
    }
  
  /* Mute frame */
  gavl_audio_frame_mute(st->frame, &st->format);
  st->frame->valid_samples = 0;
  st->samples_written += st->format.samples_per_frame;
  
  return pkt.size;
  }

#else

static int flush_audio(ffmpeg_priv_t * priv,
                       ffmpeg_audio_stream_t * st)
  {
  int out_size;
  int bytes_encoded;
  AVPacket pkt;

  if(st->stream->codec->frame_size <= 1)
    out_size = st->stream->codec->block_align * st->frame->valid_samples;
  else
    out_size = st->buffer_alloc;

  bytes_encoded = avcodec_encode_audio(st->stream->codec, st->buffer,
                                       out_size,
                                       st->frame->samples.s_16);
  
  if(bytes_encoded > 0)
    {
    av_init_packet(&pkt);
    pkt.size = bytes_encoded;
    
    if(st->stream->codec->coded_frame &&
       (st->stream->codec->coded_frame->pts != AV_NOPTS_VALUE))
      pkt.pts= av_rescale_q(st->stream->codec->coded_frame->pts,
                            st->stream->codec->time_base,
                            st->stream->time_base);
    
    pkt.flags |= PKT_FLAG_KEY;
    pkt.stream_index= st->stream->index;
    pkt.data= st->buffer;
    
    /* write the compressed frame in the media file */
    if(av_interleaved_write_frame(priv->ctx, &pkt) != 0)
      //    if(av_write_frame(priv->ctx, &pkt) != 0)
      {
      priv->got_error = 1;
      return 0;
      }
    }
  
  /* Mute frame */
  gavl_audio_frame_mute(st->frame, &st->format);
  st->frame->valid_samples = 0;
  return bytes_encoded;
  }

#endif

int bg_ffmpeg_write_audio_frame(void * data,
                                gavl_audio_frame_t * frame, int stream)
  {
  ffmpeg_audio_stream_t * st;
  ffmpeg_priv_t * priv;
  int samples_written = 0;
  int samples_copied;
  priv = data;
  st = &priv->audio_streams[stream];
  
  while(samples_written < frame->valid_samples)
    {
    samples_copied =
      gavl_audio_frame_copy(&st->format,
                            st->frame, // dst frame
                            frame,     // src frame
                            st->frame->valid_samples, // dst_pos
                            samples_written,          // src_pos,
                            st->format.samples_per_frame - st->frame->valid_samples, // dst_size,
                            frame->valid_samples - samples_written);
    
    st->frame->valid_samples += samples_copied;
    if(st->frame->valid_samples == st->format.samples_per_frame)
      {
      flush_audio(priv, st);
      
      if(priv->got_error)
        return 0;
      }
    samples_written += samples_copied;
    }
  return 1;
  }

static int flush_video(ffmpeg_priv_t * priv, ffmpeg_video_stream_t * st,
                       AVFrame * frame)
  {
  AVPacket pkt;

  int bytes_encoded = 0;
  int got_packet = 0;
  av_init_packet(&pkt);

#if ENCODE_VIDEO2
  pkt.data = st->buffer;
  pkt.size = st->buffer_alloc;
  
  if(avcodec_encode_video2(st->stream->codec, &pkt, frame, &got_packet) < 0)
    return -1;
  if(got_packet)
    bytes_encoded = pkt.size;
#else
  bytes_encoded = avcodec_encode_video(st->stream->codec,
                                       st->buffer, st->buffer_alloc,
                                       frame);
  if(bytes_encoded < 0)
    return;
  else if(bytes_encoded > 0)
    got_packet = 1;
#endif

  if(got_packet)
    {
#if ENCODE_VIDEO // Old
    pkt.pts= av_rescale_q(st->stream->codec->coded_frame->pts,
                          st->stream->codec->time_base,
                          st->stream->time_base);
    
    if(st->stream->codec->coded_frame->key_frame)
      pkt.flags |= PKT_FLAG_KEY;
    pkt.data = st->buffer;
    pkt.size = bytes_encoded;
#else // New

    if(pkt.pts != AV_NOPTS_VALUE)
      pkt.pts= av_rescale_q(pkt.pts,
                            st->stream->codec->time_base,
                            st->stream->time_base);
    if(pkt.dts != AV_NOPTS_VALUE)
      pkt.dts= av_rescale_q(pkt.dts,
                            st->stream->codec->time_base,
                            st->stream->time_base);
#endif
    
    pkt.stream_index = st->stream->index;

    //    if(av_write_frame(priv->ctx, &pkt) != 0) 
    if(av_interleaved_write_frame(priv->ctx, &pkt) != 0)
      {
      priv->got_error = 1;
      return 0;
      }

    /* Write stats */
    
    if((st->pass == 1) && st->stream->codec->stats_out && st->stats_file)
      fprintf(st->stats_file, "%s", st->stream->codec->stats_out);
    }
  return bytes_encoded;
  }
               
                

int bg_ffmpeg_write_video_frame(void * data,
                                gavl_video_frame_t * frame, int stream)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_video_stream_t * st;
  
  priv = data;
  st = &priv->video_streams[stream];

  if(st->stream->codec->time_base.num == 1) /* Variable */
    st->frame->pts = frame->timestamp;
  else
    st->frame->pts = st->frames_written;
  
  st->frame->data[0]     = frame->planes[0];
  st->frame->data[1]     = frame->planes[1];
  st->frame->data[2]     = frame->planes[2];
  st->frame->linesize[0] = frame->strides[0];
  st->frame->linesize[1] = frame->strides[1];
  st->frame->linesize[2] = frame->strides[2];
  
  flush_video(priv, st, st->frame);

  st->frames_written++;
  
  if(priv->got_error)
    return 0;
  
  return 1;
  }

static void flush_audio_encoder(ffmpeg_priv_t * priv,
                                ffmpeg_audio_stream_t * st)
  {
  /* Flush */
  if(st->frame && st->frame->valid_samples && priv->initialized)
    {
    if(!flush_audio(priv, st))
      return;
    }
  }
  
static int close_audio_encoder(ffmpeg_priv_t * priv,
                               ffmpeg_audio_stream_t * st)
  {
  
  /* Close encoder and free buffers */

  if(st->initialized)
    avcodec_close(st->stream->codec);
  
#ifndef AVFORMAT_FREE_CONTEXT  
  av_free(st->stream->codec);
#endif
  
  //  st->stream->codec = NULL;
  
  if(st->buffer)
    free(st->buffer);
  if(st->frame)
    gavl_audio_frame_destroy(st->frame);
  
  return 1;
  }

static void flush_video_encoder(ffmpeg_priv_t * priv,
                                ffmpeg_video_stream_t * st)
  {
  int result;
  if(priv->initialized)
    {
    while(1)
      {
      result = flush_video(priv, st, NULL);
      if(result <= 0)
        break;
      }
    }
  }

static void close_video_encoder(ffmpeg_priv_t * priv,
                                ffmpeg_video_stream_t * st)
  {
  if(st->stream->codec->stats_in)
    {
    free(st->stream->codec->stats_in);
    st->stream->codec->stats_in = NULL;
    }
  if(st->initialized)
    avcodec_close(st->stream->codec);
  
  /* Close encoder and free buffers */
#ifndef AVFORMAT_FREE_CONTEXT  
  av_free(st->stream->codec);
#endif
  
  //  st->stream->codec = NULL;
  
  if(st->frame)
    free(st->frame);
  if(st->buffer)
    free(st->buffer);

  if(st->stats_filename)
    free(st->stats_filename);
  
  if(st->stats_file)
    fclose(st->stats_file);
  
  }

int bg_ffmpeg_close(void * data, int do_delete)
  {
  ffmpeg_priv_t * priv;
  int i;
  priv = data;

  // Flush the streams

  for(i = 0; i < priv->num_audio_streams; i++)
    {
    flush_audio_encoder(priv, &priv->audio_streams[i]);
    }

  for(i = 0; i < priv->num_video_streams; i++)
    {
    flush_video_encoder(priv, &priv->video_streams[i]);
    }

  if(priv->initialized)
    {
    av_write_trailer(priv->ctx);
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 105, 0)  
    url_fclose(priv->ctx->pb);
#else
    avio_close(priv->ctx->pb);
#endif
    }

  // Flush the encoders

  for(i = 0; i < priv->num_audio_streams; i++)
    {
    close_audio_encoder(priv, &priv->audio_streams[i]);
    }

  for(i = 0; i < priv->num_video_streams; i++)
    {
    close_video_encoder(priv, &priv->video_streams[i]);
    }

  
#if AVFORMAT_FREE_CONTEXT  
  avformat_free_context(priv->ctx);
#else
  av_free(priv->ctx);
#endif


  priv->ctx = NULL;
 
  if(do_delete)
    remove(priv->ctx->filename);
  
  return 1;
  }

const bg_encoder_framerate_t bg_ffmpeg_mpeg_framerates[] =
  {
    { 24000, 1001 },
    {    24,    1 },
    {    25,    1 },
    { 30000, 1001 },
    {    30,    1 },
    {    50,    1 },
    { 60000, 1001 },
    {    60,    1 },
    { /* End of framerates */ }
  };

static const struct
  {
  enum PixelFormat  ffmpeg_csp;
  gavl_pixelformat_t gavl_csp;
  }
pixelformats[] =
  {
    { PIX_FMT_YUV420P,       GAVL_YUV_420_P },  ///< Planar YUV 4:2:0 (1 Cr & Cb sample per 2x2 Y samples)
#if LIBAVUTIL_VERSION_INT < (50<<16)
    { PIX_FMT_YUV422,        GAVL_YUY2      },
#else
    { PIX_FMT_YUYV422,       GAVL_YUY2      },
#endif
    { PIX_FMT_YUV422P,       GAVL_YUV_422_P },  ///< Planar YUV 4:2:2 (1 Cr & Cb sample per 2x1 Y samples)
    { PIX_FMT_YUV444P,       GAVL_YUV_444_P }, ///< Planar YUV 4:4:4 (1 Cr & Cb sample per 1x1 Y samples)
    { PIX_FMT_YUV411P,       GAVL_YUV_411_P }, ///< Planar YUV 4:1:1 (1 Cr & Cb sample per 4x1 Y samples)
    { PIX_FMT_YUVJ420P,      GAVL_YUVJ_420_P }, ///< Planar YUV 4:2:0 full scale (jpeg)
    { PIX_FMT_YUVJ422P,      GAVL_YUVJ_422_P }, ///< Planar YUV 4:2:2 full scale (jpeg)
    { PIX_FMT_YUVJ444P,      GAVL_YUVJ_444_P }, ///< Planar YUV 4:4:4 full scale (jpeg)

#if 0 // Not needed in the forseeable future    
    { PIX_FMT_RGB24,         GAVL_RGB_24    },  ///< Packed pixel, 3 bytes per pixel, RGBRGB...
    { PIX_FMT_BGR24,         GAVL_BGR_24    },  ///< Packed pixel, 3 bytes per pixel, BGRBGR...
#if LIBAVUTIL_VERSION_INT < (50<<16)
    { PIX_FMT_RGBA32,        GAVL_RGBA_32   },  ///< Packed pixel, 4 bytes per pixel, BGRABGRA..., stored in cpu endianness
#else
    { PIX_FMT_RGB32,         GAVL_RGBA_32   },  ///< Packed pixel, 4 bytes per pixel, BGRABGRA..., stored in cpu endianness
#endif
    { PIX_FMT_YUV410P,       GAVL_YUV_410_P }, ///< Planar YUV 4:1:0 (1 Cr & Cb sample per 4x4 Y samples)
    { PIX_FMT_RGB565,        GAVL_RGB_16 }, ///< always stored in cpu endianness
    { PIX_FMT_RGB555,        GAVL_RGB_15 }, ///< always stored in cpu endianness, most significant bit to 1
    { PIX_FMT_GRAY8,         GAVL_PIXELFORMAT_NONE },
    { PIX_FMT_MONOWHITE,     GAVL_PIXELFORMAT_NONE }, ///< 0 is white
    { PIX_FMT_MONOBLACK,     GAVL_PIXELFORMAT_NONE }, ///< 0 is black
    // { PIX_FMT_PAL8,          GAVL_RGB_24     }, ///< 8 bit with RGBA palette
    { PIX_FMT_XVMC_MPEG2_MC, GAVL_PIXELFORMAT_NONE }, ///< XVideo Motion Acceleration via common packet passing(xvmc_render.h)
    { PIX_FMT_XVMC_MPEG2_IDCT, GAVL_PIXELFORMAT_NONE },
#if LIBAVCODEC_BUILD >= ((51<<16)+(45<<8)+0)
    { PIX_FMT_YUVA420P,      GAVL_YUVA_32 },
#endif
    
#endif // Not needed
    { PIX_FMT_NB, GAVL_PIXELFORMAT_NONE }
};

gavl_pixelformat_t bg_pixelformat_ffmpeg_2_gavl(enum PixelFormat p)
  {
  int i;
  for(i = 0; i < sizeof(pixelformats)/sizeof(pixelformats[0]); i++)
    {
    if(pixelformats[i].ffmpeg_csp == p)
      return pixelformats[i].gavl_csp;
    }
  return GAVL_PIXELFORMAT_NONE;
  }

enum PixelFormat bg_pixelformat_gavl_2_ffmpeg(gavl_pixelformat_t p)
  {
  int i;
  for(i = 0; i < sizeof(pixelformats)/sizeof(pixelformats[0]); i++)
    {
    if(pixelformats[i].gavl_csp == p)
      return pixelformats[i].ffmpeg_csp;
    }
  return PIX_FMT_NONE;
  }


static const struct
  {
  enum SampleFormat  ffmpeg_fmt;
  gavl_sample_format_t gavl_fmt;
  }
sampleformats[] =
  {
    { SAMPLE_FMT_U8,  GAVL_SAMPLE_U8 },
    { SAMPLE_FMT_S16, GAVL_SAMPLE_S16 },    ///< signed 16 bits
    { SAMPLE_FMT_S32, GAVL_SAMPLE_S32 },    ///< signed 32 bits
    { SAMPLE_FMT_FLT, GAVL_SAMPLE_FLOAT },  ///< float
    { SAMPLE_FMT_DBL, GAVL_SAMPLE_DOUBLE }, ///< double
  };

gavl_sample_format_t bg_sample_format_ffmpeg_2_gavl(enum SampleFormat p)
  {
  int i;
  for(i = 0; i < sizeof(sampleformats)/sizeof(sampleformats[0]); i++)
    {
    if(sampleformats[i].ffmpeg_fmt == p)
      return sampleformats[i].gavl_fmt;
    }
  return GAVL_SAMPLE_NONE;
  }

static const struct
  {
  gavl_codec_id_t gavl;
  enum CodecID    ffmpeg;
  }
codec_ids[] =
  {
    /* Audio */
    { GAVL_CODEC_ID_ALAW,   CODEC_ID_PCM_ALAW  }, //!< alaw 2:1
    { GAVL_CODEC_ID_ULAW,   CODEC_ID_PCM_MULAW }, //!< mu-law 2:1
    { GAVL_CODEC_ID_MP2,    CODEC_ID_MP2       }, //!< MPEG-1 audio layer II
    { GAVL_CODEC_ID_MP3,    CODEC_ID_MP3       }, //!< MPEG-1/2 audio layer 3 CBR/VBR
    { GAVL_CODEC_ID_AC3,    CODEC_ID_AC3       }, //!< AC3
    { GAVL_CODEC_ID_AAC,    CODEC_ID_AAC       }, //!< AAC as stored in quicktime/mp4
    { GAVL_CODEC_ID_VORBIS, CODEC_ID_VORBIS    }, //!< Vorbis (segmented extradata and packets)
    
    /* Video */
    { GAVL_CODEC_ID_JPEG,      CODEC_ID_MJPEG      }, //!< JPEG image
    { GAVL_CODEC_ID_PNG,       CODEC_ID_PNG        }, //!< PNG image
    { GAVL_CODEC_ID_TIFF,      CODEC_ID_TIFF       }, //!< TIFF image
    { GAVL_CODEC_ID_TGA,       CODEC_ID_TARGA      }, //!< TGA image
    { GAVL_CODEC_ID_MPEG1,     CODEC_ID_MPEG1VIDEO }, //!< MPEG-1 video
    { GAVL_CODEC_ID_MPEG2,     CODEC_ID_MPEG2VIDEO }, //!< MPEG-2 video
    { GAVL_CODEC_ID_MPEG4_ASP, CODEC_ID_MPEG4      }, //!< MPEG-4 ASP (a.k.a. Divx4)
    { GAVL_CODEC_ID_H264,      CODEC_ID_H264       }, //!< H.264 (Annex B)
    { GAVL_CODEC_ID_THEORA,    CODEC_ID_THEORA     }, //!< Theora (segmented extradata
    { GAVL_CODEC_ID_DIRAC,     CODEC_ID_DIRAC      }, //!< Complete DIRAC frames, sequence end code appended to last packet
    { GAVL_CODEC_ID_DV,        CODEC_ID_DVVIDEO    }, //!< DV (several variants)
    { GAVL_CODEC_ID_NONE,      CODEC_ID_NONE       },
  };

enum CodecID bg_codec_id_gavl_2_ffmpeg(gavl_codec_id_t gavl)
  {
  int i = 0;
  while(codec_ids[i].gavl != GAVL_CODEC_ID_NONE)
    {
    if(codec_ids[i].gavl == gavl)
      return codec_ids[i].ffmpeg;
    i++;
    }
  return CODEC_ID_NONE;
  }
