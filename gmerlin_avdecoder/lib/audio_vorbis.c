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

#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>

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
  //  fprintf(stderr, "Read data\n");
  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;
  
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
  
  if(s->fourcc == BGAV_MK_FOURCC('O','g', 'g', 'S'))
    {
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
  else if(s->fourcc == BGAV_WAVID_2_FOURCC(0x674f))
    {
    fprintf(stderr, "Extradata:\n");
    bgav_hexdump(s->ext_data, s->ext_size, 16);
    }
  else if(s->fourcc == BGAV_WAVID_2_FOURCC(0x6750))
    {
    fprintf(stderr, "Extradata:\n");
    bgav_hexdump(s->ext_data, s->ext_size, 16);
    }
  else if(s->fourcc == BGAV_WAVID_2_FOURCC(0x6751))
    {
    fprintf(stderr, "Extradata:\n");
    bgav_hexdump(s->ext_data, s->ext_size, 16);
    }
  
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
                           BGAV_WAVID_2_FOURCC(0x674f),
                           BGAV_WAVID_2_FOURCC(0x6750),
                           BGAV_WAVID_2_FOURCC(0x6751),
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

