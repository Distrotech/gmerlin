/*****************************************************************
 
  audio_vorbis.c
 
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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>

#define BGAV_VORBIS BGAV_MK_FOURCC('V','B','I','S')

typedef struct
  {
  ogg_sync_state   dec_oy; /* sync and verify incoming physical bitstream */
  ogg_stream_state dec_os; /* take physical pages, weld into a logical
                              stream of packets */
  ogg_page         dec_og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       dec_op; /* one raw packet of data for decode */

  vorbis_info      dec_vi; /* struct that stores all the static vorbis bitstream
                                settings */
  vorbis_comment   dec_vc; /* struct that stores all the bitstream user comments */
  vorbis_dsp_state dec_vd; /* central working state for the packet->PCM decoder */
  vorbis_block     dec_vb; /* local working space for packet->PCM decode */
  int last_block_size;     /* Last block size in SAMPLES */
  gavl_audio_frame_t * frame;
  bgav_packet_t * p;
  int stream_initialized;


  } vorbis_audio_priv;

/*
 *  The following function comes at the very end, because it's 
 *  not that nice :-)
 *  (Ported from the Vorbis ACM codec)
 */

static char * get_default_vorbis_header(bgav_stream_t * stream, int *len);

static uint8_t * parse_packet(ogg_packet * op, uint8_t * str)
  {
  memcpy(op, str, sizeof(*op));
  op->packet = str + sizeof(*op);
  //  fprintf(stderr, "Parse packet %ld\n", op->bytes);
  return str + sizeof(*op) + op->bytes;
  }

/* Put raw streams into the sync engine */

static int read_data(bgav_stream_t * s)
  {
  char * buffer;
  bgav_packet_t * p;
  vorbis_audio_priv * priv;
  priv = (vorbis_audio_priv*)(s->data.audio.decoder->priv);
  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;

  //  fprintf(stderr, "Read data %d bytes\n", p->data_size);
  //  bgav_hexdump(p->data, p->data_size, 16);
  
  buffer = ogg_sync_buffer(&priv->dec_oy, p->data_size);
  memcpy(buffer, p->data, p->data_size);
  ogg_sync_wrote(&priv->dec_oy, p->data_size);
  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }

static int next_page(bgav_stream_t * s)
  {
  int result = 0;
  vorbis_audio_priv * priv;
  priv = (vorbis_audio_priv*)(s->data.audio.decoder->priv);

  //  fprintf(stderr, "Next page\n");
  
  while(result < 1)
    {
    result = ogg_sync_pageout(&priv->dec_oy, &priv->dec_og);
    
    if(result == 0)
      {
      if(!read_data(s))
        return 0;
      }
    else
      {
      /* Initialitze stream state */
      if(!priv->stream_initialized)
        {
        ogg_stream_init(&priv->dec_os, ogg_page_serialno(&priv->dec_og));
        priv->stream_initialized = 1;
        }
      ogg_stream_pagein(&priv->dec_os, &priv->dec_og);
      }

    }
  return 1;
  }

static int next_packet(bgav_stream_t * s)
  {
  int result = 0;
  vorbis_audio_priv * priv;
  priv = (vorbis_audio_priv*)(s->data.audio.decoder->priv);

  //  fprintf(stderr, "Next packet\n");

  if(s->fourcc == BGAV_VORBIS)
    {
    if(priv->p)
      bgav_demuxer_done_packet_read(s->demuxer, priv->p);
    priv->p = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!priv->p)
      return 0;
    parse_packet(&priv->dec_op, priv->p->data); 
    }
  else
    {
    while(result < 1)
      {
      result = ogg_stream_packetout(&priv->dec_os, &priv->dec_op);
      
      //    fprintf(stderr, "Getting new packet %d\n", result);
      
      if(result == 0)
        {
        if(!next_page(s))
          return 0;
        }
      }
    }
  return 1;
  }


