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

#define MAX_PACKETS 16

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
  };

/*
 * We handle the following cases:
 *
 * - True timestamps but no durations (low delay)
 * - True timestamps but no durations (B-frames)
 * - wrong B-timestamps (also calculates durations)
 */

static void set_duration(bgav_packet_timer_t * pt,
                         int index, int64_t duration)
  {
  pt->packets[index]->duration = duration;
  pt->last_duration = duration;
  }

static int get_packet(bgav_packet_timer_t * pt, int force)
  {
  bgav_packet_t * p;
  int got_eof = 0;
  int i;
  
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

  /* Flush final packets */
  if(got_eof)
    {
    if(pt->s->flags & STREAM_WRONG_B_TIMESTAMPS)
      {
      
      }
    else if(pt->s->flags & STREAM_B_FRAMES)
      {
      if(pt->num_packets && pt->num_ip_frames &&
         (PACKET_GET_CODING_TYPE(pt->packets[pt->num_packets-1]) == BGAV_CODING_TYPE_B))
        {
        for(i = pt->num_packets-1; i >= 0; i--)
          {
          if(PACKET_GET_CODING_TYPE(pt->packets[i]) != BGAV_CODING_TYPE_B)
            {
            /* Set the duration of the last B frame */
            set_duration(pt, pt->num_packets-1,
                         pt->packets[i]->pts - pt->packets[pt->num_packets-1]->pts);
            /* Set the duration of the last I/P frame */
            set_duration(pt, i, pt->last_duration);
            break;
            }
          }
        }
      }
    else
      {
      /* Simple case */
      if(pt->num_packets)
        set_duration(pt, pt->num_packets-1, pt->last_duration);
      }
    return 0;
    }
  
  /* Insert packet */
  p->duration = -1;
  pt->packets[pt->num_packets] = p;
  
  if(pt->s->flags & STREAM_WRONG_B_TIMESTAMPS)
    {
    p->dts = p->pts;
    p->pts = BGAV_TIMESTAMP_UNDEFINED;
    }
  
  /* Try to update stuff */
  if(pt->s->flags & STREAM_WRONG_B_TIMESTAMPS)
    {
    if(pt->num_packets)
      {
      /* Duration is the dts difference */
      set_duration(pt, pt->num_packets-1,
                   pt->packets[pt->num_packets]->dts -
                   pt->packets[pt->num_packets-1]->dts);
      }
    
    /* Durations are ok, just need the PTS */
    
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
    
    }
  else if(pt->s->flags & STREAM_B_FRAMES)
    {
    /* PTS are ok, just need the durations */
    
    if(pt->num_packets)
      {
      if((pt->packets[pt->num_packets]->dts != BGAV_TIMESTAMP_UNDEFINED) &&
         (pt->packets[pt->num_packets-1]->dts != BGAV_TIMESTAMP_UNDEFINED))
        {
        /* Duration is the dts difference (if dts is set) */
        set_duration(pt, pt->num_packets-1,
                     pt->packets[pt->num_packets]->dts -
                     pt->packets[pt->num_packets-1]->dts);
        }
      }
    
    switch(PACKET_GET_CODING_TYPE(p))
      {
      case BGAV_CODING_TYPE_I:
      case BGAV_CODING_TYPE_P:
        if(pt->num_packets)
          {
          /* P/I frame after B-frame: Set duration of previous B-frame from
             previous P-frame */
          if(PACKET_GET_CODING_TYPE(pt->packets[pt->num_packets-1]) ==
             BGAV_CODING_TYPE_B)
            {
            i = pt->num_packets-2;
            while((i >= 0) &&
                  (PACKET_GET_CODING_TYPE(pt->packets[i]) == BGAV_CODING_TYPE_B))
              i--;
            if(i >= 0)
              {
              set_duration(pt, pt->num_packets-1,
                           pt->packets[i]->pts - pt->packets[pt->num_packets-1]->pts);
              }
            }
          else
            {
            /* P/I frame after P/I frame: If no B-frames are there yet,
               set the duration for all P/I frames except the last 2 */
            if(!pt->num_b_frames && (pt->num_ip_frames > 2))
              {
              for(i = 0; i < pt->num_packets-2; i++)
                {
                if(pt->packets[i]->duration <= 0)
                  set_duration(pt, i, 
                               pt->packets[i+1]->pts - pt->packets[i]->pts);
                }
              }
            }
          }

        pt->num_ip_frames++;
        break;
      case BGAV_CODING_TYPE_B:
        /* Not enough reference frames: Skip this B-frame */
        if(pt->num_ip_frames < 2)
          PACKET_SET_SKIP(p);

        /* B-frame after B-frame: Set duration of previous Frame */
        else if(PACKET_GET_CODING_TYPE(pt->packets[pt->num_packets-1]) ==
                BGAV_CODING_TYPE_B)
          {
          set_duration(pt, pt->num_packets-1,
                       p->pts - pt->packets[pt->num_packets-1]->pts);
          }
        /* B-frame after P/I frame: Set duration of 2nd previous P/I-frame */
        else
          {
          i = pt->num_packets-2;
          while((i >= 0) &&
                (PACKET_GET_CODING_TYPE(pt->packets[pt->num_packets-1]) ==
                 BGAV_CODING_TYPE_B))
            i--;
          if(i >= 0)
            set_duration(pt, i, p->pts - pt->packets[i]->pts);
          }
        pt->num_b_frames++;
        break;
      }
    }
  else /* Simple case */
    {
    if(pt->num_packets)
      {
      set_duration(pt, pt->num_packets-1,
                   pt->packets[pt->num_packets]->pts -
                   pt->packets[pt->num_packets-1]->pts);
      }
    }
  
  pt->num_packets++;
  return 1;
  }

