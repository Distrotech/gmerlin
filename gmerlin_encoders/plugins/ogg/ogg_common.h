/* Generic struct for a codec. Here, we'll implement
   encoders for vorbis, theora, speex and flac */

typedef struct
  {
  char * name;
  char * long_name;
  
  void * (*create)(FILE * output, long serialno);

  bg_parameter_info_t * (*get_parameters)();
  void (*set_parameter)(void*, char * name, bg_parameter_value_t * v);

  int (*init_audio)(void*, gavl_audio_format_t * format, bg_metadata_t * metadata);
  int (*init_video)(void*, gavl_video_format_t * format, bg_metadata_t * metadata);
  
  void (*flush_header_pages)(void*);
  
  void (*encode_audio)(void*, gavl_audio_frame_t*f);
  void (*encode_video)(void*, gavl_video_frame_t*f);

  void (*close)(void*);
  } bg_ogg_codec_t;

typedef struct
  {
  int num_audio_streams;
  int num_video_streams;
  
  struct
    {
    bg_ogg_codec_t * codec;
    void           * codec_priv;
    gavl_audio_format_t format;
    } * audio_streams;

  struct
    {
    bg_ogg_codec_t * codec;
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

int bg_ogg_encoder_open(void *, const char * file, bg_metadata_t * metadata);

void bg_ogg_encoder_destroy(void*);

int bg_ogg_flush_page(ogg_stream_state * os, FILE * output, int force);

void bg_ogg_flush(ogg_stream_state * os, FILE * output, int force);

int bg_ogg_encoder_add_audio_stream(void*, gavl_audio_format_t * format);
int bg_ogg_encoder_add_video_stream(void*, gavl_video_format_t * format);

void bg_ogg_encoder_init_audio_stream(void*, int stream, bg_ogg_codec_t * codec);
void bg_ogg_encoder_init_video_stream(void*, int stream, bg_ogg_codec_t * codec);

void bg_ogg_encoder_set_audio_parameter(void*, int stream, char * name, bg_parameter_value_t * val);
void bg_ogg_encoder_set_video_parameter(void*, int stream, char * name, bg_parameter_value_t * val);

int bg_ogg_encoder_start(void*);

void bg_ogg_encoder_get_audio_format(void * data, int stream, gavl_audio_format_t*ret);
void bg_ogg_encoder_get_video_format(void * data, int stream, gavl_video_format_t*ret);

void bg_ogg_encoder_write_audio_frame(void * data, gavl_audio_frame_t*f,int stream);
void bg_ogg_encoder_write_video_frame(void * data, gavl_video_frame_t*f,int stream);
void bg_ogg_encoder_close(void * data, int do_delete);
