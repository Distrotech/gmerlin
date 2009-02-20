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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>
#include <mpeg2dec/mpeg2.h>

#include <config.h>

#include <avdec_private.h>
#include <codecs.h>
#include <ptscache.h>

#define LOG_DOMAIN "video_libmpeg2"

static const char picture_types[] = { "?IPB????" };

/* Debug function */
#if 0
void dump_sequence_header(const mpeg2_sequence_t * s)
  {
  bgav_dprintf("Sequence header:\n");
  bgav_dprintf("size:         %d x %d\n", s->width, s->height);
  bgav_dprintf("chroma_size:  %d x %d\n", s->chroma_width,
          s->chroma_height);

  bgav_dprintf("picture_size: %d x %d\n", s->picture_width,
          s->picture_height);
  bgav_dprintf("display_size: %d x %d\n", s->display_width,
          s->display_height);
  bgav_dprintf("pixel_size:   %d x %d\n", s->pixel_width,
          s->pixel_height);
  }
#endif

typedef struct
  {
  const mpeg2_info_t * info;
  mpeg2dec_t * dec;
  gavl_video_frame_t * frame;
  bgav_packet_t * p;
  
  gavl_time_t picture_duration;
  gavl_time_t picture_timestamp;

  int extern_aspect; /* Container sent us the aspect ratio already */
  
  int init;
  int have_frame;

  /*
   *  Specify how many non-B frames we saw since the
   *  last seek. We needs this to skip old B-Frames
   *  which are output by libmpeg2 by accident
   */
  
  int non_b_count;
  int eof;
  uint8_t sequence_end_code[4];
  
  int has_gop_timecode;
  gavl_timecode_t gop_timecode;

  bgav_pts_cache_t pts_cache;
  } mpeg2_priv_t;

static int get_data(bgav_stream_t*s)
  {
  mpeg2_priv_t * priv;
  int cache_index;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);
  
  if(priv->p)
    {
    bgav_demuxer_done_packet_read(s->demuxer, priv->p);
    }
  priv->p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!priv->p)
    {
    if(!priv->eof)
      {
      /* Flush the last picture */
      priv->sequence_end_code[0] = 0x00;
      priv->sequence_end_code[1] = 0x00;
      priv->sequence_end_code[2] = 0x01;
      priv->sequence_end_code[3] = 0xb7;

      mpeg2_buffer(priv->dec,
                   &priv->sequence_end_code[0],
                   &priv->sequence_end_code[4]);
      priv->eof = 1;
      return 1;
      }
    else
      return 0;
    }
  priv->eof = 0;
  mpeg2_buffer(priv->dec, priv->p->data, priv->p->data + priv->p->data_size);

  bgav_pts_cache_push(&priv->pts_cache,
                      priv->p->pts, priv->p->duration,
                      &cache_index,
                      (bgav_pts_cache_entry_t**)0);
  
  mpeg2_tag_picture(priv->dec, cache_index, 0);
  
  return 1;
  }

static int parse(bgav_stream_t*s, mpeg2_state_t * state)
  {
  mpeg2_priv_t * priv;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);

  while(1)
    {
    *state = mpeg2_parse(priv->dec);
    if(*state == STATE_BUFFER)
      {
      if(!get_data(s))
        return 0;
      continue;
      }
    
    if(*state == STATE_GOP)
      {
      if(!s->data.video.format.timecode_format.int_framerate)
        {
        s->data.video.format.timecode_format.int_framerate =
          (int)((float)s->data.video.format.timescale /
                (float)s->data.video.format.frame_duration+0.5);
        if(priv->info->gop->flags & GOP_FLAG_DROP_FRAME)
          s->data.video.format.timecode_format.flags |=
            GAVL_TIMECODE_DROP_FRAME;
        }
      gavl_timecode_from_hmsf(&priv->gop_timecode,
                              priv->info->gop->hours,
                              priv->info->gop->minutes,
                              priv->info->gop->seconds,
                              priv->info->gop->pictures);
      priv->has_gop_timecode = 1;
      }
    return 1;
    }
  }

