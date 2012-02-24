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
  
  int num_b_frames;
  int num_ip_frames;

  int num_b_frames_total;
  int num_ip_frames_total;
  
  //  void (*insert_packet)(bgav_packet_timer_t * pt);
  //  void (*flush)(bgav_packet_timer_t * pt);
  
  int (*next_packet)(bgav_packet_timer_t *);
  
  int64_t current_pts;
  int64_t last_b_pts; // For detecting B-Pyramid
  
  int b_pyramid;
  };

#define IS_B(idx) (PACKET_GET_CODING_TYPE(pt->packets[idx]) == BGAV_CODING_TYPE_B)
#define IS_IP(idx) (PACKET_GET_CODING_TYPE(pt->packets[idx]) != BGAV_CODING_TYPE_B)

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
    {
    if(pt->num_ip_frames_total != 2)
      PACKET_SET_SKIP(p);
    
    pt->num_b_frames++;
    pt->num_b_frames_total++;
    }
  else
    {
    pt->num_ip_frames++;
    pt->num_ip_frames_total++;
    }
  
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
    memmove(&pt->packets[0], &pt->packets[1], sizeof(pt->packets[0])*pt->num_packets);
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

#if 0
static void set_duration(bgav_packet_timer_t * pt,
                         bgav_packet_t * p, int64_t duration)
  {
  p->duration = duration;
  pt->last_duration = duration;
  }

static void get_first_indices(bgav_packet_timer_t * pt,
                              int * ip1, int * b1)
  {
  int i;
  *ip1 = -1;
  *b1   = -1;

  for(i = 0; i < pt->num_packets; i++)
    {
    if(IS_B(i))
      {
      if(*b1 < 0)
        *b1 = i;
      }
    else
      {
      if(*ip1 < 0)
        *ip1 = i;
      }
    if((*ip1 >= 0) && (*b1 >= 0))
      return;
    }
  }
#endif

static int get_next_ip(bgav_packet_timer_t * pt,
                       int index)
  {
  int i;
  for(i = index + 1; i < pt->num_packets; i++)
    {
    if(IS_IP(i))
      return i;
    }
  return -1;
  }


/* These are the actual worker functions */

/*
 *  Duration from DTS
 */

static int set_duration_from_dts(bgav_packet_timer_t * pt, int index)
  {
  if(index >= pt->num_packets)
    {
    // BUG!!!
    return 0;
    }
  if(index == pt->num_packets - 1)
    pt->packets[index]->duration = pt->last_duration;
  else
    {
    pt->packets[index]->duration = pt->packets[index+1]->dts - pt->packets[index]->dts;
    pt->last_duration = pt->packets[index]->duration;
    }
  return 1;
  }

static int
next_packet_duration_from_dts(bgav_packet_timer_t * pt)
  {
  if(pt->num_packets && ((pt->packets[0]->duration > 0) || PACKET_GET_SKIP(pt->packets[0])))
    return 1;
  
  while(pt->num_packets < 2)
    {
    if(!insert_packet(pt))
      break;
    }
  if(!pt->num_packets)
    return 0;
  set_duration_from_dts(pt, 0);
  return 1;
  }

/*
 *  Duration from PTS (Matroska, ASF, NSV, NUV, rm, smjpeg....)
 */

static bgav_packet_t * insert_packet_duration_from_pts(bgav_packet_timer_t * pt)
  {
  bgav_packet_t * p;
  p = insert_packet(pt);

  if(!p)
    return NULL;
  
  /* Detect B-Pyramid */

  if((PACKET_GET_CODING_TYPE(p) == BGAV_CODING_TYPE_B) && !PACKET_GET_SKIP(p))
    {
    if(!pt->b_pyramid &&
       (pt->last_b_pts != GAVL_TIME_UNDEFINED) &&
       (p->pts < pt->last_b_pts))
      {
      bgav_log(pt->s->opt, BGAV_LOG_INFO, LOG_DOMAIN,
               "Detected B-Pyramid");
      pt->b_pyramid = 1;
      }
    pt->last_b_pts = p->pts;
    }
  
  return p;
  }

/*
 *   Set duration of this_index from the pts difference of this_index and next_index
 *   If next_index < 0, we have EOF
 */ 

static int set_duration_from_pts(bgav_packet_timer_t * pt, int this_index, int next_index)
  {
  if(next_index < 0)
    {
    pt->packets[this_index]->duration = pt->last_duration;
    return 1;
    }
  pt->packets[this_index]->duration = pt->packets[next_index]->pts - pt->packets[this_index]->pts;
  pt->last_duration = pt->packets[this_index]->duration;
  return 1;
  }