static int init_vorbis(bgav_stream_t * s)
  {
  uint8_t * ptr, * buffer;
  char * default_header;
  int default_header_len;

  uint32_t header_sizes[3];
  uint32_t len;
  uint32_t fourcc;
  vorbis_audio_priv * priv;
  priv = calloc(1, sizeof(*priv));
  ogg_sync_init(&priv->dec_oy);

  vorbis_info_init(&priv->dec_vi);
  vorbis_comment_init(&priv->dec_vc);

  s->data.audio.decoder->priv = priv;
  
  /* Heroine Virtual way:
     The 3 header packets are in the first audio chunk */
  
  if((s->fourcc == BGAV_MK_FOURCC('O','g', 'g', 'S')) ||
     (s->fourcc == BGAV_WAVID_2_FOURCC(0x674f)) || /* mode 1  */
     (s->fourcc == BGAV_WAVID_2_FOURCC(0x676f)))   /* mode 1+ */
    {
    if(!next_page(s))
      return 0;
    
    if(!next_packet(s))
      return 0;
    
    /* Initialize vorbis */
    
    if(vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc,
                                 &priv->dec_op) < 0)
      {
      fprintf(stderr, "decode: vorbis_synthesis_headerin: not a vorbis header\n");
      return 1;
      }
    
    if(!next_packet(s))
      return 0;
    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc, &priv->dec_op);
    if(!next_packet(s))
      return 0;
    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc, &priv->dec_op);
    }
  /*
   * AVI way:
   * Header packets are in extradata:
   * starting with byte 22 if ext_data, we first have
   * 3 uint32_ts for the packet sizes, followed by the raw
   * packets
   */
  else if(s->fourcc == BGAV_WAVID_2_FOURCC(0xfffe))
    {
    if(s->ext_size < 22 + 3 * sizeof(uint32_t))
      {
      fprintf(stderr, "Vorbis decoder: Init data too small\n");
      return 0;
      }
    bgav_hexdump(s->ext_data, s->ext_size, 16);

    ptr = s->ext_data + 22;
    header_sizes[0] = BGAV_PTR_2_32LE(ptr);ptr+=4;
    header_sizes[1] = BGAV_PTR_2_32LE(ptr);ptr+=4;
    header_sizes[2] = BGAV_PTR_2_32LE(ptr);ptr+=4;

    priv->dec_op.packet = ptr;
    priv->dec_op.b_o_s  = 1;
    priv->dec_op.bytes  = header_sizes[0];

    if(vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc,
                                 &priv->dec_op) < 0)
      {
      fprintf(stderr, "decode: vorbis_synthesis_headerin: not a vorbis header\n");
      return 1;
      }
    ptr += header_sizes[0];

    priv->dec_op.packet = ptr;
    priv->dec_op.b_o_s  = 0;
    priv->dec_op.bytes  = header_sizes[1];

    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc,
                              &priv->dec_op);
    ptr += header_sizes[1];

    priv->dec_op.packet = ptr;
    priv->dec_op.b_o_s  = 0;
    priv->dec_op.bytes  = header_sizes[2];

    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc,
                              &priv->dec_op);
    //    ptr += header_sizes[1];
    }
  /* AVI Vorbis mode 2: Codec data in extradata starting with byte 8  */
  /* (bytes 0 - 7 are acm version and libvorbis version, both 32 bit) */

  else if((s->fourcc == BGAV_WAVID_2_FOURCC(0x6750)) ||
          (s->fourcc == BGAV_WAVID_2_FOURCC(0x6770)))
    {
    //    fprintf(stderr, "Extradata:\n");
    //    bgav_hexdump(s->ext_data, s->ext_size, 16);

    if(s->ext_size <= 8)
      {
      fprintf(stderr, "ext size too small\n");
      return 0;
      }
    buffer = ogg_sync_buffer(&priv->dec_oy, s->ext_size - 8);
    memcpy(buffer, s->ext_data + 8, s->ext_size - 8);
    ogg_sync_wrote(&priv->dec_oy, s->ext_size - 8);

    if(!next_page(s))
      return 0;
    if(!next_packet(s))
      return 0;
    /* Initialize vorbis */
    
    //    fprintf(stderr, "Init vorbis %d\n", s->ext_size);
    //    bgav_hexdump(s->ext_data, s->ext_size, 16);
    
    if(vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc,
                                 &priv->dec_op) < 0)
      {
      fprintf(stderr, "decode: vorbis_synthesis_headerin: not a vorbis header\n");
      return 1;
      }
    
    if(!next_packet(s))
      return 0;
    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc, &priv->dec_op);
    if(!next_packet(s))
      return 0;
    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc, &priv->dec_op);
    }