static void get_format(bgav_stream_t*s,
                       gavl_video_format_t * ret,
                       const mpeg2_sequence_t * sequence)
  {
  mpeg2_priv_t * priv;
  int container_time;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);

  container_time = (ret->timescale > 0) && (ret->frame_duration > 0);

  if(!container_time)
    {
    switch(sequence->frame_period)
      {
      /* Original timscale is 27.000.000, a bit too much for us */

      /* We choose duration/scale so that the duration is always even.
         Since the MPEG-2 repeat stuff happens at half-frametime level,
         we nevertheless get 100% precise timestamps
      */
    
      case 1126125: /* 24000 / 1001 */
        ret->timescale = 48000;
        ret->frame_duration = 2002;
        break;
      case 1125000: /* 24 / 1 */
        ret->timescale = 48;
        ret->frame_duration = 2;
        break;
      case 1080000: /* 25 / 1 */
        ret->timescale = 50;
        ret->frame_duration = 2;
        break;
      case 900900: /* 30000 / 1001 */
        ret->timescale = 60000;
        ret->frame_duration = 2002;
        break;
      case 900000: /* 30 / 1 */
        ret->timescale = 60;
        ret->frame_duration = 2;
        break;
      case 540000: /* 50 / 1 */
        ret->timescale = 100;
        ret->frame_duration = 2;
        break;
      case 450450: /* 60000 / 1001 */
        ret->timescale = 120000;
        ret->frame_duration = 2002;
        break;
      case 450000: /* 60 / 1 */
        ret->timescale = 120;
        ret->frame_duration = 2;
        break;
      }
    }
  
  ret->image_width  = sequence->picture_width;
  ret->image_height = sequence->picture_height;
                                                                               
  ret->frame_width  = sequence->width;
  ret->frame_height = sequence->height;

  if(!ret->pixel_width)
    {
    ret->pixel_width  = sequence->pixel_width;
    ret->pixel_height = sequence->pixel_height;
    }
  else
    priv->extern_aspect = 1;
  
  if(sequence->chroma_height == sequence->height/2)
    {
    ret->pixelformat = GAVL_YUV_420_P;
    if(sequence->flags & SEQ_FLAG_MPEG2)
      ret->chroma_placement = GAVL_CHROMA_PLACEMENT_MPEG2;
    }
  
  else if(sequence->chroma_height == sequence->height)
    ret->pixelformat = GAVL_YUV_422_P;

  if(!container_time)
    {
    if(sequence->flags & SEQ_FLAG_MPEG2)
      ret->framerate_mode = GAVL_FRAMERATE_VARIABLE;
    else /* MPEG-1 is always constant framerate */
      {
      ret->timescale /= 2;
      ret->frame_duration /= 2;
      s->data.video.frametime_mode = BGAV_FRAMETIME_CONSTANT;
      }
    }

  //  dump_sequence_header(sequence);

  // Get interlace mode
  if((sequence->flags & SEQ_FLAG_MPEG2) &&
     !(sequence->flags & SEQ_FLAG_PROGRESSIVE_SEQUENCE))
    ret->interlace_mode = GAVL_INTERLACE_MIXED;
  else
    ret->interlace_mode = GAVL_INTERLACE_NONE;
  }

/* Decode one picture, frame will be in priv->info->display_picture
   after */

