
/* This defines a track with all information
   necessary for transcoding */

typedef struct
  {
  int dummy;
  } audio_stream_t;

typedef struct
  {
  int dummy;
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

track_t * track_create_from_url(const char * url);