#if 1

  /* This is the most ugly method (AVI vorbis Mode 3): There is no header inside
     the file. Instead, an encoder is created from the stream data.
  */
  
  else if((s->fourcc == BGAV_WAVID_2_FOURCC(0x6751)) ||
          (s->fourcc == BGAV_WAVID_2_FOURCC(0x6771)))
    {
    default_header = get_default_vorbis_header(s, &default_header_len);

    fprintf(stderr, "Default header:\n");
    bgav_hexdump(default_header, default_header_len, 16); 
    
    buffer = ogg_sync_buffer(&priv->dec_oy, default_header_len);
    memcpy(buffer, default_header, default_header_len);
    ogg_sync_wrote(&priv->dec_oy, default_header_len);
    free(default_header);
    
    if(!next_page(s))
      return 0;
    if(!next_packet(s))
      return 0;
    /* Initialize vorbis */
    
    //    fprintf(stderr, "Init vorbis %d\n", s->ext_size);
    //    bgav_hexdump(s->ext_data, s->ext_size, 16);
    
    if(vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc,
                                 &priv->dec_op) < 0)
      {
      fprintf(stderr, "decode: vorbis_synthesis_headerin: not a vorbis header\n");
      return 1;
      }
    
    if(!next_packet(s))
      return 0;
    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc, &priv->dec_op);
    if(!next_packet(s))
      return 0;
    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc, &priv->dec_op);

    
    }
#endif
  
  /*
   *  OggV method (qtcomponents.sf.net):
   *  In the sample description, we have an atom of type
   *  'wave', which is parsed by the demuxer.
   *  Inside this atom, there is an atom 'OVHS', which
   *  containes the header packets encapsulated in
   *  ogg pages
   */
  
  else if(s->fourcc == BGAV_MK_FOURCC('O','g','g','V'))
    {
    ptr = s->ext_data;
    len = BGAV_PTR_2_32BE(ptr);ptr+=4;
    fourcc = BGAV_PTR_2_FOURCC(ptr);ptr+=4;
    if(fourcc != BGAV_MK_FOURCC('O','V','H','S'))
      {
      fprintf(stderr, "No OVHS Atom found\n");
      return 0;
      }
    //    fprintf(stderr, "Found OVHS Atom\n");

    buffer = ogg_sync_buffer(&priv->dec_oy, len - 8);
    memcpy(buffer, ptr, len - 8);
    ogg_sync_wrote(&priv->dec_oy, len - 8);

    if(!next_page(s))
      return 0;
    
    if(!next_packet(s))
      return 0;
    
    /* Initialize vorbis */
    
    //    fprintf(stderr, "Init vorbis %d\n", s->ext_size);
    //    bgav_hexdump(s->ext_data, s->ext_size, 16);
    
    if(vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc,
                                 &priv->dec_op) < 0)
      {
      fprintf(stderr, "decode: vorbis_synthesis_headerin: not a vorbis header\n");
      return 1;
      }
    
    if(!next_packet(s))
      return 0;
    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc, &priv->dec_op);
    if(!next_packet(s))
      return 0;
    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc, &priv->dec_op);
    }

  /* BGAV Way: Header packets are in extradata in ogg PACKETS */
    
  else if(s->fourcc == BGAV_VORBIS)
    {
    ptr = s->ext_data;
    ptr = parse_packet(&priv->dec_op, ptr);

    if(vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc,
                                 &priv->dec_op) < 0)
      {
      fprintf(stderr, "decode: vorbis_synthesis_headerin: not a vorbis header\n");
      bgav_hexdump(priv->dec_op.packet, priv->dec_op.bytes, 16);
      return 1;
      }
    ptr = parse_packet(&priv->dec_op, ptr);
    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc,
                              &priv->dec_op);
    parse_packet(&priv->dec_op, ptr);
    vorbis_synthesis_headerin(&priv->dec_vi, &priv->dec_vc,
                              &priv->dec_op);
    }

  
  vorbis_synthesis_init(&priv->dec_vd, &priv->dec_vi);
  vorbis_block_init(&priv->dec_vd, &priv->dec_vb);
  
  s->data.audio.format.sample_format   = GAVL_SAMPLE_FLOAT;
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_NONE;
  s->data.audio.format.samples_per_frame = 1024;

  priv->frame = gavl_audio_frame_create(NULL);
    
  gavl_set_channel_setup(&(s->data.audio.format));
  s->description = bgav_sprintf("Ogg Vorbis");
  return 1;
  }

