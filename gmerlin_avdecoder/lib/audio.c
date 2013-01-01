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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <avdec_private.h>
#include <parser.h>

#define LOG_DOMAIN "audio"

int bgav_num_audio_streams(bgav_t * bgav, int track)
  {
  return bgav->tt->tracks[track].num_audio_streams;
  }

const gavl_audio_format_t * bgav_get_audio_format(bgav_t *  bgav, int stream)
  {
  return &bgav->tt->cur->audio_streams[stream].data.audio.format;
  }

int bgav_set_audio_stream(bgav_t * b, int stream, bgav_stream_action_t action)
  {
  if((stream >= b->tt->cur->num_audio_streams) || (stream < 0))
    return 0;
  b->tt->cur->audio_streams[stream].action = action;
  return 1;
  }

static gavl_source_status_t get_frame(void * sp, gavl_audio_frame_t ** frame)
  {
  bgav_stream_t * s = sp;
  
  if(!(s->flags & STREAM_HAVE_FRAME) &&
     !s->data.audio.decoder->decode_frame(s))
    {
    s->flags |= STREAM_EOF_C;
    return GAVL_SOURCE_EOF;
    }
  s->flags &= ~STREAM_HAVE_FRAME; 
  s->data.audio.frame->timestamp = s->out_time;
  s->out_time += s->data.audio.frame->valid_samples;
  *frame = s->data.audio.frame;
  return GAVL_SOURCE_OK;
  }

int bgav_audio_start(bgav_stream_t * s)
  {
  bgav_audio_decoder_t * dec;

  if((s->flags & (STREAM_PARSE_FULL|STREAM_PARSE_FRAME)) &&
     !s->data.audio.parser)
    {
    s->data.audio.parser = bgav_audio_parser_create(s);
    if(!s->data.audio.parser)
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "No audio parser found for fourcc %c%c%c%c (0x%08x)",
               (s->fourcc & 0xFF000000) >> 24,
               (s->fourcc & 0x00FF0000) >> 16,
               (s->fourcc & 0x0000FF00) >> 8,
               (s->fourcc & 0x000000FF),
               s->fourcc);
      return 0;
      }
    
    /* Start the parser */
    
    if(bgav_stream_peek_packet_read(s, NULL, 1) != GAVL_SOURCE_OK)
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "EOF while initializing audio parser");
      return 0;
      }
    
    s->index_mode = INDEX_MODE_SIMPLE;
    }

  if(s->flags & STREAM_NEED_START_TIME)
    {
    bgav_packet_t * p = NULL;
    char tmp_string[128];
    
    if(bgav_stream_peek_packet_read(s, &p, 1) != GAVL_SOURCE_OK)
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "EOF while getting start time");
      }
    s->start_time = p->pts;
    sprintf(tmp_string, "%" PRId64, s->start_time);
    bgav_log(s->opt, BGAV_LOG_INFO, LOG_DOMAIN, "Got initial audio timestamp: %s",
             tmp_string);
    } /* Else */
  //  else if(s->data.audio.pre_skip > 0)
  //    s->start_time = -s->data.audio.pre_skip;

  s->out_time = s->start_time;
  
  if(!s->timescale && s->data.audio.format.samplerate)
    s->timescale = s->data.audio.format.samplerate;
  
  if(s->action == BGAV_STREAM_DECODE)
    {
    dec = bgav_find_audio_decoder(s->fourcc);
    if(!dec)
      {
      if(!(s->fourcc & 0xffff0000))
        bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "No audio decoder found for WAV ID 0x%04x", s->fourcc);
      else
        bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "No audio decoder found for fourcc %c%c%c%c (0x%08x)",
                 (s->fourcc & 0xFF000000) >> 24,
                 (s->fourcc & 0x00FF0000) >> 16,
                 (s->fourcc & 0x0000FF00) >> 8,
                 (s->fourcc & 0x000000FF),
                 s->fourcc);
      return 0;
      }
    s->data.audio.decoder = dec;
    s->data.audio.frame = gavl_audio_frame_create(NULL);
    
    if(!dec->init(s))
      return 0;

    /* Some decoders get a first frame during decoding */
    s->data.audio.frame_samples = s->data.audio.frame->valid_samples;
    
    if(!s->timescale)
      s->timescale = s->data.audio.format.samplerate;

    //    s->out_time -= s->data.audio.pre_skip;
    }

  if(s->data.audio.bits_per_sample)
    gavl_metadata_set_int(&s->m, GAVL_META_AUDIO_BITS ,
                          s->data.audio.bits_per_sample);


  if(s->container_bitrate == GAVL_BITRATE_VBR)
    gavl_metadata_set(&s->m, GAVL_META_BITRATE,
                      "VBR");
  else if(s->codec_bitrate)
    gavl_metadata_set_int(&s->m, GAVL_META_BITRATE,
                          s->codec_bitrate);
  else if(s->container_bitrate)
    gavl_metadata_set_int(&s->m, GAVL_META_BITRATE,
                          s->container_bitrate);

  if(s->action == BGAV_STREAM_DECODE)
    s->data.audio.source =
      gavl_audio_source_create(get_frame, s,
                               GAVL_SOURCE_SRC_ALLOC | s->src_flags,
                               &s->data.audio.format);
