/*****************************************************************
 
  e_flac.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <FLAC/file_encoder.h>
#include <FLAC/file_decoder.h> /* For creating the seektable */
#include <FLAC/metadata.h>

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

typedef struct
  {
  char * error_msg;
  char * filename;
  
  int clevel; /* Compression level 0..8 */

  int bits_per_sample;
  int shift_bits;
  int divisor;
  int samples_per_block;
    
  void (*copy_frame)(int32_t * dst[], gavl_audio_frame_t * src,
                     int num_channels);
  
  FLAC__FileEncoder * enc;

  /* Metadata stuff */
  
  FLAC__StreamMetadata * seektable; 
  FLAC__StreamMetadata * vorbis_comment;

  FLAC__StreamMetadata * metadata[2];
  int num_metadata;
  
  /* Configuration stuff */
    
  int use_vorbis_comment;
  int use_seektable;
  int num_seektable_entries;

  /* Buffer */
    
  int32_t * buffer[GAVL_MAX_CHANNELS];
  int buffer_alloc; /* In samples */
  
  gavl_audio_format_t format;

  int64_t samples_written;
  
  } flac_t;

static void * create_flac()
  {
  flac_t * ret;
  ret = calloc(1, sizeof(*ret));

  /* Create encoder instance */
  ret->enc = FLAC__file_encoder_new();
  
  return ret;
  }

static void destroy_flac(void * priv)
  {
  flac_t * flac;
  flac = (flac_t*)priv;

  FLAC__file_encoder_delete(flac->enc);
  
  free(flac);
  }

static const char * get_error_flac(void * priv)
  {
  flac_t * flac;
  flac = (flac_t*)priv;
  return flac->error_msg;
  }

static bg_parameter_info_t audio_parameters[] =
  {
    {
      name:      "bits",
      long_name: "Bits",
      type:      BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "16" },
      multi_names: (char*[]){ "8", "12", "16", "20", "24", (char*)0 },
    },
    {
      name:        "compression_level",
      long_name:   "Compression Level",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 8 },
      val_default: { val_i: 5 },
      help_string: "0: Fastest encoding, biggest files\n\
8: Slowest encoding, smallest files"
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "use_vorbis_comment",
      long_name:   "Write vorbis comment",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
      help_string: "Write Vorbis comment containing metadata to the file"
    },
    {
      name:        "use_seektable",
      long_name:   "Write seek table",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
      help_string: "Write seektable (strongly recommended)"
    },
    {
      name:        "num_seektable_entries",
      long_name:   "Entries in the seektable",
      type:        BG_PARAMETER_INT,
      val_min: { val_i: 1 },
      val_max: { val_i: 1000000 },
      val_default: { val_i: 100 },
      help_string: "Maximum number of entries in the seek table. Default is 100, larger numbers result in\
 shorter seeking times but also in larger files."
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_audio_parameters_flac(void * data)
  {
  return audio_parameters;
  }

static bg_parameter_info_t * get_parameters_flac(void * data)
  {
  return parameters;
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
      blocksize:                    1152, // -b
      max_lpc_order:                0,    // -l
      min_residual_partition_order: 2,    // -r min,max
      max_residual_partition_order: 2,
      do_mid_side:                  0,    // -m
      loose_mid_side:               0,    // -M
      do_exhaustive_model_search:   0,    // -e
    },
    {
      /* 1 */
      blocksize:                    1152, // -b
      max_lpc_order:                0,    // -l
      min_residual_partition_order: 2,    // -r min,max
      max_residual_partition_order: 2,
      do_mid_side:                  1,    // -m
      loose_mid_side:               1,    // -M
      do_exhaustive_model_search:   0,    // -e
    },
    {
      /* 2 */
      blocksize:                    1152, // -b
      max_lpc_order:                0,    // -l
      min_residual_partition_order: 0,    // -r min,max
      max_residual_partition_order: 3,
      do_mid_side:                  1,    // -m
      loose_mid_side:               0,    // -M
      do_exhaustive_model_search:   0,    // -e
    },
    {
      /* 3 */
      blocksize:                    4608, // -b
      max_lpc_order:                6,    // -l
      min_residual_partition_order: 3,    // -r min,max
      max_residual_partition_order: 3,
      do_mid_side:                  0,    // -m
      loose_mid_side:               0,    // -M
      do_exhaustive_model_search:   0,    // -e
    },
    {
      /* 4 */
      blocksize:                    4608, // -b
      max_lpc_order:                8,    // -l
      min_residual_partition_order: 3,    // -r min,max
      max_residual_partition_order: 3,
      do_mid_side:                  1,    // -m
      loose_mid_side:               1,    // -M
      do_exhaustive_model_search:   0,    // -e
    },
    {
      /* 5 */
      blocksize:                    4608, // -b
      max_lpc_order:                8,    // -l
      min_residual_partition_order: 3,    // -r min,max
      max_residual_partition_order: 3,
      do_mid_side:                  1,    // -m
      loose_mid_side:               0,    // -M
      do_exhaustive_model_search:   0,    // -e
    },
    {
      /* 6 */
      blocksize:                    4608, // -b
      max_lpc_order:                8,    // -l
      min_residual_partition_order: 0,    // -r min,max
      max_residual_partition_order: 4,
      do_mid_side:                  1,    // -m
      loose_mid_side:               0,    // -M
      do_exhaustive_model_search:   0,    // -e
    },
    {
      /* 7 */
      blocksize:                    4608, // -b
      max_lpc_order:                8,    // -l
      min_residual_partition_order: 0,    // -r min,max
      max_residual_partition_order: 6,
      do_mid_side:                  1,    // -m
      loose_mid_side:               0,    // -M
      do_exhaustive_model_search:   1,    // -e
    },
    {
      /* 8 */
      blocksize:                    4608, // -b
      max_lpc_order:                12,   // -l
      min_residual_partition_order: 0,    // -r min,max
      max_residual_partition_order: 6,
      do_mid_side:                  1,    // -m
      loose_mid_side:               0,    // -M
      do_exhaustive_model_search:   1,    // -e
    }
  };

