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

#include <config.h>

#include <avdec_private.h>
#include <codecs.h>
#include <a52_header.h>

#include <a52dec/a52.h>

#define FRAME_SAMPLES 1536
#define MAX_FRAME_SIZE 3840

#define LOG_DOMAIN "audio_a52"

typedef struct
  {
  bgav_a52_header_t header;
  
  a52_state_t * state;
  sample_t    * samples;

  uint8_t * buffer;
  int bytes_in_buffer;
  
  bgav_packet_t * packet;
  uint8_t       * packet_ptr;
  gavl_audio_frame_t * frame;

  /* For parsing only */
  int64_t last_position;
  int buffer_size;
  int buffer_alloc;
  
  } a52_priv;


/*
 *  Parse header and read data bytes for that frame
 *  (also does resync)
 */

static int get_data(bgav_stream_t * s, int num_bytes)
  {
  int bytes_to_copy;
  a52_priv * priv;
  priv = s->data.audio.decoder->priv;
  
  while(priv->bytes_in_buffer < num_bytes)
    {
    if(!priv->packet)
      {
      priv->packet = bgav_demuxer_get_packet_read(s->demuxer, s);
      
      if(!priv->packet)
        return 0;
      //      fprintf(stderr, "Got packet:\n");
      //      bgav_hexdump(priv->packet->data, 16, 16);
      priv->packet_ptr = priv->packet->data;
      }
    else if(priv->packet_ptr - priv->packet->data >= priv->packet->data_size)
      {
      bgav_demuxer_done_packet_read(s->demuxer, priv->packet);
      priv->packet = bgav_demuxer_get_packet_read(s->demuxer, s);
      if(!priv->packet)
        return 0;

      priv->packet_ptr = priv->packet->data;
      }
    bytes_to_copy = num_bytes - priv->bytes_in_buffer;
    
    if(bytes_to_copy >
       priv->packet->data_size - (priv->packet_ptr - priv->packet->data))
      {
      bytes_to_copy =
        priv->packet->data_size - (priv->packet_ptr - priv->packet->data);
      }
    memcpy(priv->buffer + priv->bytes_in_buffer,
           priv->packet_ptr, bytes_to_copy);
    priv->bytes_in_buffer += bytes_to_copy;
    priv->packet_ptr      += bytes_to_copy;
    }

  return 1;
  }

static void done_data(bgav_stream_t * s, int num_bytes)
  {
  a52_priv * priv;
  int bytes_left;
  
  priv = s->data.audio.decoder->priv;

  bytes_left = priv->bytes_in_buffer - num_bytes;
  
  if(bytes_left < 0)
    {
    return;
    }
  else if(bytes_left > 0)
    {
    memmove(priv->buffer,
            priv->buffer + num_bytes,
            bytes_left);
    }
  priv->bytes_in_buffer -= num_bytes;
  }

static int do_resync(bgav_stream_t * s)
  {
  a52_priv * priv;
  //  int skipped = 0;
  priv = s->data.audio.decoder->priv;
  
  while(1)
    {
    if(!get_data(s, BGAV_A52_HEADER_BYTES))
      return 0;
    if(bgav_a52_header_read(&(priv->header), priv->buffer))
      return 1;
    done_data(s, 1);
    //    skipped++;
    }
  //  if(skipped)
  //    fprintf(stderr, "A52: Skipped %d bytes\n", skipped);
  return 0;
  }

static void resync_a52(bgav_stream_t * s)
  {
  a52_priv * priv;
  priv = s->data.audio.decoder->priv;

  priv->packet = (bgav_packet_t*)0;
  priv->packet_ptr = (uint8_t*)0;
  priv->bytes_in_buffer = 0;
  do_resync(s);
  }

