/*****************************************************************
  
  bggavl.c
  
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
  
  http://gmerlin.sourceforge.net
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
  
*****************************************************************/

/* Struct for converting audio */

typedef struct
  {
  gavl_audio_options_t * opt;
  int fixed_samplerate;
  int samplerate;
  int fixed_channel_setup;

  int force_float;
  
  int num_front_channels;
  int num_rear_channels;
  int num_lfe_channels;
  
  } bg_gavl_audio_options_t;

int bg_gavl_audio_set_parameter(void * data, char * name, bg_parameter_value_t * val);

void bg_gavl_audio_options_init(bg_gavl_audio_options_t *);

void bg_gavl_audio_options_free(bg_gavl_audio_options_t *);

void bg_gavl_audio_options_set_format(bg_gavl_audio_options_t *,
                                      const gavl_audio_format_t * in_format,
                                      gavl_audio_format_t * out_format);


typedef struct
  {
  gavl_video_options_t * opt;

  int framerate_mode;
  int frame_duration;
  int timescale;

  int frame_size;
  
  int user_image_width;
  int user_image_height;

  int user_pixel_width;
  int user_pixel_height;

  double crop_left;
  double crop_right;
  double crop_top;
  double crop_bottom;
  int maintain_aspect;
  } bg_gavl_video_options_t;

int bg_gavl_video_set_parameter(void * data, char * name, bg_parameter_value_t * val);

void bg_gavl_video_options_init(bg_gavl_video_options_t *);

void bg_gavl_video_options_free(bg_gavl_video_options_t *);

void bg_gavl_video_options_set_framerate(bg_gavl_video_options_t *,
                                         const gavl_video_format_t * in_format,
                                         gavl_video_format_t * out_format);

void bg_gavl_video_options_set_framesize(bg_gavl_video_options_t *,
                                         const gavl_video_format_t * in_format,
                                         gavl_video_format_t * out_format);

void bg_gavl_video_options_set_rectangles(bg_gavl_video_options_t * opt,
                                          const gavl_video_format_t * in_format,
                                          const gavl_video_format_t * out_format);

void bg_gavl_video_options_set_interlace(bg_gavl_video_options_t * opt,
                                         const gavl_video_format_t * in_format,
                                         gavl_video_format_t * out_format);



/* Useful code for gluing gavl and gmerlin */

#define BG_GAVL_PARAM_CONVERSION_QUALITY \
  {                                      \
  name:        "conversion_quality",     \
  long_name:   "Conversion Quality",          \
    opt:         "q", \
  type:        BG_PARAMETER_SLIDER_INT,               \
  val_min:     { val_i: GAVL_QUALITY_FASTEST },       \
  val_max:     { val_i: GAVL_QUALITY_BEST    },       \
  val_default: { val_i: GAVL_QUALITY_DEFAULT },                      \
  help_string: "Set the conversion quality for format conversions. \
Lower quality means more speed. Values above 3 enable slow high quality calculations." \
    }

#define BG_GAVL_PARAM_FRAMERATE                                 \
  {                                                             \
  name:      "framerate",                                       \
  long_name: "Framerate",                                       \
  type:      BG_PARAMETER_STRINGLIST,                           \
  val_default: { val_str: "from_source" },                      \
    multi_names: (char*[]){ "from_source",                      \
                   "user_defined",                              \
                   "23_976",      \
                   "24", \
                   "25",        \
                   "29_970",                                    \
                   "30",                                        \
                   "50",                                        \
                   "59_940",                                    \
                   "60", (char*)0 },                              \
    multi_labels: (char*[]){ "From Source",                      \
                   "User defined",                              \
                   "23.976 (NTSC encapsulated film rate)",        \
                   "24 (Standard international cinema film rate)", \
                   "25 (PAL [625/50] video frame rate)",        \
                   "29.970 (NTSC video frame rate)",            \
                   "30 (NTSC drop-frame [525/60] video frame rate)",  \
                   "50 (Double frame rate / progressive PAL)",   \
                   "59.940 (Double frame rate NTSC)",     \
                   "60 (Double frame rate drop-frame NTSC)",           \
                    (char*)0 },                                         \
  help_string: "Output framerate. For user defined framerate, enter the \
timescale and frame duration below (framerate = timescale / frame_duration)."\
  },                                              \
  {                                           \
  name:      "timescale",                     \
  long_name: "Timescale",                   \
  type:      BG_PARAMETER_INT,              \
  val_min:     { val_i: 1 },                \
  val_max:     { val_i: 100000 },           \
  val_default: { val_i: 25 },                                         \
  help_string: "Timescale for user defined output framerate (Framerate = timescale / frame_duration).", \
  },                                                                  \
  {                                                                 \
  name:      "frame_duration",                                      \
  long_name: "Frame duration",                                    \
  type:      BG_PARAMETER_INT,                                    \
  val_min:     { val_i: 1 },                                      \
  val_max:     { val_i: 100000 },                                 \
  val_default: { val_i: 1 },                                      \
  help_string: "Frame duration for user defined output framerate (Framerate = timescale / frame_duration).", \
  }

