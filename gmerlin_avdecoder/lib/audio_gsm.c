/*****************************************************************
 
  audio_gsm.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <avdec_private.h>
#include "GSM610/gsm.h"

/* Audio decoder for the internal libgsm */

/* From ffmpeg */
// gsm.h miss some essential constants
#define GSM_BLOCK_SIZE 33
#define GSM_FRAME_SIZE 160

// #define GSM_BLOCK_SIZE_MS 65
// #define GSM_FRAME_SIZE_MS 320

typedef struct
  {
  gsm gsm_state;
    
  bgav_packet_t * packet;
  uint8_t       * packet_ptr;
  gavl_audio_frame_t * frame;
  int ms;
  } gsm_priv;

static int init_gsm(bgav_stream_t * s)
  {
  int tmp;
  gsm_priv * priv;

  /* Allocate stuff */
  priv = calloc(1, sizeof(*priv));
  priv->gsm_state = gsm_create();
  s->data.audio.decoder->priv = priv;

  if(s->data.audio.format.num_channels > 1)
    {
    fprintf(stderr,
            "Multichannel GSM not supported (who encodes such a nonsense?)\n");
    return 0;
    }

  if((s->fourcc == BGAV_WAVID_2_FOURCC(0x31)) ||
     (s->fourcc == BGAV_WAVID_2_FOURCC(0x32)))
    {
    priv->ms = 1;
    tmp = 1;
    gsm_option(priv->gsm_state, GSM_OPT_WAV49, &tmp);
    }

  /* Set format */
  s->data.audio.format.interleave_mode   = GAVL_INTERLEAVE_NONE;
  s->data.audio.format.sample_format     = GAVL_SAMPLE_S16;
  
  s->data.audio.format.samples_per_frame = priv->ms ? 2*GSM_FRAME_SIZE : GSM_FRAME_SIZE;
  gavl_set_channel_setup(&s->data.audio.format);
  
  priv->frame = gavl_audio_frame_create(&s->data.audio.format);

  if(priv->ms)
    s->description = bgav_sprintf("MSGM");
  else
    s->description = bgav_sprintf("GSM 6.10");
  return 1;
  }

static void close_gsm(bgav_stream_t * s)
  {
  gsm_priv * priv;
  priv = (gsm_priv*)s->data.audio.decoder->priv;

  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);
  gsm_destroy(priv->gsm_state);
  free(priv);
  }

static int decode_frame(bgav_stream_t * s)
  {
  gsm_priv * priv;

  priv = (gsm_priv*)s->data.audio.decoder->priv;
  
  if(!priv->packet)
    {
    priv->packet = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!priv->packet)
      return 0;
    priv->packet_ptr = priv->packet->data;
    }
  else if(priv->packet_ptr - priv->packet->data +
          GSM_BLOCK_SIZE + priv->ms * (GSM_BLOCK_SIZE-1)
          >= priv->packet->data_size)
    {
    bgav_demuxer_done_packet_read(s->demuxer, priv->packet);
    priv->packet = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!priv->packet)
      return 0;
    priv->packet_ptr = priv->packet->data;
    //    fprintf(stderr, "Get packet %d\n", priv->packet->data_size);
    }
  gsm_decode(priv->gsm_state, priv->packet_ptr, priv->frame->samples.s_16);
  priv->frame->valid_samples = GSM_FRAME_SIZE;
  
  if(priv->ms)
    {
    priv->packet_ptr += GSM_BLOCK_SIZE;
    //    priv->packet_ptr += block_size-1;
    gsm_decode(priv->gsm_state, priv->packet_ptr,
               priv->frame->samples.s_16+GSM_FRAME_SIZE);
    priv->frame->valid_samples += GSM_FRAME_SIZE;
    priv->packet_ptr += GSM_BLOCK_SIZE-1;
    }
  else
    priv->packet_ptr += GSM_BLOCK_SIZE;
  return 1;
  }

static int decode_gsm(bgav_stream_t * s,
                      gavl_audio_frame_t * f, int num_samples)
  {
  int samples_decoded = 0;
  int samples_copied;
  gsm_priv * priv;
  priv = (gsm_priv*)s->data.audio.decoder->priv;

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
                            GSM_FRAME_SIZE * (priv->ms + 1) -
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

static void resync_gsm(bgav_stream_t * s)
  {
  gsm_priv * priv;
  priv = (gsm_priv*)s->data.audio.decoder->priv;
  
  priv->frame->valid_samples = 0;
  priv->packet = (bgav_packet_t*)0;
  priv->packet_ptr = (uint8_t*)0;
  
  }

static bgav_audio_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x31),
               BGAV_WAVID_2_FOURCC(0x32),
               BGAV_MK_FOURCC('a', 'g', 's', 'm'),
               BGAV_MK_FOURCC('G', 'S', 'M', ' '),
               0x00 },
    name: "libgsm based decoder",

    init:   init_gsm,
    decode: decode_gsm,
    resync: resync_gsm,
    close:  close_gsm,
  };

void bgav_init_audio_decoders_gsm()
  {
  bgav_audio_decoder_register(&decoder);
  }
