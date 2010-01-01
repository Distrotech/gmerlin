#include <project.h>

typedef struct bg_nle_file_handle_s
  {
  bg_plugin_handle_t * h;
  bg_nle_file_t * file;
  
  bg_nle_track_type_t type;
  int stream;
  struct bg_nle_file_handle_s * next;

  gavl_time_t last_use_time;

  bg_input_plugin_t * plugin;
  bg_track_info_t * ti;
  } bg_nle_file_handle_t;

typedef struct bg_nle_file_cache_s bg_nle_file_cache_t;

bg_nle_file_cache_t * bg_nle_file_cache_create(bg_nle_project_t * p);

bg_nle_file_handle_t * bg_nle_file_cache_load(bg_nle_file_cache_t * c,
                                              bg_nle_id_t id,
                                              bg_nle_track_type_t type,
                                              int stream);

void bg_nle_file_cache_unload(bg_nle_file_cache_t * c, bg_nle_file_handle_t * h);


void bg_nle_file_cache_destroy(bg_nle_file_cache_t * c);
