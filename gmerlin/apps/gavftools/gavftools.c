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

#include <pthread.h>
#include <signal.h>

#include "gavftools.h"

bg_plugin_registry_t * plugin_reg;
bg_cfg_registry_t * cfg_reg;


void gavftools_init_registries()
  {
  char * tmp_path;
  bg_cfg_section_t * cfg_section;
    
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  }

static bg_cfg_section_t * ac_section = NULL;
static bg_cfg_section_t * vc_section = NULL;

static bg_parameter_info_t * ac_parameters = NULL;
static bg_parameter_info_t * vc_parameters = NULL;

void gavftools_destroy_registries()
  {
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);

  if(ac_section)
    bg_cfg_section_destroy(ac_section);
  if(vc_section)
    bg_cfg_section_destroy(vc_section);
  if(ac_parameters)
    bg_parameter_info_destroy_array(ac_parameters);
  if(vc_section)
    bg_parameter_info_destroy_array(vc_parameters);
  }

const bg_parameter_info_t * gavftools_ac_params(void)
  {
  if(!ac_parameters)
    {
    ac_parameters =
      bg_plugin_registry_create_compressor_parameters(plugin_reg,
                                                      BG_PLUGIN_AUDIO_COMPRESSOR);
    
    }
  return ac_parameters;
  }

const bg_parameter_info_t * gavftools_vc_params(void)
  {
  if(!vc_parameters)
    {
    vc_parameters =
      bg_plugin_registry_create_compressor_parameters(plugin_reg,
                                                      BG_PLUGIN_VIDEO_COMPRESSOR);
    
    }
  return vc_parameters;

  }

bg_cfg_section_t * gavftools_ac_section(void)
  {
  if(!ac_section)
    {
    ac_section =
      bg_cfg_section_create_from_parameters("ac",
                                            gavftools_ac_params());
    }
  return ac_section;
  }

bg_cfg_section_t * gavftools_vc_section(void)
  {
  if(!vc_section)
    {
    vc_section =
      bg_cfg_section_create_from_parameters("vc",
                                            gavftools_vc_params());
    }
  return vc_section;
  }


void
gavftools_opt_ac(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -ac requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(gavftools_ac_section(),
                               NULL,
                               NULL,
                               gavftools_ac_params(),
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }


void
gavftools_opt_vc(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vc requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(gavftools_vc_section(),
                               NULL,
                               NULL,
                               gavftools_vc_params(),
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }


void
gavftools_block_sigpipe(void)
  {
  signal(SIGPIPE, SIG_IGN);
#if 0
  sigset_t newset;
  /* Block SIGPIPE */
  sigemptyset(&newset);
  sigaddset(&newset, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &newset, NULL);
#endif
  }
