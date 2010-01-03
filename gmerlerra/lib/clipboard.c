#include <inttypes.h>

#include <types.h>
#include <track.h>
#include <medialist.h>
#include <clipboard.h>

void bg_nle_clipboard_free(bg_nle_clipboard_t * c)
  {
  int i;
  for(i = 0; i < c->num_tracks; i++)
    {
    
    }
  }   

void bg_nle_clipboard_from_string(bg_nle_clipboard_t * c, const char * str)
  {

  }

void bg_nle_clipboard_from_file(bg_nle_clipboard_t * c, bg_nle_file_t * file,
                                bg_nle_time_range_t * r, int * audio_streams,
                                int * video_streams)
  {
  bg_nle_clipboard_free(c);
  c->num_files = 1;
  c->files = malloc(c->num_files * sizeof(*c->files));
  c->files[0] = bg_nle_file_copy(file);
  
  }

void bg_nle_clipboard_from_project(bg_nle_clipboard_t * c, bg_nle_project_t * p,
                                   bg_nle_time_range_t * r)
  {
  
  }

char * bg_nle_clipboard_to_string(const bg_nle_clipboard_t * c)
  {
  
  }

