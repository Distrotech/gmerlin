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
#include <mpv_header.h>

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

  int intra_slice_refresh;
  
  int do_resync;
  
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

  int64_t stream_time;
  
  /* For parsing */
  uint8_t * buffer;
  int buffer_alloc;
  int buffer_size;
  bgav_mpv_sequence_header_t sh;
  int64_t last_position;
  int last_coding_type; /* Last picture type is saved here */
  int64_t last_pts;
  
  } mpeg2_priv_t;

static int get_data(bgav_stream_t*s)
  {
  mpeg2_priv_t * priv;
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
      return 1;
      }
    else
      return 0;
    }
  priv->eof = 0;
  mpeg2_buffer(priv->dec, priv->p->data, priv->p->data + priv->p->data_size);
  
  if(priv->p->pts != BGAV_TIMESTAMP_UNDEFINED)
    {
    mpeg2_tag_picture(priv->dec,
                      (priv->p->pts) >> 32,
                      priv->p->pts & 0xffffffff);
    if(priv->do_resync)
      {
      priv->picture_timestamp =
        gavl_time_rescale(s->timescale,
                          s->data.video.format.timescale,
                          priv->p->pts);
      priv->stream_time = priv->p->pts;
      }
    }
  
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
      }
    else
      return 1;
    }
  }

static void get_format(bgav_stream_t*s,
                       gavl_video_format_t * ret,
                       const mpeg2_sequence_t * sequence)
  {
  mpeg2_priv_t * priv;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);
  
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
  
  if(sequence->flags & SEQ_FLAG_MPEG2)
    ret->framerate_mode = GAVL_FRAMERATE_VARIABLE;
  else /* MPEG-1 is always constant framerate */
    {
    ret->timescale /= 2;
    ret->frame_duration /= 2;
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
  int64_t tmp;
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
#if 0
      fprintf(stderr, "Got Picture: C: %c D: %c\n",
              picture_types[priv->info->current_picture ? 
              priv->info->current_picture->flags & PIC_MASK_CODING_TYPE : 0],
              picture_types[(priv->info->display_picture) ?
              priv->info->display_picture->flags & PIC_MASK_CODING_TYPE : 0]);
#endif
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
            //            else
            //              fprintf(stderr, "Old B-Frame\n");
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
    s->data.video.still_mode = 1;
    // s->data.video.format.framerate_mode = GAVL_FRAMERATE_STILL;
    }

  /* Calculate timestamp */
    
  if(priv->info->display_picture->flags & PIC_FLAG_TAGS)
    {
    tmp = priv->info->display_picture->tag;
    tmp <<= 32;
    tmp |= priv->info->display_picture->tag2;
    priv->picture_timestamp = gavl_time_rescale(s->timescale, s->data.video.format.timescale, tmp);
    }
  else
    {
    priv->picture_timestamp += priv->picture_duration;
    }
  /* Get this pictures duration */
  
  priv->picture_duration = s->data.video.format.frame_duration;
  if(priv->info->display_picture->nb_fields > 2)
    {
    priv->picture_duration =
      (priv->picture_duration * priv->info->display_picture->nb_fields) / 2;
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
  if(s->data.video.still_mode && (s->demuxer->flags & BGAV_DEMUXER_EOF))
    return 0;
  
  if((!s->data.video.still_mode && !priv->have_frame) ||
     (s->data.video.still_mode &&
      bgav_demuxer_peek_packet_read(s->demuxer, s, 0)))
    {
    if(!decode_picture(s))
      return 0;
    priv->have_frame = 1;
    }
  s->out_time                       = priv->picture_timestamp;
  s->data.video.next_frame_duration = priv->picture_duration;
  
  if(priv->init)
    return 1;
#if 0
  if(!f)
    fprintf(stderr, "Skip: %d\n",
            priv->info->current_picture->flags & PIC_MASK_CODING_TYPE);
#endif
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
#if 0 // Don't set anything for progressive mode!!
    else
      f->interlace_mode = GAVL_INTERLACE_NONE;
#endif
    }
  s->data.video.last_frame_time     = priv->picture_timestamp;
  s->data.video.last_frame_duration = priv->picture_duration;

#if 0
  fprintf(stderr, "PTS: %ld, T: %c\n", 
          s->data.video.last_frame_time,
          picture_types[priv->info->display_picture->flags & PIC_MASK_CODING_TYPE]);
#endif
  if(!s->data.video.still_mode)
    {
    priv->have_frame = 0;
    }
  else
    priv->picture_timestamp += priv->picture_duration;
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
  /*
   *  If the first frame after a sequence header is a P-Frame,
   *  we have most likely an
   *  intra slice refresh stream
   */

  if((priv->info->current_picture->flags & PIC_MASK_CODING_TYPE) ==
     PIC_FLAG_CODING_TYPE_P)
    {
    bgav_log(s->opt, BGAV_LOG_DEBUG, LOG_DOMAIN,
             "Detected Intra slice refresh");
    priv->intra_slice_refresh = 1;
    }

  /* Decode first frame to check for still mode */
  
  priv->init = 1;
  decode_mpeg2(s, (gavl_video_frame_t*)0);
  priv->init = 0;
  s->data.video.frametime_mode = BGAV_FRAMETIME_CODEC;
  return 1;
  }