static void set_audio_parameter_flac(void * data, int stream, char * name,
                                       bg_parameter_value_t * v)
  {
  int i;
  flac_t * flac;
  flac = (flac_t*)data;
  
  if(stream)
    return;
    
  if(!name)
    {
    /* Set compression parameters from presets */

    FLAC__file_encoder_set_blocksize(flac->enc,
                                     clevels[flac->clevel].blocksize);
    flac->samples_per_block = clevels[flac->clevel].blocksize;

    FLAC__file_encoder_set_max_lpc_order(flac->enc,
                                         clevels[flac->clevel].max_lpc_order);
    FLAC__file_encoder_set_min_residual_partition_order(flac->enc,
                                                        clevels[flac->clevel].min_residual_partition_order);
    FLAC__file_encoder_set_max_residual_partition_order(flac->enc,
                                                        clevels[flac->clevel].max_residual_partition_order);
    
    if(flac->format.num_channels == 2)
      {
      FLAC__file_encoder_set_do_mid_side_stereo(flac->enc,
                                                clevels[flac->clevel].do_mid_side);
      FLAC__file_encoder_set_loose_mid_side_stereo(flac->enc,
                                                   clevels[flac->clevel].loose_mid_side);
      }

    FLAC__file_encoder_set_do_exhaustive_model_search(flac->enc,
                                                      clevels[flac->clevel].do_exhaustive_model_search);

    /* Bits per sample */

    if(flac->bits_per_sample <= 8)
      {
      flac->copy_frame = copy_frame_8;
      flac->shift_bits = 8 - flac->bits_per_sample;
      flac->format.sample_format = GAVL_SAMPLE_S8;
      }
    else if(flac->bits_per_sample <= 16)
      {
      flac->copy_frame = copy_frame_16;
      flac->shift_bits = 16 - flac->bits_per_sample;
      flac->format.sample_format = GAVL_SAMPLE_S16;
      }
    else if(flac->bits_per_sample <= 32)
      {
      flac->copy_frame = copy_frame_32;
      flac->shift_bits = 32 - flac->bits_per_sample;
      flac->format.sample_format = GAVL_SAMPLE_S32;
      }
    FLAC__file_encoder_set_bits_per_sample(flac->enc, flac->bits_per_sample);

    flac->divisor = (1 << flac->shift_bits); 
    /* Initialize encoder */
    
    if(FLAC__file_encoder_init(flac->enc) != FLAC__FILE_ENCODER_OK)
      {
      fprintf(stderr, "ERROR: FLAC__file_encoder_init failed\n");
      }
    return;
    }
  else if(!strcmp(name, "compression_level"))
    {
    flac->clevel = v->val_i;
    }
  else if(!strcmp(name, "bits"))
    {
    flac->bits_per_sample = atoi(v->val_str);
    }
  
  //  fprintf(stderr, "set_audio_parameter_flac %s\n", name);
  }

