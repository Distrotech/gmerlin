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
// #include <dca_header.h>

#include <dts.h>

#define BLOCK_SAMPLES 256

typedef struct
  {
  dts_state_t * state;
  sample_t    * samples;

  uint8_t * buffer;
  int bytes_in_buffer;
  int buffer_alloc;

  bgav_packet_t * packet;
  uint8_t       * packet_ptr;
  gavl_audio_frame_t * frame;
  int blocks_left;
  int frame_length;
  } dts_priv;

#define MAX_FRAME_SIZE 3840

/*
 *  Parse header and read data bytes for that frame
 *  (also does resync)
 */

static int get_data(bgav_stream_t * s, int num_bytes)
  {
  int bytes_to_copy;
  dts_priv * priv;
  priv = (dts_priv*)(s->data.audio.decoder->priv);

  if(priv->buffer_alloc < num_bytes)
    {
    priv->buffer_alloc = num_bytes + 1024;
    priv->buffer = realloc(priv->buffer, priv->buffer_alloc);
    }
  
  while(priv->bytes_in_buffer < num_bytes)
    {
    if(!priv->packet)
      {
      priv->packet = bgav_demuxer_get_packet_read(s->demuxer, s);
      if(!priv->packet)
        return 0;
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
  dts_priv * priv;
  int bytes_left;
  
  priv = (dts_priv*)(s->data.audio.decoder->priv);

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

static int do_resync(bgav_stream_t * s, int * flags,
                     int * sample_rate, int * bit_rate, int * frame_length)
  {
  int dummy;
  dts_priv * priv;
  priv = (dts_priv*)(s->data.audio.decoder->priv);

  while(1)
    {
    if(!get_data(s, 14))
      return 0;
    if((*frame_length = dts_syncinfo(priv->state, priv->buffer,
                                     flags, sample_rate, bit_rate, &dummy)))
      {
      return 1;
      }
    done_data(s, 1);
    }
  return 0;
  }

static void resync_dts(bgav_stream_t * s)
  {
  int flags, sample_rate, bit_rate, frame_length;
  dts_priv * priv;
  priv = (dts_priv*)(s->data.audio.decoder->priv);

  priv->packet = (bgav_packet_t*)0;
  priv->packet_ptr = (uint8_t*)0;
  priv->bytes_in_buffer = 0;
  priv->blocks_left = 0;
  do_resync(s, &flags, &sample_rate, &bit_rate, &frame_length);
  }

static int init_dts(bgav_stream_t * s)
  {
  int flags, sample_rate, bit_rate, frame_length;
  dts_priv * priv;
  
  priv = calloc(1, sizeof(*priv));
  priv->buffer = calloc(MAX_FRAME_SIZE, 1);
  priv->state = dts_init(0);
  
  s->data.audio.decoder->priv = priv;
  if(!do_resync(s, &flags, &sample_rate, &bit_rate, &frame_length))
    {
    return 0;
    }
  //  dts_header_dump(&(priv->header));
    
  /* Get format */

  s->data.audio.format.samplerate = sample_rate;
  s->data.audio.format.samples_per_frame = BLOCK_SAMPLES;
  s->codec_bitrate = bit_rate;
  s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
  
  switch(flags & DTS_CHANNEL_MASK)
    {
    case DTS_CHANNEL: // Dual mono. Two independant mono channels.
      s->data.audio.format.num_channels = 2;
      s->data.audio.format.channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      break;
    case DTS_MONO: // Mono.
      s->data.audio.format.num_channels = 1;
      s->data.audio.format.channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      break;
    case DTS_STEREO: // Stereo.
    case DTS_DOLBY: // Dolby surround compatible stereo.
      s->data.audio.format.num_channels = 2;
      s->data.audio.format.channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      break;
    case DTS_3F: // 3 front channels (left, center, right)
      s->data.audio.format.num_channels = 3;
      s->data.audio.format.channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      s->data.audio.format.channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      break;
    case DTS_2F1R: // 2 front, 1 rear surround channel (L, R, S)
      s->data.audio.format.num_channels = 3;
      s->data.audio.format.channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.channel_locations[2] = GAVL_CHID_REAR_CENTER;
      break;
    case DTS_3F1R: // 3 front, 1 rear surround channel (L, C, R, S)
      s->data.audio.format.num_channels = 4;
      s->data.audio.format.channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      s->data.audio.format.channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.channel_locations[3] = GAVL_CHID_REAR_CENTER;
      break;
    case DTS_2F2R: // 2 front, 2 rear surround channels (L, R, LS, RS)
      s->data.audio.format.num_channels = 4;
      s->data.audio.format.channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.channel_locations[2] = GAVL_CHID_REAR_LEFT;
      s->data.audio.format.channel_locations[3] = GAVL_CHID_REAR_RIGHT;
      break;
    case DTS_3F2R: // 3 front, 2 rear surround channels (L, C, R, LS, RS)
      s->data.audio.format.num_channels = 5;
      s->data.audio.format.channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      s->data.audio.format.channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.channel_locations[3] = GAVL_CHID_REAR_LEFT;
      s->data.audio.format.channel_locations[4] = GAVL_CHID_REAR_RIGHT;
      break;
    }

  if(flags & DTS_LFE)
    {
    s->data.audio.format.channel_locations[s->data.audio.format.num_channels] =
      GAVL_CHID_LFE;
    s->data.audio.format.num_channels++;
    }
#if 0  
  if(gavl_front_channels(&(s->data.audio.format)) == 3)
    s->data.audio.format.center_level = priv->header.cmixlev;
  if(gavl_rear_channels(&(s->data.audio.format)))
    s->data.audio.format.rear_level = priv->header.smixlev;
#endif
  priv->frame = gavl_audio_frame_create(&(s->data.audio.format));
  priv->samples = dts_samples(priv->state);
  s->description =
    bgav_sprintf("DTS %d kb/sec", bit_rate/1000);
  
  return 1;
  }

static int decode_block(bgav_stream_t * s)
  {
  int flags;
  int sample_rate;
  int bit_rate;
  int j;
#ifdef LIBDTS_DOUBLE
  int k;
#endif
  
  sample_t level = 1.0;
  
  dts_priv * priv;
  priv = (dts_priv*)s->data.audio.decoder->priv;

  if(!priv->blocks_left)
    {
    if(!do_resync(s, &flags,
                  &sample_rate, &bit_rate, &priv->frame_length))
      return 0;
    if(!get_data(s, priv->frame_length))
      return 0;
    
    /* Now, decode this */
    
    dts_frame(priv->state, priv->buffer, &flags,
              &level, 0.0);
    
    if(!s->opt->audio_dynrange)
      dts_dynrng(priv->state, NULL, NULL);
    priv->blocks_left = dts_blocks_num(priv->state);
    }
  
  dts_block (priv->state);

  for(j = 0; j < s->data.audio.format.num_channels; j++)
    {
#ifdef LIBDTS_DOUBLE
    for(k = 0; k < 256; k++)
      priv->frame->channels.f[i][k][i*256] = priv->samples[j*256 + k];
    
#else
    memcpy(priv->frame->channels.f[j],
           &(priv->samples[j * 256]),
           256 * sizeof(float));
#endif
    }
  priv->blocks_left--;
  if(!priv->blocks_left)
    done_data(s, priv->frame_length);
  
  priv->frame->valid_samples = BLOCK_SAMPLES;
  return 1;
  }

static int decode_dts(bgav_stream_t * s,
                      gavl_audio_frame_t * f, int num_samples)
  {
  int samples_decoded = 0;
  int samples_copied;
  dts_priv * priv;
  priv = (dts_priv*)s->data.audio.decoder->priv;

  while(samples_decoded < num_samples)
    {
    if(!priv->frame->valid_samples)
      {
      if(!decode_block(s))
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
                            BLOCK_SAMPLES -
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

static void close_dts(bgav_stream_t * s)
  {
  dts_priv * priv;
  priv = (dts_priv*)s->data.audio.decoder->priv;

  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);
  if(priv->buffer)
    free(priv->buffer);
  if(priv->state)
    dts_free(priv->state);
  free(priv);
  }

static bgav_audio_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('d', 't', 's', ' '),
                           0x00 },
    name: "libdca based decoder",

    init:   init_dts,
    decode: decode_dts,
    close:  close_dts,
    resync: resync_dts,
  };

void bgav_init_audio_decoders_dca()
  {
  bgav_audio_decoder_register(&decoder);
  }
