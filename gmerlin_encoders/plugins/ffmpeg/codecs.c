/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#include <string.h>

#include "ffmpeg_common.h"
#include "params.h"
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "ffmpeg.codecs"

#define ENCODE_PARAM_MP2                 \
  {                                      \
    .name =      "ff_bit_rate_str",        \
    .long_name = TRS("Bit rate (kbps)"),        \
    .type =      BG_PARAMETER_STRINGLIST, \
    .val_default = { .val_str = "128" },       \
    .multi_names = (char const *[]){ "32",  "48", "56", "64", "80", "96", "112", \
                   "128", "160", "192", "224", "256", "320", "384",\
                   NULL } \
  },

#define ENCODE_PARAM_MP3                 \
  {                                      \
    .name =      "ff_bit_rate_str",        \
    .long_name = TRS("Bit rate (kbps)"),        \
    .type =      BG_PARAMETER_STRINGLIST, \
    .val_default = { .val_str = "128" },       \
    .multi_names = (char const *[]){ "32", "40", "48", "56", "64", "80", "96", \
                   "112", "128", "160", "192", "224", "256", "320",\
                   NULL } \
  },

#define ENCODE_PARAM_AC3 \
  {                                      \
    .name =      "ff_bit_rate_str",        \
    .long_name = TRS("Bit rate (kbps)"),        \
    .type =      BG_PARAMETER_STRINGLIST, \
    .val_default = { .val_str = "128" },       \
    .multi_names = (char const *[]){ "32", "40", "48", "56", "64", "80", "96", "112", "128", \
                   "160", "192", "224", "256", "320", "384", "448", "512", \
                   "576", "640", NULL } \
  },

#define ENCODE_PARAM_WMA \
  {                                      \
    .name =      "ff_bit_rate_str",        \
    .long_name = TRS("Bit rate (kbps)"),        \
    .type =      BG_PARAMETER_STRINGLIST, \
    .val_default = { .val_str = "128" },       \
    .multi_names = (char const *[]){ "24", "48", "64", "96", "128", NULL } \
  },

static const bg_parameter_info_t parameters_libfaac[] =
  {
    {
      .name =      "faac_profile",
      .long_name = TRS("Profile"),
      .type =      BG_PARAMETER_STRINGLIST,
      .val_default = {val_str: "lc"},
      .multi_names = (char const *[]){"main",
                                      "lc",
                                      "ssr",
                                      "ltp",
                                      (char *)0},
      .multi_labels = (char const *[]){TRS("Main"),
                                       TRS("LC"),
                                       TRS("SSR"),
                                       TRS("LTP"),
                                       (char *)0 },
    },
    PARAM_BITRATE_AUDIO,
    {
      .name =        "faac_quality",
      .long_name =   TRS("Quality"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 10 },
      .val_max =     { .val_i = 500 },
      .val_default = { .val_i = 100 },
      .help_string = TRS("Quantizer quality"),
    },
    { /* */ }
  };

static const bg_parameter_info_t parameters_libvorbis[] =
  {
    PARAM_BITRATE_AUDIO,
    PARAM_RC_MIN_RATE,
    PARAM_RC_MAX_RATE,
    {
      .name =        "quality",
      .long_name =   TRS("Quality"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = -1 },
      .val_max =     { .val_i = 10 },
      .val_default = { .val_i = 3 },
      .help_string = TRS("Quantizer quality"),
    },
    { /* End */ },
  };
    
#define ENCODE_PARAM_VIDEO_RATECONTROL \
  {                                           \
    .name =      "rate_control",                       \
    .long_name = TRS("Rate control"),                     \
    .type =      BG_PARAMETER_SECTION,         \
  },                                        \
    PARAM_BITRATE_VIDEO,                    \
    PARAM_BITRATE_TOLERANCE,                \
    PARAM_RC_MIN_RATE,                      \
    PARAM_RC_MAX_RATE,                      \
    PARAM_RC_BUFFER_SIZE,                   \
    PARAM_RC_INITIAL_COMPLEX,               \
    PARAM_RC_INITIAL_BUFFER_OCCUPANCY

#define ENCODE_PARAM_VIDEO_QUANTIZER_I \
  {                                           \
    .name =      "quantizer",                       \
    .long_name = TRS("Quantizer"),                     \
    .type =      BG_PARAMETER_SECTION,         \
  },                                        \
    PARAM_QMIN,                             \
    PARAM_QMAX,                             \
    PARAM_MAX_QDIFF,                        \
    PARAM_FLAG_QSCALE,                      \
    PARAM_QSCALE,                      \
    PARAM_QCOMPRESS,                        \
    PARAM_QBLUR,                            \
    PARAM_TRELLIS

#define ENCODE_PARAM_VIDEO_QUANTIZER_IP \
  ENCODE_PARAM_VIDEO_QUANTIZER_I,               \
  PARAM_I_QUANT_FACTOR,                         \
  PARAM_I_QUANT_OFFSET

#define ENCODE_PARAM_VIDEO_QUANTIZER_IPB \
  ENCODE_PARAM_VIDEO_QUANTIZER_IP,        \
  PARAM_B_QUANT_FACTOR,                   \
  PARAM_B_QUANT_OFFSET

#define ENCODE_PARAM_VIDEO_FRAMETYPES_IP \
  {                                           \
    .name =      "frame_types",                       \
    .long_name = TRS("Frame types"),                     \
    .type =      BG_PARAMETER_SECTION,         \
  },                                        \
  PARAM_GOP_SIZE,                      \
  PARAM_SCENE_CHANGE_THRESHOLD,       \
  PARAM_SCENECHANGE_FACTOR,        \
  PARAM_FLAG_CLOSED_GOP,         \
  PARAM_FLAG2_STRICT_GOP

#define ENCODE_PARAM_VIDEO_FRAMETYPES_IPB \
  ENCODE_PARAM_VIDEO_FRAMETYPES_IP,        \
  PARAM_MAX_B_FRAMES,                 \
  PARAM_B_FRAME_STRATEGY

#define ENCODE_PARAM_VIDEO_ME \
  {                                           \
    .name =      "motion_estimation",                       \
    .long_name = TRS("Motion estimation"),                     \
    .type =      BG_PARAMETER_SECTION,         \
  },                                        \
    PARAM_ME_METHOD,                        \
    PARAM_ME_CMP,\
    PARAM_ME_CMP_CHROMA,\
    PARAM_ME_RANGE,\
    PARAM_ME_THRESHOLD,\
    PARAM_MB_DECISION,\
    PARAM_DIA_SIZE

#define ENCODE_PARAM_VIDEO_ME_PRE \
  {                                           \
    .name =      "motion_estimation",                       \
    .long_name = TRS("ME pre-pass"),                     \
    .type =      BG_PARAMETER_SECTION,         \
  },                                        \
    PARAM_PRE_ME,\
    PARAM_ME_PRE_CMP,\
    PARAM_ME_PRE_CMP_CHROMA,\
    PARAM_PRE_DIA_SIZE

#define ENCODE_PARAM_VIDEO_QPEL                 \
  {                                           \
    .name =      "qpel_motion_estimation",                       \
    .long_name = TRS("Qpel ME"),                     \
    .type =      BG_PARAMETER_SECTION,         \
  },                                        \
    PARAM_FLAG_QPEL, \
    PARAM_ME_SUB_CMP,\
    PARAM_ME_SUB_CMP_CHROMA,\
    PARAM_ME_SUBPEL_QUALITY