static int decode_picture(bgav_stream_t*s)
  {
  int done;
  gavl_video_format_t new_format;
  int cache_index;
  mpeg2_priv_t * priv;
  mpeg2_state_t state;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);
  done = 0;
  while(1)
    {
    if(!parse(s, &state))
      return 0;
    
    if(((state == STATE_END) || (state == STATE_SLICE) ||
        (state == STATE_INVALID_END)))
      {
      if(state == STATE_END)
        {
        priv->eof = 1;
        if(priv->info->display_picture)
          done = 1;
        }
      if(priv->info->current_picture)
        {
        switch(priv->info->current_picture->flags & PIC_MASK_CODING_TYPE)
          {
          case PIC_FLAG_CODING_TYPE_I:
          case PIC_FLAG_CODING_TYPE_P:
            priv->non_b_count++;
            done = 1;
            break;
          case PIC_FLAG_CODING_TYPE_B:
            if(priv->non_b_count >= 2)
              done = 1;
            break;
          }
        }      
      if(!priv->info->display_fbuf)
        done = 0;
      if(done)
        break;
      }
#if MPEG2_RELEASE >= MPEG2_VERSION(0,5,0)
    else if((state == STATE_SEQUENCE) ||
            (state == STATE_SEQUENCE_REPEATED) ||
            (state == STATE_SEQUENCE_MODIFIED))
#else
    else if((state == STATE_SEQUENCE) ||
            (state == STATE_SEQUENCE_REPEATED))
#endif
      {
      memset(&new_format, 0, sizeof(new_format));
      get_format(s, &(new_format), priv->info->sequence);
      
      if((new_format.image_width != s->data.video.format.image_width) ||
         (new_format.image_height != s->data.video.format.image_height))
        bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "Detected change of image size, not handled yet");

      if(!priv->extern_aspect)
        {
        if((s->data.video.format.pixel_width != new_format.pixel_width) ||
           (s->data.video.format.pixel_height != new_format.pixel_height))
          {
          bgav_log(s->opt, BGAV_LOG_INFO, LOG_DOMAIN,
                   "Detected change of pixel aspect ratio: %dx%d",
                   new_format.pixel_width,
                   new_format.pixel_height);
          if(s->opt->aspect_callback)
            {
            s->opt->aspect_callback(s->opt->aspect_callback_data,
                                    bgav_stream_get_index(s),
                                    new_format.pixel_width,
                                    new_format.pixel_height);
            }
          s->data.video.format.pixel_width = new_format.pixel_width;
          s->data.video.format.pixel_height = new_format.pixel_height;
          }
        }
      }

    }
  
  if((state == STATE_END) && priv->init)
    {
    /* If we have a sequence end code after the first frame,
       force still mode */
    bgav_log(s->opt, BGAV_LOG_DEBUG, LOG_DOMAIN, "Detected MPEG still image");
    s->flags |= STREAM_STILL_MODE;
    // s->data.video.format.framerate_mode = GAVL_FRAMERATE_STILL;
    if(priv->p)
      bgav_demuxer_done_packet_read(s->demuxer, priv->p);

    }

  /* Calculate timestamp */
    
  if(priv->info->display_picture->flags & PIC_FLAG_TAGS)
    {
    cache_index = priv->info->display_picture->tag;

    priv->picture_timestamp = priv->pts_cache.entries[cache_index].pts;
    priv->picture_duration  = priv->pts_cache.entries[cache_index].duration;
    priv->pts_cache.entries[cache_index].used = 0;
    }
  else /* Should never happen */
    {
    priv->picture_timestamp += priv->picture_duration;
    }
  
  return 1;
  }


static int decode_mpeg2(bgav_stream_t*s, gavl_video_frame_t*f)
  {
  mpeg2_priv_t * priv;

  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);
  
  /* Decode frame */
  
#if 0  
  if(f)
    mpeg2_skip(priv->dec, 0);
  else
    mpeg2_skip(priv->dec, 1);
