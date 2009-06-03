
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
    { GAVL_SAMPLE_U16LE, "Unsigned 16 bit little endian"},
    { GAVL_SAMPLE_S16LE, "Signed 16 bit little endian"},
    { GAVL_SAMPLE_U16BE, "Unsigned 16 bit big endian (native)"},
    { GAVL_SAMPLE_S16BE, "Signed 16 bit big endian (native)"},
#else
    { GAVL_SAMPLE_U16LE, "Unsigned 16 bit little endian (native)"},
    { GAVL_SAMPLE_S16LE, "Signed 16 bit little endian (native)"},
    { GAVL_SAMPLE_U16BE, "Unsigned 16 bit big endian"},
    { GAVL_SAMPLE_S16BE, "Signed 16 bit big endian"},
#endif
    { GAVL_SAMPLE_FLOAT, "Floating point (native)"},
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

const char * gavl_channel_setup_to_string(gavl_channel_setup_t setup)
  {
  
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
    { GAVL_INTERLEAVE_ALL,  "ALl channels interleaved" },
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