static void resync_mpeg2(bgav_stream_t*s)
  {
  mpeg2_state_t state;
  mpeg2_priv_t * priv;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);

  priv->p = (bgav_packet_t*)0;
  priv->non_b_count = 0;
  
  if(s->data.video.still_mode)
    {
    priv->picture_timestamp = gavl_time_rescale(s->timescale,
                                                s->data.video.format.timescale,
                                                priv->stream_time);
    }
  else
    {
    mpeg2_reset(priv->dec, 0);
    mpeg2_buffer(priv->dec, NULL, NULL);
    //  mpeg2_skip(priv->dec, 1);
    
    priv->have_frame = 0;
    priv->do_resync = 1;
    while(1)
      {
      /* Get the next picture header */
      while(1)
        {
        if(!parse(s, &state))
          return;
        if(state == STATE_PICTURE)
          break;
        }
      //      fprintf(stderr, "Parse: %d\n",
      //              priv->info->current_picture->flags & PIC_MASK_CODING_TYPE);
      
      /* Check if we can start decoding again */
      if((priv->intra_slice_refresh)  &&
         ((priv->info->current_picture->flags & PIC_MASK_CODING_TYPE) ==
          PIC_FLAG_CODING_TYPE_P))
        {
        //        priv->non_b_count++;
        break;
        }
      else if(priv->info->current_picture &&
              ((priv->info->current_picture->flags & PIC_MASK_CODING_TYPE) ==
               PIC_FLAG_CODING_TYPE_I))
        {
        //        priv->non_b_count++;
        break;
        }
      }
    priv->do_resync = 0;
    }
  //  mpeg2_skip(priv->dec, 0);

  /* Set next timestamps */
  if(s->demuxer->demux_mode == DEMUX_MODE_FI)
    priv->picture_timestamp = s->out_time;
  else
    s->out_time = priv->picture_timestamp;

  s->data.video.next_frame_duration = s->data.video.format.frame_duration;

  if(!s->data.video.still_mode)
    {
    if((priv->info->current_picture->flags & PIC_FLAG_TOP_FIELD_FIRST) &&
       (priv->info->current_picture->nb_fields > 2))
      {
      s->data.video.next_frame_duration =
        (s->data.video.next_frame_duration * priv->info->current_picture->nb_fields) / 2;
      }
    }
  
  
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
  if(priv->buffer)
    free(priv->buffer);
  
  if(priv->dec)
    mpeg2_close(priv->dec);
  
  free(priv);

  }

