/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <config.h>

#include <bgflac.h>

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

/*
 *  These compression parameters correspond to the
 *  the options -0 through -8 of the flac commandline
 *  encoder
 */

static struct
  {
  int blocksize;                    // -b
  int max_lpc_order;                // -l
  int min_residual_partition_order; // -r min,max
  int max_residual_partition_order;
  int do_mid_side;                     // -m
  int loose_mid_side;               // -M
  int do_exhaustive_model_search;   // -e
  }
clevels[] =
  {
    {
      /* 0 */
      .blocksize =                    1152, // -b
      .max_lpc_order =                0,    // -l
      .min_residual_partition_order = 2,    // -r min,max
      .max_residual_partition_order = 2,
      .do_mid_side =                  0,    // -m
      .loose_mid_side =               0,    // -M
      .do_exhaustive_model_search =   0,    // -e
    },
    {
      /* 1 */
      .blocksize =                    1152, // -b
      .max_lpc_order =                0,    // -l
      .min_residual_partition_order = 2,    // -r min,max
      .max_residual_partition_order = 2,
      .do_mid_side =                  1,    // -m
      .loose_mid_side =               1,    // -M
      .do_exhaustive_model_search =   0,    // -e
    },
    {
      /* 2 */
      .blocksize =                    1152, // -b
      .max_lpc_order =                0,    // -l
      .min_residual_partition_order = 0,    // -r min,max
      .max_residual_partition_order = 3,
      .do_mid_side =                  1,    // -m
      .loose_mid_side =               0,    // -M
      .do_exhaustive_model_search =   0,    // -e
    },
    {
      /* 3 */
      .blocksize =                    4608, // -b
      .max_lpc_order =                6,    // -l
      .min_residual_partition_order = 3,    // -r min,max
      .max_residual_partition_order = 3,
      .do_mid_side =                  0,    // -m
      .loose_mid_side =               0,    // -M
      .do_exhaustive_model_search =   0,    // -e
    },
    {
      /* 4 */
      .blocksize =                    4608, // -b
      .max_lpc_order =                8,    // -l
      .min_residual_partition_order = 3,    // -r min,max
      .max_residual_partition_order = 3,
      .do_mid_side =                  1,    // -m
      .loose_mid_side =               1,    // -M
      .do_exhaustive_model_search =   0,    // -e
    },
    {
      /* 5 */
      .blocksize =                    4608, // -b
      .max_lpc_order =                8,    // -l
      .min_residual_partition_order = 3,    // -r min,max
      .max_residual_partition_order = 3,
      .do_mid_side =                  1,    // -m
      .loose_mid_side =               0,    // -M
      .do_exhaustive_model_search =   0,    // -e
    },
    {
      /* 6 */
      .blocksize =                    4608, // -b
      .max_lpc_order =                8,    // -l
      .min_residual_partition_order = 0,    // -r min,max
      .max_residual_partition_order = 4,
      .do_mid_side =                  1,    // -m
      .loose_mid_side =               0,    // -M
      .do_exhaustive_model_search =   0,    // -e
    },
    {
      /* 7 */
      .blocksize =                    4608, // -b
      .max_lpc_order =                8,    // -l
      .min_residual_partition_order = 0,    // -r min,max
      .max_residual_partition_order = 6,
      .do_mid_side =                  1,    // -m
      .loose_mid_side =               0,    // -M
      .do_exhaustive_model_search =   1,    // -e
    },
    {
      /* 8 */
      .blocksize =                    4608, // -b
      .max_lpc_order =                12,   // -l
      .min_residual_partition_order = 0,    // -r min,max
      .max_residual_partition_order = 6,
      .do_mid_side =                  1,    // -m
      .loose_mid_side =               0,    // -M
      .do_exhaustive_model_search =   1,    // -e
    }
  };

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

void bg_flac_set_parameter(void * data, const char * name, const bg_parameter_value_t * val)
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

static void init_common(bg_flac_t * flac)
  {
  flac->format->interleave_mode = GAVL_INTERLEAVE_NONE;
  
  /* Samplerates which are no multiples of 10 are invalid */
  flac->format->samplerate = ((flac->format->samplerate + 9) / 10) * 10;
  
  /* Bits per sample */

  flac->samples_per_block = clevels[flac->clevel].blocksize;
    
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
    
  }

