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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mad.h>

#include <config.h>
#include <avdec_private.h>
#include <codecs.h>
#include <mpa_header.h>

#define LOG_DOMAIN "mad"

/* Bytes needed to decode a complete header for parsing */
#define HEADER_SIZE 4
//#define MAX_FRAME_BYTES 2881


typedef struct
  {
  struct mad_stream stream;
  struct mad_frame frame;
  struct mad_synth synth;

  uint8_t * buffer;
  int buffer_alloc;
  int buffer_size; /* Used only for parsing */

  gavl_audio_frame_t * audio_frame;
  
  int do_init;
  int64_t last_position;
  int eof; /* For decoding the very last frame */
  
  int partial; /* Partial frame is left in the buffer.
                  Either we can decode this (layer 3) or
                  we'll mute it. */
  } mad_priv_t;

static int get_data(bgav_stream_t * s)
  {
  bgav_packet_t * p;
  mad_priv_t * priv;
  int bytes_in_buffer;
  
  priv = s->data.audio.decoder->priv;
  
  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    {
    if(!priv->eof)
      {
      /* Append zeros to the end so we can decode the very last
         frame */
      bytes_in_buffer = (int)(priv->stream.bufend - priv->stream.next_frame);
      if(!bytes_in_buffer)
        {
        priv->eof = 1;
        return 0;
        }

      memmove(priv->buffer,
              priv->buffer +
              (int)(priv->stream.next_frame - priv->stream.buffer),
              bytes_in_buffer);
      //      fprintf(stderr, "Decoding last frame %d\n", bytes_in_buffer);
      priv->partial = 1;
      if(priv->buffer_alloc < MAD_BUFFER_GUARD + bytes_in_buffer)
        {
        priv->buffer_alloc = MAD_BUFFER_GUARD + bytes_in_buffer;
        priv->buffer = realloc(priv->buffer, priv->buffer_alloc);
        }
      memset(priv->buffer + bytes_in_buffer, 0,
             MAD_BUFFER_GUARD);
      
      mad_stream_buffer(&(priv->stream), priv->buffer,
                        bytes_in_buffer + MAD_BUFFER_GUARD);
      
      priv->eof = 1;
      return 1;
      }
    else
      return 0;
    }

  bytes_in_buffer = (int)(priv->stream.bufend - priv->stream.next_frame);
  
  if(priv->buffer_alloc < p->data_size + bytes_in_buffer)
    {
    priv->buffer_alloc = p->data_size + bytes_in_buffer + 32;
    priv->buffer = realloc(priv->buffer, priv->buffer_alloc);
    }

  if(bytes_in_buffer)
    memmove(priv->buffer,
            priv->buffer +
            (int)(priv->stream.next_frame - priv->stream.buffer),
            bytes_in_buffer);
  
  memcpy(priv->buffer + bytes_in_buffer, p->data, p->data_size);
  
  mad_stream_buffer(&(priv->stream), priv->buffer,
                    p->data_size + bytes_in_buffer);
    
  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }


