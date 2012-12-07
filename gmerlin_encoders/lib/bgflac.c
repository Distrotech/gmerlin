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

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gavl/metatags.h>
#include <gavl/numptr.h>

#include <config.h>
#include <bgflac.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "flacenc"

struct bg_flac_s
  {
  int clevel; /* Compression level 0..8 */

  int bits_per_sample;
  int shift_bits;
  int divisor;
  //  int samples_per_block;

  int fixed_blocksize;
  
  void (*copy_frame)(int32_t * dst[], gavl_audio_frame_t * src,
                     int num_channels);

  /* Buffer */
    
  int32_t * buffer[GAVL_MAX_CHANNELS];
  int buffer_alloc; /* In samples */
  
  gavl_audio_format_t *format;

  //  FLAC__StreamMetadata * vorbis_comment;
  FLAC__StreamEncoder * enc;

  /* Needs to be set by the client */
  gavl_packet_sink_t * psink_out;
    
  int (*streaminfo_callback)(void * data, uint8_t * si, int len);
  void * callback_priv;
  
  int64_t pts;
  
  gavl_compression_info_t ci;

  FLAC__StreamMetadata_StreamInfo si;
  };


/* Copy functions */

static void copy_frame_8(int32_t * dst[], gavl_audio_frame_t * src,
                         int num_channels)
  {
  int i, j;
  for(i = 0; i < num_channels; i++)
    {
    for(j = 0; j < src->valid_samples; j++)
      {
      dst[i][j] = src->channels.s_8[i][j];
      }
    }
  }

static void copy_frame_16(int32_t * dst[], gavl_audio_frame_t * src,
                          int num_channels)
  {
  int i, j;
  for(i = 0; i < num_channels; i++)
    {
    for(j = 0; j < src->valid_samples; j++)
      {
      dst[i][j] = src->channels.s_16[i][j];
      }
    }

  }

static void copy_frame_32(int32_t * dst[], gavl_audio_frame_t * src,
                          int num_channels)
  {
  int i;
  for(i = 0; i < num_channels; i++)
    {
    memcpy(dst[i], src->channels.s_32[i],
           src->valid_samples * sizeof(dst[0][0]));
    }
  }

static void do_shift(int32_t * dst[], int num_channels, int num_samples,
                     int divisor)
  {
  int i, j;
  
  for(i = 0; i < num_channels; i++)
    {
    for(j = 0; j < num_samples; j++)
      {
      dst[i][j] /= divisor;
      }
    }
  }

static const bg_parameter_info_t audio_parameters[] =
  {
    {
      .name =      "bits",
      .long_name = TRS("Bits"),
      .type =      BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "16" },
      .multi_names = (char const *[]){ "8", "12", "16", "20", "24", NULL },
    },
    {
      .name =        "compression_level",
      .long_name =   TRS("Compression Level"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 8 },
      .val_default = { .val_i = 5 },
      .help_string = TRS("0: Fastest encoding, biggest files\n\
8: Slowest encoding, smallest files")
    },
    { /* End of parameters */ }
  };

const bg_parameter_info_t * bg_flac_get_parameters()
  {
  return audio_parameters;
  }

void bg_flac_set_parameter(void * data, const char * name,
                           const bg_parameter_value_t * val)
  {
  bg_flac_t * flac;
  flac = data;
    
  if(!name)
    {
    return;
    }
  else if(!strcmp(name, "compression_level"))
    {
    flac->clevel = val->val_i;
    }
  else if(!strcmp(name, "bits"))
    {
    flac->bits_per_sample = atoi(val->val_str);
    }
  
  //  fprintf(stderr, "set_audio_parameter_flac %s\n", name);
  }

