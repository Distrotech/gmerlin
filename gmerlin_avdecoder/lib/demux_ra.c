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

#include <avdec_private.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LOG_DOMAIN "realaudio"

/* RealAudio demuxer inspired by xine */

typedef struct
  {
  uint32_t data_start;
  uint32_t data_size;
  uint16_t ext_data[5];
  int sub_packet_h;
  } ra_priv_t;

static int probe_ra(bgav_input_context_t * input)
  {
  uint8_t sig[3];
  
  if(bgav_input_get_data(input, sig, 3) < 3)
    return 0;
  if((sig[0] == '.') &&
     (sig[1] == 'r') &&
     (sig[2] == 'a'))
    return 1;
  return 0;
  }

#define RA_FILE_HEADER_PREV_SIZE 22

static int open_ra(bgav_demuxer_context_t * ctx,
                   bgav_redirector_context_t ** redir)
  {
  bgav_charset_converter_t * charset_cnv;
  
  uint8_t fh[RA_FILE_HEADER_PREV_SIZE], len;
  uint8_t * file_header;
  uint8_t  *audio_header;
  uint32_t  hdr_size;
  uint16_t  version;
  int framesize;
  int offset;
  bgav_stream_t * s;
  ra_priv_t * priv;
  int sub_packet_size   = 0;
  //  int sub_packet_height = 0;
  int codec_flavor      = 0;
  int coded_framesize   = 0;
  bgav_track_t * track;
    
  /* Create track */

  ctx->tt = bgav_track_table_create(1);

  track = ctx->tt->cur;
    
  if(bgav_input_read_data(ctx->input, fh, RA_FILE_HEADER_PREV_SIZE) < RA_FILE_HEADER_PREV_SIZE)
    {
    return 0;
    }
  file_header = fh;
  
  if((file_header[0] != '.') || 
     (file_header[1] != 'r') || 
     (file_header[2] != 'a'))
    {
    return 0;
    }
  
  version = BGAV_PTR_2_16BE(file_header + 0x04);
  /* read header size according to version */
  if (version == 3)
    hdr_size = BGAV_PTR_2_16BE(file_header + 0x06) + 8;
  else if (version == 4)
    hdr_size = BGAV_PTR_2_32BE(file_header + 0x12) + 16;
  else
    {
    return 0;
    }
  
  /* allocate for and read header data */
  audio_header = malloc(hdr_size);
  memcpy(audio_header, file_header, RA_FILE_HEADER_PREV_SIZE);
           
  if(bgav_input_read_data(ctx->input,
                          &(audio_header[RA_FILE_HEADER_PREV_SIZE]),
                            hdr_size - RA_FILE_HEADER_PREV_SIZE) < hdr_size - RA_FILE_HEADER_PREV_SIZE)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Unable to read header");
    free(audio_header);
    return 0;
    }
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  s = bgav_track_add_audio_stream(track, ctx->opt);
  priv->sub_packet_h = 1;
  /* read header data according to version */
  if((version == 3) && (hdr_size >= 32))
    {
    priv->data_size = BGAV_PTR_2_32BE(audio_header + 0x12);

    s->data.audio.format.num_channels = 1;
    s->data.audio.format.samplerate = 8000;
    s->data.audio.block_align = 240;
    s->data.audio.bits_per_sample = 16;
    offset = 0x16;
    }
  else if(hdr_size >= 72)
    {
    priv->data_size = BGAV_PTR_2_32BE(audio_header + 0x1C);    
    s->data.audio.block_align = BGAV_PTR_2_16BE(audio_header + 0x2A);
    s->data.audio.format.samplerate = BGAV_PTR_2_16BE(audio_header + 0x30);
    
    s->data.audio.bits_per_sample     = audio_header[0x35];
    s->data.audio.format.num_channels = audio_header[0x37];

    sub_packet_size    = BGAV_PTR_2_16BE(audio_header + 0x2C);
    priv->sub_packet_h       = BGAV_PTR_2_16BE(audio_header + 0x28);    
    framesize = BGAV_PTR_2_16BE(audio_header + 0x2A);
    codec_flavor       = BGAV_PTR_2_16BE(audio_header + 0x16);
    coded_framesize    = BGAV_PTR_2_32BE(audio_header + 0x18); 

    if(audio_header[0x3D] == 4)
      s->fourcc = BGAV_PTR_2_FOURCC(audio_header+0x3E);
    else
      {
      free(audio_header);
      return 0;
      }
    offset = 0x45;
    }
  else
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Header too small");
    free(audio_header);
    return 0;
    }

  charset_cnv = bgav_charset_converter_create(ctx->opt, "ISO-8859-1", "UTF-8");
  
  /* Read title */
  len = audio_header[offset];
  if(len && ((offset+len+2) < hdr_size))
    {
    track->metadata.title = bgav_convert_string(charset_cnv,
                                                (char*)(audio_header +offset+1), len,
                                                (int*)0);
    offset += len+1;
    }
  else
    offset++;
  
  /* Author */
  len = audio_header[offset];
  if(len && ((offset+len+1) < hdr_size))
    {
    track->metadata.author = bgav_convert_string(charset_cnv,
                                                (char*)(audio_header +offset+1), len,
                                                (int*)0);
    offset += len+1;
    }
  else
    offset++;
  
  /* Copyright/Date */
  len = audio_header[offset];
  if(len && ((offset+len+1) <= hdr_size))
    {
    track->metadata.copyright = bgav_convert_string(charset_cnv,
                                                    (char*)(audio_header +offset+1), len,
                                                    (int*)0);
    offset += len+1;
    }
  else
    offset++;

  bgav_charset_converter_destroy(charset_cnv);
  
  /* Fourcc for version 3 comes after meta info */
  if((version == 3) && ((offset+7) <= hdr_size))
    {
    if(audio_header[offset+2] == 4)
      s->fourcc = BGAV_PTR_2_FOURCC(audio_header + offset+3);
    else
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Invalid fourcc size %d",
               audio_header[offset+2]);
      free(audio_header);
      return 0;
      }
    }

  /* Set extradata */

  switch(s->fourcc)
    {
    case BGAV_MK_FOURCC('1', '4', '_', '4'):
    case BGAV_MK_FOURCC('l', 'p', 'c', 'J'):
      priv->ext_data[0] = 0;
      priv->ext_data[1] = 240;
      priv->ext_data[2] = 0;
      priv->ext_data[3] = 0x14;
      priv->ext_data[4] = 0;
      s->ext_data = (uint8_t*)priv->ext_data;
      s->ext_size = 10;
      break;
    case BGAV_MK_FOURCC('2', '8', '_', '8'):
      priv->ext_data[0]=0;
      priv->ext_data[1]=priv->sub_packet_h;
      priv->ext_data[2]=codec_flavor;
      priv->ext_data[3]=coded_framesize;
      priv->ext_data[4]=0;
      s->ext_data = (uint8_t*)priv->ext_data;
      s->ext_size = 10;
      break;
    case BGAV_MK_FOURCC('d', 'n', 'e', 't'):
      priv->ext_data[0]=0;
      priv->ext_data[1]=priv->sub_packet_h;
      priv->ext_data[2]=codec_flavor;
      priv->ext_data[3]=coded_framesize;
      priv->ext_data[4]=0;
      s->ext_data = (uint8_t*)priv->ext_data;
      s->ext_size = 10;
      break;
    }

  
  ctx->stream_description = bgav_sprintf("Real audio Version %d", version);
  return 1;
  }