static void set_parameter_flac(void * data,
                               char * name, bg_parameter_value_t * v)
  {
  flac_t * flac;
  flac = (flac_t*)data;
  
  if(!name)
    return;

  else if(!strcmp(name, "use_vorbis_comment"))
    flac->use_vorbis_comment = v->val_i;
  else if(!strcmp(name, "use_seektable"))
    flac->use_seektable = v->val_i;
  else if(!strcmp(name, "num_seektable_entries"))
    flac->num_seektable_entries = v->val_i;
  }

/* Metadata -> vorbis comment */

#define STR_COMMENT(str, key) \
  if(m->str) \
    { \
    memset(&entry, 0, sizeof(entry)); \
    entry.entry = bg_sprintf("%s=%s", key, m->str); \
    entry.length = strlen(entry.entry); \
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
    entry.entry = bg_sprintf("%s=%d", key, m->i); \
    entry.length = strlen(entry.entry); \
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
    entry.entry = bg_sprintf("%s", m->str); \
    entry.length = strlen(entry.entry); \
    FLAC__metadata_object_vorbiscomment_insert_comment(flac->vorbis_comment, \
                                                       num_comments++,  \
                                                       entry, \
                                                       1); \
    free(entry.entry); \
    }
  


static void create_vorbis_comment(flac_t * flac, bg_metadata_t * m)
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

  flac->metadata[flac->num_metadata++] = flac->vorbis_comment;
  }

static void create_seektable(flac_t * flac)
  {
  flac->seektable =
    FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);

  FLAC__metadata_object_seektable_template_append_placeholders(flac->seektable,
                                                               flac->num_seektable_entries);
  flac->metadata[flac->num_metadata++] = flac->seektable;
  }

static int open_flac(void * data, const char * filename,
                    bg_metadata_t * m)
  {
  int result = 1;
  flac_t * flac;
  
  flac = (flac_t*)data;

  flac->filename = bg_strdup(flac->filename, filename);
  FLAC__file_encoder_set_filename(flac->enc, flac->filename);

  /* Create vorbis comment */

  if(flac->use_vorbis_comment)
    {
    create_vorbis_comment(flac, m);
    }

  /* Create seektable */

  if(flac->use_seektable)
    {
    create_seektable(flac);
    }
  
  
  
  /* Insert metadata */
    
  FLAC__file_encoder_set_metadata(flac->enc, flac->metadata, flac->num_metadata);
  return result;
  }

static char * flac_extension = ".flac";

static const char * get_extension_flac(void * data)
  {
  return flac_extension;
  }

static void add_audio_stream_flac(void * data, gavl_audio_format_t * format)
  {
  flac_t * flac;
  flac = (flac_t*)data;

  /* Copy and adjust format */

  gavl_audio_format_copy(&(flac->format), format);

  flac->format.interleave_mode = GAVL_INTERLEAVE_NONE;

  /* Set initial parameters for FLAC encoder */

  /* Samplerates which are no multiples of 10 are invalid */
  flac->format.samplerate = ((flac->format.samplerate + 9) / 10) * 10;

  FLAC__file_encoder_set_sample_rate(flac->enc, flac->format.samplerate);
  FLAC__file_encoder_set_channels(flac->enc, flac->format.num_channels);
  
  
  return;
  }

