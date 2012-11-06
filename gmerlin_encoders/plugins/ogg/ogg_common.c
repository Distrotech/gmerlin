/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ogg/ogg.h>

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>

#include <gavl/metatags.h>


#include "ogg_common.h"

#define LOG_DOMAIN "ogg"

void * bg_ogg_encoder_create()
  {
  bg_ogg_encoder_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_ogg_encoder_destroy(void * data)
  {
  int i;
  bg_ogg_encoder_t * e = data;
  if(e->write_callback_data)
    bg_ogg_encoder_close(e, 1);
  if(e->audio_streams) free(e->audio_streams);

  for(i = 0; i < e->num_video_streams; i++)
    {
    if(e->video_streams[i].stats_file)
      free(e->video_streams[i].stats_file);
    }

  if(e->video_streams) free(e->video_streams);
  if(e->filename) free(e->filename);
  
  if(e->audio_parameters)
    bg_parameter_info_destroy_array(e->audio_parameters);
  
  free(e);
  }

void bg_ogg_encoder_set_callbacks(void * data, bg_encoder_callbacks_t * cb)
  {
  bg_ogg_encoder_t * e = data;
  e->cb = cb;
  }

static int write_file(void * priv, const uint8_t * data, int len)
  {
  return fwrite(data,1,len,priv);
  }

static void close_file(void * priv)
  {
  fclose(priv);
  }

int
bg_ogg_encoder_open(void * data, const char * file,
                    const gavl_metadata_t * metadata,
                    const gavl_chapter_list_t * chapter_list,
                    const char * ext)
  {
  bg_ogg_encoder_t * e = data;

  if(file)
    {
    e->filename = bg_filename_ensure_extension(file, ext);
    
    if(!bg_encoder_cb_create_output_file(e->cb, e->filename))
      return 0;
    
    if(!(e->write_callback_data = fopen(e->filename, "w")))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open file %s: %s",
             file, strerror(errno));
      return 0;
      }
    e->write_callback = write_file;
    e->close_callback = close_file;
    }
  else if(e->open_callback)
    {
    if(!e->open_callback(e->write_callback_data))
      return 0;
    }
  
  e->serialno = rand();
  if(metadata)
    gavl_metadata_copy(&e->metadata, metadata);
  return 1;
  }

int bg_ogg_stream_write_header_packet(bg_ogg_stream_t * s, ogg_packet * p)
  {
  if(!s->packetno)
    p->b_o_s = 1;
  else
    p->b_o_s = 0;
  
  p->packetno = s->packetno++;
  ogg_stream_packetin(&s->os, p);
  if(!s->num_headers)
    {
    if(bg_ogg_stream_flush_page(s, 1) <= 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Got no ID page");
      return 0;
      }
    }
  s->num_headers++;
  return 1;
  }

int bg_ogg_stream_flush_page(bg_ogg_stream_t * s, int force)
  {
  int result;
  ogg_page og;
  memset(&og, 0, sizeof(og));
  if(force)
    result = ogg_stream_flush(&s->os,&og);
  else
    result = ogg_stream_pageout(&s->os,&og);
  
  if(result)
    {
    if((s->enc->write_callback(s->enc->write_callback_data,og.header,og.header_len) < og.header_len) ||
       (s->enc->write_callback(s->enc->write_callback_data,og.body,og.body_len) < og.body_len))
      return -1;
    else
      return 1;
    }
  return 0;
  }

#if 0
int bg_ogg_flush_page(ogg_stream_state * os,
                      bg_ogg_encoder_t * output, int force)
  {
  int result;
  ogg_page og;
  memset(&og, 0, sizeof(og));
  if(force)
    result = ogg_stream_flush(os,&og);
  else
    result = ogg_stream_pageout(os,&og);
  
  if(result)
    {
    if((output->write_callback(output->write_callback_data,og.header,og.header_len) < og.header_len) ||
       (output->write_callback(output->write_callback_data,og.body,og.body_len) < og.body_len))
      return -1;
    else
      return 1;
    }
  return 0;
  }
#endif

int bg_ogg_stream_flush(bg_ogg_stream_t * s, int force)
  {
  int result, ret = 0;
  while((result = bg_ogg_stream_flush_page(s, force)) > 0)
    ret = 1;
  
  if(result < 0)
    return result;
  return ret;
  }

