#include <stdlib.h>
#include <string.h>
#include <renderer.h>

#include <gmerlin/translation.h>
#include <gmerlin/bggavl.h>

bg_nle_outstream_t *
bg_nle_outstream_create(bg_nle_track_type_t type)
  {
  bg_nle_outstream_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->type = type;
  ret->section = bg_cfg_section_create("");
  ret->flags = BG_NLE_TRACK_EXPANDED;
  return ret;
  }

void bg_nle_outstream_destroy(bg_nle_outstream_t * s)
  {
  bg_cfg_section_destroy(s->section);

  if(s->source_track_ids)
    free(s->source_track_ids);
  if(s->source_tracks)
    free(s->source_tracks);
  
  free(s);
  }

#define PARAM_NAME \
  { \
  .name = "name", \
  .long_name = "Name", \
  .type = BG_PARAMETER_STRING, \
  }

#define PARAM_AUDIO \
    BG_GAVL_PARAM_CHANNEL_SETUP

#define PARAM_VIDEO \
    BG_GAVL_PARAM_PIXELFORMAT, \
    BG_GAVL_PARAM_FRAMESIZE_NOSOURCE, \
    BG_GAVL_PARAM_FRAMERATE_NOSOURCE

const bg_parameter_info_t bg_nle_outstream_video_parameters[] =
  {
    PARAM_VIDEO,
    { /* End */ },
  };

const bg_parameter_info_t bg_nle_outstream_audio_parameters[] =
  {
    PARAM_AUDIO,
    { /* End */ },
  };

static const bg_parameter_info_t video_parameters[] =
  {
    PARAM_NAME,
    PARAM_VIDEO,
    { /* End */ },
  };

static const bg_parameter_info_t audio_parameters[] =
  {
    PARAM_NAME,
    PARAM_AUDIO,
    { /* End */ },
  };


const bg_parameter_info_t *
bg_nle_outstream_get_parameters(bg_nle_outstream_t * t)
  {
  switch(t->type)
    {
    case BG_NLE_TRACK_AUDIO:
      return audio_parameters;
      break;
    case BG_NLE_TRACK_VIDEO:
      return video_parameters;
      break;
    default:
      return NULL;
    }
  }

void bg_nle_outstream_set_name(bg_nle_outstream_t * t, const char * name)
  {
  bg_cfg_section_set_parameter_string(t->section, "name", name);
  }

const char * bg_nle_outstream_get_name(bg_nle_outstream_t * t)
  { 
  const char * ret;
  bg_cfg_section_get_parameter_string(t->section, "name", &ret);
  return ret;
  }

void bg_nle_outstream_attach_track(bg_nle_outstream_t * os,
                                   bg_nle_track_t * t)
  {
  if(os->num_source_tracks >= os->source_tracks_alloc)
    {
    os->source_tracks_alloc += 16;
    os->source_tracks = realloc(os->source_tracks,
                                os->source_tracks_alloc *
                                sizeof(*os->source_tracks));
    }
  os->source_tracks[os->num_source_tracks] = t;
  os->num_source_tracks++;
  }

void bg_nle_outstream_detach_track(bg_nle_outstream_t * os,
                                   bg_nle_track_t * t)
  {
  int i;
  for(i = 0; i < os->num_source_tracks; i++)
    {
    if(t == os->source_tracks[i])
      {
      if(i < os->num_source_tracks-1)
        memmove(os->source_tracks+i, os->source_tracks+i+1,
                (os->num_source_tracks-1-i) * sizeof(*os->source_tracks));
      os->num_source_tracks--;
      break;
      }
    }
  }

int bg_nle_outstream_has_track(bg_nle_outstream_t * os,
                               bg_nle_track_t * t)
  {
  int i;
  for(i = 0; i < os->num_source_tracks; i++)
    {
    if(t == os->source_tracks[i])
      return 1;
    }
  return 0;
  }
