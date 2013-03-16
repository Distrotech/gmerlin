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

#include <config.h>
#include AVFORMAT_HEADER
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>

#ifdef HAVE_LIBAVCORE_AVCORE_H
#include <libavcore/avcore.h>
#endif

#if LIBAVCODEC_VERSION_MAJOR >= 53
#define guess_format(a, b, c) av_guess_format(a, b, c)
#endif


#define FLAG_CONSTANT_FRAMERATE (1<<0)
#define FLAG_INTRA_ONLY         (1<<1)
#define FLAG_B_FRAMES           (1<<2)
#define FLAG_PIPE               (1<<3) // Format can be written savely to pipes

typedef struct
  {
  char * name;
  char * long_name;
  enum CodecID id;
  const bg_parameter_info_t * parameters;

  int flags;
  const bg_encoder_framerate_t * framerates;
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
                              AVDictionary ** options,
                              const char * name,
                              const bg_parameter_value_t * val);

enum CodecID
bg_ffmpeg_find_audio_encoder(const ffmpeg_format_info_t * format,
                             const char * name);

enum CodecID
bg_ffmpeg_find_video_encoder(const ffmpeg_format_info_t * format,
                             const char * name);

const char *
bg_ffmpeg_get_codec_name(enum CodecID id);

const bg_parameter_info_t *
bg_ffmpeg_get_codec_parameters(enum CodecID id, int type);

const ffmpeg_codec_info_t *
bg_ffmpeg_get_codec_info(enum CodecID id, int type);


/*
 *  Standalone codecs
 */

/*
 *  Create a codec context.
 *  If avctx is NULL, it will be created and destroyed.
 *  If id is CODEC_ID_NONE, a "codec" parameter will be supported
 *
 *  Type is one of CODEC_TYPE_VIDEO or CODEC_TYPE_AUDIO
 */


typedef struct bg_ffmpeg_codec_context_s bg_ffmpeg_codec_context_t;

void bg_ffmpeg_set_video_dimensions(AVCodecContext * avctx,
                                    const gavl_video_format_t * fmt);

void bg_ffmpeg_set_audio_format(AVCodecContext * avctx,
                                const gavl_audio_format_t * fmt);


bg_ffmpeg_codec_context_t * bg_ffmpeg_codec_create(int type,
                                                   AVCodecContext * avctx,
                                                   enum CodecID id,
                                                   const ffmpeg_format_info_t * format);

const bg_parameter_info_t * bg_ffmpeg_codec_get_parameters(bg_ffmpeg_codec_context_t * ctx);


void bg_ffmpeg_codec_set_parameter(bg_ffmpeg_codec_context_t * ctx,
                                   const char * name,
                                   const bg_parameter_value_t * val);

int bg_ffmpeg_codec_set_video_pass(bg_ffmpeg_codec_context_t * ctx,
                                   int pass,
                                   int total_passes,
                                   const char * stats_filename);


gavl_audio_sink_t * bg_ffmpeg_codec_open_audio(bg_ffmpeg_codec_context_t * ctx,
                                               gavl_compression_info_t * ci,
                                               gavl_audio_format_t * fmt,
                                               gavl_metadata_t * m);

gavl_video_sink_t * bg_ffmpeg_codec_open_video(bg_ffmpeg_codec_context_t * ctx,
                                               gavl_compression_info_t * ci,
                                               gavl_video_format_t * fmt,
                                               gavl_metadata_t * m);


void bg_ffmpeg_codec_destroy(bg_ffmpeg_codec_context_t * ctx);

void bg_ffmpeg_codec_set_packet_sink(bg_ffmpeg_codec_context_t * ctx,
                                     gavl_packet_sink_t * psink);


/* ffmpeg_common.c */

typedef struct ffmpeg_priv_s ffmpeg_priv_t;

#define STREAM_ENCODER_INITIALIZED (1<<0)
#define STREAM_IS_COMPRESSED       (1<<1)

typedef struct
  {
  AVStream * stream;

  bg_ffmpeg_codec_context_t * codec;
  
  gavl_packet_t gp;

  int flags;
  
  gavl_packet_sink_t * psink;
  ffmpeg_priv_t * ffmpeg;
  gavl_compression_info_t ci;

  AVDictionary * options;
  
  gavl_metadata_t m;
  } bg_ffmpeg_stream_common_t;

