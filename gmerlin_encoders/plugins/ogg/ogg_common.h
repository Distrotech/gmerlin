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

/* Generic struct for a codec. Here, we'll implement
   encoders for vorbis, theora, speex and flac */

typedef struct
  {
  char * name;
  char * long_name;
  
  void * (*create)(FILE * output, long serialno);

  const bg_parameter_info_t * (*get_parameters)();
  void (*set_parameter)(void*, const char * name, const bg_parameter_value_t * v);

  int (*init_audio)(void*, gavl_audio_format_t * format, bg_metadata_t * metadata);
  int (*init_video)(void*, gavl_video_format_t * format, bg_metadata_t * metadata);
  
  int (*flush_header_pages)(void*);
  
  int (*encode_audio)(void*, gavl_audio_frame_t*f);
  int (*encode_video)(void*, gavl_video_frame_t*f);

  int (*close)(void*);
  } bg_ogg_codec_t;

typedef struct
  {
  int num_audio_streams;
  int num_video_streams;
  
  struct
    {
    const bg_ogg_codec_t * codec;
    void           * codec_priv;
    gavl_audio_format_t format;
    } * audio_streams;

  struct
    {
    const bg_ogg_codec_t * codec;
    void           * codec_priv;
    gavl_video_format_t format;
    } * video_streams;

  FILE * output;
  long serialno;
  
  bg_metadata_t metadata;
  char * filename;
  
  bg_parameter_info_t * audio_parameters;

  } bg_ogg_encoder_t;

void * bg_ogg_encoder_create();

int bg_ogg_encoder_open(void *, const char * file,
                        bg_metadata_t * metadata,
                        bg_chapter_list_t * chapter_list);

void bg_ogg_encoder_destroy(void*);

int bg_ogg_flush_page(ogg_stream_state * os, FILE * output, int force);

int bg_ogg_flush(ogg_stream_state * os, FILE * output, int force);

int bg_ogg_encoder_add_audio_stream(void*, gavl_audio_format_t * format);
int bg_ogg_encoder_add_video_stream(void*, gavl_video_format_t * format);

void bg_ogg_encoder_init_audio_stream(void*, int stream, const bg_ogg_codec_t * codec);
void bg_ogg_encoder_init_video_stream(void*, int stream, const bg_ogg_codec_t * codec);

void bg_ogg_encoder_set_audio_parameter(void*, int stream, const char * name, const bg_parameter_value_t * val);
void bg_ogg_encoder_set_video_parameter(void*, int stream, const char * name, const bg_parameter_value_t * val);

int bg_ogg_encoder_start(void*);

void bg_ogg_encoder_get_audio_format(void * data, int stream, gavl_audio_format_t*ret);
void bg_ogg_encoder_get_video_format(void * data, int stream, gavl_video_format_t*ret);

int bg_ogg_encoder_write_audio_frame(void * data, gavl_audio_frame_t*f,int stream);
int bg_ogg_encoder_write_video_frame(void * data, gavl_video_frame_t*f,int stream);
int bg_ogg_encoder_close(void * data, int do_delete);
