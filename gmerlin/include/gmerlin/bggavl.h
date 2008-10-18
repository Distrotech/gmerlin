/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/


#ifndef __BG_GAVL_H_
#define __BG_GAVL_H_


/* Struct for converting audio */

typedef struct
  {
  gavl_audio_options_t * opt;
  int fixed_samplerate;
  int samplerate;
  int fixed_channel_setup;

  gavl_sample_format_t force_format;
  
  int num_front_channels;
  int num_rear_channels;
  int num_lfe_channels;

  int options_changed;
  } bg_gavl_audio_options_t;

int
bg_gavl_audio_set_parameter(void * data, const char * name,
                            const bg_parameter_value_t * val);

void bg_gavl_audio_options_init(bg_gavl_audio_options_t *);

void bg_gavl_audio_options_free(bg_gavl_audio_options_t *);

void bg_gavl_audio_options_set_format(const bg_gavl_audio_options_t *,
                                      const gavl_audio_format_t * in_format,
                                      gavl_audio_format_t * out_format);


typedef struct
  {
  gavl_video_options_t * opt;
  
  int framerate_mode;
  int frame_duration;
  int timescale;
  
  int options_changed;
  } bg_gavl_video_options_t;

int bg_gavl_video_set_parameter(void * data, const char * name,
                                const bg_parameter_value_t * val);

void bg_gavl_video_options_init(bg_gavl_video_options_t *);

void bg_gavl_video_options_free(bg_gavl_video_options_t *);

void bg_gavl_video_options_set_format(const bg_gavl_video_options_t *,
                                      const gavl_video_format_t * in_format,
                                      gavl_video_format_t * out_format);

void bg_gavl_video_options_set_rectangles(const bg_gavl_video_options_t * opt,
                                          const gavl_video_format_t * in_format,
                                          const gavl_video_format_t * out_format,
                                          int do_crop);

gavl_scale_mode_t bg_gavl_string_to_scale_mode(const char * str);
gavl_downscale_filter_t bg_gavl_string_to_downscale_filter(const char * str);

#if 0
void bg_gavl_video_options_set_framerate(const bg_gavl_video_options_t *,
                                         const gavl_video_format_t * in_format,
                                         gavl_video_format_t * out_format);
void bg_gavl_video_options_set_framesize(const bg_gavl_video_options_t *,
                                         const gavl_video_format_t * in_format,
                                         gavl_video_format_t * out_format);
void bg_gavl_video_options_set_interlace(const bg_gavl_video_options_t * opt,
                                         const gavl_video_format_t * in_format,
                                         gavl_video_format_t * out_format);
#endif

/* Useful code for gluing gavl and gmerlin */

#define BG_GAVL_PARAM_CONVERSION_QUALITY \
  {                                      \
  .name =        "conversion_quality",     \
  .long_name =   TRS("Conversion Quality"),          \
    .opt =         "q", \
  .type =        BG_PARAMETER_SLIDER_INT,               \
  .flags =       BG_PARAMETER_SYNC,                     \
  .val_min =     { .val_i = GAVL_QUALITY_FASTEST },       \
  .val_max =     { .val_i = GAVL_QUALITY_BEST    },       \
  .val_default = { .val_i = GAVL_QUALITY_DEFAULT },                      \
  .help_string = TRS("Set the conversion quality for format conversions. \
Lower quality means more speed. Values above 3 enable slow high quality calculations.") \
    }

#define BG_GAVL_PARAM_FRAMERATE                                 \
  {                                                             \
  .name =      "framerate",                                       \
  .long_name = TRS("Framerate"),                                       \
  .type =      BG_PARAMETER_STRINGLIST,                           \
  .flags =       BG_PARAMETER_SYNC,                     \
  .val_default = { .val_str = "from_source" },                      \
  .multi_names = (char const *[]){ "from_source",                      \
                   "user_defined",                              \
                   "23_976",      \
                   "24", \
                   "25",        \
                   "29_970",                                    \
                   "30",                                        \
                   "50",                                        \
                   "59_940",                                    \
                   "60", (char*)0 },                              \
    .multi_labels = (char const *[]){ TRS("From Source"),                      \
                   TRS("User defined"),                              \
                   TRS("23.976 (NTSC encapsulated film rate)"),        \
                   TRS("24 (Standard international cinema film rate)"), \
                   TRS("25 (PAL [625/50] video frame rate)"),        \
                   TRS("29.970 (NTSC video frame rate)"),            \
                   TRS("30 (NTSC drop-frame [525/60] video frame rate)"),  \
                   TRS("50 (Double frame rate / progressive PAL)"),   \
                   TRS("59.940 (Double frame rate NTSC)"),     \
                   TRS("60 (Double frame rate drop-frame NTSC)"),           \
                    (char*)0 },                                         \
  .help_string = TRS("Output framerate. For user defined framerate, enter the \
timescale and frame duration below (framerate = timescale / frame duration).")\
  },                                              \
  {                                           \
  .name =      "timescale",                     \
  .long_name = TRS("Timescale"),                   \
  .type =      BG_PARAMETER_INT,              \
  .val_min =     { .val_i = 1 },                \
  .val_max =     { .val_i = 100000 },           \
  .val_default = { .val_i = 25 },                                         \
  .help_string = TRS("Timescale for user defined output framerate (Framerate = timescale / frame duration)."), \
  },                                                                  \
  {                                                                 \
  .name =      "frame_duration",                                      \
  .long_name = TRS("Frame duration"),                                    \
  .type =      BG_PARAMETER_INT,                                    \
  .val_min =     { .val_i = 1 },                                      \
  .val_max =     { .val_i = 100000 },                                 \
  .val_default = { .val_i = 1 },                                      \
  .help_string = TRS("Frame duration for user defined output framerate (Framerate = timescale / frame duration)."), \
  }

