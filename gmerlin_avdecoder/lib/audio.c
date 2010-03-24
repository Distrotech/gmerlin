/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <avdec_private.h>
#include <parser.h>

#define LOG_DOMAIN "audio"

#define DUMP_INPUT
#define DUMP_OUTPUT

int bgav_num_audio_streams(bgav_t * bgav, int track)
  {
  return bgav->tt->tracks[track].num_audio_streams;
  }

const gavl_audio_format_t * bgav_get_audio_format(bgav_t *  bgav, int stream)
  {
  return &(bgav->tt->cur->audio_streams[stream].data.audio.format);
  }

int bgav_set_audio_stream(bgav_t * b, int stream, bgav_stream_action_t action)
  {
  if((stream >= b->tt->cur->num_audio_streams) || (stream < 0))
    return 0;
  b->tt->cur->audio_streams[stream].action = action;
  return 1;
  }

int bgav_audio_start(bgav_stream_t * s)
  {
  bgav_audio_decoder_t * dec;
  bgav_audio_decoder_context_t * ctx;

  if((s->flags & STREAM_PARSE_FULL) && !s->data.audio.parser)
    {
    int result, done = 0;
    bgav_packet_t * p;
    const gavl_audio_format_t * format;
    bgav_audio_parser_t * parser;
    
    parser = bgav_audio_parser_create(s);
    if(!parser)
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

    if(s->ext_data)
      {
      if(!bgav_audio_parser_set_header(parser, s->ext_data, s->ext_size))
        {
        bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "Audio parser doesn't support out-of-band header");
        }
      }
  
    /* Start the parser and extract the header */

    while(!done)
      {
      result = bgav_audio_parser_parse(parser);
      switch(result)
        {
        case PARSER_NEED_DATA:
          p = bgav_demuxer_get_packet_read(s->demuxer, s);

          if(!p)
            {
            bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                     "EOF while initializing audio parser");
            return 0;
            }
          bgav_audio_parser_add_packet(parser, p);
          bgav_demuxer_done_packet_read(s->demuxer, p);
          break;
        case PARSER_HAVE_FORMAT:
          done = 1;

          format = bgav_audio_parser_get_format(parser);
          gavl_audio_format_copy(&s->data.audio.format, format);
          
          break;
        }
      }
    s->data.audio.parser = parser;
    s->parsed_packet = bgav_packet_create();
    s->parsed_packet->dts = BGAV_TIMESTAMP_UNDEFINED;
    s->index_mode = INDEX_MODE_SIMPLE;
    }

  if(s->flags & STREAM_NEED_START_TIME)
    {
    bgav_packet_t * p;
    char tmp_string[128];
    p = bgav_demuxer_peek_packet_read(s->demuxer, s, 1);
    if(!p)
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "EOF while getting start time");
      }
    s->start_time = p->pts;
    s->out_time = s->start_time;

    sprintf(tmp_string, "%" PRId64, s->out_time);
    bgav_log(s->opt, BGAV_LOG_INFO, LOG_DOMAIN, "Got initial audio timestamp: %s",
             tmp_string);
    s->flags &= ~STREAM_NEED_START_TIME;
    }

  if(!s->timescale && s->data.audio.format.samplerate)
    s->timescale = s->data.audio.format.samplerate;
  
  if(s->action == BGAV_STREAM_DECODE)
    {
    dec = bgav_find_audio_decoder(s);
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
    ctx = calloc(1, sizeof(*ctx));
    s->data.audio.decoder = ctx;
    s->data.audio.decoder->decoder = dec;
    s->data.audio.frame = gavl_audio_frame_create(NULL);
    
    if(!dec->init(s))
      return 0;

    /* Some decoders get a first frame during decoding */
    s->data.audio.frame_samples = s->data.audio.frame->valid_samples;
    
    if(!s->timescale)
      s->timescale = s->data.audio.format.samplerate;
    }
  
  return 1;
  }