static int decode_frame(bgav_stream_t * s)
  {
  mad_priv_t * priv;
  const char * version_string;
  char * bitrate_string;
  int i, j, done;
  priv = s->data.audio.decoder->priv;
  
  /* Check if we need new data */
  if(priv->stream.bufend - priv->stream.next_frame <= MAD_BUFFER_GUARD)
    if(!get_data(s))
      {
      return 0;
      }
  done = 0;
  while(mad_frame_decode(&(priv->frame), &(priv->stream)) == -1)
    {
    switch(priv->stream.error)
      {
      case MAD_ERROR_BUFLEN:
        if(priv->partial)
          {
          mad_frame_mute(&priv->frame);
          priv->partial = 0;
          done = 1;
          break;
          }
        if(!get_data(s))
          {
          return 0;
          }
        break;
      default:
        mad_frame_mute(&priv->frame);
        //        fprintf(stderr, "Decode error %04x\n", priv->stream.error);
        break;
      }
    if(done)
      break;
    }
  
  //  fprintf(stderr, "Decoded frame %ld\n", priv->stream.next_frame -
  //          priv->stream.this_frame);
  
  if(priv->do_init)
    {
    /* Get audio format and create frame */

    s->data.audio.format.samplerate = priv->frame.header.samplerate;

    if(priv->frame.header.mode == MAD_MODE_SINGLE_CHANNEL)
      s->data.audio.format.num_channels = 1;
    else
      s->data.audio.format.num_channels = 2;
    
    s->data.audio.format.samplerate = priv->frame.header.samplerate;
    s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
    s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_NONE;
    s->data.audio.format.samples_per_frame =
      MAD_NSBSAMPLES(&(priv->frame.header)) * 32;

    if(!s->codec_bitrate)
      s->codec_bitrate = priv->frame.header.bitrate;
    
    gavl_set_channel_setup(&(s->data.audio.format));

    if(priv->frame.header.flags & MAD_FLAG_MPEG_2_5_EXT)
      {
      if(priv->frame.header.layer == 3)
        s->data.audio.preroll = s->data.audio.format.samples_per_frame * 30;
      else
        s->data.audio.preroll = s->data.audio.format.samples_per_frame;
      version_string = "2.5";
      }
    else if(priv->frame.header.flags & MAD_FLAG_LSF_EXT)
      {
      if(priv->frame.header.layer == 3)
        s->data.audio.preroll = s->data.audio.format.samples_per_frame * 30;
      else
        s->data.audio.preroll = s->data.audio.format.samples_per_frame;
      version_string = "2";
      }
    else
      {
      if(priv->frame.header.layer == 3)
        s->data.audio.preroll = s->data.audio.format.samples_per_frame * 10;
      else
        s->data.audio.preroll = s->data.audio.format.samples_per_frame;
      version_string = "1";
      }
    if(s->codec_bitrate == BGAV_BITRATE_VBR)
      bitrate_string = bgav_sprintf("VBR");
    else
      bitrate_string = bgav_sprintf("%d kb/sec", s->codec_bitrate/1000);
    s->description =
      bgav_sprintf("MPEG-%s layer %d, %s",
                   version_string, priv->frame.header.layer, bitrate_string);
    free(bitrate_string);

    
    priv->audio_frame = gavl_audio_frame_create(&(s->data.audio.format));
    }

  
  mad_synth_frame(&priv->synth, &priv->frame);

  for(i = 0; i < s->data.audio.format.num_channels; i++)
    {
    for(j = 0; j < s->data.audio.format.samples_per_frame; j++)
      {
      if (priv->synth.pcm.samples[i][j] >= MAD_F_ONE)
        priv->synth.pcm.samples[i][j] = MAD_F_ONE - 1;
      else if (priv->synth.pcm.samples[i][j] < -MAD_F_ONE)
        priv->synth.pcm.samples[i][j] = -MAD_F_ONE;
      
      priv->audio_frame->channels.f[i][j] =
        (float)(priv->synth.pcm.samples[i][j]) /
        (float)MAD_F_ONE;
      }
    }
  priv->audio_frame->valid_samples = s->data.audio.format.samples_per_frame;

    
  return 1;
  }

static int init_mad(bgav_stream_t * s)
  {
  mad_priv_t * priv;
  
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;

  if(s->action == BGAV_STREAM_PARSE)
    {
    priv->last_position = -1;
    return 1;
    }
  mad_frame_init(&priv->frame);
  mad_synth_init(&priv->synth);
  mad_stream_init(&priv->stream);
  
  /* Now, decode the first header to get the format */

  
  get_data(s);

  priv->do_init = 1;

  if(!decode_frame(s))
    return 0;

  priv->do_init = 0;
  
  return 1;
  }