static void metadata_callback(const FLAC__StreamEncoder *enc,
                              const FLAC__StreamMetadata *m,
                              void *client_data)
  {
  bg_flac_t * flac = client_data;
  
  if((m->type == FLAC__METADATA_TYPE_STREAMINFO) && flac->streaminfo_callback)
    {
    const FLAC__StreamMetadata_StreamInfo * si;
    uint8_t * ptr;
    uint32_t i;
    
    /* Re-write stream info */

    si = &m->data.stream_info;
    ptr = flac->ci.global_header + 8; // Signature + metadata header
    
    GAVL_16BE_2_PTR(si->min_blocksize, ptr); ptr += 2;
    GAVL_16BE_2_PTR(si->max_blocksize, ptr); ptr += 2;
    GAVL_24BE_2_PTR(si->min_framesize, ptr); ptr += 3;
    GAVL_24BE_2_PTR(si->max_framesize, ptr); ptr += 3;

    i = si->sample_rate >> 4;
    GAVL_16BE_2_PTR(i, ptr); ptr += 2;

    i = si->sample_rate & 0x0f;   // Samplerate (lower 4 bits)

    i <<= 3;                      // Channels
    i |= (si->channels-1) & 0x7;

    i <<= 5;                      // Bits
    i |= (si->bits_per_sample-1) & 0x1f;

    i <<= 4;                      // Total samples
    i |= (si->total_samples >> 32) & 0xf;

    GAVL_16BE_2_PTR(i, ptr); ptr += 2;

    i = (si->total_samples) & 0xffffffff;
    GAVL_32BE_2_PTR(i, ptr); ptr += 4;
    
    memcpy(ptr, si->md5sum, 16); ptr += 16;
    
    flac->streaminfo_callback(flac->callback_priv,
                              flac->ci.global_header,
                              flac->ci.global_header_len);
    }
     
  }

static FLAC__StreamEncoderWriteStatus
write_callback(const FLAC__StreamEncoder *encoder,
               const FLAC__byte buffer[],
               size_t bytes,
               unsigned samples,
               unsigned current_frame,
               void *data)
  {
  bg_flac_t * flac = data;

  if(flac->ci.global_header_len < BG_FLAC_HEADER_SIZE)
    {
    //    fprintf(stderr, "write callback: %d %ld\n", flac->ci.global_header_len,
    //            bytes);
    
    memcpy(flac->ci.global_header + flac->ci.global_header_len, buffer, bytes);
    flac->ci.global_header_len += bytes;
    
    if(flac->ci.global_header_len == BG_FLAC_HEADER_SIZE)
      {
      /*
       *  Last metadata packet. By default libflac will emit a vorbis_comment
       *  with just the vendor_string after that.
       */
      flac->ci.global_header[4] |= 0x80;
      
      /* Extract codec header */
      if(flac->streaminfo_callback)
        flac->streaminfo_callback(flac->callback_priv,
                                  flac->ci.global_header,
                                  flac->ci.global_header_len);
      }
    }
  
  /* Compressed packet */
  if(samples)
    {
    gavl_packet_t gp;
    gavl_packet_init(&gp);
    gp.data_len = bytes;
    gp.data = (uint8_t*)buffer;
    gp.duration = samples;
    gp.pts = flac->pts;
    flac->pts += samples;

    if(gavl_packet_sink_put_packet(flac->psink_out, &gp) != GAVL_SINK_OK)
      return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    else
      return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }
  return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
  }

void bg_flac_set_callbacks(bg_flac_t * flac,
                           int (*streaminfo_callback)(void*, uint8_t *, int),
                           void * priv)
  {
  flac->streaminfo_callback = streaminfo_callback;
  flac->callback_priv = priv;
  }

static gavl_sink_status_t
write_audio_packet_func_flac(void * priv, gavl_packet_t * packet)
  {
  bg_flac_t * flac = priv;
  
  // fprintf(stderr, "%ld %ld\n", packet->duration, flac->samples_written);
  
  if(packet->data_len < 6)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Packet data too small: %d",
           packet->data_len);
    return GAVL_SINK_ERROR;
    }
  
  if(!flac->si.total_samples)
    {
    flac->fixed_blocksize = !(packet->data[1] & 0x01); // bit 15
    flac->si.min_blocksize = packet->duration;
    flac->si.max_blocksize = packet->duration;
    }
  else if(!flac->fixed_blocksize)
    {
    if(packet->duration < flac->si.min_blocksize)
      flac->si.min_blocksize = packet->duration;
    if(packet->duration > flac->si.max_blocksize)
      flac->si.max_blocksize = packet->duration;
    }
  
  if(!flac->si.min_framesize || (packet->data_len < flac->si.min_framesize))
    flac->si.min_framesize = packet->data_len;
  if(packet->data_len > flac->si.max_framesize)
    flac->si.max_framesize = packet->data_len;
  
  flac->si.total_samples += packet->duration;
  
  return gavl_packet_sink_put_packet(flac->psink_out, packet);
  }


