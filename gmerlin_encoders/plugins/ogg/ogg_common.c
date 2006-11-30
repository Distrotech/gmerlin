#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ogg/ogg.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include "ogg_common.h"

void * bg_ogg_encoder_create()
  {
  bg_ogg_encoder_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_ogg_encoder_destroy(void * data)
  {
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;
  if(e->output)
    bg_ogg_encoder_close(e, 1);
  if(e->audio_streams) free(e->audio_streams);
  if(e->video_streams) free(e->video_streams);
  if(e->filename) free(e->filename);

  if(e->audio_parameters)
    bg_parameter_info_destroy_array(e->audio_parameters);
  
  free(e);
  }

int bg_ogg_encoder_open(void * data, const char * file, bg_metadata_t * metadata)
  {
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;

  e->filename = bg_strdup(e->filename, file);
  
  if(!(e->output = fopen(file, "w")))
    return 0;
  e->serialno = rand();
  bg_metadata_copy(&(e->metadata), metadata);
  return 1;
  }

int bg_ogg_flush_page(ogg_stream_state * os, FILE * output, int force)
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
    fwrite(og.header,1,og.header_len,output);
    fwrite(og.body,1,og.body_len,output);
    return 1;
    }
  return 0;
  }

void bg_ogg_flush(ogg_stream_state * os, FILE * output, int force)
  {
  while(bg_ogg_flush_page(os, output, force))
    ;
  }

int bg_ogg_encoder_add_audio_stream(void * data, gavl_audio_format_t * format)
  {
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;
  e->audio_streams =
    realloc(e->audio_streams, (e->num_audio_streams+1)*(sizeof(*e->audio_streams)));
  memset(e->audio_streams + e->num_audio_streams, 0, sizeof(*e->audio_streams));
  gavl_audio_format_copy(&e->audio_streams[e->num_audio_streams].format,
                         format);
  e->num_audio_streams++;
  return e->num_audio_streams-1;
  }

int bg_ogg_encoder_add_video_stream(void * data, gavl_video_format_t * format)
  {
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;
  e->video_streams =
    realloc(e->video_streams, (e->num_video_streams+1)*(sizeof(*e->video_streams)));
  memset(e->video_streams + e->num_video_streams, 0, sizeof(*e->video_streams));
  gavl_video_format_copy(&e->video_streams[e->num_video_streams].format,
                         format);
  e->num_video_streams++;
  return e->num_video_streams-1;
  }

void bg_ogg_encoder_init_audio_stream(void * data, int stream, bg_ogg_codec_t * codec)
  {
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;
  e->audio_streams[stream].codec = codec;
  e->audio_streams[stream].codec_priv = e->audio_streams[stream].codec->create(e->output, e->serialno);
  e->serialno++;
  }

void bg_ogg_encoder_init_video_stream(void * data, int stream, bg_ogg_codec_t * codec)
  {
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;
  e->video_streams[stream].codec = codec;
  e->video_streams[stream].codec_priv = e->video_streams[stream].codec->create(e->output, e->serialno);
  e->serialno++;
  }

void bg_ogg_encoder_set_audio_parameter(void * data, int stream, char * name, bg_parameter_value_t * val)
  {
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;
  e->audio_streams[stream].codec->set_parameter(e->audio_streams[stream].codec_priv, name, val);
  }

void bg_ogg_encoder_set_video_parameter(void * data, int stream, char * name, bg_parameter_value_t * val)
  {
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;
  e->video_streams[stream].codec->set_parameter(e->video_streams[stream].codec_priv, name, val);
  }

static int start_audio(bg_ogg_encoder_t * e, int stream)
  {
  if(!e->audio_streams[stream].codec->init_audio(e->audio_streams[stream].codec_priv,
                                                 &(e->audio_streams[stream].format),
                                                 &(e->metadata)))
    return 0;
  return 1;
  }

static int start_video(bg_ogg_encoder_t * e, int stream)
  {
  if(!e->video_streams[stream].codec->init_video(e->video_streams[stream].codec_priv,
                                                 &(e->video_streams[stream].format),
                                                 &(e->metadata)))
    return 0;
  return 1;
  }

int bg_ogg_encoder_start(void * data)
  {
  int i;
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;

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
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;
  gavl_audio_format_copy(ret, &e->audio_streams[stream].format);
  }

void bg_ogg_encoder_get_video_format(void * data, int stream, gavl_video_format_t*ret)
  {
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;
  gavl_video_format_copy(ret, &e->video_streams[stream].format);
  }

void bg_ogg_encoder_write_audio_frame(void * data, gavl_audio_frame_t*f,int stream)
  {
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;
  e->audio_streams[stream].codec->encode_audio(e->audio_streams[stream].codec_priv, f);
  }

void bg_ogg_encoder_write_video_frame(void * data, gavl_video_frame_t*f,int stream)
  {
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;
  e->video_streams[stream].codec->encode_video(e->video_streams[stream].codec_priv, f);
  }

void bg_ogg_encoder_close(void * data, int do_delete)
  {
  int i;
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;

  if(!e->output)
    return;
  
  for(i = 0; i < e->num_audio_streams; i++)
    e->audio_streams[i].codec->close(e->audio_streams[i].codec_priv);
  for(i = 0; i < e->num_video_streams; i++)
    e->video_streams[i].codec->close(e->video_streams[i].codec_priv);
  
  fclose(e->output);
  e->output = (FILE*)0;
  if(do_delete)
    remove(e->filename);
  }
