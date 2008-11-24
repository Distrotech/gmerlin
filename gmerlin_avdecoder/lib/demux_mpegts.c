/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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


/* System includes */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Package includes */

#include <avdec_private.h>
#include <pes_header.h>
#include <a52_header.h>

#include <mpegts_common.h>

#define LOG_DOMAIN "demux_ts"

/* Packet to read at once during scanning */
#define SCAN_PACKETS      1000

/* Packet to read at once during scanning if input is seekable */
#define SCAN_PACKETS_SEEK 32000

#define TS_PACKET_SIZE      188
#define TS_DVHS_PACKET_SIZE 192
#define TS_FEC_PACKET_SIZE  204

#define TS_MAX_PACKET_SIZE  204

/* Maximum number of consecutive error packets */
#define MAX_ERROR_PACKETS   10

typedef struct
  {
  int64_t last_pts;
  int64_t pts_offset;
  int64_t pts_offset_2nd;
  } stream_priv_t;

typedef struct
  {
  int initialized;
  
  uint16_t program_map_pid;
  
  int64_t start_pcr;
  int64_t end_pcr;
  int64_t end_pcr_test; /* For scanning the end timestamps */
  
  uint16_t pcr_pid;
  
  pmt_section_t pmts;
  
  stream_priv_t * streams;
  } program_priv_t;

static void init_streams_priv(program_priv_t * program,
                              bgav_track_t * track)
  {
  int num_streams, index;
  int i;
  num_streams =
    track->num_audio_streams +
    track->num_video_streams;
  
  program->streams = calloc(num_streams, sizeof(*program->streams));
  index = 0;
  for(i = 0; i < track->num_audio_streams; i++)
    {
    program->streams[index].last_pts = BGAV_TIMESTAMP_UNDEFINED;
    track->audio_streams[i].priv = &program->streams[index];
    index++;
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    program->streams[index].last_pts = BGAV_TIMESTAMP_UNDEFINED;
    track->video_streams[i].priv = &program->streams[index];
    index++;
    }
  }

static void reset_streams_priv(bgav_track_t * track)
  {
  int i;
  stream_priv_t * priv;
  for(i = 0; i < track->num_audio_streams; i++)
    {
    priv = (stream_priv_t*)(track->audio_streams[i].priv);
    priv->last_pts = BGAV_TIMESTAMP_UNDEFINED;
    priv->pts_offset = 0;
    }

  for(i = 0; i < track->num_video_streams; i++)
    {
    priv = (stream_priv_t*)(track->video_streams[i].priv);
    priv->last_pts = BGAV_TIMESTAMP_UNDEFINED;
    priv->pts_offset = 0;
    }
  }

#define WRAP_THRESHOLD 9000000 /* 100 seconds */

static void check_pts_wrap(bgav_stream_t * s, int64_t * pts)
  {
  char tmp_string1[128];
  char tmp_string2[128];
  stream_priv_t * priv;
  priv = (stream_priv_t*)(s->priv);

  if(priv->last_pts == BGAV_TIMESTAMP_UNDEFINED)
    {
    priv->last_pts = *pts;
    return;
    }

  /* Detected PTS wrap */
  if(*pts + WRAP_THRESHOLD < priv->last_pts)
    {
    priv->pts_offset_2nd = priv->pts_offset;
    priv->pts_offset += ((int64_t)1) << 33;
    sprintf(tmp_string1, "%" PRId64, *pts);
    sprintf(tmp_string2, "%" PRId64, priv->last_pts);
    
    bgav_log(s->opt, BGAV_LOG_INFO, LOG_DOMAIN,
             "Detected pts wrap (%s < %s)",
             tmp_string1, tmp_string2);
    priv->last_pts = *pts;
    *pts += priv->pts_offset;
    }
  /* Old timestamp (from before PTS wrap, might come due to MPEG frame reordering) */
  else if(*pts - WRAP_THRESHOLD > priv->last_pts)
    *pts += priv->pts_offset_2nd;
  else
    {
    priv->last_pts = *pts;
    *pts += priv->pts_offset;
    }
  }

typedef struct
  {
  int packet_size;
  
  int num_programs;
  program_priv_t * programs;
  
  /* Input needed for pes header parsing */
  bgav_input_context_t * input_mem;
  
  int64_t first_packet_pos;

  int current_program;
  
  transport_packet_t packet;

  uint8_t * buffer;
  uint8_t * ptr;
  uint8_t * packet_start; 
  
  int buffer_size;
  int do_sync;

  int error_counter;
  
  } mpegts_t;

