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

#include <stdlib.h>
#include <string.h>

#include <config.h>

#include "ffmpeg_common.h"
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#include <gavl/metatags.h>


#define LOG_DOMAIN "ffmpeg"


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
                         const gavl_metadata_t * m)
  {
  const char * val;
  
  if((val = gavl_metadata_get(m, GAVL_META_TITLE)))
    av_metadata_set2(&priv->ctx->metadata, "title", val, 0);
  
  if((val = gavl_metadata_get(m, GAVL_META_AUTHOR)))
    av_metadata_set2(&priv->ctx->metadata, "composer", val, 0);
  
  if((val = gavl_metadata_get(m, GAVL_META_ALBUM)))
    av_metadata_set2(&priv->ctx->metadata, "album", val, 0);

  if((val = gavl_metadata_get(m, GAVL_META_COPYRIGHT)))
    av_metadata_set2(&priv->ctx->metadata, "copyright", val, 0);

  if((val = gavl_metadata_get(m, GAVL_META_COMMENT)))
    av_metadata_set2(&priv->ctx->metadata, "comment", val, 0);
  
  if((val = gavl_metadata_get(m, GAVL_META_GENRE)))
    av_metadata_set2(&priv->ctx->metadata, "genre", val, 0);
  
  if((val = gavl_metadata_get(m, GAVL_META_DATE)))
    av_metadata_set2(&priv->ctx->metadata, "date", val, 0);

  if((val = gavl_metadata_get(m, GAVL_META_TRACKNUMBER)))
    av_metadata_set2(&priv->ctx->metadata, "track", val, 0);
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

static void set_chapters(AVFormatContext * ctx,
                         const gavl_chapter_list_t * chapter_list)
  {
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51,5,0)
  int i;
  if(!chapter_list || !chapter_list->num_chapters)
    return;
  
  ctx->chapters =
    av_malloc(chapter_list->num_chapters * sizeof(*ctx->chapters));
  ctx->nb_chapters = chapter_list->num_chapters;
  
  for(i = 0; i < chapter_list->num_chapters; i++)
    {
    ctx->chapters[i] = av_mallocz(sizeof(*(ctx->chapters[i])));
    ctx->chapters[i]->time_base.num = 1;
    ctx->chapters[i]->time_base.den = chapter_list->timescale;
    ctx->chapters[i]->start = chapter_list->chapters[i].time;
    if(i < chapter_list->num_chapters - 1)
      ctx->chapters[i]->end = chapter_list->chapters[i+1].time;
    
    if(chapter_list->chapters[i].name)
      av_dict_set(&ctx->chapters[i]->metadata,
                  "title", chapter_list->chapters[i].name, 0);
    }
#else
  if(!chapter_list || !chapter_list->num_chapters)
    return;
  bg_log(BG_LOG_WARNING, LOG_DOMAIN,
         "Not writing chapters (ffmpeg version too old)");
#endif
  }

int bg_ffmpeg_open(void * data, const char * filename,
                   const gavl_metadata_t * metadata,
                   const gavl_chapter_list_t * chapter_list)
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
    set_metadata(priv, metadata);
  set_chapters(priv->ctx, chapter_list);
  
  return 1;
  }

static void set_audio_params(bg_ffmpeg_audio_stream_t * st, enum CodecID id)
  {

  /* Adjust format */
  st->format.interleave_mode = GAVL_INTERLEAVE_ALL;
  st->format.sample_format   = GAVL_SAMPLE_S16;

  /* Set format for codec */

  /* Will be cleared later if we don't write compressed
     packets */
  st->com.stream->codec->codec_id = id;
  st->com.stream->codec->sample_rate = st->format.samplerate;
  st->com.stream->codec->channels    = st->format.num_channels;
  st->com.stream->codec->codec_type  = CODEC_TYPE_AUDIO;
  }

static void set_video_params(bg_ffmpeg_video_stream_t * st, enum CodecID id)
  {
  st->com.stream->codec->codec_type = CODEC_TYPE_VIDEO;
  st->com.stream->codec->codec_id = id;
  /* Set format for codec */
  st->com.stream->codec->width  = st->format.image_width;
  st->com.stream->codec->height = st->format.image_height;
  
  st->com.stream->codec->sample_aspect_ratio.num = st->format.pixel_width;
  st->com.stream->codec->sample_aspect_ratio.den = st->format.pixel_height;
  st->com.stream->sample_aspect_ratio.num = st->format.pixel_width;
  st->com.stream->sample_aspect_ratio.den = st->format.pixel_height;
  }


int bg_ffmpeg_add_audio_stream(void * data,
                               const gavl_metadata_t * m,
                               const gavl_audio_format_t * format)
  {
  ffmpeg_priv_t * priv;
  bg_ffmpeg_audio_stream_t * st;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,10,0)
  const char *lang;
