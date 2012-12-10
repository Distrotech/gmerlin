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

#include <string.h>

#include "gavftools.h"

#define LOG_DOMAIN "gavf-play"

static bg_plug_t * in_plug = NULL;

static char * infile = NULL;

static bg_cfg_section_t * audio_section = NULL;
static bg_cfg_section_t * video_section = NULL;

static const bg_parameter_info_t audio_parameters[] =
  {
    {
      .name      = "plugin",
      .long_name = TRS("Plugin"),
      .type      = BG_PARAMETER_MULTI_MENU,
      .flags     = BG_PARAMETER_PLUGIN,
    },
    { /* End */ },
  };

static const bg_parameter_info_t video_parameters[] =
  {
    {
      .name      = "plugin",
      .long_name = TRS("Plugin"),
      .type      = BG_PARAMETER_MULTI_MENU,
      .flags     = BG_PARAMETER_PLUGIN,
    },
    { /* End */ },
  };

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-aud",
      .help_arg =    "<audio_options>",
      .help_string = "Set audio recording options",
      .callback =    opt_aud,
    },
    {
      .arg =         "-vid",
      .help_arg =    "<video_options>",
      .help_string = "Set video recording options",
      .callback =    opt_vid,
    },
    {
      .arg =         "-o",
      .help_arg =    "<location>",
      .help_string = "Set output file or location",
      .callback =    opt_out,
    },
    {
      /* End */
    }
  };


typedef struct
  {
  bg_plugin_handle_t * h;
  bg_oa_plugin_t * plugin;
  bg_parameter_info_t * parameters;
  } audio_stream_t;

typedef struct
  {
  bg_plugin_handle_t * h;
  bg_ov_t * ov;
  bg_parameter_info_t * parameters;
  } video_stream_t;

int main(int argc, char ** argv)
  {
  bg_mediaconnector_t conn;
  
  bg_mediaconnector_init(&conn);
  gavftools_init_registries();

  in_plug = bg_plug_create_reader(plugin_reg);

  /* Select streams */

  /* Start plug */
  
  /* Create media connector */

  /* Start media connector */


  /* Main loop */
  while(1)
    {

    }

  /* Cleanup */
  
  }
