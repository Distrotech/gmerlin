#include <gavl/gavl.h>

#include <gmerlin/translation.h>

#include <track.h>
#include <stdlib.h>

#include <gmerlin/bggavl.h>

bg_nle_track_t * bg_nle_track_create(bg_nle_track_type_t type)
  {
  bg_nle_track_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->type = type;
  ret->flags = BG_NLE_TRACK_SELECTED | BG_NLE_TRACK_EXPANDED | BG_NLE_TRACK_PLAYBACK;
  ret->section = bg_cfg_section_create("");
  return ret;
  }

void bg_nle_track_destroy(bg_nle_track_t * t)
  {
  bg_cfg_section_destroy(t->section);
  free(t);
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
  BG_GAVL_PARAM_FRAMESIZE_NOSOURCE


const bg_parameter_info_t bg_nle_track_video_parameters[] =
  {
    PARAM_VIDEO,
    { /* End */ },
  };

const bg_parameter_info_t bg_nle_track_audio_parameters[] =
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
bg_nle_track_get_parameters(bg_nle_track_t * t)
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

void bg_nle_track_set_name(bg_nle_track_t * t, const char * name)
  {
  bg_cfg_section_set_parameter_string(t->section, "name", name);
  }

const char * bg_nle_track_get_name(bg_nle_track_t * t)
  { 
  const char * ret;
  bg_cfg_section_get_parameter_string(t->section, "name", &ret);
  return ret;
  }