gavl_packet_sink_t *
bg_flac_start_compressed(bg_flac_t * flac,
                         gavl_audio_format_t * fmt,
                         gavl_compression_info_t * ci,
                         gavl_metadata_t * stream_metadata)
  {
  uint16_t i_tmp;
  uint8_t * ptr;
  
  flac->format = fmt;
  gavl_compression_info_copy(&flac->ci, ci);

  ptr = flac->ci.global_header
    + 8  // Signature + metadata header
    + 10 // min/max frame/blocksize
    + 2; // upper 16 bit of 20 samplerate bits

  // |4|3|5|4|
  i_tmp = ptr[0];
  i_tmp <<= 8;
  i_tmp |= ptr[1];
  
  flac->si.sample_rate = flac->format->samplerate;
  flac->si.channels = flac->format->num_channels;
  flac->si.bits_per_sample = ((i_tmp >> 4) & 0x1f)+1;
  flac->si.total_samples = 0;
  
  memcpy(flac->si.md5sum, flac->ci.global_header + 42 - 16, 16);

  if(flac->streaminfo_callback)
    flac->streaminfo_callback(flac->callback_priv,
                              flac->ci.global_header,
                              flac->ci.global_header_len);


  return gavl_packet_sink_create(NULL, write_audio_packet_func_flac, flac);;
  }

static gavl_sink_status_t
encode_audio_func(void * priv, gavl_audio_frame_t * frame)
  {
  int i;
  bg_flac_t * flac = priv;
  
  /* Reallocate sample buffer */
  if(flac->buffer_alloc < frame->valid_samples)
    {
    flac->buffer_alloc = frame->valid_samples + 10;
    for(i = 0; i < flac->format->num_channels; i++)
      flac->buffer[i] = realloc(flac->buffer[i], flac->buffer_alloc *
                                sizeof(flac->buffer[0][0]));
    }

  /* Copy and shift */

  flac->copy_frame(flac->buffer, frame, flac->format->num_channels);

  if(flac->shift_bits)
    do_shift(flac->buffer, flac->format->num_channels,
             frame->valid_samples, flac->divisor);

  if(!FLAC__stream_encoder_process(flac->enc,
                                   (const FLAC__int32 **) flac->buffer,
                                   frame->valid_samples))
    return 0;

  return 1;
  }