#endif  
  priv = data;
  
  priv->audio_streams =
    realloc(priv->audio_streams,
            (priv->num_audio_streams+1)*sizeof(*priv->audio_streams));
  
  st = priv->audio_streams + priv->num_audio_streams;
  memset(st, 0, sizeof(*st));
  
  gavl_audio_format_copy(&st->format, format);

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,10,0)
  st->com.stream = avformat_new_stream(priv->ctx, NULL);
  /* Set language */
  lang = gavl_metadata_get(m, GAVL_META_LANGUAGE);
  if(lang)
    av_dict_set(&st->com.stream->metadata, "language", lang, 0);
#else
  st->stream = av_new_stream(priv->ctx,
                             priv->num_audio_streams +
                             priv->num_video_streams +
                             priv->num_text_streams);
#endif 

  st->com.ffmpeg = priv;
  set_audio_params(st, CODEC_ID_NONE);
  
  priv->num_audio_streams++;
  return priv->num_audio_streams-1;
  }

int bg_ffmpeg_add_video_stream(void * data,
                               const gavl_metadata_t * m,
                               const gavl_video_format_t * format)
  {
  ffmpeg_priv_t * priv;
  bg_ffmpeg_video_stream_t * st;
  priv = data;

  priv->video_streams =
    realloc(priv->video_streams,
            (priv->num_video_streams+1)*sizeof(*priv->video_streams));
  
  st = priv->video_streams + priv->num_video_streams;
  memset(st, 0, sizeof(*st));
  
  gavl_video_format_copy(&st->format, format);

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,10,0)
  st->com.stream = avformat_new_stream(priv->ctx, NULL);
#else
  st->com.stream = av_new_stream(priv->ctx,
                                 priv->num_audio_streams +
                                 priv->num_video_streams +
                                 priv->num_text_streams);
#endif 

  /* Will be cleared later if we don't write compressed
     packets */
  set_video_params(st, CODEC_ID_NONE);
  st->com.ffmpeg = priv;
  priv->num_video_streams++;
  return priv->num_video_streams-1;
  }

/*
  int bg_ffmpeg_write_subtitle_text(void * data,const char * text,
                                  int64_t start,
                                  int64_t duration, int stream)
*/

static gavl_sink_status_t
write_text_packet_func(void * data, gavl_packet_t * p)
  {
  AVPacket pkt;
  bg_ffmpeg_text_stream_t * st = data;

  ffmpeg_priv_t * priv = st->com.ffmpeg;
  
  av_init_packet(&pkt);
  
  pkt.data     = p->data;
  pkt.size     = p->data_len + 1; // Let's hope the packet got padded!!!
  
  pkt.pts= av_rescale_q(p->pts,
                        st->com.stream->codec->time_base,
                        st->com.stream->time_base);
  
  pkt.duration= av_rescale_q(p->duration,
                             st->com.stream->codec->time_base,
                             st->com.stream->time_base);
  
  pkt.convergence_duration = pkt.duration;
  pkt.dts = pkt.pts;
  pkt.stream_index = st->com.stream->index;
  
  if(av_interleaved_write_frame(priv->ctx, &pkt) != 0)
    {
    priv->got_error = 1;
    return GAVL_SINK_ERROR;
    }
  return GAVL_SINK_OK;
  }

int bg_ffmpeg_add_text_stream(void * data,
                              const gavl_metadata_t * m,
                              uint32_t * timescale)
  {
  ffmpeg_priv_t * priv;
  bg_ffmpeg_text_stream_t * st;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,10,0)
  const char * lang;
#endif
  priv = data;

  priv->text_streams =
    realloc(priv->text_streams,
            (priv->num_text_streams+1)*sizeof(*priv->text_streams));
  
  st = priv->text_streams + priv->num_text_streams;
  memset(st, 0, sizeof(*st));

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,10,0)
  st->com.stream = avformat_new_stream(priv->ctx, NULL);

  lang = gavl_metadata_get(m, GAVL_META_LANGUAGE);
  if(lang)
    av_dict_set(&st->com.stream->metadata, "language", lang, 0);
#else
  st->stream = av_new_stream(priv->ctx,
                             priv->num_audio_streams +
                             priv->num_video_streams +
                             priv->num_text_streams);
#endif 

  st->com.stream->codec->codec_type = CODEC_TYPE_SUBTITLE;
  st->com.stream->codec->codec_id = CODEC_ID_TEXT;

  st->com.stream->codec->time_base.num = 1;
  st->com.stream->codec->time_base.den = *timescale;
  st->com.ffmpeg = priv;
  
  priv->num_text_streams++;
  return priv->num_text_streams-1;
  }