void bg_ogg_packet_to_gavl(bg_ogg_stream_t * s, ogg_packet * src,
                           gavl_packet_t * dst)
  {
  dst->data = src->packet;
  dst->data_len = src->bytes;
  dst->pts = s->last_pts;
  dst->duration = src->granulepos - dst->pts;
  if(src->e_o_s)
    dst->flags |= GAVL_PACKET_LAST;
  else
    dst->flags &= ~GAVL_PACKET_LAST;
  }

void bg_ogg_packet_from_gavl(bg_ogg_stream_t * s, gavl_packet_t * src,
                             ogg_packet * dst)
  {
  dst->packet = src->data;
  dst->bytes = src->data_len;
  dst->granulepos = src->pts + src->duration;
  if(src->flags & GAVL_PACKET_LAST)
    dst->e_o_s = 1;
  else
    dst->e_o_s = 0;
  }

int bg_ogg_stream_write_gavl_packet(bg_ogg_stream_t * s, gavl_packet_t * p)
  {
  ogg_packet op;
  memset(&op, 0, sizeof(op));
  bg_ogg_packet_from_gavl(s, p, &op);

  op.packetno = s->packetno++;
  ogg_stream_packetin(&s->os, &op);
  /* Flush pages if any */
  if(bg_ogg_stream_flush(s, 0) < 0)
    return 0;
  return 1;
  }

static bg_ogg_stream_t * append_stream(bg_ogg_encoder_t * e,
                                       bg_ogg_stream_t ** streams_p,
                                       int * num_streams_p,
                                       const gavl_metadata_t * m)
  {
  bg_ogg_stream_t * ret;
  bg_ogg_stream_t * streams = *streams_p;
  int num_streams = *num_streams_p;

  streams = realloc(streams, (num_streams+1)*sizeof(*streams));

  ret = streams + num_streams;

  memset(ret, 0, sizeof(*ret));
  ogg_stream_init(&ret->os, e->serialno++);
  ret->m = m;
  ret->enc = e;
  return ret;
  }

bg_ogg_stream_t *
bg_ogg_encoder_add_audio_stream(void * data,
                                const gavl_metadata_t * m,
                                const gavl_audio_format_t * format)
  {
  bg_ogg_stream_t * s;
  bg_ogg_encoder_t * e = data;

  s = append_stream(e, &e->audio_streams, &e->num_audio_streams, m);
  
  gavl_audio_format_copy(&e->audio_streams[e->num_audio_streams].afmt,
                         format);
  s->index = e->num_audio_streams++;
  return s;
  }

bg_ogg_stream_t *
bg_ogg_encoder_add_video_stream(void * data,
                                const gavl_metadata_t * m,
                                const gavl_video_format_t * format)
  {
  bg_ogg_stream_t * s;
  bg_ogg_encoder_t * e = data;
  s = append_stream(e, &e->video_streams, &e->num_video_streams, m);

  gavl_video_format_copy(&e->video_streams[e->num_video_streams].vfmt,
                         format);
  s->index = e->num_video_streams++;
  return s;
  }

bg_ogg_stream_t *
bg_ogg_encoder_add_audio_stream_compressed(void * data,
                                           const gavl_metadata_t * m,
                                           const gavl_audio_format_t * format,
                                           const gavl_compression_info_t * ci)
  {
  bg_ogg_stream_t * s;
  s = bg_ogg_encoder_add_audio_stream(data, m, format);
  s->ci = ci;
  return s;
  }

bg_ogg_stream_t *
bg_ogg_encoder_add_video_stream_compressed(void * data,
                                           const gavl_metadata_t * m,
                                           const gavl_video_format_t * format,
                                           const gavl_compression_info_t * ci)
  {
  bg_ogg_stream_t * s;
  s = bg_ogg_encoder_add_video_stream(data, m, format);
  s->ci = ci;
  return s;
  }

void bg_ogg_encoder_init_stream(void * data, bg_ogg_stream_t * s,
                                const bg_ogg_codec_t * codec)
  {
  s->codec = codec;
  s->codec_priv = s->codec->create(s);
  }

void
bg_ogg_encoder_set_audio_parameter(void * data, int stream,
                                   const char * name,
                                   const bg_parameter_value_t * val)
  {
  bg_ogg_encoder_t * e = data;
  bg_ogg_stream_t * s = &e->audio_streams[stream];
  s->codec->set_parameter(s->codec_priv, name, val);
  }

