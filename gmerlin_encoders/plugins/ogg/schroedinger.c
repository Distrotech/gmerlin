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
#include <stdlib.h>

#include <config.h>

#include <gavl/metatags.h>
#include <gavl/numptr.h>


#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "schroedinger"

#include <schroedinger/schro.h>
// #include <schroedinger/schrodebug.h>
#include <schroedinger/schrovideoformat.h>
#ifdef HAVE_SCHROEDINGER_SCHROVERSION_H
#include <schroedinger/schroversion.h>
#else
#define SCHRO_CHECK_VERSION(a,b,c) 0
#endif

#include <ogg/ogg.h>
#include "ogg_common.h"


typedef struct
  {
  gavl_packet_sink_t * psink;
  } schro_t;

static void set_packet_sink(void * data, gavl_packet_sink_t * psink)
  {
  schro_t * s = data;
  s->psink = psink;
  }

static void * create_schro()
  {
  schro_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static const bg_parameter_info_t parameters[] =
  {
    { 
      .name =        "rc",
      .long_name =   TRS("Rate control"),
      .type =        BG_PARAMETER_SECTION,
    },
    { 
      .name =        "enc_rate_control",
      .long_name =   TRS("Rate control"),
      .type =        BG_PARAMETER_STRINGLIST,
#if SCHRO_CHECK_VERSION(1,0,9)
      .val_default = { .val_str = "Constant quality" },
#else
      .val_default = { .val_str = "Constant noise threshold" },
#endif
      .multi_names = (const char *[]){ TRS("Constant noise threshold"),
                                 TRS("Constant bitrate"),
                                 TRS("Low delay"), 
                                 TRS("Lossless"),
                                 TRS("Constant lambda"),
                                 TRS("Constant error"),
#if SCHRO_CHECK_VERSION(1,0,6)
                                 TRS("Constant quality"),
#endif
                                        (char *)0 },
    },
    {
      .name = "enc_bitrate",
      .long_name = TRS("Bitrate"),
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i =   13824000 },
    },
#if 0 /* Not used */
    {
      .name = "enc_min_bitrate",
      .long_name = TRS("Minimum bitrate"),
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i =   13824000 },
    },
    {
      .name = "enc_max_bitrate",
      .long_name = TRS("Maximum bitrate"),
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i =   13824000 },
    },
#endif
    {
      .name      = "enc_buffer_size",
      .long_name = TRS("Buffer size"),
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i =          0 },
    },
    {
      .name      = "enc_buffer_level",
      .long_name = TRS("Buffer level"),
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i =          0 },
    },
    {
      .name      = "enc_noise_threshold",
      .long_name = TRS("Noise Threshold"),
      .type = BG_PARAMETER_FLOAT,
      .val_default = { .val_f =       25.0 },
      .val_min     = { .val_f =          0 },
      .val_max     = { .val_f =      100.0 },
      .num_digits  = 2,
    },
#if SCHRO_CHECK_VERSION(1,0,6)
    {
      .name      = "enc_quality",
      .long_name = TRS("Quality"),
      .type = BG_PARAMETER_FLOAT,
#if SCHRO_CHECK_VERSION(1,0,9)
      .val_default = { .val_f =        5.0 },
#else
      .val_default = { .val_f =        7.0 },
#endif
      .val_min     = { .val_f =        0.0 },
      .val_max     = { .val_f =       10.0 },
      .num_digits  = 2,
    },
#endif
#if SCHRO_CHECK_VERSION(1,0,9)
    {
      .name =        "enc_enable_rdo_cbr",
      .long_name =   TRS("rdo cbr"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i =     1 },
    },
#endif
    { 
      .name =        "enc_frame_types",
      .long_name =   TRS("Frame types"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
      .name = "enc_gop_structure",
      .long_name = TRS("GOP Stucture"),
      .type = BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "Adaptive" },
      .multi_names = (const char *[]){ TRS("Adaptive"),
                                        TRS("Intra only"),
                                        TRS("Backref"), 
                                        TRS("Chained backref"),
                                        TRS("Biref"),
                                        TRS("Chained biref"),
                                        (char *)0 },
    },
    {
      .name = "enc_au_distance",
      .long_name = TRS("GOP size"),
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 30 },
    },