#define ENCODE_PARAM_VIDEO_MASKING \
  {                                \
    .name =      "masking",                       \
    .long_name = TRS("Masking"),                     \
    .type =      BG_PARAMETER_SECTION,         \
  },                                        \
    PARAM_LUMI_MASKING, \
    PARAM_DARK_MASKING, \
    PARAM_TEMPORAL_CPLX_MASKING, \
    PARAM_SPATIAL_CPLX_MASKING, \
    PARAM_BORDER_MASKING, \
    PARAM_P_MASKING,                                       \
    PARAM_FLAG_NORMALIZE_AQP

#define ENCODE_PARAM_VIDEO_MISC \
  {                                           \
    .name =      "misc",                       \
    .long_name = TRS("Misc"),                     \
    .type =      BG_PARAMETER_SECTION,         \
  },                                        \
    PARAM_STRICT_STANDARD_COMPLIANCE,       \
    PARAM_NOISE_REDUCTION, \
    PARAM_FLAG_GRAY, \
    PARAM_FLAG_BITEXACT

static const bg_parameter_info_t parameters_mpeg4[] = {
  ENCODE_PARAM_VIDEO_FRAMETYPES_IPB,
  PARAM_FLAG_AC_PRED_MPEG4,
  ENCODE_PARAM_VIDEO_RATECONTROL,
  ENCODE_PARAM_VIDEO_QUANTIZER_IPB,
  PARAM_FLAG_CBP_RD,
  ENCODE_PARAM_VIDEO_ME,
  PARAM_FLAG_GMC,
  PARAM_FLAG_4MV,
  PARAM_FLAG_MV0,
  PARAM_FLAG_QP_RD,
  ENCODE_PARAM_VIDEO_ME_PRE,
  ENCODE_PARAM_VIDEO_QPEL,
  ENCODE_PARAM_VIDEO_MASKING,
  ENCODE_PARAM_VIDEO_MISC,
  { /* End of parameters */ }
};

static const bg_parameter_info_t parameters_mjpeg[] = {
  ENCODE_PARAM_VIDEO_RATECONTROL,
  ENCODE_PARAM_VIDEO_QUANTIZER_I,
  ENCODE_PARAM_VIDEO_MISC,
  { /* End of parameters */ }
};

static const bg_parameter_info_t parameters_mpeg1[] = {
  ENCODE_PARAM_VIDEO_FRAMETYPES_IPB,
  ENCODE_PARAM_VIDEO_RATECONTROL,
  ENCODE_PARAM_VIDEO_QUANTIZER_IPB,
  ENCODE_PARAM_VIDEO_ME,
  ENCODE_PARAM_VIDEO_ME_PRE,
  ENCODE_PARAM_VIDEO_MASKING,
  ENCODE_PARAM_VIDEO_MISC,
  { /* End of parameters */ }
};


static const bg_parameter_info_t parameters_msmpeg4v3[] = {
  ENCODE_PARAM_VIDEO_FRAMETYPES_IP,
  ENCODE_PARAM_VIDEO_RATECONTROL,
  ENCODE_PARAM_VIDEO_QUANTIZER_IP,
  ENCODE_PARAM_VIDEO_ME,
  ENCODE_PARAM_VIDEO_ME_PRE,
  ENCODE_PARAM_VIDEO_MASKING,
  ENCODE_PARAM_VIDEO_MISC,
  { /* End of parameters */ }
};

static const bg_parameter_info_t parameters_libx264[] = {
  {
    .name =      "libx264_preset",
    .long_name = TRS("Preset"),
    .type =      BG_PARAMETER_STRINGLIST,
    .val_default = {val_str: "medium"},
    .multi_names = (char const *[]){"ultrafast",
                                    "superfast",
                                    "veryfast",
                                    "faster",
                                    "fast",
                                    "medium",
                                    "slow",
                                    "slower",
                                    "veryslow",
                                    "placebo",
                                    (char *)0},
    .multi_labels = (char const *[]){TRS("Ultrafast"),
                                     TRS("Superfast"),
                                     TRS("Veryfast"),
                                     TRS("Faster"),
                                     TRS("Fast"),
                                     TRS("Medium"),
                                     TRS("Slow"),
                                     TRS("Slower"),
                                     TRS("Veryslow"),
                                     TRS("Placebo"),
                                     (char *)0 },
  },
  {
    .name =      "libx264_tune",
    .long_name = TRS("Tune"),
    .type =      BG_PARAMETER_STRINGLIST,
    .val_default = {val_str: "$none"},
    .multi_names = (char const *[]){"$none",
                                    "film",
                                    "animation",
                                    "grain",
                                    "stillimage",
                                    "psnr",
                                    "ssim",
                                    "fastdecode",
                                    "zerolatency",
                                    (char *)0},
    .multi_labels = (char const *[]){TRS("None"),
                                     TRS("Film"),
                                     TRS("Animation"),
                                     TRS("Grain"),
                                     TRS("Still image"),
                                     TRS("PSNR"),
                                     TRS("SSIM"),
                                     TRS("Fast decode"),
                                     TRS("Zero latency"),
                                     (char *)0},
  },
  {
    .name =      "ff_bit_rate_video",
    .long_name = TRS("Bit rate (kbps)"),
    .type =      BG_PARAMETER_INT,
    .val_min     = { .val_i = 0 },
    .val_max     = { .val_i = 10000 },
    .val_default = { .val_i = 0 },
    .help_string = TRS("If > 0 encode with average bitrate"),
  },
  {
    .name =      "libx264_crf",
    .long_name = TRS("Quality-based VBR"),
    .type =      BG_PARAMETER_SLIDER_FLOAT,
    .val_min     = { .val_f = -1.0 },
    .val_max     = { .val_f = 51.0 },
    .val_default = { .val_f = 23.0 },
    .help_string = TRS("Negative means disable"),
    .num_digits  = 2,
  },
  {
    .name =      "libx264_qp",
    .long_name = TRS("Force constant QP"),
    .type =      BG_PARAMETER_SLIDER_INT,
    .val_min     = { .val_i = -1 },
    .val_max     = { .val_i = 69 },
    .val_default = { .val_i = -1 },
    .help_string = TRS("Negative means disable, 0 means lossless"),
  },
  { /* End */ },
};

