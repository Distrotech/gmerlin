/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

  //  uint8_t * buffer;
  //  int buffer_alloc;
  
  bgav_bytebuffer_t buf;
  
  gavl_audio_frame_t * audio_frame;
  
  int do_init;
  
  int eof; /* For decoding the very last frame */
  
  int partial; /* Partial frame is left in the buffer.
                  Either we can decode this (layer 3) or
                  we'll mute it. */
  } mad_priv_t;

static gavl_source_status_t get_data(bgav_stream_t * s)
  {
  gavl_source_status_t st;
  bgav_packet_t * p = NULL;
  mad_priv_t * priv;
  
  priv = s->data.audio.decoder->priv;

  st = bgav_stream_get_packet_read(s, &p);

  switch(st)
    {
    case GAVL_SOURCE_AGAIN:
      return st;
      break;
    case GAVL_SOURCE_EOF:
      if(!priv->eof)
        {
        /* Append zeros to the end so we can decode the very last
           frame */
        if(!priv->buf.size)
          {
          priv->eof = 1;
          return GAVL_SOURCE_EOF;
          }
        priv->partial = 1;
        priv->eof = 1;
        }
      break;
    case GAVL_SOURCE_OK:
      bgav_bytebuffer_append(&priv->buf, p, MAD_BUFFER_GUARD);
      bgav_stream_done_packet_read(s, p);
      break;
    }

  return GAVL_SOURCE_OK;
  }

static void get_format(bgav_stream_t * s)
  {
  mad_priv_t * priv;
  const char * version_string;
  
  priv = s->data.audio.decoder->priv;

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
    MAD_NSBSAMPLES(&priv->frame.header) * 32;

  if(!s->codec_bitrate)
    {
    if(s->container_bitrate == GAVL_BITRATE_VBR)
      s->codec_bitrate = GAVL_BITRATE_VBR;
    else
      s->codec_bitrate = priv->frame.header.bitrate;
    }
  gavl_set_channel_setup(&s->data.audio.format);

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
  
  gavl_metadata_set_nocpy(&s->m, GAVL_META_FORMAT,
                          bgav_sprintf("MPEG-%s layer %d",
                                       version_string, priv->frame.header.layer));
  
  priv->audio_frame = gavl_audio_frame_create(&s->data.audio.format);
  }

static gavl_source_status_t decode_frame_mad(bgav_stream_t * s)
  {
  mad_priv_t * priv;
  int i, j, done;
  gavl_source_status_t st;
  priv = s->data.audio.decoder->priv;

  if(priv->eof)
    return GAVL_SOURCE_EOF;
  
  /* Check if we need new data */
  if((priv->buf.size <= MAD_BUFFER_GUARD) && 
     ((st = get_data(s)) != GAVL_SOURCE_OK))
    return st;

  if(priv->partial)
    mad_stream_buffer(&priv->stream, priv->buf.buffer,
                      priv->buf.size + MAD_BUFFER_GUARD);
  else
    mad_stream_buffer(&priv->stream, priv->buf.buffer,
                      priv->buf.size);
  
  done = 0;
  while(mad_frame_decode(&priv->frame, &priv->stream) == -1)
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
        
        if((st = get_data(s)) != GAVL_SOURCE_OK)
          return st;
        
        mad_stream_buffer(&priv->stream, priv->buf.buffer,
                          priv->buf.size);

        break;
      default:
        mad_frame_mute(&priv->frame);
        break;
      }
    if(done)
      break;
    }
  
  if(priv->do_init)
    get_format(s);
 
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
  priv->audio_frame->valid_samples   = s->data.audio.format.samples_per_frame;
  gavl_audio_frame_copy_ptrs(&s->data.audio.format,
                             s->data.audio.frame, priv->audio_frame);
#if 0
  fprintf(stderr, "Done decode %ld %ld\n",
          priv->stream.this_frame - priv->stream.buffer,
          priv->stream.next_frame - priv->stream.this_frame);
#endif

  s->flags |= STREAM_HAVE_FRAME;
  
  bgav_bytebuffer_remove(&priv->buf, 
                         priv->stream.next_frame - priv->stream.buffer);
  return GAVL_SOURCE_OK;
  }

static int init_mad(bgav_stream_t * s)
  {
  mad_priv_t * priv;
  
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;

  mad_frame_init(&priv->frame);
  mad_synth_init(&priv->synth);
  mad_stream_init(&priv->stream);
  
  /* Now, decode the first header to get the format */
  
  get_data(s);

  priv->do_init = 1;

  if(!decode_frame_mad(s))
    return 0;

  priv->do_init = 0;
  
  return 1;
  }

static void resync_mad(bgav_stream_t * s)
  {
  mad_priv_t * priv;
  priv = s->data.audio.decoder->priv;
  priv->eof = 0;
  priv->partial = 0;
  mad_frame_finish(&priv->frame);
  mad_synth_finish(&priv->synth);
  mad_stream_finish(&priv->stream);
  
  //  fprintf(stderr, "Resync mad\n");

  bgav_bytebuffer_flush(&priv->buf);
  
  mad_frame_init(&priv->frame);
  mad_synth_init(&priv->synth);
  mad_stream_init(&priv->stream);

  }

static void close_mad(bgav_stream_t * s)
  {
  mad_priv_t * priv;
  priv = s->data.audio.decoder->priv;

  mad_synth_finish(&priv->synth);
  mad_frame_finish(&priv->frame);
  mad_stream_finish(&priv->stream);

  bgav_bytebuffer_free(&priv->buf);
  
  if(priv->audio_frame)
    gavl_audio_frame_destroy(priv->audio_frame);
  free(priv);
  }

static bgav_audio_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('.','m','p','3'),
                             BGAV_MK_FOURCC('m','p','g','a'),
                             BGAV_MK_FOURCC('.','m','p','2'),
                             BGAV_MK_FOURCC('.','m','p','1'),
                           BGAV_MK_FOURCC('m','s',0x00,0x55),
                           BGAV_MK_FOURCC('m','s',0x00,0x50),
                           BGAV_WAVID_2_FOURCC(0x50),
                           BGAV_WAVID_2_FOURCC(0x55),
                           BGAV_MK_FOURCC('M','P','3',' '), /* NSV */
                           BGAV_MK_FOURCC('L','A','M','E'), /* NUV */
                           BGAV_MK_FOURCC('m','p','g','a'), /* Program-/Raw transport */
                           0x00 },
    .name   = "MPEG audio decoder (mad)",
    .init         = init_mad,
    .close        = close_mad,
    .resync       = resync_mad,
    .decode_frame = decode_frame_mad,
  };

void bgav_init_audio_decoders_mad()
  {
  bgav_audio_decoder_register(&decoder);
  }

