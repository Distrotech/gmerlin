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

#include <ogg/ogg.h>
#include "ogg_common.h"

#include <bgflac.h>

/* Way too large but save */
#define BUFFER_SIZE (MAX_BYTES_PER_FRAME*10)

typedef struct
  {
  bg_flac_t com;
  
  ogg_stream_state os;
  
  long serialno;
  bg_ogg_encoder_t * output;
  
  FLAC__StreamEncoder * enc;
  FLAC__StreamMetadata * vorbis_comment;
    
  uint8_t header[128];
  int header_size;
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


static FLAC__StreamEncoderWriteStatus
write_callback(const FLAC__StreamEncoder *encoder,
               const FLAC__byte buffer[],
               size_t bytes,
               unsigned samples,
               unsigned current_frame,
               void *data)
  {
  ogg_packet op;
  int result;
  flacogg_t * flacogg;
  flacogg = data;
  
  
  if(!flacogg->header_written)
    {
    memcpy(flacogg->header + flacogg->header_size, buffer, bytes);
    flacogg->header_size+=bytes;

    /* Flush header */
    if(flacogg->header_size == 9+4+38)
      {
      op.bytes  = flacogg->header_size;
      op.packet = flacogg->header;
      op.b_o_s  = 1;
      op.e_o_s  = 0;
      op.packetno = 0;
      op.granulepos = 0;
      ogg_stream_packetin(&flacogg->os, &op);

      result = bg_ogg_flush_page(&flacogg->os, flacogg->output, 1);
      if(!result)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Got no Flac ID page");
        return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
        }
      else if(result < 0)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Writing to file failed: %s", strerror(errno));
        return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
        }
      
      flacogg->header_written = 1;
      }
    }
  else if((buffer[0] & 0x7f) == 0x04) /* Vorbis comment */
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
    ogg_stream_packetin(&flacogg->os, &op);
    }
  else
    {
    /* Encoded data */

    if(flacogg->frame_size)
      {
      op.bytes  = flacogg->frame_size;
      op.packet = flacogg->frame;
      op.b_o_s  = 0;
      op.e_o_s  = !bytes; /* bytes is zero when called from close() */
      op.packetno = 2+flacogg->frames_encoded;
      op.granulepos = flacogg->samples_encoded + flacogg->frame_samples;
      ogg_stream_packetin(&flacogg->os, &op);
      result = bg_ogg_flush(&flacogg->os, flacogg->output, !op.e_o_s);

      if(result < 0)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Writing to file failed: %s",
               strerror(errno));
        return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
        }
      flacogg->frame_size = 0;
      flacogg->frames_encoded++;
      flacogg->samples_encoded += flacogg->frame_samples;

      }

    if(bytes) /* Save next frame */
      {
      if(flacogg->frame_alloc < bytes)
        {
        flacogg->frame_alloc = bytes + 1024;
        flacogg->frame = realloc(flacogg->frame, flacogg->frame_alloc);
        }
      memcpy(flacogg->frame, buffer, bytes);
      flacogg->frame_size = bytes;
      flacogg->frame_samples = samples;
      }
    }
  
  return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
  }

static void metadata_callback(const FLAC__StreamEncoder *encoder,
                              const FLAC__StreamMetadata *metadata,
                              void *client_data)
  {
  }

static void * create_flacogg(bg_ogg_encoder_t * output, long serialno)
  {
  uint8_t header_bytes[] =
    {
      0x7f,
      0x46, 0x4C, 0x41, 0x43, // FLAC
      0x01, 0x00, // Major, minor version
      0x00, 0x01, // Number of other header packets (Big Endian)
    };
  
  flacogg_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->serialno = serialno;
  ret->output = output;
  ret->enc    = FLAC__stream_encoder_new();
  
  /* We already can set the first bytes */ 
  memcpy(ret->header, header_bytes, 9);
  ret->header_size = 9;
  return ret;
  }

static const bg_parameter_info_t * get_parameters_flacogg()
  {
  return bg_flac_get_parameters(NULL);
  }

static void set_parameter_flacogg(void * data, const char * name,
                                  const bg_parameter_value_t * v)
  {
  flacogg_t * flacogg;
  flacogg = data;
  
  if(!name)
    return;
  bg_flac_set_parameter(&flacogg->com, name, v);
  }

static int init_flacogg(void * data, gavl_audio_format_t * format,
                        gavl_metadata_t * metadata)
  {
  flacogg_t * flacogg = data;

  flacogg->com.format = format;
  
  ogg_stream_init(&flacogg->os, flacogg->serialno);

  bg_flac_init_metadata(&flacogg->com, metadata);
  FLAC__stream_encoder_set_metadata(flacogg->enc, &flacogg->com.vorbis_comment, 1);
  
  bg_flac_init_stream_encoder(&flacogg->com, flacogg->enc);

  /* Initialize encoder */
    
  if(FLAC__stream_encoder_init_stream(flacogg->enc,
                                      write_callback,
                                      NULL,
                                      NULL,
                                      metadata_callback,
                                      flacogg) != FLAC__STREAM_ENCODER_OK)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "FLAC__stream_encoder_init_stream failed");
    return 0;
    }
  
  flacogg->com.samples_per_block =
    FLAC__stream_encoder_get_blocksize(flacogg->enc);
  
  return 1;
  }

static int flush_header_pages_flacogg(void*data)
  {
  flacogg_t * flacogg;
  flacogg = data;
  if(bg_ogg_flush(&flacogg->os, flacogg->output, 1) <= 0)
    return 0;
  return 1;
  }

static int write_audio_frame_flacogg(void * data, gavl_audio_frame_t * frame)
  {
  flacogg_t * flacogg;
  flacogg = data;
  
  bg_flac_prepare_audio_frame(&flacogg->com, frame);

  if(!FLAC__stream_encoder_process(flacogg->enc, (const FLAC__int32 **) flacogg->com.buffer,
                                   frame->valid_samples))
    return 0;
  return 1;
  }

static int close_flacogg(void * data)
  {
  int ret = 1;
  uint8_t buf[1] = { 0x00 };
  flacogg_t * flacogg;
  flacogg = data;
  
  if(flacogg->enc)
    {
    FLAC__stream_encoder_finish(flacogg->enc);
    FLAC__stream_encoder_delete(flacogg->enc);

    /* Flush data */
    if(write_callback(NULL,
                      buf,
                      0,
                      0,
                      0,
                      flacogg) == FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR)
      ret = 0;
    
    flacogg->enc = NULL;
    }
  
  ogg_stream_clear(&flacogg->os);
  
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
    .flush_header_pages = flush_header_pages_flacogg,
    
    .encode_audio = write_audio_frame_flacogg,
    .close = close_flacogg,
  };