#if 1
  if(s->action == BGAV_STREAM_READRAW)
    s->psrc =
      gavl_packet_source_create_audio(bgav_stream_read_packet_func, // get_packet,
                                      s, GAVL_SOURCE_SRC_ALLOC,
                                      &s->ci, &s->data.audio.format);
#endif
  
  //  if(s->data.audio.pre_skip && s->data.audio.source)
  //    gavl_audio_source_skip_src(s->data.audio.source, s->data.audio.pre_skip);
  
  return 1;
  }

void bgav_audio_stop(bgav_stream_t * s)
  {
  if(s->data.audio.decoder)
    {
    s->data.audio.decoder->close(s);
    s->data.audio.decoder = NULL;
    }
  if(s->data.audio.parser)
    {
    bgav_audio_parser_destroy(s->data.audio.parser);
    s->data.audio.parser = NULL;
    }
  if(s->data.audio.frame)
    {
    gavl_audio_frame_null(s->data.audio.frame);
    gavl_audio_frame_destroy(s->data.audio.frame);
    s->data.audio.frame = NULL;
    }
  if(s->data.audio.source)
    {
    gavl_audio_source_destroy(s->data.audio.source);
    s->data.audio.source = NULL;
    }

  }

const char * bgav_get_audio_description(bgav_t * b, int s)
  {
  return gavl_metadata_get(&b->tt->cur->audio_streams[s].m,
                           GAVL_META_FORMAT);
  }

const char * bgav_get_audio_info(bgav_t * b, int s)
  {
  return gavl_metadata_get(&b->tt->cur->audio_streams[s].m,
                           GAVL_META_LABEL);
  }

const bgav_metadata_t *
bgav_get_audio_metadata(bgav_t * b, int s)
  {
  return &b->tt->cur->audio_streams[s].m;
  }


const char * bgav_get_audio_language(bgav_t * b, int s)
  {
  return gavl_metadata_get(&b->tt->cur->audio_streams[s].m,
                           GAVL_META_LANGUAGE);
  }

int bgav_get_audio_bitrate(bgav_t * bgav, int stream)
  {
  bgav_stream_t * s = bgav->tt->cur->audio_streams + stream;
  if(s->codec_bitrate)
    return s->codec_bitrate;
  else if(s->container_bitrate)
    return s->container_bitrate;
  else
    return 0;
  }

int bgav_read_audio(bgav_t * b, gavl_audio_frame_t * frame,
                    int stream, int num_samples)
  {
  bgav_stream_t * s = &b->tt->cur->audio_streams[stream];
  return gavl_audio_source_read_samples(s->data.audio.source,
                                        frame, num_samples);
  }

void bgav_audio_dump(bgav_stream_t * s)
  {
  bgav_dprintf("  Bits per sample:   %d\n", s->data.audio.bits_per_sample);
  bgav_dprintf("  Block align:       %d\n", s->data.audio.block_align);
  bgav_dprintf("  Pre skip:          %d\n", s->data.audio.pre_skip);
  bgav_dprintf("Format:\n");
  gavl_audio_format_dump(&s->data.audio.format);
  }


