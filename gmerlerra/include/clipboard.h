typedef struct
  {
  int num_tracks;
  bg_nle_track_t ** tracks;
  int num_files;
  bg_nle_file_t ** files;
  } bg_nle_clipboard_t;

void bg_nle_clipboard_free(bg_nle_clipboard_t*);

void bg_nle_clipboard_from_file(bg_nle_clipboard_t*, bg_nle_file_t * file,
                                bg_nle_time_range_t * r, int * audio_streams,
                                int * video_streams);

void bg_nle_clipboard_from_project(bg_nle_clipboard_t*, bg_nle_project_t * p,
                                   bg_nle_time_range_t * r);

/* clipboard_xml.c */

char * bg_nle_clipboard_to_string(const bg_nle_clipboard_t*);

int bg_nle_clipboard_from_string(bg_nle_clipboard_t*, const char * str);

