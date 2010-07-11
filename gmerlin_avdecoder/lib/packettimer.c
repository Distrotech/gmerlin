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
#include <string.h>
#include <avdec_private.h>

#define LOG_DOMAIN "packettimer"

#define MAX_PACKETS 16

// #define DUMP_INPUT
// #define DUMP_OUTPUT

struct bgav_packet_timer_s
  {
  bgav_packet_t * packets[MAX_PACKETS];
  int num_packets;
  bgav_stream_t * s;

  bgav_packet_source_t src;

  int eof;
  int64_t last_duration;
  
  bgav_packet_t * out_packet;

  bgav_packet_t * last_ip_frame_1;
  bgav_packet_t * last_ip_frame_2;
  
  int num_b_frames;
  int num_ip_frames;

  void (*insert_packet)(bgav_packet_timer_t * pt);
  void (*flush)(bgav_packet_timer_t * pt);
  
  int64_t current_pts;
  
  };

/*
 * We handle the following cases:
 *
 * - True timestamps but no durations (low delay)
 * - True timestamps but no durations (B-frames)
 * - wrong B-timestamps (also calculates durations)
 */

static void set_duration(bgav_packet_timer_t * pt,
                         bgav_packet_t * p, int64_t duration)
  {
  p->duration = duration;
  pt->last_duration = duration;
  }

/* Methods */

/* Simple */

static void flush_simple(bgav_packet_timer_t * pt)
  {
  /* Simple case */
  if(pt->num_packets)
    set_duration(pt, pt->packets[pt->num_packets-1], pt->last_duration);
  }

static void insert_simple(bgav_packet_timer_t * pt)
  {
  if(pt->num_packets >= 2)
    set_duration(pt, pt->packets[pt->num_packets-2],
                 pt->packets[pt->num_packets-1]->pts -
                 pt->packets[pt->num_packets-2]->pts);
  }

/* Duration from DTS */

static void insert_duration_from_dts(bgav_packet_timer_t * pt)
  {
  if(pt->num_packets >= 2)
    set_duration(pt, pt->packets[pt->num_packets-2],
                 pt->packets[pt->num_packets-1]->dts -
                 pt->packets[pt->num_packets-2]->dts);
  }

/* PTS from DTS */

static void set_pts(bgav_packet_timer_t * pt,
                    bgav_packet_t * p)
  {
  p->pts = pt->current_pts;
  pt->current_pts += p->duration;
  }

static void flush_pts_from_dts(bgav_packet_timer_t * pt)
  {
  // TODO
  if(pt->num_packets)
    {
    bgav_packet_t * last_packet = pt->packets[pt->num_packets-1];
    
    set_duration(pt, last_packet, pt->last_duration);
    
    if(PACKET_GET_CODING_TYPE(last_packet) == BGAV_CODING_TYPE_B)
      set_pts(pt, last_packet);
    if(pt->last_ip_frame_1)
      set_pts(pt, pt->last_ip_frame_1);
    }
  }

static void insert_pts_from_dts(bgav_packet_timer_t * pt)
  {
  bgav_packet_t * packet;
  bgav_packet_t * last_packet;
  
  //  fprintf(stderr, "insert_pts_from_dts %d\n",
  //          pt->num_packets);
  
  packet = pt->packets[pt->num_packets-1];
  if(pt->num_packets >= 2)
    last_packet = pt->packets[pt->num_packets-2];
  else
    last_packet = NULL;
  
  /* Clear wrong AVI pts */
  packet->dts = packet->pts;
  packet->pts = BGAV_TIMESTAMP_UNDEFINED;

  if(!last_packet)
    return;

  set_duration(pt, last_packet,
               packet->dts - last_packet->dts);
  
  if(PACKET_GET_CODING_TYPE(last_packet) != BGAV_CODING_TYPE_B)
    {
    /* I/P */
    if(pt->current_pts == BGAV_TIMESTAMP_UNDEFINED)
      {
      pt->current_pts = last_packet->dts;
      set_pts(pt, last_packet);
      }
    else
      {
      if(pt->last_ip_frame_1)
        set_pts(pt, pt->last_ip_frame_1);
      pt->last_ip_frame_1 = last_packet;
      }
    }
  else
    {
    /* B */
    if(pt->num_ip_frames < 2)
      PACKET_SET_SKIP(last_packet);
    else
      set_pts(pt, last_packet);
    }
  }

