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
  .long_name = TRS("Name"), \
  .type = BG_PARAMETER_STRING, \
  }

#define PARAM_GENERAL \
  { \
  .name = "general", \
  .long_name = TRS("General"), \
  .type = BG_PARAMETER_SECTION, \
  }

#define PARAM_SIZE \
  { \
  .name = "size_sec", \
  .long_name = TRS("Frame size"), \
  .type = BG_PARAMETER_SECTION, \
  }, \
  BG_GAVL_PARAM_FRAMESIZE_NOSOURCE

#define PARAM_CHANNELS \
  { \
  .name = "channel_sec", \
  .long_name = TRS("Channels"), \
  .type = BG_PARAMETER_SECTION, \
  }, \
BG_GAVL_PARAM_CHANNEL_SETUP_NOSOURCE

#define PARAM_OVERLAY_MODE \
  { \
  .name = "overlay_mode", \
  .long_name = TRS("Overlay mode"), \
  .type = BG_PARAMETER_STRINGLIST, \
  .multi_names = (char const *[]) { "blend", "replace", (char*)0 }, \
  .multi_labels = (char const *[]) { TRS("Alpha blend"), TRS("Replace"), (char*)0 }, \
  .val_default = { .val_str = "blend" }, \
  .help_string = TRS("Set the overlay mode for this track\n\
Blend: Do Porter & Duff alpha blending\n\
Replace: Replace output pixels by track pixels\n\
For tracks without alpha channel blend is the same as replace"), \
  }

#define PARAM_OVERLAY_MODE_AUDIO \
  { \
  .name = "overlay_mode", \
  .long_name = TRS("Overlay mode"), \
  .type = BG_PARAMETER_STRINGLIST, \
  .multi_names = (char const *[]) { "replace", "add", (char*)0 }, \
  .multi_labels = (char const *[]) { TRS("Replace"), TRS("Add"), (char*)0 }, \
  .val_default = { .val_str = "add" }, \
  .help_string = TRS("Set the overlay mode for this track\n\
Replace: Replace output samples by track samples\n\
Add: Add track samples to output samples\n\
For tracks without alpha channel blend is the same as replace"), \
  }


const bg_parameter_info_t bg_nle_track_video_parameters[] =
  {
    PARAM_GENERAL,
    BG_GAVL_PARAM_PIXELFORMAT,
    PARAM_OVERLAY_MODE,
    PARAM_SIZE,
    { /* End */ },
  };

const bg_parameter_info_t bg_nle_track_audio_parameters[] =
  {
    PARAM_GENERAL,
    BG_GAVL_PARAM_SAMPLERATE_NOSOURCE,
    PARAM_CHANNELS,
    { /* End */ },
  };

static const bg_parameter_info_t video_parameters[] =
  {
    PARAM_GENERAL,
    PARAM_NAME,
    BG_GAVL_PARAM_PIXELFORMAT,
    PARAM_OVERLAY_MODE,
    PARAM_SIZE,
    { /* End */ },
  };

static const bg_parameter_info_t audio_parameters[] =
  {
    PARAM_GENERAL,
    PARAM_NAME,
    BG_GAVL_PARAM_SAMPLERATE_NOSOURCE,
    PARAM_CHANNELS,
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

gavl_time_t bg_nle_track_duration(bg_nle_track_t * t)
  {
  if(!t->num_segments)
    return 0;

  return t->segments[t->num_segments-1].dst_pos +
    t->segments[t->num_segments-1].len;
  }