static int init_a52(bgav_stream_t * s)
  {
  a52_priv * priv;
  
  priv = calloc(1, sizeof(*priv));
  priv->buffer = calloc(MAX_FRAME_SIZE, 1);
  s->data.audio.decoder->priv = priv;

  if(s->action == BGAV_STREAM_PARSE)
    {
    priv->last_position = -1;
    return 1;
    }


  if(!do_resync(s))
    {
    return 0;
    }
  //  a52_header_dump(&(priv->header));
    
  /* Get format */

  s->data.audio.format.samplerate = priv->header.samplerate;
  s->codec_bitrate = priv->header.bitrate;
  s->data.audio.format.samples_per_frame = FRAME_SAMPLES;
  s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
  
  if(priv->header.lfe)
    {
    s->data.audio.format.num_channels = 1;
    s->data.audio.format.channel_locations[0] = GAVL_CHID_LFE;
    }
  else
    s->data.audio.format.num_channels = 0;
  
  switch(priv->header.acmod)
    {
    case A52_CHANNEL:
    case A52_STEREO:
      
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+1] = 
        GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.num_channels += 2;
      break;
    case A52_MONO:
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT_CENTER;
      s->data.audio.format.num_channels += 1;
      break;
    case A52_3F:
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+1] = 
        GAVL_CHID_FRONT_CENTER;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+2] = 
        GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.num_channels += 3;
      break;
    case A52_2F1R:
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+1] = 
        GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+2] = 
        GAVL_CHID_REAR_CENTER;
      s->data.audio.format.num_channels += 3;
      break;
    case A52_3F1R:
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+1] = 
        GAVL_CHID_FRONT_CENTER;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+2] = 
        GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+3] = 
        GAVL_CHID_REAR_CENTER;
      s->data.audio.format.num_channels += 4;

      break;
    case A52_2F2R:
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+1] = 
        GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+2] = 
        GAVL_CHID_REAR_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+3] = 
        GAVL_CHID_REAR_RIGHT;
      s->data.audio.format.num_channels += 4;
      break;
    case A52_3F2R:
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+1] = 
        GAVL_CHID_FRONT_CENTER;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+2] = 
        GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+3] = 
        GAVL_CHID_REAR_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+4] = 
        GAVL_CHID_REAR_RIGHT;
      s->data.audio.format.num_channels += 5;
      break;
    }

  if(gavl_front_channels(&(s->data.audio.format)) == 3)
    s->data.audio.format.center_level = priv->header.cmixlev;
  if(gavl_rear_channels(&(s->data.audio.format)))
    s->data.audio.format.rear_level = priv->header.smixlev;

  priv->frame = gavl_audio_frame_create(&(s->data.audio.format));
  priv->state = a52_init(0);
  priv->samples = a52_samples(priv->state);
  s->description =
    bgav_sprintf("AC3 %d kb/sec", priv->header.bitrate/1000);
  
  return 1;
  }

static int decode_frame(bgav_stream_t * s)
  {
  int flags;
  int sample_rate;
  int bit_rate;
  int i, j;
#ifdef LIBA52_DOUBLE
  int k;
#endif
  
  sample_t level = 1.0;
  
  a52_priv * priv;
  priv = s->data.audio.decoder->priv;

  if(!do_resync(s))
    return 0;
  if(!get_data(s, priv->header.total_bytes))
    return 0;

  /* Now, decode this */
  
  if(!a52_syncinfo(priv->buffer, &flags,
                   &sample_rate, &bit_rate))
    return 0;

  a52_frame(priv->state, priv->buffer, &flags,
            &level, 0.0);
  if(!s->opt->audio_dynrange)
    {
    a52_dynrng(priv->state, NULL, NULL);
    }
  
  for(i = 0; i < 6; i++)
    {
    a52_block (priv->state);

    for(j = 0; j < s->data.audio.format.num_channels; j++)
      {
#ifdef LIBA52_DOUBLE
      for(k = 0; k < 256; k++)
        priv->frame->channels.f[i][k][i*256] = priv->samples[j*256 + k];
        
#else
      memcpy(&(priv->frame->channels.f[j][i*256]),
             &(priv->samples[j * 256]),
             256 * sizeof(float));
#endif
      }
    }
  done_data(s, priv->header.total_bytes);
  
  priv->frame->valid_samples = FRAME_SAMPLES;
  return 1;
  }

