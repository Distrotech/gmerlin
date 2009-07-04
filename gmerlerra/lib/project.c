#include <gavl/gavl.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>

#include <track.h>
#include <project.h>


bg_nle_project_t * bg_nle_project_create(bg_plugin_registry_t * plugin_reg)
  {
  bg_nle_project_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->end_visible = GAVL_TIME_SCALE * 10;
  ret->start_selection = 0;
  ret->end_selection = -1;
  ret->media_list = bg_nle_media_list_create(plugin_reg);
  ret->plugin_reg = plugin_reg;

  ret->audio_track_section =
    bg_cfg_section_create_from_parameters("",
                                          bg_nle_track_audio_parameters);
  ret->video_track_section =
    bg_cfg_section_create_from_parameters("",
                                          bg_nle_track_video_parameters);
  ret->audio_outstream_section =
    bg_cfg_section_create_from_parameters("",
                                          bg_nle_outstream_audio_parameters);
  ret->video_outstream_section =
    bg_cfg_section_create_from_parameters("",
                                          bg_nle_outstream_video_parameters);
  
  return ret;
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

  if(p->tracks)
    free(p->tracks);
  
  free(p);
  }

static bg_nle_id_t create_track_id(bg_nle_track_t ** tracks,
                                   int num_tracks)
  {
  bg_nle_id_t ret = 0;
  int keep_going;
  int i;
  
  do{
    ret++;
    keep_going = 0;
    for(i = 0; i < num_tracks; i++)
      {
      if(tracks[i]->id == ret)
        {
        keep_going = 1;
        break;
        }
      }
    }while(keep_going);
  return ret;
  }

bg_nle_track_t * bg_nle_project_add_audio_track(bg_nle_project_t * p)
  {
  int i;
  bg_nle_track_t * track;
  char * tmp_string;
  
  track = bg_nle_track_create(BG_NLE_TRACK_AUDIO);

  /* Create ID */
  track->id = create_track_id(p->audio_tracks, p->num_audio_tracks);
  bg_cfg_section_transfer(p->audio_track_section,
                          track->section);
  
  tmp_string = bg_sprintf("Audio track %d", p->num_audio_tracks+1);
  bg_nle_track_set_name(track, tmp_string);
  free(tmp_string);

  bg_nle_project_append_track(p, track);
  for(i = 0; i < p->num_video_outstreams; i++)
    {
    bg_nle_outstream_attach_track(p->audio_outstreams[i],
                                  track);
    }
  
  p->changed_flags |= BG_NLE_PROJECT_TRACKS_CHANGED;
  
  return track;
  }

bg_nle_track_t * bg_nle_project_add_video_track(bg_nle_project_t * p)
  {
  int i;
  bg_nle_track_t * track;
  char * tmp_string;
  
  track = bg_nle_track_create(BG_NLE_TRACK_VIDEO);

  /* Create ID */
  track->id = create_track_id(p->video_tracks, p->num_video_tracks);
  
  bg_cfg_section_transfer(p->video_track_section,
                          track->section);
  
  tmp_string = bg_sprintf("Video track %d", p->num_video_tracks+1);
  bg_nle_track_set_name(track, tmp_string);
  free(tmp_string);

  p->changed_flags |= BG_NLE_PROJECT_TRACKS_CHANGED;
  
  bg_nle_project_append_track(p, track);
  for(i = 0; i < p->num_video_outstreams; i++)
    {
    bg_nle_outstream_attach_track(p->video_outstreams[i],
                                  track);
    }
  
  return track;
  }

void bg_nle_project_append_track(bg_nle_project_t * p, bg_nle_track_t * t)
  {
  if(p->num_tracks+1 > p->tracks_alloc)
    {
    p->tracks_alloc += 16;
    p->tracks = realloc(p->tracks,
                        sizeof(*p->tracks) *
                        (p->tracks_alloc));
    }
  p->tracks[p->num_tracks] = t;
  p->num_tracks++;
  
  switch(t->type)
    {
    case BG_NLE_TRACK_AUDIO:
      if(p->num_audio_tracks+1 > p->audio_tracks_alloc)
        {
        p->audio_tracks_alloc += 16;
        p->audio_tracks = realloc(p->audio_tracks,
                                  sizeof(*p->audio_tracks) *
                                  (p->audio_tracks_alloc));
        }
      p->audio_tracks[p->num_audio_tracks] = t;
      p->num_audio_tracks++;
      break;
    case BG_NLE_TRACK_VIDEO:
      if(p->num_video_tracks+1 > p->video_tracks_alloc)
        {
        p->video_tracks_alloc += 16;
        p->video_tracks = realloc(p->video_tracks,
                                  sizeof(*p->video_tracks) *
                                  (p->video_tracks_alloc));
        }
      p->video_tracks[p->num_video_tracks] = t;
      p->num_video_tracks++;
      break;
    case BG_NLE_TRACK_NONE:
      break;
    }
  
  }