static const bg_parameter_info_t parameters_libvpx[] = {
  {
    .name = "rc",
    .long_name = TRS("Rate control"),
    .type = BG_PARAMETER_SECTION,
  },
  PARAM_BITRATE_VIDEO,
  PARAM_RC_MIN_RATE,
  PARAM_RC_MAX_RATE,
  PARAM_RC_BUFFER_SIZE,
  PARAM_RC_INITIAL_BUFFER_OCCUPANCY,
  {
    .name =        "ff_qcompress",
    .long_name =   TRS("2-Pass VBR/CBR"),
    .type =        BG_PARAMETER_SLIDER_FLOAT,
    .val_default = { .val_f = 0.5 },
    .val_min =     { .val_f = 0.0 },
    .val_max =     { .val_f = 1.0 },
    .num_digits =  2,
    .help_string = TRS("0: CBR, 1: VBR")
  },
  {
    .name =      "ff_qmin",
    .long_name = TRS("Minimum quantizer scale"),
    .type =      BG_PARAMETER_SLIDER_INT,
    .val_default = { .val_i = 2 },
    .val_min =     { .val_i = 0 },
    .val_max =     { .val_i = 63 },
    .help_string = TRS("recommended value 0-4")
  },
  {
    .name =      "ff_qmax",
    .long_name = TRS("Maximum quantizer scale"),
    .type =      BG_PARAMETER_SLIDER_INT,
    .val_default = { .val_i = 55 },
    .val_min =     { .val_i = 0 },
    .val_max =     { .val_i = 63 },
    .help_string = TRS("Must be larger than minimum, recommended value 50-63")
  },
  {
    .name =      "libvpx_crf",
    .long_name = TRS("Constant quality"),
    .type =      BG_PARAMETER_SLIDER_INT,
    .val_default = { .val_i = 10 },
    .val_min =     { .val_i = 0 },
    .val_max =     { .val_i = 63 },
    .help_string = TRS("Set this to 0 for enabling VBR")
  },
  {
    .name = "speed",
    .long_name = TRS("Speed"),
    .type = BG_PARAMETER_SECTION,
  },
  {
    .name =      "libvpx_deadline",
    .long_name = TRS("Speed"),
    .type =      BG_PARAMETER_STRINGLIST,
    .val_default = { .val_str = "good"},
    .multi_names = (char const *[]){"best",
                                    "good",
                                    "realtime",
                                    (char *)0},
    .multi_labels = (char const *[]){TRS("Best quality"),
                                     TRS("Good quality"),
                                     TRS("Realtime"),
                                     (char *)0},
  },
  {
    .name = "libvpx_cpu-used",
    .long_name = TRS("CPU usage modifier"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 0 },
    .val_max = { .val_i = 1000 }, // Bogus
    .val_default = { .val_i = 0 },
  },
  PARAM_THREAD_COUNT,
  {
    .name = "frametypes",
    .long_name = TRS("Frame types"),
    .type = BG_PARAMETER_SECTION,
  },
#if 0 // Has no effect??
  {
    .name = "ff_keyint_min",
    .long_name = TRS("Minimum GOP size"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 0 },
    .val_max = { .val_i = 1000 }, // Bogus
    .val_default = { .val_i = 0 },
  },
#endif
  {
    .name = "ff_gop_size",
    .long_name = TRS("Maximum GOP size"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = -1 },
    .val_max = { .val_i = 1000 }, // Bogus
    .val_default = { .val_i = -1 },
    .help_string = TRS("Maximum keyframe distance, -1 means automatic"),
  },
  {
    .name = "libvpx_auto-alt-ref",
    .long_name = TRS("Enable alternate reference frames"),
    .help_string = TRS("Enable use of alternate reference frames (2-pass only)"),
    .type = BG_PARAMETER_CHECKBUTTON,
  },
  {
    .name = "libvpx_lag-in-frames",
    .long_name = TRS("Lookahead"),
    .help_string = TRS("Number of frames to look ahead for alternate reference frame selection"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 0 },
    .val_max = { .val_i = 100 },
    .val_default = { .val_i = 0 },
  },
  {
    .name = "libvpx_arnr-maxframes",
    .long_name = TRS("Frame count for noise reduction"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 0 },
    .val_max = { .val_i = 100 },
    .val_default = { .val_i = 0 },
  },
  {
    .name =      "libvpx_arnr-type",
    .long_name = TRS("Noise reduction filter"),
    .type =      BG_PARAMETER_STRINGLIST,
    .val_default = {val_str: "backward"},
    .multi_names = (char const *[]){"$none",
                                    "backward",
                                    "forward",
                                    "centered",
                                    (char *)0},
    .multi_labels = (char const *[]){TRS("None"),
                                     TRS("Backward"),
                                     TRS("Forward"),
                                     TRS("Centered"),
                                     (char *)0},
  },
  { /* End */ },
};

static const bg_parameter_info_t parameters_tga[] = {
  {
    .name =      "tga_rle",
    .long_name = TRS("Use RLE compression"),
    .type =      BG_PARAMETER_CHECKBUTTON,
  },
  { /* */ }
};

/* Audio */

static const bg_parameter_info_t parameters_ac3[] = {
  ENCODE_PARAM_AC3
  { /* End of parameters */ }
};

static const bg_parameter_info_t parameters_mp2[] = {
  ENCODE_PARAM_MP2
  { /* End of parameters */ }
};

static const bg_parameter_info_t parameters_mp3[] = {
  ENCODE_PARAM_MP3
  { /* End of parameters */ }
};

static const bg_parameter_info_t parameters_wma[] = {
  ENCODE_PARAM_WMA
  { /* End of parameters */ }
};

static const ffmpeg_codec_info_t audio_codecs[] =
  {
    {
      .name      = "pcm_s16be",
      .long_name = TRS("16 bit PCM"),
      .id        = CODEC_ID_PCM_S16BE,
    },
    {
      .name      = "pcm_s16le",
      .long_name = TRS("16 bit PCM"),
      .id        = CODEC_ID_PCM_S16LE,
    },
    {
      .name      = "pcm_s8",
      .long_name = TRS("8 bit PCM"),
      .id        = CODEC_ID_PCM_S8,
    },
    {
      .name      = "pcm_u8",
      .long_name = TRS("8 bit PCM"),
      .id        = CODEC_ID_PCM_U8,
    },
    {
      .name      = "pcm_alaw",
      .long_name = TRS("alaw"),
      .id        = CODEC_ID_PCM_ALAW,
    },
    {
      .name      = "pcm_mulaw",
      .long_name = TRS("mulaw"),
      .id        = CODEC_ID_PCM_MULAW,
    },
    {
      .name      = "ac3",
      .long_name = TRS("AC3"),
      .id        = CODEC_ID_AC3,
      .parameters = parameters_ac3,
    },
    {
      .name      = "mp2",
      .long_name = TRS("MPEG audio layer 2"),
      .id        = CODEC_ID_MP2,
      .parameters = parameters_mp2,
    },
    {
      .name      = "wma1",
      .long_name = TRS("Windows media Audio 1"),
      .id        = CODEC_ID_WMAV1,
      .parameters = parameters_wma,
    },
    {
      .name      = "wma2",
      .long_name = TRS("Windows media Audio 2"),
      .id        = CODEC_ID_WMAV2,
      .parameters = parameters_wma,
    },
    {
      .name      = "mp3",
      .long_name = TRS("MPEG audio layer 3"),
      .id        = CODEC_ID_MP3,
      .parameters = parameters_mp3,
    },
    {
      .name      = "libfaac",
      .long_name = TRS("AAC"),
      .id        = CODEC_ID_AAC,
      .parameters = parameters_libfaac,
    },
    {
      .name      = "libvorbis",
      .long_name = TRS("Vorbis"),
      .id        = CODEC_ID_VORBIS,
      .parameters = parameters_libvorbis,
    },
    { /* End of array */ }
  };