typedef struct
  {
  bg_ffmpeg_stream_common_t com;
  gavl_audio_sink_t * sink;
  gavl_audio_format_t format;
  } bg_ffmpeg_audio_stream_t;

typedef struct
  {
  bg_ffmpeg_stream_common_t com;
  gavl_video_sink_t * sink;
  gavl_video_format_t format;
  int64_t dts;
  } bg_ffmpeg_video_stream_t;

typedef struct
  {
  bg_ffmpeg_stream_common_t com;
  } bg_ffmpeg_text_stream_t;

struct ffmpeg_priv_s
  {
  int num_audio_streams;
  int num_video_streams;
  int num_text_streams;
  
  bg_ffmpeg_audio_stream_t * audio_streams;
  bg_ffmpeg_video_stream_t * video_streams;
  bg_ffmpeg_text_stream_t * text_streams;
  
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
  };

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
                   const gavl_metadata_t * metadata,
                   const gavl_chapter_list_t * chapter_list);

const bg_parameter_info_t * bg_ffmpeg_get_audio_parameters(void * data);
const bg_parameter_info_t * bg_ffmpeg_get_video_parameters(void * data);

int bg_ffmpeg_add_audio_stream(void * data,
                               const gavl_metadata_t * metadata,
                               const gavl_audio_format_t * format);

int bg_ffmpeg_add_video_stream(void * data,
                               const gavl_metadata_t * metadata,
                               const gavl_video_format_t * format);

int bg_ffmpeg_add_text_stream(void * data,
                              const gavl_metadata_t * metadata,
                              uint32_t * timescale);

void bg_ffmpeg_set_audio_parameter(void * data, int stream, const char * name,
                                   const bg_parameter_value_t * v);

void bg_ffmpeg_set_video_parameter(void * data, int stream, const char * name,
                                  const bg_parameter_value_t * v);


int bg_ffmpeg_set_video_pass(void * data, int stream, int pass,
                             int total_passes,
                             const char * stats_file);

int bg_ffmpeg_start(void * data);

void bg_ffmpeg_get_audio_format(void * data, int stream,
                                gavl_audio_format_t*ret);

gavl_audio_sink_t *
bg_ffmpeg_get_audio_sink(void * data, int stream);

gavl_video_sink_t *
bg_ffmpeg_get_video_sink(void * data, int stream);

gavl_packet_sink_t *
bg_ffmpeg_get_audio_packet_sink(void * data, int stream);

gavl_packet_sink_t *
bg_ffmpeg_get_video_packet_sink(void * data, int stream);

gavl_packet_sink_t *
bg_ffmpeg_get_text_packet_sink(void * data, int stream);


int bg_ffmpeg_write_subtitle_text(void * data,const char * text,
                                  int64_t start,
                                  int64_t duration, int stream);

int bg_ffmpeg_close(void * data, int do_delete);

#define CONVERT_ENDIAN (1<<8)
#define CONVERT_OTHER  (1<<9)

void bg_ffmpeg_choose_pixelformat(const enum PixelFormat * supported,
                                  enum PixelFormat * ffmpeg_fmt,
                                  gavl_pixelformat_t * gavl_fmt, int * do_convert);

gavl_sample_format_t bg_sample_format_ffmpeg_2_gavl(enum AVSampleFormat p,
                                                    gavl_interleave_mode_t * il);

enum CodecID bg_codec_id_gavl_2_ffmpeg(gavl_codec_id_t gavl);
gavl_codec_id_t bg_codec_id_ffmpeg_2_gavl(enum CodecID ffmpeg);

uint64_t
bg_ffmpeg_get_channel_layout(gavl_audio_format_t * format);


/* Compressed stream support */

int bg_ffmpeg_writes_compressed_audio(void * priv,
                                      const gavl_audio_format_t * format,
                                      const gavl_compression_info_t * info);

int bg_ffmpeg_writes_compressed_video(void * priv,
                                      const gavl_video_format_t * format,
                                      const gavl_compression_info_t * info);

int bg_ffmpeg_add_audio_stream_compressed(void * priv,
                                          const gavl_metadata_t * metadata,
                                          const gavl_audio_format_t * format,
                                          const gavl_compression_info_t * info);

int bg_ffmpeg_add_video_stream_compressed(void * priv,
                                          const gavl_metadata_t * metadata,
                                          const gavl_video_format_t * format,
                                          const gavl_compression_info_t * info);
