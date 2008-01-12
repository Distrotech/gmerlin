/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

#include "ffmpeg_common.h"
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "ffmpeg"

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

void bg_ffmpeg_destroy(void * data)
  {
  ffmpeg_priv_t * priv;
  priv = (ffmpeg_priv_t *)data;

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
  priv = (ffmpeg_priv_t *)data;
  return priv->parameters;
  }

const bg_parameter_info_t * bg_ffmpeg_get_audio_parameters(void * data)
  {
  ffmpeg_priv_t * priv;
  priv = (ffmpeg_priv_t *)data;
  return priv->audio_parameters;
  }

const bg_parameter_info_t * bg_ffmpeg_get_video_parameters(void * data)
  {
  ffmpeg_priv_t * priv;
  priv = (ffmpeg_priv_t *)data;
  return priv->video_parameters;
  }

void bg_ffmpeg_set_parameter(void * data, const char * name,
                             const bg_parameter_value_t * v)
  {
  int i;
  ffmpeg_priv_t * priv;
  priv = (ffmpeg_priv_t *)data;
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

const char * bg_ffmpeg_get_extension(void * data)
  {
  ffmpeg_priv_t * priv;
  priv = (ffmpeg_priv_t *)data;

  if(priv->format)
    return priv->format->extension;
  return (const char *)0;
  }

int bg_ffmpeg_open(void * data, const char * filename,
                   bg_metadata_t * metadata,
                   bg_chapter_list_t * chapter_list)
  {
  ffmpeg_priv_t * priv;
  AVOutputFormat *fmt;

  priv = (ffmpeg_priv_t *)data;
  if(!priv->format)
    return 0;

  /* Initialize format context */
  fmt = guess_format(priv->format->short_name, (char*)0, (char*)0);
  if(!fmt)
    return 0;
  
  priv->ctx = av_alloc_format_context();
  priv->ctx->oformat = fmt;
  snprintf(priv->ctx->filename,
           sizeof(priv->ctx->filename), "%s", filename);

  /* Add metadata */

  if(metadata)
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
  
  return 1;
  }

int bg_ffmpeg_add_audio_stream(void * data, const char * language,
                               gavl_audio_format_t * format)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_audio_stream_t * st;
  
  priv = (ffmpeg_priv_t *)data;
  
  priv->audio_streams =
    realloc(priv->audio_streams,
            (priv->num_audio_streams+1)*sizeof(*priv->audio_streams));
  
  st = priv->audio_streams + priv->num_audio_streams;
  memset(st, 0, sizeof(*st));
  
  gavl_audio_format_copy(&st->format, format);

  st->stream = av_new_stream(priv->ctx,
                             priv->num_audio_streams +
                             priv->num_video_streams);
  
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

int bg_ffmpeg_add_video_stream(void * data, gavl_video_format_t * format)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_video_stream_t * st;
  priv = (ffmpeg_priv_t *)data;

  priv->video_streams =
    realloc(priv->video_streams,
            (priv->num_video_streams+1)*sizeof(*priv->video_streams));
  
  st = priv->video_streams + priv->num_video_streams;
  memset(st, 0, sizeof(*st));
  
  gavl_video_format_copy(&st->format, format);

  st->stream = av_new_stream(priv->ctx,
                             priv->num_audio_streams +
                             priv->num_video_streams);
  
  st->stream->codec->codec_type = CODEC_TYPE_VIDEO;
  st->stream->codec->pix_fmt = PIX_FMT_YUV420P;
  
  /* Adjust format */
  st->format.framerate_mode = GAVL_FRAMERATE_CONSTANT;
  st->format.pixelformat    = GAVL_YUV_420_P;
  
  /* Set format for codec */
  st->stream->codec->width  = st->format.image_width;
  st->stream->codec->height = st->format.image_height;

  st->stream->codec->time_base.den = st->format.timescale;
  st->stream->codec->time_base.num = st->format.frame_duration;
  st->stream->codec->sample_aspect_ratio.num = st->format.pixel_width;
  st->stream->codec->sample_aspect_ratio.den = st->format.pixel_height;
  
  
  priv->num_video_streams++;
  return priv->num_video_streams-1;
  }