/* Duration from PTS */

static void flush_duration_from_pts(bgav_packet_timer_t * pt)
  {
  bgav_packet_t * last_packet;

  if(!pt->num_packets)
    return;
  
  last_packet = pt->packets[pt->num_packets-1];
  
  if(PACKET_GET_CODING_TYPE(last_packet) == BGAV_CODING_TYPE_B)
    {
    if(pt->last_ip_frame_1)
      {
      set_duration(pt, last_packet,
                   pt->last_ip_frame_1->pts - last_packet->pts);
      set_duration(pt, pt->last_ip_frame_1, pt->last_duration);
      }
    }
  }

static void insert_duration_from_pts(bgav_packet_timer_t * pt)
  {
  bgav_packet_t * packet;
  bgav_packet_t * last_packet;
  
  /* PTS are ok, need duration */

  packet = pt->packets[pt->num_packets-1];
  if(pt->num_packets >= 2)
    last_packet = pt->packets[pt->num_packets-2];
  else
    last_packet = NULL;

  if(!last_packet)
    {
    if(PACKET_GET_CODING_TYPE(packet) != BGAV_CODING_TYPE_B)
      pt->last_ip_frame_1 = packet;
    return;
    }
  if(PACKET_GET_CODING_TYPE(packet) == BGAV_CODING_TYPE_B)
    {
    if(PACKET_GET_CODING_TYPE(last_packet) == BGAV_CODING_TYPE_B)
      {
      /* B-frame after B-frame */
      set_duration(pt, last_packet, packet->pts - last_packet->pts);
      }
    else
      {
      /* B-frame after P-frame */
      if(pt->last_ip_frame_2)
        {
        set_duration(pt, pt->last_ip_frame_2,
                     packet->pts - pt->last_ip_frame_2->pts);
        pt->last_ip_frame_2 = NULL;
        }
      else
        {
        PACKET_SET_SKIP(packet);
        }
      }
    }
  else
    {
    if(PACKET_GET_CODING_TYPE(last_packet) == BGAV_CODING_TYPE_B)
      {
      /* IP-frame after B-frame */
      set_duration(pt, last_packet,
                   pt->last_ip_frame_1->pts - last_packet->pts);
      }
    pt->last_ip_frame_2 = pt->last_ip_frame_1;
    pt->last_ip_frame_1 = packet;
    }
  
  }