void bgav_audio_resync(bgav_stream_t * s)
  {
  s->data.audio.frame->valid_samples = 0;

  if(s->out_time == GAVL_TIME_UNDEFINED)
    {
    s->out_time =
      gavl_time_rescale(s->timescale,
                        s->data.audio.format.samplerate,
                        STREAM_GET_SYNC(s));
    }
  
  if(s->data.audio.parser)
    {
    bgav_packet_t * p = NULL;
    bgav_audio_parser_reset(s->data.audio.parser,
                            GAVL_TIME_UNDEFINED, s->out_time);

    if(bgav_stream_peek_packet_read(s, &p, 1) != GAVL_SOURCE_OK)
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "EOF while resyncing");
      }
    s->out_time = p->pts;
    }

  
  if(s->data.audio.decoder &&
     s->data.audio.decoder->resync)
    s->data.audio.decoder->resync(s);
  
  if(s->data.audio.source)
    gavl_audio_source_reset(s->data.audio.source);
  }

int bgav_audio_skipto(bgav_stream_t * s, int64_t * t, int scale)
  {
  int64_t num_samples;
  // int samples_skipped = 0;  
  int64_t skip_time;
  
  skip_time = gavl_time_rescale(scale,
                                s->data.audio.format.samplerate,
                                *t);
  
  num_samples = skip_time - s->out_time;
  
  if(num_samples < 0)
    {
    char str1[128];
    char str2[128];
    char str3[128];
    //    sprintf(str1, "%"PRId64, s->out_time);
    //    sprintf(str2, "%"PRId64, skip_time);
    //    sprintf(str3, "%"PRId64, num_samples);
    sprintf(str1, "%"PRId64, s->out_time);
    sprintf(str2, "%"PRId64, skip_time);
    sprintf(str3, "%"PRId64, num_samples);
    
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Cannot skip backwards: Stream time: %s skip time: %s difference: %s",
             str1, str2, str3);
    return 1;
    }
  
  gavl_audio_source_skip_src(s->data.audio.source, num_samples);
  return 1;
  }

static uint32_t alaw_fourccs[] =
  {
  BGAV_MK_FOURCC('a', 'l', 'a', 'w'),
  BGAV_MK_FOURCC('A', 'L', 'A', 'W'),
  BGAV_WAVID_2_FOURCC(0x06),
  0x00
  };

static uint32_t ulaw_fourccs[] =
  {
  BGAV_MK_FOURCC('u', 'l', 'a', 'w'),
  BGAV_MK_FOURCC('U', 'L', 'A', 'W'),
  BGAV_WAVID_2_FOURCC(0x07),
  0x00
  };

static uint32_t mp2_fourccs[] =
  {
  BGAV_MK_FOURCC('.', 'm', 'p', '2'),
  BGAV_WAVID_2_FOURCC(0x0050),
  0x00
  };

static uint32_t mp3_fourccs[] =
  {
  BGAV_MK_FOURCC('.', 'm', 'p', '3'),
  BGAV_WAVID_2_FOURCC(0x0055),
  0x00
  };

static uint32_t ac3_fourccs[] =
  {
    BGAV_WAVID_2_FOURCC(0x2000),
    BGAV_MK_FOURCC('.', 'a', 'c', '3'),
    BGAV_MK_FOURCC('d', 'n', 'e', 't'), 
    0x00
  };

static uint32_t vorbis_fourccs[] =
  {
    BGAV_MK_FOURCC('V','B','I','S'),
    0x00
  };

static uint32_t aac_fourccs[] =
  {
    BGAV_MK_FOURCC('m','p','4','a'),
    0x00
  };

static uint32_t flac_fourccs[] =
  {
    BGAV_MK_FOURCC('F','L','A','C'),
    0x00
  };

static uint32_t opus_fourccs[] =
  {
    BGAV_MK_FOURCC('O','P','U','S'),
    0x00
  };

static uint32_t speex_fourccs[] =
  {
    BGAV_MK_FOURCC('S','P','E','X'),
    0x00
  };