void bg_ffmpeg_set_audio_parameter(void * data, int stream, const char * name,
                                   const bg_parameter_value_t * v)
  {
  ffmpeg_priv_t * priv;
  bg_ffmpeg_audio_stream_t * st;
  priv = data;

  st = &priv->audio_streams[stream];
  
  if(!name)
    {
    /* Set the bitrate for PCM codecs */
    switch(st->com.stream->codec->codec_id)
      {
      case CODEC_ID_PCM_S16BE:
      case CODEC_ID_PCM_S16LE:
        st->com.stream->codec->bit_rate =
          st->format.samplerate * st->format.num_channels * 16;
        break;
      case CODEC_ID_PCM_S8:
      case CODEC_ID_PCM_U8:
      case CODEC_ID_PCM_ALAW:
      case CODEC_ID_PCM_MULAW:
        st->com.stream->codec->bit_rate =
          st->format.samplerate * st->format.num_channels * 8;
        break;
      default:
        break;
      }
    
    return;
    }
  
  
  if(!strcmp(name, "codec"))
    {
    enum CodecID id;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,10,0)
    AVCodec * codec;
#endif
    st->com.stream->codec->codec_type        = CODEC_TYPE_AUDIO;
    id = bg_ffmpeg_find_audio_encoder(priv->format, v->val_str);
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,10,0)
    codec = avcodec_find_encoder(id);
    if(codec)
      avcodec_get_context_defaults3(st->com.stream->codec, codec);
#endif
    set_audio_params(st, id);
    }
  else
    bg_ffmpeg_set_codec_parameter(st->com.stream->codec,
#if LIBAVCODEC_VERSION_MAJOR >= 54
                                  &st->com.options,
#endif
                                  name, v);
  
  }

void bg_ffmpeg_set_video_parameter(void * data, int stream, const char * name,
                                   const bg_parameter_value_t * v)
  {
  ffmpeg_priv_t * priv;
  bg_ffmpeg_video_stream_t * st;
  priv = data;

  if(!name)
    {
    return;
    }
  st = &priv->video_streams[stream];
  
  if(!strcmp(name, "codec"))
    {
    enum CodecID id;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,10,0)
    AVCodec * codec;
#endif
    st->com.stream->codec->codec_type = CODEC_TYPE_VIDEO;
    id = bg_ffmpeg_find_video_encoder(priv->format, v->val_str);
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,10,0)
    codec = avcodec_find_encoder(id);
    if(codec)
      avcodec_get_context_defaults3(st->com.stream->codec, codec);
#endif

    // avcodec_get_context_defaults3
    // does a memset(... , 0, ...) on the whole
    // context, so we need to set all fields again

    set_video_params(st, id);
    
    }
  else if(bg_encoder_set_framerate_parameter(&st->fr, name, v))
    {
    return;
    }
  else
    bg_ffmpeg_set_codec_parameter(st->com.stream->codec,
#if LIBAVCODEC_VERSION_MAJOR >= 54
                                  &st->com.options,
#endif
                                  name, v);
  }


int bg_ffmpeg_set_video_pass(void * data, int stream, int pass,
                             int total_passes,
                             const char * stats_filename)
  {
  ffmpeg_priv_t * priv;
  bg_ffmpeg_video_stream_t * st;
  priv = data;

  st = &priv->video_streams[stream];
  st->pass           = pass;
  st->total_passes   = total_passes;
  st->stats_filename = bg_strdup(st->stats_filename, stats_filename);

  return 1;
  }

static int64_t rescale_video_timestamp(bg_ffmpeg_video_stream_t * st,
                                       int64_t ts)
  {
  if(st->format.framerate_mode == GAVL_FRAMERATE_CONSTANT)
    return av_rescale_q(ts / st->format.frame_duration,
                        st->com.stream->codec->time_base,
                        st->com.stream->time_base);
  else
    return av_rescale_q(ts,
                        st->com.stream->codec->time_base,
                        st->com.stream->time_base);
  
  }

static int flush_video(ffmpeg_priv_t * priv, bg_ffmpeg_video_stream_t * st,
                       AVFrame * frame)
  {
  AVPacket pkt;

  int bytes_encoded = 0;
  int got_packet = 0;
  av_init_packet(&pkt);

#if ENCODE_VIDEO2
  pkt.data = st->com.gp.data;
  pkt.size = st->com.gp.data_alloc;
  
  if(avcodec_encode_video2(st->com.stream->codec, &pkt, frame, &got_packet) < 0)
    return -1;
  if(got_packet)
    bytes_encoded = pkt.size;
#else
  bytes_encoded = avcodec_encode_video(st->com.stream->codec,
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
    pkt.pts= rescale_video_timestamp(st, st->com.stream->codec->coded_frame->pts);
    
    if(st->com.stream->codec->coded_frame->key_frame)
      pkt.flags |= PKT_FLAG_KEY;
    pkt.data = st->buffer;
    pkt.size = bytes_encoded;
#else // New

    if(pkt.pts != AV_NOPTS_VALUE)
      pkt.pts=rescale_video_timestamp(st, pkt.pts);
    
    if(pkt.dts != AV_NOPTS_VALUE)
      pkt.dts=rescale_video_timestamp(st, pkt.dts);
#endif
    
    pkt.stream_index = st->com.stream->index;

    //    if(av_write_frame(priv->ctx, &pkt) != 0) 
    if(av_interleaved_write_frame(priv->ctx, &pkt) != 0)
      {
      priv->got_error = 1;
      return 0;
      }

    /* Write stats */
    
    if((st->pass == 1) && st->com.stream->codec->stats_out && st->stats_file)
      fprintf(st->stats_file, "%s", st->com.stream->codec->stats_out);
    }
  return bytes_encoded;
  }