static int decode_mad(bgav_stream_t * s, gavl_audio_frame_t * f,
                      int num_samples)
  {
  int samples_decoded = 0;
  int samples_copied;

  mad_priv_t * priv;
  priv = s->data.audio.decoder->priv;
  
  while(samples_decoded < num_samples)
    {
    if(!priv->audio_frame->valid_samples)
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
                            priv->audio_frame,
                            samples_decoded, /* out_pos */
                            s->data.audio.format.samples_per_frame -
                            priv->audio_frame->valid_samples,  /* in_pos */
                            num_samples - samples_decoded, /* out_size, */
                            priv->audio_frame->valid_samples /* in_size */);
    priv->audio_frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  if(f)
    {
    f->valid_samples = samples_decoded;
    }
  return samples_decoded;
  }


static void parse_mad(bgav_stream_t * s)
  {
  bgav_packet_t * p;
  mad_priv_t * priv;
  uint8_t * ptr;
  bgav_mpa_header_t h;
  int old_buffer_size;
  int size_needed;
  priv = s->data.audio.decoder->priv;
  
  while(bgav_demuxer_peek_packet_read(s->demuxer, s, 0))
    {
    /* Get the packet and append data to the buffer */
    p = bgav_demuxer_get_packet_read(s->demuxer, s);

    //    fprintf(stderr, "Parse mad pts: %ld duration: %ld\n", p->pts, s->duration); 
    
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
    while(priv->buffer_size > HEADER_SIZE)
      {
      //      fprintf(stderr, "decode header: ");
      //      bgav_hexdump(ptr, 8, 8);
      if(!bgav_mpa_header_decode(&h, ptr))
        {
        bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Lost sync during parsing");
        return; 
        }
      s->data.audio.format.samplerate = h.samplerate;
      /* If frame starts in the previous packet,
         use the previous index */
      if(ptr - priv->buffer < old_buffer_size)
        bgav_file_index_append_packet(s->file_index,
                                      priv->last_position,
                                      s->duration,
                                      1);
      else
        bgav_file_index_append_packet(s->file_index,
                                      p->position,
                                      s->duration,
                                      1);
      
      s->duration += h.samples_per_frame;
      ptr += h.frame_bytes;
      priv->buffer_size -= h.frame_bytes;
      //      fprintf(stderr, "Parse mad got packet %d bytes left\n", priv->buffer_size); 
      }
    if(priv->buffer_size > 0)
      memmove(priv->buffer, ptr, priv->buffer_size);
    priv->last_position = p->position;
    bgav_demuxer_done_packet_read(s->demuxer, p);
    //    fprintf( stderr, "Done with packet, %d bytes\n",
    //             priv->buffer_size);
    }
  }

static void resync_mad(bgav_stream_t * s)
  {
  mad_priv_t * priv;
  priv = s->data.audio.decoder->priv;
  priv->eof = 0;
  priv->partial = 0;
  mad_frame_mute(&(priv->frame));
  mad_synth_mute(&(priv->synth));

  priv->stream.bufend = 0;
  priv->stream.next_frame = 0;
  }

static void close_mad(bgav_stream_t * s)
  {
  mad_priv_t * priv;
  priv = s->data.audio.decoder->priv;

  mad_synth_finish(&(priv->synth));
  mad_frame_finish(&(priv->frame));
  mad_stream_finish(&(priv->stream));
  if(priv->buffer)
    free(priv->buffer);
  
  if(priv->audio_frame)
    gavl_audio_frame_destroy(priv->audio_frame);
  free(priv);
  }

static bgav_audio_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('.','m','p','3'),
                           BGAV_MK_FOURCC('.','m','p','2'),
                           BGAV_MK_FOURCC('m','s',0x00,0x55),
                           BGAV_MK_FOURCC('m','s',0x00,0x50),
                           BGAV_WAVID_2_FOURCC(0x50),
                           BGAV_WAVID_2_FOURCC(0x55),
                           BGAV_MK_FOURCC('M','P','3',' '), /* NSV */
                           BGAV_MK_FOURCC('L','A','M','E'), /* NUV */
                           0x00 },
    .name   = "MPEG audio decoder (mad)",
    .init   = init_mad,
    .close  = close_mad,
    .resync = resync_mad,
    .decode = decode_mad,
    .parse  = parse_mad,
  };

void bgav_init_audio_decoders_mad()
  {
  bgav_audio_decoder_register(&decoder);
  }