static inline int
parse_transport_packet(const bgav_options_t * opt,
                       mpegts_t * priv)
  {
  if(!bgav_transport_packet_parse(opt, &priv->ptr, &priv->packet))
    return 0;

  if(priv->packet.transport_error)
    {
    priv->error_counter++;
    if(priv->error_counter > MAX_ERROR_PACKETS)
      {
      bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Too many transport errors");
      return 0;
      }
    bgav_log(opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Transport error");
    return 1;
    }

  priv->error_counter = 0;
  return 1;
  }




static inline int next_packet(mpegts_t * priv)
  {
  priv->packet_start += priv->packet_size;
  priv->ptr = priv->packet_start;
  return (priv->ptr - priv->buffer < priv->buffer_size);
  }

static inline int next_packet_scan(bgav_input_context_t * input,
                                   mpegts_t * priv, int can_seek)
  {
  int packets_scanned;
  if(!next_packet(priv))
    {
    if(can_seek)
      {
      packets_scanned =
        (input->position - priv->first_packet_pos) / priv->packet_size;

      if(packets_scanned < SCAN_PACKETS_SEEK)
        {
        priv->buffer_size =
          bgav_input_read_data(input, priv->buffer,
                               priv->packet_size * SCAN_PACKETS);
        if(!priv->buffer_size)
          return 0;
        
        priv->ptr = priv->buffer;
        priv->packet_start = priv->buffer;
        }
      else
        return 0;
      }
    else
      return 0;
    }
  return 1;
  }

#define PROBE_SIZE 32000

static int test_packet_size(uint8_t * probe_data, int size)
  {
  int i;
  for(i = 0; i < PROBE_SIZE; i+= size)
    {
    if(probe_data[i] != 0x47)
      return 0;
    }
  return 1;
  }

static int guess_packet_size(bgav_input_context_t * input)
  {
  uint8_t probe_data[PROBE_SIZE];
  if(bgav_input_get_data(input, probe_data, PROBE_SIZE) < PROBE_SIZE)
    return 0;

  if(test_packet_size(probe_data, TS_FEC_PACKET_SIZE))
    return TS_FEC_PACKET_SIZE;
  if(test_packet_size(probe_data, TS_DVHS_PACKET_SIZE))
    return TS_DVHS_PACKET_SIZE;
  if(test_packet_size(probe_data, TS_PACKET_SIZE))
    return TS_PACKET_SIZE;
  
  return 0;
  }
     
static int probe_mpegts(bgav_input_context_t * input)
  {
  if(guess_packet_size(input))
    return 1;
  return 0;
  }

/* Get program durations */

static int64_t
get_program_timestamp(bgav_demuxer_context_t * ctx, int * program)
  {
  int i, j, program_index;
  mpegts_t * priv;
  bgav_pes_header_t pes_header;

  priv = (mpegts_t*)(ctx->priv);

  /* Get the program to which the packet belongs */

  program_index = -1;
  for(i = 0; i < priv->num_programs; i++)
    {
    if(priv->programs[i].pcr_pid == priv->packet.pid)
      program_index = i;
    }
  
  if(program_index < 0)
    {
    for(i = 0; i < priv->num_programs; i++)
      {
      if(priv->programs[i].pcr_pid > 0)
        continue;
      for(j = 0; j < ctx->tt->tracks[i].num_audio_streams; j++)
        {
        if(ctx->tt->tracks[i].audio_streams[j].stream_id ==
           priv->packet.pid)
          {
          program_index = i;
          break;
          }
        }
      if(program_index < 0)
        {
        for(j = 0; j < ctx->tt->tracks[i].num_video_streams; j++)
          {
          if(ctx->tt->tracks[i].video_streams[j].stream_id ==
             priv->packet.pid)
            {
            program_index = i;
            break;
            }
          }
        }
      if(program_index >= 0)
        break;
      }
    }

  if(program_index < 0)
    return -1;
  
  /* PCR timestamp */
  if(priv->programs[program_index].pcr_pid == priv->packet.pid)
    {
    if(priv->packet.adaption_field.pcr > 0)
      {
      *program = program_index;
      return priv->packet.adaption_field.pcr;
      }
    else
      return -1;
    }
  /* PES timestamp */
  if(!priv->packet.payload_start)
    return -1;
  
  bgav_input_reopen_memory(priv->input_mem, priv->ptr,
                           priv->packet.payload_size);
  
  bgav_pes_header_read(priv->input_mem, &pes_header);
  priv->ptr += priv->input_mem->position;

  if(pes_header.pts > 0)
    {
    *program = program_index;
    return pes_header.pts;
    }
  else
    return -1;
  }

static int get_program_durations(bgav_demuxer_context_t * ctx)
  {
  mpegts_t * priv;
  int keep_going;
  int i;
  int64_t pts;
  int program_index = -1;
  int64_t total_packets;
  int64_t position;
  
  priv = (mpegts_t*)(ctx->priv);
  
  bgav_input_seek(ctx->input, priv->first_packet_pos, SEEK_SET);
  
  for(i = 0; i < priv->num_programs; i++)
    {
    priv->programs[i].start_pcr    = -1;
    priv->programs[i].end_pcr      = -1;
    priv->programs[i].end_pcr_test = -1;
    }
  
  /* Get the start timestamps of all programs */

  keep_going = 1;
  
  priv->buffer_size =
    bgav_input_read_data(ctx->input, priv->buffer,
                         priv->packet_size * SCAN_PACKETS);
  if(!priv->buffer_size)
    return 0;
  
  priv->ptr = priv->buffer;
  priv->packet_start = priv->buffer;
  
  while(keep_going)
    {
    if(!parse_transport_packet(ctx->opt, priv))
      return 0;

    pts = get_program_timestamp(ctx, &program_index);
    if((pts > 0) && (priv->programs[program_index].start_pcr < 0))
      {
      priv->programs[program_index].start_pcr = pts;
      }
    if(!next_packet_scan(ctx->input, priv, 1))
      return 0;
    
    /* Check if we are done */
    keep_going = 0;
    for(i = 0; i < priv->num_programs; i++)
      {
      if(priv->programs[i].initialized && (priv->programs[i].start_pcr == -1))
        {
        keep_going = 1;
        break;
        }
      }
    }

  /* Now, get the end timestamps */

  total_packets =
    (ctx->input->total_bytes - priv->first_packet_pos) / priv->packet_size;
  position =
    priv->first_packet_pos + (total_packets - SCAN_PACKETS) * priv->packet_size;
  
  keep_going = 1;

  while(keep_going)
    {
    bgav_input_seek(ctx->input, position, SEEK_SET);

    priv->buffer_size =
      bgav_input_read_data(ctx->input, priv->buffer,
                           priv->packet_size * SCAN_PACKETS);
    if(!priv->buffer_size)
      return 0;
    
    priv->ptr = priv->buffer;
    priv->packet_start = priv->buffer;

    for(i = 0; i < SCAN_PACKETS; i++)
      {
      if(!parse_transport_packet(ctx->opt, priv))
        return 0;
      
      pts = get_program_timestamp(ctx, &program_index);
      if(pts > 0)
        priv->programs[program_index].end_pcr_test = pts;
      
      next_packet(priv);
      }
    
    /* Check if we are done */
    for(i = 0; i < priv->num_programs; i++)
      {
      if(priv->programs[i].end_pcr < priv->programs[i].end_pcr_test)
        priv->programs[i].end_pcr = priv->programs[i].end_pcr_test;
      }
    
    keep_going = 0;
    for(i = 0; i < priv->num_programs; i++)
      {
      if(priv->programs[i].initialized &&
         (priv->programs[i].end_pcr == -1))
        {
        keep_going = 1;
        break;
        }
      }
    position -= SCAN_PACKETS * priv->packet_size;
    if(position < priv->first_packet_pos)
      return 0; // Should never happen
    }
  
  /* Set the durations */
  for(i = 0; i < priv->num_programs; i++)
    {
    if(priv->programs[i].initialized)
      {
      if(priv->programs[i].end_pcr > priv->programs[i].start_pcr)
        ctx->tt->tracks[i].duration =
          gavl_time_unscale(90000,
                            priv->programs[i].end_pcr -
                            priv->programs[i].start_pcr);
      else
        return 0;
      }
    }
  return 1;
  }

/*
 *  Initialize using a PAT and PMTs
 *  This function expects a PAT table at the beginning
 *  of the parsed buffer.
 */

static int init_psi(bgav_demuxer_context_t * ctx,
                    int input_can_seek)
  {
  int program;
  int keep_going;
  pat_section_t pats;
  int skip;
  mpegts_t * priv;
  int i, j;
  
  priv = (mpegts_t*)(ctx->priv);
  //  bgav_hexdump(data, packet.payload_size, 16);

  /* We are at the beginning of the payload of a PAT packet */
  skip = 1 + priv->ptr[0];
  
  priv->ptr += skip;
  
  if(!bgav_pat_section_read(priv->ptr, priv->packet.payload_size - skip,
                       &pats))
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "PAT section spans multiple packets, please report");
    return 0;
    }
  
  //  bgav_pat_section_dump(&pats);
  
  if(pats.section_number || pats.last_section_number)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "PAT has multiple sections, please report");
    return 0;
    }
  
  /* Count the programs */
  
  for(i = 0; i < pats.num_programs; i++)
    {
    if(pats.programs[i].program_number != 0x0000)
      priv->num_programs++;
    }
    
  /* Allocate programs and track table */
  
  priv->programs = calloc(priv->num_programs, sizeof(*(priv->programs)));
  ctx->tt = bgav_track_table_create(priv->num_programs);
  
  /* Assign program map pids */

  j = 0;

  for(i = 0; i < pats.num_programs; i++)
    {
    if(pats.programs[i].program_number != 0x0000)
      {
      priv->programs[j].program_map_pid =
        pats.programs[i].program_map_pid;
      j++;
      }
    }

  if(!next_packet_scan(ctx->input, priv, input_can_seek))
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Premature EOF");
    return 0;
    }
  
  /* Next, we want to get all programs */

  keep_going = 1;

  while(keep_going)
    {
    if(!parse_transport_packet(ctx->opt, priv))
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Lost sync during initializing");
      return 0;
      }
    
    for(program = 0; program < priv->num_programs; program++)
      {
      /* Check if we got the PMT of a program */
      if(priv->packet.pid == priv->programs[program].program_map_pid)
        break;
      /* Check if the PMT is already parsed and we got a stream ID */
      else if(priv->programs[program].pmts.table_id == 0x02)
        {
        for(i = 0; i < priv->programs[program].pmts.num_streams; i++)
          {
          if(priv->packet.pid == priv->programs[program].pmts.streams[i].pid)
            {
            priv->programs[program].pmts.streams[i].present = 1;
            }
          }
        }
      }
    
    if(program == priv->num_programs)
      {
      if(!next_packet_scan(ctx->input, priv, input_can_seek))
        break;
      continue;
      }

    skip = 1 + priv->ptr[0];
    priv->ptr += skip;

    if(!priv->programs[program].pmts.table_id)
      {
      if(!bgav_pmt_section_read(priv->ptr, priv->packet.payload_size-skip,
                                &priv->programs[program].pmts))
        {
        bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "PMT section spans multiple packets, please report");
        return 0;
        }
      //      bgav_pmt_section_dump(&priv->programs[program].pmts);
      if(priv->programs[program].pmts.section_number ||
         priv->programs[program].pmts.last_section_number)
        {
        bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "PMT has multiple sections, please report");
        return 0;
        }
      //      bgav_pmt_section_dump(&priv->programs[program].pmts);
      }
    if(!next_packet_scan(ctx->input, priv, input_can_seek))
      break;
    }

  for(program = 0; program < priv->num_programs; program++)
    {
    if(bgav_pmt_section_setup_track(&priv->programs[program].pmts,
                                    &ctx->tt->tracks[program],
                                    ctx->opt, -1, -1, -1, (int*)0, (int*)0))
      {
      priv->programs[program].pcr_pid = priv->programs[program].pmts.pcr_pid;
      priv->programs[program].initialized = 1;
      init_streams_priv(&priv->programs[program],
                        &ctx->tt->tracks[program]);
      }
    }
  return 1;
  }