#endif

  /* Return EOF if we are in still mode and the demuxer ran out of
     data */
  if((s->flags & STREAM_STILL_MODE) &&
     (s->flags & STREAM_STILL_SHOWN) &&
     (s->flags & STREAM_EOF))
    return 0;
  
  if((!(s->flags & STREAM_STILL_MODE) && !priv->have_frame) ||
     ((s->flags & STREAM_STILL_MODE) &&
      bgav_demuxer_peek_packet_read(s->demuxer, s, 0)))
    {
    if(!decode_picture(s))
      return 0;
    priv->have_frame = 1;
    }
  
  if(priv->init)
    {
    s->out_time = priv->picture_timestamp;
    return 1;
    }
  if(f)
    {
    priv->frame->planes[0] = priv->info->display_fbuf->buf[0];
    priv->frame->planes[1] = priv->info->display_fbuf->buf[1];
    priv->frame->planes[2] = priv->info->display_fbuf->buf[2];
    gavl_video_frame_copy(&(s->data.video.format), f, priv->frame);
    if(s->data.video.format.interlace_mode == GAVL_INTERLACE_MIXED)
      {
      if(priv->info->display_picture->flags & PIC_FLAG_PROGRESSIVE_FRAME)
        f->interlace_mode = GAVL_INTERLACE_NONE;
      else
        f->interlace_mode =
          (priv->info->display_picture->flags & PIC_FLAG_TOP_FIELD_FIRST) ?
          GAVL_INTERLACE_TOP_FIRST : GAVL_INTERLACE_BOTTOM_FIRST;
      }
    f->timestamp = priv->picture_timestamp;
    f->duration  = priv->picture_duration;

    if(!(s->flags & STREAM_STILL_MODE))
      priv->have_frame = 0;
    else
      {
      priv->picture_timestamp += priv->picture_duration;
      s->flags |= STREAM_STILL_SHOWN;
      }
    }

  if(((priv->info->display_picture->flags & PIC_MASK_CODING_TYPE) ==
      PIC_FLAG_CODING_TYPE_I) &&
     priv->has_gop_timecode)
    {
    s->codec_timecode = priv->gop_timecode;
    s->has_codec_timecode = 1;
    priv->has_gop_timecode = 0;
    }
  return 1;
  }


static int init_mpeg2(bgav_stream_t*s)
  {
  mpeg2_state_t state;
  mpeg2_priv_t * priv;
  
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;
  
  if(s->action == BGAV_STREAM_PARSE)
    {
    return 1;
    }

  priv->dec  = mpeg2_init();
  priv->info = mpeg2_info(priv->dec);
  priv->non_b_count = 0;
  while(1)
    {
    if(!parse(s, &state))
      return 0;
    if(state == STATE_SEQUENCE)
      break;
    }
  
  /* Get format */
  
  get_format(s, &s->data.video.format, priv->info->sequence);
  priv->frame = gavl_video_frame_create(NULL);

  priv->frame->strides[0] = priv->info->sequence->width;
  priv->frame->strides[1] = priv->info->sequence->chroma_width;
  priv->frame->strides[2] = priv->info->sequence->chroma_width;
  
  s->description =
    bgav_sprintf("MPEG-%d, %d kb/sec", 
                 (priv->info->sequence->flags & SEQ_FLAG_MPEG2) ? 2 : 1,
                 (priv->info->sequence->byte_rate * 8) / 1000);
  s->codec_bitrate = priv->info->sequence->byte_rate * 8;

  if(!s->timescale)
    s->timescale = s->data.video.format.timescale;
  
  while(1)
    {
    if(!parse(s, &state))
      return 0;
    if(state == STATE_PICTURE)
      break;
    }
  
  /* Decode first frame to check for still mode */
  
  priv->init = 1;
  decode_mpeg2(s, (gavl_video_frame_t*)0);
  priv->init = 0;
  s->out_time = priv->picture_timestamp;
  return 1;
  }

static void resync_mpeg2(bgav_stream_t*s)
  {
  mpeg2_priv_t * priv;
  bgav_packet_t * p;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);

  priv->p = (bgav_packet_t*)0;
  priv->non_b_count = 0;

  bgav_pts_cache_clear(&priv->pts_cache);
  
  if(s->flags & STREAM_STILL_MODE)
    {
    priv->picture_timestamp = priv->p->pts;
    }
  else
    {
    mpeg2_reset(priv->dec, 0);
    mpeg2_buffer(priv->dec, NULL, NULL);
    //  mpeg2_skip(priv->dec, 1);
    
    priv->have_frame = 0;
    while(1)
      {
      /* Skip pictures until we have the next keyframe */
      p = bgav_demuxer_peek_packet_read(s->demuxer, s, 1);
      if(!p)
        return;

      if(PACKET_GET_KEYFRAME(p))
        {
        s->out_time = p->pts;
        break;
        }
      /* Skip this packet */
      p = bgav_demuxer_get_packet_read(s->demuxer, s);
      bgav_demuxer_done_packet_read(s->demuxer, p);
      }
    }
  //  mpeg2_skip(priv->dec, 0);
  }
  
