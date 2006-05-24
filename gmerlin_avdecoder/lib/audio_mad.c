/*****************************************************************
 
  audio_mad.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <mad.h>

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>

typedef struct
  {
  struct mad_stream stream;
  struct mad_frame frame;
  struct mad_synth synth;

  uint8_t * buffer;
  int buffer_alloc;
  int buffer_size;

  gavl_audio_frame_t * audio_frame;
  
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
    //    fprintf(stderr, "No packet\n");
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
  //  fprintf(stderr, "Get data %d %d\n", p->data_size, bytes_in_buffer);
  //  bgav_hexdump(priv->buffer, 16, 16);
    
  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }

#if 0

static int get_frame_bytes(struct mad_header * h)
  {
  int pad;
  int slots_per_frame;
  int ret;
  
  pad = (h->flags & MAD_FLAG_PADDING) ? 1 : 0;
  
  if(h->layer == 1)
    {
    ret = ((12 * h->bitrate / h->samplerate) + pad) * 4;
    }
  else
    {
    slots_per_frame = ((h->layer == 3) &&
                       (h->flags & MAD_FLAG_LSF_EXT)) ? 72 : 144;
    ret = (slots_per_frame * h->bitrate) / h->samplerate + pad;
    }
  return ret;
  }

#endif

static int decode_frame(bgav_stream_t * s)
  {
  mad_priv_t * priv;
  priv = s->data.audio.decoder->priv;
  const char * version_string;
  char * bitrate_string;
  int i, j;

  /* Check if we need new data */

  if(priv->stream.bufend - priv->stream.next_frame <= MAD_BUFFER_GUARD)
    if(!get_data(s))
      {
      fprintf(stderr, "No data left\n");
      return 0;
      }
  //  fprintf(stderr, "Decode frame\n");

  while(mad_frame_decode(&(priv->frame), &(priv->stream)) == -1)
    {
    switch(priv->stream.error)
      {
      case MAD_ERROR_BUFLEN:
        if(!get_data(s))
          return 0;
        break;
      default:
        fprintf(stderr, "mad_decode_frame returned: %s\n",
                mad_stream_errorstr(&priv->stream));
        mad_frame_mute(&priv->frame);
        break;
      }
    }

  if(!priv->audio_frame)
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
      version_string = "2.5";
    else if(priv->frame.header.flags & MAD_FLAG_LSF_EXT)
      version_string = "2";
    else
      version_string = "1";

    if(s->codec_bitrate == BGAV_BITRATE_VBR)
      bitrate_string = bgav_sprintf("VBR");
    else
      bitrate_string = bgav_sprintf("%d kb/sec", s->codec_bitrate/1000);
    s->description =
      bgav_sprintf("MPEG-%s layer %d, %s",
                   version_string, priv->frame.header.layer, bitrate_string);
    free(bitrate_string);

    //    fprintf(stderr, "Creating audio frame %d\n",
    //            s->data.audio.format.samples_per_frame);
    
    priv->audio_frame = gavl_audio_frame_create(&(s->data.audio.format));
    }

  //  fprintf(stderr, "Bytes left: %d\n", (int)(priv->stream.bufend - priv->stream.next_frame));
  
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

  //  fprintf(stderr, "Init MAD\n");

  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;
  
  mad_frame_init(&priv->frame);
  mad_synth_init(&priv->synth);
  mad_stream_init(&priv->stream);
  
  /* Now, decode the first header to get the format */

  get_data(s);
  if(!decode_frame(s))
    {
    fprintf(stderr, "Decode frame failed\n");
    return 0;
    }
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

    //    fprintf(stderr, "Copy frame: in_pos: %d, out_pos: %d, in_size: %d, out_size: %d\n",
    //            s->data.audio.format.samples_per_frame -
    //            priv->audio_frame->valid_samples,
    //            samples_decoded,
    //            priv->audio_frame->valid_samples,
    //            num_samples - samples_decoded);
    
    samples_copied =
      gavl_audio_frame_copy(&(s->data.audio.format),
                            f,
                            priv->audio_frame,
                            samples_decoded, /* out_pos */
                            s->data.audio.format.samples_per_frame -
                            priv->audio_frame->valid_samples,  /* in_pos */
                            num_samples - samples_decoded, /* out_size, */
                            priv->audio_frame->valid_samples /* in_size */);
    //    fprintf(stderr, "done %d\n", samples_copied);
    priv->audio_frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  if(f)
    {
    f->valid_samples = samples_decoded;
    }
  return samples_decoded;
  }


static void resync_mad(bgav_stream_t * s)
  {
  
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
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('.','m', 'p', '3'),
                           BGAV_MK_FOURCC('.','m', 'p', '2'),
                           BGAV_MK_FOURCC('m', 's', 0x00, 0x55),
                           BGAV_MK_FOURCC('m', 's', 0x00, 0x50),
                           BGAV_WAVID_2_FOURCC(0x50),
                           BGAV_WAVID_2_FOURCC(0x55),
                           BGAV_MK_FOURCC('M','P', '3', ' '), /* NSV */
                           0x00 },
    name:   "Mpeg audio decoder (mad)",
    init:   init_mad,
    close:  close_mad,
    resync: resync_mad,
    decode: decode_mad
  };

void bgav_init_audio_decoders_mad()
  {
  bgav_audio_decoder_register(&decoder);
  }

