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

#include <config.h>
#include AVFORMAT_HEADER
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>

#define FLAG_CONSTANT_FRAMERATE (1<<9)

#ifdef HAVE_LIBAVCORE_AVCORE_H
#include <libavcore/avcore.h>
#endif

#if (LIBAVCORE_VERSION_INT >= ((0<<16)|(10<<8)|0)) || (LIBAVUTIL_VERSION_INT >= ((50<<16)|(38<<8)|0))
#define SampleFormat    AVSampleFormat
#define SAMPLE_FMT_U8   AV_SAMPLE_FMT_U8
#define SAMPLE_FMT_S16  AV_SAMPLE_FMT_S16
#define SAMPLE_FMT_S32  AV_SAMPLE_FMT_S32
#define SAMPLE_FMT_FLT  AV_SAMPLE_FMT_FLT
#define SAMPLE_FMT_DBL  AV_SAMPLE_FMT_DBL
#define SAMPLE_FMT_NONE AV_SAMPLE_FMT_NONE
#endif

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(54,1,0)
#define ENCODE_VIDEO2 1
#else
#define ENCODE_VIDEO 1
#endif

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,34,0)
#define ENCODE_AUDIO2 1
#else
#define ENCODE_AUDIO 1
#endif


typedef struct
  {
  char * name;
  char * long_name;
  enum CodecID id;
  const bg_parameter_info_t * parameters;

  /* Terminated with PIX_FMT_NB */
  enum PixelFormat * pixelformats;
  
  } ffmpeg_codec_info_t;

typedef struct
  {
  char * name;
  char * short_name;
  char * extension;
  
  int max_audio_streams;
  int max_video_streams;
  
  const enum CodecID * audio_codecs;
  const enum CodecID * video_codecs;
  
  int flags;
  const bg_encoder_framerate_t * framerates;
  
  } ffmpeg_format_info_t;

/* codecs.c */

bg_parameter_info_t *
bg_ffmpeg_create_audio_parameters(const ffmpeg_format_info_t * format_info);

bg_parameter_info_t *
bg_ffmpeg_create_video_parameters(const ffmpeg_format_info_t * format_info);

bg_parameter_info_t *
bg_ffmpeg_create_parameters(const ffmpeg_format_info_t * format_info);

void
bg_ffmpeg_set_codec_parameter(AVCodecContext * ctx,
#if LIBAVCODEC_VERSION_MAJOR >= 54
                              AVDictionary ** options,
#endif
                              const char * name,
                              const bg_parameter_value_t * val);

enum CodecID
bg_ffmpeg_find_audio_encoder(const ffmpeg_format_info_t * format, const char * name);

enum CodecID
bg_ffmpeg_find_video_encoder(const ffmpeg_format_info_t * format, const char * name);

typedef struct
  {
  AVStream * stream;
  gavl_audio_format_t format;
  
  uint8_t * buffer;
  int buffer_alloc;

  gavl_audio_frame_t * frame;

  int initialized;

#if LIBAVCODEC_VERSION_MAJOR >= 54
  AVDictionary * options;
#endif

#if ENCODE_VIDEO2
  int64_t samples_written;
#endif
  
  const gavl_compression_info_t * ci;
  int64_t pts_offset;
  } ffmpeg_audio_stream_t;

typedef struct
  {
  AVStream * stream;
  gavl_video_format_t format;

  uint8_t * buffer;
  int buffer_alloc;
  AVFrame * frame;

  int initialized;

  /* Multipass stuff */
  char * stats_filename;
  int pass;
  int total_passes;
  FILE * stats_file;
  bg_encoder_framerate_t fr;
  
  int64_t frames_written;

#if LIBAVCODEC_VERSION_MAJOR >= 54
  AVDictionary * options;
#endif
  const gavl_compression_info_t * ci;
  
  int64_t dts;

  int64_t pts_offset;
  } ffmpeg_video_stream_t;

typedef struct
  {
  AVStream * stream;
  int64_t pts_offset;
  } ffmpeg_text_stream_t;