static void write_audio_frame_flac(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  int i;
  flac_t * flac;
  
  flac = (flac_t*)data;

  /* Reallocate sample buffer */
  if(flac->buffer_alloc < frame->valid_samples)
    {
    flac->buffer_alloc = frame->valid_samples + 10;
    for(i = 0; i < flac->format.num_channels; i++)
      flac->buffer[i] = realloc(flac->buffer[i], flac->buffer_alloc *
                                sizeof(flac->buffer[0][0]));
    }

  /* Copy and shift */

  flac->copy_frame(flac->buffer, frame, flac->format.num_channels);

  if(flac->shift_bits)
    do_shift(flac->buffer, flac->format.num_channels,
             frame->valid_samples, flac->divisor);
  
  /* Encode */

  FLAC__file_encoder_process(flac->enc, (const FLAC__int32 **) flac->buffer,
                             frame->valid_samples);
  flac->samples_written += frame->valid_samples;
  }

static void get_audio_format_flac(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  flac_t * flac;
  flac = (flac_t*)data;
  gavl_audio_format_copy(ret, &(flac->format));
  
  }

/*
 *  Finalize seektable.
 *  It's a bit ugly but we want to encode flac files without having the slightest
 *  idea about the total number of samples.
 *  For this reason, we fire up a decoder and let it create the seektable
 *  A similar procedure is implemented in the metaflac utility, but here, things are
 *  simpler because we remembered some properties of the file we just encoded.
 */

typedef struct
  {
  uint64_t * seek_samples;
  uint64_t sample_position;
  uint64_t byte_position;

  uint64_t start_position;
  uint64_t table_position;
  uint64_t table_size;
  
  FLAC__StreamMetadata    * metadata;

  } seektable_client_data;