typedef struct
  {
  int pid;
  int pes_id;
  uint8_t * buffer;
  int buffer_size;
  int buffer_alloc;
  int done;
  } test_stream_t;

typedef struct
  {
  int num_streams;
  int last_added;
  test_stream_t * streams;
  } test_streams_t;

static void
test_streams_append_packet(test_streams_t * s, uint8_t * data,
                           int data_size,
                           int pid, bgav_pes_header_t * header)
  {
  int i, index = -1;
  for(i = 0; i < s->num_streams; i++)
    {
    if(s->streams[i].pid == pid)
      {
      index = i;
      break;
      }
    }

  if(index == -1)
    {
    if(!header)
      {
      s->last_added = -1;
      return;
      }
    else
      {
      s->streams =
        realloc(s->streams, (s->num_streams+1)*sizeof(*s->streams));
      memset(s->streams + s->num_streams, 0, sizeof(*s->streams));
      s->last_added = s->num_streams;
      s->num_streams++;
      s->streams[s->last_added].pes_id = header->stream_id;
      s->streams[s->last_added].pid = pid;
      }
    }
  else if(s->streams[index].done)
    {
    s->last_added = -1;
    return;
    }
  else
    s->last_added = index;
    
  if(s->streams[s->last_added].buffer_size + data_size >
     s->streams[s->last_added].buffer_alloc)
    {
    s->streams[s->last_added].buffer_alloc =
      s->streams[s->last_added].buffer_size + data_size + 1024;
    s->streams[s->last_added].buffer =
      realloc(s->streams[s->last_added].buffer,
              s->streams[s->last_added].buffer_alloc);
    }
  memcpy(s->streams[s->last_added].buffer +
         s->streams[s->last_added].buffer_size,
         data, data_size);
  s->streams[s->last_added].buffer_size += data_size;
  }