static int get_packet(bgav_packet_timer_t * pt, int force)
  {
  bgav_packet_t * p;
  int got_eof = 0;

  if(pt->num_packets >= MAX_PACKETS)
    {
    bgav_log(pt->s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Packet cache full");
    return 0;
    }
  
  if(force)
    {
    p = pt->src.get_func(pt->src.data);
    if(!p)
      {
      got_eof = 1;
      pt->eof = 1;
      }
    }
  else
    {
    p = pt->src.peek_func(pt->src.data, 0);
    if(!p)
      return 0;
    p = pt->src.get_func(pt->src.data);
    }
#ifdef DUMP_INPUT
  bgav_dprintf("packet_timer in:  ");
  bgav_packet_dump(p);
#endif
  
  /* Flush final packets */
  if(got_eof)
    {
    pt->flush(pt);
    return 0;
    }
  
  /* Insert packet */
  p->duration = -1;
  pt->packets[pt->num_packets] = p;
  pt->num_packets++;
  
  pt->insert_packet(pt);

  /* Need packet counts *without* this one */
  switch(PACKET_GET_CODING_TYPE(p))
    {
    case BGAV_CODING_TYPE_I:
    case BGAV_CODING_TYPE_P:
      pt->num_ip_frames++;
      break;
    case BGAV_CODING_TYPE_B:
      pt->num_b_frames++;
      break;
    }

  return 1;
  }

static int have_frame(bgav_packet_timer_t * pt)
  {
  if(!pt->num_packets)
    return 0;

  if(((pt->packets[0]->pts != BGAV_TIMESTAMP_UNDEFINED) &&
      (pt->packets[0]->duration >= 0)) ||
     PACKET_GET_SKIP(pt->packets[0]))
    return 1;
  else
    return 0;
  }

static bgav_packet_t * remove_packet(bgav_packet_timer_t * pt)
  {
  bgav_packet_t * ret;
  ret = pt->packets[0];

  pt->num_packets--;
  if(pt->num_packets)
    memmove(pt->packets, pt->packets+1, sizeof(*pt->packets)*pt->num_packets);
#ifdef DUMP_OUTPUT
  bgav_dprintf("packet_timer out: ");
  bgav_packet_dump(ret);
#endif
  return ret;
  }

static bgav_packet_t * peek_func(void * pt1, int force)
  {
  bgav_packet_timer_t * pt = pt1;
  
  if(pt->out_packet)
    return pt->out_packet;
  
  while(!have_frame(pt))
    {
    if(!get_packet(pt, force))
      break;
    }
  
  if(!have_frame(pt))
    return NULL;
  
  pt->out_packet = remove_packet(pt);
  return pt->out_packet;
  }

static bgav_packet_t * get_func(void * pt1)
  {
  bgav_packet_t * ret;
  bgav_packet_timer_t * pt = pt1;
  
  if(pt->out_packet)
    {
    ret = pt->out_packet;
    pt->out_packet = NULL;
    return ret;
    }
  
  while(!have_frame(pt))
    {
    if(!get_packet(pt, 1))
      break;
    }
  
  if(!have_frame(pt))
    return NULL;
  
  return remove_packet(pt);
  }

bgav_packet_timer_t * bgav_packet_timer_create(bgav_stream_t * s)
  {
  bgav_packet_timer_t * ret = calloc(1, sizeof(*ret));
  ret->s = s;
  ret->current_pts = BGAV_TIMESTAMP_UNDEFINED;

  bgav_packet_source_copy(&ret->src, &s->src);

  s->src.get_func = get_func;
  s->src.peek_func = peek_func;
  s->src.data = ret;
  
  /* Clear wrong B-timestamps flag */
  if((ret->s->flags & STREAM_WRONG_B_TIMESTAMPS) &&
     !(ret->s->flags & STREAM_B_FRAMES))
    ret->s->flags &= ~STREAM_WRONG_B_TIMESTAMPS;

  /* Set insert and flush functions */
  if(ret->s->flags & STREAM_WRONG_B_TIMESTAMPS)
    {
    ret->insert_packet = insert_pts_from_dts;
    ret->flush         = flush_pts_from_dts;
    }
  else if(ret->s->flags & STREAM_B_FRAMES)
    {
    if(ret->s->flags & STREAM_HAS_DTS)
      {
      ret->insert_packet = insert_duration_from_dts;
      ret->flush         = flush_simple;
      }
    else
      {
      ret->insert_packet = insert_duration_from_pts;
      ret->flush         = flush_duration_from_pts;
      }
    }
  else
    {
    ret->insert_packet = insert_simple;
    ret->flush         = flush_simple;
    }
  return ret;
  }

void bgav_packet_timer_destroy(bgav_packet_timer_t * pt)
  {
  free(pt);
  }

void bgav_packet_timer_reset(bgav_packet_timer_t * pt)
  {
  int i;

  //  fprintf(stderr, "bgav_packet_timer_reset %d\n",
  //          pt->num_packets);
  
  pt->num_b_frames = 0;
  pt->num_ip_frames = 0;
  pt->eof = 0;
  pt->current_pts = BGAV_TIMESTAMP_UNDEFINED;
  pt->last_ip_frame_1 = NULL;
  pt->last_ip_frame_2 = NULL;
  
  for(i = 0; i < pt->num_packets; i++)
    bgav_packet_pool_put(pt->s->pp, pt->packets[i]);
  pt->num_packets = 0;

  if(pt->out_packet)
    {
    bgav_packet_pool_put(pt->s->pp, pt->out_packet);
    pt->out_packet = NULL;
    }
  }
