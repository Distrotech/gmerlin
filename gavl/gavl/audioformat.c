#include <stdio.h>
#include <gavl/gavl.h>

static struct
  {
  gavl_sample_format_t format;
  char * name;
  }
sample_format_names[] =
  {
    { GAVL_SAMPLE_NONE,  "Not specified" },
    { GAVL_SAMPLE_U8,    "Unsigned 8 bit"},
    { GAVL_SAMPLE_S8,    "Signed 8 bit"},
#ifdef GAVL_PROCESSOR_BIG_ENDIAN
    { GAVL_SAMPLE_U16LE, "Unsigned 16 bit LE"},
    { GAVL_SAMPLE_S16LE, "Signed 16 bit LE"},
    { GAVL_SAMPLE_U16BE, "Unsigned 16 bit BE (native)"},
    { GAVL_SAMPLE_S16BE, "Signed 16 bit BE (native)"},
#else
    { GAVL_SAMPLE_U16LE, "Unsigned 16 bit LE (native)"},
    { GAVL_SAMPLE_S16LE, "Signed 16 bit LE (native)"},
    { GAVL_SAMPLE_U16BE, "Unsigned 16 bit BE"},
    { GAVL_SAMPLE_S16BE, "Signed 16 bit BE"},
#endif
    { GAVL_SAMPLE_FLOAT, "Floating point"},
  };

const char * gavl_sample_format_to_string(gavl_sample_format_t format)
  {
  int i;
  for(i = 0; i < sizeof(sample_format_names)/sizeof(sample_format_names[0]); i++)
    {
    if(format == sample_format_names[i].format)
      return sample_format_names[i].name;
    }
  return (char*)0;
  }

static struct
  {
  gavl_channel_setup_t channel_setup;
  char * name;
  }
channel_setup_names[] =
  {
    { GAVL_CHANNEL_NONE,  "Not Specified" },
    { GAVL_CHANNEL_MONO,  "Mono" },
    { GAVL_CHANNEL_1,     "Channel 1" },   /* First (left) channel */
    { GAVL_CHANNEL_2,     "Channel 2" },   /* Second (right) channel */
    { GAVL_CHANNEL_2F,    "2 Front" },   /* 2 Front channels (Stereo or Dual channels) */
    { GAVL_CHANNEL_3F,    "3 Front" },
    { GAVL_CHANNEL_2F1R,  "2 Front, 1 Rear" },
    { GAVL_CHANNEL_3F1R,  "3 Front, 1 Rear" },
    { GAVL_CHANNEL_2F2R,  "2 Front, 2 Rear" },
    { GAVL_CHANNEL_3F2R,  "3 Front, 2 Rear" }
  };

const char * gavl_channel_setup_to_string(gavl_channel_setup_t channel_setup)
  {
  int i;
  for(i = 0; i < sizeof(channel_setup_names)/sizeof(channel_setup_names[0]); i++)
    {
    if(channel_setup == channel_setup_names[i].channel_setup)
      return channel_setup_names[i].name;
    }
  return (char*)0;
  }

static struct
  {
  gavl_interleave_mode_t mode;
  char * name;
  }
interleave_mode_names[] =
  {
    { GAVL_INTERLEAVE_NONE, "Not interleaved" },
    { GAVL_INTERLEAVE_2,    "Interleaved channel pairs" },
    { GAVL_INTERLEAVE_ALL,  "All channels interleaved" },
  };


const char * gavl_interleave_mode_to_string(gavl_interleave_mode_t mode)
  {
  int i;
  for(i = 0;
      i < sizeof(interleave_mode_names)/sizeof(interleave_mode_names[0]);
      i++)
    {
    if(mode == interleave_mode_names[i].mode)
      return interleave_mode_names[i].name;
    }
  return (char*)0;
  }

void gavl_audio_format_dump(gavl_audio_format_t * f)
  {
  fprintf(stderr, "         Channels: %d (%s", f->num_channels,
          gavl_channel_setup_to_string(f->channel_setup));
  if(f->lfe)
    fprintf(stderr, "+LFE)\n");
  else
    fprintf(stderr, ")\n");
  fprintf(stderr, "       Samplerate: %d\n", f->samplerate);
  fprintf(stderr, "Samples per frame: %d\n", f->samples_per_frame);
  fprintf(stderr, "  Interleave Mode: %s\n",
          gavl_interleave_mode_to_string(f->interleave_mode));
  fprintf(stderr, "    Sample format: %s\n",
          gavl_sample_format_to_string(f->sample_format));
  }
