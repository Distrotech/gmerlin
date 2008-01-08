/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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
#include <log.h>
#define LOG_DOMAIN "oa_pulse"

static int open_pulse(void * data,
                      gavl_audio_format_t * format)
  {
  bg_pa_t * priv;
  priv = (bg_pa_t *)data;

  gavl_audio_format_copy(&priv->format, format);
  
  if(!bg_pa_open(priv, 0))
    {
    return 0;
    }
  gavl_audio_format_copy(format, &priv->format);
  priv->sample_count = 0;
  return 1;
  }

static int start_pulse(void * p)
  {
  return 1;
  }

static void stop_pulse(void * p)
  {
  
  }

static void close_pulse(void * p)
  {
  bg_pa_close(p);
  }

static void write_frame_pulse(void * p, gavl_audio_frame_t * f)
  {
  bg_pa_t * priv;
  int error;
  priv = (bg_pa_t *)p;
  pa_simple_write(priv->pa,
                  f->samples.u_8,
                  priv->block_align * f->valid_samples,
                  &error);
  }

static int get_delay_pulse(void * p)
  {
  bg_pa_t * priv;
  int error;
  int ret;
  priv = (bg_pa_t *)p;
  ret = gavl_time_rescale(1000000, priv->format.samplerate,
                          pa_simple_get_latency(priv->pa, &error));
  return ret;
  }

bg_oa_plugin_t the_plugin =
  {
    common:
    {
      BG_LOCALE,
      name:          "oa_pulse",
      long_name:     TRS("Pulse"),
      description:   TRS("PulseAudio output"),
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      type:          BG_PLUGIN_OUTPUT_AUDIO,
      flags:         BG_PLUGIN_PLAYBACK,
      priority:      BG_PLUGIN_PRIORITY_MAX,
      create:        bg_pa_create,
      destroy:       bg_pa_destroy,
      
      //      get_parameters: get_parameters_alsa,
      //      set_parameter:  set_parameter_alsa,
    },

    open:          open_pulse,
    start:         start_pulse,
    write_frame:   write_frame_pulse,
    stop:          stop_pulse,
    close:         close_pulse,
    get_delay:     get_delay_pulse,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