static const ffmpeg_codec_info_t video_codecs[] =
  {
    {
      .name       = "mjpeg",
      .long_name  = TRS("Motion JPEG"),
      .id         = CODEC_ID_MJPEG,
      .parameters = parameters_mjpeg,
      .flags      = FLAG_INTRA_ONLY,
    },
    {
      .name       = "mpeg4",
      .long_name  = TRS("MPEG-4"),
      .id         = CODEC_ID_MPEG4,
      .parameters = parameters_mpeg4,
      .flags      = FLAG_B_FRAMES,
    },
    {
      .name       = "msmpeg4v3",
      .long_name  = TRS("Divx 3 compatible"),
      .id         = CODEC_ID_MSMPEG4V3,
      .parameters = parameters_msmpeg4v3,
    },
    {
      .name       = "mpeg1video",
      .long_name  = TRS("MPEG-1 Video"),
      .id         = CODEC_ID_MPEG1VIDEO,
      .parameters = parameters_mpeg1,
      .flags      = FLAG_CONSTANT_FRAMERATE|FLAG_B_FRAMES,
      .framerates = bg_ffmpeg_mpeg_framerates,
    },
    {
      .name       = "mpeg2video",
      .long_name  = TRS("MPEG-2 Video"),
      .id         = CODEC_ID_MPEG2VIDEO,
      .parameters = parameters_mpeg1,
      .flags      = FLAG_CONSTANT_FRAMERATE|FLAG_B_FRAMES,
      .framerates = bg_ffmpeg_mpeg_framerates,
    },
    {
      .name       = "flv1",
      .long_name  = TRS("Flash 1"),
      .id         = CODEC_ID_FLV1,
      .parameters = parameters_msmpeg4v3,
    },
    {
      .name       = "wmv1",
      .long_name  = TRS("WMV 1"),
      .id         = CODEC_ID_WMV1,
      .parameters = parameters_msmpeg4v3,
    },
    {
      .name       = "rv10",
      .long_name  = TRS("Real Video 1"),
      .id         = CODEC_ID_RV10,
      .parameters = parameters_msmpeg4v3,
    },
    {
      .name       = "libx264",
      .long_name  = TRS("H.264"),
      .id         = CODEC_ID_H264,
      .parameters = parameters_libx264,
      .flags      = FLAG_B_FRAMES,
    },
    {
      .name       = "tga",
      .long_name  = TRS("Targa"),
      .id         = CODEC_ID_TARGA,
      .parameters = parameters_tga,
      .flags      = FLAG_INTRA_ONLY,
    },
    {
      .name       = "libvpx",
      .long_name  = TRS("VP8"),
      .id         = CODEC_ID_VP8,
      .parameters = parameters_libvpx,
      .flags      = 0,
    },
#if 0
    {
      .name       = "wmv2",
      .long_name  = TRS("WMV 2"),
      .id         = CODEC_ID_WMV2,
      .parameters = parameters_msmpeg4v3
    },
#endif
    { /* End of array */ }
  };

static const ffmpeg_codec_info_t **
add_codec_info(const ffmpeg_codec_info_t ** info, enum CodecID id, int * num)
  {
  int i;
  /* Check if the codec id is already in the array */

  for(i = 0; i < *num; i++)
    {
    if(info[i]->id == id)
      return info;
    }

  info = realloc(info, ((*num)+1) * sizeof(*info));
  
  info[*num] = NULL;

  i = 0;
  while(audio_codecs[i].name)
    {
    if(audio_codecs[i].id == id)
      {
      info[*num] = &audio_codecs[i];
      break;
      }
    i++;
    }

  if(!info[*num])
    {
    i = 0;
    while(video_codecs[i].name)
      {
      if(video_codecs[i].id == id)
        {
        info[*num] = &video_codecs[i];
        break;
        }
      i++;
      }
    }
  (*num)++;
  return info;
  }


static const bg_parameter_info_t audio_parameters[] =
  {
    {
      .name      = "codec",
      .long_name = TRS("Codec"),
      .type      = BG_PARAMETER_MULTI_MENU,
    },
    { /* */ }
  };

static const bg_parameter_info_t video_parameters[] =
  {
    {
      .name      = "codec",
      .long_name = TRS("Codec"),
      .type      = BG_PARAMETER_MULTI_MENU,
    },
    BG_ENCODER_FRAMERATE_PARAMS,
    { /* */ }
  };

static void create_codec_parameter(bg_parameter_info_t * parameter_info,
                                   const ffmpeg_codec_info_t ** infos,
                                   int num_infos)
  {
  int i;
  parameter_info[0].multi_names_nc =
    calloc(num_infos+1, sizeof(*parameter_info[0].multi_names));
  parameter_info[0].multi_labels_nc =
    calloc(num_infos+1, sizeof(*parameter_info[0].multi_labels));
  parameter_info[0].multi_parameters_nc =
    calloc(num_infos+1, sizeof(*parameter_info[0].multi_parameters));

  for(i = 0; i < num_infos; i++)
    {
    parameter_info[0].multi_names_nc[i]  =
      gavl_strrep(parameter_info[0].multi_names_nc[i], infos[i]->name);

    parameter_info[0].multi_labels_nc[i] =
      gavl_strrep(parameter_info[0].multi_labels_nc[i], infos[i]->long_name);

    if(infos[i]->parameters)
      parameter_info[0].multi_parameters_nc[i] =
        bg_parameter_info_copy_array(infos[i]->parameters);
    }
  parameter_info[0].val_default.val_str =
    gavl_strrep(parameter_info[0].val_default.val_str, infos[0]->name);
  bg_parameter_info_set_const_ptrs(&parameter_info[0]);
  }


bg_parameter_info_t * 
bg_ffmpeg_create_audio_parameters(const ffmpeg_format_info_t * format_info)
  {
  int i, j, num_infos = 0;
  bg_parameter_info_t * ret;
  const ffmpeg_codec_info_t ** infos = NULL;
  
  /* Create codec array */
  i = 0;
  while(format_info[i].name)
    {
    if(!format_info[i].audio_codecs)
      {
      i++;
      continue;
      }
    j = 0;
    while(format_info[i].audio_codecs[j] != CODEC_ID_NONE)
      {
      infos = add_codec_info(infos, format_info[i].audio_codecs[j],
                             &num_infos);
      j++;
      }
    i++;
    }

  if(!infos)
    return NULL;
  
  /* Create parameters */
  ret = bg_parameter_info_copy_array(audio_parameters);
  create_codec_parameter(&ret[0], infos, num_infos);
  free(infos);
  
  return ret;
  }

bg_parameter_info_t * 
bg_ffmpeg_create_video_parameters(const ffmpeg_format_info_t * format_info)
  {
  int i, j, num_infos = 0;
  bg_parameter_info_t * ret;
  const ffmpeg_codec_info_t ** infos = NULL;

  /* Create codec array */
  i = 0;
  while(format_info[i].name)
    {
    if(!format_info[i].video_codecs)
      {
      i++;
      continue;
      }
    j = 0;
    while(format_info[i].video_codecs[j] != CODEC_ID_NONE)
      {
      infos = add_codec_info(infos, format_info[i].video_codecs[j],
                             &num_infos);
      j++;
      }
    i++;
    }

  if(!infos)
    return NULL;
  
  /* Create parameters */

  /* Create parameters */
  ret = bg_parameter_info_copy_array(video_parameters);
  create_codec_parameter(&ret[0], infos, num_infos);
  free(infos);
  
  return ret;
  }

enum CodecID
bg_ffmpeg_find_audio_encoder(const ffmpeg_format_info_t * format,
                             const char * name)
  {
  int i = 0, found = 0;
  enum CodecID ret = CODEC_ID_NONE;
  
  while(audio_codecs[i].name)
    {
    if(!strcmp(name, audio_codecs[i].name))
      {
      ret = audio_codecs[i].id;
      break;
      }
    i++;
    }
  if(!format)
    return ret;
  
  i = 0;
  while(format->audio_codecs[i] != CODEC_ID_NONE)
    {
    if(format->audio_codecs[i] == ret)
      {
      found = 1;
      break;
      }
    i++;
    }

  if(!found)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Audio codec %s is not supported by %s",
           name, format->name);
    ret = CODEC_ID_NONE;
    }
  
  return ret;
  }

enum CodecID
bg_ffmpeg_find_video_encoder(const ffmpeg_format_info_t * format,
                             const char * name)
  {
  int i = 0, found = 0;
  enum CodecID ret = CODEC_ID_NONE;
  
  while(video_codecs[i].name)
    {
    if(!strcmp(name, video_codecs[i].name))
      {
      ret = video_codecs[i].id;
      break;
      }
    i++;
    }

  if(!format)
    return ret;
  
  i = 0;
  while(format->video_codecs[i] != CODEC_ID_NONE)
    {
    if(format->video_codecs[i] == ret)
      {
      found = 1;
      break;
      }
    i++;
    }

  if(!found)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Video codec %s is not supported by %s",
           name, format->name);
    ret = CODEC_ID_NONE;
    }
  
  return ret;
  }