static void parse_libmpeg2(bgav_stream_t * s)
  {
  bgav_packet_t * p;
  mpeg2_priv_t * priv;
  int old_buffer_size;
  int result;
  uint8_t * ptr;
  bgav_mpv_picture_header_t ph;
  int keyframe;
  int duration;
  priv = (mpeg2_priv_t*)(s->data.video.decoder->priv);
  memset(&ph, 0, sizeof(ph));
  
  while(bgav_demuxer_peek_packet_read(s->demuxer, s, 0))
    {
    p = bgav_demuxer_get_packet_read(s->demuxer, s);
    old_buffer_size = priv->buffer_size;

    if(priv->buffer_alloc < priv->buffer_size + p->data_size)
      {
      priv->buffer_alloc = priv->buffer_size + p->data_size + 1024;
      priv->buffer = realloc(priv->buffer, priv->buffer_alloc);
      }
    
    memcpy(priv->buffer + priv->buffer_size, p->data, p->data_size);
    priv->buffer_size += p->data_size;
    ptr = priv->buffer;
    
    while(priv->buffer_size > MPV_PROBE_SIZE)
      {
      /* Check for sequence header */
      if(!s->data.video.format.timescale &&
         bgav_mpv_sequence_header_probe(ptr))
        {
        result =
          bgav_mpv_sequence_header_parse(s->opt,
                                         &priv->sh,
                                         ptr, priv->buffer_size);
        if(!result) /* Out of data */
          break;
        else if(result > 0)
          {
          s->data.video.format.timescale      = priv->sh.timescale;
          s->data.video.format.frame_duration = priv->sh.frame_duration;
          ptr               += result;
          priv->buffer_size -= result;
          }
        else
          return; /* Error */
        }
      /* Check for sequence extension. We parse only the first one */
      else if(s->data.video.format.timescale &&
              !priv->sh.mpeg2 &&
              bgav_mpv_sequence_extension_probe(ptr))
        {
        result =
          bgav_mpv_sequence_extension_parse(s->opt,
                                            &priv->sh.ext,
                                            ptr, priv->buffer_size);
        if(!result) /* Out of data */
          break;
        else if(result > 0)
          {
          priv->sh.mpeg2 = 1;
          s->data.video.format.timescale      *= (priv->sh.ext.timescale_ext+1);
          s->data.video.format.frame_duration *= (priv->sh.ext.frame_duration_ext+1);
          ptr               += result;
          priv->buffer_size -= result;
          s->data.video.format.timescale      *= 2;
          s->data.video.format.frame_duration *= 2;
          }
        else
          return; /* Error */
        }
      /* Picture header (we skip pictures before the first sequence header) */
      else if(s->data.video.format.timescale &&
              bgav_mpv_picture_header_probe(ptr))
        {
        result =
          bgav_mpv_picture_header_parse(s->opt,
                                        &ph,
                                        ptr, priv->buffer_size);
        if(!result) /* Out of data */
          break;
        else if(result > 0)
          {
          //          fprintf(stderr, "Got picture header %c\n",
          //                  picture_types[ph.coding_type]);
          /* First frame after sequence header is a P-Frame */
          if(!s->duration && ph.coding_type == MPV_CODING_TYPE_P)
            priv->intra_slice_refresh = 1;

          /*
           * We don't generate index entries for B-frames. Instead,
           * we add the B-frame duration to the pts of the last non-Bframe.
           * This makes frame-refordering unneccesary and produces an Index
           * with strictly monotonous timestamps, so we can do a binary search
           * afterwards.
           */
          
          if(ph.coding_type != MPV_CODING_TYPE_B)
            {
            if(priv->intra_slice_refresh)
              keyframe = (ph.coding_type == MPV_CODING_TYPE_P);
            else
              keyframe = (ph.coding_type == MPV_CODING_TYPE_I);

            /* Frame started in the last packet */
            if(ptr - priv->buffer < old_buffer_size)
              {
              bgav_file_index_append_packet(s->file_index,
                                            priv->last_position,
                                            s->duration,
                                            keyframe);
              if(!s->duration && (priv->last_pts != BGAV_TIMESTAMP_UNDEFINED))
                s->first_timestamp = priv->last_pts;
              }
            else
              {
              bgav_file_index_append_packet(s->file_index,
                                            p->position,
                                            s->duration,
                                            keyframe);
              if(!s->duration && (p->pts != BGAV_TIMESTAMP_UNDEFINED))
                s->first_timestamp = p->pts;
              }
            priv->non_b_count++;
            s->duration += s->data.video.format.frame_duration;
            }
          else
            {
            if(priv->non_b_count >= 2)
              {
              s->duration += s->data.video.format.frame_duration;
              s->file_index->entries[s->file_index->num_entries-1].time +=
                s->data.video.format.frame_duration;
              }
            }
          
          ptr               += result;
          priv->buffer_size -= result;

          priv->last_coding_type = ph.coding_type;
          }
        else
          return; /* Error */
        }
      /* Picture coding extension */
      else if(s->data.video.format.timescale &&
              bgav_mpv_picture_extension_probe(ptr))
        {
        result =
          bgav_mpv_picture_extension_parse(s->opt,
                                           &ph.ext,
                                           ptr, priv->buffer_size);
        if(!result) /* Out of data */
          break;
        else if(result > 0)
          {
          //          fprintf(stderr, "Got picture coding extension\n");
          duration = 0;
          if(ph.ext.repeat_first_field)
            {
            if(priv->sh.ext.progressive_sequence)
              {
              if(ph.ext.top_field_first)
                duration = s->data.video.format.frame_duration * 2;
              else
                duration = s->data.video.format.frame_duration;
              }
            else if(ph.ext.progressive_frame)
              duration = s->data.video.format.frame_duration / 2;
            
            if(priv->last_coding_type == MPV_CODING_TYPE_B)
              {
              if(priv->non_b_count >= 2)
                {
                s->duration += duration;
                s->file_index->entries[s->file_index->num_entries-1].time +=
                  duration;
                }
              }
            else
              s->duration += duration;
            
            }
          ptr               += result;
          priv->buffer_size -= result;
          }
        else
          return; /* Error */
        }
      else /* Advance by one byte */
        {
        ptr++;
        priv->buffer_size--;
        }
      }
    if(priv->buffer_size > 0)
      memmove(priv->buffer, ptr, priv->buffer_size);
    priv->last_position = p->position;
    bgav_demuxer_done_packet_read(s->demuxer, p);
    }
  }

static bgav_video_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('m','p','g','v'), 0x00 },
    .name =    "libmpeg2 decoder",

    .init =    init_mpeg2,
    .decode =  decode_mpeg2,
    .close =   close_mpeg2,
    .resync =  resync_mpeg2,
    .parse =   parse_libmpeg2,
  };

void bgav_init_video_decoders_libmpeg2()
  {
  bgav_video_decoder_register(&decoder);
  }