static int decode_vorbis(bgav_stream_t * s, gavl_audio_frame_t * f, int num_samples)
  {
  vorbis_audio_priv * priv;
  int samples_copied;
  float ** channels;
  int i;
  int samples_decoded = 0;
  
  priv = (vorbis_audio_priv*)(s->data.audio.decoder->priv);
    
  while(samples_decoded < num_samples)
    {
    if(!priv->frame->valid_samples)
      {
      /* Decode stuff */
      
      while((priv->last_block_size =
             vorbis_synthesis_pcmout(&priv->dec_vd, &(channels))) < 1)
        {
        //        fprintf(stderr, "Decode Next Packet\n");
        if(!next_packet(s))
          {
          if(f)
            f->valid_samples = samples_decoded;
          return samples_decoded;
          }
        if(vorbis_synthesis(&priv->dec_vb, &priv->dec_op) == 0)
          {
          //          fprintf(stderr, "Decode Next Block\n");
          vorbis_synthesis_blockin(&priv->dec_vd,
                                   &priv->dec_vb);
          }
        }
      for(i = 0; i < s->data.audio.format.num_channels; i++)
        {
        priv->frame->channels.f[i] = channels[i];
        }
      //      fprintf(stderr, "Decoded %d samples\n", priv->last_block_size);
      priv->frame->valid_samples = priv->last_block_size;
      vorbis_synthesis_read(&priv->dec_vd,priv->last_block_size);
      }
    //    fprintf(stderr, "Copy samples out_pos: %d, in_pos: %d, out_size: %d, in_size: %d\n",
    //            f->valid_samples,
    //            priv->last_block_size - priv->frame->valid_samples,
    //            num_samples - f->valid_samples,
     //           priv->frame->valid_samples);
    samples_copied = gavl_audio_frame_copy(&(s->data.audio.format),
                                           f,
                                           priv->frame,
                                           samples_decoded, /* out_pos */
                                           priv->last_block_size - priv->frame->valid_samples,  /* in_pos */
                                           num_samples - samples_decoded, /* out_size, */
                                           priv->frame->valid_samples /* in_size */);
    priv->frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  if(f)
    f->valid_samples = samples_decoded;
  return samples_decoded;
  }

static void resync_vorbis(bgav_stream_t * s)
  {
  vorbis_audio_priv * priv;
  priv = (vorbis_audio_priv*)(s->data.audio.decoder->priv);
  //  fprintf(stderr, "Resync vorbis: %p\n", priv);
  vorbis_dsp_clear(&priv->dec_vd);
  vorbis_block_clear(&priv->dec_vb);

  if(s->fourcc != BGAV_VORBIS)
    {
    ogg_stream_clear(&priv->dec_os);
    ogg_sync_reset(&priv->dec_oy);
    priv->stream_initialized = 0;
    if(!next_page(s))
      return;
    ogg_sync_init(&priv->dec_oy);
    ogg_stream_init(&priv->dec_os, ogg_page_serialno(&priv->dec_og));
    }
  vorbis_synthesis_init(&priv->dec_vd, &priv->dec_vi);
  vorbis_block_init(&priv->dec_vd, &priv->dec_vb);

  priv->last_block_size = 0;
  priv->frame->valid_samples = 0;
  }

static void close_vorbis(bgav_stream_t * s)
  {
  vorbis_audio_priv * priv;
  priv = (vorbis_audio_priv*)(s->data.audio.decoder->priv);

  ogg_stream_clear(&priv->dec_os);
  ogg_sync_clear(&priv->dec_oy);
  vorbis_block_clear(&priv->dec_vb);
  vorbis_dsp_clear(&priv->dec_vd);
  vorbis_comment_clear(&priv->dec_vc);
  vorbis_info_clear(&priv->dec_vi);
  gavl_audio_frame_null(priv->frame);
  gavl_audio_frame_destroy(priv->frame);
  
  free(priv);
  }

static bgav_audio_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('O','g', 'g', 'S'),
                           BGAV_MK_FOURCC('O','g', 'g', 'V'),
                           BGAV_VORBIS,
                           BGAV_WAVID_2_FOURCC(0xfffe),
                           BGAV_WAVID_2_FOURCC(0x674f), // Mode 1  (header in first packet)
                           BGAV_WAVID_2_FOURCC(0x676f), // Mode 1+
                           BGAV_WAVID_2_FOURCC(0x6750), // Mode 2  (header in extradata)
                           BGAV_WAVID_2_FOURCC(0x6770), // Mode 2+
                           //                           BGAV_WAVID_2_FOURCC(0x6751), // Mode 3  (no header)
                           //                           BGAV_WAVID_2_FOURCC(0x6771), // Mode 3+
                           0x00 },
    name: "Ogg vorbis audio decoder",
    init:   init_vorbis,
    close:  close_vorbis,
    resync: resync_vorbis,
    decode: decode_vorbis
  };

