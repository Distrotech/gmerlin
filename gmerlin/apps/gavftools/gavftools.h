/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <config.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>
#include <gmerlin/cmdline.h>
#include <gmerlin/log.h>
#include <gmerlin/translation.h>
#include <gmerlin/bgplug.h>
#include <gmerlin/mediaconnector.h>

extern bg_plugin_registry_t * plugin_reg;
extern bg_cfg_registry_t * cfg_reg;

void gavftools_init_registries();
void gavftools_destroy_registries();

const bg_parameter_info_t *
gavftools_ac_params(void);

const bg_parameter_info_t *
gavftools_vc_params(void);

const bg_parameter_info_t *
gavftools_oc_params(void);

bg_cfg_section_t *
gavftools_ac_section(void);

bg_cfg_section_t *
gavftools_vc_section(void);

bg_cfg_section_t *
gavftools_oc_section(void);


void
gavftools_opt_ac(void * data, int * argc, char *** _argv, int arg);

void
gavftools_opt_vc(void * data, int * argc, char *** _argv, int arg);

void
gavftools_opt_oc(void * data, int * argc, char *** _argv, int arg);

void
gavftools_opt_as(void * data, int * argc, char *** _argv, int arg);

void
gavftools_opt_vs(void * data, int * argc, char *** _argv, int arg);

void
gavftools_opt_os(void * data, int * argc, char *** _argv, int arg);

void
gavftools_opt_ts(void * data, int * argc, char *** _argv, int arg);

bg_stream_action_t * gavftools_get_stream_actions(int num, gavf_stream_type_t type);


#define GAVFTOOLS_AUDIO_COMPRESSOR_OPTIONS \
  { \
    .arg =         "-ac", \
    .help_arg =    "<compression_options>", \
    .help_string = "Set audio compression options", \
    .callback =    gavftools_opt_ac, \
  }

#define GAVFTOOLS_VIDEO_COMPRESSOR_OPTIONS \
  { \
    .arg =         "-vc", \
    .help_arg =    "<compression_options>", \
    .help_string = "Set video compression options", \
    .callback =    gavftools_opt_vc, \
  }

#define GAVFTOOLS_OVERLAY_COMPRESSOR_OPTIONS      \
  { \
    .arg =         "-oc", \
    .help_arg =    "<compression_options>", \
    .help_string = "Set overlay compression options", \
    .callback =    gavftools_opt_oc, \
  }

#define GAVFTOOLS_AUDIO_STREAM_OPTIONS \
  { \
    .arg =         "-as", \
    .help_arg =    "<stream_selector>", \
    .help_string = "", \
    .callback =    gavftools_opt_as, \
  }

#define GAVFTOOLS_VIDEO_STREAM_OPTIONS          \
  { \
    .arg =         "-vs", \
    .help_arg =    "<stream_selector>", \
    .help_string = "", \
    .callback =    gavftools_opt_vs, \
  }

#define GAVFTOOLS_TEXT_STREAM_OPTIONS          \
  { \
    .arg =         "-ts", \
    .help_arg =    "<stream_selector>", \
    .help_string = "", \
    .callback =    gavftools_opt_ts, \
  }

#define GAVFTOOLS_OVERLAY_STREAM_OPTIONS           \
  { \
    .arg =         "-os", \
    .help_arg =    "<stream_selector>", \
    .help_string = "", \
    .callback =    gavftools_opt_os, \
  }

bg_stream_action_t * gavftools_get_stream_action(gavf_stream_type_t type,
                                                 int num);

void gavftools_set_compresspor_options(bg_cmdline_arg_t * global_options);

void gavftools_block_sigpipe(void);
