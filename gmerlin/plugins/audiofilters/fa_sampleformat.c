/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "fa_sampleformat"

typedef struct
  {
  gavl_sample_format_t sample_format;
  int need_restart;

  bg_read_audio_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_audio_format_t format;

  bg_parameter_info_t * parameters;
  } sampleformat_priv_t;

static void * create_sampleformat()
  {
  sampleformat_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_sampleformat(void * priv)
  {
  sampleformat_priv_t * vp;
  vp = (sampleformat_priv_t *)priv;
  if(vp->parameters)
    bg_parameter_info_destroy_array(vp->parameters);
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "sampleformat",
      .long_name = TRS("Sampleformat"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "YUV 420 Planar" },
    },
    { /* End of parameters */ },
  };

static bg_parameter_info_t * create_parameters()
  {
  int i, index, num;
  bg_parameter_info_t * ret;
  gavl_sample_format_t f;

  ret = bg_parameter_info_copy_array(parameters);

  num = gavl_num_sample_formats();

  ret->multi_names_nc  = calloc(num+1, sizeof(*ret->multi_names));
  ret->multi_labels_nc = calloc(num+1, sizeof(*ret->multi_labels));
  bg_parameter_info_set_const_ptrs(ret);
  index = 0;
  for(i = 0; i < num; i++)
    {
    f = gavl_get_sample_format(i);
    if(f != GAVL_SAMPLE_NONE)
      {
      ret->multi_names_nc[index] = bg_strdup((char*)0,
                                          gavl_sample_format_to_string(f));
      ret->multi_labels_nc[index] = bg_strdup((char*)0,
                                           gavl_sample_format_to_string(f));
      index++;
      }
    }
  
  return ret;
  }

static const bg_parameter_info_t * get_parameters_sampleformat(void * priv)
  {
  sampleformat_priv_t * vp;
  vp = (sampleformat_priv_t *)priv;
  if(!vp->parameters)
    vp->parameters = create_parameters();
  return vp->parameters;
  }

static void
set_parameter_sampleformat(void * priv, const char * name,
                          const bg_parameter_value_t * val)
  {
  sampleformat_priv_t * vp;
  gavl_sample_format_t f;
  vp = (sampleformat_priv_t *)priv;
  
  if(!name)
    return;
  
  if(!strcmp(name, "sampleformat"))
    {
    f = gavl_string_to_sample_format(val->val_str);
    if(vp->sample_format != f)
      {
      vp->need_restart = 1;
      vp->sample_format = f;
      }
    }
  }

static void connect_input_port_sampleformat(void * priv,
                                    bg_read_audio_func_t func,
                                    void * data, int stream, int port)
  {
  sampleformat_priv_t * vp;
  vp = (sampleformat_priv_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

static void set_input_format_sampleformat(void * priv,
                                         gavl_audio_format_t * format,
                                         int port)
  {
  sampleformat_priv_t * vp;
  vp = (sampleformat_priv_t *)priv;

  if(!port)
    {
    format->sample_format = vp->sample_format;
    gavl_audio_format_copy(&vp->format, format);
    vp->need_restart = 0;
    }
  }

static void get_output_format_sampleformat(void * priv, gavl_audio_format_t * format)
  {
  sampleformat_priv_t * vp;
  vp = (sampleformat_priv_t *)priv;
  
  gavl_audio_format_copy(format, &vp->format);
  }

static int need_restart_sampleformat(void * priv)
  {
  sampleformat_priv_t * vp;
  vp = (sampleformat_priv_t *)priv;
  return vp->need_restart;
  }

static int read_audio_sampleformat(void * priv,
                                  gavl_audio_frame_t * frame, int stream, int num_samples)
  {
  sampleformat_priv_t * vp;
  vp = (sampleformat_priv_t *)priv;
  
  return vp->read_func(vp->read_data, frame, vp->read_stream, num_samples);
  }

const bg_fa_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fa_sampleformat",
      .long_name = TRS("Force sampleformat"),
      .description = TRS("This forces a sampleformat as input for the next filter. Its mainly used for testing."),
      .type =     BG_PLUGIN_FILTER_AUDIO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_sampleformat,
      .destroy =   destroy_sampleformat,
      .get_parameters =   get_parameters_sampleformat,
      .set_parameter =    set_parameter_sampleformat,
      .priority =         1,
    },
    
    .connect_input_port = connect_input_port_sampleformat,
    
    .set_input_format = set_input_format_sampleformat,
    .get_output_format = get_output_format_sampleformat,

    .read_audio = read_audio_sampleformat,
    .need_restart = need_restart_sampleformat,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
