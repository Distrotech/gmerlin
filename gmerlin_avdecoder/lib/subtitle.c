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
#include <parser.h>

#define LOG_DOMAIN "subtitle"

int bgav_num_subtitle_streams(bgav_t * bgav, int track)
  {
  return bgav->tt->tracks[track].num_text_streams +
    bgav->tt->tracks[track].num_overlay_streams;
  }

int bgav_num_text_streams(bgav_t * bgav, int track)
  {
  return bgav->tt->tracks[track].num_text_streams;
  }

int bgav_num_overlay_streams(bgav_t * bgav, int track)
  {
  return bgav->tt->tracks[track].num_overlay_streams;
  }

int bgav_set_subtitle_stream(bgav_t * b, int stream, bgav_stream_action_t action)
  {
  bgav_stream_t * s = bgav_track_get_subtitle_stream(b->tt->cur, stream);
  if(!s)
    return 0;
  s->action = action;
  return 1;
  }

int bgav_set_text_stream(bgav_t * b, int stream, bgav_stream_action_t action)
  {
  b->tt->cur->text_streams[stream].action = action;
  return 1;
  }

int bgav_set_overlay_stream(bgav_t * b, int stream, bgav_stream_action_t action)
  {
  b->tt->cur->overlay_streams[stream].action = action;
  return 1;
  }

const gavl_video_format_t * bgav_get_subtitle_format(bgav_t * b, int stream)
  {
  bgav_stream_t * s = bgav_track_get_subtitle_stream(b->tt->cur, stream);
  return &s->data.subtitle.format;
  }

int bgav_subtitle_is_text(bgav_t * b, int stream)
  {
  bgav_stream_t * s = bgav_track_get_subtitle_stream(b->tt->cur, stream);
  if(s->type == BGAV_STREAM_SUBTITLE_TEXT)
    return 1;
  else
    return 0;
  }

const char * bgav_get_subtitle_language(bgav_t * b, int stream)
  {
  bgav_stream_t * s = bgav_track_get_subtitle_stream(b->tt->cur, stream);
  return gavl_metadata_get(&s->m, GAVL_META_LANGUAGE);
  }

/* LEGACY */
int bgav_read_subtitle_overlay(bgav_t * b, gavl_overlay_t * ovl, int stream)
  {
  bgav_stream_t * s = bgav_track_get_subtitle_stream(b->tt->cur, stream);
  
  if(bgav_has_subtitle(b, stream))
    {
    if(s->flags & STREAM_EOF_C)
      return 0;
    }
  else
    return 0;
  return s->data.subtitle.decoder->decoder->decode(s, ovl);
  }