void bg_ogg_encoder_set_video_parameter(void * data, int stream,
                                        const char * name,
                                        const bg_parameter_value_t * val)
  {
  bg_ogg_encoder_t * e = data;
  bg_ogg_stream_t * s = &e->video_streams[stream];
  s->codec->set_parameter(s->codec_priv, name, val);
  }

int bg_ogg_encoder_set_video_pass(void * data, int stream,
                                  int pass, int total_passes,
                                  const char * stats_file)
  {
  bg_ogg_encoder_t * e = data;

  e->video_streams[stream].pass = pass;
  e->video_streams[stream].total_passes = total_passes;
  e->video_streams[stream].stats_file =
    bg_strdup(e->video_streams[stream].stats_file, stats_file);
  return 1;
  }

static gavl_sink_status_t
write_audio_func(void * data, gavl_audio_frame_t * f)
  {
  bg_ogg_stream_t * as = data;
  return as->codec->encode_audio(as->codec_priv, f) ?
    GAVL_SINK_OK : GAVL_SINK_ERROR;
  }

static gavl_sink_status_t
write_video_func(void * data, gavl_video_frame_t*f)
  {
  bg_ogg_stream_t * vs = data;
  return vs->codec->encode_video(vs->codec_priv, f) ?
    GAVL_SINK_OK : GAVL_SINK_ERROR;
  }

static gavl_sink_status_t
write_packet_func(void * data, gavl_packet_t*p)
  {
  bg_ogg_stream_t * s = data;
  return s->codec->write_packet(s->codec_priv, p) ? GAVL_SINK_OK :
    GAVL_SINK_ERROR;
  }

static int start_audio(bg_ogg_encoder_t * e, int stream)
  {
  gavl_compression_info_t ci;
  bg_ogg_stream_t * s = &e->audio_streams[stream];

  memset(&ci, 0, sizeof(ci));
  
  if(s->ci)
    {
    if(!s->codec->init_audio_compressed(s->codec_priv,
                                        &s->afmt,
                                        s->ci, &e->metadata, s->m))
      return 0;
    }
  else
    {
    if(!s->codec->init_audio(s->codec_priv,
                             &s->afmt, &e->metadata, s->m, &ci))
      return 0;

    if(ci.id != GAVL_CODEC_ID_NONE)
      {
      if(!s->codec->init_audio_compressed(s->codec_priv,
                                          &s->afmt, &ci, &e->metadata, s->m))
        {
        gavl_compression_info_free(&ci);
        return 0;
        }
      }
    s->asink = gavl_audio_sink_create(NULL, write_audio_func, s, &s->afmt);
    }
  s->psink = gavl_packet_sink_create(NULL, write_packet_func, s);
  gavl_compression_info_free(&ci);
  return 1;
  }

static int start_video(bg_ogg_encoder_t * e, int stream)
  {
  gavl_compression_info_t ci;

  bg_ogg_stream_t * s = &e->video_streams[stream];
  
  memset(&ci, 0, sizeof(ci));
  if(s->ci)
    {
    if(!s->codec->init_video_compressed(s->codec_priv,
                                        &s->vfmt,
                                        s->ci,
                                        &e->metadata, s->m))
      return 0;
    }
  else
    {
    if(!s->codec->init_video(s->codec_priv,
                             &s->vfmt,
                             &e->metadata, s->m, &ci))
      return 0;
    
    if(s->pass)
      {
      if(!s->codec->set_video_pass)
        return 0;
      
      else 
        if(!s->codec->set_video_pass(s->codec_priv,
                                     s->pass,
                                     s->total_passes,
                                     s->stats_file))
          return 0;
      }
    if(ci.id != GAVL_CODEC_ID_NONE)
      {
      if(!s->codec->init_video_compressed(s->codec_priv,
                                          &s->vfmt, &ci, &e->metadata, s->m))
        {
        gavl_compression_info_free(&ci);
        return 0;
        }
      }
    s->vsink = gavl_video_sink_create(NULL, write_video_func, s, &s->vfmt);
    }
  
  s->psink = gavl_packet_sink_create(NULL, write_packet_func, s);
  gavl_compression_info_free(&ci);
  return 1;
  }