static int
next_packet_duration_from_pts(bgav_packet_timer_t * pt)
  {
  int next_ip;
  int last_b;
  bgav_packet_t * p;
  int i;
  
  if(pt->num_packets && ((pt->packets[0]->duration > 0) || PACKET_GET_SKIP(pt->packets[0])))
    return 1;
  
  while(pt->num_packets < 3)
    {
    if(!insert_packet_duration_from_pts(pt))
      break;
    }
  
  if(!pt->num_packets)
    return 0;
  
  /* Last packet */
  if(pt->num_packets == 1)
    {
    set_duration_from_pts(pt, 0, -1);
    return 1;
    }
  
  /* Last 2 packets */
  if(pt->num_packets == 2)
    {
    if(IS_IP(0))
      {
      // PP
      if(IS_IP(1))
        {
        set_duration_from_pts(pt, 0, 1);
        set_duration_from_pts(pt, 1, -1);
        return 1;
        }
      // PB
      else
        {
        set_duration_from_pts(pt, 1, 0);
        set_duration_from_pts(pt, 0, -1);
        return 1;
        }
      }
    else
      {
      // BUG
      }
    }

  // 3 Packets
  if(IS_IP(0))
    {
    if(IS_IP(1))
      {
      // IPP
      if(IS_IP(2))
        {
        set_duration_from_pts(pt, 0, 1);
        return 1;
        }
      // IPB
      else
        {
        // This will be wrong for B-Pyramid!
        set_duration_from_pts(pt, 0, 2);
        return 1;
        }
      }
    else
      {
      // PB

      next_ip = get_next_ip(pt, 1);

      if(next_ip < 0)
        {
        // Read packets until this B-frame sequence is finished

        while(1)
          {
          p = insert_packet_duration_from_pts(pt);

          if(!p)
            break;

          if(PACKET_GET_CODING_TYPE(p) != BGAV_CODING_TYPE_B)
            {
            next_ip = pt->num_packets-1;
            break;
            }
          }
        }
      
      if(next_ip < 0)
        {
        if(!pt->eof)
          return 0; // Cache full
        last_b = pt->num_packets-1;
        }
      else
        last_b = next_ip - 1;
      
      // Get durations of the B-frame sequence

      for(i = 1; i < last_b; i++)
        {
        // This will be wrong for B-Pyramid!
        set_duration_from_pts(pt, i, i+1);
        }
      set_duration_from_pts(pt, last_b, 0);

      /* If we have eof, we stop here */
      if(next_ip < 0)
        {
        set_duration_from_pts(pt, 0, -1);
        return 1;
        }
      else
        {
        if(next_ip == pt->num_packets - 1)
          {
          p = insert_packet_duration_from_pts(pt);

          // PBBBP.
          if(!p)
            {
            if(!pt->eof)
              return 0; // Cache full
            
            set_duration_from_pts(pt, 0, pt->num_packets - 1);
            set_duration_from_pts(pt, pt->num_packets - 1, -1);
            return 1;
            }
          }

        // PBBBPP
        if(IS_IP(next_ip+1))
          {
          set_duration_from_pts(pt, 0, next_ip);
          return 1;
          }
        // PBBBPB
        else
          {
          set_duration_from_pts(pt, 0, next_ip+1);
          return 1;
          }
        
        }
      
      }
    }
  else
    {
    //BUG
    }
  
  return 0;
  }

/*
 *  PTS from DTS (AVI, OGM)
 */

static void set_pts_from_dts(bgav_packet_timer_t * pt,
                             bgav_packet_t * p)
  {
  if(pt->current_pts == GAVL_TIME_UNDEFINED)
    pt->current_pts = p->dts;
  
  p->pts = pt->current_pts;
  pt->current_pts += p->duration;
  }

static bgav_packet_t * insert_packet_pts_from_dts(bgav_packet_timer_t * pt)
  {
  bgav_packet_t * p;
  p = insert_packet(pt);

  if(!p)
    {
    if(pt->num_packets)
      set_duration_from_dts(pt, pt->num_packets-1);
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
    set_duration_from_dts(pt, pt->num_packets-2);
  return p;
  }

static int
next_packet_pts_from_dts(bgav_packet_timer_t * pt)
  {
  int i;
  
  if(pt->num_packets &&
     ((pt->packets[0]->pts != GAVL_TIME_UNDEFINED) || PACKET_GET_SKIP(pt->packets[0])))
    return 1;
  
  while(pt->num_packets < 2)
    {
    if(!insert_packet_pts_from_dts(pt))
      break;
    }

  if(!pt->num_packets)
    return 0;

  if(PACKET_GET_CODING_TYPE(pt->packets[0]) != BGAV_CODING_TYPE_B)
    {
    /* Last packet (non B) */
    if(pt->num_packets == 1)
      {
      set_pts_from_dts(pt, pt->packets[0]);
      return 1;
      }
    else
      {
      /* IP , PP -> output first packet */
      if(PACKET_GET_CODING_TYPE(pt->packets[1]) != BGAV_CODING_TYPE_B)
        {
        set_pts_from_dts(pt, pt->packets[0]);
        return 1;
        }
      /* IB, PB -> go to the end of the B-frame
         sequence to get the pts of the non-B frame */
      else
        {
        bgav_packet_t * p;
        while(1)
          {
          /* Get packets until the next non B-frame */
          p = insert_packet_pts_from_dts(pt);

          if(!p)
            {
            if(pt->eof)
              break;
            else
              return 0; // Cache full
            }
          if(PACKET_GET_CODING_TYPE(p) != BGAV_CODING_TYPE_B)
            break;
          }

        for(i = 1; i < pt->num_packets; i++)
          {
          if(PACKET_GET_CODING_TYPE(pt->packets[i]) != BGAV_CODING_TYPE_B)
            break;
          
          if(!PACKET_GET_SKIP(pt->packets[i]))
            set_pts_from_dts(pt, pt->packets[i]);
          }
        set_pts_from_dts(pt, pt->packets[0]);
        return 1;
        }
      }
    }
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
  pt->num_b_frames_total = 0;
  pt->num_ip_frames_total = 0;
  
  pt->eof = 0;
  pt->current_pts = BGAV_TIMESTAMP_UNDEFINED;
  pt->last_b_pts = BGAV_TIMESTAMP_UNDEFINED;
  
  for(i = 0; i < pt->num_packets; i++)
    bgav_packet_pool_put(pt->s->pp, pt->packets[i]);
  pt->num_packets = 0;

  if(pt->out_packet)
    {
    bgav_packet_pool_put(pt->s->pp, pt->out_packet);
    pt->out_packet = NULL;
    }
  }