static void close_mpeg2(bgav_stream_t*s)
  {
  mpeg2_priv_t * priv;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);

  if(priv->frame)
    {
    gavl_video_frame_null(priv->frame);
    gavl_video_frame_destroy(priv->frame);
    }
  
  if(priv->dec)
    mpeg2_close(priv->dec);
  
  free(priv);

  }

static int skipto_mpeg2(bgav_stream_t * s, int64_t time)
  {
  mpeg2_priv_t * priv = s->data.video.decoder->priv;
  while(1)
    {
    /* TODO: Skip B-frames */
    if(!decode_picture(s))
      return 0;
    if(priv->picture_timestamp + priv->picture_duration > time)
      {
      s->out_time = priv->picture_timestamp;
      return 1;
      }
    }
  return 1;
  }

static bgav_video_decoder_t decoder =
  {
    .flags = VCODEC_FLAG_DELAY,
    .fourccs = (uint32_t[]){ /* Set by MPEG demuxers */
                             BGAV_MK_FOURCC('m','p','g','v'),
                             /* Quicktime fourccs */
                             BGAV_MK_FOURCC('h','d','v','1'), // HDV 720p30
                             BGAV_MK_FOURCC('h','d','v','2'), // 1080i60 25 Mbps CBR
                             BGAV_MK_FOURCC('h','d','v','3'), // 1080i50 25 Mbps CBR
                             BGAV_MK_FOURCC('h','d','v','5'), // HDV 720p25
                             BGAV_MK_FOURCC('h','d','v','6'), // 1080p24 25 Mbps CBR
                             BGAV_MK_FOURCC('h','d','v','7'), // 1080p25 25 Mbps CBR
                             BGAV_MK_FOURCC('h','d','v','8'), // 1080p30 25 Mbps CBR
                             BGAV_MK_FOURCC('x','d','v','1'), // XDCAM EX 720p30 VBR
                             BGAV_MK_FOURCC('x','d','v','2'), // XDCAM HD 1080i60 VBR
                             BGAV_MK_FOURCC('x','d','v','3'), // XDCAM HD 1080i50 VBR
                             BGAV_MK_FOURCC('x','d','v','4'), // XDCAM EX 720p24 VBR
                             BGAV_MK_FOURCC('x','d','v','5'), // XDCAM EX 720p25 VBR
                             BGAV_MK_FOURCC('x','d','v','6'), // XDCAM HD 1080p24 VBR
                             BGAV_MK_FOURCC('x','d','v','7'), // XDCAM HD 1080p25 VBR
                             BGAV_MK_FOURCC('x','d','v','8'), // XDCAM HD 1080p30 VBR
                             BGAV_MK_FOURCC('x','d','v','9'), // XDCAM EX 720p60 VBR
                             BGAV_MK_FOURCC('x','d','v','a'), // XDCAM EX 720p50 VBR
                             BGAV_MK_FOURCC('x','d','v','b'), // XDCAM EX 1080i60 VBR
                             BGAV_MK_FOURCC('x','d','v','c'), // XDCAM EX 1080i50 VBR
                             BGAV_MK_FOURCC('x','d','v','d'), // XDCAM EX 1080p24 VBR
                             BGAV_MK_FOURCC('x','d','v','e'), // XDCAM EX 1080p25 VBR
                             BGAV_MK_FOURCC('x','d','v','f'), // XDCAM EX 1080p30 VBR
                             0x00 },
    .name =    "libmpeg2 decoder",
    .init =    init_mpeg2,
    .decode =  decode_mpeg2,
    .close =   close_mpeg2,
    .resync =  resync_mpeg2,
    .skipto =  skipto_mpeg2,
  };

void bgav_init_video_decoders_libmpeg2()
  {
  bgav_video_decoder_register(&decoder);
  }
