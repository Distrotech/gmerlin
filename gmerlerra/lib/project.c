#include <gavl/gavl.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/translation.h>

#include <track.h>
#include <project.h>


bg_nle_project_t * bg_nle_project_create(const char * file)
  {
  bg_nle_project_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_nle_project_save(bg_nle_project_t * p, const char * file)
  {
  
  }

void bg_nle_project_destroy(bg_nle_project_t * p)
  {
  int i;

  /* Free tracks */
  for(i = 0; i < p->num_audio_tracks; i++)
    bg_nle_track_destroy(p->audio_tracks[i]);

  if(p->audio_tracks)
    free(p->audio_tracks);

  for(i = 0; i < p->num_video_tracks; i++)
    bg_nle_track_destroy(p->video_tracks[i]);

  if(p->video_tracks)
    free(p->video_tracks);
    
  free(p);
  }

bg_nle_track_t * bg_nle_project_add_audio_track(bg_nle_project_t * p)
  {
  bg_nle_track_t * track;

  p->audio_tracks = realloc(p->audio_tracks,
                            sizeof(*p->audio_tracks) *
                            (p->num_audio_tracks+1));

  track = bg_nle_track_create(BG_NLE_TRACK_AUDIO);
  
  p->audio_tracks[p->num_audio_tracks] = track;

  p->num_audio_tracks++;

  track->name = bg_sprintf("Audio track %d", p->num_audio_tracks);

  return track;
  }

bg_nle_track_t * bg_nle_project_add_video_track(bg_nle_project_t * p)
  {
  bg_nle_track_t * track;
  p->video_tracks = realloc(p->video_tracks,
                            sizeof(*p->video_tracks) *
                            (p->num_video_tracks+1));
  track = bg_nle_track_create(BG_NLE_TRACK_VIDEO);
  p->video_tracks[p->num_video_tracks] = track;

  p->num_video_tracks++;
  
  track->name = bg_sprintf("Video track %d", p->num_video_tracks);
  
  return track;
  }

