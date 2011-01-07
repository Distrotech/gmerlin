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

#include <pulseaudio_common.h>
#include <string.h>

#include <gmerlin/utils.h>

static int open_pulse(void * data,
                      gavl_audio_format_t * format,
                      gavl_video_format_t * video_format)
  {
  bg_pa_t * priv;
  priv = (bg_pa_t *)data;

  // gavl_audio_format_copy(&priv->format, format);
  
  if(!bg_pa_open(priv, 1))
    {
    return 0;
    }
  gavl_audio_format_copy(format, &priv->format);
  priv->sample_count = 0;
  return 1;
  }

static void close_pulse(void * p)
  {
  bg_pa_close(p);
  }

static int read_frame_pulse(void * p, gavl_audio_frame_t * f,
                            int stream,
                            int num_samples)
  {
  bg_pa_t * priv;
  int error = 0;
  priv = (bg_pa_t *)p;
  pa_simple_read(priv->pa, f->samples.u_8,
                 priv->block_align * num_samples,
                 &error);
  f->valid_samples = num_samples;
  f->timestamp = priv->sample_count;
  priv->sample_count += num_samples;
  return num_samples;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "channel_mode",
      .long_name =   TRS("Channel Mode"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "stereo" },
      .multi_names =   (char const *[]){ "mono", "stereo", (char*)0 },
      .multi_labels =  (char const *[]){ TRS("Mono"), TRS("Stereo"), (char*)0 },
    },
    {
      .name =        "bits",
      .long_name =   TRS("Bits"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "16" },
      .multi_names =     (char const *[]){ "8", "16", (char*)0 },
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
      .name =        "server",
      .long_name =   TRS("Server"),
      .type =        BG_PARAMETER_STRING,
      .help_string = TRS("Server to connect to. Leave empty for default."),
    },
    {
      .name =        "dev",
      .long_name =   TRS("Source"),
      .type =        BG_PARAMETER_STRING,
      .help_string = TRS("Source to open. Use the PulseAudio manager for available Sources."),
    },
    { },
  };

static const bg_parameter_info_t * get_parameters_pulse(void * data)
  {
  return parameters;
  }

static void
set_parameter_pulse(void * p, const char * name,
                    const bg_parameter_value_t * val)
  {
  bg_pa_t * priv = (bg_pa_t*)(p);
  
  if(!name)
    return;
  
  if(!strcmp(name, "channel_mode"))
    {
    if(!strcmp(val->val_str, "mono"))
      priv->num_channels = 1;
    else if(!strcmp(val->val_str, "stereo"))
      priv->num_channels = 2;
    }
  else if(!strcmp(name, "bits"))
    {
    if(!strcmp(val->val_str, "8"))
      priv->bytes_per_sample = 1;
    else if(!strcmp(val->val_str, "16"))
      priv->bytes_per_sample = 2;
    }
  else if(!strcmp(name, "samplerate"))
    {
    priv->samplerate = val->val_i;
    }
  else if(!strcmp(name, "dev"))
    {
    priv->dev = bg_strdup(priv->dev, val->val_str);
    }
  else if(!strcmp(name, "server"))
    {
    priv->server = bg_strdup(priv->server, val->val_str);
    }
  }


const bg_recorder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "i_pulse",
      .long_name =     TRS("PulseAudio"),
      .description =   TRS("PulseAudio capture. You can specify the source, where we'll get the audio."),
      .type =          BG_PLUGIN_RECORDER_AUDIO,
      .flags =         BG_PLUGIN_RECORDER,
      .priority =      1,
      .create =        bg_pa_create,
      .destroy =       bg_pa_destroy,

      .get_parameters = get_parameters_pulse,
      .set_parameter =  set_parameter_pulse,
    },
    
    .open =          open_pulse,
    .read_audio =    read_frame_pulse,
    .close =         close_pulse,
  };
/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