void bgav_init_audio_decoders_vorbis()
  {
  bgav_audio_decoder_register(&decoder);
  }

/* Now comes the ugly part: Create a vorbis header from a bgav stream
   (needed for Vorbis type 3).

   It works the following: With the stream format (samplerate, bitrate,
   channels) we search in a table for the vorbis encoder parameters.
   Then, we start an encoder, get the header data and use them for decoding.

   WARNING: This code assumes, that the vorbis encoder always emits the same
   codebooks for a particular setup, which may not be the case across
   libvorbis versions :-(
*/

#define WORD  uint16_t
#define DWORD uint32_t
#define LONG  long

static const DWORD aSamplesPerSec[] = {
        48000, 44100, 22050, 11025
};

static const WORD aChannels[] = { 2, 1 };

static const WORD aBitsPerSample[] = { 16 };

static const DWORD aAvgBytesPerSec[][11] = {
        { 64000/8, 80000/8, 96000/8, 112000/8, 128000/8, 160000/8, 192000/8, 240000/8, 256000/8, 350000/8, 450000/8 }, // 48K,Stereo
        { 48000/8, 64000/8, 72000/8,  80000/8,  88000/8,  96000/8, 112000/8, 128000/8, 144000/8, 192000/8, 256000/8 }, // 48K,Mono
        { 64000/8, 80000/8, 96000/8, 112000/8, 128000/8, 160000/8, 192000/8, 240000/8, 256000/8, 350000/8, 450000/8 }, // 44K,Stereo
        { 48000/8, 64000/8, 72000/8,  80000/8,  88000/8,  96000/8, 112000/8, 128000/8, 144000/8, 192000/8, 256000/8 }, // 44K,Mono
        { 56000/8, 72000/8, 80000/8,  88000/8,  96000/8, 112000/8, 144000/8, 176000/8, 192000/8, 256000/8, 320000/8 }, // 22K,Stereo
        { 36000/8, 42000/8, 48000/8,  52000/8,  56000/8,  64000/8,  80000/8,  88000/8,  96000/8, 128000/8, 168000/8 }, // 22K,Mono
        { 36000/8, 44000/8, 50000/8,  52000/8,  56000/8,  64000/8,  80000/8,  96000/8, 112000/8, 144000/8, 168000/8 }, // 11K,Stereo
        { 22000/8, 26000/8, 28000/8,  30000/8,  32000/8,  34000/8,  40000/8,  48000/8,  56000/8,  72000/8,  88000/8 }, // 11K,Mono
};

