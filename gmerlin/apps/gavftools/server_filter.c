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
#include <gavl/metatags.h>
#include <gmerlin/charset.h>

#include "gavf-server.h"

#define LOG_DOMAIN "gavf-server.client"

typedef struct
  {
  int metaint;
  int byte_counter;
  gavl_metadata_t m;
  int changed;
  bg_charset_converter_t * cnv;
  } icy_t;

static void icy_init(icy_t * m,
                     const gavl_metadata_t * req,
                     gavl_metadata_t * res)
  {
  int val;
#if 1
  if(gavl_metadata_get_int(req, "Icy-MetaData", &val) &&
     (val > 0))
    {
    m->metaint = 16000;
    gavl_metadata_set_int(res, "icy-metaint", m->metaint);
    m->cnv = bg_charset_converter_create("UTF-8", "ISO_8859-1");
    }
#endif
  }

static int icy_write_metadata(icy_t * m,
                              gavf_io_t * io)
  {
  int len, ret;
  const char * artist;
  const char * title;
  const char * label;
  char * buffer;
  
  char * tmp_string;
  char * stream_title;
  if(!m->changed)
    {
    uint8_t len = 0;
    m->byte_counter = 0;
    return gavf_io_write_data(io, &len, 1) == 1;
    }

  artist = gavl_metadata_get(&m->m, GAVL_META_ARTIST);
  title  = gavl_metadata_get(&m->m, GAVL_META_TITLE);
  label  = gavl_metadata_get(&m->m, GAVL_META_LABEL);

  if(artist && title)
    stream_title = bg_sprintf("%s - %s", artist, title);
  else if(label)
    stream_title = gavl_strdup(label);
  else if(title)
    stream_title = gavl_strdup(title);
  else
    stream_title = gavl_strdup("Stream title");

  /* Convert charset */
  tmp_string = bg_convert_string(m->cnv, stream_title, -1, NULL);
  free(stream_title);
  stream_title = tmp_string;

  /* Escape */
  stream_title = gavl_escape_string(stream_title, ";'");

  /* Create buffer */
  len = 14 + // <len>StreamTitle='
    strlen(stream_title) + 
    3;       // ';\0
  
  len = ((len + 15) / 16) * 16;
  
  buffer = calloc(len+1, 1);
  snprintf(buffer, len+1, "%cStreamTitle='%s';", len/16, stream_title);

  gavl_hexdump((uint8_t*)buffer, len+1, 16);
  
  ret = (gavf_io_write_data(io, (uint8_t*)buffer, len+1) == len+1);

  free(buffer);
  free(stream_title);
  
  m->changed = 0;
  m->byte_counter = 0;
  return ret;
  }

static int icy_write(icy_t * m,
                     uint8_t * data, int len,
                     gavf_io_t * io)
  {
  int bytes_written;
  int bytes;
  
  if(!m->metaint)
    return (gavf_io_write_data(io, data, len) == len);

  // fprintf(stderr, "icy_write\n");
  
  bytes_written = 0;
  while(bytes_written < len)
    {
    bytes = len - bytes_written;
    
    if(m->byte_counter + bytes > m->metaint)
      {
      /* Write data */
      bytes = m->metaint - m->byte_counter;
      }

    if(bytes &&
       (gavf_io_write_data(io, data + bytes_written, bytes) < bytes))
      return 0;
    
    m->byte_counter += bytes;
    bytes_written += bytes;
    
    if(m->byte_counter == m->metaint)
      {
      /* Write metadata */
      if(!icy_write_metadata(m, io))
        return 0;
      }
    }
  return len;
  }

static void icy_update(icy_t * m,
                       const gavl_metadata_t * new_metadata)
  {
  gavl_metadata_copy(&m->m, new_metadata);
  m->changed = 1;
  }

static void icy_free(icy_t * m)
  {
  gavl_metadata_free(&m->m);
  if(m->cnv)
    bg_charset_converter_destroy(m->cnv);
  }


/*
 *  bgplug
 */

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
  icy_t m;
  gavf_io_t * io;
  } mp3_t;

static int supported_mp3()
  {
  return 1;
  }

static int probe_mp3(const gavf_program_header_t * ph,
                     const gavl_metadata_t * req)
  {
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
  mp3_t * ret = calloc(1, sizeof(*ret));
  icy_init(&ret->m, req, res);

  gavl_metadata_set(res, "Content-Type", "audio/mpeg");

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
  
  switch(el->type)
    {
    case BUFFER_TYPE_PACKET:
      return icy_write(&m->m, el->p.data, el->p.data_len, m->io);
      break;
    case BUFFER_TYPE_METADATA:
      icy_update(&m->m, &el->m);
      return 1;
      break;
    }
  return 1;
  }

static void destroy_mp3(void * priv)
  {
  mp3_t * m = priv;
  icy_free(&m->m);
  if(m->io)
    gavf_io_destroy(m->io);
  free(m);
  }

static filter_t filter_bgplug =
  {
    .is_supported = 1,
    .probe = probe_bgplug,
    .create = create_bgplug,
    .start = start_bgplug,
    .put_buffer = put_buffer_bgplug,
    .destroy = destroy_bgplug,
  };

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
                             const gavl_metadata_t * req)
  {
  int i = 0;
  const char * var;
  
  /* bgplug applications will always get a gavf stream */
  if((var = gavl_metadata_get(req, "User-Agent")) &&
     !strcmp(var, bg_plug_app_id))
    return &filter_bgplug;
  
  while(filters[i].supported)
    {
    if(filters[i].is_supported &&
       filters[i].probe(ph, req))
      return &filters[i];
    i++;
    }
  
  /* Fallback */
  return &filter_bgplug;
  }