void bg_ffmpeg_set_audio_parameter(void * data, int stream, const char * name,
                                   const bg_parameter_value_t * v)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_audio_stream_t * st;
  priv = (ffmpeg_priv_t *)data;

  if(!name)
    return;

  st = &priv->audio_streams[stream];
  
  if(!strcmp(name, "codec"))
    st->stream->codec->codec_id = bg_ffmpeg_find_audio_encoder(v->val_str);
  else
    bg_ffmpeg_set_codec_parameter(st->stream->codec, name, v);
  
  }


void bg_ffmpeg_set_video_parameter(void * data, int stream, const char * name,
                                   const bg_parameter_value_t * v)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_video_stream_t * st;
  priv = (ffmpeg_priv_t *)data;

  if(!name)
    return;

  st = &priv->video_streams[stream];
  
  if(!strcmp(name, "codec"))
    st->stream->codec->codec_id = bg_ffmpeg_find_video_encoder(v->val_str);
  else
    bg_ffmpeg_set_codec_parameter(st->stream->codec, name, v);
  
  }


int bg_ffmpeg_set_video_pass(void * data, int stream, int pass,
                             int total_passes,
                             const char * stats_filename)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_video_stream_t * st;
  priv = (ffmpeg_priv_t *)data;

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
  codec = avcodec_find_encoder(st->stream->codec->codec_id);
  if(!codec)
    return 0;
  
  if(avcodec_open(st->stream->codec, codec) < 0)
    return 0;

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
  codec = avcodec_find_encoder(st->stream->codec->codec_id);
  if(!codec)
    return 0;

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
      fread(st->stream->codec->stats_in, stats_len, 1, st->stats_file);
      st->stream->codec->stats_in[stats_len] = '\0';
      
      fclose(st->stats_file);
      st->stats_file = (FILE*)0;
      
      st->stream->codec->flags |= CODEC_FLAG_PASS2;
      }
    }
  
  if(avcodec_open(st->stream->codec, codec) < 0)
    return 0;

  st->buffer_alloc = st->format.image_width * st->format.image_width * 4;
  st->buffer = malloc(st->buffer_alloc);
  st->frame = avcodec_alloc_frame();
  st->initialized = 1;
  
  return 1;
  }

int bg_ffmpeg_start(void * data)
  {
  ffmpeg_priv_t * priv;
  int i, j;
  priv = (ffmpeg_priv_t *)data;

  /* Check if all codecs are supported by the format */

  for(i = 0; i < priv->num_audio_streams; i++)
    {
    j = 0;

    while(priv->format->audio_codecs[j] != CODEC_ID_NONE)
      {
      if(priv->format->audio_codecs[j] ==
         priv->audio_streams[i].stream->codec->codec_id)
        break;
      j++;
      }
    if(priv->format->audio_codecs[j] == CODEC_ID_NONE)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Audio codec is not compatible with format");
      return 0;
      }
    }
  for(i = 0; i < priv->num_video_streams; i++)
    {
    j = 0;

    while(priv->format->video_codecs[j] != CODEC_ID_NONE)
      {
      if(priv->format->video_codecs[j] ==
         priv->video_streams[i].stream->codec->codec_id)
        break;
      j++;
      }
    if(priv->format->video_codecs[j] == CODEC_ID_NONE)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Video codec is not compatible with format");
      return 0;
      }
    }
  
  /* set the output parameters (must be done even if no
     parameters). */
  if(av_set_parameters(priv->ctx, NULL) < 0)
    {
    return 0;
    }

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
  
  /* Open file */
  if(url_fopen(&priv->ctx->pb, priv->ctx->filename, URL_WRONLY) < 0)
    {
    return 0;
    }
  
  av_write_header(priv->ctx);
  priv->initialized = 1;
  return 1;
  }

void bg_ffmpeg_get_audio_format(void * data, int stream,
                                gavl_audio_format_t*ret)
  {
  ffmpeg_priv_t * priv;
  priv = (ffmpeg_priv_t *)data;

  gavl_audio_format_copy(ret, &priv->audio_streams[stream].format);
                         
  }

void bg_ffmpeg_get_video_format(void * data, int stream,
                                gavl_video_format_t*ret)
  {
  ffmpeg_priv_t * priv;
  priv = (ffmpeg_priv_t *)data;

  gavl_video_format_copy(ret, &priv->video_streams[stream].format);
  
  }

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
    if(av_write_frame(priv->ctx, &pkt) != 0)
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

