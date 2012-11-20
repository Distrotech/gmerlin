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

#include <ogg/ogg.h>

/* Generic struct for a codec. Here, we'll implement
   encoders for vorbis, theora, speex and flac */

typedef struct bg_ogg_encoder_s bg_ogg_encoder_t;
typedef struct bg_ogg_stream_s bg_ogg_stream_t;

#define STREAM_FORCE_FLUSH (1<<0)
#define STREAM_COMPRESSED  (1<<1)

typedef struct
  {
  char * name;
  char * long_name;
  
  void * (*create)(void);
  
  const bg_parameter_info_t * (*get_parameters)(void);
  void (*set_parameter)(void*, const char * name, const bg_parameter_value_t * v);

  gavl_audio_sink_t * (*init_audio)(void*, gavl_compression_info_t * ci_ret,
                                    gavl_audio_format_t * format,
                                    gavl_metadata_t * stream_metadata);

  gavl_video_sink_t * (*init_video)(void*, gavl_compression_info_t * ci_ret,
                                    gavl_video_format_t * format,
                                    gavl_metadata_t * stream_metadata);

  int (*init_audio_compressed)(bg_ogg_stream_t * s);
  int (*init_video_compressed)(bg_ogg_stream_t * s);
  
  int (*set_video_pass)(void*, int pass, int total_passes,
                        const char * stats_file);
  
  int (*flush_header_pages)(void*);

  void (*convert_packet)(bg_ogg_stream_t * s, gavl_packet_t * src, ogg_packet * dst);

  void (*set_packet_sink)(void*, gavl_packet_sink_t * psink);
  
  int (*close)(void*);
  } bg_ogg_codec_t;

struct bg_ogg_stream_s
  {
  bg_ogg_encoder_t * enc;
  
  const bg_ogg_codec_t * codec;
  void           * codec_priv;
  gavl_audio_format_t afmt;
  gavl_video_format_t vfmt;
  
  gavl_compression_info_t ci;

  gavl_audio_sink_t * asink;
  gavl_video_sink_t * vsink;

  gavl_packet_sink_t * psink_in;
  gavl_packet_sink_t * psink_out;
  
  ogg_stream_state os;

  int flags;
  
  /* 2-pass stuff */
  int pass;
  int total_passes;
  char * stats_file;

  /* Counter for header packets */
  int num_headers;

  /* Counter for packets */
  int64_t packetno;
  
  int index;

  /* Last packet */
  gavl_packet_t last_packet;

  /* Metadata */

  const gavl_metadata_t * m_global;
  gavl_metadata_t m_stream;

  };

int bg_ogg_stream_write_header_packet(bg_ogg_stream_t * s, ogg_packet * p);

int bg_ogg_stream_write_gavl_packet(bg_ogg_stream_t * s, gavl_packet_t * p);

int bg_ogg_stream_flush(bg_ogg_stream_t * s, int force);

void bg_ogg_packet_to_gavl(ogg_packet * src, gavl_packet_t * dst, int64_t * pts);

void bg_ogg_packet_from_gavl(bg_ogg_stream_t * s, gavl_packet_t * src, ogg_packet * dst);

struct bg_ogg_encoder_s
  {
  int num_audio_streams;
  int num_video_streams;
  
  bg_ogg_stream_t * audio_streams;
  bg_ogg_stream_t * video_streams;

  long serialno;
  
  gavl_metadata_t metadata;
  char * filename;
  
  bg_parameter_info_t * audio_parameters;
  bg_parameter_info_t * video_parameters;

  bg_encoder_callbacks_t * cb;

  void * write_callback_data;
  int (*write_callback)(void * priv, const uint8_t * data, int len);
  void (*close_callback)(void * priv);
  int (*open_callback)(void * priv);
  
  };

void * bg_ogg_encoder_create(void);

void bg_ogg_encoder_set_callbacks(void *, bg_encoder_callbacks_t * cb);


int bg_ogg_encoder_open(void *, const char * file,
                        const gavl_metadata_t * metadata,
                        const gavl_chapter_list_t * chapter_list,
                        const char * ext);

void bg_ogg_encoder_destroy(void*);

//int bg_ogg_flush_page(ogg_stream_state * os, bg_ogg_encoder_t * output, int force);
int bg_ogg_flush(ogg_stream_state * os, bg_ogg_encoder_t * output, int force);

bg_ogg_stream_t *
bg_ogg_encoder_add_audio_stream(void*, const gavl_metadata_t * m,
                                const gavl_audio_format_t * format);

bg_ogg_stream_t *
bg_ogg_encoder_add_video_stream(void*, const gavl_metadata_t * m,
                                const gavl_video_format_t * format);

bg_ogg_stream_t * 
bg_ogg_encoder_add_audio_stream_compressed(void*, const gavl_metadata_t * m,
                                           const gavl_audio_format_t * format,
                                           const gavl_compression_info_t * ci);

bg_ogg_stream_t * 
bg_ogg_encoder_add_video_stream_compressed(void*, const gavl_metadata_t * m,
                                           const gavl_video_format_t * format,
                                           const gavl_compression_info_t * ci);

void
bg_ogg_encoder_init_stream(void*, bg_ogg_stream_t * s,
                           const bg_ogg_codec_t * codec);

void bg_ogg_encoder_set_audio_parameter(void*, int stream,
                                        const char * name,
                                        const bg_parameter_value_t * val);

void bg_ogg_encoder_set_video_parameter(void*, int stream,
                                        const char * name,
                                        const bg_parameter_value_t * val);

int bg_ogg_encoder_set_video_pass(void*, int stream,
                                  int pass, int total_passes,
                                  const char * stats_file);


int bg_ogg_encoder_start(void*);

void bg_ogg_encoder_get_audio_format(void * data, int stream,
                                     gavl_audio_format_t*ret);
void bg_ogg_encoder_get_video_format(void * data, int stream,
                                     gavl_video_format_t*ret);

gavl_audio_sink_t *
bg_ogg_encoder_get_audio_sink(void * data, int stream);

gavl_packet_sink_t *
bg_ogg_encoder_get_audio_packet_sink(void * data, int stream);

gavl_video_sink_t *
bg_ogg_encoder_get_video_sink(void * data, int stream);

gavl_packet_sink_t *
bg_ogg_encoder_get_video_packet_sink(void * data, int stream);

int bg_ogg_encoder_write_audio_frame(void * data,
                                     gavl_audio_frame_t*f,int stream);

int bg_ogg_encoder_write_video_frame(void * data,
                                     gavl_video_frame_t*f,int stream);

int bg_ogg_encoder_write_audio_packet(void * data,
                                      gavl_packet_t*p,int stream);

int bg_ogg_encoder_write_video_packet(void * data,
                                      gavl_packet_t*p,int stream);

int bg_ogg_encoder_close(void * data, int do_delete);

bg_parameter_info_t *
bg_ogg_encoder_get_audio_parameters(bg_ogg_encoder_t * e,
                                    bg_ogg_codec_t const * const * audio_codecs);

bg_parameter_info_t *
bg_ogg_encoder_get_video_parameters(bg_ogg_encoder_t * e,
                                    bg_ogg_codec_t const * const * video_codecs);


void
bg_ogg_create_comment_packet(const uint8_t * prefix,
                             int prefix_len,
                             const gavl_metadata_t * m_stream,
                             const gavl_metadata_t * m_global,
                             int framing, ogg_packet * op);

void bg_ogg_free_comment_packet(ogg_packet * op);

void bg_ogg_set_vorbis_channel_setup(gavl_audio_format_t * format);
