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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ogg/ogg.h>

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>

#include "ogg_common.h"

#define LOG_DOMAIN "ogg"

void * bg_ogg_encoder_create()
  {
  bg_ogg_encoder_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_ogg_encoder_destroy(void * data)
  {
  int i;
  bg_ogg_encoder_t * e = data;
  if(e->write_callback_data)
    bg_ogg_encoder_close(e, 1);
  if(e->audio_streams) free(e->audio_streams);

  for(i = 0; i < e->num_video_streams; i++)
    {
    if(e->video_streams[i].stats_file)
      free(e->video_streams[i].stats_file);
    }

  if(e->video_streams) free(e->video_streams);
  if(e->filename) free(e->filename);
  
  if(e->audio_parameters)
    bg_parameter_info_destroy_array(e->audio_parameters);
  
  free(e);
  }

void bg_ogg_encoder_set_callbacks(void * data, bg_encoder_callbacks_t * cb)
  {
  bg_ogg_encoder_t * e = data;
  e->cb = cb;
  }

static int write_file(void * priv, const uint8_t * data, int len)
  {
  return fwrite(data,1,len,priv);
  }

static void close_file(void * priv)
  {
  fclose(priv);
  }

int
bg_ogg_encoder_open(void * data, const char * file,
                    const gavl_metadata_t * metadata,
                    const gavl_chapter_list_t * chapter_list,
                    const char * ext)
  {
  bg_ogg_encoder_t * e = data;

  if(file)
    {
    e->filename = bg_filename_ensure_extension(file, ext);
    
    if(!bg_encoder_cb_create_output_file(e->cb, e->filename))
      return 0;
    
    if(!(e->write_callback_data = fopen(e->filename, "w")))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open file %s: %s",
             file, strerror(errno));
      return 0;
      }
    e->write_callback = write_file;
    e->close_callback = close_file;
    }
  else if(e->open_callback)
    {
    if(!e->open_callback(e->write_callback_data))
      return 0;
    }
  
  e->serialno = rand();
  if(metadata)
    gavl_metadata_copy(&e->metadata, metadata);
  return 1;
  }

int bg_ogg_flush_page(ogg_stream_state * os, bg_ogg_encoder_t * output, int force)
  {
  int result;
  ogg_page og;
  memset(&og, 0, sizeof(og));
  if(force)
    result = ogg_stream_flush(os,&og);
  else
    result = ogg_stream_pageout(os,&og);
  
  if(result)
    {
    if((output->write_callback(output->write_callback_data,og.header,og.header_len) < og.header_len) ||
       (output->write_callback(output->write_callback_data,og.body,og.body_len) < og.body_len))
      return -1;
    else
      return 1;
    }
  return 0;
  }

int bg_ogg_flush(ogg_stream_state * os, bg_ogg_encoder_t * output, int force)
  {
  int result, ret = 0;
  while((result = bg_ogg_flush_page(os, output, force)) > 0)
    {
    ret = 1;
    }
  if(result < 0)
    return result;
  return ret;
  }

int bg_ogg_encoder_add_audio_stream(void * data, const gavl_audio_format_t * format)
  {
  bg_ogg_encoder_t * e = data;
  e->audio_streams =
    realloc(e->audio_streams, (e->num_audio_streams+1)*(sizeof(*e->audio_streams)));
  memset(e->audio_streams + e->num_audio_streams, 0, sizeof(*e->audio_streams));
  gavl_audio_format_copy(&e->audio_streams[e->num_audio_streams].format,
                         format);
  e->num_audio_streams++;
  return e->num_audio_streams-1;
  }

int bg_ogg_encoder_add_video_stream(void * data, const gavl_video_format_t * format)
  {
  bg_ogg_encoder_t * e = data;
  e->video_streams =
    realloc(e->video_streams, (e->num_video_streams+1)*(sizeof(*e->video_streams)));
  memset(e->video_streams + e->num_video_streams, 0, sizeof(*e->video_streams));
  gavl_video_format_copy(&e->video_streams[e->num_video_streams].format,
                         format);
  e->num_video_streams++;
  return e->num_video_streams-1;
  }

int bg_ogg_encoder_add_audio_stream_compressed(void * data,
                                               const gavl_audio_format_t * format,
                                               const gavl_compression_info_t * ci)
  {
  bg_ogg_encoder_t * e = data;
  e->audio_streams =
    realloc(e->audio_streams, (e->num_audio_streams+1)*(sizeof(*e->audio_streams)));
  memset(e->audio_streams + e->num_audio_streams, 0, sizeof(*e->audio_streams));
  gavl_audio_format_copy(&e->audio_streams[e->num_audio_streams].format,
                         format);
  e->audio_streams[e->num_audio_streams].ci = ci;
  e->num_audio_streams++;
  return e->num_audio_streams-1;
  }

