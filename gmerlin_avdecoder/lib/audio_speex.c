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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>
#include <codecs.h>

#include <speex/speex.h>
#include <speex/speex_header.h>
#include <speex/speex_stereo.h>
#include <speex/speex_callbacks.h>

#define LOG_DOMAIN "speex"

typedef struct
  {
  SpeexBits bits;
  void *dec_state;
  SpeexHeader *header;
  SpeexStereoState stereo;
  int frame_size;

  gavl_audio_frame_t * frame;
  } speex_priv_t;

static SpeexStereoState __stereo = SPEEX_STEREO_STATE_INIT;

static int init_speex(bgav_stream_t * s)
  {
  SpeexCallback callback;
  speex_priv_t * priv;
  
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;

  speex_bits_init(&priv->bits);
  
  if(!s->ext_data)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Speex needs extradata");
    return 0;
    }

  priv->header = speex_packet_to_header((char*)s->ext_data, s->ext_size);

  if(!priv->header)
    return 0;
  
  priv->dec_state = speex_decoder_init(speex_mode_list[priv->header->mode]);

  
  /* Set up format */

  s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
  s->data.audio.format.num_channels = priv->header->nb_channels;
  s->data.audio.format.samplerate = priv->header->rate;
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  gavl_set_channel_setup(&s->data.audio.format);

  speex_decoder_ctl(priv->dec_state, SPEEX_GET_FRAME_SIZE, &priv->frame_size);
  s->data.audio.format.samples_per_frame = priv->frame_size * priv->header->frames_per_packet;

  priv->frame = gavl_audio_frame_create(&s->data.audio.format);

  /* Set stereo callback */

  if(priv->header->nb_channels > 1)
    {
    memcpy(&priv->stereo, &__stereo, sizeof(__stereo));
    
    callback.callback_id = SPEEX_INBAND_STEREO;
    callback.func = speex_std_stereo_request_handler;
    callback.data = &priv->stereo;
    speex_decoder_ctl(priv->dec_state, SPEEX_SET_HANDLER, &callback);
    }
  s->description = bgav_sprintf("Speex");
  return 1;
  }

static int decode_frame_speex(bgav_stream_t * s)
  {
  int i;
  bgav_packet_t * p;
  speex_priv_t * priv;
  priv = (speex_priv_t*)s->data.audio.decoder->priv;

  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;

  speex_bits_read_from(&priv->bits, (char*)p->data, p->data_size);
  
  for(i = 0; i < priv->header->frames_per_packet; i++)
    {
    speex_decode(priv->dec_state, &priv->bits,
                 priv->frame->samples.f +
                 i * priv->frame_size * s->data.audio.format.num_channels);
    if(s->data.audio.format.num_channels > 1)
      {
      speex_decode_stereo(priv->frame->samples.f +
                          i * priv->frame_size * s->data.audio.format.num_channels,
                          priv->frame_size, &priv->stereo);
      }
    }
  
  /* Speex output is scaled like int16_t */
  
  for(i = 0;
      i < priv->frame_size * priv->header->frames_per_packet * s->data.audio.format.num_channels;
      i++)
    {
    priv->frame->samples.f[i] /= 32768.0;
    }
  bgav_packet_done_read(p);

  priv->frame->valid_samples = priv->frame_size * priv->header->frames_per_packet;
  gavl_audio_frame_copy_ptrs(&s->data.audio.format, s->data.audio.frame, priv->frame);
  
  return 1;
  }


static void close_speex(bgav_stream_t * s)
  {
  speex_priv_t * priv;

  priv = (speex_priv_t *)(s->data.audio.decoder->priv);

  speex_bits_destroy(&priv->bits);

  gavl_audio_frame_destroy(priv->frame);
  speex_decoder_destroy(priv->dec_state);
  free(priv->header);
  free(priv);
  }

static bgav_audio_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('S','P','E','X'), 0x00 },
    .name = "Speex decoder",

    .init =   init_speex,
    .decode_frame = decode_frame_speex,
    .close =  close_speex,
    //    .resync = resync_speex,
  };

void bgav_init_audio_decoders_speex()
  {
  bgav_audio_decoder_register(&decoder);
  }