static int test_a52(test_stream_t * st)
  {
  /* Check for 2 consecutive a52 headers */
  bgav_a52_header_t header;
  uint8_t * ptr, *ptr_end;
  
  ptr     = st->buffer;
  ptr_end = st->buffer + st->buffer_size;

  while(ptr_end - ptr > BGAV_A52_HEADER_BYTES)
    {
    if(bgav_a52_header_read(&header, ptr))
      {
      ptr += header.total_bytes;
      
      if((ptr_end - ptr > BGAV_A52_HEADER_BYTES) &&
         bgav_a52_header_read(&header, ptr))
        {
        return 1;
        }
      }
    else
      ptr++;
    }
  
  return 0;
  }

static bgav_stream_t *
test_streams_detect(test_streams_t * s, bgav_track_t * track,
                    const bgav_options_t * opt)
  {
  bgav_stream_t * ret = (bgav_stream_t*)0;
  test_stream_t * st;
  if(s->last_added < 0)
    return (bgav_stream_t *)0;

  st = s->streams + s->last_added;
  
  if(test_a52(st))
    {
    ret = bgav_track_add_audio_stream(track, opt);
    ret->fourcc = BGAV_MK_FOURCC('.','a','c','3');
    }

  if(ret)
    st->done = 1;
  
  return ret;
  }