static void swap_bytes_dnet(uint8_t * data, int len)
  {
  int i;
  uint8_t swp;
  
  for(i = 0; i < len/2; i++)
    {
    swp = data[0];
    data[0] = data[1];
    data[1] = swp;
    data += 2;
    }
  
  }

static int next_packet_ra(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * s;
  ra_priv_t * priv;
  int len;
  priv = (ra_priv_t *)ctx->priv;
  
  s = &(ctx->tt->cur->audio_streams[0]);
  p = bgav_stream_get_packet_write(s);

  len = s->data.audio.block_align * priv->sub_packet_h;
  bgav_packet_alloc(p, len);
  if(bgav_input_read_data(ctx->input, p->data, len) < len)
    return 0;
  
  p->data_size = len;

  if(s->fourcc == BGAV_MK_FOURCC('d', 'n', 'e', 't'))
    swap_bytes_dnet(p->data, p->data_size);
  
  p->keyframe = 1;
  bgav_packet_done_write(p);
  return 1;
  }

#if 0
static void seek_ra(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {

  }
#endif

static void close_ra(bgav_demuxer_context_t * ctx)
  {
  
  }


const bgav_demuxer_t bgav_demuxer_ra =
  {
    .probe =       probe_ra,
    .open =        open_ra,
    .next_packet = next_packet_ra,
    //    .seek =        seek_ra,
    .close =       close_ra
  };
