#include <gavl/gavl.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/translation.h>
#include <gmerlin/bggavl.h>

#include <track.h>
#include <project.h>

static const bg_parameter_info_t audio_parameters[] =
  {
    {
    .name = "samplerate",
    .long_name = TRS("Samplerate"),
    .type = BG_PARAMETER_INT,
    .val_default = { .val_i = 48000 },
    },
    {
      .name =        "preview_quality",
      .long_name =   TRS("Preview Quality"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = GAVL_QUALITY_FASTEST },
      .val_max =     { .val_i = GAVL_QUALITY_BEST    },
      .val_default = { .val_i = GAVL_QUALITY_DEFAULT },
      .help_string = TRS("Set the conversion quality for format conversions. \
Lower quality means more speed. Values above 3 enable slow high quality calculations.")
    },      
    {
      .name =        "render_quality",
      .long_name =   TRS("Render Quality"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = GAVL_QUALITY_FASTEST },
      .val_max =     { .val_i = GAVL_QUALITY_BEST    },
      .val_default = { .val_i = GAVL_QUALITY_BEST },
      .help_string = TRS("Set the conversion quality for format conversions. \
Lower quality means more speed. Values above 3 enable slow high quality calculations.")
    },      
    { /* End */ },
  };

static const bg_parameter_info_t video_parameters[] =
  {
    BG_GAVL_PARAM_FRAMESIZE,
    {
      .name =        "preview_quality",
      .long_name =   TRS("Preview Quality"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = GAVL_QUALITY_FASTEST },
      .val_max =     { .val_i = GAVL_QUALITY_BEST    },
      .val_default = { .val_i = GAVL_QUALITY_DEFAULT },
      .help_string = TRS("Set the conversion quality for format conversions. \
Lower quality means more speed. Values above 3 enable slow high quality calculations.")
    },      
    {
      .name =        "render_quality",
      .long_name =   TRS("Render Quality"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = GAVL_QUALITY_FASTEST },
      .val_max =     { .val_i = GAVL_QUALITY_BEST    },
      .val_default = { .val_i = GAVL_QUALITY_BEST },
      .help_string = TRS("Set the conversion quality for format conversions. \
Lower quality means more speed. Values above 3 enable slow high quality calculations.")
    },      
    { /* End */ },
  };