static void test_data_free(test_streams_t * s)
  {
  int i;
  for(i = 0; i < s->num_streams; i++)
    {
    if(s->streams[i].buffer)
      free(s->streams[i].buffer);
    }
  free(s->streams);
  }

static int init_raw(bgav_demuxer_context_t * ctx, int input_can_seek)
  {
  mpegts_t * priv;
  bgav_pes_header_t pes_header;
  bgav_stream_t * s;

  test_streams_t ts;

  memset(&ts, 0, sizeof(ts));
  
  priv = (mpegts_t*)(ctx->priv);
  
  /* Allocate programs and track table */
  priv->num_programs = 1;
  priv->programs = calloc(1, sizeof(*(priv->programs)));
  ctx->tt = bgav_track_table_create(priv->num_programs);

  while(1)
    {
    if(!parse_transport_packet(ctx->opt, priv))
      break;

    /* Find the PCR PID */
    if((priv->packet.adaption_field.pcr > 0) &&
       !priv->programs[0].pcr_pid)
      {
      priv->programs[0].pcr_pid = priv->packet.pid;
      }
    
    if(bgav_track_find_stream_all(&ctx->tt->tracks[0],
                                  priv->packet.pid))
      {
      if(!next_packet_scan(ctx->input, priv, input_can_seek))
        break;
      else
        continue;
      }

    if(priv->packet.payload_start)
      {
      bgav_input_reopen_memory(priv->input_mem, priv->ptr,
                               priv->buffer_size -
                               (priv->ptr - priv->buffer));
      
      bgav_pes_header_read(priv->input_mem, &pes_header);
      priv->ptr += priv->input_mem->position;
      
      bgav_pes_header_dump(&pes_header);
      
      /* MPEG-2 Video */
      if((pes_header.stream_id >= 0xe0) && (pes_header.stream_id <= 0xef))
        {
        s = bgav_track_add_video_stream(&ctx->tt->tracks[0], ctx->opt);
        s->fourcc = BGAV_MK_FOURCC('m', 'p', 'g', 'v');
        s->data.video.frametime_mode = BGAV_FRAMETIME_CODEC;
        }
      /* MPEG Audio */
      else if((pes_header.stream_id & 0xe0) == 0xc0)
        {
        s = bgav_track_add_audio_stream(&ctx->tt->tracks[0], ctx->opt);
        s->fourcc = BGAV_MK_FOURCC('.', 'm', 'p', '3');
        }
      else
        {
        test_streams_append_packet(&ts, priv->ptr,
                                   priv->packet.payload_size -
                                   priv->input_mem->position,
                                   priv->packet.pid,
                                   &pes_header);
        s = test_streams_detect(&ts, &ctx->tt->tracks[0], ctx->opt);
        }
      }
    else
      {
      test_streams_append_packet(&ts, priv->ptr,
                                 priv->packet.payload_size,
                                 priv->packet.pid,
                                 (bgav_pes_header_t*)0);
      s = test_streams_detect(&ts, &ctx->tt->tracks[0], ctx->opt);
      }
    
    if(s)
      {
      s->stream_id = priv->packet.pid;
      s->timescale = 90000;
      s->not_aligned = 1;
      }
    if(!next_packet_scan(ctx->input, priv, input_can_seek))
        break;
    }
  test_data_free(&ts);
  priv->programs[0].initialized = 1;
  init_streams_priv(&priv->programs[0],
                    &ctx->tt->tracks[0]);

  return 1;
  }