static const ffmpeg_codec_info_t *
get_codec_info(const ffmpeg_codec_info_t * codecs, enum CodecID id) 
  {
  int i = 0;
  while(codecs[i].name)
    {
    if(id == codecs[i].id)
      return &codecs[i];
    i++;
    }
  return NULL;
  }

const ffmpeg_codec_info_t *
bg_ffmpeg_get_codec_info(enum CodecID id, int type)
  {
  if(type == AVMEDIA_TYPE_AUDIO)
    return get_codec_info(audio_codecs, id);
  else if(type == AVMEDIA_TYPE_VIDEO)
    return get_codec_info(video_codecs, id);
  return NULL;
  }


const bg_parameter_info_t *
bg_ffmpeg_get_codec_parameters(enum CodecID id, int type)
  {
  const ffmpeg_codec_info_t * ci = NULL;
  
  if(type == AVMEDIA_TYPE_AUDIO)
    ci = get_codec_info(audio_codecs, id);
  else if(type == AVMEDIA_TYPE_VIDEO)
    ci =  get_codec_info(video_codecs, id);
  
  if(!ci)
    return NULL;
  return ci->parameters;
  }

const char *
bg_ffmpeg_get_codec_name(enum CodecID id)
  {
  const ffmpeg_codec_info_t * info;
  if(!(info = get_codec_info(audio_codecs, id)) &&
     !(info = get_codec_info(video_codecs, id)))
    return NULL;
  return info->name;
  }

typedef struct
  {
  const char * s;
  int i;
  } enum_t;

#define PARAM_INT(n, var) \
  if(!strcmp(n, name)) \
    { \
    ctx->var = val->val_i;                         \
    found = 1; \
    }

#define PARAM_INT_SCALE(n, var, scale)   \
  if(!strcmp(n, name)) \
    { \
    ctx->var = val->val_i * scale;                 \
    found = 1; \
    }

#define PARAM_STR_INT_SCALE(n, var, scale) \
  if(!strcmp(n, name)) \
    { \
    ctx->var = atoi(val->val_str) * scale; \
    found = 1; \
    }


#define PARAM_QP2LAMBDA(n, var)   \
  if(!strcmp(n, name)) \
    { \
    ctx->var = (int)(val->val_f * FF_QP2LAMBDA+0.5);  \
    found = 1; \
    }
#define PARAM_FLOAT(n, var)   \
  if(!strcmp(n, name)) \
    { \
    ctx->var = val->val_f;                 \
    found = 1; \
    }

#define PARAM_CMP_CHROMA(n,var)              \
  {                                             \
  if(!strcmp(n, name))                    \
    {                                           \
    if(val->val_i)                            \
      ctx->var |= FF_CMP_CHROMA;                \
    else                                        \
      ctx->var &= ~FF_CMP_CHROMA;               \
                                                \
    found = 1;                                  \
    }                                           \
  }

#define PARAM_FLAG(n,flag)                  \
  {                                             \
  if(!strcmp(n, name))                    \
    {                                           \
    if(val->val_i)                            \
      ctx->flags |= flag;                \
    else                                        \
      ctx->flags &= ~flag;               \
                                                \
    found = 1;                                  \
    }                                           \
  }

#define PARAM_FLAG2(n,flag)                  \
  {                                             \
  if(!strcmp(n, name))                    \
    {                                           \
    if(val->val_i)                            \
      ctx->flags2 |= flag;                \
    else                                        \
      ctx->flags2 &= ~flag;               \
                                                \
    found = 1;                                  \
    }                                           \
  }

  

static const enum_t me_method[] =
  {
    { "Zero",  ME_ZERO },
    { "Phods", ME_PHODS },
    { "Log",   ME_LOG },
    { "X1",    ME_X1 },
    { "Epzs",  ME_EPZS },
    { "Full",  ME_FULL }
  };

static const enum_t prediction_method[] =
  {
    { "Left",   FF_PRED_LEFT },
    { "Plane",  FF_PRED_PLANE },
    { "Median", FF_PRED_MEDIAN }
  };

static const enum_t compare_func[] =
  {
    { "SAD",  FF_CMP_SAD },
    { "SSE",  FF_CMP_SSE },
    { "SATD", FF_CMP_SATD },
    { "DCT",  FF_CMP_DCT },
    { "PSNR", FF_CMP_PSNR },
    { "BIT",  FF_CMP_BIT },
    { "RD",   FF_CMP_RD },
    { "ZERO", FF_CMP_ZERO },
    { "VSAD", FF_CMP_VSAD },
    { "VSSE", FF_CMP_VSSE },
    { "NSSE", FF_CMP_NSSE }
  };

static const enum_t mb_decision[] =
  {
    { "Use compare function", FF_MB_DECISION_SIMPLE },
    { "Fewest bits",          FF_MB_DECISION_BITS },
    { "Rate distoration",     FF_MB_DECISION_RD }
  };

static const enum_t faac_profile[] =
  {
    { "main", FF_PROFILE_AAC_MAIN },
    { "lc",   FF_PROFILE_AAC_LOW  },
    { "ssr",  FF_PROFILE_AAC_SSR  },
    { "ltp",  FF_PROFILE_AAC_LTP  }
  };

#define PARAM_ENUM(n, var, arr) \
  if(!strcmp(n, name)) \
    { \
    for(i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) \
      {                                             \
      if(!strcmp(val->val_str, arr[i].s))       \
        {                                           \
        ctx->var = arr[i].i;                        \
        break;                                      \
        }                                           \
      }                                             \
    found = 1;                                      \
    }

#define PARAM_DICT_STRING(n, ffmpeg_key) \
  if(!strcmp(n, name)) \
    { \
    if(val->val_str && (val->val_str[0] != '$')) \
      av_dict_set(options,                      \
                  ffmpeg_key, val->val_str, 0); \
    }

#define PARAM_DICT_FLOAT(n, ffmpeg_key) \
  if(!strcmp(n, name)) \
    { \
    char * str; \
    str = bg_sprintf("%f", val->val_f); \
    av_dict_set(options, ffmpeg_key, str, 0); \
    free(str); \
    }

#define PARAM_DICT_INT(n, ffmpeg_key) \
  if(!strcmp(n, name)) \
    { \
    char * str; \
    str = bg_sprintf("%d", val->val_i); \
    av_dict_set(options, ffmpeg_key, str, 0); \
    free(str); \
    }