#define BG_GAVL_PARAM_DEINTERLACE           \
 {                                            \
  name:      "deinterlace_mode",              \
  long_name: "Deinterlace mode",             \
  opt:       "dm", \
  type:      BG_PARAMETER_STRINGLIST,        \
  val_default: { val_str: "none" },          \
  multi_names:  (char*[]){ "none", "copy", "scale", (char*)0 },         \
  multi_labels: (char*[]){ "None", "Copy", "Scale", (char*)0 },         \
  help_string: "Specify interlace mode. Higher modes are better but slower." \
  },                                                                   \
  {                                                                  \
  name:      "deinterlace_drop_mode",                                            \
  opt:       "ddm", \
  long_name: "Drop mode",                                          \
  type:      BG_PARAMETER_STRINGLIST,                              \
  val_default: { val_str: "top" },                                 \
  multi_names:   (char*[]){ "top", "bottom", (char*)0 },               \
  multi_labels:  (char*[]){ "Drop top field", "Drop bottom field", (char*)0 },               \
  help_string: "Specifies which field the deinterlacer should drop." \
  },                                                              \
  {\
  name:      "force_deinterlacing",           \
  long_name: "Force deinterlacing",         \
  opt:       "fd", \
  type:      BG_PARAMETER_CHECKBUTTON,              \
  val_default: { val_i: 0 },                \
  help_string: "Force deinterlacing if you want progressive output and the input format pretends to be progressive also." \
  }                                                                  \

#define BG_GAVL_PARAM_CROP                                         \
 {                                                                \
  name:      "crop_left",                                          \
    long_name: "Crop left",                                        \
    opt:       "crl",                                                    \
    type:      BG_PARAMETER_FLOAT,                                       \
    val_min:     { val_f: 0.0 },                                     \
      val_max:     { val_f: 100000.0 },\
      val_default: { val_f: 0.0 },\
      num_digits: 3,\
      help_string: "Cut this many pixels from the left border of the source frames."\
    },\
    {\
      name:      "crop_right",\
      long_name: "Crop right",\
      opt:       "crr",                         \
      type:      BG_PARAMETER_FLOAT,\
      val_min:     { val_f: 0.0 },\
      val_max:     { val_f: 100000.0 },\
      val_default: { val_f: 0.0 },\
      num_digits: 3,\
      help_string: "Cut this many pixels from the right border of the source frames."\
    },\
    {\
      name:      "crop_top",\
      long_name: "Crop top",\
      opt:       "crt",                         \
      type:      BG_PARAMETER_FLOAT,\
      val_min:     { val_f: 0.0 },\
      val_max:     { val_f: 100000.0 },\
      val_default: { val_f: 0.0 },\
      num_digits: 3,\
      help_string: "Cut this many pixels from the top border of the source frames."\
    },\
    {\
      name:      "crop_bottom",\
      long_name: "Crop bottom",\
      opt:       "crb",                         \
      type:      BG_PARAMETER_FLOAT,\
      val_min:     { val_f: 0.0 },\
      val_max:     { val_f: 100000.0 },\
      val_default: { val_f: 0.0 },\
      num_digits: 3,\
      help_string: "Cut this many pixels from the bottom border of the source frames."\
    }