bg_nle_outstream_t * bg_nle_project_add_video_outstream(bg_nle_project_t * p)
  {
  bg_nle_outstream_t * outstream;
  char * tmp_string;
  int i;
  outstream = bg_nle_outstream_create(BG_NLE_TRACK_VIDEO);
  bg_cfg_section_transfer(p->video_outstream_section,
                          outstream->section);
  tmp_string = bg_sprintf("Video outstream %d", p->num_video_outstreams+1);
  bg_nle_outstream_set_name(outstream, tmp_string);
  free(tmp_string);
  
  bg_nle_project_append_outstream(p, outstream);
  p->changed_flags |= BG_NLE_PROJECT_OUTSTREAMS_CHANGED;

  for(i = 0; i < p->num_video_tracks; i++)
    {
    bg_nle_outstream_attach_track(outstream,
                                  p->video_tracks[i]);
    }
  
  return outstream;
  
  }


bg_nle_outstream_t * bg_nle_project_add_audio_outstream(bg_nle_project_t * p)
  {
  bg_nle_outstream_t * outstream;
  char * tmp_string;
  int i;
  outstream = bg_nle_outstream_create(BG_NLE_TRACK_AUDIO);
  bg_cfg_section_transfer(p->audio_outstream_section,
                          outstream->section);
  tmp_string = bg_sprintf("Audio outstream %d", p->num_audio_outstreams+1);
  bg_nle_outstream_set_name(outstream, tmp_string);
  free(tmp_string);
  
  bg_nle_project_append_outstream(p, outstream);
  p->changed_flags |= BG_NLE_PROJECT_OUTSTREAMS_CHANGED;

  for(i = 0; i < p->num_video_tracks; i++)
    {
    bg_nle_outstream_attach_track(outstream,
                                  p->audio_tracks[i]);
    }
  
  return outstream;
  
  }

void bg_nle_project_append_outstream(bg_nle_project_t * p,
                                     bg_nle_outstream_t * t)
  {
  if(p->num_outstreams+1 > p->outstreams_alloc)
    {
    p->outstreams_alloc += 16;
    p->outstreams = realloc(p->outstreams,
                        sizeof(*p->outstreams) *
                        (p->outstreams_alloc));
    }
  p->outstreams[p->num_outstreams] = t;
  p->num_outstreams++;

  t->p = p;
  
  switch(t->type)
    {
    case BG_NLE_TRACK_AUDIO:
      if(p->num_audio_outstreams+1 > p->audio_outstreams_alloc)
        {
        p->audio_outstreams_alloc += 16;
        p->audio_outstreams = realloc(p->audio_outstreams,
                                  sizeof(*p->audio_outstreams) *
                                  (p->audio_outstreams_alloc));
        }
      p->audio_outstreams[p->num_audio_outstreams] = t;
      p->num_audio_outstreams++;
      break;
    case BG_NLE_TRACK_VIDEO:
      if(p->num_video_outstreams+1 > p->video_outstreams_alloc)
        {
        p->video_outstreams_alloc += 16;
        p->video_outstreams = realloc(p->video_outstreams,
                                  sizeof(*p->video_outstreams) *
                                  (p->video_outstreams_alloc));
        }
      p->video_outstreams[p->num_video_outstreams] = t;
      p->num_video_outstreams++;
      break;
    case BG_NLE_TRACK_NONE:
      break;
    }
  
  }

static void resolve_source_tracks(bg_nle_outstream_t * s,
                                  bg_nle_track_t ** tracks, int num_tracks)
  {
  int i, j;
  s->source_tracks = malloc(s->source_tracks_alloc *
                            sizeof(*s->source_tracks));

  for(i = 0; i < s->num_source_tracks; i++)
    {
    for(j = 0; j < num_tracks; j++)
      {
      if(s->source_track_ids[i] == tracks[j]->id)
        {
        s->source_tracks[i] = tracks[j];
        break;
        }
      }
    }
  }


void bg_nle_project_resolve_ids(bg_nle_project_t * p)
  {
  int i;
  /* Set the source tracks of the outstreams */

  for(i = 0; i < p->num_audio_outstreams; i++)
    {
    resolve_source_tracks(p->audio_outstreams[i],
                          p->audio_tracks, p->num_audio_tracks);
    }
  for(i = 0; i < p->num_video_outstreams; i++)
    {
    resolve_source_tracks(p->video_outstreams[i],
                          p->video_tracks, p->num_video_tracks);
    }
  
  }