static gavl_sink_status_t
write_video_func(void * data, gavl_video_frame_t * frame)
  {
  ffmpeg_priv_t * priv;
  bg_ffmpeg_video_stream_t * st;
  
  st = data;
  priv = st->com.ffmpeg;
  
  st->frame->pts = frame->timestamp;
  
  st->frame->data[0]     = frame->planes[0];
  st->frame->data[1]     = frame->planes[1];
  st->frame->data[2]     = frame->planes[2];
  st->frame->linesize[0] = frame->strides[0];
  st->frame->linesize[1] = frame->strides[1];
  st->frame->linesize[2] = frame->strides[2];
  
  flush_video(priv, st, st->frame);

  st->frames_written++;
  
  if(priv->got_error)
    return GAVL_SINK_ERROR;
  
  return GAVL_SINK_OK;
  }

static gavl_sink_status_t
write_video_packet_func(void * priv, gavl_packet_t * packet)
  {
  AVPacket pkt;
  bg_ffmpeg_video_stream_t * st = priv;
  ffmpeg_priv_t * f = st->com.ffmpeg;
  
  //  fprintf(stderr, "Write video packet: ");
  //  gavl_packet_dump(packet);
  
  if(packet->pts == GAVL_TIME_UNDEFINED)
    return 1; // Drop undecodable packet
  
  av_init_packet(&pkt);
  pkt.data = packet->data;
  pkt.size = packet->data_len;

  pkt.pts= rescale_video_timestamp(st, packet->pts);
    
  if(st->com.ci.flags & GAVL_COMPRESSION_HAS_B_FRAMES)
    {
    if(st->dts == GAVL_TIME_UNDEFINED)
      st->dts = packet->pts - 3*packet->duration;

    pkt.dts= rescale_video_timestamp(st, st->dts);
    st->dts += packet->duration;
    }
  else
    pkt.dts = pkt.pts;
  
  if(packet->flags & GAVL_PACKET_KEYFRAME)  
    pkt.flags |= PKT_FLAG_KEY;
  
  pkt.stream_index= st->com.stream->index;
  
  /* write the compressed frame in the media file */
  if(av_interleaved_write_frame(f->ctx, &pkt) != 0)
    {
    f->got_error = 1;
    return GAVL_SINK_ERROR;
    }

  return GAVL_SINK_OK;
  }


