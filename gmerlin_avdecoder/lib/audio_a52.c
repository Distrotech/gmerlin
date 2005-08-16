/*****************************************************************
 
  audio_a52.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <string.h>

#include <config.h>
#include <codecs.h>

#include <avdec_private.h>
#include <a52dec/a52.h>

#define FRAME_SAMPLES 1536

typedef struct
  {
  int total_bytes;
  int samplerate;
  int bitrate;

  int acmod;
  int lfe;
  int dolby;

  float cmixlev;
  float smixlev;
  sample_t * samples;
  
  } a52_header;

typedef struct
  {
  a52_header header;
  
  a52_state_t * state;
  sample_t    * samples;

  uint8_t * buffer;
  int bytes_in_buffer;
  
  bgav_packet_t * packet;
  uint8_t       * packet_ptr;
  gavl_audio_frame_t * frame;
  } a52_priv;

#define MAX_FRAME_SIZE 3840
#define HEADER_BYTES 7

#define LEVEL_3DB 0.7071067811865476
#define LEVEL_45DB 0.5946035575013605
#define LEVEL_6DB 0.5

static uint8_t halfrate[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3};
static int rate[] = { 32,  40,  48,  56,  64,  80,  96, 112,
                      128, 160, 192, 224, 256, 320, 384, 448,
                      512, 576, 640};
static uint8_t lfeon[8] = {0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01};

static float clev[4] = {LEVEL_3DB, LEVEL_45DB, LEVEL_6DB, LEVEL_45DB};
static float slev[4] = {LEVEL_3DB, LEVEL_6DB,          0, LEVEL_6DB};

static int a52_header_read(a52_header * ret, uint8_t * buf)
  {
  int half;
  int frmsizecod;
  int bitrate;

  int cmixlev;
  int smixlev;
  
  
  if ((buf[0] != 0x0b) || (buf[1] != 0x77))   /* syncword */
    return 0;

  if (buf[5] >= 0x60)         /* bsid >= 12 */
    return 0;

  half = halfrate[buf[5] >> 3];

  /* acmod, dsurmod and lfeon */
  ret->acmod = buf[6] >> 5;

  /* cmixlev and surmixlev */

  if((ret->acmod & 0x01) && (ret->acmod != 0x01))
    {
    cmixlev = (buf[6] & 0x18) >> 3;
    }
  else
    cmixlev = -1;
  
  if(ret->acmod & 0x04)
    {
    if((cmixlev) == -1)
      smixlev = (buf[6] & 0x18) >> 3;
    else
      smixlev = (buf[6] & 0x06) >> 1;
    }
  else
    smixlev = -1;

  if(smixlev >= 0)
    ret->smixlev = slev[smixlev];
  if(cmixlev >= 0)
    ret->cmixlev = clev[cmixlev];
  
  if((buf[6] & 0xf8) == 0x50)
    ret->dolby = 1;

  if(buf[6] & lfeon[ret->acmod])
    ret->lfe = 1;

  frmsizecod = buf[4] & 63;
  if (frmsizecod >= 38)
    return 0;
  bitrate = rate[frmsizecod >> 1];
  ret->bitrate = (bitrate * 1000) >> half;

  switch (buf[4] & 0xc0)
    {
    case 0:
      ret->samplerate = 48000 >> half;
      ret->total_bytes = 4 * bitrate;
      break;
    case 0x40:
      ret->samplerate = 44100 >> half;
      ret->total_bytes =  2 * (320 * bitrate / 147 + (frmsizecod & 1));
      break;
    case 0x80:
      ret->samplerate = 32000 >> half;
      ret->total_bytes =  6 * bitrate;
      break;
    default:
      return 0;
    }
  
  return 1;
  }

static void a52_header_dump(a52_header * h)
  {
  fprintf(stderr, "A52 header:\n");
  fprintf(stderr, "  Frame bytes: %d\n", h->total_bytes);
  fprintf(stderr, "  Samplerate:  %d\n", h->samplerate);
  fprintf(stderr, "  acmod:  0x%0x\n",   h->acmod);
  if(h->smixlev >= 0.0)
    fprintf(stderr, "  smixlev: %f\n", h->smixlev);
  if(h->cmixlev >= 0.0)
    fprintf(stderr, "  cmixlev: %f\n", h->cmixlev);
  }

/*
 *  Parse header and read data bytes for that frame
 *  (also does resync)
 */