#define BG_GAVL_PARAM_SCALE_MODE                                    \
  {                                                                 \
  name:        "scale_mode",                                          \
  long_name:   "Scale mode",                                          \
  opt:       "sm",                                                  \
  type:        BG_PARAMETER_STRINGLIST,                               \
    multi_names:  (char*[]){ "auto",\
                             "nearest",         \
                             "bilinear", \
                             "quadratic", \
                             "cubic_bspline", \
                             "cubic_mitchell", \
                             "cubic_catmull", \
                             "sinc_lanczos", \
                             (char*)0                                     \
                         },                                    \
    multi_labels: (char*[]){ "Auto", \
                             "Nearest",                             \
                             "Bilinear", \
                             "Quadratic", \
                             "Cubic B-Spline", \
                             "Cubic Mitchell-Netravali", \
                             "Cubic Catmull-Rom", \
                             "Sinc with Lanczos window",                         \
                             (char*)0 },                                         \
    val_default: { val_str: "auto" },                                   \
    help_string: "Choose scaling method. Auto means to choose based on the conversion quality. Nearest is fastest, Sinc with Lanczos window is slowest.", \
    },                                                                  \
    {                                                                   \
    name:        "scale_order",                                         \
  long_name:   "Scale order",                                        \
  opt:       "so",                                                  \
  type:        BG_PARAMETER_INT,                               \
  val_min:     { val_i: 4 },                                 \
  val_max:     { val_i: 1000 },                              \
  val_default: { val_i: 4 },                                \
  help_string: "Order for sinc scaling.\n",\
  }

#define BG_GAVL_PARAM_FRAME_SIZE  \
    { \
      name:        "frame_size", \
      long_name:   "Frame Size", \
      opt:         "s", \
      type:        BG_PARAMETER_STRINGLIST, \
      multi_names: (char*[]){ "from_input", \
                              "user_defined", \
                              "pal_d1", \
                              "pal_d1_wide", \
                              "pal_dv", \
                              "pal_dv_wide", \
                              "pal_vcd", \
                              "pal_svcd", \
                              "pal_svcd_wide", \
                              "ntsc_d1", \
                              "ntsc_d1_wide", \
                              "ntsc_dv", \
                              "ntsc_dv_wide", \
                              "ntsc_vcd", \
                              "ntsc_svcd", \
                              "ntsc_svcd_wide", \
                              "vga", \
                              "qvga", \
                               (char*)0 }, \
      multi_labels:  (char*[]){ "From Source", \
                                "User defined", \
                                "PAL DVD D1 4:3 (720 x 576)", \
                                "PAL DVD D1 16:9 (720 x 576)", \
                                "PAL DV 4:3 (720 x 576)", \
                                "PAL DV 16:9 (720 x 576)", \
                                "PAL VCD (352 x 288)", \
                                "PAL SVCD 4:3 (480 x 576)", \
                                "PAL SVCD 16:9 (480 x 576)", \
                                "NTSC DVD D1 4:3 (720 x 480)", \
                                "NTSC DVD D1 16:9 (720 x 480)", \
                                "NTSC DV 4:3 (720 x 480)", \
                                "NTSC DV 16:9 (720 x 480)", \
                                "NTSC VCD (352 x 240)", \
                                "NTSC SVCD 4:3 (480 x 480)", \
                                "NTSC SVCD 16:9 (480 x 480)", \
                                "VGA (640 x 480)", \
                                "QVGA (320 x 240)", \
                                (char*)0 }, \
      val_default: { val_str: "from_input" }, \
      help_string: "Set the output frame size. For a user defined size, you must specify the width and height as well as the pixel width and pixel height (for nonsquare pixels).", \
    }, \
    { \
      name:      "user_image_width", \
      long_name: "User defined width", \
      opt:       "w", \
      type:      BG_PARAMETER_INT,    \
      val_min:     { val_i: 1 }, \
      val_max:     { val_i: 100000 }, \
      val_default: { val_i: 640 }, \
      help_string: "User defined width in pixels. Only meaningful if you selected \"User defined\" for the framesize.", \
    }, \
    {                                        \
      name:      "user_image_height", \
      long_name: "User defined height", \
      opt:       "h", \
      type:      BG_PARAMETER_INT, \
      val_min:     { val_i: 1 }, \
      val_max:     { val_i: 100000 }, \
      val_default: { val_i: 480 }, \
      help_string: "User defined height in pixels. Only meaningful if you selected \"User defined\" for the framesize.", \
      }, \
    { \
      name:      "user_pixel_width", \
      long_name: "User defined pixel width", \
      opt:       "sw", \
      type:      BG_PARAMETER_INT,    \
      val_min:     { val_i: 1 }, \
      val_max:     { val_i: 100000 }, \
      val_default: { val_i: 1 }, \
      help_string: "User defined pixel width. Only meaningful if you selected \"User defined\" for the framesize.", \
    }, \
    {                                        \
      name:      "user_pixel_height", \
      long_name: "User defined pixel height", \
      opt:       "sh", \
      type:      BG_PARAMETER_INT, \
      val_min:     { val_i: 1 }, \
      val_max:     { val_i: 100000 }, \
      val_default: { val_i: 1 }, \
      help_string: "User defined pixel height. Only meaningful if you selected \"User defined\" for the framesize.", \
      }, \
    { \
      name:      "maintain_aspect", \
      long_name: "Maintain aspect ratio", \
      opt:       "ka", \
      type:      BG_PARAMETER_CHECKBUTTON, \
      val_default: { val_i: 1 }, \
      help_string: "Let the aspect ratio appear the same as in the source, probably resulting in additional black borders." \
      },                                                                \
      BG_GAVL_PARAM_SCALE_MODE