int bg_ogg_encoder_start(void * data)
  {
  int i;
  bg_ogg_encoder_t * e = data;

  /* Start encoders and write identification headers */
  for(i = 0; i < e->num_video_streams; i++)
    {
    if(!start_video(e, i))
      return 0;
    }
  for(i = 0; i < e->num_audio_streams; i++)
    {
    if(!start_audio(e, i))
      return 0;
    }

  /* Write remaining header pages */
  for(i = 0; i < e->num_video_streams; i++)
    {
    bg_ogg_stream_t * s = &e->video_streams[i];
    if(bg_ogg_stream_flush(s, 1) < 0)
      return 1;
    }
  for(i = 0; i < e->num_audio_streams; i++)
    {
    bg_ogg_stream_t * s = &e->audio_streams[i];
    if(bg_ogg_stream_flush(s, 1) < 0)
      return 1;
    }
  
  return 1;
  }

void bg_ogg_encoder_get_audio_format(void * data, int stream,
                                     gavl_audio_format_t*ret)
  {
  bg_ogg_encoder_t * e = data;
  gavl_audio_format_copy(ret, &e->audio_streams[stream].afmt);
  }

void bg_ogg_encoder_get_video_format(void * data, int stream,
                                     gavl_video_format_t*ret)
  {
  bg_ogg_encoder_t * e = data;
  gavl_video_format_copy(ret, &e->video_streams[stream].vfmt);
  }

gavl_audio_sink_t * bg_ogg_encoder_get_audio_sink(void * data, int stream)
  {
  bg_ogg_encoder_t * e = data;
  return e->audio_streams[stream].asink;
  }

gavl_video_sink_t * bg_ogg_encoder_get_video_sink(void * data, int stream)
  {
  bg_ogg_encoder_t * e = data;
  return e->video_streams[stream].vsink;
  }

gavl_packet_sink_t *
bg_ogg_encoder_get_audio_packet_sink(void * data, int stream)
  {
  bg_ogg_encoder_t * e = data;
  return e->audio_streams[stream].psink;
  }

gavl_packet_sink_t *
bg_ogg_encoder_get_video_packet_sink(void * data, int stream)
  {
  bg_ogg_encoder_t * e = data;
  return e->video_streams[stream].psink;
  }


int bg_ogg_encoder_write_audio_frame(void * data,
                                     gavl_audio_frame_t*f,int stream)
  {
  bg_ogg_encoder_t * e = data;
  bg_ogg_stream_t * s = &e->audio_streams[stream];
  return gavl_audio_sink_put_frame(s->asink, f) == GAVL_SINK_OK;
  }

int bg_ogg_encoder_write_video_frame(void * data,
                                     gavl_video_frame_t*f,int stream)
  {
  bg_ogg_encoder_t * e = data;
  bg_ogg_stream_t * s = &e->video_streams[stream];
  return gavl_video_sink_put_frame(s->vsink, f) == GAVL_SINK_OK;
  }

int bg_ogg_encoder_write_audio_packet(void * data,
                                      gavl_packet_t*p,int stream)
  {
  bg_ogg_encoder_t * e = data;
  bg_ogg_stream_t * s = &e->audio_streams[stream];
  return gavl_packet_sink_put_packet(s->psink, p) == GAVL_SINK_OK;
  }

int bg_ogg_encoder_write_video_packet(void * data,
                                      gavl_packet_t*p,int stream)
  {
  bg_ogg_encoder_t * e = data;
  bg_ogg_stream_t * s = &e->video_streams[stream];
  return gavl_packet_sink_put_packet(s->psink, p) == GAVL_SINK_OK;
  }

int bg_ogg_encoder_close(void * data, int do_delete)
  {
  int ret = 1;
  int i;
  bg_ogg_encoder_t * e = data;

  if(!e->write_callback_data)
    return 1;
  
  for(i = 0; i < e->num_audio_streams; i++)
    {
    if(!e->audio_streams[i].codec->close(e->audio_streams[i].codec_priv))
      {
      ret = 0;
      break;
      }

    ogg_stream_clear(&e->audio_streams[i].os);
        
    if(e->audio_streams[i].asink)
      {
      gavl_audio_sink_destroy(e->audio_streams[i].asink);
      e->audio_streams[i].asink = NULL;
      }
    if(e->audio_streams[i].psink)
      {
      gavl_packet_sink_destroy(e->audio_streams[i].psink);
      e->audio_streams[i].psink = NULL;
      }
    }
  for(i = 0; i < e->num_video_streams; i++)
    {
    if(!e->video_streams[i].codec->close(e->video_streams[i].codec_priv))
      {
      ret = 0;
      break;
      }
    
    ogg_stream_clear(&e->video_streams[i].os);

    if(e->video_streams[i].vsink)
      {
      gavl_video_sink_destroy(e->video_streams[i].vsink);
      e->video_streams[i].vsink = NULL;
      }
    if(e->video_streams[i].psink)
      {
      gavl_packet_sink_destroy(e->video_streams[i].psink);
      e->video_streams[i].psink = NULL;
      }
    }
  
  e->close_callback(e->write_callback_data);
  e->write_callback_data = NULL;
  if(do_delete && e->filename)
    remove(e->filename);
  return ret;
  }