int bg_ffmpeg_write_audio_frame(void * data,
                                gavl_audio_frame_t * frame, int stream)
  {
  ffmpeg_audio_stream_t * st;
  ffmpeg_priv_t * priv;
  int samples_written = 0;
  int samples_copied;
  priv = (ffmpeg_priv_t *)data;
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
  int bytes_encoded;

  bytes_encoded = avcodec_encode_video(st->stream->codec,
                                       st->buffer, st->buffer_alloc,
                                       frame);

  if(bytes_encoded > 0)
    {
    av_init_packet(&pkt);
    pkt.pts= av_rescale_q(st->stream->codec->coded_frame->pts,
                          st->stream->codec->time_base,
                          st->stream->time_base);
    
    if(st->stream->codec->coded_frame->key_frame)
      pkt.flags |= PKT_FLAG_KEY;
    pkt.stream_index = st->stream->index;
    pkt.data = st->buffer;
    pkt.size = bytes_encoded;

    
    if(av_write_frame(priv->ctx, &pkt) != 0)
      {
      priv->got_error = 1;
      return 0;
      }

    /* Write stats */
    
    if((st->pass == 1) && st->stream->codec->stats_out && st->stats_file)
      fprintf(st->stats_file, st->stream->codec->stats_out);
    }
  return bytes_encoded;
  }
               
                

int bg_ffmpeg_write_video_frame(void * data,
                                gavl_video_frame_t * frame, int stream)
  {
  ffmpeg_priv_t * priv;
  ffmpeg_video_stream_t * st;
  
  priv = (ffmpeg_priv_t *)data;
  st = &(priv->video_streams[stream]);
    
  st->frame->data[0]     = frame->planes[0];
  st->frame->data[1]     = frame->planes[1];
  st->frame->data[2]     = frame->planes[2];
  st->frame->linesize[0] = frame->strides[0];
  st->frame->linesize[1] = frame->strides[1];
  st->frame->linesize[2] = frame->strides[2];
  
  flush_video(priv, st, st->frame);
  
  if(priv->got_error)
    return 0;
  
  return 1;
  }

static int close_audio_encoder(ffmpeg_priv_t * priv,
                               ffmpeg_audio_stream_t * st)
  {
  /* Flush */
  if(st->frame && st->frame->valid_samples && priv->initialized)
    {
    if(!flush_audio(priv, st))
      return 0;
    }
  
  /* Close encoder and free buffers */
  if(st->initialized)
    avcodec_close(st->stream->codec);
  else
    av_free(st->stream->codec);

  if(st->buffer)
    free(st->buffer);
  if(st->frame)
    gavl_audio_frame_destroy(st->frame);
  
  return 1;
  }

static void close_video_encoder(ffmpeg_priv_t * priv,
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
  
  /* Close encoder and free buffers */
  if(st->initialized)
    avcodec_close(st->stream->codec);
  else
    av_free(st->stream->codec);
  
  if(st->frame)
    free(st->frame);
  
  if(st->buffer)
    free(st->buffer);
  
  if(st->stream->codec->stats_in)
    free(st->stream->codec->stats_in);

  if(st->stats_filename)
    free(st->stats_filename);
  
  if(st->stats_file)
    fclose(st->stats_file);
  
  }

int bg_ffmpeg_close(void * data, int do_delete)
  {
  ffmpeg_priv_t * priv;
  int i;
  priv = (ffmpeg_priv_t *)data;

  // Flush and close the streams

  for(i = 0; i < priv->num_audio_streams; i++)
    {
    close_audio_encoder(priv, &priv->audio_streams[i]);
    }

  for(i = 0; i < priv->num_video_streams; i++)
    {
    close_video_encoder(priv, &priv->video_streams[i]);
    }

  if(priv->initialized)
    {
    av_write_trailer(priv->ctx);
#if LIBAVFORMAT_VERSION_INT < ((52<<16)+(0<<8)+0)
    url_fclose(&priv->ctx->pb);
#else
    url_fclose(priv->ctx->pb);
#endif
    }
  if(do_delete)
    remove(priv->ctx->filename);
  
  return 1;
  }
