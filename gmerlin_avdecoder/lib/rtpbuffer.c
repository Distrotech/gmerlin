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

#include <avdec_private.h>
#include <rtp.h>
#include <stdlib.h>

#define MAX_PACKETS 10

struct bgav_rtp_packet_buffer_s
  {
  rtp_packet_t packets[MAX_PACKETS];
  int next_seq;
  const bgav_options_t * opt;
  int num;
  uint64_t seq_offset;
  rtp_stats_t * stats;
  int timescale;
  
  int timestamp_wrap;
  int64_t timestamp_offset;
  int64_t last_timestamp;
  };

/* Statistics handling (from RFC 1889) */

#define RTP_SEQ_MOD (1<<16)

static void init_stats(rtp_stats_t *s, uint16_t seq, int64_t timestamp,
                       int timescale)
  {
  s->base_seq = seq - 1;
  s->max_seq = seq;
  s->bad_seq = RTP_SEQ_MOD + 1;
  s->cycles = 0;
  s->received = 0;
  s->received_prior = 0;
  s->expected_prior = 0;
  /* other initialization */
  s->initialized = 1;
  gavl_timer_stop(s->timer);
  gavl_timer_set(s->timer, 0);
  gavl_timer_start(s->timer);
  
  s->time_offset = gavl_time_unscale(timescale, timestamp);
  }

static int update_stats(rtp_stats_t * s, uint16_t seq,
                        uint64_t timestamp, int timescale)
  {
  uint16_t udelta = seq - s->max_seq;
  const int MAX_DROPOUT = 3000;
  const int MAX_MISORDER = 100;
  const int MIN_SEQUENTIAL = 2;
  int64_t arrival;
  
  /* Jitter estimation */
  int transit;
  int d;
  
  /*
   * Source is not valid until MIN_SEQUENTIAL packets with
   * sequential sequence numbers have been received.
   */
  if (s->probation)
    {
    /* packet is in sequence */
    if (seq == s->max_seq + 1)
      {
      s->probation--;
      s->max_seq = seq;
      if (s->probation == 0)
        {
        init_stats(s, seq, timestamp, timescale);
        s->received++;
        return 1;
        }
      }
    else
      {
      s->probation = MIN_SEQUENTIAL - 1;
      s->max_seq = seq;
      }
    return 0;
    }
  else if (udelta < MAX_DROPOUT)
    {
    /* in order, with permissible gap */
    if (seq < s->max_seq)
      {
      /*
       * Sequence number wrapped - count another 64K cycle.
       */
      s->cycles += RTP_SEQ_MOD;
      }
    s->max_seq = seq;
    }
  else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER)
    {
    /* the sequence number made a very large jump */
    if (seq == s->bad_seq)
      {
      /*
       * Two sequential packets -- assume that the other side
       * restarted without telling us so just re-sync
       * (i.e., pretend this was the first packet).
       */
      init_stats(s, seq, timestamp, timescale);
      }
    else
      {
      s->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
      return 0;
      }
    }
  else
    {
    /* duplicate or reordered packet */
    }
  s->received++;

  /* Jitter estimation */
  arrival = gavl_time_scale(timescale,
                            gavl_timer_get(s->timer) + s->time_offset);
  
  transit = arrival - timestamp;
  d = transit - s->transit;
  s->transit = transit;
  if (d < 0)
    d = -d;
  s->jitter += d - ((s->jitter + 8) >> 4);
  
  
  return 1;
  }

/* */ 

bgav_rtp_packet_buffer_t *
bgav_rtp_packet_buffer_create(const bgav_options_t * opt,
                              rtp_stats_t * stats, int timescale)
  {
  bgav_rtp_packet_buffer_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->stats = stats;
  ret->next_seq = -1;
  ret->opt = opt;
  ret->timescale = timescale;
  ret->last_timestamp = BGAV_TIMESTAMP_UNDEFINED;
  return ret;
  }

void bgav_rtp_packet_buffer_destroy(bgav_rtp_packet_buffer_t * b)
  {
  free(b);
  }