static const bg_parameter_info_t audio_parameters[] =
  {
    {
      .name =      "codec",
      .long_name = TRS("Codec"),
      .type =      BG_PARAMETER_MULTI_MENU,
      .val_default = { .val_str = "vorbis" },
    },
    { /* End of parameters */ }
  };

bg_parameter_info_t *
bg_ogg_encoder_get_audio_parameters(bg_ogg_encoder_t * e,
                                    bg_ogg_codec_t const * const * audio_codecs)
  {
  int num_audio_codecs;
  int i;
  if(!e->audio_parameters)
    {
    num_audio_codecs = 0;
    while(audio_codecs[num_audio_codecs])
      num_audio_codecs++;
    
    e->audio_parameters = bg_parameter_info_copy_array(audio_parameters);
    e->audio_parameters[0].multi_names_nc =
      calloc(num_audio_codecs+1, sizeof(*e->audio_parameters[0].multi_names));
    e->audio_parameters[0].multi_labels_nc =
      calloc(num_audio_codecs+1, sizeof(*e->audio_parameters[0].multi_labels));
    e->audio_parameters[0].multi_parameters_nc =
      calloc(num_audio_codecs+1, sizeof(*e->audio_parameters[0].multi_parameters));
    for(i = 0; i < num_audio_codecs; i++)
      {
      e->audio_parameters[0].multi_names_nc[i]  =
        bg_strdup(NULL, audio_codecs[i]->name);
      e->audio_parameters[0].multi_labels_nc[i] =
        bg_strdup(NULL, audio_codecs[i]->long_name);
      
      if(audio_codecs[i]->get_parameters)
        e->audio_parameters[0].multi_parameters_nc[i] =
          bg_parameter_info_copy_array(audio_codecs[i]->get_parameters());
      }
    bg_parameter_info_set_const_ptrs(&e->audio_parameters[0]);
    }
  return e->audio_parameters;
  }

/* Comment building stuff */

#define writeint(buf, base, val) \
  buf[base+3]=((val)>>24)&0xff;  \
  buf[base+2]=((val)>>16)&0xff;  \
  buf[base+1]=((val)>>8)&0xff;   \
  buf[base]=(val)&0xff;

static void append_tag(uint8_t ** buf_p, int * len_p, const char * tag,
                       const char * val, int * num_entries)
  {
  int tag_len, val_len;
  int len = *len_p;
  uint8_t * buf = *buf_p;

  if(tag)
    tag_len = strlen(tag);
  else
    tag_len = 0;
  
  val_len = strlen(val);

  buf = realloc(buf, len + 4 + tag_len + val_len);

  writeint(buf, len, tag_len + val_len); len += 4;

  if(tag_len)
    {
    memcpy(buf + len, tag, tag_len);
    len += tag_len;
    }

  memcpy(buf + len, val, val_len);
  len += val_len;

  *buf_p = buf;
  *len_p = len;
  (*num_entries)++;
  }

