/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

#include <string.h>
#include <stdlib.h>

#include <config.h>

#include <gavl/numptr.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "oggvorbis"

#include <gmerlin/translation.h>

#include <vorbis/vorbisenc.h>
#include "ogg_common.h"

#include <gavl/metatags.h>


#define BITRATE_MODE_VBR         0
#define BITRATE_MODE_VBR_BITRATE 1
#define BITRATE_MODE_MANAGED     2

typedef struct
  {
  /* Ogg vorbis stuff */
    
  //  ogg_page enc_og;
  vorbis_info enc_vi;
  vorbis_comment enc_vc;
  vorbis_dsp_state enc_vd;
  vorbis_block enc_vb;

  bg_ogg_encoder_t * output;
  
  /* Options */

  int managed;

  int bitrate_mode;
  
  int min_bitrate;
  int nominal_bitrate;
  int max_bitrate;

  float quality;     /* Float from 0 to 1 (low->high) */
  int quality_set;

  int64_t samples_read;
  
  gavl_audio_format_t * format;
  gavl_audio_frame_t * frame;

  gavl_packet_sink_t * psink;

  int64_t pts;
  
  } vorbis_t;

static void * create_vorbis()
  {
  vorbis_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "bitrate_mode",
      .long_name =   TRS("Bitrate mode"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "VBR" },
      .multi_names = (char const *[]){ "vbr", "vbr_bitrate", "managed", NULL },
      .multi_labels = (char const *[]){ TRS("VBR"), TRS("VBR (bitrate)"), TRS("Managed"), NULL },
      .help_string = TRS("Bitrate mode:\n\
VBR: You specify a quality and (optionally) a minimum and maximum bitrate\n\
VBR (bitrate): The specified nominal bitrate will be used for selecting the encoder mode.\n\
Managed: You specify a nominal bitrate and (optionally) a minimum and maximum bitrate\n\
VBR is recommended, managed bitrate might result in a worse quality")
    },
    {
      .name =        "nominal_bitrate",
      .long_name =   TRS("Nominal bitrate (kbps)"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 1000 },
      .val_default = { .val_i = 128 },
      .help_string = TRS("Nominal bitrate (in kbps) for managed mode"),
    },
    {
      .name =      "quality",
      .long_name = TRS("VBR Quality (10: best)"),
      .type =      BG_PARAMETER_SLIDER_FLOAT,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 10.0 },
      .val_default = { .val_f = 3.0 },
      .num_digits =  1,
      .help_string = TRS("Quality for VBR mode\n\
10: best (largest output file)\n\
0:  worst (smallest output file)"),
    },
    {
      .name =        "min_bitrate",
      .long_name =   TRS("Minimum bitrate (kbps)"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 1000 },
      .val_default = { .val_i = 0 },
      .help_string = TRS("Optional minimum bitrate (in kbps)\n\
0 = unspecified"),
    },
    {
      .name =        "max_bitrate",
      .long_name =   TRS("Maximum bitrate (kbps)"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 1000 },
      .val_default = { .val_i = 0 },
      .help_string = TRS("Optional maximum bitrate (in kbps)\n\
0 = unspecified"),
    },
    { /* End of parameters */ }
  };


static const bg_parameter_info_t * get_parameters_vorbis()
  {
  return parameters;
  }

static void build_comment(vorbis_comment * vc, const gavl_metadata_t * metadata)
  {
  const char * val;
  
  vorbis_comment_init(vc);
  
  if((val = gavl_metadata_get(metadata, GAVL_META_ARTIST)))
    vorbis_comment_add_tag(vc, "ARTIST", val);

  if((val = gavl_metadata_get(metadata, GAVL_META_ALBUMARTIST)))
    {
    vorbis_comment_add_tag(vc, "ALBUMARTIST", val);
    vorbis_comment_add_tag(vc, "ALBUM ARTIST", val);
    }
  
  if((val = gavl_metadata_get(metadata, GAVL_META_TITLE)))
    vorbis_comment_add_tag(vc, "TITLE", val);

  if((val = gavl_metadata_get(metadata, GAVL_META_ALBUM)))
    vorbis_comment_add_tag(vc, "ALBUM", val);
    
  if((val = gavl_metadata_get(metadata, GAVL_META_GENRE)))
    vorbis_comment_add_tag(vc, "GENRE", val);


  if((val = gavl_metadata_get(metadata, GAVL_META_DATE)))
    vorbis_comment_add_tag(vc, "DATE", val);
  else if((val = gavl_metadata_get(metadata, GAVL_META_YEAR)))
    vorbis_comment_add_tag(vc, "DATE", val);
  
  if((val = gavl_metadata_get(metadata, GAVL_META_COPYRIGHT)))
    vorbis_comment_add_tag(vc, "COPYRIGHT", val);

  if((val = gavl_metadata_get(metadata, GAVL_META_TRACKNUMBER)))
    vorbis_comment_add_tag(vc, "TRACKNUMBER", val);
  
  if((val = gavl_metadata_get(metadata, GAVL_META_COMMENT)))
    vorbis_comment_add(vc, val);
  }

