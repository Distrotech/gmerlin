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

#include <gavl/numptr.h>


#include <bgflac.h>

typedef struct
  {
  bg_flac_t * enc; /* Must be first for bg_flac_set_audio_parameter */
  
  char * filename;

  FILE * out;
  
  gavl_audio_format_t format;
  
  /* Configuration stuff */
  int use_vorbis_comment;
  int use_seektable;
  int num_seektable_entries;

  int compressed;
  
  int64_t samples_written;

  bg_encoder_callbacks_t * cb;

  FLAC__StreamMetadata_StreamInfo si;
  
  int64_t data_start;
  int64_t bytes_written;

  /* Table with *all* frames */
  FLAC__StreamMetadata_SeekPoint * frame_table;
  FLAC__StreamMetadata_SeekPoint * seektable; 
  
  uint32_t frame_table_len;
  uint32_t frame_table_alloc;

  gavl_compression_info_t ci;
  
  int fixed_blocksize;

  gavl_audio_sink_t * sink;
  gavl_packet_sink_t * psink_int;
  gavl_packet_sink_t * psink_ext;
  
  gavl_metadata_t * stream_metadata;
  gavl_metadata_t * global_metadata;
  
  } flac_t;

#define MY_WRITE(ptr, len) \
  if(fwrite(ptr, 1, len, out) < len)                \
    return 0;

static int write_comment(const gavl_metadata_t * m_stream,
                         const gavl_metadata_t * m_global,
                         FILE * out,
                         int last)
  {
  return 1;
  }

static int write_seektable(FLAC__StreamMetadata_SeekPoint * index,
                           int len,
                           FILE * out,
                           int last)
  {
  int i;
  int len_bytes;
  uint8_t buf[8];

  buf[0] = 3;
  if(last)
    buf[0] |= 0x80;
  MY_WRITE(buf, 1);

  len_bytes = len * 18;
  GAVL_24BE_2_PTR(len_bytes, buf);
  MY_WRITE(buf, 3);

  for(i = 0; i < len; i++)
    {
    GAVL_64BE_2_PTR(index[i].sample_number, buf);
    MY_WRITE(buf, 8);

    GAVL_64BE_2_PTR(index[i].stream_offset, buf);
    MY_WRITE(buf, 8);

    GAVL_16BE_2_PTR(index[i].frame_samples, buf);
    MY_WRITE(buf, 2);
    }
  
  return 1;
  }

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
                     const gavl_metadata_t * m,
                     const gavl_chapter_list_t * chapter_list)
  {
  int result = 1;
  flac_t * flac;
  
  flac = data;
  flac->enc = bg_flac_create();
  
  flac->filename = bg_filename_ensure_extension(filename, "flac");

  if(!bg_encoder_cb_create_output_file(flac->cb, flac->filename))
    return 0;

  flac->out = fopen(flac->filename, "wb");
  
  /* Create seektable */

  if(flac->use_seektable)
    {
    int i;
    flac->seektable = calloc(flac->num_seektable_entries, sizeof(*flac->seektable));
    for(i = 0; i < flac->num_seektable_entries; i++)
      flac->seektable[i].sample_number = 0xFFFFFFFFFFFFFFFFLL;
    }
  
  
  return result;
  }

