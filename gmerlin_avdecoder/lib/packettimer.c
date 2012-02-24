/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#define MAX_PACKETS 32

#define DUMP_INPUT
#define DUMP_OUTPUT

struct bgav_packet_timer_s
  {
  bgav_packet_t * packets[MAX_PACKETS];
  int num_packets;
  bgav_stream_t * s;
  
  bgav_packet_source_t src;

  int eof;
  int64_t last_duration;
  
  bgav_packet_t * out_packet;
  
  int num_b_frames;
  int num_ip_frames;

  int num_b_frames_total;
  int num_ip_frames_total;
  
  //  void (*insert_packet)(bgav_packet_timer_t * pt);
  //  void (*flush)(bgav_packet_timer_t * pt);
  
  int (*next_packet)(bgav_packet_timer_t *);
  
  int64_t current_pts;
  
  };

static bgav_packet_t * insert_packet(bgav_packet_timer_t * pt)
  {
  bgav_packet_t * p;
  if(pt->num_packets >= MAX_PACKETS)
    {
    bgav_log(pt->s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Packet cache full");
    return NULL;
    }
  p = pt->src.get_func(pt->src.data);
  if(!p)
    {
    pt->eof = 1;
    return NULL;
    }
#ifdef DUMP_INPUT
  if(p)
    {
    bgav_dprintf("packet_timer in:  ");
    bgav_packet_dump(p);
    }
#endif
  
  if(PACKET_GET_CODING_TYPE(p) == BGAV_CODING_TYPE_B)
    pt->num_b_frames++;
  else
    pt->num_ip_frames++;
  
  pt->packets[pt->num_packets] = p;
  pt->num_packets++;
  return p;
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

  if(PACKET_GET_CODING_TYPE(ret) == BGAV_CODING_TYPE_B)
    pt->num_b_frames--;
  else
    pt->num_ip_frames--;
  return ret;
  }

static void set_duration(bgav_packet_timer_t * pt,
                         bgav_packet_t * p, int64_t duration)
  {
  p->duration = duration;
  pt->last_duration = duration;
  }

static void set_pts(bgav_packet_timer_t * pt,
                    bgav_packet_t * p)
  {
  p->pts = pt->current_pts;
  pt->current_pts += p->duration;
  }

static void get_first_indices(bgav_packet_timer_t * pt,
                              int * ip1, int * b1)
  {
  int i;
  *ip1 = -1;
  *b   = -1;

  for(i = 0; i < pt->num_packets; i++)
    {
    if(PACKET_GET_CODING_TYPE(pt->packets) == BGAV_CODING_TYPE_B)
      {
      if(*b < 0)
        *b = i;
      }
    else
      {
      if(*ip1 < 0)
        *ip1 = i;
      }
    if((*ip1 >= 0) && (ip2 >= 0))
      return;
    }
  }

static void get_next_ip(bgav_packet_timer_t * pt,
                        int ip1, int * ip2)
  {
  *ip2 = -1;
  for(i = ip1 + 1; i < ip->num_packets; i++)
    {
    if(PACKET_GET_CODING_TYPE(pt->packets) != BGAV_CODING_TYPE_B)
      {
      *ip2 = i;
      return;
      }
    }
  }


/* These are the actual worker functions */

/*
 *  Duration from DTS
 */

static int
next_packet_duration_from_dts(bgav_packet_timer_t * pt)
  {
  while(pt->num_packets < 2)
    {
    if(!insert_packet(pt))
      break;
    }
  
  if(pt->num_packets >= 2)
    {
    set_duration(pt, pt->packets[0], pt->packets[1]->dts - pt->packets[0]->dts);
    return 1;
    }
  else if(pt->num_packets == 1)
    {
    set_duration(pt, pt->packets[0], pt->last_duration);
    return 1;
    }
  else
    return 0;
  }

/*
 *  Duration from PTS
 */

static int
next_packet_duration_from_pts(bgav_packet_timer_t * pt)
  {
  int ip1, ip2, b;
  
  if(pt->num_packets && (pt->packets[0].duration > 0))
    return 1;
  
  while(pt->num_packets < 3)
    {
    if(!insert_packet(pt))
      break;
    }

  get_first_indices(pt, &ip1, &b1);
  
  
  return 0;
  }

/*
 *  PTS from DTS
 */

static bgav_packet_t * insert_packet_pts_from_dts(bgav_packet_timer_t * pt)
  {
  bgav_packet_t * p;
  p = insert_packet(pt);

  if(!p)
    {
    if(pt->num_packets)
      set_duration(pt, pt->packets[pt->num_packets-1], pt->last_duration);
    return NULL;
    }
  
  /* Some demuxers output dts as pts */
  if(p->dts == BGAV_TIMESTAMP_UNDEFINED)
    {
    p->dts = p->pts;
    p->pts = BGAV_TIMESTAMP_UNDEFINED;
    }

  /* Set duration */
  if(pt->num_packets > 1)
    {
    set_duration(pt, pt->packets[pt->num_packets-2],
                 pt->packets[pt->num_packets-1]->dts -
                 pt->packets[pt->num_packets-2]->dts);
    }
  return p;
  }

static int
next_packet_pts_from_dts(bgav_packet_timer_t * pt)
  {
  return 0;
  }

/*
 *  Generic functions
 */ 

static bgav_packet_t * peek_func(void * pt1, int force)
  {
  bgav_packet_timer_t * pt = pt1;

  if(pt->out_packet)
    return pt->out_packet;

  if(!pt->num_packets && pt->eof)
    return NULL;
  
  if(!pt->next_packet(pt))
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
  
  if(!pt->num_packets && pt->eof)
    return NULL;
  
  if(!pt->next_packet(pt))
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
    ret->next_packet = next_packet_pts_from_dts;
  else if(ret->s->flags & STREAM_NO_DURATIONS)
    {
    if(ret->s->flags & STREAM_HAS_DTS)
      ret->next_packet = next_packet_duration_from_dts;
    else
      ret->next_packet = next_packet_duration_from_pts;
    }
  return ret;
  }

void bgav_packet_timer_destroy(bgav_packet_timer_t * pt)
  {
  int i;
  for(i = 0; i < pt->num_packets; i++)
    bgav_packet_pool_put(pt->s->pp, pt->packets[i]);

  if(pt->out_packet)
    bgav_packet_pool_put(pt->s->pp, pt->out_packet);
  
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
  
  for(i = 0; i < pt->num_packets; i++)
    bgav_packet_pool_put(pt->s->pp, pt->packets[i]);
  pt->num_packets = 0;

  if(pt->out_packet)
    {
    bgav_packet_pool_put(pt->s->pp, pt->out_packet);
    pt->out_packet = NULL;
    }
  }