void bgav_audio_stop(bgav_stream_t * s)
  {
  if(s->data.audio.decoder)
    {
    s->data.audio.decoder->decoder->close(s);
    free(s->data.audio.decoder);
    s->data.audio.decoder = (bgav_audio_decoder_context_t*)0;
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
  }

const char * bgav_get_audio_description(bgav_t * b, int s)
  {
  return b->tt->cur->audio_streams[s].description;
  }

const char * bgav_get_audio_info(bgav_t * b, int s)
  {
  return b->tt->cur->audio_streams[s].info;
  }


const char * bgav_get_audio_language(bgav_t * b, int s)
  {
  return (b->tt->cur->audio_streams[s].language[0] != '\0') ?
    b->tt->cur->audio_streams[s].language : (char*)0;
  }

static int read_audio(bgav_stream_t * s, gavl_audio_frame_t * frame,
                      int num_samples)
  {
  int samples_decoded = 0;
  int samples_copied;
  if(s->flags & STREAM_EOF_C)
    {
    if(frame)
      frame->valid_samples = 0;
    return 0;
    }
  while(samples_decoded < num_samples)
    {
    if(!s->data.audio.frame->valid_samples)
      {
      if(!s->data.audio.decoder->decoder->decode_frame(s))
        {
        s->flags |= STREAM_EOF_C;
        break;
        }
      s->data.audio.frame_samples = s->data.audio.frame->valid_samples;
      }
    samples_copied =
      gavl_audio_frame_copy(&(s->data.audio.format),
                            frame,
                            s->data.audio.frame,
                            samples_decoded, /* out_pos */
                            s->data.audio.frame_samples -
                            s->data.audio.frame->valid_samples,  /* in_pos */
                            num_samples - samples_decoded, /* out_size, */
                            s->data.audio.frame->valid_samples /* in_size */);

    s->data.audio.frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }

  if(frame)
    {
    frame->timestamp = s->out_time;
    frame->valid_samples = samples_decoded;
    }
  s->out_time += samples_decoded;
  //  fprintf(stderr, "Decode audio: %d / %d, time: %ld\n",
  //          num_samples, samples_decoded, s->out_time);
  return samples_decoded;
  
  }

int bgav_read_audio(bgav_t * b, gavl_audio_frame_t * frame,
                    int stream, int num_samples)
  {
  bgav_stream_t * s = &(b->tt->cur->audio_streams[stream]);

  if(b->eof)
    return 0;

  return read_audio(s, frame, num_samples);
  }

void bgav_audio_dump(bgav_stream_t * s)
  {
  bgav_dprintf("  Bits per sample:   %d\n", s->data.audio.bits_per_sample);
  bgav_dprintf("  Block align:       %d\n", s->data.audio.block_align);
  bgav_dprintf("Format:\n");
  gavl_audio_format_dump(&(s->data.audio.format));
  }


void bgav_audio_resync(bgav_stream_t * s)
  {
  s->data.audio.frame->valid_samples = 0;

  if(s->out_time == BGAV_TIMESTAMP_UNDEFINED)
    {
    s->out_time =
      gavl_time_rescale(s->timescale,
                        s->data.audio.format.samplerate,
                        STREAM_GET_SYNC(s));
    }
  
  if(s->data.audio.parser)
    {
    if(s->parsed_packet)
      s->parsed_packet->valid = 0;
    bgav_audio_parser_reset(s->data.audio.parser, BGAV_TIMESTAMP_UNDEFINED, s->out_time);
    }
  if(s->data.audio.decoder &&
     s->data.audio.decoder->decoder->resync)
    s->data.audio.decoder->decoder->resync(s);
  }

int bgav_audio_skipto(bgav_stream_t * s, int64_t * t, int scale)
  {
  int64_t num_samples;
  int samples_skipped = 0;  
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
    sprintf(str1, "%"PRId64,
            gavl_time_unscale(s->data.audio.format.samplerate, s->out_time));
    sprintf(str2, "%"PRId64,
            gavl_time_unscale(s->data.audio.format.samplerate, skip_time));
    sprintf(str3, "%"PRId64, num_samples);
    
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Cannot skip backwards: Stream time: %s skip time: %s difference: %s",
             str1, str2, str3);
    }
  else
    {
    if(num_samples > 0)
      {
      char str1[128];
      sprintf(str1, "%"PRId64, num_samples);
      
      bgav_log(s->opt, BGAV_LOG_DEBUG, LOG_DOMAIN,
               "Skipping %s samples", str1);
      
      samples_skipped = read_audio(s, (gavl_audio_frame_t*)0, num_samples);
      }
    }
  if(samples_skipped < num_samples)
    {
    return 0;
    }
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

