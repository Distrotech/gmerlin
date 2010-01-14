/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <jack/jack.h>
#include <jack/ringbuffer.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/plugin.h>

#include <gmerlin/log.h>


typedef struct
  {
  gavl_channel_id_t channel_id;
  const char * ext_name;
  jack_ringbuffer_t * buffer;
  jack_port_t * int_port;
  int active;
  int index; /* In format */
  } port_t;

typedef struct
  {
  jack_client_t *client;
  
  gavl_audio_format_t format;

  const char ** ext_ports;

  /* Thread is running */
  int running;
  pthread_mutex_t running_mutex;

  /* Do stop */
  int active;
  pthread_mutex_t active_mutex;
  
  int num_ports;
  port_t * ports;

  int samples_per_frame;
  int samplerate;

  int connect_ports;

  int64_t samples_read; /* For i_jack only */
  } jack_t;

const bg_parameter_info_t * bg_jack_get_parameters(void * p);

void
bg_jack_set_parameter(void * p, const char * name,
                      const bg_parameter_value_t * val);


void * bg_jack_create();

int bg_jack_start(void * data);

void bg_jack_stop(void * data);

int bg_jack_open_client(jack_t * priv, int output,
                        int (*process)(jack_nframes_t, void *));

int bg_jack_close_client(jack_t * priv);

void bg_jack_destroy(void * p);