void
bg_ffmpeg_set_codec_parameter(AVCodecContext * ctx,
                              AVDictionary ** options,
                              const char * name,
                              const bg_parameter_value_t * val)
  {
  int found = 0, i;
/*
 *   IMPORTANT: To keep the mess at a reasonable level,
 *   *all* parameters *must* appear in the same order as in
 *   the AVCocecContext structure, except the flags, which come at the very end
 */
  
  PARAM_INT_SCALE("ff_bit_rate_video",bit_rate,1000);
  PARAM_INT_SCALE("ff_bit_rate_audio",bit_rate,1000);
  
  PARAM_STR_INT_SCALE("ff_bit_rate_str", bit_rate, 1000);

  PARAM_INT_SCALE("ff_bit_rate_tolerance",bit_rate_tolerance,1000);
  PARAM_ENUM("ff_me_method",me_method,me_method);
  PARAM_INT("ff_gop_size",gop_size);
  PARAM_FLOAT("ff_qcompress",qcompress);
  PARAM_FLOAT("ff_qblur",qblur);
  PARAM_INT("ff_qmin",qmin);
  PARAM_INT("ff_qmax",qmax);
  PARAM_INT("ff_max_qdiff",max_qdiff);
  PARAM_INT("ff_max_b_frames",max_b_frames);
  PARAM_FLOAT("ff_b_quant_factor",b_quant_factor);
  PARAM_INT("ff_b_frame_strategy",b_frame_strategy);
  PARAM_INT("ff_strict_std_compliance",strict_std_compliance);
  PARAM_QP2LAMBDA("ff_b_quant_offset",b_quant_offset);
  PARAM_INT("ff_rc_min_rate",rc_min_rate);
  PARAM_INT("ff_rc_max_rate",rc_max_rate);
  PARAM_INT_SCALE("ff_rc_buffer_size",rc_buffer_size,1000);
  PARAM_FLOAT("ff_rc_buffer_aggressivity",rc_buffer_aggressivity);
  PARAM_FLOAT("ff_i_quant_factor",i_quant_factor);
  PARAM_QP2LAMBDA("ff_i_quant_offset",i_quant_offset);
  PARAM_FLOAT("ff_rc_initial_cplx",rc_initial_cplx);
  PARAM_FLOAT("ff_lumi_masking",lumi_masking);
  PARAM_FLOAT("ff_temporal_cplx_masking",temporal_cplx_masking);
  PARAM_FLOAT("ff_spatial_cplx_masking",spatial_cplx_masking);
  PARAM_FLOAT("ff_p_masking",p_masking);
  PARAM_FLOAT("ff_dark_masking",dark_masking);
  PARAM_ENUM("ff_prediction_method",prediction_method,prediction_method);
  PARAM_ENUM("ff_me_cmp",me_cmp,compare_func);
  PARAM_CMP_CHROMA("ff_me_cmp_chroma",me_cmp);
  PARAM_ENUM("ff_me_sub_cmp",me_sub_cmp,compare_func);
  PARAM_CMP_CHROMA("ff_me_sub_cmp_chroma",me_sub_cmp);
  PARAM_ENUM("ff_mb_cmp",mb_cmp,compare_func);
  PARAM_CMP_CHROMA("ff_mb_cmp_chroma",mb_cmp);
  PARAM_ENUM("ff_ildct_cmp",ildct_cmp,compare_func);
  PARAM_CMP_CHROMA("ff_ildct_cmp_chroma",ildct_cmp);
  PARAM_INT("ff_dia_size",dia_size);
  PARAM_INT("ff_last_predictor_count",last_predictor_count);
  PARAM_INT("ff_pre_me",pre_me);
  PARAM_ENUM("ff_me_pre_cmp",me_pre_cmp,compare_func);
  PARAM_CMP_CHROMA("ff_pre_me_cmp_chroma",me_pre_cmp);
  PARAM_INT("ff_pre_dia_size",pre_dia_size);
  PARAM_INT("ff_me_subpel_quality",me_subpel_quality);
  PARAM_INT("ff_me_range",me_range);
  PARAM_ENUM("ff_mb_decision",mb_decision,mb_decision);
  PARAM_INT("ff_scenechange_threshold",scenechange_threshold);
  PARAM_QP2LAMBDA("ff_lmin", lmin);
  PARAM_QP2LAMBDA("ff_lmax", lmax);
  PARAM_INT("ff_noise_reduction",noise_reduction);
  PARAM_INT_SCALE("ff_rc_initial_buffer_occupancy",rc_initial_buffer_occupancy,1000);
  PARAM_INT("ff_me_threshold",me_threshold);
  PARAM_INT("ff_mb_threshold",mb_threshold);
  PARAM_INT("ff_nsse_weight",nsse_weight);
  PARAM_FLOAT("ff_border_masking",border_masking);
  PARAM_QP2LAMBDA("ff_mb_lmin", mb_lmin);
  PARAM_QP2LAMBDA("ff_mb_lmax", mb_lmax);
  PARAM_INT("ff_me_penalty_compensation",me_penalty_compensation);
  PARAM_INT("ff_bidir_refine",bidir_refine);
  PARAM_INT("ff_brd_scale",brd_scale);
  PARAM_INT("ff_keyint_min",keyint_min);
  PARAM_INT("ff_scenechange_factor",scenechange_factor);
  PARAM_FLAG("ff_flag_qscale",CODEC_FLAG_QSCALE);
  PARAM_FLAG("ff_flag_4mv",CODEC_FLAG_4MV);
  PARAM_FLAG("ff_flag_qpel",CODEC_FLAG_QPEL);
  PARAM_FLAG("ff_flag_gmc",CODEC_FLAG_GMC);
  PARAM_FLAG("ff_flag_mv0",CODEC_FLAG_MV0);
  //  PARAM_FLAG("ff_flag_part",CODEC_FLAG_PART);
  PARAM_FLAG("ff_flag_gray",CODEC_FLAG_GRAY);
  PARAM_FLAG("ff_flag_emu_edge",CODEC_FLAG_EMU_EDGE);
  PARAM_FLAG("ff_flag_normalize_aqp",CODEC_FLAG_NORMALIZE_AQP);
  //  PARAM_FLAG("ff_flag_alt_scan",CODEC_FLAG_ALT_SCAN);
  PARAM_INT("ff_trellis",trellis);
  PARAM_FLAG("ff_flag_bitexact",CODEC_FLAG_BITEXACT);
  PARAM_FLAG("ff_flag_ac_pred",CODEC_FLAG_AC_PRED);
  //  PARAM_FLAG("ff_flag_h263p_umv",CODEC_FLAG_H263P_UMV);
  PARAM_FLAG("ff_flag_cbp_rd",CODEC_FLAG_CBP_RD);
  PARAM_FLAG("ff_flag_qp_rd",CODEC_FLAG_QP_RD);
  //  PARAM_FLAG("ff_flag_h263p_aiv",CODEC_FLAG_H263P_AIV);
  //  PARAM_FLAG("ffx_flag_obmc",CODEC_FLAG_OBMC);
  PARAM_FLAG("ff_flag_loop_filter",CODEC_FLAG_LOOP_FILTER);
  //  PARAM_FLAG("ff_flag_h263p_slice_struct",CODEC_FLAG_H263P_SLICE_STRUCT);
  PARAM_FLAG("ff_flag_closed_gop",CODEC_FLAG_CLOSED_GOP);
  PARAM_FLAG2("ff_flag2_fast",CODEC_FLAG2_FAST);
  PARAM_FLAG2("ff_flag2_strict_gop",CODEC_FLAG2_STRICT_GOP);
  PARAM_INT("ff_thread_count",thread_count);
  
  PARAM_DICT_STRING("libx264_preset", "preset");
  PARAM_DICT_STRING("libx264_tune",   "tune");
  PARAM_DICT_FLOAT("libx264_crf", "crf");
  PARAM_DICT_FLOAT("libx264_qp", "qp");

  PARAM_ENUM("faac_profile", profile, faac_profile);

  if(!strcmp(name, "faac_quality"))
    ctx->global_quality = FF_QP2LAMBDA * val->val_i;
  else if(!strcmp(name, "vorbis_quality"))
    ctx->global_quality = FF_QP2LAMBDA * val->val_i;
  
  if(!strcmp(name, "tga_rle"))
    {
    if(val->val_i)
      ctx->coder_type = FF_CODER_TYPE_RLE;
    else
      ctx->coder_type = FF_CODER_TYPE_RAW;
    }
  
  PARAM_DICT_STRING("libvpx_deadline", "deadline");
  PARAM_DICT_INT("libvpx_cpu-used",   "cpu-used");
  PARAM_DICT_INT("libvpx_auto-alt-ref", "alt-ref");
  PARAM_DICT_INT("libvpx_lag-in-frames", "lag-in-frames");
  PARAM_DICT_INT("libvpx_arnr-max-frames", "arnr-max-frames");
  PARAM_DICT_INT("libvpx_crf", "crf");
  PARAM_DICT_STRING("libvpx_arnr-type", "arnr-type");
  }