#define BG_GAVL_PARAM_DEINTERLACE           \
 {                                            \
  .name =      "deinterlace_mode",              \
  .long_name = TRS("Deinterlace mode"),             \
  .opt =       "dm", \
  .type =      BG_PARAMETER_STRINGLIST,        \
  .val_default = { .val_str = "none" },          \
  .multi_names =  (char const *[]){ "none", "copy", "scale", (char*)0 },         \
  .multi_labels = (char const *[]){ TRS("None"), TRS("Copy"), TRS("Scale"), (char*)0 },         \
  .help_string = "Specify interlace mode. Higher modes are better but slower." \
  },                                                                   \
  {                                                                  \
  .name =      "deinterlace_drop_mode",                                            \
  .opt =       "ddm", \
  .long_name = "Drop mode",                                          \
  .type =      BG_PARAMETER_STRINGLIST,                              \
  .val_default = { .val_str = "top" },                                 \
  .multi_names =   (char const *[]){ "top", "bottom", (char*)0 },               \
  .multi_labels =  (char const *[]){ TRS("Drop top field"), TRS("Drop bottom field"), (char*)0 },   \
  .help_string = TRS("Specifies which field the deinterlacer should drop.") \
  },                                                              \
  {\
  .name =      "force_deinterlacing",           \
  .long_name = TRS("Force deinterlacing"),         \
  .opt =       "fd", \
  .type =      BG_PARAMETER_CHECKBUTTON,              \
  .val_default = { .val_i = 0 },                \
  .help_string = TRS("Force deinterlacing if you want progressive output and the input format pretends to be progressive also.") \
  }                                                                  \


#define BG_GAVL_SCALE_MODE_NAMES \
   (char const *[]){ "auto",\
              "nearest",         \
              "bilinear", \
              "quadratic", \
              "cubic_bspline", \
              "cubic_mitchell", \
              "cubic_catmull", \
              "sinc_lanczos", \
              (char*)0 }

#define BG_GAVL_SCALE_MODE_LABELS \
  (char const *[]){ TRS("Auto"), \
             TRS("Nearest"),            \
             TRS("Bilinear"), \
             TRS("Quadratic"), \
             TRS("Cubic B-Spline"), \
             TRS("Cubic Mitchell-Netravali"), \
             TRS("Cubic Catmull-Rom"), \
             TRS("Sinc with Lanczos window"), \
            (char*)0 }

#define BG_GAVL_TRANSFORM_MODE_NAMES \
   (char const *[]){ "auto",\
              "nearest",         \
              "bilinear", \
              "quadratic", \
              "cubic_bspline", \
              (char*)0 }

#define BG_GAVL_TRANSFORM_MODE_LABELS \
  (char const *[]){ TRS("Auto"), \
             TRS("Nearest"),            \
             TRS("Bilinear"), \
             TRS("Quadratic"), \
             TRS("Cubic B-Spline"), \
            (char*)0 }

#define BG_GAVL_DOWNSCALE_FILTER_NAMES \
  (char const *[]){ "auto", \
                    "none", \
                    "wide", \
                    "gauss", \
                    (char*)0 }

#define BG_GAVL_DOWNSCALE_FILTER_LABELS \
  (char const *[]){ TRS("Auto"), \
                    TRS("None"),            \
                    TRS("Widening"), \
                    TRS("Gaussian preblur"), \
                    (char*)0 }