#if ENCODE_AUDIO2
static int flush_audio(ffmpeg_priv_t * priv,
                       bg_ffmpeg_audio_stream_t * st)
  {
  AVPacket pkt;
  AVFrame f;
  int got_packet;
  
  av_init_packet(&pkt);

  pkt.data = st->com.gp.data;
  pkt.size = st->com.gp.data_alloc;
  
  avcodec_get_frame_defaults(&f);
  f.nb_samples = st->frame->valid_samples;

  f.pts = st->samples_written;
  
  avcodec_fill_audio_frame(&f, st->format.num_channels,
                           st->com.stream->codec->sample_fmt,
                           st->frame->samples.u_8,
                           st->com.stream->codec->frame_size *
                           st->format.num_channels * 2, 1);
    
  if(avcodec_encode_audio2(st->com.stream->codec, &pkt,
                           &f, &got_packet) < 0)
    return 0;
  
  if(got_packet && pkt.size)
    {
    if(pkt.pts != AV_NOPTS_VALUE)
      pkt.pts= av_rescale_q(pkt.pts,
                            st->com.stream->codec->time_base,
                            st->com.stream->time_base);
    
    pkt.flags |= PKT_FLAG_KEY;
    pkt.stream_index= st->com.stream->index;
    
    /* write the compressed frame in the media file */
    if(av_interleaved_write_frame(priv->ctx, &pkt) != 0)
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

  if(st->com.stream->codec->frame_size <= 1)
    out_size = st->com.stream->codec->block_align * st->frame->valid_samples;
  else
    out_size = st->buffer_alloc;

  bytes_encoded = avcodec_encode_audio(st->com.stream->codec, st->buffer,
                                       out_size,
                                       st->frame->samples.s_16);
  
  if(bytes_encoded > 0)
    {
    av_init_packet(&pkt);
    pkt.size = bytes_encoded;
    
    if(st->com.stream->codec->coded_frame &&
       (st->com.stream->codec->coded_frame->pts != AV_NOPTS_VALUE))
      pkt.pts= av_rescale_q(st->com.stream->codec->coded_frame->pts,
                            st->com.stream->codec->time_base,
                            st->com.stream->time_base)  + st->pts_offset;
    
    pkt.flags |= PKT_FLAG_KEY;
    pkt.stream_index= st->com.stream->index;
    pkt.data= st->buffer;
    
    /* write the compressed frame in the media file */
    if(av_interleaved_write_frame(priv->ctx, &pkt) != 0)
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

static gavl_sink_status_t
write_audio_func(void * data, gavl_audio_frame_t * frame)
  {
  bg_ffmpeg_audio_stream_t * st;
  ffmpeg_priv_t * priv;
  int samples_written = 0;
  int samples_copied;
  st = data;
  priv = st->com.ffmpeg;
  
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
        return GAVL_SINK_ERROR;
      }
    samples_written += samples_copied;
    }
  return GAVL_SINK_OK;
  }

static gavl_sink_status_t
write_audio_packet_func(void * data, gavl_packet_t * packet)
  {
  AVPacket pkt;
  ffmpeg_priv_t * f;
  bg_ffmpeg_audio_stream_t * st = data;
  f = st->com.ffmpeg;
  
  if(packet->pts == GAVL_TIME_UNDEFINED)
    return 1; // Drop undecodable packet
  
  av_init_packet(&pkt);

  pkt.data = packet->data;
  pkt.size = packet->data_len;
    
  pkt.pts= av_rescale_q(packet->pts,
                        st->com.stream->codec->time_base,
                        st->com.stream->time_base);


  pkt.dts = pkt.pts;
  pkt.flags |= PKT_FLAG_KEY;
  pkt.stream_index= st->com.stream->index;
  
  /* write the compressed frame in the media file */
  if(av_interleaved_write_frame(f->ctx, &pkt) != 0)
    {
    f->got_error = 1;
    return GAVL_SINK_ERROR;
    }
  return GAVL_SINK_OK;
  }


static int open_audio_encoder(ffmpeg_priv_t * priv,
                              bg_ffmpeg_audio_stream_t * st)
  {
  AVCodec * codec;
  
  if(st->com.flags & STREAM_IS_COMPRESSED)
    {
    st->com.psink = gavl_packet_sink_create(NULL, write_audio_packet_func, st);
    return 1;
    }
  if(st->com.stream->codec->codec_id == CODEC_ID_NONE)
    return 0;
  
  codec = avcodec_find_encoder(st->com.stream->codec->codec_id);
  if(!codec)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Audio codec not available in your libavcodec installation");
    return 0;
    }

  st->com.stream->codec->sample_fmt = codec->sample_fmts[0];
  st->format.sample_format =
    bg_sample_format_ffmpeg_2_gavl(codec->sample_fmts[0]);

  /* Extract extradata */
  if(priv->ctx->oformat->flags & AVFMT_GLOBALHEADER)
    st->com.stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
  
#if LIBAVCODEC_VERSION_MAJOR < 54
  if(avcodec_open(st->com.stream->codec, codec) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "avcodec_open failed for audio");
    return 0;
    }
#else
  if(avcodec_open2(st->com.stream->codec, codec, &st->com.options) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "avcodec_open2 failed for audio");
    return 0;
    }
#endif
  
  if(st->com.stream->codec->frame_size <= 1)
    st->format.samples_per_frame = 1024; // Frame size for uncompressed codecs
  else
    st->format.samples_per_frame = st->com.stream->codec->frame_size;
  
  st->frame = gavl_audio_frame_create(&st->format);
  /* Mute frame */
  gavl_audio_frame_mute(st->frame, &st->format);

  gavl_packet_alloc(&st->com.gp, 32768);
  
  st->sink = gavl_audio_sink_create(NULL, write_audio_func, st, &st->format);
  
  st->com.flags |= STREAM_ENCODER_INITIALIZED;
  
  return 1;
  }

static void set_framerate(bg_ffmpeg_video_stream_t * st)
  {
  if(st->format.framerate_mode == GAVL_FRAMERATE_CONSTANT)
    {
    st->com.stream->codec->time_base.den = st->format.timescale;
    st->com.stream->codec->time_base.num = st->format.frame_duration;
    }
  else
    {
    st->com.stream->codec->time_base.den = st->format.timescale;
    st->com.stream->codec->time_base.num = 1;
    }
  }