typedef struct
  {
  int num_audio_streams;
  int num_video_streams;
  int num_text_streams;
  
  ffmpeg_audio_stream_t * audio_streams;
  ffmpeg_video_stream_t * video_streams;
  ffmpeg_text_stream_t * text_streams;
  
  AVFormatContext * ctx;
  
  bg_parameter_info_t * audio_parameters;
  bg_parameter_info_t * video_parameters;
  bg_parameter_info_t * parameters;
  
  const ffmpeg_format_info_t * formats;
  const ffmpeg_format_info_t * format;

  int initialized;
  int got_error;

  // Needed when we write compressed video packets with B-frames
  int need_pts_offset;
  
  bg_encoder_callbacks_t * cb;
  } ffmpeg_priv_t;

extern const bg_encoder_framerate_t
bg_ffmpeg_mpeg_framerates[];

void * bg_ffmpeg_create(const ffmpeg_format_info_t * formats);

void bg_ffmpeg_destroy(void*);

void bg_ffmpeg_set_callbacks(void * data,
                             bg_encoder_callbacks_t * cb);


const bg_parameter_info_t * bg_ffmpeg_get_parameters(void * data);

void bg_ffmpeg_set_parameter(void * data, const char * name,
                             const bg_parameter_value_t * v);

int bg_ffmpeg_open(void * data, const char * filename,
                   const bg_metadata_t * metadata,
                   const bg_chapter_list_t * chapter_list);

const bg_parameter_info_t * bg_ffmpeg_get_audio_parameters(void * data);
const bg_parameter_info_t * bg_ffmpeg_get_video_parameters(void * data);

int bg_ffmpeg_add_audio_stream(void * data, const char * language,
                               const gavl_audio_format_t * format);

int bg_ffmpeg_add_video_stream(void * data, const gavl_video_format_t * format);
int bg_ffmpeg_add_text_stream(void * data, const char * language,
                              int * timescale);


void bg_ffmpeg_set_audio_parameter(void * data, int stream, const char * name,
                                   const bg_parameter_value_t * v);

void bg_ffmpeg_set_video_parameter(void * data, int stream, const char * name,
                                  const bg_parameter_value_t * v);

enum PixelFormat * bg_ffmpeg_get_pixelformats(enum CodecID id);


int bg_ffmpeg_set_video_pass(void * data, int stream, int pass,
                             int total_passes,
                             const char * stats_file);

int bg_ffmpeg_start(void * data);

void bg_ffmpeg_get_audio_format(void * data, int stream,
                                gavl_audio_format_t*ret);
void bg_ffmpeg_get_video_format(void * data, int stream,
                                gavl_video_format_t*ret);

int bg_ffmpeg_write_audio_frame(void * data,
                                gavl_audio_frame_t * frame, int stream);

int bg_ffmpeg_write_video_frame(void * data,
                                gavl_video_frame_t * frame, int stream);

int bg_ffmpeg_write_subtitle_text(void * data,const char * text,
                                  int64_t start,
                                  int64_t duration, int stream);


int bg_ffmpeg_close(void * data, int do_delete);

gavl_pixelformat_t bg_pixelformat_ffmpeg_2_gavl(enum PixelFormat p);
enum PixelFormat bg_pixelformat_gavl_2_ffmpeg(gavl_pixelformat_t p);

gavl_sample_format_t bg_sample_format_ffmpeg_2_gavl(enum SampleFormat p);

enum CodecID bg_codec_id_gavl_2_ffmpeg(gavl_codec_id_t gavl);


/* Compressed stream support */

int bg_ffmpeg_writes_compressed_audio(void * priv,
                                      const gavl_audio_format_t * format,
                                      const gavl_compression_info_t * info);

int bg_ffmpeg_writes_compressed_video(void * priv,
                                      const gavl_video_format_t * format,
                                      const gavl_compression_info_t * info);

int bg_ffmpeg_add_audio_stream_compressed(void * priv, const char * language,
                                          const gavl_audio_format_t * format,
                                          const gavl_compression_info_t * info);

int bg_ffmpeg_add_video_stream_compressed(void * priv,
                                          const gavl_video_format_t * format,
                                          const gavl_compression_info_t * info);


int bg_ffmpeg_write_audio_packet(void * data, gavl_packet_t * packet, int stream);
int bg_ffmpeg_write_video_packet(void * data, gavl_packet_t * packet, int stream);