/* Type conversion */

const bg_encoder_framerate_t bg_ffmpeg_mpeg_framerates[] =
  {
    { 24000, 1001 },
    {    24,    1 },
    {    25,    1 },
    { 30000, 1001 },
    {    30,    1 },
    {    50,    1 },
    { 60000, 1001 },
    {    60,    1 },
    { /* End of framerates */ }
  };

#ifdef WORDS_BIGENDIAN
#define PIX_FMT_LE CONVERT_ENDIAN
#define PIX_FMT_BE 0
#else
#define PIX_FMT_LE 0
#define PIX_FMT_BE CONVERT_ENDIAN
#endif

static const struct
  {
  enum PixelFormat  ffmpeg_csp;
  gavl_pixelformat_t gavl_csp;
  int convert_flags;
  }
pixelformats[] =
  {
    { PIX_FMT_YUV420P,  GAVL_YUV_420_P },  ///< Planar YUV 4:2:0 (1 Cr & Cb sample per 2x2 Y samples)
    { PIX_FMT_YUYV422,  GAVL_YUY2      },
    { PIX_FMT_YUV422P,  GAVL_YUV_422_P },  ///< Planar YUV 4:2:2 (1 Cr & Cb sample per 2x1 Y samples)
    { PIX_FMT_YUV444P,  GAVL_YUV_444_P }, ///< Planar YUV 4:4:4 (1 Cr & Cb sample per 1x1 Y samples)
    { PIX_FMT_YUV411P,  GAVL_YUV_411_P }, ///< Planar YUV 4:1:1 (1 Cr & Cb sample per 4x1 Y samples)
    { PIX_FMT_YUVJ420P, GAVL_YUVJ_420_P }, ///< Planar YUV 4:2:0 full scale (jpeg)
    { PIX_FMT_YUVJ422P, GAVL_YUVJ_422_P }, ///< Planar YUV 4:2:2 full scale (jpeg)
    { PIX_FMT_YUVJ444P, GAVL_YUVJ_444_P }, ///< Planar YUV 4:4:4 full scale (jpeg)
    { PIX_FMT_GRAY8,    GAVL_GRAY_8 },

    { PIX_FMT_RGB565BE, GAVL_RGB_16, PIX_FMT_BE }, ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), big-endian
    { PIX_FMT_RGB565LE, GAVL_RGB_16, PIX_FMT_LE }, ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), little-endian
    { PIX_FMT_RGB555BE, GAVL_RGB_15, PIX_FMT_BE }, ///< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian, most significant bit to 0
    { PIX_FMT_RGB555LE, GAVL_RGB_15, PIX_FMT_LE }, ///< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian, most significant bit to 0

    { PIX_FMT_BGR565BE, GAVL_BGR_16, PIX_FMT_BE }, ///< packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), big-endian
    { PIX_FMT_BGR565LE, GAVL_BGR_16, PIX_FMT_LE }, ///< packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), little-endian
    { PIX_FMT_BGR555BE, GAVL_BGR_15, PIX_FMT_BE }, ///< packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), big-endian, most significant bit to 1
    { PIX_FMT_BGR555LE, GAVL_BGR_15, PIX_FMT_LE }, ///< packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), little-endian, most significant bit to 1
    { PIX_FMT_RGB24,    GAVL_RGB_24    },  ///< Packed pixel, 3 bytes per pixel, RGBRGB...
    { PIX_FMT_BGR24,    GAVL_BGR_24    },  ///< Packed pixel, 3 bytes per pixel, BGRBGR...
    { PIX_FMT_BGRA,     GAVL_RGBA_32, CONVERT_OTHER },
    
#if 0 // Not needed in the forseeable future    
#if LIBAVUTIL_VERSION_INT < (50<<16)
    { PIX_FMT_RGBA32,        GAVL_RGBA_32   },  ///< Packed pixel, 4 bytes per pixel, BGRABGRA..., stored in cpu endianness
#else
    { PIX_FMT_RGB32,         GAVL_RGBA_32   },  ///< Packed pixel, 4 bytes per pixel, BGRABGRA..., stored in cpu endianness
#endif
    { PIX_FMT_YUV410P,       GAVL_YUV_410_P }, ///< Planar YUV 4:1:0 (1 Cr & Cb sample per 4x4 Y samples)
#endif // Not needed
};

static gavl_pixelformat_t bg_pixelformat_ffmpeg_2_gavl(enum PixelFormat p)
  {
  int i;
  for(i = 0; i < sizeof(pixelformats)/sizeof(pixelformats[0]); i++)
    {
    if(pixelformats[i].ffmpeg_csp == p)
      return pixelformats[i].gavl_csp;
    }
  return GAVL_PIXELFORMAT_NONE;
  }

static enum PixelFormat bg_pixelformat_gavl_2_ffmpeg(gavl_pixelformat_t p, int * do_convert,
                                                     const enum PixelFormat * supported)
  {
  int i, j;
  for(i = 0; i < sizeof(pixelformats)/sizeof(pixelformats[0]); i++)
    {
    if(pixelformats[i].gavl_csp == p)
      {
      j = 0;

      while(supported[j] != PIX_FMT_NONE)
        {
        if(pixelformats[i].ffmpeg_csp == supported[j])
          {
          if(do_convert)
            *do_convert = pixelformats[i].convert_flags;
          return pixelformats[i].ffmpeg_csp;
          }
        j++;
        }
      }
    }
  return PIX_FMT_NONE;
  }

void bg_ffmpeg_choose_pixelformat(const enum PixelFormat * supported,
                                  enum PixelFormat * ffmpeg_fmt,
                                  gavl_pixelformat_t * gavl_fmt, int * do_convert)
  {
  int i, num;
  gavl_pixelformat_t * gavl_fmts;

  /* Count pixelformats */
  i = 0;
  num = 0;

  while(supported[i] != PIX_FMT_NONE)
    {
    if(bg_pixelformat_ffmpeg_2_gavl(supported[i]) != GAVL_PIXELFORMAT_NONE)
      num++;
    i++;
    }
  
  gavl_fmts = malloc((num+1) * sizeof(gavl_fmts));
  
  i = 0;
  num = 0;
  
  while(supported[i] != PIX_FMT_NONE)
    {
    if((gavl_fmts[num] = bg_pixelformat_ffmpeg_2_gavl(supported[i])) != GAVL_PIXELFORMAT_NONE)
      num++;
    i++;
    }
  gavl_fmts[num] = GAVL_PIXELFORMAT_NONE;

  *gavl_fmt = gavl_pixelformat_get_best(*gavl_fmt, gavl_fmts, NULL);
  *ffmpeg_fmt = bg_pixelformat_gavl_2_ffmpeg(*gavl_fmt, do_convert, supported);
  free(gavl_fmts);
  }

static const struct
  {
  enum AVSampleFormat  ffmpeg_fmt;
  gavl_sample_format_t gavl_fmt;
  int planar;
  }