static int decode_a52(bgav_stream_t * s,
                      gavl_audio_frame_t * f, int num_samples)
  {
  int samples_decoded = 0;
  int samples_copied;
  a52_priv * priv;
  priv = s->data.audio.decoder->priv;

  while(samples_decoded < num_samples)
    {
    if(!priv->frame->valid_samples)
      {
      if(!decode_frame(s))
        {
        if(f)
          f->valid_samples = samples_decoded;
        return samples_decoded;
        }
      }
    samples_copied =
      gavl_audio_frame_copy(&(s->data.audio.format),
                            f,
                            priv->frame,
                            samples_decoded, /* out_pos */
                            FRAME_SAMPLES -
                            priv->frame->valid_samples,  /* in_pos */
                            num_samples - samples_decoded, /* out_size, */
                            priv->frame->valid_samples /* in_size */);
    priv->frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  if(f)
    {
    f->valid_samples = samples_decoded;
    }
  return samples_decoded;
  }

static void close_a52(bgav_stream_t * s)
  {
  a52_priv * priv;
  priv = s->data.audio.decoder->priv;

  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);
  if(priv->buffer)
    free(priv->buffer);
  if(priv->state)
    a52_free(priv->state);
  free(priv);
  }
#if 1
static void parse_a52(bgav_stream_t * s)
  {
  bgav_packet_t * p;
  uint8_t * ptr;
  int old_buffer_size;
  int size_needed;

  a52_priv * priv;
  priv = s->data.audio.decoder->priv;
  
  while(bgav_demuxer_peek_packet_read(s->demuxer, s, 0))
    {
    /* Get the packet and append data to the buffer */
    p = bgav_demuxer_get_packet_read(s->demuxer, s);

    
    //    fprintf(stderr, "Got packet:\n");
    //    bgav_hexdump(p->data, 16, 16);
    
    old_buffer_size = priv->buffer_size;

    if(priv->buffer_size < 0)
      size_needed = priv->buffer_size;
    else
      size_needed = priv->buffer_size + p->data_size;
        
    if(priv->buffer_alloc < priv->buffer_size + p->data_size)
      {
      priv->buffer_alloc = priv->buffer_size + p->data_size + 1024;
      priv->buffer = realloc(priv->buffer, priv->buffer_alloc);
      }

    if(old_buffer_size < 0)
      {
      memcpy(priv->buffer, p->data, p->data_size);
      priv->buffer_size += p->data_size;
      
      ptr = priv->buffer - old_buffer_size;
      }
    else
      {
      memcpy(priv->buffer + priv->buffer_size, p->data, p->data_size);
      priv->buffer_size += p->data_size;
      
      ptr = priv->buffer;
      }
    
    while(priv->buffer_size >= BGAV_A52_HEADER_BYTES)
      {
      //      fprintf(stderr, "decode header: ");
      //      bgav_hexdump(ptr, 16, 16);

      if(!bgav_a52_header_read(&(priv->header), ptr))
        {
        bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Lost sync during parsing");
        return;
        }
      s->data.audio.format.samplerate = priv->header.samplerate;
      
      /* If frame starts in the previous packet,
         use the previous index */
      if(ptr - priv->buffer < old_buffer_size)
        {
        //        fprintf(stderr, "Append packet (prev)\n");
        bgav_file_index_append_packet(s->file_index,
                                      priv->last_position,
                                      s->duration,
                                      1);
        }
      else
        {
        //        fprintf(stderr, "Append packet (this)\n");
        bgav_file_index_append_packet(s->file_index,
                                      p->position,
                                      s->duration,
                                      1);
        }
      s->duration += FRAME_SAMPLES;
      ptr += priv->header.total_bytes;
      priv->buffer_size -= priv->header.total_bytes;
      }
    if(priv->buffer_size > 0)
      memmove(priv->buffer, ptr, priv->buffer_size);
    priv->last_position = p->position;
    bgav_demuxer_done_packet_read(s->demuxer, p);
    //    fprintf( stderr, "Done with packet, %d bytes\n",
    //             priv->buffer_size);
    }
  
  }
#endif

static bgav_audio_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x2000),
                           BGAV_MK_FOURCC('.', 'a', 'c', '3'),
                           /* Will be swapped to AC3 by the demuxer */
                           BGAV_MK_FOURCC('d', 'n', 'e', 't'), 
                           0x00 },
    .name = "liba52 based decoder",

    .init   = init_a52,
    .decode = decode_a52,
    .parse  = parse_a52,
    .close  = close_a52,
    .resync = resync_a52,
  };

void bgav_init_audio_decoders_a52()
  {
  bgav_audio_decoder_register(&decoder);
  }
