
/* This defines a track with all information
   necessary for transcoding */

typedef struct
  {
  gavl_audio_format_t input_format;
  gavl_audio_format_t output_format;

  int fixed_samplerate;
  int fixed_channel_setup;

  bg_parameter_info_t * format_parameters;
  bg_parameter_info_t * encoder_parameters;
  
  gavl_audio_options_t opt;
  } bg_transcoder_audio_stream_t;

bg_parameter_info_t *
bg_transcoder_audio_stream_get_parameters(bg_transcoder_audio_stream_t*);

void bg_transcoder_audio_stream_set_format_parameter(void * data, char * name,
                                                     bg_parameter_value_t * val);

void bg_transcoder_audio_stream_set_encoder_parameter(void * data, char * name,
                                                      bg_parameter_value_t * val);

typedef struct
  {
  gavl_video_format_t input_format;
  gavl_video_format_t output_format;

  bg_parameter_info_t * format_parameters;
  bg_parameter_info_t * encoder_parameters;
  
  int fixed_framerate;
    
  gavl_video_options_t opt;
  } bg_transcoder_video_stream_t;

bg_parameter_info_t *
bg_transcoder_video_stream_get_parameters(bg_transcoder_video_stream_t*);

void bg_transcoder_video_stream_set_format_parameter(void * data, char * name,
                                                     bg_parameter_value_t * val);

void bg_transcoder_video_stream_set_encoder_parameter(void * data, char * name,
                                                      bg_parameter_value_t * val);

typedef struct bg_transcoder_track_s
  {
  char * name;
  char * plugin;
  gavl_time_t duration;
  
  int num_audio_streams;
  int num_video_streams;

  bg_transcoder_audio_stream_t * audio_streams;
  bg_transcoder_video_stream_t * video_streams;
  bg_metadata_t metadata;

  /* For chaining */
  struct bg_transcoder_track_s * next;

  /* Is the track selected by the GUI? */

  int selected;

  } bg_transcoder_track_t;

bg_transcoder_track_t *
bg_transcoder_track_create(const char * url,
                           const bg_plugin_info_t * plugin,
                           int track, bg_plugin_registry_t * plugin_reg,
                           bg_cfg_section_t * section,
                           bg_plugin_handle_t * audio_encoder,
                           bg_plugin_handle_t * video_encoder);

bg_transcoder_track_t *
bg_transcoder_track_create_from_urilist(const char * list,
                                        int len,
                                        bg_plugin_registry_t * plugin_reg,
                                        bg_cfg_section_t * section,
                                        bg_plugin_handle_t * audio_encoder,
                                        bg_plugin_handle_t * video_encoder);

bg_transcoder_track_t *
bg_transcoder_track_create_from_albumentries(const char * xml_string,
                                             int len,
                                             bg_plugin_registry_t * plugin_reg,
                                             bg_cfg_section_t * section,
                                             bg_plugin_handle_t * audio_encoder,
                                             bg_plugin_handle_t * video_encoder);


void bg_transcoder_track_destroy(bg_transcoder_track_t * t);

bg_parameter_info_t *
bg_transcoder_track_get_parameters_general(bg_transcoder_track_t * t);

void bg_transcoder_track_set_parameter_general(void * data, char * name,
                                               bg_parameter_value_t * val);