#define PTR_2_32BE(p) \
  ((*(p) << 24) |     \
   (*(p+1) << 16) |   \
   (*(p+2) << 8) |    \
   *(p+3))

#define INT_32BE_2_PTR(num, p) \
  (p)[0] = ((num)>>24) & 0xff; \
  (p)[1] = ((num)>>16) & 0xff; \
  (p)[2] = ((num)>>8) & 0xff;  \
  (p)[3] = (num) & 0xff;

#define PTR_2_32LE(p) \
  ((*(p+3) << 24) |     \
   (*(p+2) << 16) |   \
   (*(p+1) << 8) |    \
   *(p))

#define WRITE_STRING(s, p) string_len = strlen(s); \
  GAVL_32LE_2_PTR(string_len, p); p += 4; \
  memcpy(p, s, string_len); \
  p+=string_len;

static const uint8_t comment_header[7] = { 0x03, 'v', 'o', 'r', 'b', 'i', 's' };

static uint8_t * create_comment_packet(vorbis_comment * comment, int * len1)
  {
  int i;
  uint8_t * ret, * ptr;
  int string_len;
  // comment_header + Vendor length + num_user_comments + framing bit
  int len = 7 + 4 + 4 + 1 + strlen(comment->vendor); 
  
  for(i = 0; i < comment->comments; i++)
    {
    len += 4 + strlen(comment->user_comments[i]);
    }
  
  ret = malloc(len);
  ptr = ret;

  /* Comment header */
  memcpy(ptr, comment_header, 7);
  ptr += 7;
  
  /* Vendor length + vendor */
  WRITE_STRING(comment->vendor, ptr);

  GAVL_32LE_2_PTR(comment->comments, ptr); ptr += 4;

  for(i = 0; i < comment->comments; i++)
    {
    WRITE_STRING(comment->user_comments[i], ptr);
    }
  *ptr = 0x01;
  
  *len1 = len;
  return ret;
  }

static int init_compressed_vorbis(bg_ogg_stream_t * s)
  {
  ogg_packet packet;
  uint8_t * ptr;
  uint32_t len;
  uint32_t vendor_len;

  uint8_t * comment_packet;
  int comment_len;
  vorbis_t * vorbis = s->codec_priv;
  
  memset(&packet, 0, sizeof(packet));
  
  /* Write ID packet */
  ptr = s->ci.global_header;
  
  len = PTR_2_32BE(ptr); ptr += 4;

  packet.packet = ptr;
  packet.bytes  = len;
  packet.b_o_s = 1;

  if(!bg_ogg_stream_write_header_packet(s, &packet))
    return 0;
  
  ptr += len;
  
  /* Comment is more complicated: We need to copy the vendor field from the
     original stream, because it didn't get compressed by our libvorbis */

  /* Build comment (comments are UTF-8, good for us :-) */
  len = PTR_2_32BE(ptr); ptr += 4;
  
  build_comment(&vorbis->enc_vc, s->m_global);
  
  /* Copy the vendor id */
  vendor_len = PTR_2_32LE(ptr + 7);
  vorbis->enc_vc.vendor = calloc(1, vendor_len + 1);
  memcpy(vorbis->enc_vc.vendor, ptr + 11, vendor_len);
  // fprintf(stderr, "Got vendor %s\n", vorbis->enc_vc.vendor);
  
  comment_packet = create_comment_packet(&vorbis->enc_vc, &comment_len);
  packet.packet = comment_packet;
  packet.bytes  = comment_len;
  packet.b_o_s = 0;

  if(!bg_ogg_stream_write_header_packet(s, &packet))
    return 0;
  
  free(comment_packet);

  free(vorbis->enc_vc.vendor);
  vorbis->enc_vc.vendor = NULL;
  
  ptr += len;
  
  /* Codepages */
  len = PTR_2_32BE(ptr); ptr += 4;
  
  packet.packet = ptr;
  packet.bytes  = len;
  
  if(!bg_ogg_stream_write_header_packet(s, &packet))
    return 0;
  return 1;
  }

static void set_packet_sink(void * data, gavl_packet_sink_t * psink)
  {
  vorbis_t * vorbis = data;
  vorbis->psink = psink;
  }

static int flush_packet(vorbis_t * vorbis, ogg_packet * op)
  {
  gavl_packet_t gp;
  gavl_packet_init(&gp);

  bg_ogg_packet_to_gavl(op, &gp, &vorbis->pts);
  
  //  ogg_stream_packetin(&vorbis->enc_os,op);
  return 1;
  }