int bg_ogg_encoder_add_video_stream_compressed(void * data,
                                               const gavl_video_format_t * format,
                                               const gavl_compression_info_t * ci)
  {
  bg_ogg_encoder_t * e = data;
  e->video_streams =
    realloc(e->video_streams, (e->num_video_streams+1)*(sizeof(*e->video_streams)));
  memset(e->video_streams + e->num_video_streams, 0, sizeof(*e->video_streams));
  gavl_video_format_copy(&e->video_streams[e->num_video_streams].format,
                         format);
  e->video_streams[e->num_video_streams].ci = ci;
  e->num_video_streams++;
  return e->num_video_streams-1;
  }

void bg_ogg_encoder_init_audio_stream(void * data, int stream, const bg_ogg_codec_t * codec)
  {
  bg_ogg_encoder_t * e = data;
  e->audio_streams[stream].codec = codec;
  e->audio_streams[stream].codec_priv =
    e->audio_streams[stream].codec->create(e, e->serialno);
  e->serialno++;
  }

void bg_ogg_encoder_init_video_stream(void * data, int stream, const bg_ogg_codec_t * codec)
  {
  bg_ogg_encoder_t * e = data;
  e->video_streams[stream].codec = codec;
  e->video_streams[stream].codec_priv =
    e->video_streams[stream].codec->create(e, e->serialno);
  e->serialno++;
  }

void
bg_ogg_encoder_set_audio_parameter(void * data, int stream,
                                   const char * name, const bg_parameter_value_t * val)
  {
  bg_ogg_encoder_t * e = data;
  e->audio_streams[stream].codec->set_parameter(e->audio_streams[stream].codec_priv, name, val);
  }

void bg_ogg_encoder_set_video_parameter(void * data, int stream,
                                        const char * name, const bg_parameter_value_t * val)
  {
  bg_ogg_encoder_t * e = data;
  e->video_streams[stream].codec->set_parameter(e->video_streams[stream].codec_priv, name, val);
  }

int bg_ogg_encoder_set_video_pass(void * data, int stream,
                                  int pass, int total_passes, const char * stats_file)
  {
  bg_ogg_encoder_t * e = data;

  e->video_streams[stream].pass = pass;
  e->video_streams[stream].total_passes = total_passes;
  e->video_streams[stream].stats_file =
    bg_strdup(e->video_streams[stream].stats_file, stats_file);
  return 1;
  }

static gavl_sink_status_t
write_audio_func(void * data, gavl_audio_frame_t * f)
  {
  bg_ogg_audio_stream_t * as = data;
  return as->codec->encode_audio(as->codec_priv, f) ? GAVL_SINK_OK : GAVL_SINK_ERROR;
  }

static gavl_sink_status_t
write_video_func(void * data, gavl_video_frame_t*f)
  {
  bg_ogg_video_stream_t * vs = data;
  return vs->codec->encode_video(vs->codec_priv, f) ? GAVL_SINK_OK : GAVL_SINK_ERROR;
  }

static int start_audio(bg_ogg_encoder_t * e, int stream)
  {
  int result;
  bg_ogg_audio_stream_t * s = &e->audio_streams[stream];

  if(s->ci)
    result =
      s->codec->init_audio_compressed(s->codec_priv, &s->format, s->ci, &e->metadata);
  else
    {
    result = s->codec->init_audio(s->codec_priv, &s->format, &e->metadata);
    if(result)
      s->sink = gavl_audio_sink_create(NULL, write_audio_func, s, &s->format);
    }
  return result;
  }

static int start_video(bg_ogg_encoder_t * e, int stream)
  {
  bg_ogg_video_stream_t * s = &e->video_streams[stream];

  if(s->ci)
    {
    if(!s->codec->init_video_compressed(s->codec_priv,
                                        &s->format,
                                        s->ci,
                                        &e->metadata))
      return 0;
    
    }
  else
    {
    if(!s->codec->init_video(s->codec_priv,
                             &s->format,
                             &e->metadata))
      return 0;
    
    if(s->pass)
      {
      if(!s->codec->set_video_pass)
        return 0;
      
      else 
        if(!s->codec->set_video_pass(s->codec_priv,
                                     s->pass,
                                     s->total_passes,
                                     s->stats_file))
          return 0;
      }

    s->sink = gavl_video_sink_create(NULL, write_video_func, s, &s->format);
    }
  
  return 1;
  }

int bg_ogg_encoder_start(void * data)
  {
  int i;
  bg_ogg_encoder_t * e = data;

  /* Start encoders and write identification headers */
  for(i = 0; i < e->num_video_streams; i++)
    {
    if(!start_video(e, i))
      return 0;
    }
  for(i = 0; i < e->num_audio_streams; i++)
    {
    if(!start_audio(e, i))
      return 0;
    }

  /* Write remaining header pages */
  for(i = 0; i < e->num_video_streams; i++)
    {
    e->video_streams[i].codec->flush_header_pages(e->video_streams[i].codec_priv);
    }
  for(i = 0; i < e->num_audio_streams; i++)
    {
    e->audio_streams[i].codec->flush_header_pages(e->audio_streams[i].codec_priv);
    }
  
  return 1;
  }

