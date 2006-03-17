/*****************************************************************
 
  audio_speex.c
 
  Copyright (c) 2005-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>
#include <speex/speex.h>
#include <speex/speex_header.h>
#include <speex/speex_stereo.h>
#include <speex/speex_callbacks.h>

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
    fprintf(stderr, "Speex needs extradata\n");
    return 0;
    }

  priv->header = speex_packet_to_header(s->ext_data, s->ext_size);

  if(!priv->header)
    return 0;
  
  priv->dec_state = speex_decoder_init(speex_mode_list[priv->header->mode]);

  fprintf(stderr, "Mode: %d\n", priv->header->mode);
  
  /* Set up format */

  s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
  s->data.audio.format.num_channels = priv->header->nb_channels;
  s->data.audio.format.samplerate = priv->header->rate;
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  gavl_set_channel_setup(&s->data.audio.format);

  speex_decoder_ctl(priv->dec_state, SPEEX_GET_FRAME_SIZE, &(priv->frame_size));
  s->data.audio.format.samples_per_frame = priv->frame_size * priv->header->frames_per_packet;

  priv->frame = gavl_audio_frame_create(&s->data.audio.format);

  /* Set stereo callback */

  if(priv->header->nb_channels > 1)
    {
    memcpy(&(priv->stereo), &__stereo, sizeof(__stereo));
    
    callback.callback_id = SPEEX_INBAND_STEREO;
    callback.func = speex_std_stereo_request_handler;
    callback.data = &priv->stereo;
    speex_decoder_ctl(priv->dec_state, SPEEX_SET_HANDLER, &callback);
    }
  s->description = bgav_sprintf("Speex");
  return 1;
  }

static int decode_packet(bgav_stream_t * s)
  {
  int i;
  bgav_packet_t * p;
  speex_priv_t * priv;
  priv = (speex_priv_t*)s->data.audio.decoder->priv;

  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;

  speex_bits_read_from(&(priv->bits), (char*)p->data, p->data_size);
  
  for(i = 0; i < priv->header->frames_per_packet; i++)
    {
    //    fprintf(stderr, "speex_decode %d\n", p->data_size);
    speex_decode(priv->dec_state, &(priv->bits),
                 priv->frame->samples.f + i * priv->frame_size * s->data.audio.format.num_channels);
    //    fprintf(stderr, "speex_decode done\n");
    if(s->data.audio.format.num_channels > 1)
      {
      speex_decode_stereo(priv->frame->samples.f +
                          i * priv->frame_size * s->data.audio.format.num_channels,
                          priv->frame_size, &(priv->stereo));
      }
    }
  priv->frame->valid_samples = priv->frame_size * priv->header->frames_per_packet;

  /* Speex output is scaled like int16_t */
  
  for(i = 0;
      i < priv->frame_size * priv->header->frames_per_packet * s->data.audio.format.num_channels;
      i++)
    {
    priv->frame->samples.f[i] /= 32768.0;
    }
  

  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }

static int decode_speex(bgav_stream_t * s,
                        gavl_audio_frame_t * f, int num_samples)
  {
  speex_priv_t * priv;
  int samples_copied;
  int samples_decoded = 0;

  priv = (speex_priv_t *)(s->data.audio.decoder->priv);
  
  while(samples_decoded < num_samples)
    {
    if(!priv->frame->valid_samples)
      {
      //      fprintf(stderr, "decode frame...");
      if(!decode_packet(s))
        {
        if(f)
          f->valid_samples = samples_decoded;
        return samples_decoded;
        }
      //      fprintf(stderr, "done\n");
      }
    samples_copied = gavl_audio_frame_copy(&(s->data.audio.format),
                                           f,
                                           priv->frame,
                                           samples_decoded, /* out_pos */
                                           priv->frame_size * priv->header->frames_per_packet -
                                           priv->frame->valid_samples,  /* in_pos */
                                           num_samples - samples_decoded, /* out_size, */
                                           priv->frame->valid_samples /* in_size */);
    priv->frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  if(f)
    f->valid_samples = samples_decoded;
  return samples_decoded;
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
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('S','P','E','X'), 0x00 },
    name: "Speex decoder",

    init:   init_speex,
    decode: decode_speex,
    close:  close_speex,
    //    resync: resync_speex,
  };

void bgav_init_audio_decoders_speex()
  {
  bgav_audio_decoder_register(&decoder);
  }
