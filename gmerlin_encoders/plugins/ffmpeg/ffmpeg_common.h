#include <ffmpeg/avformat.h>
#include <gmerlin/plugin.h>

typedef struct
  {
  char * name;
  char * long_name;
  enum CodecID id;
  bg_parameter_info_t * parameters;
  } ffmpeg_codec_info_t;

typedef struct
  {
  char * name;
  char * short_name;
  char * extension;
  
  int max_audio_streams;
  int max_video_streams;
  
  enum CodecID * audio_codecs;
  enum CodecID * video_codecs;
  } ffmpeg_format_info_t;

/* codecs.c */

bg_parameter_info_t *
bg_ffmpeg_create_audio_parameters(const ffmpeg_format_info_t * format_info);

bg_parameter_info_t *
bg_ffmpeg_create_video_parameters(const ffmpeg_format_info_t * format_info);

bg_parameter_info_t *
bg_ffmpeg_create_parameters(const ffmpeg_format_info_t * format_info);

void
bg_ffmpeg_set_codec_parameter(AVCodecContext * ctx, char * name,
                              bg_parameter_value_t * val);

enum CodecID
bg_ffmpeg_find_audio_encoder(const char * name);

enum CodecID
bg_ffmpeg_find_video_encoder(const char * name);

typedef struct
  {
  AVStream * stream;
  gavl_audio_format_t format;
  
  uint8_t * buffer;
  int buffer_alloc;

  gavl_audio_frame_t * frame;

  int initialized;
  } ffmpeg_audio_stream_t;

typedef struct
  {
  AVStream * stream;
  gavl_video_format_t format;
  
  uint8_t * buffer;
  int buffer_alloc;
  AVFrame * frame;

  int initialized;
  } ffmpeg_video_stream_t;

typedef struct
  {
  int num_audio_streams;
  int num_video_streams;
  
  ffmpeg_audio_stream_t * audio_streams;
  ffmpeg_video_stream_t * video_streams;
  
  AVFormatContext * ctx;
  
  bg_parameter_info_t * audio_parameters;
  bg_parameter_info_t * video_parameters;
  bg_parameter_info_t * parameters;
  
  const ffmpeg_format_info_t * formats;
  const ffmpeg_format_info_t * format;

  int initialized;
  char * error_msg;
  } ffmpeg_priv_t;

void * bg_ffmpeg_create(const ffmpeg_format_info_t * formats);

void bg_ffmpeg_destroy(void*);

bg_parameter_info_t * bg_ffmpeg_get_parameters(void * data);

void bg_ffmpeg_set_parameter(void * data, char * name,
                             bg_parameter_value_t * v);

const char * bg_ffmpeg_get_extension(void * data);

const char * bg_ffmpeg_get_error(void * data);

int bg_ffmpeg_open(void * data, const char * filename,
                   bg_metadata_t * metadata,
                   bg_chapter_list_t * chapter_list);

bg_parameter_info_t * bg_ffmpeg_get_audio_parameters(void * data);
bg_parameter_info_t * bg_ffmpeg_get_video_parameters(void * data);

int bg_ffmpeg_add_audio_stream(void * data, const char * language,
                               gavl_audio_format_t * format);

int bg_ffmpeg_add_video_stream(void * data, gavl_video_format_t * format);

void bg_ffmpeg_set_audio_parameter(void * data, int stream, char * name,
                                  bg_parameter_value_t * v);

void bg_ffmpeg_set_video_parameter(void * data, int stream, char * name,
                                  bg_parameter_value_t * v);


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

int bg_ffmpeg_close(void * data, int do_delete);