static int flush_data(vorbis_t * vorbis, int force)
  {
  ogg_packet op;
  memset(&op, 0, sizeof(op));
  /* While we can get enough data from the library to analyse, one
     block at a time... */

  while(vorbis_analysis_blockout(&vorbis->enc_vd,&vorbis->enc_vb)==1)
    {
    if(vorbis->managed)
      {
      /* Do the main analysis, creating a packet */
      vorbis_analysis(&vorbis->enc_vb, NULL);
      vorbis_bitrate_addblock(&vorbis->enc_vb);
      
      while(vorbis_bitrate_flushpacket(&vorbis->enc_vd, &op))
        {
        /* Add packet to bitstream */
        if(!flush_packet(vorbis, &op))
          return 0;
        }
      }
    else
      {
      vorbis_analysis(&vorbis->enc_vb, &op);
      /* Add packet to bitstream */
      if(!flush_packet(vorbis, &op))
        return 0;
      }
    }
  /* Flush pages if any */
  //  if((result = bg_ogg_flush(&vorbis->enc_os, vorbis->output, force)) <= 0)
  //    return result;
  return 1;
  }

static gavl_sink_status_t write_audio_frame_vorbis(void * data, gavl_audio_frame_t * frame)
  {
  int i;
  vorbis_t * vorbis;
  float **buffer;
     
  vorbis = data;

  buffer = vorbis_analysis_buffer(&vorbis->enc_vd, frame->valid_samples);

  for(i = 0; i < vorbis->format->num_channels; i++)
    {
    vorbis->frame->channels.f[i] = buffer[i];
    }
  gavl_audio_frame_copy(vorbis->format,
                        vorbis->frame,
                        frame,
                        0, 0, frame->valid_samples, frame->valid_samples);
  vorbis_analysis_wrote(&vorbis->enc_vd, frame->valid_samples);
  if(flush_data(vorbis, 0) < 0)
    return GAVL_SINK_ERROR;

  vorbis->samples_read += frame->valid_samples;
  return GAVL_SINK_OK;
  }


static gavl_audio_sink_t * init_vorbis(void * data,
                                       gavl_audio_format_t * format,
                                       gavl_metadata_t * stream_metadata,
                                       gavl_compression_info_t * ci_ret)
  {
  ogg_packet header_main;
  ogg_packet header_comments;
  ogg_packet header_codebooks;
  //  struct ovectl_ratemanage2_arg ai;

  vorbis_t * vorbis = data;
  uint8_t * ptr;
  char * vendor;
  int vendor_len;
  
  vorbis->format = format;
  vorbis->frame = gavl_audio_frame_create(NULL);
  
  vorbis->managed = 0;
  
  /* Adjust the format */

  vorbis->format->interleave_mode = GAVL_INTERLEAVE_NONE;
  vorbis->format->sample_format = GAVL_SAMPLE_FLOAT;
  bg_ogg_set_vorbis_channel_setup(vorbis->format);
  
  vorbis_info_init(&vorbis->enc_vi);

  /* VBR Initialization */

  switch(vorbis->bitrate_mode)
    {
    case BITRATE_MODE_VBR:
      vorbis_encode_init_vbr(&vorbis->enc_vi, vorbis->format->num_channels,
                              vorbis->format->samplerate, vorbis->quality);
      break;
    case BITRATE_MODE_VBR_BITRATE:
      vorbis_encode_setup_managed(&vorbis->enc_vi,
                                  vorbis->format->num_channels,
                                  vorbis->format->samplerate,-1,128000,-1);
      vorbis_encode_ctl(&vorbis->enc_vi,OV_ECTL_RATEMANAGE2_SET,NULL);
      vorbis_encode_setup_init(&vorbis->enc_vi);
      break;
    case BITRATE_MODE_MANAGED:
      vorbis_encode_init(&vorbis->enc_vi, vorbis->format->num_channels,
                         vorbis->format->samplerate,
                         vorbis->max_bitrate>0 ? vorbis->max_bitrate : -1,
                         vorbis->nominal_bitrate,
                         vorbis->min_bitrate>0 ? vorbis->min_bitrate : -1);
      vorbis->managed = 1;
      break;
    }
  
  vorbis_analysis_init(&vorbis->enc_vd,&vorbis->enc_vi);
  vorbis_block_init(&vorbis->enc_vd,&vorbis->enc_vb);
  
  /* Build the packets */
  vorbis_analysis_headerout(&vorbis->enc_vd,&vorbis->enc_vc,
                            &header_main,&header_comments,&header_codebooks);

  /* Extract vendor ID */
  ptr = header_comments.packet + 7;
  vendor_len = PTR_2_32LE(ptr); ptr += 4;
  vendor = calloc(1, vendor_len + 1);
  memcpy(vendor, ptr, vendor_len);
  gavl_metadata_set_nocpy(stream_metadata, GAVL_META_SOFTWARE, vendor);
  
  /* And stream them out */
  
  ci_ret->global_header_len =
    header_main.bytes + header_comments.bytes + header_codebooks.bytes + 12;
  
  ci_ret->global_header = malloc(ci_ret->global_header_len);
  ptr = ci_ret->global_header;

  GAVL_32LE_2_PTR(header_main.bytes, ptr); ptr += 4;
  memcpy(ptr, header_main.packet, header_main.bytes);
  ptr += header_main.bytes;

  GAVL_32LE_2_PTR(header_comments.bytes, ptr); ptr += 4;
  memcpy(ptr, header_comments.packet, header_comments.bytes);
  ptr += header_comments.bytes;

  GAVL_32LE_2_PTR(header_codebooks.bytes, ptr); ptr += 4;
  memcpy(ptr, header_codebooks.packet, header_codebooks.bytes);
  ptr += header_codebooks.bytes;
  
  ci_ret->id = GAVL_CODEC_ID_VORBIS;
  return gavl_audio_sink_create(NULL, write_audio_frame_vorbis, vorbis, vorbis->format);
  }

