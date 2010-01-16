#include <inttypes.h>
#include <string.h>

#include <types.h>
#include <track.h>
#include <medialist.h>
#include <clipboard.h>
#include <project.h>

void bg_nle_clipboard_free(bg_nle_clipboard_t * c)
  {
  int i;
  for(i = 0; i < c->num_tracks; i++)
    bg_nle_track_destroy(c->tracks[i]);
  
  if(c->tracks)
    free(c->tracks);
  
  for(i = 0; i < c->num_files; i++)
    {
    for(i = 0; i < c->num_files; i++)
      bg_nle_file_destroy(c->files[i]);
    
    }
  if(c->files)
    free(c->files);
  memset(c, 0, sizeof(*c));
  }


static void init_file_track(bg_nle_track_t * t, bg_nle_file_t * file)
  {
  t->num_segments = 1;
  t->segments_alloc = 1;
  t->segments = calloc(t->segments_alloc, sizeof(*t->segments));
  t->segments[0].file_id = file->id;
  }

void bg_nle_clipboard_from_file(bg_nle_clipboard_t * c, bg_nle_file_t * file,
                                bg_nle_time_range_t * r, int * audio_streams,
                                int * video_streams)
  {
  int i, index;
  
  bg_nle_clipboard_free(c);

  /* Length */
  c->len = r->end - r->start;
  
  /* File */
  c->num_files = 1;
  c->files = malloc(c->num_files * sizeof(*c->files));
  c->files[0] = bg_nle_file_copy(file);

  /* Tracks */
  for(i = 0; i < file->num_audio_streams; i++)
    {
    if(audio_streams[i])
      c->num_tracks++;
    }
  for(i = 0; i < file->num_video_streams; i++)
    {
    if(video_streams[i])
      c->num_tracks++;
    }

  c->tracks = malloc(c->num_tracks * sizeof(*c->tracks));

  index = 0;
  for(i = 0; i < file->num_audio_streams; i++)
    {
    if(audio_streams[i])
      {
      
      c->tracks[index] = bg_nle_track_create(BG_NLE_TRACK_AUDIO);
      init_file_track(c->tracks[index], file);

      c->tracks[index]->segments[0].scale = file->audio_streams[i].timescale;
      c->tracks[index]->segments[0].src_pos =
        gavl_time_scale(c->tracks[index]->segments[0].scale, r->start + 5);
      c->tracks[index]->segments[0].dst_pos = 0;
      c->tracks[index]->segments[0].len = r->end - r->start;
      c->tracks[index]->segments[0].file_id = file->id;
      index++;
      }
    }
  for(i = 0; i < file->num_video_streams; i++)
    {
    if(video_streams[i])
      {
      c->tracks[index] = bg_nle_track_create(BG_NLE_TRACK_VIDEO);
      init_file_track(c->tracks[index], file);
      
      c->tracks[index]->segments[0].scale = file->video_streams[i].timescale;
      c->tracks[index]->segments[0].src_pos =
        gavl_time_scale(c->tracks[index]->segments[0].scale, r->start + 5);
      c->tracks[index]->segments[0].dst_pos = 0;
      c->tracks[index]->segments[0].len = r->end - r->start;
      c->tracks[index]->segments[0].file_id = file->id;
      
      index++;
      }
    }
  }

static int has_file(bg_nle_file_t ** files, int num_files,
                     bg_nle_file_t * file)
  {
  int i;
  for(i = 0; i < num_files; i++)
    {
    if(files[i]->id == file->id)
      return 1;
    }
  return 0;
  }

void bg_nle_clipboard_from_project(bg_nle_clipboard_t * c, bg_nle_project_t * p,
                                   bg_nle_time_range_t * r)
  {
  int i, j, index;
  bg_nle_file_t * f;
  bg_nle_track_segment_t * seg;
  
  bg_nle_clipboard_free(c);

  /* Count tracks and create files */

  for(i = 0; i < p->num_tracks; i++)
    {
    if(!(p->tracks[i]->flags & BG_NLE_TRACK_SELECTED))
      continue;

    c->num_tracks++;

    for(j = 0; j < p->tracks[i]->num_segments; j++)
      {
      f = bg_nle_media_list_find_file_by_id(p->media_list,
                                        p->tracks[i]->id);
      
      if(!has_file(c->files, c->num_files, f))
        {
        c->num_files++;
        c->files = realloc(c->files,
                          (c->num_files+1) * sizeof(*c->files));
        c->files[c->num_files] = bg_nle_file_copy(f);
        c->num_files++;
        }
      }
    }
  
  /* Create tracks */

  index = 0;
  c->tracks = malloc(c->num_tracks * sizeof(*c->tracks));

  for(i = 0; i < p->num_tracks; i++)
    {
    if(!(p->tracks[i]->flags & BG_NLE_TRACK_SELECTED))
      continue;

    c->tracks[index] = bg_nle_track_create(p->tracks[i]->type);
    
    /* Create segments */
    
    for(j = 0; j < p->tracks[i]->num_segments; i++)
      {
      seg = &p->tracks[i]->segments[j];

      /* Check is segment is inside the time range */
      
      if((r->start < seg->dst_pos + seg->len) &&
         (r->end >= seg->dst_pos))
        {
        
        }
      }
    
    
    }
  
  }

