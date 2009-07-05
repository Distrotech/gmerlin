#include <string.h>

#include <gavl/gavl.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>

#include <track.h>
#include <project.h>
#include <editops.h>

static void bg_nle_edit_callback_stub(bg_nle_project_t * p,
                                      bg_nle_edit_op_t op,
                                      void * op_data,
                                      void * user_data)
  {

  }

static void edited(bg_nle_project_t * p,
                   bg_nle_edit_op_t op,
                   void * op_data)
  {
  /* TODO: Push data */
  
  p->edit_callback(p, op, op_data, p->edit_callback_data);
  }


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

  ret->edit_callback = bg_nle_edit_callback_stub;
  
  return ret;
  }

void bg_nle_project_destroy(bg_nle_project_t * p)
  {
  int i;

  /* Free tracks */
  for(i = 0; i < p->num_tracks; i++)
    bg_nle_track_destroy(p->tracks[i]);
  
  if(p->tracks)
    free(p->tracks);

  for(i = 0; i < p->num_outstreams; i++)
    bg_nle_outstream_destroy(p->outstreams[i]);
  
  if(p->outstreams)
    free(p->outstreams);

  
  free(p);
  }

void bg_nle_project_set_edit_callback(bg_nle_project_t * p,
                                      bg_nle_edit_callback callback,
                                      void * callback_data)
  {
  p->edit_callback = callback;
  p->edit_callback_data = callback_data;
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

void bg_nle_project_add_audio_track(bg_nle_project_t * p)
  {
  int i;
  bg_nle_track_t * track;
  char * tmp_string;
  bg_nle_op_add_track_t d;
  int num_tracks;
  track = bg_nle_track_create(BG_NLE_TRACK_AUDIO);
  
  /* Create ID */
  track->id = create_track_id(p->tracks, p->num_tracks);
  bg_cfg_section_transfer(p->audio_track_section,
                          track->section);

  /* Create default name */
  num_tracks = 0;
  for(i = 0; i < p->num_tracks; i++)
    {
    if(p->tracks[i]->type == BG_NLE_TRACK_AUDIO)
      num_tracks++;
    }
  
  tmp_string = bg_sprintf("Audio track %d", num_tracks+1);
  bg_nle_track_set_name(track, tmp_string);
  free(tmp_string);

  bg_nle_project_append_track(p, track);
  for(i = 0; i < p->num_outstreams; i++)
    {
    if(p->outstreams[i]->type == BG_NLE_TRACK_AUDIO)
      bg_nle_outstream_attach_track(p->outstreams[i],
                                    track);
    }
  
  p->changed_flags |= BG_NLE_PROJECT_TRACKS_CHANGED;

  d.track = track;
  edited(p, BG_NLE_EDIT_ADD_TRACK, &d);
  
  }

void bg_nle_project_add_video_track(bg_nle_project_t * p)
  {
  int i;
  bg_nle_track_t * track;
  char * tmp_string;
  bg_nle_op_add_track_t d;
  int num_tracks;
  
  track = bg_nle_track_create(BG_NLE_TRACK_VIDEO);

  /* Create ID */
  track->id = create_track_id(p->tracks, p->num_tracks);
  
  bg_cfg_section_transfer(p->video_track_section,
                          track->section);
  /* Create default name */
  num_tracks = 0;
  for(i = 0; i < p->num_tracks; i++)
    {
    if(p->tracks[i]->type == BG_NLE_TRACK_VIDEO)
      num_tracks++;
    }
  
  tmp_string = bg_sprintf("Video track %d", num_tracks+1);
  bg_nle_track_set_name(track, tmp_string);
  free(tmp_string);

  p->changed_flags |= BG_NLE_PROJECT_TRACKS_CHANGED;
  
  bg_nle_project_append_track(p, track);
  for(i = 0; i < p->num_outstreams; i++)
    {
    if(p->outstreams[i]->type == BG_NLE_TRACK_VIDEO)
      bg_nle_outstream_attach_track(p->outstreams[i],
                                    track);
    }
  d.track = track;
  edited(p, BG_NLE_EDIT_ADD_TRACK, &d);
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
  t->p = p;
  
  }

void bg_nle_project_add_video_outstream(bg_nle_project_t * p)
  {
  bg_nle_outstream_t * outstream;
  char * tmp_string;
  int i;
  int num;
  bg_nle_op_add_outstream_t d;
  
  outstream = bg_nle_outstream_create(BG_NLE_TRACK_VIDEO);
  bg_cfg_section_transfer(p->video_outstream_section,
                          outstream->section);

  /* Get default label */
  num = 0;
  for(i = 0; i < p->num_outstreams; i++)
    {
    if(p->outstreams[i]->type == BG_NLE_TRACK_VIDEO)
      num++;
    }
  
  tmp_string = bg_sprintf("Video outstream %d", num+1);
  bg_nle_outstream_set_name(outstream, tmp_string);
  free(tmp_string);
  
  bg_nle_project_append_outstream(p, outstream);
  p->changed_flags |= BG_NLE_PROJECT_OUTSTREAMS_CHANGED;

  for(i = 0; i < p->num_tracks; i++)
    {
    if(p->tracks[i]->type == BG_NLE_TRACK_VIDEO)
      bg_nle_outstream_attach_track(outstream,
                                    p->tracks[i]);
    }

  d.outstream = outstream;
  edited(p, BG_NLE_EDIT_ADD_OUTSTREAM, &d);
  
  }

void bg_nle_project_add_audio_outstream(bg_nle_project_t * p)
  {
  bg_nle_outstream_t * outstream;
  char * tmp_string;
  int i;
  bg_nle_op_add_outstream_t d;
  int num;
  
  outstream = bg_nle_outstream_create(BG_NLE_TRACK_AUDIO);
  bg_cfg_section_transfer(p->audio_outstream_section,
                          outstream->section);

  num = 0;
  for(i = 0; i < p->num_outstreams; i++)
    {
    if(p->outstreams[i]->type == BG_NLE_TRACK_VIDEO)
      num++;
    }
  
  tmp_string = bg_sprintf("Audio outstream %d", num);
  bg_nle_outstream_set_name(outstream, tmp_string);
  free(tmp_string);
  
  bg_nle_project_append_outstream(p, outstream);
  p->changed_flags |= BG_NLE_PROJECT_OUTSTREAMS_CHANGED;
  
  for(i = 0; i < p->num_tracks; i++)
    {
    if(p->tracks[i]->type == BG_NLE_TRACK_VIDEO)
      bg_nle_outstream_attach_track(outstream,
                                    p->tracks[i]);
    }
  d.outstream = outstream;
  edited(p, BG_NLE_EDIT_ADD_OUTSTREAM, &d);
  
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

  for(i = 0; i < p->num_outstreams; i++)
    {
    resolve_source_tracks(p->outstreams[i],
                          p->tracks, p->num_tracks);
    }
  
  }

int bg_nle_project_outstream_index(bg_nle_project_t * p, bg_nle_outstream_t * outstream)
  {
  int i;
  for(i = 0; i < p->num_outstreams; i++)
    {
    if(p->outstreams[i] == outstream)
      return i;
    }
  return -1;
  }

void bg_nle_project_delete_outstream(bg_nle_project_t * p, bg_nle_outstream_t * t)
  {
  bg_nle_op_delete_outstream_t d;
  int index;
  
  index = bg_nle_project_outstream_index(p, t);
  
  d.outstream = t;
  d.index = index;
  
  if(index < p->num_outstreams - 1)
    memmove(p->outstreams + index,
            p->outstreams + index + 1, (p->num_outstreams - 1 - index) * sizeof(*p->outstreams));
  p->num_outstreams--;
  
  edited(p, BG_NLE_EDIT_DELETE_OUTSTREAM, &d);
  }

int bg_nle_project_track_index(bg_nle_project_t * p, bg_nle_track_t * track)
  {
  int i;
  
  for(i = 0; i < p->num_tracks; i++)
    {
    if(p->tracks[i] == track)
      return i;
    }
  return -1;
  }

void bg_nle_project_delete_track(bg_nle_project_t * p, bg_nle_track_t * t)
  {
  bg_nle_op_delete_track_t d;
  int index, i;

  /* Detach from source tracks */
  for(i = 0; i < p->num_outstreams; i++)
    {
    if(bg_nle_outstream_has_track(p->outstreams[i], t))
      bg_nle_outstream_detach_track(p->outstreams[i], t);
    }

  index = bg_nle_project_track_index(p, t);

  /* First call callback */
  d.track = t;
  d.index = index;
  
  if(index < p->num_tracks - 1)
    memmove(p->tracks + index, p->tracks + index + 1,
            (p->num_tracks - 1 - index) * sizeof(*p->tracks));


  p->num_tracks--;
  
  edited(p, BG_NLE_EDIT_DELETE_TRACK, &d);
  }
