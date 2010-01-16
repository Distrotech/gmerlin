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

#define PARAM_FRAMERATE \
  { \
  .name = "framerate_sec", \
  .long_name = TRS("Framerate"), \
  .type = BG_PARAMETER_SECTION, \
  }, \
  BG_GAVL_PARAM_FRAMERATE_NOSOURCE

#define PARAM_TIMECODE \
  { \
  .name = "timecode_sec", \
  .long_name = TRS("Timecodes"), \
  .type = BG_PARAMETER_SECTION, \
  },                            \
  { \
  .name      = "int_framerate", \
  .long_name = "Integer framerate", \
  .type      = BG_PARAMETER_INT, \
  .val_min     = { .val_i = 1 }, \
  .val_max     = { .val_i = 999 }, \
  .val_default = { .val_i = 25 }, \
      .help_string = TRS("Set the integer framerate used when adding new timecodes"), \
  }, \
  {  \
  .name      = "drop", \
  .long_name = "Drop frame", \
  .type  = BG_PARAMETER_CHECKBUTTON, \
  .help_string = TRS("Set the if drop frame is used when adding new timecodes"),\
  }, \
  { \
  .name      = "hours", \
  .long_name = "Start hour", \
  .type  = BG_PARAMETER_INT, \
  .val_min = { .val_i = 0 }, \
  .val_max = { .val_i = 23 }, \
  .val_default = { .val_i = 0 }, \
  .help_string = TRS("Set the start hours used when adding new timecodes"), \
  }, \
  { \
  .name      = "minutes", \
  .long_name = "Start minute", \
  .type  = BG_PARAMETER_INT, \
  .val_min = { .val_i = 0 }, \
  .val_max     = { .val_i = 59 }, \
  .val_default = { .val_i = 0 }, \
  .help_string = TRS("Set the start minutes used when adding new timecodes"), \
  }, \
  { \
  .name      = "seconds", \
  .long_name = "Start second", \
  .type  = BG_PARAMETER_INT, \
  .val_min     = { .val_i = 0 }, \
  .val_max     = { .val_i = 59 }, \
  .val_default = { .val_i = 0 }, \
  .help_string = TRS("Set the start seconds used when adding new timecodes"), \
  }, \
  { \
  .name      = "frames", \
  .long_name = "Start frames", \
  .type      = BG_PARAMETER_INT, \
  .val_min     = { .val_i = 0 }, \
  .val_max     = { .val_i = 999 }, \
  .val_default = { .val_i = 0 }, \
  .help_string = TRS("Set the start frames used when adding new timecodes"), \
  }


#define PARAM_BGCOLOR \
  { \
  .name = "background", \
  .long_name = TRS("Background"), \
  .type = BG_PARAMETER_COLOR_RGBA, \
  .val_default = { .val_color = { 0.0, 0.0, 0.0, 1.0 }}, \
  }
    
#define PARAM_CHANNELS \
  { \
  .name = "channel_sec", \
  .long_name = TRS("Channels"), \
  .type = BG_PARAMETER_SECTION, \
  }, \
BG_GAVL_PARAM_CHANNEL_SETUP_NOSOURCE

const bg_parameter_info_t bg_nle_outstream_video_parameters[] =
  {
    PARAM_GENERAL,
    BG_GAVL_PARAM_PIXELFORMAT,
    PARAM_BGCOLOR,
    PARAM_SIZE,
    PARAM_FRAMERATE,
    PARAM_TIMECODE,
    { /* End */ },
  };

const bg_parameter_info_t bg_nle_outstream_audio_parameters[] =
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
    PARAM_BGCOLOR,
    PARAM_SIZE,
    PARAM_FRAMERATE,
    PARAM_TIMECODE,
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

gavl_time_t bg_nle_outstream_duration(bg_nle_outstream_t * os)
  {
  int i;
  gavl_time_t ret = 0;
  gavl_time_t test_time;
  
  for(i = 0; i < os->num_source_tracks; i++)
    {
    if(os->source_tracks[i]->flags & BG_NLE_TRACK_PLAYBACK)
      {
      test_time = bg_nle_track_duration(os->source_tracks[i]);
      if(test_time > ret)
        ret = test_time;
      }
    }
  return ret;
  }

static void set_audio_parameter(void * data, const char * name,
                                const bg_parameter_value_t * val)
  {
  bg_gavl_audio_set_parameter(data, name, val);
  }

static void get_audio_format(bg_nle_outstream_t * os,
                             gavl_audio_format_t * format)
  {
  bg_gavl_audio_options_t opt;
  memset(&opt, 0, sizeof(opt));
  memset(format, 0, sizeof(*format));
  
  bg_gavl_audio_options_init(&opt);

  bg_cfg_section_apply(os->section, bg_nle_outstream_audio_parameters,
                       set_audio_parameter, &opt);
  
  bg_gavl_audio_options_set_format(&opt, NULL, format);
  bg_gavl_audio_options_free(&opt);
  }


static void set_video_parameter(void * data, const char * name,
                                const bg_parameter_value_t * val)
  {
  bg_gavl_video_set_parameter(data, name, val);
  }

static void get_video_format(bg_nle_outstream_t * os,
                             gavl_video_format_t * format)
  {
  bg_gavl_video_options_t opt;
  memset(&opt, 0, sizeof(opt));
  memset(format, 0, sizeof(*format));
  
  bg_gavl_video_options_init(&opt);

  bg_cfg_section_apply(os->section, bg_nle_outstream_video_parameters,
                       set_video_parameter, &opt);

  bg_gavl_video_options_set_framerate(&opt, NULL, format);
  bg_gavl_video_options_set_frame_size(&opt, NULL, format);
  bg_gavl_video_options_set_pixelformat(&opt, NULL, format);
  bg_gavl_video_options_free(&opt);
  }

void bg_nle_outstream_set_audio_stream(bg_nle_outstream_t * os,
                                       bg_nle_audio_stream_t * s)
  {
  gavl_audio_format_t format;
  get_audio_format(os, &format);
  s->timescale = format.samplerate;
  s->duration = 
    gavl_time_scale(format.samplerate,
                    bg_nle_outstream_duration(os) + 5);
  }

void bg_nle_outstream_set_video_stream(bg_nle_outstream_t * os,
                                       bg_nle_video_stream_t * s)
  {
  gavl_video_format_t format;
  int64_t num_frames;
  get_video_format(os, &format);
  
  num_frames =
    gavl_time_scale(format.timescale,
                    bg_nle_outstream_duration(os) + 5) /
    format.frame_duration;
  
  s->timescale = format.timescale;
  s->frametable =  gavl_frame_table_create_cfr(0, format.frame_duration,
                                                num_frames,
                                                GAVL_TIMECODE_UNDEFINED);
  }