#if BGAV_FLAC_VERSION_INT <= MAKE_VERSION(1, 1, 2)
void bg_flac_init_file_encoder(bg_flac_t * flac, FLAC__FileEncoder * enc)
  {
  init_common(flac);
  /* Set compression parameters from presets */

  FLAC__file_encoder_set_sample_rate(enc, flac->format->samplerate);
  FLAC__file_encoder_set_channels(enc, flac->format->num_channels);
  
  FLAC__file_encoder_set_blocksize(enc,
                                   clevels[flac->clevel].blocksize);
    
  FLAC__file_encoder_set_max_lpc_order(enc,
                                       clevels[flac->clevel].max_lpc_order);
  FLAC__file_encoder_set_min_residual_partition_order(enc,
                                                      clevels[flac->clevel].min_residual_partition_order);
  FLAC__file_encoder_set_max_residual_partition_order(enc,
                                                      clevels[flac->clevel].max_residual_partition_order);
    
  if(flac->format->num_channels == 2)
    {
    FLAC__file_encoder_set_do_mid_side_stereo(enc,
                                              clevels[flac->clevel].do_mid_side);
    FLAC__file_encoder_set_loose_mid_side_stereo(enc,
                                                 clevels[flac->clevel].loose_mid_side);
    }

  FLAC__file_encoder_set_do_exhaustive_model_search(enc,
                                                    clevels[flac->clevel].do_exhaustive_model_search);

  FLAC__file_encoder_set_bits_per_sample(enc, flac->bits_per_sample);
  
  }
#endif

void bg_flac_init_stream_encoder(bg_flac_t * flac, FLAC__StreamEncoder * enc)
  {
  init_common(flac);
  /* Set compression parameters from presets */

  FLAC__stream_encoder_set_sample_rate(enc, flac->format->samplerate);
  FLAC__stream_encoder_set_channels(enc, flac->format->num_channels);

  FLAC__stream_encoder_set_blocksize(enc,
                                   clevels[flac->clevel].blocksize);
    
  FLAC__stream_encoder_set_max_lpc_order(enc,
                                       clevels[flac->clevel].max_lpc_order);
  FLAC__stream_encoder_set_min_residual_partition_order(enc,
                                                      clevels[flac->clevel].min_residual_partition_order);
  FLAC__stream_encoder_set_max_residual_partition_order(enc,
                                                      clevels[flac->clevel].max_residual_partition_order);
    
  if(flac->format->num_channels == 2)
    {
    FLAC__stream_encoder_set_do_mid_side_stereo(enc,
                                              clevels[flac->clevel].do_mid_side);
    FLAC__stream_encoder_set_loose_mid_side_stereo(enc,
                                                 clevels[flac->clevel].loose_mid_side);
    }

  FLAC__stream_encoder_set_do_exhaustive_model_search(enc,
                                                    clevels[flac->clevel].do_exhaustive_model_search);

  FLAC__stream_encoder_set_bits_per_sample(enc, flac->bits_per_sample);

  }

void bg_flac_prepare_audio_frame(bg_flac_t * flac, gavl_audio_frame_t * frame)
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
  }

/* Metadata -> vorbis comment */

#define STR_COMMENT(str, key) \
  if(m->str) \
    { \
    memset(&entry, 0, sizeof(entry)); \
    entry.entry = (uint8_t*)bg_sprintf("%s=%s", key, m->str);   \
    entry.length = strlen((char*)(entry.entry));                        \
    FLAC__metadata_object_vorbiscomment_insert_comment(flac->vorbis_comment, \
                                                       num_comments++,  \
                                                       entry, \
                                                       1); \
    free(entry.entry); \
    }

#define INT_COMMENT(i, key) \
  if(m->i) \
    { \
    memset(&entry, 0, sizeof(entry)); \
    entry.entry = (uint8_t*)bg_sprintf("%s=%d", key, m->i);     \
    entry.length = strlen((char*)(entry.entry));                        \
    FLAC__metadata_object_vorbiscomment_insert_comment(flac->vorbis_comment, \
                                                       num_comments++,  \
                                                       entry, \
                                                       1); \
    free(entry.entry); \
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

void bg_flac_init_metadata(bg_flac_t * flac, const bg_metadata_t * m)
  {
  FLAC__StreamMetadata_VorbisComment_Entry entry;
  
  int num_comments = 0;
  
  flac->vorbis_comment =
    FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
  STR_COMMENT(artist,    "ARTIST");
  STR_COMMENT(title,     "TITLE");
  STR_COMMENT(album,     "ALBUM");
  STR_COMMENT(genre,     "GENRE");
  STR_COMMENT(date,      "DATE");
  STR_COMMENT(copyright, "COPYRIGHT");
  INT_COMMENT(track,     "TRACKNUMBER");
  RAW_COMMENT(comment);
  }
