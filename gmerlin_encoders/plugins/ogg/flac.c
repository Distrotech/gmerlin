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
#include <errno.h>

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "oggflac"

#include <gavl/numptr.h>


#include <ogg/ogg.h>
#include "ogg_common.h"

#include <bgflac.h>
#include <vorbiscomment.h>

/* Way too large but save */
#define BUFFER_SIZE (MAX_BYTES_PER_FRAME*10)

typedef struct
  {
  bg_flac_t * enc;
  
  int header_written;
    
  /* This is ugly: To properly set the e_o_s flag of the last
     page, we must cache the last frame and write it delayed. */
  
  uint8_t * frame;
  int frame_size;
  int frame_alloc;
  int64_t samples_encoded;
  int64_t frames_encoded;
  
  int frame_samples;

  int write_error;

  } flacogg_t;

static int init_compressed_flacogg(bg_ogg_stream_t * s)
  {
  ogg_packet op;
  int len;
  uint8_t * ptr;
  
  uint8_t header_bytes[] =
    {
      0x7f,
      0x46, 0x4C, 0x41, 0x43, // FLAC
      0x01, 0x00, // Major, minor version
      0x00, 0x01, // Number of other header packets (Big Endian)
    };
  
  memset(&op, 0, sizeof(op));
  
  /* Not the last metadata packet */
  s->ci.global_header[4] &= 0x7f;

  op.packet = malloc(9 + s->ci.global_header_len);
  
  memcpy(op.packet, header_bytes, 9);
  memcpy(op.packet+9, s->ci.global_header,
         s->ci.global_header_len);

  op.bytes  = s->ci.global_header_len + 9;
  
  if(!bg_ogg_stream_write_header_packet(s, &op))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Got no Flac ID page");
    free(op.packet);
    return 0;
    }
  
  free(op.packet);
  
  /* Vorbis comment */
  len = bg_vorbis_comment_bytes(&s->m_stream, s->m_global, 0);

  op.packet = malloc(4 + len);
  ptr = op.packet;
  
  ptr[0] = 0x84;
  ptr++;

  GAVL_24BE_2_PTR(len, ptr); ptr += 3;
  bg_vorbis_comment_write(ptr, &s->m_stream, s->m_global, 0);
  op.bytes = len + 4;
  
  if(!bg_ogg_stream_write_header_packet(s, &op))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Got no Flac ID page");
    free(op.packet);
    return 0;
    }

  
  free(op.packet);
  return 1;
  }


#if 0
static FLAC__StreamEncoderWriteStatus
write_callback(const FLAC__StreamEncoder *encoder,
               const FLAC__byte buffer[],
               size_t bytes,
               void *data)
  {
  ogg_packet op;
  flacogg_t * flacogg;
  flacogg = data;
  
  if((buffer[0] & 0x7f) == 0x04) /* Vorbis comment */
    {
    if(flacogg->frame_alloc < bytes)
      {
      flacogg->frame_alloc = bytes + 1024;
      flacogg->frame = realloc(flacogg->frame, flacogg->frame_alloc);
      }
    memcpy(flacogg->frame, buffer, bytes);
    
    op.bytes  = bytes;
    op.packet = flacogg->frame;
    op.b_o_s  = 0;
    op.e_o_s  = 0;
    op.packetno = 1;
    op.granulepos = 0;
    /* Page will be flushed later */

    if(!bg_ogg_stream_write_header_packet(flacogg->s, &op))
      return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    }
  
  return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
  }
#endif

static void * create_flacogg()
  {
  flacogg_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->enc = bg_flac_create();
  
  //  ret->output = output;
  
  /* We already can set the first bytes */
  //  memcpy(ret->header, header_bytes, 9);
  //  ret->header_size = 9;
  return ret;
  }

static const bg_parameter_info_t * get_parameters_flacogg()
  {
  return bg_flac_get_parameters();
  }

static void set_parameter_flacogg(void * data, const char * name,
                                  const bg_parameter_value_t * v)
  {
  flacogg_t * flacogg;
  flacogg = data;
  
  if(!name)
    return;
  bg_flac_set_parameter(flacogg->enc, name, v);
  }

static gavl_audio_sink_t *
init_flacogg(void * data, gavl_audio_format_t * format,
             gavl_metadata_t * stream_metadata,
             gavl_compression_info_t * ci)
  {
  flacogg_t * flacogg = data;
  return bg_flac_start_uncompressed(flacogg->enc, format, ci, stream_metadata);
  }

static void set_packet_sink(void * data, gavl_packet_sink_t * psink)
  {
  flacogg_t * flacogg = data;
  bg_flac_set_sink(flacogg->enc, psink);
  }


static int close_flacogg(void * data)
  {
  int ret = 1;
  // uint8_t buf[1] = { 0x00 };
  flacogg_t * flacogg;
  flacogg = data;

  bg_flac_free(flacogg->enc);
  
  if(flacogg->frame)
    {
    free(flacogg->frame);
    flacogg->frame = NULL;
    }
  free(flacogg);
  return ret;
  }


const bg_ogg_codec_t bg_flacogg_codec =
  {
    .name =      "flacogg",
    .long_name = TRS("Flac encoder"),
    .create = create_flacogg,

    .get_parameters = get_parameters_flacogg,
    .set_parameter =  set_parameter_flacogg,
    
    .init_audio =     init_flacogg,
    .init_audio_compressed = init_compressed_flacogg,
    .set_packet_sink = set_packet_sink,
    .close = close_flacogg,
  };
