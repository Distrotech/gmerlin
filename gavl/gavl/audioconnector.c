/*****************************************************************
 * gavl - a general purpose audio/video processing library
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <gavl/gavl.h>

typedef struct
  {
  gavl_audio_sink_t * sink;
  gavl_audio_frame_t * frame;
  gavl_audio_source_t * src;
  } sink_t;

struct gavl_audio_connector_s
  {
  gavl_audio_source_t * src;

  sink_t * sinks;
  int num_sinks;
  int sinks_alloc;

  gavl_audio_connector_process_func process_func;
  void * process_priv;
  };

gavl_audio_connector_t *
gavl_audio_connector_create(gavl_audio_source_t * src)
  {
  gavl_audio_connector_t * ret = calloc(1, sizeof(*ret));
  ret->src = src;
  return ret;
  }

void gavl_audio_connector_destroy(gavl_audio_connector_t * c)
  {
  if(c->sinks)
    free(c->sinks);
  free(c);
  }

void gavl_audio_connector_connect(gavl_audio_connector_t * c,
                                  gavl_audio_sink_t * sink)
  {
  if(c->num_sinks + 1 > c->sinks_alloc)
    {
    c->sinks_alloc += 8;
    c->sinks = realloc(c->sinks, c->sinks_alloc * sizeof(*c->sinks));
    memset(c->sinks + c->num_sinks, 0,
           (c->sinks_alloc - c->num_sinks) * sizeof(*c->sinks));
    }
  c->sinks[c->num_sinks].sink = sink;
  c->num_sinks++;
  }

void
gavl_audio_connector_set_process_func(gavl_audio_connector_t * c,
                                      gavl_audio_connector_process_func func,
                                      void * priv)
  {
  c->process_func = func;
  c->process_priv = priv;
  }

int gavl_audio_connector_process(gavl_audio_connector_t * c)
  {
  return 0;
  }

void gavl_audio_connector_start(gavl_audio_connector_t * c)
  {
  
  }
