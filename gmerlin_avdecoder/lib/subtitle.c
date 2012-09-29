/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <avdec_private.h>

#define LOG_DOMAIN "subtitle"

int bgav_num_subtitle_streams(bgav_t * bgav, int track)
  {
  return bgav->tt->tracks[track].num_subtitle_streams;
  }

int bgav_set_subtitle_stream(bgav_t * b, int stream, bgav_stream_action_t action)
  {
  if((stream >= b->tt->cur->num_subtitle_streams) || (stream < 0))
    return 0;
  b->tt->cur->subtitle_streams[stream].action = action;
  return 1;
  }

const gavl_video_format_t * bgav_get_subtitle_format(bgav_t * bgav, int stream)
  {
  return &bgav->tt->cur->subtitle_streams[stream].data.subtitle.format;
  }

int bgav_subtitle_is_text(bgav_t * bgav, int stream)
  {
  if(bgav->tt->cur->subtitle_streams[stream].type == BGAV_STREAM_SUBTITLE_TEXT)
    return 1;
  else
    return 0;
  }


const char * bgav_get_subtitle_language(bgav_t * b, int s)
  {
  return gavl_metadata_get(&b->tt->cur->subtitle_streams[s].m,
                           GAVL_META_LANGUAGE);
  }

int bgav_read_subtitle_overlay(bgav_t * b, gavl_overlay_t * ovl, int stream)
  {
  bgav_stream_t * s = &b->tt->cur->subtitle_streams[stream];

  if(bgav_has_subtitle(b, stream))
    {
    if(s->flags & STREAM_EOF_C)
      return 0;
    }
  else
    return 0;
  
  if(s->data.subtitle.subreader)
    {
    return bgav_subtitle_reader_read_overlay(s, ovl);
    }
  return s->data.subtitle.decoder->decoder->decode(s, ovl);
  }

static void remove_cr(char * str)
  {
  char * dst;
  int i;
  int len = strlen(str);
  dst = str;
  
  for(i = 0; i <= len; i++)
    {
    if(str[i] != '\r')
      {
      *dst = str[i];
      dst++;
      }
    }
  }

int bgav_read_subtitle_text(bgav_t * b, char ** ret, int *ret_alloc,
                            int64_t * start_time, int64_t * duration,
                            int stream)
  {
  int out_len;
  bgav_packet_t * p = NULL;
  bgav_stream_t * s = &b->tt->cur->subtitle_streams[stream];

  if(bgav_has_subtitle(b, stream))
    {
    if(s->flags & STREAM_EOF_C)
      return 0;
    }
  else
    return 0;
  
  if(s->packet_buffer)
    {
    p = bgav_stream_get_packet_read(s);
    // bgav_packet_get_text_subtitle(p, ret, ret_alloc, start_time, duration);
    }
  else if(s->data.subtitle.subreader)
    {
    p = bgav_subtitle_reader_read_text(s);
    }
  else
    return 0; /* Never get here */
  
  if(!p)
    return 0;

  /* Convert packet to subtitle */
  
  if(s->data.subtitle.cnv)
    {
    if(!bgav_convert_string_realloc(s->data.subtitle.cnv,
                                    (const char *)p->data, p->data_size,
                                    &out_len,
                                    ret, ret_alloc))
      return 0;
    }
  else
    {
    if(*ret_alloc < p->data_size+1)
      {
      *ret_alloc = p->data_size + 128;
      *ret = realloc(*ret, *ret_alloc);
      }
    memcpy(*ret, p->data, p->data_size);
    (*ret)[p->data_size] = '\0';
    }
  
  *start_time = p->pts;
  *duration   = p->duration;
  
  remove_cr(*ret);
    
  if(s->packet_buffer)
    bgav_stream_done_packet_read(s, p);
  
  return 1;
  }

int bgav_has_subtitle(bgav_t * b, int stream)
  {
  bgav_stream_t * s = &b->tt->cur->subtitle_streams[stream];
  int force;
  
  if(s->packet_buffer)
    {
    if(s->type == BGAV_STREAM_SUBTITLE_TEXT)
      {
      if(b->tt->cur->flags & TRACK_SAMPLE_ACCURATE)
        force = 1;
      else
        force = 0;
      
      if(bgav_stream_peek_packet_read(s, force))
        return 1;
      else
        {
        if(s->flags & STREAM_EOF_D)
          {
          s->flags |= STREAM_EOF_C;
          return 1;
          }
        else
          return 0;
        }
      }
    else
      {
      if(s->data.subtitle.decoder->decoder->has_subtitle(s))
        return 1;
      else if(s->flags & STREAM_EOF_C)
        return 1;
      else
        return 0;
      }
    }
  else if(s->data.subtitle.subreader)
    {
    if(bgav_subtitle_reader_has_subtitle(s))
      return 1;
    else
      {
      s->flags |= STREAM_EOF_C;
      return 1;
      }
    }
  else
    return 0;
  }


