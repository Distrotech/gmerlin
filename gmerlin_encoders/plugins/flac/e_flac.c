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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <config.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "e_flac"

#include <gmerlin/translation.h>


#include <bgflac.h>

typedef struct
  {
  bg_flac_t com; /* Must be first for bg_flac_set_audio_parameter */
  
  char * filename;

  FILE * out;
  
  gavl_audio_format_t format;
  FLAC__StreamEncoder * enc;

  /* Metadata stuff */
  
  FLAC__StreamMetadata * seektable; 

  FLAC__StreamMetadata * metadata[2];
  int num_metadata;
  
  /* Configuration stuff */
  int use_vorbis_comment;
  int use_seektable;
  int num_seektable_entries;

  int64_t samples_written;

  bg_encoder_callbacks_t * cb;

  FLAC__StreamMetadata_StreamInfo si;
  
  int64_t data_start;
  int64_t bytes_written;

  /* Table with *all* frames */
  FLAC__StreamMetadata_SeekPoint * frame_table;
  
  uint32_t frame_table_len;
  uint32_t frame_table_alloc;

  const gavl_compression_info_t * ci;

  int fixed_blocksize;
  } flac_t;

static void * create_flac()
  {
  flac_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void set_callbacks_flac(void * data, bg_encoder_callbacks_t * cb)
  {
  flac_t * flac = data;
  flac->cb = cb;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "use_vorbis_comment",
      .long_name =   TRS("Write vorbis comment"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Write Vorbis comment containing metadata to the file")
    },
    {
      .name =        "use_seektable",
      .long_name =   TRS("Write seek table"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Write seektable (strongly recommended)")
    },
    {
      .name =        "num_seektable_entries",
      .long_name =   TRS("Entries in the seektable"),
      .type =        BG_PARAMETER_INT,
      .val_min = { .val_i = 1 },
      .val_max = { .val_i = 1000000 },
      .val_default = { .val_i = 100 },
      .help_string = TRS("Maximum number of entries in the seek table. Default is 100, larger numbers result in\
 shorter seeking times but also in larger files.")
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_flac(void * data)
  {
  return parameters;
  }

static void set_parameter_flac(void * data,
                               const char * name,
                               const bg_parameter_value_t * v)
  {
  flac_t * flac;
  flac = data;
  
  if(!name)
    return;

  else if(!strcmp(name, "use_vorbis_comment"))
    flac->use_vorbis_comment = v->val_i;
  else if(!strcmp(name, "use_seektable"))
    flac->use_seektable = v->val_i;
  else if(!strcmp(name, "num_seektable_entries"))
    flac->num_seektable_entries = v->val_i;
  }

static int open_flac(void * data, const char * filename,
                     const bg_metadata_t * m, const bg_chapter_list_t * chapter_list)
  {
  int result = 1;
  flac_t * flac;
  
  flac = data;

  /* Create encoder instance */
  flac->enc = FLAC__stream_encoder_new();
  
  flac->filename = bg_filename_ensure_extension(filename, "flac");

  if(!bg_encoder_cb_create_output_file(flac->cb, flac->filename))
    return 0;

  flac->out = fopen(flac->filename, "wb");
  
  /* Create vorbis comment */

  if(flac->use_vorbis_comment && m)
    {
    bg_flac_init_metadata(&flac->com, m);
    flac->metadata[flac->num_metadata++] = flac->com.vorbis_comment;
    }
  
  /* Create seektable */

  if(flac->use_seektable)
    {
    flac->seektable =
      FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
    
    FLAC__metadata_object_seektable_template_append_placeholders(flac->seektable,
                                                                 flac->num_seektable_entries);
    flac->metadata[flac->num_metadata++] = flac->seektable;
    }
  
  
  
  /* Insert metadata */
    
  FLAC__stream_encoder_set_metadata(flac->enc, flac->metadata, flac->num_metadata);
  return result;
  }

static int add_audio_stream_flac(void * data,
                                 const char * language,
                                 const gavl_audio_format_t * format)
  {
  flac_t * flac;
  flac = data;

  /* Copy and adjust format */

  gavl_audio_format_copy(&flac->format, format);
  flac->com.format = &flac->format;
  
  return 0;
  }

static void append_packet(flac_t * f, int samples)
  {
  if(f->frame_table_len + 1 > f->frame_table_alloc)
    {
    f->frame_table_alloc += 10000;
    f->frame_table = realloc(f->frame_table,
                             f->frame_table_alloc * sizeof(*f->frame_table));
    }
  if(f->frame_table_len)
    {
    f->frame_table[f->frame_table_len].sample_number =
      f->frame_table[f->frame_table_len-1].sample_number +
      f->frame_table[f->frame_table_len-1].frame_samples;
    }
  else
    {
    f->frame_table[f->frame_table_len].sample_number = 0;
    }
  f->frame_table[f->frame_table_len].frame_samples = samples;
  f->frame_table[f->frame_table_len].stream_offset = f->bytes_written - f->data_start;
  f->frame_table_len++;
  }


static void
metadata_callback(const FLAC__StreamEncoder *decoder,
                  const FLAC__StreamMetadata *metadata,
                  void *data)
  {
  flac_t * flac = data;
  
  if((metadata->type == FLAC__METADATA_TYPE_STREAMINFO) &&
     !flac->ci)
    memcpy(&flac->si, &metadata->data.stream_info, sizeof(flac->si));
  }

static FLAC__StreamEncoderWriteStatus
write_callback(const FLAC__StreamEncoder *encoder,
               const FLAC__byte buffer[],
               size_t bytes,
               unsigned samples,
               unsigned current_frame,
               void *data)
  {
  flac_t * flac;
  flac = data;

  /* Check if we got a real frame */
  if(samples > 0)
    {
    if(flac->data_start < 0)
      flac->data_start = flac->bytes_written;
    
    append_packet(flac, samples);
    }
  
  if(fwrite(buffer, 1, bytes, flac->out) == bytes)
    {
    flac->bytes_written += bytes;
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }
  return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
  }

static int start_flac(void * data)
  {
  flac_t * flac;
  flac = data;

  bg_flac_init_stream_encoder(&flac->com, flac->enc);
  
  if(FLAC__stream_encoder_init_stream(flac->enc,
                                      write_callback,
                                      NULL, // Seek
                                      NULL, // Tell
                                      metadata_callback, // Metadata
                                      flac) != FLAC__STREAM_ENCODER_OK)
    {
    if(errno)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Initializing encoder failed: %s",
             strerror(errno));
    else
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Initializing encoder failed");
    return 0;
    }

  /* At this point the flac encoder wrote the header, close it here */
  if(flac->ci)
    {
    FLAC__stream_encoder_finish(flac->enc);
    FLAC__stream_encoder_delete(flac->enc);
    flac->enc = NULL;
    }
  else
    {
    flac->com.samples_per_block =
      FLAC__stream_encoder_get_blocksize(flac->enc);
    //    fprintf(stderr, "Got blocksize %d\n", flac->com.samples_per_block);
    }
  flac->data_start = -1;
  
  return 1;
  }

static int write_audio_frame_flac(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  int ret = 1;
  flac_t * flac;
  
  flac = data;

  bg_flac_prepare_audio_frame(&flac->com, frame);
  
  /* Encode */

  if(!FLAC__stream_encoder_process(flac->enc, (const FLAC__int32 **) flac->com.buffer,
                                 frame->valid_samples))
    ret = 0;
  flac->samples_written += frame->valid_samples;
  return ret;
  }

static void get_audio_format_flac(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  flac_t * flac;
  flac = data;
  gavl_audio_format_copy(ret, &flac->format);
  
  }

#if 0

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
seektable_write_callback(const FLAC__StreamDecoder *decoder,
                         const FLAC__Frame *frame,
                         const FLAC__int32 * const buffer[],
                         void *client_data)
  {
  FLAC__StreamMetadata_SeekPoint seekpoint;

  seektable_client_data * cd;

  cd = client_data;
  
  if(cd->sample_position >= cd->seek_samples[cd->table_position])
    {
    seekpoint.sample_number = cd->sample_position;
    seekpoint.stream_offset = cd->byte_position - cd->start_position;
    seekpoint.frame_samples = frame->header.blocksize;
    FLAC__metadata_object_seektable_set_point(cd->metadata,
                                              cd->table_position,
                                              seekpoint);
    cd->table_position++;
    
    if(cd->table_position >= cd->table_size)
      return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

  FLAC__stream_decoder_get_decode_position(decoder,
                                         &cd->byte_position);
  cd->sample_position += frame->header.blocksize;
  
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
  }

static void seektable_error_callback(const FLAC__StreamDecoder *decoder,
                                     FLAC__StreamDecoderErrorStatus status, void *client_data)
  {
  }

static void seektable_metadata_callback(const FLAC__StreamDecoder *decoder,
                                        const FLAC__StreamMetadata *metadata,
                                        void *client_data)
  {
  
  }

static void finalize_seektable(flac_t * flac)
  {
  int i;
  uint64_t index;
  
  FLAC__StreamDecoder       * decoder;
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
                              (flac->num_seektable_entries * flac->com.samples_per_block)) *
      flac->com.samples_per_block;

    /* Avoid duplicate entries */
    if(cd.seek_samples[index] > cd.seek_samples[index-1])
      {
      index++;
      }
    else
      cd.table_size--;
    }

  cd.table_position = 0;
  
  /* Populate the seek table */
    
  decoder = FLAC__stream_decoder_new();
  FLAC__stream_decoder_set_md5_checking(decoder, false);
  FLAC__stream_decoder_set_metadata_ignore_all(decoder);

  FLAC__stream_decoder_init_file(decoder,
                                 flac->filename,
                                 seektable_write_callback,
                                 seektable_metadata_callback,
                                 seektable_error_callback,
                                 &cd);
  
  FLAC__stream_decoder_process_until_end_of_metadata(decoder);
  FLAC__stream_decoder_get_decode_position(decoder, &cd.start_position);

  cd.byte_position = cd.start_position;
  
  FLAC__stream_decoder_process_until_end_of_stream(decoder);
  FLAC__stream_decoder_delete(decoder);

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

#else

static void build_seek_table(flac_t * flac, FLAC__StreamMetadata * tab)
  {
  int i;

  /* We encoded fewer frames than we have in the seektable: Placeholders will remain there */
  if(flac->frame_table_len <= flac->num_seektable_entries)
    {
    for(i = 0; i < flac->frame_table_len; i++)
      {
      FLAC__metadata_object_seektable_set_point(tab,
                                                i, flac->frame_table[i]);
      }
    }
  /* More common case: We have more frames than we will have in the seek table */
  else
    {
    int index = 0;
    int64_t next_seek_sample;
    
    /* First entry is always copied */
    FLAC__metadata_object_seektable_set_point(tab,
                                              0, flac->frame_table[0]);
    index = 1;
    next_seek_sample = (flac->samples_written * index) / flac->num_seektable_entries;
    
    for(i = 1; i < flac->frame_table_len; i++)
      {
      if(flac->frame_table[i].sample_number >= next_seek_sample)
        {
        FLAC__metadata_object_seektable_set_point(tab,
                                                  index, flac->frame_table[i]);

        index++;
        next_seek_sample = (flac->samples_written * index) / flac->num_seektable_entries;

        if(index >= flac->num_seektable_entries)
          break;
        }
      }
    }
  }

static void finalize(flac_t * flac)
  {
  FLAC__Metadata_Chain    * chain;
  FLAC__Metadata_Iterator * iter;
  FLAC__StreamMetadata    * metadata;

  if(!flac->filename)
    return;
  
  chain = FLAC__metadata_chain_new();
  FLAC__metadata_chain_read(chain, flac->filename);
  iter = FLAC__metadata_iterator_new();
  FLAC__metadata_iterator_init(iter, chain);

  /* Update stream info */
  while(FLAC__metadata_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_STREAMINFO)
    FLAC__metadata_iterator_next(iter);

  metadata = FLAC__metadata_iterator_get_block(iter);
  memcpy(&metadata->data.stream_info, &flac->si, sizeof(flac->si));

  /* Seek table */
  if(flac->seektable) // Build seek table
    {
    while(FLAC__metadata_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_SEEKTABLE)
      FLAC__metadata_iterator_next(iter);
    metadata = FLAC__metadata_iterator_get_block(iter);
    build_seek_table(flac, metadata);
    }
  
  /* Write metadata back to the file */

  FLAC__metadata_chain_write(chain,
                             true, /* use_padding */
                             true  /* preserve_file_stats */ );

  /* Clean up stuff */

  FLAC__metadata_iterator_delete(iter);
  FLAC__metadata_chain_delete(chain);
  }
#endif

static int close_flac(void * data, int do_delete)
  {
  flac_t * flac;
  flac = data;

  if(flac->seektable && !do_delete)
    {
    FLAC__metadata_object_seektable_template_sort(flac->seektable, 1);
    }
  if(flac->enc)
    {
    FLAC__stream_encoder_finish(flac->enc);
    FLAC__stream_encoder_delete(flac->enc);
    flac->enc = NULL;
    }

  if(flac->out)
    {
    fclose(flac->out);
    flac->out = NULL;
    }
  if(do_delete && flac->filename)
    remove(flac->filename);
  else
    finalize(flac);
  
  free(flac->filename);
  flac->filename = NULL;
  
  if(flac->seektable)
    {
    FLAC__metadata_object_delete(flac->seektable);
    flac->seektable = NULL;
    }

  if(flac->frame_table)
    {
    free(flac->frame_table);
    flac->frame_table = NULL;
    }
  
  bg_flac_free(&flac->com);
  return 1;
  }

static void destroy_flac(void * priv)
  {
  flac_t * flac;
  flac = priv;
  close_flac(priv, 1);
  free(flac);
  }

static void set_audio_parameter_flac(void * data, int stream,
                                     const char * name,
                                     const bg_parameter_value_t * val)
  {
  flac_t * flac;
  flac = data;
  bg_flac_set_parameter(&flac->com, name, val);
  }

/* Compressed packet support */

static int writes_compressed_audio_flac(void * priv,
                            const gavl_audio_format_t * format,
                            const gavl_compression_info_t * info)
  {
  if((info->id == GAVL_CODEC_ID_FLAC) && (info->global_header_len == 42))
    return 1;
  return 0;
  }

static int add_audio_stream_compressed_flac(void * priv, const char * language,
                                            const gavl_audio_format_t * format,
                                            const gavl_compression_info_t * info)
  {
  uint16_t i_tmp;
  uint8_t * ptr;
  flac_t * flac = priv;
  flac->ci = info;

  gavl_audio_format_copy(&flac->format, format);
  flac->com.format = &flac->format;
  
  ptr = flac->ci->global_header
    + 8  // Signature + metadata header
    + 10 // min/max frame/blocksize
    + 2; // upper 16 bit of 20 samplerate bits

  // |4|3|5|4|
  i_tmp = ptr[0];
  i_tmp <<= 8;
  i_tmp |= ptr[1];
  
  flac->si.sample_rate = format->samplerate;
  flac->si.channels = format->num_channels;
  flac->si.bits_per_sample = ((i_tmp >> 4) & 0x1f)+1;
  
  memcpy(flac->si.md5sum, flac->ci->global_header + 42 - 16, 16);
  return 0;
  }

static int write_audio_packet_flac(void * priv, gavl_packet_t * packet, int stream)
  {
  flac_t * flac = priv;

  // fprintf(stderr, "%ld %ld\n", packet->duration, flac->samples_written);
  
  if(packet->data_len < 6)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Packet data too small: %d",
           packet->data_len);
    return 0;
    }
  
  if(!flac->samples_written)
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

  if(flac->data_start < 0)
    flac->data_start = flac->bytes_written;
  
  append_packet(flac, packet->duration);
  flac->samples_written += packet->duration;

  flac->si.total_samples = flac->samples_written;

  if(fwrite(packet->data, 1, packet->data_len, flac->out) == packet->data_len)
    {
    flac->bytes_written += packet->data_len;
    return 1;
    }
  else
    return 0;
  }

const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            "e_flac",       /* Unique short name */
      .long_name =       TRS("Flac encoder"),
      .description =     TRS("Encoder for flac files. Based on libflac (http://flac.sourceforge.net)"),
      .type =            BG_PLUGIN_ENCODER_AUDIO,
      .flags =           BG_PLUGIN_FILE,
      .priority =        5,
      
      .create =            create_flac,
      .destroy =           destroy_flac,
      .get_parameters =    get_parameters_flac,
      .set_parameter =     set_parameter_flac,
    },
    .max_audio_streams =   1,
    .max_video_streams =   0,
    
    .set_callbacks =       set_callbacks_flac,
    
    .open =                open_flac,
    
    .get_audio_parameters =    bg_flac_get_parameters,

    .writes_compressed_audio = writes_compressed_audio_flac,
    
    .add_audio_stream =        add_audio_stream_flac,
    .add_audio_stream_compressed = add_audio_stream_compressed_flac,
    
    
    .set_audio_parameter =     set_audio_parameter_flac,

    .get_audio_format =        get_audio_format_flac,

    .start =                   start_flac,
    
    .write_audio_frame =   write_audio_frame_flac,
    .write_audio_packet =   write_audio_packet_flac,
    .close =               close_flac
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