static int open_mpegts(bgav_demuxer_context_t * ctx)
  {
  int have_pat;
  int input_can_seek;
  int packets_scanned;
  mpegts_t * priv;
  int i;
  
  /* Allocate private data */
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  priv->packet_size = guess_packet_size(ctx->input);

  if(ctx->input->input->seek_byte)
    input_can_seek = 1;
  else
    input_can_seek = 0;
  
  if(!priv->packet_size)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot get packet size");
    return 0;
    }
  else
    bgav_log(ctx->opt, BGAV_LOG_INFO, LOG_DOMAIN, "Packet size: %d",
             priv->packet_size);
  
  priv->buffer = malloc(priv->packet_size * SCAN_PACKETS);

  
  priv->input_mem =
    bgav_input_open_memory((uint8_t*)0, 0, ctx->opt);
  
  priv->ptr = priv->buffer;
  priv->packet_start = priv->buffer;
  
  priv->first_packet_pos = ctx->input->position;
  /* Scan the stream for a PAT */

  if(!ctx->tt)
    {
    packets_scanned = 0;
    have_pat = 0;
    
    if(input_can_seek)
      priv->buffer_size =
        bgav_input_read_data(ctx->input, priv->buffer,
                             priv->packet_size * SCAN_PACKETS);
    else
      priv->buffer_size =
        bgav_input_get_data(ctx->input, priv->buffer,
                            priv->packet_size * SCAN_PACKETS);
    
    while(1)
      {
      parse_transport_packet(ctx->opt, priv);
    
      if(priv->packet.pid == 0x0000)
        {
        have_pat = 1;
        break;
        }
      packets_scanned++;

      if(!next_packet(priv))
        {
        if(input_can_seek && (packets_scanned < SCAN_PACKETS_SEEK))
          {
          priv->buffer_size =
            bgav_input_read_data(ctx->input, priv->buffer,
                                 priv->packet_size * SCAN_PACKETS);
          if(!priv->buffer_size)
            break;
          priv->ptr = priv->buffer;
          priv->packet_start = priv->buffer;
          }
        else
          break;
        }
      }
  
    if(have_pat)
      {
      /* Initialize using PAT and PMTs */
      if(!init_psi(ctx, input_can_seek))
        return 0;
      }
    else
      {
      /* Initialize raw TS */
      if(input_can_seek)
        {
        bgav_input_seek(ctx->input, priv->first_packet_pos,
                        SEEK_SET);

        priv->buffer_size =
          bgav_input_read_data(ctx->input, priv->buffer,
                               priv->packet_size * SCAN_PACKETS);
        priv->ptr = priv->buffer;
        priv->packet_start = priv->buffer;
        }
      else
        {
        priv->ptr = priv->buffer;
        priv->packet_start = priv->buffer;
        }
      if(!init_raw(ctx, input_can_seek))
        return 0;
      }
    if(input_can_seek)
      bgav_input_seek(ctx->input, priv->first_packet_pos, SEEK_SET);
    }
  else /* Track table already present */
    {
    priv->num_programs = ctx->tt->num_tracks;
    priv->programs = calloc(priv->num_programs, sizeof(*priv->programs));
    priv->programs[0].pcr_pid = ctx->input->sync_id;

    for(i = 0; i < priv->num_programs; i++)
      {
      init_streams_priv(&priv->programs[i],
                        &ctx->tt->tracks[i]);
      }
    }
  
  //  transport_packet_dump(&packet);
  if(input_can_seek)
    {
    if(get_program_durations(ctx))
      {
      ctx->flags |=
        (BGAV_DEMUXER_CAN_SEEK|BGAV_DEMUXER_SEEK_ITERATIVE);
      }
    else
      {
      bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "Could not get program durations, seeking disabled");

      }
    }
  ctx->stream_description = bgav_sprintf("MPEG-2 transport stream");
  ctx->index_mode = INDEX_MODE_MPEG;
  return 1;
  }

#if 0
static void predict_pcr_wrap(const bgav_options_t * opt, int64_t pcr)
  {
  char str[GAVL_TIME_STRING_LEN];
  int64_t time_scaled;
  gavl_time_t time;
  time_scaled = (1LL << 33) - pcr;
  time = gavl_time_unscale(90000, time_scaled);
  gavl_time_prettyprint(time, str);
  bgav_log(ctx->opt, BGAV_LOG_DEBUG, LOG_DOMAIN, "Next PCR wrap in %s", str);
  }
#endif

#define NUM_PACKETS 5 /* Packets to be processed at once */

