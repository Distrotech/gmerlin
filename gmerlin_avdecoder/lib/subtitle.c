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
  
  if(s->flags & STREAM_SUBREADER)
    {
    return bgav_subtitle_reader_read_overlay(s, ovl);
    }
  return s->data.subtitle.decoder->decoder->decode(s, ovl);
  }

int bgav_read_subtitle_text(bgav_t * b, char ** ret, int *ret_alloc,
                            int64_t * start_time, int64_t * duration,
                            int stream)
  {
  bgav_stream_t * s = &b->tt->cur->subtitle_streams[stream];
  gavl_packet_t p;
  gavl_packet_t * pp;
  
  if(bgav_has_subtitle(b, stream))
    {
    if(s->flags & STREAM_EOF_C)
      return 0;
    }
  else
    return 0;

  gavl_packet_init(&p);
  
  p.data = (uint8_t*)(*ret);
  p.data_alloc = *ret_alloc;

  pp = &p;
  
  if(gavl_packet_source_read_packet(s->psrc, &pp) != GAVL_SOURCE_OK)
    return 0;

  *ret = (char*)p.data;
  *ret_alloc = p.data_alloc;
  
  *start_time = p.pts;
  *duration   = p.duration;
  
  return 1;
  }

int bgav_has_subtitle(bgav_t * b, int stream)
  {
  bgav_stream_t * s = &b->tt->cur->subtitle_streams[stream];
  int force;
  
  if(s->flags & STREAM_SUBREADER)
    {
    return 1;
    }
  else
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
  }


void bgav_subtitle_dump(bgav_stream_t * s)
  {
  if(s->type == BGAV_STREAM_SUBTITLE_TEXT)
    {
    bgav_dprintf( "  Character set:     %s\n",
                  (s->data.subtitle.charset ? s->data.subtitle.charset :
                   BGAV_UTF8));
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
    
    s->data.subtitle.cnv =
      bgav_subtitle_converter_create(s);

    s->psrc =
      gavl_packet_source_create(bgav_stream_read_packet_func, // get_packet,
                                s,
                                GAVL_SOURCE_SRC_ALLOC,
                                NULL, NULL, NULL);
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
    bgav_subtitle_converter_destroy(s->data.subtitle.cnv);
    s->data.subtitle.cnv = NULL;
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

  if(s->data.subtitle.cnv)
    bgav_subtitle_converter_reset(s->data.subtitle.cnv);
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
