#include <gavl/gavl.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/translation.h>

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
    {
      .name =        "frame_size",
      .long_name =   TRS("Image size"),
      .type =        BG_PARAMETER_STRINGLIST,
      .flags =     BG_PARAMETER_SYNC,
      .multi_names = (char const *[]){
        "user_defined",
        "pal_d1",
        "pal_d1_wide",
        "pal_dv",
        "pal_dv_wide",
        "pal_cvd",
        "pal_vcd",
        "pal_svcd",
        "pal_svcd_wide",
        "ntsc_d1",
        "ntsc_d1_wide",
        "ntsc_dv",
        "ntsc_dv_wide",
        "ntsc_cvd",
        "ntsc_vcd",
        "ntsc_svcd",
        "ntsc_svcd_wide",
        "720",
        "1080",
        "vga",
        "qvga",
        "sqcif",
        "qcif",
        "cif",
        "4cif",
        "16cif",
        (char*)0 },
      .multi_labels =  (char const *[]){ 
        TRS("User defined"),
        TRS("PAL DVD D1 4:3 (720 x 576)"),
        TRS("PAL DVD D1 16:9 (720 x 576)"),
        TRS("PAL DV 4:3 (720 x 576)"),
        TRS("PAL DV 16:9 (720 x 576)"),
        TRS("PAL CVD (352 x 576)"),
        TRS("PAL VCD (352 x 288)"),
        TRS("PAL SVCD 4:3 (480 x 576)"),
        TRS("PAL SVCD 16:9 (480 x 576)"),
        TRS("NTSC DVD D1 4:3 (720 x 480)"),
        TRS("NTSC DVD D1 16:9 (720 x 480)"),
        TRS("NTSC DV 4:3 (720 x 480)"),
        TRS("NTSC DV 16:9 (720 x 480)"),
        TRS("NTSC CVD (352 x 480)"),
        TRS("NTSC VCD (352 x 240)"),
        TRS("NTSC SVCD 4:3 (480 x 480)"),
        TRS("NTSC SVCD 16:9 (480 x 480)"),
        TRS("HD 720p/i (1280x720)"),
        TRS("HD 1080p/i (1920x1080)"),
        TRS("VGA (640 x 480)"),
        TRS("QVGA (320 x 240)"),
        TRS("SQCIF (128 × 96)"),
        TRS("QCIF (176 × 144)"),
        TRS("CIF (352 × 288)"),
        TRS("4CIF (704 × 576)"),
        TRS("16CIF (1408 × 1152)"),
        (char*)0 },
      .val_default = { .val_str = "pal_d1" },
      .help_string = TRS("Set the output image size. For a user defined size, you must specify the width and height as well as the pixel width and pixel height."),
    },
    {
      .name =      "user_image_width",
      .long_name = TRS("User defined width"),
      .opt =       "w",
      .type =      BG_PARAMETER_INT,   
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 100000 },
      .val_default = { .val_i = 640 },
      .help_string = TRS("User defined width in pixels. Only meaningful if you selected \"User defined\" for the image size."),
    },
    {                                       
      .name =      "user_image_height",
      .long_name = TRS("User defined height"),
      .opt =       "h",
      .type =      BG_PARAMETER_INT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 100000 },
      .val_default = { .val_i = 480 },
      .help_string = TRS("User defined height in pixels. Only meaningful if you selected \"User defined\" for the image size."),
      },
    {
      .name =      "user_pixel_width",
      .long_name = TRS("User defined pixel width"),
      .opt =       "sw",
      .type =      BG_PARAMETER_INT,   
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 100000 },
      .val_default = { .val_i = 1 },
      .help_string = TRS("User defined pixel width. Only meaningful if you selected \"User defined\" for the image size."),
    },
    {                                       
      .name =      "user_pixel_height",
      .long_name = TRS("User defined pixel height"),
      .opt =       "sh",
      .type =      BG_PARAMETER_INT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 100000 },
      .val_default = { .val_i = 1 },
      .help_string = TRS("User defined pixel height. Only meaningful if you selected \"User defined\" for the image size."),
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

const bg_parameter_info_t * bg_nle_project_get_audio_parameters()
  {
  return audio_parameters;
  }

const bg_parameter_info_t * bg_nle_project_get_video_parameters()
  {
  return video_parameters;
  }

void bg_nle_project_set_audio_parameter(void * data, const char * name,
                                        const bg_parameter_value_t * val)
  {
  if(!name)
    {

    }
  }

void bg_nle_project_set_video_parameter(void * data, const char * name,
                                        const bg_parameter_value_t * val)
  {
  if(!name)
    {

    }
  
  }