static int add_audio_stream_flac(void * data,
                                 const gavl_metadata_t * m,
                                 const gavl_audio_format_t * format)
  {
  flac_t * flac;
  flac = data;

  /* Copy and adjust format */

  gavl_audio_format_copy(&flac->format, format);
  
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

static gavl_sink_status_t put_packet_int(void * data, gavl_packet_t * p)
  {
  flac_t * flac;
  flac = data;

  /* Check if we got a real frame */
  if(flac->data_start < 0)
    flac->data_start = flac->bytes_written;
  
  append_packet(flac, p->duration);
  
  if(fwrite(p->data, 1, p->data_len, flac->out) == p->data_len)
    {
    flac->bytes_written += p->data_len;
    return GAVL_SINK_OK;
    }
  return GAVL_SINK_ERROR;
  }

static gavl_sink_status_t
write_audio_func_flac(void * data, gavl_audio_frame_t * frame)
  {
  gavl_sink_status_t ret = GAVL_SINK_OK;
  flac_t * flac;
  
  flac = data;

  bg_flac_encode_audio_frame(flac->enc, frame);
  flac->samples_written += frame->valid_samples;
  return ret;
  }

static gavl_sink_status_t
write_audio_packet_func_flac(void * priv, gavl_packet_t * packet)
  {
  flac_t * flac = priv;

  // fprintf(stderr, "%ld %ld\n", packet->duration, flac->samples_written);
  
  if(packet->data_len < 6)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Packet data too small: %d",
           packet->data_len);
    return GAVL_SINK_ERROR;
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
    return GAVL_SINK_OK;
    }
  else
    return GAVL_SINK_ERROR;
  }


static int write_audio_frame_flac(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  flac_t * flac;
  flac = data;
  return (gavl_audio_sink_put_frame(flac->sink, frame) == GAVL_SINK_OK);
  }

static int
write_audio_packet_flac(void * priv, gavl_packet_t * packet, int stream)
  {
  flac_t * flac = priv;
  return (gavl_packet_sink_put_packet(flac->psink_ext, packet) == GAVL_SINK_OK);
  }

static int start_flac(void * data)
  {
  flac_t * flac;
  flac = data;

  if(!bg_flac_start(flac->enc, &flac->format, &flac->ci,
                    flac->stream_metadata))
    return 0;
  
  flac->data_start = -1;

  if(flac->compressed)
    flac->psink_ext = gavl_packet_sink_create(NULL, write_audio_packet_func_flac,
                                              flac);
  else
    flac->sink = gavl_audio_sink_create(NULL, write_audio_func_flac,
                                        flac, &flac->format);
  
  return 1;
  }

static void get_audio_format_flac(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  flac_t * flac;
  flac = data;
  gavl_audio_format_copy(ret, &flac->format);
  }

static gavl_audio_sink_t * get_audio_sink_flac(void * data, int stream)
  {
  flac_t * flac;
  flac = data;
  return flac->sink;
  }

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

static int close_flac(void * data, int do_delete)
  {
  flac_t * flac;
  flac = data;

  bg_flac_free(flac->enc);
#if 0
  if(flac->enc)
    {
    FLAC__stream_encoder_finish(flac->enc);
    FLAC__stream_encoder_delete(flac->enc);
    flac->enc = NULL;
    }
#endif
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
    free(flac->seektable);
    flac->seektable = NULL;
    }

  if(flac->frame_table)
    {
    free(flac->frame_table);
    flac->frame_table = NULL;
    }

  if(flac->sink)
    {
    gavl_audio_sink_destroy(flac->sink);
    flac->sink = NULL;
    }
  if(flac->psink_ext)
    {
    gavl_packet_sink_destroy(flac->psink_ext);
    flac->psink_ext = NULL;
    }
  
  bg_flac_free(flac->enc);
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
  bg_flac_set_parameter(flac->enc, name, val);
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

static int
add_audio_stream_compressed_flac(void * priv,
                                 const gavl_metadata_t * m,
                                 const gavl_audio_format_t * format,
                                 const gavl_compression_info_t * info)
  {
  uint16_t i_tmp;
  uint8_t * ptr;
  flac_t * flac = priv;

  gavl_compression_info_copy(&flac->ci, info);
  flac->compressed = 1;
  
  gavl_audio_format_copy(&flac->format, format);
  
  ptr = flac->ci.global_header
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
  
  memcpy(flac->si.md5sum, flac->ci.global_header + 42 - 16, 16);
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
    .get_audio_sink =          get_audio_sink_flac,
    .start =                   start_flac,
    
    .write_audio_frame =   write_audio_frame_flac,
    .write_audio_packet =   write_audio_packet_flac,
    .close =               close_flac
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