static void set_parameter_vorbis(void * data, const char * name,
                                 const bg_parameter_value_t * v)
  {
  vorbis_t * vorbis;
  vorbis = data;
  
  if(!name)
    {
    return;
    }
  else if(!strcmp(name, "nominal_bitrate"))
    {
    vorbis->nominal_bitrate = v->val_i * 1000;
    if(vorbis->nominal_bitrate < 0)
      vorbis->nominal_bitrate = -1;
    }
  else if(!strcmp(name, "min_bitrate"))
    {
    vorbis->min_bitrate = v->val_i * 1000;
    if(vorbis->min_bitrate < 0)
      vorbis->min_bitrate = -1;
    }
  else if(!strcmp(name, "max_bitrate"))
    {
    vorbis->max_bitrate = v->val_i * 1000;
    if(vorbis->max_bitrate < 0)
      vorbis->max_bitrate = -1;
    }
  else if(!strcmp(name, "quality"))
    {
    vorbis->quality = v->val_f * 0.1;
    }
  else if(!strcmp(name, "bitrate_mode"))
    {
    if(!strcmp(v->val_str, "vbr"))
      vorbis->bitrate_mode = BITRATE_MODE_VBR;
    else if(!strcmp(v->val_str, "vbr_bitrate"))
      vorbis->bitrate_mode = BITRATE_MODE_VBR_BITRATE;
    else if(!strcmp(v->val_str, "managed"))
      vorbis->bitrate_mode = BITRATE_MODE_MANAGED;
    }
  }



#if 0
static int write_packet_vorbis(void * data, gavl_packet_t * packet)
  {
  ogg_packet op;
  
  vorbis_t * vorbis = data;
  
  memset(&op, 0, sizeof(op));

  op.packet = packet->data;
  op.bytes = packet->data_len;
  op.granulepos = packet->pts + packet->duration;
  if(packet->flags & GAVL_PACKET_LAST)
    op.e_o_s = 1;
  
  ogg_stream_packetin(&vorbis->s->os, &op);
  
  /* Flush pages if any */
  if(bg_ogg_stream_flush(vorbis->s, 0) < 0)
    return 0;
  return 1;
  }
#endif

static int close_vorbis(void * data)
  {
  int ret = 1;
  vorbis_t * vorbis;
  int result;
  vorbis = data;

  if(vorbis->samples_read)
    {
    vorbis_analysis_wrote(&vorbis->enc_vd, 0);
    result = flush_data(vorbis, 1);
    if(result < 0)
      ret = 0;
    }
  
  vorbis_block_clear(&vorbis->enc_vb);
  vorbis_dsp_clear(&vorbis->enc_vd);
  vorbis_comment_clear(&vorbis->enc_vc);
  vorbis_info_clear(&vorbis->enc_vi);

  if(vorbis->frame)
    gavl_audio_frame_destroy(vorbis->frame);
  
  free(vorbis);
  return ret;
  }



const bg_ogg_codec_t bg_vorbis_codec =
  {
    .name      = "vorbis",
    .long_name = TRS("Vorbis encoder"),
    .create    = create_vorbis,

    .get_parameters = get_parameters_vorbis,
    .set_parameter =  set_parameter_vorbis,
    
    .init_audio            = init_vorbis,
    .init_audio_compressed = init_compressed_vorbis,
    
    //    .encode_audio = write_audio_frame_vorbis,
    //    .write_packet = write_packet_vorbis,
    
    .close = close_vorbis,
    .set_packet_sink = set_packet_sink,
  };