static int open_video_encoder(ffmpeg_priv_t * priv,
                              bg_ffmpeg_video_stream_t * st)
  {
  int stats_len;
  AVCodec * codec;
  const ffmpeg_codec_info_t * ci =
    bg_ffmpeg_get_codec_info(st->com.stream->codec->codec_id,
                             CODEC_TYPE_VIDEO);
  
  if(st->com.stream->codec->codec_id == CODEC_ID_NONE)
    return 0;

  if(st->com.flags & STREAM_IS_COMPRESSED)
    {
    st->com.psink = gavl_packet_sink_create(NULL, write_video_packet_func, st);
    set_framerate(st);
    return 1;
    }
  
  codec = avcodec_find_encoder(st->com.stream->codec->codec_id);
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
      st->com.stream->codec->flags |= CODEC_FLAG_PASS1;
      }
    else if(st->pass == st->total_passes)
      {
      st->stats_file = fopen(st->stats_filename, "r");
      fseek(st->stats_file, 0, SEEK_END);
      stats_len = ftell(st->stats_file);
      fseek(st->stats_file, 0, SEEK_SET);
      
      st->com.stream->codec->stats_in = av_malloc(stats_len + 1);
      if(fread(st->com.stream->codec->stats_in, 1,
               stats_len, st->stats_file) < stats_len)
        {
        av_free(st->com.stream->codec->stats_in);
        st->com.stream->codec->stats_in = NULL;
        }
      else
        st->com.stream->codec->stats_in[stats_len] = '\0';
      
      fclose(st->stats_file);
      st->stats_file = NULL;
      
      st->com.stream->codec->flags |= CODEC_FLAG_PASS2;
      }
    }

  /* Set up pixelformat */
  
  st->com.stream->codec->pix_fmt = codec->pix_fmts[0];
  st->format.pixelformat =
    bg_pixelformat_ffmpeg_2_gavl(st->com.stream->codec->pix_fmt);
  
  /* Set up framerate */

  if((ci->flags & FLAG_CONSTANT_FRAMERATE) ||
    (priv->format->flags & FLAG_CONSTANT_FRAMERATE))
    {
    if(ci->framerates)
      bg_encoder_set_framerate_nearest(&st->fr,
                                       ci->framerates,
                                       &st->format);
    else
      bg_encoder_set_framerate(&st->fr, &st->format);
    }

  set_framerate(st);

  /* Extract extradata */
  if(priv->ctx->oformat->flags & AVFMT_GLOBALHEADER)
    st->com.stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
  
#if LIBAVCODEC_VERSION_MAJOR < 54
  if(avcodec_open(st->com.stream->codec, codec) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "avcodec_open failed for video");
    return 0;
    }
#else
  if(avcodec_open2(st->com.stream->codec, codec, &st->com.options) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "avcodec_open2 failed for video");
    return 0;
    }
#endif

  gavl_packet_alloc(&st->com.gp, st->format.image_width * st->format.image_width * 4);
  
  st->frame = avcodec_alloc_frame();

  st->sink = gavl_video_sink_create(NULL, write_video_func, st, &st->format);
  
  st->com.flags |= STREAM_ENCODER_INITIALIZED;
  
  return 1;
  }


int bg_ffmpeg_write_audio_frame(void * data,
                                gavl_audio_frame_t * frame, int stream)
  {
  ffmpeg_priv_t * priv;
  priv = data;
  return (gavl_audio_sink_put_frame(priv->audio_streams[stream].sink,
                                    frame) == GAVL_SINK_OK);
  }

