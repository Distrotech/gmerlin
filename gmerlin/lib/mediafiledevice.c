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

#include <config.h>
#include <string.h>


#include <gmerlin/translation.h>
#include <gmerlin/converters.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/pluginregistry.h>

#include <gmerlin/mediafiledevice.h>
#include <gmerlin/utils.h>

typedef struct
  {
  bg_plugin_registry_t * plugin_reg;

  gavl_audio_format_t in_format;
  gavl_audio_format_t out_format;

  bg_audio_converter_t * cnv;
  gavl_audio_options_t * opt;

  int num_files;
  int current;
  int do_shuffle;
  char * album_file;
  
  struct
    {
    const char * location;
    const char * plugin;
    } * files;

  /* Shuffle list */
  int * indices;
  
  } audiofile_t;

static const bg_parameter_info_t parameters_audio[] =
  {
    {
      .name =        "channel_mode",
      .long_name =   TRS("Channel Mode"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "stereo" },
      .multi_names =   (char const *[]){ "mono", "stereo", NULL },
      .multi_labels =  (char const *[]){ TRS("Mono"), TRS("Stereo"), NULL },
    },
    {
      .name =        "samplerate",
      .long_name =   TRS("Samplerate [Hz]"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 44100 },
      .val_min =     { .val_i =  8000 },
      .val_max =     { .val_i = 96000 },
    },
    {
      .name =        "album",
      .long_name =   TRS("Album"),
      .type        =   BG_PARAMETER_FILE,
    },
    {
      .name =       "shuffle",
      .long_name =     TRS("Shuffle"),
      .type      =   BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of parameters */ }
  };


static const bg_parameter_info_t * get_parameters_audio(void * priv)
  {
  return parameters_audio;
  }


static void
set_parameter_audio(void * p, const char * name,
                    const bg_parameter_value_t * val)
  {
  audiofile_t * m = p;
  if(!name)
    return;

  if(!strcmp(name, "channel_mode"))
    {
    if(!strcmp(val->val_str, "mono"))
      m->out_format.num_channels = 1;
    else if(!strcmp(val->val_str, "stereo"))
      m->out_format.num_channels = 2;
    }
  else if(!strcmp(name, "samplerate"))
    m->out_format.samplerate = val->val_i;
  else if(!strcmp(name, "album"))
    m->album_file = bg_strdup(m->album_file, val->val_str);
  else if(!strcmp(name, "shuffle"))
    m->do_shuffle = val->val_i;
  
  }

static void destroy_audio(void * p)
  {
  audiofile_t * m = p;
  bg_audio_converter_destroy(m->cnv);
  gavl_audio_options_destroy(m->opt);
  }

static int open_audio(void * p,
                      gavl_audio_format_t * format,
                      gavl_video_format_t * video_format)
  {
  char * album_xml;
  audiofile_t * m = p;
  /* Finalize audio format */

  m->out_format.channel_locations[0] = GAVL_CHID_NONE;
  gavl_set_channel_setup(&m->out_format);

  gavl_audio_format_copy(format, &m->out_format);

  /* Create track list */
  if(!m->album_file)
    return 0;

  
  
  /* Open first file */
  
  
  return 0;
  }

static void close_audio(void * p)
  {
  audiofile_t * m = p;
  
  }

static int read_frame_audio(void * p, gavl_audio_frame_t * f,
                            int stream,
                            int num_samples)
  {
  audiofile_t * m = p;
  return 0;
  }

static const bg_recorder_plugin_t audiofile_input =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "i_audiofile",
      .long_name =     TRS("Audiofile recorder"),
      .description =   TRS("Take a bunch of audio file and make them available as a recording device"),
      .type =          BG_PLUGIN_RECORDER_AUDIO,
      .flags =         BG_PLUGIN_RECORDER,
      .priority =      1,
      .destroy =       destroy_audio,

      .get_parameters = get_parameters_audio,
      .set_parameter =  set_parameter_audio,
    },
    .open =          open_audio,
    .read_audio =    read_frame_audio,
    .close =         close_audio,

  };

bg_plugin_info_t * bg_audiofiledevice_info(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * ret;

  const bg_recorder_plugin_t * plugin = &audiofile_input;
  
  if(!bg_plugin_registry_get_num_plugins(reg, BG_PLUGIN_INPUT, BG_PLUGIN_FILE))
    return NULL;
  
  ret = calloc(1, sizeof(*ret));

  ret->gettext_domain      = bg_strdup(ret->gettext_domain, plugin->common.gettext_domain);
  
  ret->gettext_directory   = bg_strdup(ret->gettext_directory,
                                       plugin->common.gettext_directory);
  
  
  ret->name      = bg_strdup(ret->name, plugin->common.name);
  ret->long_name = bg_strdup(ret->long_name, plugin->common.long_name);
  ret->description = bg_strdup(ret->description, plugin->common.description);
  
  ret->priority  =  plugin->common.priority;
  ret->type  =  plugin->common.type;
  ret->flags =  plugin->common.flags;
  ret->parameters = bg_parameter_info_copy_array(parameters_audio);
  return ret;
  }

const bg_plugin_common_t * bg_audiofiledevice_get()
  {
  return (const bg_plugin_common_t*)(&audiofile_input);
  }

void * bg_audiofiledevice_create(bg_plugin_registry_t * reg)
  {
  audiofile_t * ret = calloc(1, sizeof(*ret));
  ret->plugin_reg = reg;

  /* Set common audio format parameters */
  ret->out_format.sample_format = GAVL_SAMPLE_FLOAT;
  ret->out_format.interleave_mode = GAVL_INTERLEAVE_NONE;

  ret->opt = gavl_audio_options_create();
  ret->cnv = bg_audio_converter_create(ret->opt);
  
  return ret;
  } 
