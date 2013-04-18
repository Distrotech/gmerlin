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

#include "gavf-server.h"

#define LOG_DOMAIN "gavf-server.client"

typedef struct
  {
  int metaint;
  int byte_counter;
  gavl_metadata_t m;
  int changed;
  } icy_metadata_t;

static void icy_metadata_init(icy_metadata_t * m,
                              const gavl_metadata_t * req,
                              gavl_metadata_t * res)
  {
  int val;
  if(gavl_metadata_get_int(req, "Icy-MetaData", &val) &&
     (val > 0))
    {
    m->metaint = 16000;
    gavl_metadata_set_int(res, "icy-metaint", m->metaint);
    }
  }

static int icy_metadata_write(icy_metadata_t * m,
                              uint8_t * data, int len,
                              gavf_io_t * io)
  {
  if(!m->metaint)
    return gavf_io_write_data(io, data, len);

  if(len + m->byte_counter > m->metaint)
    {

    }

  return len;
  }

static void icy_metadata_update(icy_metadata_t * m,
                                const gavl_metadata_t * new_metadata)
  {
  
  }

/*
 *  bgplug
 */

static int supported_bgplug()
  {
  return 1;
  }

static int probe_bgplug(const gavf_program_header_t * ph,
                        const gavl_metadata_t * request)
  {
  return 1;
  }

static void * create_bgplug(const gavf_program_header_t * ph,
                            const gavl_metadata_t * req,
                            const gavl_metadata_t * vars,
                            gavl_metadata_t * res)
  {
  bg_plug_t * ret;

  ret = bg_plug_create_writer(plugin_reg);
  
  return ret;
  }

static int start_bgplug(void * priv,
                        const gavf_program_header_t * ph,
                        const gavl_metadata_t * req,
                        const gavl_metadata_t * vars,
                        gavf_io_t * io, int flags)
  {
  bg_parameter_value_t val;

  bg_plug_t * p = priv;

  val.val_i = 1024;
  gavl_metadata_get_int(vars, "shm", &val.val_i);
  bg_plug_set_parameter(p, "shm", &val);

  if(!bg_plug_open(p, io, NULL, NULL, flags))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "bg_plug_open failed");
    return 0;
    }
  
  if(!bg_plug_set_from_ph(p, ph))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "bg_plug_set_from_ph failed");
    return 0;
    }
  return 1;  
  }

static int put_buffer_bgplug(void * priv, buffer_element_t * el)
  {
  bg_plug_t * p = priv;

  switch(el->type)
    {
    case BUFFER_TYPE_PACKET:
      if(bg_plug_put_packet(p, &el->p) != GAVL_SINK_OK)
        return 0;
      break;
    case BUFFER_TYPE_METADATA:
      if(!gavf_update_metadata(bg_plug_get_gavf(p),
                               &el->m))
        return 0;
      break;
    }
  return 1;
  }

static void destroy_bgplug(void * priv)
  {
  bg_plug_t * p = priv;
  bg_plug_destroy(p);
  }

/*
 *  mp3
 */

typedef struct
  {
  icy_metadata_t m;
  gavf_io_t * io;
  } mp3_t;

static int supported_mp3()
  {
  return 1;
  }

static int probe_mp3(const gavf_program_header_t * ph,
                     const gavl_metadata_t * req)
  {
  const char * var;
  
  /* bgplug applications will always get a gavf stream */
  if((var = gavl_metadata_get(req, "User-Agent")) &&
     !strcmp(var, bg_plug_app_id))
    return 0;

  if(ph->num_streams != 1)
    return 0;
  if(ph->streams[0].type != GAVF_STREAM_AUDIO)
    return 0;
  if(ph->streams[0].ci.id != GAVL_CODEC_ID_MP3)
    return 0;
  if(ph->streams[0].ci.bitrate == GAVL_BITRATE_VBR)
    return 0;
  return 1;
  }

static void * create_mp3(const gavf_program_header_t * ph,
                         const gavl_metadata_t * req,
                         const gavl_metadata_t * vars,
                         gavl_metadata_t * res)
  {
  mp3_t * ret = calloc(1, sizeof(ret));
  return ret; 
  }

static int start_mp3(void * priv,
                     const gavf_program_header_t * ph,
                     const gavl_metadata_t * req,
                     const gavl_metadata_t * vars,
                     gavf_io_t * io, int flags)
  {
  mp3_t * m = priv;
  m->io = io;
  return 1;
  }

static int put_buffer_mp3(void * priv, buffer_element_t * el)
  {
  mp3_t * m = priv;
  return 0;
  }

static void destroy_mp3(void * priv)
  {
  
  }



static filter_t filters[] =
  {
    {
    .supported = supported_mp3,
    .probe = probe_mp3,
    .create = create_mp3,
    .start = start_mp3,
    .put_buffer = put_buffer_mp3,
    .destroy = destroy_mp3,
    },
#if 0
    {
    .supported = supported_aac,
    .probe = probe_aac,
    .create = create_aac,
    .start = start_aac,
    .put_buffer = put_buffer_aac,
    .destroy = destroy_aac,
    },
#endif
/* bgplug must come as last entry! */
    {
    .supported = supported_bgplug,
    .probe = probe_bgplug,
    .create = create_bgplug,
    .start = start_bgplug,
    .put_buffer = put_buffer_bgplug,
    .destroy = destroy_bgplug,
    },

    { /* End */ },
  }; 



void filter_init()
  {
  int i = 0;
  while(filters[i].supported)
    {
    filters[i].is_supported = filters[i].supported();
    i++;
    }
  }

const filter_t * filter_find(const gavf_program_header_t * ph,
                             const gavl_metadata_t * request)
  {
  int i = 0;
  while(filters[i].supported)
    {
    if(filters[i].is_supported &&
       filters[i].probe(ph, request))
      return &filters[i];
    i++;
    }
  return NULL;
  }