static int check_fourcc(uint32_t fourcc, uint32_t * fourccs)
  {
  int i = 0;
  while(fourccs[i])
    {
    if(fourccs[i] == fourcc)
      return 1;
    else
      i++;
    }
  return 0;
  }

int bgav_get_audio_compression_info(bgav_t * bgav, int stream,
                                    gavl_compression_info_t * info)
  {
  int need_header = 0;
  int need_bitrate = 1;
  gavl_codec_id_t id = GAVL_CODEC_ID_NONE;
  bgav_stream_t * s = &(bgav->tt->cur->audio_streams[stream]);
  
  memset(info, 0, sizeof(*info));
  
  if(check_fourcc(s->fourcc, alaw_fourccs))
    {
    id = GAVL_CODEC_ID_ALAW;
    }
  else if(check_fourcc(s->fourcc, ulaw_fourccs))
    {
    id = GAVL_CODEC_ID_ULAW;
    }
  else if(check_fourcc(s->fourcc, ac3_fourccs))
    {
    id = GAVL_CODEC_ID_AC3;
    }
  else if(check_fourcc(s->fourcc, mp2_fourccs))
    {
    id = GAVL_CODEC_ID_MP2;
    }
  else if(check_fourcc(s->fourcc, mp3_fourccs))
    {
    id = GAVL_CODEC_ID_MP3;
    }
  else if(check_fourcc(s->fourcc, vorbis_fourccs))
    {
    id = GAVL_CODEC_ID_VORBIS;
    need_bitrate = 0;
    need_header = 1;
    }
  if(id == GAVL_CODEC_ID_NONE)
    {
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Cannot output compressed audio stream %d: Unsupported codec",
             stream+1);
    return 0;
    }
  if(need_header && !s->ext_size)
    {
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Cannot output compressed audio stream %d: Global header missing",
             stream+1);
    return 0;
    }
  if(need_bitrate && !s->container_bitrate && !s->codec_bitrate)
    {
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Cannot output compressed audio stream %d: No bitrate specified",
             stream+1);
    return 0;
    }
  info->id = id;

  if(need_header)
    {
    info->global_header = malloc(s->ext_size);
    memcpy(info->global_header, s->ext_data, s->ext_size);
    info->global_header_len = s->ext_size;
    }
  
  if(need_bitrate)
    {
    if(s->container_bitrate)
      info->bitrate = s->container_bitrate;
    else
      info->bitrate = s->codec_bitrate;
    }
  return 1;
  }

int bgav_read_audio_packet(bgav_t * bgav, int stream, gavl_packet_t * p)
  {
  bgav_packet_t * bp;
  bgav_stream_t * s = &(bgav->tt->cur->audio_streams[stream]);

  bp = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!bp)
    return 0;
  
  gavl_packet_alloc(p, bp->data_size);
  memcpy(p->data, bp->data, bp->data_size);
  p->data_len = bp->data_size;
  p->pts = bp->pts;
  p->duration = bp->duration;

  p->flags = GAVL_PACKET_KEYFRAME;

  if(bp->flags & PACKET_FLAG_LAST)
    p->flags |= GAVL_PACKET_LAST;
  
  bgav_demuxer_done_packet_read(s->demuxer, bp);
  
  return 1;
  }