int bg_ffmpeg_write_video_frame(void * data,
                                gavl_video_frame_t * frame, int stream)
  {
  ffmpeg_priv_t * priv;
  priv = data;
  return (gavl_video_sink_put_frame(priv->video_streams[stream].sink,
                                    frame) == GAVL_SINK_OK);
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

  for(i = 0; i < priv->num_text_streams ; i++)
    {
    
    priv->text_streams[i].com.psink =
      gavl_packet_sink_create(NULL, write_text_packet_func,
                              &priv->text_streams[i]);

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

gavl_audio_sink_t *
bg_ffmpeg_get_audio_sink(void * data, int stream)
  {
  ffmpeg_priv_t * priv;
  priv = data;
  return priv->audio_streams[stream].sink;
  }

gavl_video_sink_t *
bg_ffmpeg_get_video_sink(void * data, int stream)
  {
  ffmpeg_priv_t * priv;
  priv = data;
  return priv->video_streams[stream].sink;
  }

int bg_ffmpeg_write_subtitle_text(void * data,const char * text,
                                  int64_t start,
                                  int64_t duration, int stream)
  {
  gavl_packet_t pkt;
  ffmpeg_priv_t * priv;
  bg_ffmpeg_text_stream_t * st;
  int ret;
  int len;
  
  priv = data;
  st = &priv->text_streams[stream];

  gavl_packet_init(&pkt);

  len = strlen(text);

  gavl_packet_alloc(&pkt, len);
  memcpy(pkt.data, text, len);
  pkt.data_len = len;
  
  pkt.pts      = start;
  pkt.duration = duration;

  ret = (gavl_packet_sink_put_packet(st->com.psink, &pkt) == GAVL_SINK_OK);
  gavl_packet_free(&pkt);
  return ret;
  }

static void flush_audio_encoder(ffmpeg_priv_t * priv,
                                bg_ffmpeg_audio_stream_t * st)
  {
  /* Flush */
  if(st->frame && st->frame->valid_samples && priv->initialized)
    {
    if(!flush_audio(priv, st))
      return;
    }
  }

static void close_common(bg_ffmpeg_stream_common_t * com)
  {
  if((com->flags & STREAM_ENCODER_INITIALIZED) && (com->stream))
    {
    avcodec_close(com->stream->codec);
    }
  gavl_packet_free(&com->gp);
  }


static int close_audio_encoder(ffmpeg_priv_t * priv,
                               bg_ffmpeg_audio_stream_t * st)
  {
  close_common(&st->com);
  /* Close encoder and free buffers */

#ifndef AVFORMAT_FREE_CONTEXT  
  av_free(st->com.stream->codec);
#endif
  
  //  st->com.stream->codec = NULL;
  
  if(st->frame)
    gavl_audio_frame_destroy(st->frame);

  if(st->sink)
    gavl_audio_sink_destroy(st->sink);
  
  return 1;
  }

static int close_text_encoder(ffmpeg_priv_t * priv,
                              bg_ffmpeg_text_stream_t * st)
  {
  close_common(&st->com);
  return 1;
  }

static void flush_video_encoder(ffmpeg_priv_t * priv,
                                bg_ffmpeg_video_stream_t * st)
  {
  int result;

  if(st->com.flags & STREAM_IS_COMPRESSED)
    return;
  
  if(st->com.flags & STREAM_ENCODER_INITIALIZED)
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
                                bg_ffmpeg_video_stream_t * st)
  {
  close_common(&st->com);
  
  if(st->com.stream->codec->stats_in)
    {
    free(st->com.stream->codec->stats_in);
    st->com.stream->codec->stats_in = NULL;
    }
  if(st->com.flags & STREAM_ENCODER_INITIALIZED)
    avcodec_close(st->com.stream->codec);
  
  /* Close encoder and free buffers */
#ifndef AVFORMAT_FREE_CONTEXT  
  av_free(st->com.stream->codec);
#endif
  
  //  st->com.stream->codec = NULL;
  
  if(st->frame)
    free(st->frame);

  if(st->stats_filename)
    free(st->stats_filename);
  
  if(st->stats_file)
    fclose(st->stats_file);

  if(st->sink)
    gavl_video_sink_destroy(st->sink);
  }

int bg_ffmpeg_close(void * data, int do_delete)
  {
  ffmpeg_priv_t * priv;
  int i;
  priv = data;

  // Flush the streams

  for(i = 0; i < priv->num_audio_streams; i++)
    flush_audio_encoder(priv, &priv->audio_streams[i]);
  
  for(i = 0; i < priv->num_video_streams; i++)
    flush_video_encoder(priv, &priv->video_streams[i]);
  
  if(priv->initialized)
    {
    av_write_trailer(priv->ctx);
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 105, 0)  
    url_fclose(priv->ctx->pb);
#else
    avio_close(priv->ctx->pb);
#endif
    }

  // Close the encoders

  for(i = 0; i < priv->num_audio_streams; i++)
    {
    bg_ffmpeg_audio_stream_t * st = &priv->audio_streams[i];
    close_audio_encoder(priv, st);
    if(st->com.psink)
      gavl_packet_sink_destroy(st->com.psink);
    }
  for(i = 0; i < priv->num_video_streams; i++)
    {
    bg_ffmpeg_video_stream_t * st = &priv->video_streams[i];
    close_video_encoder(priv, st);
    if(st->com.psink)
      gavl_packet_sink_destroy(st->com.psink);
    }
  for(i = 0; i < priv->num_text_streams; i++)
    {
    bg_ffmpeg_text_stream_t * st = &priv->text_streams[i];
    close_text_encoder(priv, st);
    if(st->com.psink)
      gavl_packet_sink_destroy(st->com.psink);
    }
  if(do_delete)
    remove(priv->ctx->filename);
  
#if AVFORMAT_FREE_CONTEXT  
  avformat_free_context(priv->ctx);
#else
  av_free(priv->ctx);
#endif


  priv->ctx = NULL;
 
  
  return 1;
  }

int bg_ffmpeg_writes_compressed_audio(void * priv,
                                      const gavl_audio_format_t * format,
                                      const gavl_compression_info_t * info)
  {
  int i;
  enum CodecID ffmpeg_id;
  ffmpeg_priv_t * f = priv;
  
  ffmpeg_id = bg_codec_id_gavl_2_ffmpeg(info->id);
  
  i = 0;
  while(f->format->audio_codecs[i] != CODEC_ID_NONE)
    {
    if(f->format->audio_codecs[i] == ffmpeg_id)
      return 1;
    i++;
    }
  
  return 0;
  }

int bg_ffmpeg_writes_compressed_video(void * priv,
                                      const gavl_video_format_t * format,
                                      const gavl_compression_info_t * info)
  {
  int i;
  enum CodecID ffmpeg_id;
  ffmpeg_priv_t * f = priv;

  ffmpeg_id = bg_codec_id_gavl_2_ffmpeg(info->id);

  i = 0;
  while(f->format->video_codecs[i] != CODEC_ID_NONE)
    {
    if(f->format->video_codecs[i] == ffmpeg_id)
      return 1;
    i++;
    }
  
  return 0;
  }

int
bg_ffmpeg_add_audio_stream_compressed(void * priv,
                                      const gavl_metadata_t * m,
                                      const gavl_audio_format_t * format,
                                      const gavl_compression_info_t * info)
  {
  int ret;
  bg_ffmpeg_audio_stream_t * st;
  ffmpeg_priv_t * f = priv;

  ret = bg_ffmpeg_add_audio_stream(priv, m, format);
  st = f->audio_streams + ret;

  gavl_compression_info_copy(&st->com.ci, info);
  st->com.flags |= STREAM_IS_COMPRESSED;
    
  st->com.stream->codec->codec_id = bg_codec_id_gavl_2_ffmpeg(st->com.ci.id);

  st->com.stream->codec->time_base.num = 1;
  st->com.stream->codec->time_base.den = st->format.samplerate;
  if(st->com.ci.bitrate)
    {
    st->com.stream->codec->bit_rate = st->com.ci.bitrate;
    st->com.stream->codec->rc_max_rate = st->com.ci.bitrate;
    }
  return ret;
  }

int bg_ffmpeg_add_video_stream_compressed(void * priv,
                                          const gavl_metadata_t * m,
                                          const gavl_video_format_t * format,
                                          const gavl_compression_info_t * info)
  {
  int ret;
  bg_ffmpeg_video_stream_t * st;
  ffmpeg_priv_t * f = priv;

  ret = bg_ffmpeg_add_video_stream(priv, m, format);
  st = f->video_streams + ret;

  gavl_compression_info_copy(&st->com.ci, info);
  st->com.flags |= STREAM_IS_COMPRESSED;
  st->com.stream->codec->codec_id = bg_codec_id_gavl_2_ffmpeg(st->com.ci.id);
  st->dts = GAVL_TIME_UNDEFINED;

  if(st->com.ci.bitrate)
    {
    st->com.stream->codec->rc_max_rate = st->com.ci.bitrate;
    st->com.stream->codec->bit_rate = st->com.ci.bitrate;
    }
  /* Set extradata */
  if(st->com.ci.global_header_len)
    {
    st->com.stream->codec->extradata_size = st->com.ci.global_header_len;
    st->com.stream->codec->extradata =
      av_malloc(st->com.stream->codec->extradata_size);
    memcpy(st->com.stream->codec->extradata,
           st->com.ci.global_header,
           st->com.ci.global_header_len);
    st->com.stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
  return ret;
  }


int bg_ffmpeg_write_audio_packet(void * priv,
                                 gavl_packet_t * packet, int stream)
  {
  ffmpeg_priv_t * f = priv;
  bg_ffmpeg_audio_stream_t * st = f->audio_streams + stream;
  return gavl_packet_sink_put_packet(st->com.psink, packet) == GAVL_SINK_OK;
  }

int bg_ffmpeg_write_video_packet(void * priv, gavl_packet_t * packet,
                                 int stream)
  {
  ffmpeg_priv_t * f = priv;
  bg_ffmpeg_video_stream_t * st = f->video_streams + stream;
  return gavl_packet_sink_put_packet(st->com.psink, packet) == GAVL_SINK_OK;
  }

gavl_packet_sink_t *
bg_ffmpeg_get_audio_packet_sink(void * data, int stream)
  {
  ffmpeg_priv_t * f = data;
  return f->audio_streams[stream].com.psink;
  }

gavl_packet_sink_t *
bg_ffmpeg_get_video_packet_sink(void * data, int stream)
  {
  ffmpeg_priv_t * f = data;
  return f->video_streams[stream].com.psink;
  }

gavl_packet_sink_t *
bg_ffmpeg_get_text_packet_sink(void * data, int stream)
  {
  ffmpeg_priv_t * f = data;
  return f->text_streams[stream].com.psink;
  }