#define BG_GAVL_PARAM_SCALE_MODE                                    \
  {                                                                 \
  .name =        "scale_mode",                                          \
  .long_name =   TRS("Scale mode"),                                          \
  .opt =       "sm",                                                  \
  .type =        BG_PARAMETER_STRINGLIST,                               \
  .flags =       BG_PARAMETER_SYNC,                     \
  .multi_names = BG_GAVL_SCALE_MODE_NAMES, \
  .multi_labels = BG_GAVL_SCALE_MODE_LABELS, \
  .val_default = { .val_str = "auto" },                                   \
  .help_string = TRS("Choose scaling method. Auto means to choose based on the conversion quality. Nearest is fastest, Sinc with Lanczos window is slowest."), \
  },                                                                  \
  {                                                                   \
  .name =        "scale_order",                                         \
  .long_name =   TRS("Scale order"),                                        \
  .opt =       "so",                                                  \
  .type =        BG_PARAMETER_INT,                               \
  .flags =       BG_PARAMETER_SYNC,                     \
  .val_min =     { .val_i = 4 },                                 \
  .val_max =     { .val_i = 1000 },                              \
  .val_default = { .val_i = 4 },                                \
  .help_string = TRS("Order for sinc scaling"),\
  }

#define BG_GAVL_PARAM_RESAMPLE_CHROMA \
  {                                                                 \
  .name =        "resample_chroma",                                          \
  .long_name =   TRS("Resample chroma"),                                          \
  .opt =       "sm",                                                  \
  .type =        BG_PARAMETER_CHECKBUTTON,                               \
  .flags =       BG_PARAMETER_SYNC,                     \
    .help_string = TRS("Always perform chroma resampling if chroma subsampling factors or chroma placements are different. Usually, this is only done for qualities above 3."), \
  }


#define BG_GAVL_PARAM_ALPHA                \
    { \
      .name =        "alpha_mode", \
      .long_name =   TRS("Alpha mode"), \
      .type =        BG_PARAMETER_STRINGLIST, \
      .flags =       BG_PARAMETER_SYNC,                     \
      .val_default = { .val_str = "ignore" }, \
      .multi_names = (char const *[]){"ignore", "blend_color", (char*)0}, \
      .multi_labels = (char const *[]){TRS("Ignore"), TRS("Blend background color"), (char*)0}, \
    .help_string = TRS("This option is used if the source has an alpha (=transparency) channel, but the output supports no transparency. Either, the transparency is ignored, or the background color you specify below is blended in."),\
    }, \
    { \
      .name =        "background_color", \
      .long_name =   TRS("Background color"), \
      .type =      BG_PARAMETER_COLOR_RGB, \
      .flags =       BG_PARAMETER_SYNC,                     \
      .val_default = { .val_color = { 0.0, 0.0, 0.0 } }, \
      .help_string = TRS("Background color to use, when alpha mode above is \"Blend background color\"."), \
    }


#define BG_GAVL_PARAM_SAMPLERATE                \
    {\
      .name =      "fixed_samplerate",\
      .long_name = TRS("Fixed samplerate"),\
      .type =      BG_PARAMETER_CHECKBUTTON,\
      .flags =       BG_PARAMETER_SYNC,                     \
      .val_default = { .val_i = 0 },\
      .help_string = TRS("If disabled, the output samplerate is taken from the source. If enabled, the samplerate you specify below us used.")\
    },\
    {\
      .name =        "samplerate",\
      .long_name =   TRS("Samplerate"),\
      .type =        BG_PARAMETER_INT,\
      .flags =       BG_PARAMETER_SYNC,                     \
      .val_min =     { .val_i = 8000 },\
      .val_max =     { .val_i = 192000 },\
      .val_default = { .val_i = 44100 },\
      .help_string = TRS("Fixed output samplerate"),\
    }