static int process_packet(bgav_demuxer_context_t * ctx)
  {
  int i; // int skip;
  bgav_stream_t * s;
  mpegts_t * priv;
  int num_packets;
  int bytes_to_copy;
  bgav_pes_header_t pes_header;
  int64_t position;
  //  pat_section_t pats;
  //  pmt_section_t pmts;
  priv = (mpegts_t*)(ctx->priv);

  if(!priv->packet_size)
    return 0;
  
  position = ctx->input->position;

  num_packets = NUM_PACKETS;
  
  if(ctx->next_packet_pos &&
     (position + priv->packet_size * num_packets > ctx->next_packet_pos))
    num_packets = (ctx->next_packet_pos - position) / priv->packet_size;
    
  priv->buffer_size =
    bgav_input_read_data(ctx->input,
                         priv->buffer, priv->packet_size * num_packets);
  
  if(priv->buffer_size < priv->packet_size)
    return 0;
  
  priv->ptr = priv->buffer;
  priv->packet_start = priv->buffer;
  
  num_packets = priv->buffer_size / priv->packet_size;
  
  for(i = 0; i < num_packets; i++)
    {
    if(!parse_transport_packet(ctx->opt, priv))
      {
      return 0;
      }
    if(priv->packet.transport_error)
      {
      next_packet(priv);
      position += priv->packet_size;
      continue;
      }
    
    
#if 1
    //    bgav_transport_packet_dump(&priv->packet);
        
    if(!(ctx->flags & BGAV_DEMUXER_HAS_TIMESTAMP_OFFSET) &&
       (priv->programs[priv->current_program].pcr_pid > 0))
      {
      if(priv->packet.adaption_field.pcr < 0)
        {
        next_packet(priv);
        position += priv->packet_size;
        continue;
        }
      else if(priv->packet.pid !=
              priv->programs[priv->current_program].pcr_pid)
        {
        next_packet(priv);
        position += priv->packet_size;
        continue;
        }
      else
        {
        ctx->flags |= BGAV_DEMUXER_HAS_TIMESTAMP_OFFSET;
        ctx->timestamp_offset = -priv->packet.adaption_field.pcr;
        //        predict_pcr_wrap(priv->packet.adaption_field.pcr);
        }
      }
#endif
    if(!priv->packet.pid)
      {
#if 0
      bgav_pat_section_read(priv->ptr, 188 - (priv->ptr - priv->packet_start),
                            &pats);
      bgav_pat_section_dump(&pats);
#endif
      next_packet(priv);
      position += priv->packet_size;
      continue;
      }
    else if(priv->packet.pid == priv->programs[priv->current_program].program_map_pid)
      {
#if 0
      skip = 1 + priv->ptr[0];
      priv->ptr += skip;
      
      bgav_pmt_section_read(priv->ptr, 188 - (priv->ptr - priv->packet_start),
                            &pmts);
      bgav_pmt_section_dump(&pmts);
#endif
      next_packet(priv);
      position += priv->packet_size;
      continue;
      }
    
    s = bgav_track_find_stream(ctx, priv->packet.pid);
    
    if(!s)
      {
      next_packet(priv);
      position += priv->packet_size;
      continue;
      }

    if(priv->packet.payload_start) /* New packet starts here */
      {
      bgav_input_reopen_memory(priv->input_mem, priv->ptr,
                               priv->buffer_size -
                               (priv->ptr - priv->buffer));
      /* Read PES header */
      
      if(!bgav_pes_header_read(priv->input_mem, &pes_header))
        {
        return !!i;
        }
      priv->ptr += priv->input_mem->position;
      
      if(s->packet)
        {
        bgav_packet_done_write(s->packet);
        s->packet = (bgav_packet_t*)0;
        }

      /* Get start pts */

#if 1
      if(!(ctx->flags & BGAV_DEMUXER_HAS_TIMESTAMP_OFFSET) &&
         (priv->programs[priv->current_program].pcr_pid <= 0))
        {
        if(pes_header.pts < 0)
          {
          next_packet(priv);
          position += priv->packet_size;
          continue;
          }
        ctx->timestamp_offset = -pes_header.pts;
        ctx->flags |= BGAV_DEMUXER_HAS_TIMESTAMP_OFFSET;
        }
#endif
      if(!s->packet)
        {
        if(priv->do_sync)
          {
          if(s->in_time != BGAV_TIMESTAMP_UNDEFINED)
            {
            s->packet = bgav_stream_get_packet_write(s);
            s->packet->position = position;
            }
          else if(pes_header.pts < 0)
            {
            next_packet(priv);
            position += priv->packet_size;
            continue;
            }
          else
            {
            s->in_time = pes_header.pts + ctx->timestamp_offset;
            s->packet = bgav_stream_get_packet_write(s);
            }
          }
        else
          {
          s->packet = bgav_stream_get_packet_write(s);
          s->packet->position = position;
          }
        }
     
      /*
       *  Now the bad news: Some transport streams contain PES packets
       *  with a packet length field of zero. This means, we must use
       *  the payload_start bit to find out when a packet ended.
       *
       *  Here, we allocate 1024 bytes (>> transport payload) to
       *  reduce realloc overhead afterwards
       */
      
      bytes_to_copy =
        //        priv->packet_size - (priv->ptr - priv->packet_start);
        188 - (priv->ptr - priv->packet_start);
      
      bgav_packet_alloc(s->packet, 1024);

      /* Read data */
      
      memcpy(s->packet->data, priv->ptr, bytes_to_copy);

      s->packet->data_size = bytes_to_copy;
      
      if(pes_header.pts > 0)
        {
        s->packet->pts = pes_header.pts;
        check_pts_wrap(s, &s->packet->pts);
        s->packet->pts += ctx->timestamp_offset;
        }
      }
    else if(s->packet)
      {
      /* Read data */
      
      bgav_packet_alloc(s->packet,
                        s->packet->data_size +
                        priv->packet.payload_size);
      
      memcpy(s->packet->data + s->packet->data_size, priv->ptr,
             priv->packet.payload_size);
      s->packet->data_size  += priv->packet.payload_size;
      }
    next_packet(priv);
    position += priv->packet_size;
    }
  
  return 1;
  }

