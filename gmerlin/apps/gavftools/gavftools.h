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

bg_cfg_section_t *
gavftools_ac_section(void);

bg_cfg_section_t *
gavftools_vc_section(void);

void
gavftools_opt_ac(void * data, int * argc, char *** _argv, int arg);

void
gavftools_opt_vc(void * data, int * argc, char *** _argv, int arg);


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