rtp_packet_t *
bgav_rtp_packet_buffer_get_write(bgav_rtp_packet_buffer_t * b)
  {
  int i;
  for(i = 0; i < MAX_PACKETS; i++)
    {
    if(!b->packets[i].valid)
      return &b->packets[i];
    }
  return (rtp_packet_t*)0;
  }

void bgav_rtp_packet_buffer_done_write(bgav_rtp_packet_buffer_t * b,
                                       rtp_packet_t * p)
  {
  p->valid = 1;
  b->num++;
  if(b->next_seq == -1)
    b->next_seq = p->h.sequence_number;
  
  /* Correct timestamp */
  //  fprintf(stderr, "RTP Time 1: %ld (%ld)", p->h.timestamp, b->last_timestamp);
  if((b->last_timestamp != BGAV_TIMESTAMP_UNDEFINED) &&
     (int64_t)b->last_timestamp - (int64_t)p->h.timestamp > 0x80000000LL)
    b->timestamp_wrap = 1;
  else if(b->timestamp_wrap &&
          (p->h.timestamp > 0x80000000LL))
    {
    b->timestamp_wrap = 0;
    b->timestamp_offset += 0x100000000LL;
    }

  //  fprintf(stderr, "Wrap: %d\n", b->timestamp_wrap);
  
  b->last_timestamp = p->h.timestamp;
  
  if(b->timestamp_wrap && 
     (p->h.timestamp < 0x80000000LL))
    p->h.timestamp += 0x100000000LL + b->timestamp_offset;
  else
    p->h.timestamp += b->timestamp_offset;

  //  fprintf(stderr, "RTP Time 2: %ld\n", p->h.timestamp);
  
  /* Update statistics */
  if(!b->stats->initialized)
    init_stats(b->stats, p->h.sequence_number, p->h.timestamp,
               b->timescale);
  else
    update_stats(b->stats, p->h.sequence_number, p->h.timestamp,
                 b->timescale);
  
  }

rtp_packet_t *
bgav_rtp_packet_buffer_get_read(bgav_rtp_packet_buffer_t * b)
  {
  int min_seq, max_seq, i;
  int min_index;
  int test;
  
  for(i = 0; i < MAX_PACKETS; i++)
    {
    if(b->packets[i].valid & (b->next_seq == b->packets[i].h.sequence_number))
      {
      return &b->packets[i];
      }
    }
  if(b->num < MAX_PACKETS)
    return (rtp_packet_t*)0;

  /* Drop packets */
  min_seq = 0x10000;
  max_seq = -1;
  
  for(i = 0; i < MAX_PACKETS; i++)
    {
    if(b->packets[i].valid)
      {
      if(min_seq > b->packets[i].h.sequence_number)
        {
        min_seq = b->packets[i].h.sequence_number;
        min_index = i;
        }
      if(max_seq < b->packets[i].h.sequence_number)
        max_seq = b->packets[i].h.sequence_number;
      }
    }
  
  /* No wrap */
  if(max_seq - min_seq < 0x8000)
    return &b->packets[min_index];

  min_seq = 0x10000;
  
  for(i = 0; i < MAX_PACKETS; i++)
    {
    if(b->packets[i].valid)
      {
      test = b->packets[i].h.sequence_number > 0x8000 ?
        (int)b->packets[i].h.sequence_number - 0x10000 :
        b->packets[i].h.sequence_number;
      
      if(min_seq > test)
        {
        min_seq = test;
        min_index = i;
        }
      }
    }
  return &b->packets[min_index];
  }

void bgav_rtp_packet_buffer_done_read(bgav_rtp_packet_buffer_t * b, rtp_packet_t * p)
  {
  int i;
  p->valid = 0;
  b->num--;
  
  /* Delete obsolete packets */
  for(i = 0; i < MAX_PACKETS; i++)
    {
    if(b->packets[i].valid & (b->packets[i].h.sequence_number <
                              p->h.sequence_number))
      {
      b->packets[i].valid = 0;
      b->num--;
      }
    }

  b->next_seq = p->h.sequence_number+1;
  if(b->next_seq > 0xffff)
    b->next_seq = 0;
  }