void bgav_subtitle_dump(bgav_stream_t * s)
  {
  if(s->type == BGAV_STREAM_SUBTITLE_TEXT)
    {
    bgav_dprintf( "  Character set:     %s\n", s->data.subtitle.charset);
    }
  bgav_dprintf( "  Format:\n");
  gavl_video_format_dump(&s->data.subtitle.format);
  }

int bgav_subtitle_start(bgav_stream_t * s)
  {
  bgav_subtitle_overlay_decoder_t * dec;
  bgav_subtitle_overlay_decoder_context_t * ctx;

  s->flags &= ~(STREAM_EOF_C|STREAM_EOF_D);
  
  if(s->type == BGAV_STREAM_SUBTITLE_TEXT)
    {
    if(s->data.subtitle.subreader)
      if(!bgav_subtitle_reader_start(s))
        return 0;
    
    if(s->data.subtitle.charset)
      {
      if(strcmp(s->data.subtitle.charset, "UTF-8"))
        s->data.subtitle.cnv =
          bgav_charset_converter_create(s->opt, s->data.subtitle.charset, "UTF-8");
      }
    else if(strcmp(s->opt->default_subtitle_encoding, "UTF-8"))
      s->data.subtitle.cnv =
        bgav_charset_converter_create(s->opt, s->opt->default_subtitle_encoding,
                                      "UTF-8");
    s->flags |= STREAM_DISCONT;
    }
  else
    {
    if(s->data.subtitle.subreader)
      {
      if(!bgav_subtitle_reader_start(s))
        return 0;
      else
        return 1;
      }
    
    dec = bgav_find_subtitle_overlay_decoder(s);
    if(!dec)
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "No subtitle decoder found for fourcc %c%c%c%c (0x%08x)",
               (s->fourcc & 0xFF000000) >> 24,
               (s->fourcc & 0x00FF0000) >> 16,
               (s->fourcc & 0x0000FF00) >> 8,
               (s->fourcc & 0x000000FF),
               s->fourcc);
      return 0;
      }
    ctx = calloc(1, sizeof(*ctx));
    s->data.subtitle.decoder = ctx;
    s->data.subtitle.decoder->decoder = dec;
    
    if(!dec->init(s))
      return 0;
    }
  s->data.subtitle.format.timescale = s->timescale;
  return 1;
  }

void bgav_subtitle_stop(bgav_stream_t * s)
  {
  if(s->data.subtitle.cnv)
    {
    bgav_charset_converter_destroy(s->data.subtitle.cnv);
    s->data.subtitle.cnv = NULL;
    }
  if(s->data.subtitle.charset)
    {
    free(s->data.subtitle.charset);
    s->data.subtitle.charset = NULL;
    }
  if(s->data.subtitle.subreader)
    {
    bgav_subtitle_reader_stop(s);
    }
  if(s->data.subtitle.decoder)
    {
    s->data.subtitle.decoder->decoder->close(s);
    free(s->data.subtitle.decoder);
    s->data.subtitle.decoder = NULL;
    }
  
  }

void bgav_subtitle_resync(bgav_stream_t * s)
  {
  /* Nothing to do here */
  if(s->type == BGAV_STREAM_SUBTITLE_TEXT)
    return;

  if(s->data.subtitle.decoder &&
     s->data.subtitle.decoder->decoder->resync)
    s->data.subtitle.decoder->decoder->resync(s);
  }

int bgav_subtitle_skipto(bgav_stream_t * s, int64_t * time, int scale)
  {
  return 0;
  }

const char * bgav_get_subtitle_info(bgav_t * b, int s)
  {
  return gavl_metadata_get(&b->tt->cur->subtitle_streams[s].m,
                           GAVL_META_LABEL);
  }

const bgav_metadata_t *
bgav_get_subtitle_metadata(bgav_t * b, int s)
  {
  return &b->tt->cur->subtitle_streams[s].m;
  }