static const float aQuality[][11] = {
        { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f }, // 48Kf,Stereo
        { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f }, // 48Kf,Mono
        { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f }, // 44Kf,Stereo
        { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f }, // 44Kf,Mono
        { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f }, // 22Kf,Stereo
        { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f }, // 22Kf,Mono
        { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f }, // 11Kf,Stereo
        { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f }, // 11Kf,Mono
};

typedef struct
{
        DWORD nSamplesPerSec;
        WORD  nChannels;
        WORD  wBitsPerSample;
        DWORD nAvgBytesPerSec;
        float flQuality;
} FORMATDETAIL;

static FORMATDETAIL aOggFormatIndexToDetail[] =
  {
    // WAVE_FORMAT_VORBIS(48K,16Bits,Stereo)
    // { 64000/8, 80000/8, 96000/8, 112000/8, 128000/8, 160000/8, 192000/8, 240000/8, 256000/8, 350000/8, 450000/8 }, // 48K,Stereo
    
    { 48000, 2, 16,  64000/8, 0.0f },
    { 48000, 2, 16,  80000/8, 0.1f },
    { 48000, 2, 16,  96000/8, 0.2f },
    { 48000, 2, 16, 112000/8, 0.3f },
    { 48000, 2, 16, 128000/8, 0.4f },
    { 48000, 2, 16, 160000/8, 0.5f },
    { 48000, 2, 16, 192000/8, 0.6f },
    { 48000, 2, 16, 240000/8, 0.7f },
    { 48000, 2, 16, 256000/8, 0.8f },
    { 48000, 2, 16, 350000/8, 0.9f },
    { 48000, 2, 16, 450000/8, 1.0f },
    // WAVE_FORMAT_VORBIS(48K,16Bits,Mono)
    // { 48000/8, 64000/8, 72000/8,  80000/8,  88000/8,  96000/8, 112000/8, 128000/8, 144000/8, 192000/8, 256000/8 }, // 48K,Mono

    { 48000, 1, 16,  48000/8, 0.0f },
    { 48000, 1, 16,  64000/8, 0.1f },
    { 48000, 1, 16,  72000/8, 0.2f },
    { 48000, 1, 16,  80000/8, 0.3f },
    { 48000, 1, 16,  88000/8, 0.4f },
    { 48000, 1, 16,  96000/8, 0.5f },
    { 48000, 1, 16, 112000/8, 0.6f },
    { 48000, 1, 16, 128000/8, 0.7f },
    { 48000, 1, 16, 144000/8, 0.8f },
    { 48000, 1, 16, 192000/8, 0.9f },
    { 48000, 1, 16, 256000/8, 1.0f },
    // WAVE_FORMAT_VORBIS(44K,16Bits,Stereo)
    // { 64000/8, 80000/8, 96000/8, 112000/8, 128000/8, 160000/8, 192000/8, 240000/8, 256000/8, 350000/8, 450000/8 }, // 44K,Stereo
    { 44100, 2, 16,  64000/8, 0.0f },
    { 44100, 2, 16,  80000/8, 0.1f },
    { 44100, 2, 16,  96000/8, 0.2f },
    { 44100, 2, 16, 112000/8, 0.3f },
    { 44100, 2, 16, 128000/8, 0.4f },
    { 44100, 2, 16, 160000/8, 0.5f },
    { 44100, 2, 16, 192000/8, 0.6f },
    { 44100, 2, 16, 240000/8, 0.7f },
    { 44100, 2, 16, 256000/8, 0.8f },
    { 44100, 2, 16, 350000/8, 0.9f },
    { 44100, 2, 16, 450000/8, 1.0f },
    // WAVE_FORMAT_VORBIS(44K,16Bits,Mono)
    // { 48000/8, 64000/8, 72000/8,  80000/8,  88000/8,  96000/8, 112000/8, 128000/8, 144000/8, 192000/8, 256000/8 }, // 44K,Mono
    { 44100, 1, 16,  48000/8, 0.0f },
    { 44100, 1, 16,  64000/8, 0.1f },
    { 44100, 1, 16,  72000/8, 0.2f },
    { 44100, 1, 16,  80000/8, 0.3f },
    { 44100, 1, 16,  88000/8, 0.4f },
    { 44100, 1, 16,  96000/8, 0.5f },
    { 44100, 1, 16, 112000/8, 0.6f },
    { 44100, 1, 16, 128000/8, 0.7f },
    { 44100, 1, 16, 144000/8, 0.8f },
    { 44100, 1, 16, 192000/8, 0.9f },
    { 44100, 1, 16, 256000/8, 1.0f },
    // WAVE_FORMAT_VORBIS(22K,16Bits,Stereo)
    // { 56000/8, 72000/8, 80000/8,  88000/8,  96000/8, 112000/8, 144000/8, 176000/8, 192000/8, 256000/8, 320000/8 }, // 22K,Stereo
    { 22050, 2, 16,  56000/8, 0.0f },
    { 22050, 2, 16,  72000/8, 0.1f },
    { 22050, 2, 16,  80000/8, 0.2f },
    { 22050, 2, 16,  88000/8, 0.3f },
    { 22050, 2, 16,  96000/8, 0.4f },
    { 22050, 2, 16, 112000/8, 0.5f },
    { 22050, 2, 16, 144000/8, 0.6f },
    { 22050, 2, 16, 176000/8, 0.7f },
    { 22050, 2, 16, 192000/8, 0.8f },
    { 22050, 2, 16, 256000/8, 0.9f },
    { 22050, 2, 16, 320000/8, 1.0f },
    // WAVE_FORMAT_VORBIS(22K,16Bits,Mono)
    // { 36000/8, 42000/8, 48000/8,  52000/8,  56000/8,  64000/8,  80000/8,  88000/8,  96000/8, 128000/8, 168000/8 }, // 22K,Mono
    { 22050, 1, 16,  36000/8, 0.0f },
    { 22050, 1, 16,  42000/8, 0.1f },
    { 22050, 1, 16,  48000/8, 0.2f },
    { 22050, 1, 16,  52000/8, 0.3f },
    { 22050, 1, 16,  56000/8, 0.4f },
    { 22050, 1, 16,  64000/8, 0.5f },
    { 22050, 1, 16,  80000/8, 0.6f },
    { 22050, 1, 16,  88000/8, 0.7f },
    { 22050, 1, 16,  96000/8, 0.8f },
    { 22050, 1, 16, 128000/8, 0.9f },
    { 22050, 1, 16, 168000/8, 1.0f },
    // WAVE_FORMAT_VORBIS(11K,16Bits,Stereo)
    // { 36000/8, 44000/8, 50000/8,  52000/8,  56000/8,  64000/8,  80000/8,  96000/8, 112000/8, 144000/8, 168000/8 }, // 11K,Stereo
    { 11025, 2, 16,  36000/8, 0.0f },
    { 11025, 2, 16,  44000/8, 0.1f },
    { 11025, 2, 16,  50000/8, 0.2f },
    { 11025, 2, 16,  52000/8, 0.3f },
    { 11025, 2, 16,  56000/8, 0.4f },
    { 11025, 2, 16,  64000/8, 0.5f },
    { 11025, 2, 16,  80000/8, 0.6f },
    { 11025, 2, 16,  96000/8, 0.7f },
    { 11025, 2, 16, 112000/8, 0.8f },
    { 11025, 2, 16, 144000/8, 0.9f },
    { 11025, 2, 16, 168000/8, 1.0f },
    // WAVE_FORMAT_VORBIS(11K,16Bits,Mono)
    // { 22000/8, 26000/8, 28000/8,  30000/8,  32000/8,  34000/8,  40000/8,  48000/8,  56000/8,  72000/8,  88000/8 }, // 11K,Mono

    { 11025, 1, 16,  22000/8, 0.0f },
    { 11025, 1, 16,  26000/8, 0.1f },
    { 11025, 1, 16,  28000/8, 0.2f },
    { 11025, 1, 16,  30000/8, 0.3f },
    { 11025, 1, 16,  32000/8, 0.4f },
    { 11025, 1, 16,  34000/8, 0.5f },
    { 11025, 1, 16,  40000/8, 0.6f },
    { 11025, 1, 16,  48000/8, 0.7f },
    { 11025, 1, 16,  56000/8, 0.8f },
    { 11025, 1, 16,  72000/8, 0.9f },
    { 11025, 1, 16,  88000/8, 1.0f },
  };