#if SCHRO_CHECK_VERSION(1,0,6)
    {
      .name      = "enc_open_gop",
      .long_name = TRS("Open GOPs"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Choose whether GOPs should be open or closed. Closed GOPs improve seeking accuracy for buggy decoders, open GOPs have a slightly better compression"),
    },
#endif
    { 
      .name =        "enc_me",
      .long_name =   TRS("Motion estimation"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
      .name =        "enc_mv_precision",
      .long_name =   TRS("MV Precision"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i =     0 },
      .val_min     = { .val_i =     3 },
      .val_max     = { .val_i =     0 },
    },
    {
      .name =        "enc_motion_block_size",
      .long_name =   TRS("Block size"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "Automatic" },
      .multi_names = (const char *[]){ TRS("Automatic"),
                                        TRS("Small"),
                                        TRS("Medium"),
                                        TRS("Large"),
                                        (char *)0 },
      
    },
    {
      .name =        "enc_motion_block_overlap",
      .long_name =   TRS("Block overlap"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "Automatic" },
      .multi_names = (const char *[]){ TRS("Automatic"),
                                        TRS("None"),
                                        TRS("Partial"),
                                        TRS("Full"),
                                        (char *)0 },
    },
#if SCHRO_CHECK_VERSION(1,0,9)
    {
      .name =        "enc_enable_chroma_me",
      .long_name =   TRS("Enable chroma ME"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i =     0 },
    },
#endif

#if SCHRO_CHECK_VERSION(1,0,6)
    {
      .name      = "enc_enable_global_motion",
      .long_name = TRS("Enable GMC"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
      .help_string = TRS("Enable global motion estimation"),
    },
    {
      .name      = "enc_enable_phasecorr_estimation",
      .long_name = TRS("Enable phasecorrelation estimation"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
    },
#endif
#if SCHRO_CHECK_VERSION(1,0,9)
    {
      .name =        "enc_enable_hierarchical_estimation",
      .long_name =   TRS("Hierarchical estimation"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i =     1 },
    },
    {
      .name =        "enc_enable_phasecorr_estimation",
      .long_name =   TRS("Phasecorr estimation"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i =     0 },
      .val_min     = { .val_i =     0 },
      .val_max     = { .val_i =     1 },
    },
    {
      .name =        "enc_enable_bigblock_estimation",
      .long_name =   TRS("Bigblock estimation"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i =     1 },
      .val_min     = { .val_i =     0 },
      .val_max     = { .val_i =     1 },
    },
    {
      .name =        "enc_enable_global_motion",
      .long_name =   TRS("Global motion estimation"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i =     0 },
      .val_min     = { .val_i =     0 },
      .val_max     = { .val_i =     1 },
    },
    {
      .name =        "enc_enable_deep_estimation",
      .long_name =   TRS("Deep estimation"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i =     1 },
      .val_min     = { .val_i =     0 },
      .val_max     = { .val_i =     1 },
    },
    {
      .name =        "enc_enable_scene_change_detection",
      .long_name =   TRS("Scene change detection"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i =     1 },
      .val_min     = { .val_i =     0 },
      .val_max     = { .val_i =     1 },
    },
#endif
    { 
      .name =        "enc_wavelets",
      .long_name =   TRS("Wavelets"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
      .name = "enc_intra_wavelet",
      .long_name = TRS("Intra wavelet"),
      .type = BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "Deslauriers-Debuc (9,3)" },
      .multi_names = (const char *[]){ TRS("Deslauriers-Debuc (9,3)"),
                                        TRS("LeGall (5,3)"),
                                        TRS("Deslauriers-Debuc (13,5)"),
                                        TRS("Haar (no shift)"),
                                        TRS("Haar (single shift)"),
                                        TRS("Fidelity"),
                                        TRS("Daubechies (9,7)"),
                                        (char *)0 },
    },
    {
      .name = "enc_inter_wavelet",
      .long_name = TRS("Inter wavelet"),
      .type = BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "LeGall (5,3)" },
      .multi_names = (const char *[]){ TRS("Deslauriers-Debuc (9,3)"),
                                        TRS("LeGall (5,3)"),
                                        TRS("Deslauriers-Debuc (13,5)"),
                                        TRS("Haar (no shift)"),
                                        TRS("Haar (single shift)"),
                                        TRS("Fidelity"),
                                        TRS("Daubechies (9,7)"),
                                        (char *)0 },
    },
    { 
      .name =        "enc_filter",
      .long_name =   TRS("Filter"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
      .name = "enc_filtering",
      .long_name = TRS("Filter"),
      .type = BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "None" },
      .multi_names = (const char *[]){ TRS("None"),
                                        TRS("Center weighted median"),
                                        TRS("Gaussian"),
                                        TRS("Add noise"),
                                        TRS("Adaptive Gaussian"),
                                        (char *)0 },
    },
    {
      .name = "enc_filter_value",
      .long_name = TRS("Filter value"),
      .type = BG_PARAMETER_FLOAT,
      .val_default = { .val_f = 5.0 },
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 100.0 },
    },
#if SCHRO_CHECK_VERSION(1,0,6)
    { 
      .name =        "enc_misc",
      .long_name =   TRS("Misc"),
      .type =        BG_PARAMETER_SECTION,
    },
#if SCHRO_CHECK_VERSION(1,0,9)
    {
      .name = "enc_force_profile",
      .long_name = TRS("Force profile"),
      .type = BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "Auto" },
      .multi_names = (const char *[]){ TRS("Auto"),
                                        TRS("VC-2 low delay"),
                                        TRS("VC-2 simple"),
                                        TRS("VC-2 main"),
                                        TRS("Main"),
                                        (char *)0 },
    },
    {
      .name = "enc_codeblock_size",
      .long_name = TRS("Codeblock size"),
      .type = BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "Auto" },
      .multi_names = (const char *[]){ TRS("Auto"),
                                        TRS("Small"),
                                        TRS("Medium"),
                                        TRS("Large"),
                                        TRS("Full"),
                                        (char *)0 },
    },