int bgav_get_audio_compression_info(bgav_t * bgav, int stream,
                                    gavl_compression_info_t * ret)
  {
  int need_header = 0;
  int need_bitrate = 1;
  gavl_codec_id_t id = GAVL_CODEC_ID_NONE;
  bgav_stream_t * s = &bgav->tt->cur->audio_streams[stream];

  if(ret)
    memset(ret, 0, sizeof(*ret));
  
  if(s->flags & STREAM_GOT_CI)
    {
    if(ret)
      gavl_compression_info_copy(ret, &s->ci);
    return 1;
    }
  else if(s->flags & STREAM_GOT_NO_CI)
    return 0;

  bgav_track_get_compression(bgav->tt->cur);
  
  if(bgav_check_fourcc(s->fourcc, alaw_fourccs))
    id = GAVL_CODEC_ID_ALAW;
  else if(bgav_check_fourcc(s->fourcc, ulaw_fourccs))
    id = GAVL_CODEC_ID_ULAW;
  else if(bgav_check_fourcc(s->fourcc, ac3_fourccs))
    id = GAVL_CODEC_ID_AC3;
  else if(bgav_check_fourcc(s->fourcc, mp2_fourccs))
    id = GAVL_CODEC_ID_MP2;
  else if(bgav_check_fourcc(s->fourcc, mp3_fourccs))
    id = GAVL_CODEC_ID_MP3;
  else if(bgav_check_fourcc(s->fourcc, aac_fourccs))
    {
    id = GAVL_CODEC_ID_AAC;
    need_header = 1;
    need_bitrate = 0;
    }
  else if(bgav_check_fourcc(s->fourcc, speex_fourccs))
    {
    id = GAVL_CODEC_ID_SPEEX;
    need_header = 1;
    need_bitrate = 0;
    }
  else if(bgav_check_fourcc(s->fourcc, vorbis_fourccs))
    {
    id = GAVL_CODEC_ID_VORBIS;
    need_bitrate = 0;
    need_header = 1;
    }
  else if(bgav_check_fourcc(s->fourcc, flac_fourccs))
    {
    id = GAVL_CODEC_ID_FLAC;
    need_bitrate = 0;
    need_header = 1;
    }
  else if(bgav_check_fourcc(s->fourcc, opus_fourccs))
    {
    id = GAVL_CODEC_ID_OPUS;
    need_bitrate = 0;
    need_header = 1;
    }
  
  if(id == GAVL_CODEC_ID_NONE)
    {
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Cannot output compressed audio stream %d: Unsupported codec",
             stream+1);
    s->flags |= STREAM_GOT_NO_CI;
    return 0;
    }
  if(need_header && !s->ext_size)
    {
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Cannot output compressed audio stream %d: Global header missing",
             stream+1);
    s->flags |= STREAM_GOT_NO_CI;
    return 0;
    }
  if(need_bitrate && !s->container_bitrate && !s->codec_bitrate)
    {
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Cannot output compressed audio stream %d: No bitrate specified",
             stream+1);
    s->flags |= STREAM_GOT_NO_CI;
    return 0;
    }
  s->ci.id = id;

  if(s->flags & STREAM_SBR)
    s->ci.flags |= GAVL_COMPRESSION_SBR;
  
  if(need_header)
    {
    s->ci.global_header = malloc(s->ext_size);
    memcpy(s->ci.global_header, s->ext_data, s->ext_size);
    s->ci.global_header_len = s->ext_size;
    }
  
  if(s->codec_bitrate)
    s->ci.bitrate = s->codec_bitrate;
  else if(s->container_bitrate)
    s->ci.bitrate = s->container_bitrate;

  s->ci.max_packet_size = s->max_packet_size;
  s->ci.pre_skip = s->data.audio.pre_skip;

  if(ret)
    gavl_compression_info_copy(ret, &s->ci);
  
  s->flags |= STREAM_GOT_CI;
  return 1;
  }

int bgav_read_audio_packet(bgav_t * bgav, int stream, gavl_packet_t * p)
  {
  bgav_stream_t * s = &bgav->tt->cur->audio_streams[stream];
  return (gavl_packet_source_read_packet(s->psrc, &p) == GAVL_SOURCE_OK);
  }

gavl_audio_source_t * bgav_get_audio_source(bgav_t * bgav, int stream)
  {
  bgav_stream_t * s = &bgav->tt->cur->audio_streams[stream];
  return s->data.audio.source;
  }

gavl_packet_source_t * bgav_get_audio_packet_source(bgav_t * bgav, int stream)
  {
  bgav_stream_t * s = &bgav->tt->cur->audio_streams[stream];
  return s->psrc;
  }