static int get_data(bgav_stream_t * s, int num_bytes)
  {
  int bytes_to_copy;
  a52_priv * priv;
  priv = (a52_priv*)(s->data.audio.decoder->priv);
  
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
  a52_priv * priv;
  int bytes_left;
  
  priv = (a52_priv*)(s->data.audio.decoder->priv);

  bytes_left = priv->bytes_in_buffer - num_bytes;
  
  if(bytes_left < 0)
    {
    fprintf(stderr, "BUUUUUG in liba52!!!\n");
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
  priv = (a52_priv*)(s->data.audio.decoder->priv);

  while(1)
    {
    if(!get_data(s, HEADER_BYTES))
      return 0;
    if(a52_header_read(&(priv->header), priv->buffer))
      return 1;
    done_data(s, 1);
    }
  return 0;
  }

static void resync_a52(bgav_stream_t * s)
  {
  a52_priv * priv;
  priv = (a52_priv*)(s->data.audio.decoder->priv);

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
  do_resync(s);
  
  a52_header_dump(&(priv->header));

  /* Get format */

  s->data.audio.format.samplerate = priv->header.samplerate;
  s->codec_bitrate = priv->header.bitrate;
  s->data.audio.format.samples_per_frame = FRAME_SAMPLES;
  s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
  
  if(priv->header.lfe)
    {
    s->data.audio.format.lfe = 1;
    s->data.audio.format.num_channels = 1;
    s->data.audio.format.channel_locations[0] = GAVL_CHID_LFE;
    }
  else
    s->data.audio.format.num_channels = 0;
  
  switch(priv->header.acmod)
    {
    case A52_CHANNEL:
    case A52_STEREO:
      s->data.audio.format.channel_setup = GAVL_CHANNEL_STEREO;
      
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+1] = 
        GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.num_channels += 2;
      break;
    case A52_MONO:
      s->data.audio.format.channel_setup = GAVL_CHANNEL_MONO;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT;
      s->data.audio.format.num_channels += 1;
      break;
    case A52_3F:
      s->data.audio.format.channel_setup = GAVL_CHANNEL_3F;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+1] = 
        GAVL_CHID_FRONT_CENTER;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+2] = 
        GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.num_channels += 3;
      break;
    case A52_2F1R:
      s->data.audio.format.channel_setup = GAVL_CHANNEL_2F1R;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+1] = 
        GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+2] = 
        GAVL_CHID_REAR;
      s->data.audio.format.num_channels += 3;
      break;
    case A52_3F1R:
      s->data.audio.format.channel_setup = GAVL_CHANNEL_3F1R;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels] = 
        GAVL_CHID_FRONT_LEFT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+1] = 
        GAVL_CHID_FRONT_CENTER;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+2] = 
        GAVL_CHID_FRONT_RIGHT;
      s->data.audio.format.channel_locations[s->data.audio.format.num_channels+3] = 
        GAVL_CHID_REAR;
      s->data.audio.format.num_channels += 4;

      break;
    case A52_2F2R:
      s->data.audio.format.channel_setup = GAVL_CHANNEL_2F2R;
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
      s->data.audio.format.channel_setup = GAVL_CHANNEL_3F2R;
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
  priv = (a52_priv*)s->data.audio.decoder->priv;

  //  fprintf(stderr, "Decode frame");
  
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
  a52_dynrng(priv->state, NULL, NULL);

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
  priv = (a52_priv*)s->data.audio.decoder->priv;

  while(samples_decoded < num_samples)
    {
    if(!priv->frame->valid_samples)
      {
      if(!decode_frame(s))
        {
        if(f)
          f->valid_samples = samples_decoded;
        //        fprintf(stderr, "A52: EOF %d\n", samples_decoded);
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
  //  fprintf(stderr, "done %d\n", samples_decoded);
  return samples_decoded;
  }

static void close_a52(bgav_stream_t * s)
  {
  a52_priv * priv;
  priv = (a52_priv*)s->data.audio.decoder->priv;

  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);
  if(priv->buffer)
    free(priv->buffer);

  a52_free(priv->state);
  free(priv);
  }

static bgav_audio_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x2000),
                      BGAV_MK_FOURCC('.', 'a', 'c', '3'), 0x00 },
    name: "liba52 based decoder",

    init:   init_a52,
    decode: decode_a52,
    close:  close_a52,
    resync: resync_a52,
  };

void bgav_init_audio_decoders_a52()
  {
  bgav_audio_decoder_register(&decoder);
  }
