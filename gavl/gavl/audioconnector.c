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
#include <audio.h>

typedef struct
  {
  gavl_audio_sink_t * sink;
  gavl_audio_frame_t * frame;
  gavl_audio_source_t * src;

  const gavl_audio_format_t * fmt;
  
  int * penalties;
  } sink_t;

struct gavl_audio_connector_s
  {
  gavl_audio_source_t * src;
  const gavl_audio_format_t * fmt;
  
  sink_t * sinks;
  int num_sinks;
  int sinks_alloc;

  gavl_audio_connector_process_func process_func;
  void * process_priv;
  
  gavl_audio_options_t opt;
  };

gavl_audio_connector_t *
gavl_audio_connector_create(gavl_audio_source_t * src)
  {
  gavl_audio_connector_t * ret = calloc(1, sizeof(*ret));
  ret->src = src;
  
  ret->fmt = gavl_audio_source_get_src_format(ret->src);
  gavl_audio_options_set_defaults(&ret->opt);
  return ret;
  }

gavl_audio_options_t *
gavl_audio_connector_get_options(gavl_audio_connector_t * c)
  {
  return &c->opt;
  }
  
void gavl_audio_connector_destroy(gavl_audio_connector_t * c)
  {
  int i;

  for(i = 0; i < c->num_sinks; i++)
    {
    if(c->sinks[i].src)
      gavl_audio_source_destroy(c->sinks[i].src);
    }
  
  if(c->sinks)
    free(c->sinks);
  free(c);
  }

void gavl_audio_connector_connect(gavl_audio_connector_t * c,
                                  gavl_audio_sink_t * sink)
  {
  sink_t * s;
  if(c->num_sinks + 1 > c->sinks_alloc)
    {
    c->sinks_alloc += 8;
    c->sinks = realloc(c->sinks, c->sinks_alloc * sizeof(*c->sinks));
    memset(c->sinks + c->num_sinks, 0,
           (c->sinks_alloc - c->num_sinks) * sizeof(*c->sinks));
    }
  s = c->sinks + c->num_sinks;
  s->sink = sink;
  s->fmt = gavl_audio_sink_get_format(s->sink);

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

static int get_penalty(gavl_audio_converter_t * cnv,
                       const gavl_audio_format_t * src_fmt,
                       const gavl_audio_format_t * dst_fmt)
  {
  return 0;
  }

void gavl_audio_connector_start(gavl_audio_connector_t * c)
  {
  int i, j;
  gavl_audio_converter_t * cnv;
  sink_t * s;
  cnv = gavl_audio_converter_create();
  
  /* get penalties */
  for(i = 0; i < c->num_sinks; i++)
    {
    s = c->sinks + i;
    if(s->penalties)
      free(s->penalties);
    
    s->penalties =
      malloc(c->num_sinks * sizeof(*s->penalties));
    
    for(j = 0; j < c->num_sinks; j++)
      {
      if(j == i) // Get penalty with respect to source
        s->penalties[j] = get_penalty(cnv, c->fmt, s->fmt);
      else
        s->penalties[j] = get_penalty(cnv, c->sinks[j].fmt, s->fmt);
      }
    
    }
  }