static int have_frame(bgav_packet_timer_t * pt)
  {
  if(pt->num_packets &&
     ((pt->packets[0]->pts != BGAV_TIMESTAMP_UNDEFINED) &&
      (pt->packets[0]->duration >= 0)) ||
     PACKET_GET_SKIP(pt->packets[0]->flags))
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

  switch(PACKET_GET_CODING_TYPE(ret))
    {
    case BGAV_CODING_TYPE_I:
    case BGAV_CODING_TYPE_P:
      pt->num_ip_frames--;
      break;
    case BGAV_CODING_TYPE_B:
      pt->num_b_frames--;
      break;
    }
  
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
    {
    
    }
  
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
    {
    
    }
  
  return remove_packet(pt);
  }

bgav_packet_timer_t * bgav_packet_timer_create(bgav_stream_t * s)
  {
  bgav_packet_timer_t * ret = calloc(1, sizeof(*ret));
  ret->s = s;
  
  bgav_packet_source_copy(&ret->src, &s->src);

  s->src.get_func = get_func;
  s->src.peek_func = peek_func;
  s->src.data = ret;
  
  /* Clear wrong B-timestamps flag */
  if((ret->s->flags & STREAM_WRONG_B_TIMESTAMPS) &&
     !(ret->s->flags & STREAM_B_FRAMES))
    ret->s->flags &= ~STREAM_WRONG_B_TIMESTAMPS;

  
  return ret;
  }

void bgav_packet_timer_destroy(bgav_packet_timer_t * pt)
  {
  free(pt);
  }

void bgav_packet_timer_reset(bgav_packet_timer_t * pt)
  {
  int i;

  pt->num_b_frames = 0;
  pt->num_ip_frames = 0;
  pt->eof = 0;

  for(i = 0; i < pt->num_packets; i++)
    bgav_packet_pool_put(pt->s->pp, pt->packets[i]);
  pt->num_packets = 0;

  if(pt->out_packet)
    bgav_packet_pool_put(pt->s->pp, pt->out_packet);
  }