#endif
    {
      .name      = "enc_enable_multiquant",
      .long_name = TRS("Enable multiquant"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      .name      = "enc_enable_dc_multiquant",
      .long_name = TRS("Enable DC multiquant"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
    },
#if SCHRO_CHECK_VERSION(1,0,9)
    {
      .name =        "enc_enable_noarith",
      .long_name =   TRS("Disable arithmetic coding"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i =     0 },
    },
    {
      .name =        "enc_downsample_levels",
      .long_name =   TRS("Downsample levels"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i =     5 },
      .val_min     = { .val_i =     2 },
      .val_max     = { .val_i =     8 },
    },
    {
      .name =        "enc_transform_depth",
      .long_name =   TRS("Transform depth"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i =     3 },
      .val_min     = { .val_i =     0 },
      .val_max     = { .val_i =     6 },
    },
#endif


#endif
    { /* End of parameters */ }

  };

static const bg_parameter_info_t * get_parameters_schro()
  {
  return parameters;
  }

static void set_parameter_schro(void * data, const char * name,
                                const bg_parameter_value_t * v)
  {
  
  }

static gavl_video_sink_t *
init_schro(void * data, gavl_video_format_t * format,
            gavl_metadata_t * stream_metadata,
            gavl_compression_info_t * ci)
  {
  return NULL;
  }

static int init_compressed_schro(bg_ogg_stream_t * s)
  {
  return 0;
  }

static void convert_packet(bg_ogg_stream_t * s, gavl_packet_t * src, ogg_packet * dst)
  {
  
  }

static int close_schro(void * data)
  {
  return 0;
  }

const bg_ogg_codec_t bg_schroedinger_codec =
  {
    .name =      "schroedinger",
    .long_name = TRS("Dirac encoder"),
    .create = create_schro,

    .get_parameters = get_parameters_schro,
    .set_parameter =  set_parameter_schro,
    //    .set_video_pass = set_video_pass_schro,
    .init_video =     init_schro,
    .init_video_compressed =     init_compressed_schro,

    .set_packet_sink = set_packet_sink,
    
    .convert_packet = convert_packet,
    .close = close_schro,
  };