#define ARRAYLEN(arr) (sizeof(arr)/sizeof(arr[0]))

DWORD oggFormatToIndex(const bgav_stream_t *pwfx)
  {
  DWORD index = 0;
  LONG  delta = LONG_MAX;
  DWORD n;
  LONG d;
        
  for(n=0; n<ARRAYLEN(aOggFormatIndexToDetail); n++)
    {
    if(pwfx->data.audio.format.samplerate!=aOggFormatIndexToDetail[n].nSamplesPerSec) continue;
    if(pwfx->data.audio.format.num_channels!=aOggFormatIndexToDetail[n].nChannels     ) continue;
    d = pwfx->container_bitrate/8 - aOggFormatIndexToDetail[n].nAvgBytesPerSec;
    if(d==0)
      {
      index = n;
      break;
      }
    if(abs(d)<abs(delta))
      {
      index = n;
      delta = d;
      }
    }
  return index;
  }

#define MEMZERO(p) memset(&p, 0, sizeof(p))

static char * get_default_vorbis_header(bgav_stream_t * stream, int * len)
  {
  int bitrate;

  char * ret = (char*)0;
  int ret_len = 0;

  vorbis_info vi;
  vorbis_block vb;
  vorbis_comment vc;
  vorbis_dsp_state vd;

  ogg_packet header_main;
  ogg_packet header_comments;
  ogg_packet header_codebooks;
  ogg_stream_state os;
  ogg_page og;
  
  int format_index;

  /* Initialize */

  MEMZERO(vi);
  MEMZERO(vb);
  MEMZERO(vc);
  MEMZERO(vd);

  MEMZERO(header_main);
  MEMZERO(header_comments);
  MEMZERO(header_codebooks);
  MEMZERO(os);
  MEMZERO(og);
    
  /* Get format description */
  
  format_index = oggFormatToIndex(stream);

  /* Fire up an encoder */
  
  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);
  vorbis_comment_add_tag(&vc,"ENCODER","vorbis.acm"); /* LOL */

  //  fprintf(stderr, "Quality: %f, Bitrate: %d\n",
  //          aOggFormatIndexToDetail[format_index].flQuality, stream->container_bitrate);

  bitrate = aOggFormatIndexToDetail[format_index].nAvgBytesPerSec * 8;
  
  vorbis_encode_init_vbr(&vi,stream->data.audio.format.num_channels,
                         stream->data.audio.format.num_channels,aOggFormatIndexToDetail[format_index].flQuality);
  
  //  vorbis_encode_init(&vi,stream->data.audio.format.num_channels,
  //                     stream->data.audio.format.num_channels,bitrate, bitrate, bitrate);
  
  vorbis_analysis_init(&vd,&vi);
  vorbis_block_init(&vd,&vb);

  vorbis_analysis_headerout(&vd,&vc,&header_main,&header_comments,&header_codebooks);

  ogg_stream_init(&os,0);
  ogg_stream_packetin(&os,&header_main);
  ogg_stream_packetin(&os,&header_comments);
  ogg_stream_packetin(&os,&header_codebooks);

  /* Extract header pages */

  while(ogg_stream_flush(&os, &og))
    {
    //    fprintf(stderr, "Got header page %ld + %ld bytes\n",
    //            codec->enc_og.header_len, codec->enc_og.body_len);
    
    ret = realloc(ret, ret_len + og.header_len + og.body_len);
     
    memcpy(ret + ret_len, og.header, og.header_len);
    memcpy(ret + ret_len + og.header_len, og.body, og.body_len);

    ret_len += og.header_len + og.body_len;
    }

  ogg_stream_clear(&os);
  vorbis_block_clear(&vb);
  vorbis_dsp_clear(&vd);
  vorbis_comment_clear(&vc);
  vorbis_info_clear(&vi);
  
  if(len)
    *len = ret_len;
  return ret;
  }