void bg_ogg_encoder_get_audio_format(void * data, int stream, gavl_audio_format_t*ret)
  {
  bg_ogg_encoder_t * e = data;
  gavl_audio_format_copy(ret, &e->audio_streams[stream].format);
  }

void bg_ogg_encoder_get_video_format(void * data, int stream, gavl_video_format_t*ret)
  {
  bg_ogg_encoder_t * e = data;
  gavl_video_format_copy(ret, &e->video_streams[stream].format);
  }

int bg_ogg_encoder_write_audio_frame(void * data, gavl_audio_frame_t*f,int stream)
  {
  bg_ogg_encoder_t * e = data;
  return gavl_audio_sink_put_frame(e->audio_streams[stream].sink, f);
  }

int bg_ogg_encoder_write_video_frame(void * data, gavl_video_frame_t*f,int stream)
  {
  bg_ogg_encoder_t * e = data;
  return gavl_video_sink_put_frame(e->video_streams[stream].sink, f);
  }

int bg_ogg_encoder_write_audio_packet(void * data, gavl_packet_t*p,int stream)
  {
  bg_ogg_encoder_t * e = data;
  return
    e->audio_streams[stream].codec->write_packet(e->audio_streams[stream].codec_priv, p);
  }

int bg_ogg_encoder_write_video_packet(void * data, gavl_packet_t*p,int stream)
  {
  bg_ogg_encoder_t * e = data;
  return
    e->video_streams[stream].codec->write_packet(e->video_streams[stream].codec_priv, p);
  
  }


int bg_ogg_encoder_close(void * data, int do_delete)
  {
  int ret = 1;
  int i;
  bg_ogg_encoder_t * e = data;

  if(!e->write_callback_data)
    return 1;
  
  for(i = 0; i < e->num_audio_streams; i++)
    {
    if(!e->audio_streams[i].codec->close(e->audio_streams[i].codec_priv))
      {
      ret = 0;
      break;
      }
    if(e->audio_streams[i].sink)
      {
      gavl_audio_sink_destroy(e->audio_streams[i].sink);
      e->audio_streams[i].sink = NULL;
      }
    }
  for(i = 0; i < e->num_video_streams; i++)
    {
    if(!e->video_streams[i].codec->close(e->video_streams[i].codec_priv))
      {
      ret = 0;
      break;
      }
    if(e->video_streams[i].sink)
      {
      gavl_video_sink_destroy(e->video_streams[i].sink);
      e->video_streams[i].sink = NULL;
      }
    }
  
  e->close_callback(e->write_callback_data);
  e->write_callback_data = NULL;
  if(do_delete && e->filename)
    remove(e->filename);
  return ret;
  }

static const bg_parameter_info_t audio_parameters[] =
  {
    {
      .name =      "codec",
      .long_name = TRS("Codec"),
      .type =      BG_PARAMETER_MULTI_MENU,
      .val_default = { .val_str = "vorbis" },
    },
    { /* End of parameters */ }
  };

bg_parameter_info_t *
bg_ogg_encoder_get_audio_parameters(bg_ogg_encoder_t * e,
                                    bg_ogg_codec_t const * const * audio_codecs)
  {
  int num_audio_codecs;
  int i;
  if(!e->audio_parameters)
    {
    num_audio_codecs = 0;
    while(audio_codecs[num_audio_codecs])
      num_audio_codecs++;
    
    e->audio_parameters = bg_parameter_info_copy_array(audio_parameters);
    e->audio_parameters[0].multi_names_nc =
      calloc(num_audio_codecs+1, sizeof(*e->audio_parameters[0].multi_names));
    e->audio_parameters[0].multi_labels_nc =
      calloc(num_audio_codecs+1, sizeof(*e->audio_parameters[0].multi_labels));
    e->audio_parameters[0].multi_parameters_nc =
      calloc(num_audio_codecs+1, sizeof(*e->audio_parameters[0].multi_parameters));
    for(i = 0; i < num_audio_codecs; i++)
      {
      e->audio_parameters[0].multi_names_nc[i]  =
        bg_strdup(NULL, audio_codecs[i]->name);
      e->audio_parameters[0].multi_labels_nc[i] =
        bg_strdup(NULL, audio_codecs[i]->long_name);
      
      if(audio_codecs[i]->get_parameters)
        e->audio_parameters[0].multi_parameters_nc[i] =
          bg_parameter_info_copy_array(audio_codecs[i]->get_parameters());
      }
    bg_parameter_info_set_const_ptrs(&e->audio_parameters[0]);
    }
  return e->audio_parameters;
  }
