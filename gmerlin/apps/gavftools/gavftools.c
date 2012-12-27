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
#include <string.h>

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
  
  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  bg_cfg_registry_save(cfg_reg, tmp_path);

  if(tmp_path)
    free(tmp_path);
  }

static bg_cfg_section_t * ac_section = NULL;
static bg_cfg_section_t * vc_section = NULL;
static bg_cfg_section_t * oc_section = NULL;

static bg_parameter_info_t * ac_parameters = NULL;
static bg_parameter_info_t * vc_parameters = NULL;
static bg_parameter_info_t * oc_parameters = NULL;

static bg_stream_action_t * a_actions = NULL;
static int num_a_actions = 0;

static bg_stream_action_t * v_actions = NULL;
static int num_v_actions = 0;

static bg_stream_action_t * t_actions = NULL;
static int num_t_actions = 0;

static bg_stream_action_t * o_actions = NULL;
static int num_o_actions = 0;

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

const bg_parameter_info_t * gavftools_oc_params(void)
  {
  if(!oc_parameters)
    {
    oc_parameters =
      bg_plugin_registry_create_compressor_parameters(plugin_reg,
                                                      BG_PLUGIN_OVERLAY_COMPRESSOR);
    
    }
  return oc_parameters;

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

bg_cfg_section_t * gavftools_oc_section(void)
  {
  if(!oc_section)
    {
    oc_section =
      bg_cfg_section_create_from_parameters("oc",
                                            gavftools_oc_params());
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
gavftools_opt_oc(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -oc requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(gavftools_oc_section(),
                               NULL,
                               NULL,
                               gavftools_oc_params(),
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

/* Stream actions */

static bg_stream_action_t * stream_actions_from_arg(const char * arg, int * num_p)
  {
  int i;
  char ** actions_c;
  bg_stream_action_t * ret;
  int num;

  actions_c = bg_strbreak(arg, ',');
  num = 0;
  while(actions_c[num])
    num++;

  ret = calloc(num, sizeof(*ret));

  for(i = 0; i < num; i++)
    {
    if(*(actions_c[i]) == 'd')
      ret[i] = BG_STREAM_ACTION_DECODE;
    else if(*(actions_c[i]) == 'm')
      ret[i] = BG_STREAM_ACTION_OFF;
    else if(*(actions_c[i]) == 'c')
      ret[i] = BG_STREAM_ACTION_READRAW;
    }
  *num_p = num;
  return ret;
  }

void
gavftools_opt_as(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -as requires an argument\n");
    exit(-1);
    }

  a_actions = stream_actions_from_arg((*_argv)[arg], &num_a_actions);
  bg_cmdline_remove_arg(argc, _argv, arg);
  
  }

void
gavftools_opt_vs(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vs requires an argument\n");
    exit(-1);
    }
  v_actions = stream_actions_from_arg((*_argv)[arg], &num_v_actions);
  bg_cmdline_remove_arg(argc, _argv, arg);
  
  }

void
gavftools_opt_os(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -os requires an argument\n");
    exit(-1);
    }
  o_actions = stream_actions_from_arg((*_argv)[arg], &num_o_actions);
  bg_cmdline_remove_arg(argc, _argv, arg);

  }

void
gavftools_opt_ts(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -ts requires an argument\n");
    exit(-1);
    }
  t_actions = stream_actions_from_arg((*_argv)[arg], &num_t_actions);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

bg_stream_action_t * gavftools_get_stream_actions(int num, gavf_stream_type_t type)
  {
  int i;
  int num_actions;
  bg_stream_action_t * actions;
  bg_stream_action_t * ret;

  if(!num)
    return NULL;
  
  switch(type)
    {
    case GAVF_STREAM_AUDIO:
      actions = a_actions;
      num_actions = num_a_actions;
      break;
    case GAVF_STREAM_VIDEO:
      actions = v_actions;
      num_actions = num_v_actions;
      break;
    case GAVF_STREAM_TEXT:
      actions = t_actions;
      num_actions = num_t_actions;
      break;
    case GAVF_STREAM_OVERLAY:
      actions = o_actions;
      num_actions = num_o_actions;
      break;
    default:
      return NULL;
    }

  ret = calloc(num, sizeof(*ret));
  
  i = num;
  if(i > num_actions)
    i = num_actions;

  memcpy(ret, actions, i * sizeof(*actions));

  if(num > num_actions)
    {
    for(i = num_actions; i < num; i++)
      ret[i] = BG_STREAM_ACTION_OFF;
    }
  return ret;
  }

