
#if 0
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
#endif

static void set_track(bg_track_info_t * track_info)
  {
  
  }

track_t * track_create(const char * url, const char * plugin,
                       int track, bg_plugin_registry_t * plugin_reg)
  {
  const bg_plugin_info_t * plugin_info;
  bg_plugin_handle_t     * plugin_handle = (bg_plugin_handle_t*);
  bg_input_plugin_t      * input;
  bg_track_info_t        * track_info;
  
  /* Load the plugin */
  
  }

void track_destroy(track_t * t)
  {
  
  }