static FLAC__StreamDecoderWriteStatus
seektable_write_callback(const FLAC__FileDecoder *decoder,
                         const FLAC__Frame *frame,
                         const FLAC__int32 * const buffer[],
                         void *client_data)
  {
  FLAC__StreamMetadata_SeekPoint seekpoint;

  seektable_client_data * cd;

  cd = (seektable_client_data*)client_data;
  
  if(cd->sample_position >= cd->seek_samples[cd->table_position])
    {
    seekpoint.sample_number = cd->sample_position;
    seekpoint.stream_offset = cd->byte_position - cd->start_position;
    seekpoint.frame_samples = frame->header.blocksize;
    
    fprintf(stderr, "Seektable[%lld]: Byte pos: %lld sample pos: %lld frame_samples: %d\n",
            cd->table_position,
            cd->byte_position - cd->start_position, cd->sample_position,
            frame->header.blocksize);

    FLAC__metadata_object_seektable_set_point(cd->metadata,
                                              cd->table_position,
                                              seekpoint);
    cd->table_position++;
    
    if(cd->table_position >= cd->table_size)
      return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

  FLAC__file_decoder_get_decode_position(decoder,
                                         &(cd->byte_position));
  cd->sample_position += frame->header.blocksize;
  
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
  }

static void seektable_error_callback(const FLAC__FileDecoder *decoder,
                                     FLAC__StreamDecoderErrorStatus status, void *client_data)
  {
  fprintf(stderr, "seektable_error_callback called!!\n");
  }

static void seektable_metadata_callback(const FLAC__FileDecoder *decoder,
                                        const FLAC__StreamMetadata *metadata,
                                        void *client_data)
  {
  fprintf(stderr, "seektable_metadata_callback called!!\n");
  }

static void finalize_seektable(flac_t * flac)
  {
  int i;
  uint64_t index;
  
  FLAC__FileDecoder       * decoder;
  FLAC__Metadata_Chain    * chain;
  FLAC__Metadata_Iterator * iter;

  seektable_client_data cd;
  
  memset(&cd, 0, sizeof(cd));
  
  /* Read all metadata from the file and extract seektable */

  chain = FLAC__metadata_chain_new();
  FLAC__metadata_chain_read(chain, flac->filename);
  iter = FLAC__metadata_iterator_new();
  FLAC__metadata_iterator_init(iter, chain);

  while(FLAC__metadata_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_SEEKTABLE)
    FLAC__metadata_iterator_next(iter);


  cd.metadata = FLAC__metadata_iterator_get_block(iter);

  /* Check, which seektable entries we want. */

  cd.seek_samples = malloc(flac->num_seektable_entries * sizeof(*(cd.seek_samples)));

  cd.table_size = flac->num_seektable_entries;
  index = 1;
  cd.seek_samples[0] = 0;

  for(i = 1; i < flac->num_seektable_entries; i++)
    {
    cd.seek_samples[index] = ((i * flac->samples_written) /
                              (flac->num_seektable_entries * flac->samples_per_block)) *
      flac->samples_per_block;

    /* Avoid duplicate entries */
    if(cd.seek_samples[index] > cd.seek_samples[index-1])
      {
      fprintf(stderr, "Seek samples[%lld]: %lld (total: %lld)\n", index, cd.seek_samples[index],
              flac->samples_written);
      index++;
      }
    else
      cd.table_size--;
    }

  cd.table_position = 0;
  
  /* Populate the seek table */
    
  decoder = FLAC__file_decoder_new();
  FLAC__file_decoder_set_md5_checking(decoder, false);
  FLAC__file_decoder_set_filename(decoder, flac->filename);
  FLAC__file_decoder_set_metadata_ignore_all(decoder);
  FLAC__file_decoder_set_write_callback(decoder, seektable_write_callback);
  FLAC__file_decoder_set_metadata_callback(decoder, seektable_metadata_callback);
  FLAC__file_decoder_set_error_callback(decoder, seektable_error_callback);
  FLAC__file_decoder_set_client_data(decoder, &cd);
  FLAC__file_decoder_init(decoder);
  FLAC__file_decoder_process_until_end_of_metadata(decoder);
  FLAC__file_decoder_get_decode_position(decoder, &cd.start_position);

  cd.byte_position = cd.start_position;
  
  fprintf(stderr, "start_position: %lld\n", cd.start_position);
  
  FLAC__file_decoder_process_until_end_of_file(decoder);
  FLAC__file_decoder_delete(decoder);

  /* Let the seektable shrink if necessary */
  if(cd.table_size < flac->num_seektable_entries)
    {
    FLAC__metadata_object_seektable_resize_points(cd.metadata, cd.table_size);
    }
  
  /* Write metadata back to the file */

  FLAC__metadata_chain_write(chain,
                             true, /* use_padding */
                             true  /* preserve_file_stats */ );

  /* Clean up stuff */

  FLAC__metadata_iterator_delete(iter);
  FLAC__metadata_chain_delete(chain);

  free(cd.seek_samples);

  }

static void close_flac(void * data, int do_delete)
  {
  int i;
  flac_t * flac;
  flac = (flac_t*)data;

  if(flac->seektable)
    FLAC__metadata_object_seektable_template_sort(flac->seektable, 1);

  FLAC__file_encoder_finish(flac->enc);
  //  FLAC__file_encoder_delete(flac->enc);

  if(flac->seektable)
    finalize_seektable(flac);

  free(flac->filename);

  if(flac->seektable)
    FLAC__metadata_object_delete(flac->seektable);
  if(flac->vorbis_comment)
    FLAC__metadata_object_delete(flac->vorbis_comment);

  for(i = 0; i < flac->format.num_channels; i++)
    free(flac->buffer[i]);
  
  }

bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      name:            "e_flac",       /* Unique short name */
      long_name:       "Flac encoder",
      mimetypes:       NULL,
      extensions:      "ogg",
      type:            BG_PLUGIN_ENCODER_AUDIO,
      flags:           BG_PLUGIN_FILE,
      
      create:            create_flac,
      destroy:           destroy_flac,
      get_error:         get_error_flac,
      get_parameters:    get_parameters_flac,
      set_parameter:     set_parameter_flac,
    },
    max_audio_streams:   1,
    max_video_streams:   0,
    
    get_extension:       get_extension_flac,
    
    open:                open_flac,
    
    get_audio_parameters:    get_audio_parameters_flac,

    add_audio_stream:        add_audio_stream_flac,
    
    set_audio_parameter:     set_audio_parameter_flac,

    get_audio_format:        get_audio_format_flac,
    
    write_audio_frame:   write_audio_frame_flac,
    close:               close_flac
  };
