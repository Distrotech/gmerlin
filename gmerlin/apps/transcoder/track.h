
/* This defines a track with all information
   necessary for transcoding */

typedef struct
  {
  gavl_audio_format_t format;
  } audio_stream_t;

typedef struct
  {
  gavl_video_format_t format;
  } video_stream_t;

typedef struct track_s
  {
  char * name;
  char * plugin;
  gavl_time_t duration;
  
  int num_audio_streams;
  int num_video_streams;

  audio_stream_t * audio_streams;
  video_stream_t * video_streams;
  bg_metadata_t metadata;

  /* For chaining */
  struct track_s * next;

  } track_t;

track_t * track_create(const char * url, const char * plugin,
                       int track, bg_plugin_registry_t * plugin_reg);

void track_destroy(track_t * t);
