/*****************************************************************
 
  flacogg.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <stdlib.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

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
  FILE * output;
  
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
  
  } flacogg_t;


static FLAC__StreamEncoderWriteStatus
write_callback(const FLAC__StreamEncoder *encoder,
               const FLAC__byte buffer[],
               unsigned bytes,
               unsigned samples,
               unsigned current_frame,
               void *data)
  {
  ogg_packet op;
  
  flacogg_t * flacogg;
  flacogg = (flacogg_t*)data;
  
  //  fprintf(stderr, "Write callback: bytes: %d, samples: %d, current_frame: %d\n", bytes, samples, current_frame);
  // bg_hexdump(buffer, bytes, 16);
  
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
      
      if(!bg_ogg_flush_page(&flacogg->os, flacogg->output, 1))
        fprintf(stderr, "Warning: Got no Flac ID page\n");
      flacogg->header_written = 1;
      //      fprintf(stderr, "Wrote header\n");
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
      bg_ogg_flush(&flacogg->os, flacogg->output, !bytes);

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
  //  fprintf(stderr, "Metadata callback\n");
  }

static void * create_flacogg(FILE * output, long serialno)
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
  FLAC__stream_encoder_set_write_callback(ret->enc, write_callback);
  FLAC__stream_encoder_set_metadata_callback(ret->enc, metadata_callback);
  FLAC__stream_encoder_set_client_data(ret->enc, ret);

  /* We already can set the first bytes */ 
  memcpy(ret->header, header_bytes, 9);
  ret->header_size = 9;
  return ret;
  }

static bg_parameter_info_t * get_parameters_flacogg()
  {
  return bg_flac_get_parameters(NULL);
  }

static void set_parameter_flacogg(void * data, char * name,
                                  bg_parameter_value_t * v)
  {
  flacogg_t * flacogg;
  flacogg = (flacogg_t*)data;
  
  if(!name)
    return;
  bg_flac_set_parameter(&flacogg->com, name, v);
  }

static int init_flacogg(void * data, gavl_audio_format_t * format, bg_metadata_t * metadata)
  {
  flacogg_t * flacogg = (flacogg_t *)data;

  flacogg->com.format = format;
  
  ogg_stream_init(&flacogg->os, flacogg->serialno);

  bg_flac_init_metadata(&flacogg->com, metadata);
  FLAC__stream_encoder_set_metadata(flacogg->enc, &flacogg->com.vorbis_comment, 1);
  
  bg_flac_init_stream_encoder(&flacogg->com, flacogg->enc);
  
  /* Initialize encoder */

  //  fprintf(stderr, "FLAC__stream_encoder_init...\n");
  if(FLAC__stream_encoder_init(flacogg->enc) != FLAC__STREAM_ENCODER_OK)
    {
    fprintf(stderr, "ERROR: FLAC__stream_encoder_init failed\n");
    return 0;
    }
  //  fprintf(stderr, "FLAC__stream_encoder_done...\n");
  
  return 1;
  }

static void flush_header_pages_flacogg(void*data)
  {
  flacogg_t * flacogg;
  flacogg = (flacogg_t*)data;
  bg_ogg_flush(&flacogg->os, flacogg->output, 1);
  }

static void write_audio_frame_flacogg(void * data, gavl_audio_frame_t * frame)
  {
  flacogg_t * flacogg;
  flacogg = (flacogg_t*)data;

  bg_flac_prepare_audio_frame(&flacogg->com, frame);
  FLAC__stream_encoder_process(flacogg->enc, (const FLAC__int32 **) flacogg->com.buffer,
                               frame->valid_samples);
  
  }

static void close_flacogg(void * data)
  {
  uint8_t buf[1];
  flacogg_t * flacogg;
  flacogg = (flacogg_t*)data;
  
  if(flacogg->enc)
    {
    //    fprintf(stderr, "Closing encoder\n");
    FLAC__stream_encoder_finish(flacogg->enc);
    FLAC__stream_encoder_delete(flacogg->enc);
    //    fprintf(stderr, "Closing encoder done\n");

    /* Flush data */
    write_callback(NULL,
                   buf,
                   0,
                   0,
                   0,
                   flacogg);
    
    flacogg->enc = NULL;
    }
  
  ogg_stream_clear(&flacogg->os);
  
  if(flacogg->frame)
    {
    free(flacogg->frame);
    flacogg->frame = (uint8_t*)0;
    }
  free(flacogg);
  }


bg_ogg_codec_t bg_flacogg_codec =
  {
    name:      "flacogg",
    long_name: "Flac encoder",
    create: create_flacogg,

    get_parameters: get_parameters_flacogg,
    set_parameter:  set_parameter_flacogg,
    
    init_audio:     init_flacogg,
    
    //  int (*init_video)(void*, gavl_video_format_t * format);
  
    flush_header_pages: flush_header_pages_flacogg,
    
    encode_audio: write_audio_frame_flacogg,
    close: close_flacogg,
  };