#define BG_GAVL_PARAM_ALPHA                \
    { \
      name:        "alpha_mode", \
      long_name:   "Alpha mode", \
      type:        BG_PARAMETER_STRINGLIST, \
      val_default: { val_str: "Ignore" }, \
      multi_names: (char*[]){"Ignore", "Blend background color", (char*)0}, \
    help_string: "This option is used if the source has an alpha (=transparency) channel, but the output supports no transparency. Either, the transparency is ignored, or the background color you specify below is blended in.",\
    }, \
    { \
      name:        "background_color", \
      long_name:   "Background color", \
      type:      BG_PARAMETER_COLOR_RGB, \
      val_default: { val_color: (float[]){ 0.0, 0.0, 0.0 } }, \
      help_string: "Background color to use, when alpha mode above is \"Blend background color\".", \
    }


#define BG_GAVL_PARAM_SAMPLERATE                \
    {\
      name:      "fixed_samplerate",\
      long_name: "Fixed samplerate",\
      type:      BG_PARAMETER_CHECKBUTTON,\
      val_default: { val_i: 0 },\
      help_string: "If disabled, the output samplerate is taken from the source. If enabled, the samplerate you specify below us used."\
    },\
    {\
      name:        "samplerate",\
      long_name:   "Samplerate",\
      type:        BG_PARAMETER_INT,\
      val_min:     { val_i: 8000 },\
      val_max:     { val_i: 192000 },\
      val_default: { val_i: 44100 },\
      help_string: "Fixed output samplerate",\
    }


#define BG_GAVL_PARAM_CHANNEL_SETUP \
    { \
      name:      "fixed_channel_setup", \
      long_name: "Fixed channel setup", \
      type:      BG_PARAMETER_CHECKBUTTON,\
      val_default: { val_i: 0 },\
      help_string: "If disabled, the output channel configuration is taken from the source. If enabled, the setup you specify below us used." \
    },                                        \
    {                                         \
    name:        "num_front_channels",          \
    long_name:   "Front channels",              \
    type:        BG_PARAMETER_INT,              \
    val_min:     { val_i: 1 },                  \
    val_max:     { val_i: 5 },                  \
    val_default: { val_i: 2 },                  \
    },\
    {                                         \
    name:        "num_rear_channels",          \
    long_name:   "Rear channels",              \
    type:        BG_PARAMETER_INT,              \
    val_min:     { val_i: 0 },                  \
    val_max:     { val_i: 3 },                  \
    val_default: { val_i: 0 },                  \
    },                                        \
    {                                         \
    name:        "num_lfe_channels",          \
    long_name:   "LFE",                        \
    type:        BG_PARAMETER_CHECKBUTTON,     \
    val_default: { val_i: 0 },                  \
    },                                        \
    {                                 \
      name:        "front_to_rear", \
      long_name:   "Front to rear mode", \
      type:        BG_PARAMETER_STRINGLIST, \
      val_default: { val_str: "Copy" }, \
      multi_names:  (char*[]){ "Mute", \
                              "Copy", \
                              "Diff", \
                              (char*)0 }, \
      help_string: "Mix mode when the output format has rear channels, \
but the source doesn't.", \
    }, \
    { \
      name:        "stereo_to_mono", \
      long_name:   "Stereo to mono mode", \
      type:        BG_PARAMETER_STRINGLIST, \
      val_default: { val_str: "Mix" }, \
      multi_names:  (char*[]){ "Choose left", \
                              "Choose right", \
                              "Mix", \
                              (char*)0 }, \
      help_string: "Mix mode when downmixing Stereo to Mono.", \
    }


#define BG_GAVL_PARAM_FORCE_FLOAT \
    { \
      name:      "force_float", \
      long_name: "Force floating point", \
      type:      BG_PARAMETER_CHECKBUTTON,\
      val_default: { val_i: 0 },\
      help_string: "Force floating point processing. This will inprove the quality but might slow things down." \
    }