#define BG_GAVL_PARAM_CHANNEL_SETUP \
    { \
      .name =      "fixed_channel_setup", \
      .long_name = TRS("Fixed channel setup"), \
      .type =      BG_PARAMETER_CHECKBUTTON,\
      .flags =       BG_PARAMETER_SYNC,                     \
      .val_default = { .val_i = 0 },\
      .help_string = TRS("If disabled, the output channel configuration is taken from the source. If enabled, the setup you specify below us used.") \
    },                                        \
    {                                         \
    .name =        "num_front_channels",          \
    .long_name =   TRS("Front channels"),              \
    .type =        BG_PARAMETER_INT,              \
    .flags =       BG_PARAMETER_SYNC,                     \
    .val_min =     { .val_i = 1 },                  \
    .val_max =     { .val_i = 5 },                  \
    .val_default = { .val_i = 2 },                  \
    },\
    {                                         \
    .name =        "num_rear_channels",          \
    .long_name =   TRS("Rear channels"),              \
    .type =        BG_PARAMETER_INT,              \
    .flags =       BG_PARAMETER_SYNC,                     \
    .val_min =     { .val_i = 0 },                  \
    .val_max =     { .val_i = 3 },                  \
    .val_default = { .val_i = 0 },                  \
    },                                        \
    {                                         \
    .name =        "num_lfe_channels",          \
    .long_name =   TRS("LFE"),                        \
    .type =        BG_PARAMETER_CHECKBUTTON,     \
    .flags =       BG_PARAMETER_SYNC,                     \
    .val_default = { .val_i = 0 },                  \
    },                                        \
    {                                 \
      .name =        "front_to_rear", \
      .long_name =   TRS("Front to rear mode"), \
      .type =        BG_PARAMETER_STRINGLIST, \
      .flags =       BG_PARAMETER_SYNC,                     \
      .val_default = { .val_str = "copy" }, \
      .multi_names =  (char const *[]){ "mute", \
                              "copy", \
                              "diff", \
                              (char*)0 }, \
      .multi_labels =  (char const *[]){ TRS("Mute"), \
                              TRS("Copy"), \
                              TRS("Diff"), \
                              (char*)0 }, \
      .help_string = TRS("Mix mode when the output format has rear channels, \
but the source doesn't."), \
    }, \
    { \
      .name =        "stereo_to_mono", \
      .long_name =   TRS("Stereo to mono mode"), \
      .type =        BG_PARAMETER_STRINGLIST, \
      .flags =       BG_PARAMETER_SYNC,                     \
      .val_default = { .val_str = "mix" }, \
      .multi_names =  (char const *[]){ "left", \
                              "right", \
                              "mix", \
                              (char*)0 }, \
      .multi_labels =  (char const *[]){ TRS("Choose left"), \
                              TRS("Choose right"), \
                              TRS("Mix"), \
                              (char*)0 }, \
      .help_string = TRS("Mix mode when downmixing Stereo to Mono."), \
    }


#define BG_GAVL_PARAM_FORCE_SAMPLEFORMAT \
    { \
      .name =      "force_sampleformat", \
      .long_name = TRS("Force sampleformat"), \
      .type =      BG_PARAMETER_STRINGLIST,\
      .flags =       BG_PARAMETER_SYNC, \
      .val_default = { .val_str = "none" },\
      .multi_names  = (char const *[]){ "none", "8", "16", "32", "f", "d", (char*)0 },\
      .multi_labels = (char const *[]){ TRS("None"), TRS("8 bit"), TRS("16 bit"), TRS("32 bit"), TRS("Float"), TRS("Double"), (char*)0 }, \
      .help_string = TRS("Force a sampleformat to be used for processing. None means to take the input format.") \
    }

#define BG_GAVL_PARAM_AUDIO_DITHER_MODE \
    { \
      .name =      "dither_mode", \
      .long_name = TRS("Dither mode"), \
      .type =      BG_PARAMETER_STRINGLIST,\
      .flags =       BG_PARAMETER_SYNC,                     \
      .val_default = { .val_str = "auto" },\
      .multi_names =  (char const *[]){ "auto", "none", "rect",        "tri",        "shaped", (char*)0 },\
      .multi_labels = (char const *[]){ TRS("Auto"), TRS("None"), TRS("Rectangular"), \
                               TRS("Triangular"), TRS("Shaped"), (char*)0 },\
      .help_string = TRS("Dither mode. Auto means to use the quality level. Subsequent options are ordered by increasing quality (i.e. decreasing speed).") \
    }

#define BG_GAVL_PARAM_RESAMPLE_MODE \
    { \
      .name =      "resample_mode", \
      .long_name = TRS("Resample mode"), \
      .type =      BG_PARAMETER_STRINGLIST,\
      .flags =       BG_PARAMETER_SYNC,                     \
      .val_default = { .val_str = "auto" },\
      .multi_names =  (char const *[]){ "auto", "zoh", "linear", "sinc_fast",  "sinc_medium", "sinc_best", (char*)0 },\
      .multi_labels = (char const *[]){ TRS("Auto"), TRS("Zero order hold"), TRS("Linear"), \
                               TRS("Sinc fast"),  TRS("Sinc medium"), TRS("Sinc best"), (char*)0 },\
      .help_string = TRS("Resample mode. Auto means to use the quality level. Subsequent options are ordered by increasing quality (i.e. decreasing speed).") \
    }

/* Subtitle display decisions */
int bg_overlay_too_old(gavl_time_t time, gavl_time_t ovl_time,
                       gavl_time_t ovl_duration);

int bg_overlay_too_new(gavl_time_t time, gavl_time_t ovl_time);

#endif // __BG_GAVL_H_