void
bg_ogg_create_comment_packet(uint8_t * prefix,
                             int prefix_len,
                             const char * vendor_string,
                             const gavl_metadata_t * metadata,
                             ogg_packet * op)
  {
  uint8_t * buf;
  int len;
  int pos = 0;
  int num_entries_pos;
  int num_entries = 0;
  const char * val;
  int vendor_len = strlen(vendor_string);

  /* Write mandatory fields */
  len = prefix_len + 4 + vendor_len + 4;
  buf = malloc(len);

  if(prefix_len)
    {
    memcpy(buf, prefix, prefix_len);
    pos += prefix_len;
    }

  writeint(buf, pos, vendor_len); pos += 4;
  memcpy(buf + pos, vendor_string, vendor_len); pos += vendor_len;
  
  num_entries_pos = pos;

  writeint(buf, num_entries_pos, num_entries); pos += 4;

  /* Write tags */
  
  if((val = gavl_metadata_get(metadata, GAVL_META_ARTIST)))
    append_tag(&buf, &len, "ARTIST=", val, &num_entries);
  
  if((val = gavl_metadata_get(metadata, GAVL_META_TITLE)))
    append_tag(&buf, &len, "TITLE=", val, &num_entries);
  
  if((val = gavl_metadata_get(metadata, GAVL_META_ALBUM)))
    append_tag(&buf, &len, "ALBUM=", val, &num_entries);

  if((val = gavl_metadata_get(metadata, GAVL_META_ALBUMARTIST)))
    {
    append_tag(&buf, &len, "ALBUMARTIST=", val, &num_entries);
    append_tag(&buf, &len, "ALBUM ARTIST=", val, &num_entries);
    }
  
  if((val = gavl_metadata_get(metadata, GAVL_META_GENRE)))
    append_tag(&buf, &len, "GENRE=", val, &num_entries);

  if((val = gavl_metadata_get(metadata, GAVL_META_DATE)))
    append_tag(&buf, &len, "DATE=", val, &num_entries);
  else if((val = gavl_metadata_get(metadata, GAVL_META_YEAR)))
    append_tag(&buf, &len, "DATE=", val, &num_entries);

  if((val = gavl_metadata_get(metadata, GAVL_META_COPYRIGHT)))
    append_tag(&buf, &len, "COPYRIGHT=", val, &num_entries);

  if((val = gavl_metadata_get(metadata, GAVL_META_TRACKNUMBER)))
    append_tag(&buf, &len, "TRACKNUMBER=", val, &num_entries);
  
  if((val = gavl_metadata_get(metadata, GAVL_META_COMMENT)))
    append_tag(&buf, &len, NULL, val, &num_entries);
  
  writeint(buf, num_entries_pos, num_entries);
  
  op->packet = buf;
  op->bytes = len;
  }

void bg_ogg_free_comment_packet(ogg_packet * op)
  {
  free(op->packet);
  }

void bg_ogg_set_vorbis_channel_setup(gavl_audio_format_t * format)
  {
  if(format->channel_locations[0] == GAVL_CHID_AUX)
    return;
  
  switch(format->num_channels)
    {
    case 1:
      format->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      break;
    case 2:
      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      break;
    case 3:
      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_CENTER;
      format->channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      break;
    case 4:
      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[2] = GAVL_CHID_REAR_LEFT;
      format->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
      break;
    case 5:
      format->channel_locations[0] =  GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] =  GAVL_CHID_FRONT_CENTER;
      format->channel_locations[2] =  GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[3] =  GAVL_CHID_REAR_LEFT;
      format->channel_locations[4] =  GAVL_CHID_REAR_RIGHT;
      break;
    case 6:
      format->channel_locations[0] =  GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] =  GAVL_CHID_FRONT_CENTER;
      format->channel_locations[2] =  GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[3] =  GAVL_CHID_REAR_LEFT;
      format->channel_locations[4] =  GAVL_CHID_REAR_RIGHT;
      format->channel_locations[5] =  GAVL_CHID_LFE;
      break;
    case 7:
      format->channel_locations[0] =  GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] =  GAVL_CHID_FRONT_CENTER;
      format->channel_locations[2] =  GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[3] =  GAVL_CHID_SIDE_LEFT;
      format->channel_locations[4] =  GAVL_CHID_SIDE_RIGHT;
      format->channel_locations[5] =  GAVL_CHID_REAR_CENTER;
      format->channel_locations[6] =  GAVL_CHID_LFE;
      break;
    case 8:
      format->channel_locations[0] =  GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] =  GAVL_CHID_FRONT_CENTER;
      format->channel_locations[2] =  GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[3] =  GAVL_CHID_SIDE_LEFT;
      format->channel_locations[4] =  GAVL_CHID_SIDE_RIGHT;
      format->channel_locations[5] =  GAVL_CHID_REAR_LEFT;
      format->channel_locations[6] =  GAVL_CHID_REAR_RIGHT;
      format->channel_locations[7] =  GAVL_CHID_LFE;
      break;
    }
  }