/* LEGACY */
int bgav_read_subtitle_text(bgav_t * b, char ** ret, int *ret_alloc,
                            int64_t * start_time, int64_t * duration,
                            int stream)
  {
  bgav_stream_t * s = bgav_track_get_subtitle_stream(b->tt->cur, stream);
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
  bgav_stream_t * s = bgav_track_get_subtitle_stream(b->tt->cur, stream);
  int force;
  
  if(s->flags & STREAM_SUBREADER)
    {
    return 1;
    }
  else
    {
    if(b->tt->cur->flags & TRACK_SAMPLE_ACCURATE)
      force = 1;
    else
      force = 0;
    
    if(bgav_stream_peek_packet_read(s, NULL, force) == GAVL_SOURCE_AGAIN)
      return 0;
    else
      return 1; 
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
  else
    {
    bgav_dprintf( "  Format:\n");
    gavl_video_format_dump(&s->data.subtitle.format);
    }
  }

static gavl_source_status_t read_video_copy(void * sp,
                                            gavl_video_frame_t ** frame)
  {
  gavl_source_status_t st;
  bgav_stream_t * s = sp;
  
  if(frame)
    {
    if((st = s->data.subtitle.decoder->decoder->decode(sp, *frame)) != GAVL_SOURCE_OK)
      return st;
    s->out_time = (*frame)->timestamp + (*frame)->duration;
    }
  else
    {
    if((st = s->data.subtitle.decoder->decoder->decode(sp, NULL)) != GAVL_SOURCE_OK)
      return st;
    }
#ifdef DUMP_TIMESTAMPS
  bgav_dprintf("Overlay timestamp: %"PRId64"\n", s->data.video.frame->timestamp);
#endif
  // s->flags &= ~STREAM_HAVE_FRAME;
  return GAVL_SOURCE_OK;
  }

int bgav_text_start(bgav_stream_t * s)
  {
  s->flags &= ~(STREAM_EOF_C|STREAM_EOF_D);

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
  return 1;
  }

int bgav_overlay_start(bgav_stream_t * s)
  {
  bgav_video_decoder_t * dec;
  bgav_video_decoder_context_t * ctx;
  
  s->flags &= ~(STREAM_EOF_C|STREAM_EOF_D);
  
  if(s->data.subtitle.subreader)
    {
    if(!bgav_subtitle_reader_start(s))
      return 0;
    else
      return 1;
    }

  if((s->fourcc == BGAV_MK_FOURCC('m', 'p', '4', 's')) ||
     (s->fourcc == BGAV_MK_FOURCC('D', 'V', 'D', 'S')))
    s->flags |= STREAM_PARSE_FULL;
    
  if((s->flags & (STREAM_PARSE_FULL|STREAM_PARSE_FRAME)) &&
     !s->data.subtitle.parser)
    {
    s->data.subtitle.parser = bgav_video_parser_create(s);
    if(!s->data.subtitle.parser)
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "No subtitle parser found for fourcc %c%c%c%c (0x%08x)",
               (s->fourcc & 0xFF000000) >> 24,
               (s->fourcc & 0x00FF0000) >> 16,
               (s->fourcc & 0x0000FF00) >> 8,
               (s->fourcc & 0x000000FF),
               s->fourcc);
      return 0;
      }
    s->index_mode = INDEX_MODE_SIMPLE;
    }

  if(s->action == BGAV_STREAM_DECODE)
    {
    dec = bgav_find_video_decoder(s->fourcc);
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

    s->data.subtitle.format.timescale = s->timescale;
    s->data.subtitle.vsrc =
      gavl_video_source_create(read_video_copy,
                               s, 0,
                               &s->data.subtitle.format);
    }
  else if(s->action == BGAV_STREAM_READRAW)
    {
    s->psrc =
      gavl_packet_source_create(bgav_stream_read_packet_func, // get_packet,
                                s,
                                GAVL_SOURCE_SRC_ALLOC,
                                &s->ci,
                                NULL, &s->data.subtitle.format);
    }
  
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
  if(s->data.subtitle.vsrc)
    {
    gavl_video_source_destroy(s->data.subtitle.vsrc);
    s->data.subtitle.vsrc = NULL;
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
  
  if(s->data.subtitle.vsrc)
    gavl_video_source_reset(s->data.subtitle.vsrc);
  }

int bgav_subtitle_skipto(bgav_stream_t * s, int64_t * time, int scale)
  {
  return 0;
  }

const char * bgav_get_subtitle_info(bgav_t * b, int stream)
  {
  bgav_stream_t * s = bgav_track_get_subtitle_stream(b->tt->cur, stream);
  return gavl_metadata_get(&s->m, GAVL_META_LABEL);
  }

const bgav_metadata_t *
bgav_get_subtitle_metadata(bgav_t * b, int stream)
  {
  bgav_stream_t * s = bgav_track_get_subtitle_stream(b->tt->cur, stream);
  return &s->m;
  }

const bgav_metadata_t *
bgav_get_text_metadata(bgav_t * b, int stream)
  {
  return &b->tt->cur->text_streams[stream].m;
  }

const bgav_metadata_t *
bgav_get_overlay_metadata(bgav_t * b, int stream)
  {
  return &b->tt->cur->overlay_streams[stream].m;
  }

gavl_packet_source_t *
bgav_get_text_packet_source(bgav_t * b, int stream)
  {
  return b->tt->cur->text_streams[stream].psrc;
  }

const gavl_video_format_t * bgav_get_overlay_format(bgav_t * bgav, int stream)
  {
  return &bgav->tt->cur->overlay_streams[stream].data.subtitle.format;
  }


int bgav_get_text_timescale(bgav_t * bgav, int stream)
  {
  return bgav->tt->cur->text_streams[stream].timescale;
  }

gavl_packet_source_t *
bgav_get_overlay_packet_source(bgav_t * b, int stream)
  {
  return b->tt->cur->overlay_streams[stream].psrc;
  }

gavl_video_source_t *
bgav_get_overlay_source(bgav_t * b, int stream)
  {
  return b->tt->cur->overlay_streams[stream].data.subtitle.vsrc;
  }