sampleformats[] =
  {
    { AV_SAMPLE_FMT_U8,   GAVL_SAMPLE_U8,     0 },
    { AV_SAMPLE_FMT_S16,  GAVL_SAMPLE_S16,    0 },    ///< signed 16 bits
    { AV_SAMPLE_FMT_S32,  GAVL_SAMPLE_S32,    0 },    ///< signed 32 bits
    { AV_SAMPLE_FMT_FLT,  GAVL_SAMPLE_FLOAT,  0 },  ///< float
    { AV_SAMPLE_FMT_DBL,  GAVL_SAMPLE_DOUBLE, 0 }, ///< double
    { AV_SAMPLE_FMT_U8P,  GAVL_SAMPLE_U8,     1 },
    { AV_SAMPLE_FMT_S16P, GAVL_SAMPLE_S16,    1 },    ///< signed 16 bits
    { AV_SAMPLE_FMT_S32P, GAVL_SAMPLE_S32,    1 },    ///< signed 32 bits
    { AV_SAMPLE_FMT_FLTP, GAVL_SAMPLE_FLOAT,  1 },  ///< float
    { AV_SAMPLE_FMT_DBLP, GAVL_SAMPLE_DOUBLE, 1 }, ///< double
  };

gavl_sample_format_t bg_sample_format_ffmpeg_2_gavl(enum AVSampleFormat p,
                                                    gavl_interleave_mode_t * il)
  {
  int i;
  for(i = 0; i < sizeof(sampleformats)/sizeof(sampleformats[0]); i++)
    {
    if(sampleformats[i].ffmpeg_fmt == p)
      {
      if(il)
        {
        if(sampleformats[i].planar)
          *il = GAVL_INTERLEAVE_NONE;
        else
          *il = GAVL_INTERLEAVE_ALL;
        }
      return sampleformats[i].gavl_fmt;
      }
    }
  return GAVL_SAMPLE_NONE;
  }

/* Compressed stream support */

static const struct
  {
  gavl_codec_id_t gavl;
  enum CodecID    ffmpeg;
  }
codec_ids[] =
  {
    /* Audio */
    { GAVL_CODEC_ID_ALAW,   CODEC_ID_PCM_ALAW  }, //!< alaw 2:1
    { GAVL_CODEC_ID_ULAW,   CODEC_ID_PCM_MULAW }, //!< mu-law 2:1
    { GAVL_CODEC_ID_MP2,    CODEC_ID_MP2       }, //!< MPEG-1 audio layer II
    { GAVL_CODEC_ID_MP3,    CODEC_ID_MP3       }, //!< MPEG-1/2 audio layer 3 CBR/VBR
    { GAVL_CODEC_ID_AC3,    CODEC_ID_AC3       }, //!< AC3
    { GAVL_CODEC_ID_AAC,    CODEC_ID_AAC       }, //!< AAC as stored in quicktime/mp4
    { GAVL_CODEC_ID_VORBIS, CODEC_ID_VORBIS    }, //!< Vorbis (segmented extradata and packets)
    { GAVL_CODEC_ID_AAC,    CODEC_ID_AAC       }, //!< AAC
    
    /* Video */
    { GAVL_CODEC_ID_JPEG,      CODEC_ID_MJPEG      }, //!< JPEG image
    { GAVL_CODEC_ID_PNG,       CODEC_ID_PNG        }, //!< PNG image
    { GAVL_CODEC_ID_TIFF,      CODEC_ID_TIFF       }, //!< TIFF image
    { GAVL_CODEC_ID_TGA,       CODEC_ID_TARGA      }, //!< TGA image
    { GAVL_CODEC_ID_MPEG1,     CODEC_ID_MPEG1VIDEO }, //!< MPEG-1 video
    { GAVL_CODEC_ID_MPEG2,     CODEC_ID_MPEG2VIDEO }, //!< MPEG-2 video
    { GAVL_CODEC_ID_MPEG4_ASP, CODEC_ID_MPEG4      }, //!< MPEG-4 ASP (a.k.a. Divx4)
    { GAVL_CODEC_ID_H264,      CODEC_ID_H264       }, //!< H.264 (Annex B)
    { GAVL_CODEC_ID_THEORA,    CODEC_ID_THEORA     }, //!< Theora (segmented extradata
    { GAVL_CODEC_ID_DIRAC,     CODEC_ID_DIRAC      }, //!< Complete DIRAC frames, sequence end code appended to last packet
    { GAVL_CODEC_ID_DV,        CODEC_ID_DVVIDEO    }, //!< DV (several variants)
    { GAVL_CODEC_ID_VP8,       CODEC_ID_VP8        }, //!< VP8 (as in webm)
    { GAVL_CODEC_ID_NONE,      CODEC_ID_NONE       },
  };

enum CodecID bg_codec_id_gavl_2_ffmpeg(gavl_codec_id_t gavl)
  {
  int i = 0;
  while(codec_ids[i].gavl != GAVL_CODEC_ID_NONE)
    {
    if(codec_ids[i].gavl == gavl)
      return codec_ids[i].ffmpeg;
    i++;
    }
  return CODEC_ID_NONE;
  }

gavl_codec_id_t bg_codec_id_ffmpeg_2_gavl(enum CodecID ffmpeg)
  {
  int i = 0;
  while(codec_ids[i].gavl != GAVL_CODEC_ID_NONE)
    {
    if(codec_ids[i].ffmpeg == ffmpeg)
      return codec_ids[i].gavl;
    i++;
    }
  return GAVL_CODEC_ID_NONE;
  }

/* Channel layout */

struct
  {
  gavl_channel_id_t gavl_id;
  uint64_t    ffmpeg_id;
  }
channel_ids[] =
  {
    { GAVL_CHID_FRONT_CENTER,       AV_CH_FRONT_CENTER },
    { GAVL_CHID_FRONT_LEFT,         AV_CH_FRONT_LEFT     },
    { GAVL_CHID_FRONT_RIGHT,        AV_CH_FRONT_RIGHT   },
    { GAVL_CHID_FRONT_CENTER_LEFT,  AV_CH_FRONT_LEFT_OF_CENTER },
    { GAVL_CHID_FRONT_CENTER_RIGHT, AV_CH_FRONT_RIGHT_OF_CENTER },
    { GAVL_CHID_REAR_LEFT,          AV_CH_BACK_LEFT },
    { GAVL_CHID_REAR_RIGHT,         AV_CH_BACK_RIGHT },
    { GAVL_CHID_REAR_CENTER,        AV_CH_BACK_CENTER },
    { GAVL_CHID_SIDE_LEFT,          AV_CH_SIDE_LEFT },
    { GAVL_CHID_SIDE_RIGHT,         AV_CH_SIDE_RIGHT },
    { GAVL_CHID_LFE,                AV_CH_LOW_FREQUENCY },
    //    { GAVL_CHID_AUX, 0 }
    { GAVL_CHID_NONE, 0 },
  };

static uint64_t chid_gavl_2_ffmpeg(gavl_channel_id_t gavl_id)
  {
  int i = 0;

  while(channel_ids[i].gavl_id != GAVL_CHID_NONE)
    {
    if(channel_ids[i].gavl_id == gavl_id)
      return channel_ids[i].ffmpeg_id;
    i++;
    }
  return 0;
  }

static gavl_channel_id_t chid_ffmpeg_2_gavl(uint64_t ffmpeg_id)
  {
  int i = 0;

  while(channel_ids[i].gavl_id != GAVL_CHID_NONE)
    {
    if(channel_ids[i].ffmpeg_id == ffmpeg_id)
      return channel_ids[i].gavl_id;
    i++;
    }
  return GAVL_CHID_NONE;
  }

uint64_t
bg_ffmpeg_get_channel_layout(gavl_audio_format_t * format)
  {
  int i, idx;
  uint64_t mask = 1;
  uint64_t ret = 0;

  for(i = 0; i < format->num_channels; i++)
    ret |= chid_gavl_2_ffmpeg(format->channel_locations[i]);
  
  idx = 0;
  for(i = 0; i < 64; i++)
    {
    if(ret & mask)
      {
      format->channel_locations[idx] = chid_ffmpeg_2_gavl(mask);
      idx++;
      }
    mask <<= 1;
    }

  return ret;
  }

