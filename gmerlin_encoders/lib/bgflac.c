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

#include <config.h>
#include <bgflac.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "flacenc"

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

const bg_parameter_info_t * bg_flac_get_parameters(void * data)
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

static void metadata_callback(const FLAC__StreamEncoder *encoder,
                              const FLAC__StreamMetadata *metadata,
                              void *client_data)
  {
  if(metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
    {
    
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

    if(gavl_packet_sink_put_packet(flac->psink, &gp) != GAVL_SINK_OK)
      return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    else
      return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }
  
  if(flac->header_size < BG_FLAC_HEADER_SIZE)
    {
    memcpy(flac->header + flac->header_size, buffer, bytes);
    flac->header_size+=bytes;

    if(flac->header_size == BG_FLAC_HEADER_SIZE)
      {
      /* TODO: Extract codec header */
      }
    }

  if(flac->write_callback)
    return flac->write_callback(encoder, buffer, bytes, data);
  return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
  }

int bg_flac_init_stream_encoder(bg_flac_t * flac)
  {
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

  if(FLAC__stream_encoder_init_stream(flac->enc,
                                      write_callback,
                                      NULL,
                                      NULL,
                                      metadata_callback,
                                      flac) != FLAC__STREAM_ENCODER_OK)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "FLAC__stream_encoder_init_stream failed");
    return 0;
    }
  
  flac->samples_per_block =
    FLAC__stream_encoder_get_blocksize(flac->enc);
  return 1;
  }

int bg_flac_encode_audio_frame(bg_flac_t * flac, gavl_audio_frame_t * frame)
  {
  int i;
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

void bg_flac_free(bg_flac_t * flac)
  {
  int i;

  if(flac->buffer[0])
    {
    for(i = 0; i < flac->format->num_channels; i++)
      {
      free(flac->buffer[i]);
      flac->buffer[i] = NULL;
      }
    }
  if(flac->vorbis_comment)
    {
    FLAC__metadata_object_delete(flac->vorbis_comment);
    flac->vorbis_comment = NULL;
    }

  FLAC__stream_encoder_finish(flac->enc);
  FLAC__stream_encoder_delete(flac->enc);
  
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

void bg_flac_init_metadata(bg_flac_t * flac, const gavl_metadata_t * m)
  {
  FLAC__StreamMetadata_VorbisComment_Entry entry;
  const char * val;
  int num_comments = 0;
  int year;
  
  flac->vorbis_comment =
    FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
  STR_COMMENT(GAVL_META_ARTIST, "ARTIST");
  STR_COMMENT(GAVL_META_TITLE, "TITLE");
  STR_COMMENT(GAVL_META_ALBUM, "ALBUM");
  STR_COMMENT(GAVL_META_ALBUMARTIST, "ALBUM ARTIST");
  STR_COMMENT(GAVL_META_ALBUMARTIST, "ALBUMARTIST");
  STR_COMMENT(GAVL_META_GENRE, "GENRE");

  /* TODO: Get year */
  year = bg_metadata_get_year(m);
  if(year > 0)
    {
    INT_COMMENT(year, "DATE");
    }
  STR_COMMENT(GAVL_META_COPYRIGHT, "COPYRIGHT");
  STR_COMMENT(GAVL_META_TRACKNUMBER, "TRACKNUMBER");
  STR_COMMENT(GAVL_META_COMMENT, "COMMENT");
  }


void bg_flac_init(bg_flac_t * flac)
  {
  flac->enc = FLAC__stream_encoder_new();

  
  }