gavl_audio_sink_t *
bg_flac_start_uncompressed(bg_flac_t * flac,
                           gavl_audio_format_t * fmt,
                           gavl_compression_info_t * ci,
                           gavl_metadata_t * stream_metadata)
  {
  flac->format = fmt;
  
  /* Common initialization */
  flac->format->interleave_mode = GAVL_INTERLEAVE_NONE;
  
  /* Samplerates which are no multiples of 10 are invalid */
  flac->format->samplerate = ((flac->format->samplerate + 9) / 10) * 10;
  
  /* Bits per sample */

  /* bits_per_sample is zero when writing compressed packets */
  if(!flac->bits_per_sample)
    flac->bits_per_sample = 16;
    
  if(flac->bits_per_sample <= 8)
    {
    flac->copy_frame = copy_frame_8;
    flac->shift_bits = 8 - flac->bits_per_sample;
    flac->format->sample_format = GAVL_SAMPLE_S8;
    }
  else if(flac->bits_per_sample <= 16)
    {
    flac->copy_frame = copy_frame_16;
    flac->shift_bits = 16 - flac->bits_per_sample;
    flac->format->sample_format = GAVL_SAMPLE_S16;
    }
  else if(flac->bits_per_sample <= 32)
    {
    flac->copy_frame = copy_frame_32;
    flac->shift_bits = 32 - flac->bits_per_sample;
    flac->format->sample_format = GAVL_SAMPLE_S32;
    }
  flac->divisor = (1 << flac->shift_bits); 

  /* Set compression parameters from presets */
  
  FLAC__stream_encoder_set_sample_rate(flac->enc, flac->format->samplerate);
  FLAC__stream_encoder_set_channels(flac->enc, flac->format->num_channels);

  /* */
  FLAC__stream_encoder_set_compression_level(flac->enc, flac->clevel);
  
  FLAC__stream_encoder_set_bits_per_sample(flac->enc, flac->bits_per_sample);

  /* Initialize */

  /* Set vendor string: Must be done early because it's needed in the streaminfo callback */
  gavl_metadata_set(stream_metadata, GAVL_META_SOFTWARE, FLAC__VENDOR_STRING);
  
  flac->ci.id = GAVL_CODEC_ID_FLAC;
  
  if(FLAC__stream_encoder_init_stream(flac->enc,
                                      write_callback,
                                      NULL,
                                      NULL,
                                      metadata_callback,
                                      flac) != FLAC__STREAM_ENCODER_OK)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "FLAC__stream_encoder_init_stream failed");
    return NULL;
    }
  
  //  flac->samples_per_block =
  //    FLAC__stream_encoder_get_blocksize(flac->enc);
  
  gavl_compression_info_copy(ci, &flac->ci);
  return gavl_audio_sink_create(NULL, encode_audio_func, flac, flac->format);
  }


void bg_flac_free(bg_flac_t * flac)
  {
  int i;
  
  FLAC__stream_encoder_finish(flac->enc);
  FLAC__stream_encoder_delete(flac->enc);

  if(flac->buffer[0])
    {
    for(i = 0; i < flac->format->num_channels; i++)
      {
      free(flac->buffer[i]);
      flac->buffer[i] = NULL;
      }
    }
  gavl_compression_info_free(&flac->ci);  
  free(flac);
  }

/* Metadata -> vorbis comment */

#define STR_COMMENT(gavl_name, key)         \
  if((val = gavl_metadata_get(m, gavl_name)))    \
    { \
    memset(&entry, 0, sizeof(entry)); \
    entry.entry = (uint8_t*)bg_sprintf("%s=%s", key, val);   \
    entry.length = strlen((char*)entry.entry);                        \
    FLAC__metadata_object_vorbiscomment_insert_comment(flac->vorbis_comment, \
                                                       num_comments++,  \
                                                       entry, \
                                                       1); \
    free(entry.entry); \
    }

#define INT_COMMENT(num, key) \
   {\
   memset(&entry, 0, sizeof(entry));            \
   entry.entry = (uint8_t*)bg_sprintf("%s=%d", key, num);             \
   entry.length = strlen((char*)entry.entry);                           \
   FLAC__metadata_object_vorbiscomment_insert_comment(flac->vorbis_comment, \
                                                      num_comments++,   \
                                                      entry,            \
                                                      1);               \
   free(entry.entry);                                                   \
   }

#define RAW_COMMENT(str) \
  if(m->str) \
    { \
    memset(&entry, 0, sizeof(entry)); \
    entry.entry = (uint8_t*)bg_sprintf("%s", m->str);   \
    entry.length = strlen((char*)(entry.entry));                        \
    FLAC__metadata_object_vorbiscomment_insert_comment(flac->vorbis_comment, \
                                                       num_comments++,  \
                                                       entry, \
                                                       1); \
    free(entry.entry); \
    }

bg_flac_t * bg_flac_create()
  {
  bg_flac_t * flac = calloc(1, sizeof(*flac));
  flac->enc = FLAC__stream_encoder_new();
  flac->ci.id = GAVL_CODEC_ID_FLAC;
  flac->ci.global_header = malloc(BG_FLAC_HEADER_SIZE);
  return flac;
  }

void bg_flac_set_sink(bg_flac_t * flac, gavl_packet_sink_t * psink)
  {
  flac->psink_out = psink;
  }