static int next_packet_mpegts(bgav_demuxer_context_t * ctx)
  {
  if(ctx->next_packet_pos)
    {
    int ret = 0;
    while(1)
      {
      if(!process_packet(ctx))
        {
        if(ctx->request_stream && ctx->request_stream->packet)
          {
          bgav_packet_done_write(ctx->request_stream->packet);
          ctx->request_stream->packet = (bgav_packet_t*)0;
          }
        return ret;
        }
      else
        ret = 1;
      if(ctx->input->position >= ctx->next_packet_pos)
        {
        /* We would send this packet only after the next
           packet starts, but since we know the packet is
           finished, we can do it now */
        if(ctx->request_stream && ctx->request_stream->packet)
          {
          bgav_packet_done_write(ctx->request_stream->packet);
          ctx->request_stream->packet = (bgav_packet_t*)0;
          }
        return ret;
        }
      }
    }
  else
    {
    return process_packet(ctx);
    }
  return 0;
  }

static void seek_mpegts(bgav_demuxer_context_t * ctx, int64_t time, int scale)
  {
  int64_t total_packets;
  int64_t packet;
  int64_t position;
  
  mpegts_t * priv;
  priv = (mpegts_t*)(ctx->priv);

  reset_streams_priv(ctx->tt->cur);
  
  total_packets =
    (ctx->input->total_bytes - priv->first_packet_pos) / priv->packet_size;

  packet =
    (int64_t)((double)total_packets *
              (double)gavl_time_unscale(scale, time) /
              (double)(ctx->tt->cur->duration)+0.5);
  
  position = priv->first_packet_pos + packet * priv->packet_size;
  bgav_input_seek(ctx->input, position, SEEK_SET);

  priv->do_sync = 1;
  while(!bgav_track_has_sync(ctx->tt->cur))
    {
    if(!next_packet_mpegts(ctx))
      break;
    }
  priv->do_sync = 0;
  }

static void close_mpegts(bgav_demuxer_context_t * ctx)
  {
  int i;
  mpegts_t * priv;
  priv = (mpegts_t*)(ctx->priv);
  
  if(!priv)
    return;

  if(priv->input_mem)
    {
    bgav_input_close(priv->input_mem);
    bgav_input_destroy(priv->input_mem);
    }
  if(priv->buffer)
    free(priv->buffer);
  if(priv->programs)
    {
    for(i = 0; i < priv->num_programs; i++)
      {
      if(priv->programs[i].streams)
        free(priv->programs[i].streams);
      }
    free(priv->programs);
    }
  free(priv);
  }

static int select_track_mpegts(bgav_demuxer_context_t * ctx,
                                int track)
  {
  mpegts_t * priv;
  priv = (mpegts_t*)(ctx->priv);
  priv->current_program = track;
  priv->error_counter = 0;
  
  if(ctx->flags & BGAV_DEMUXER_CAN_SEEK)
    {
    ctx->flags |= BGAV_DEMUXER_HAS_TIMESTAMP_OFFSET;
    ctx->timestamp_offset = -priv->programs[track].start_pcr;
    }
  else
    ctx->flags &= ~BGAV_DEMUXER_HAS_TIMESTAMP_OFFSET;
  
  if(ctx->input->input->seek_byte)
    {
    bgav_input_seek(ctx->input, priv->first_packet_pos,
                    SEEK_SET);
    return 1;
    }
  else
    return 0;
    
  }

const bgav_demuxer_t bgav_demuxer_mpegts =
  {
    .probe =        probe_mpegts,
    .open =         open_mpegts,
    .next_packet =  next_packet_mpegts,
    .seek =         seek_mpegts,
    .close =        close_mpegts,
    .select_track = select_track_mpegts
  };

