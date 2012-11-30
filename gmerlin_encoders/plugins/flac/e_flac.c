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
#include <vorbiscomment.h>


typedef struct
  {
  bg_flac_t * enc;
  
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
  
  int64_t data_start;
  int64_t seektable_start;
  
  int64_t bytes_written;

  /* Table with *all* frames */
  FLAC__StreamMetadata_SeekPoint * frame_table;
  uint32_t frame_table_len;
  uint32_t frame_table_alloc;
  
  /* Generated seek table */
  FLAC__StreamMetadata_SeekPoint * seektable; 
  
  gavl_compression_info_t ci;
  
  int fixed_blocksize;

  gavl_audio_sink_t * sink;
  gavl_packet_sink_t * psink_int;
  gavl_packet_sink_t * psink_ext;
  
  gavl_metadata_t m_stream;
  const gavl_metadata_t * m_global;
  
  } flac_t;

static int write_data(flac_t * f, const uint8_t * data, int len)
  {
  if(fwrite(data, 1, len, f->out) < len)
    return 0;
  f->bytes_written += len;
  return 1;
  }

static int write_comment(flac_t * f, int last)
  {
  int ret = 0;
  uint8_t * buf;
  uint8_t * ptr;
  
  int len = bg_vorbis_comment_bytes(&f->m_stream, f->m_global, 0);
  
  buf = malloc(4 + len);
  ptr = buf;
  
  ptr[0] = 0x04;
  if(last)
    ptr[0] |= 0x80;
  ptr++;

  GAVL_24BE_2_PTR(len, ptr); ptr += 3;
  bg_vorbis_comment_write(ptr, &f->m_stream, f->m_global, 0);
  write_data(f, buf, len+4);
  free(buf);
  return ret;
  }

static int write_seektable(FLAC__StreamMetadata_SeekPoint * index,
                           int len,
                           flac_t * f,
                           int last)
  {
  int i;
  int len_bytes;
  uint8_t buf[18];
  int ret = 0;
  uint8_t * ptr;

  ptr = buf;
  
  ptr[0] = 3;
  if(last)
    ptr[0] |= 0x80;

  ptr++;
  
  len_bytes = len * 18;
  GAVL_24BE_2_PTR(len_bytes, ptr);
  write_data(f, buf, 4);
  
  for(i = 0; i < len; i++)
    {
    ptr = buf;
    GAVL_64BE_2_PTR(index[i].sample_number, ptr); ptr += 8;
    GAVL_64BE_2_PTR(index[i].stream_offset, ptr); ptr += 8;
    GAVL_16BE_2_PTR(index[i].frame_samples, ptr); ptr += 2;
    write_data(f, buf, 18);
    }
  
  return ret;
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

static int streaminfo_callback(void * data, uint8_t * si, int len)
  {
  int first;
  flac_t * flac = data;

  if(flac->bytes_written)
    first = 0;
  else
    first = 1;

  fprintf(stderr, "streaminfo_callback %d\n", first);
  bg_hexdump(si, len, 16);
  
  if(first)
    {
    /* Set or clear the "last metadata packet" flag */

    if(flac->use_seektable || flac->use_vorbis_comment)
      si[4] &= 0x7f;
    else
      si[4] |= 0x80;
    
    /* Write stream info */
    if(!write_data(flac, si, len))
      return 0;

    if(flac->use_vorbis_comment)
      {
      write_comment(flac,
                    !flac->use_seektable);
      }
    if(flac->use_seektable)
      {
      flac->seektable_start = flac->bytes_written;
      write_seektable(flac->seektable,
                      flac->num_seektable_entries,
                      flac, 1);
      }
    }
  else
    {
    fseek(flac->out, 0, SEEK_SET);
    if(!write_data(flac, si, len))
      return 0;
    }
  return 1;
  }

static int open_flac(void * data, const char * filename,
                     const gavl_metadata_t * m,
                     const gavl_chapter_list_t * chapter_list)
  {
  int result = 1;
  flac_t * flac;
  
  flac = data;
  flac->enc = bg_flac_create();

  bg_flac_set_callbacks(flac->enc,
                        streaminfo_callback, flac);
  
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

  flac->m_global = m;
  
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
  gavl_metadata_copy(&flac->m_stream, m);
  
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

  f->frame_table[f->frame_table_len].sample_number = f->samples_written;
  f->frame_table[f->frame_table_len].frame_samples = samples;
  f->frame_table[f->frame_table_len].stream_offset = f->bytes_written - f->data_start;
  f->frame_table_len++;

  //  fprintf(stderr, "Append packet %ld %d -> %ld\n", f->samples_written, samples,
  //          f->samples_written + samples);
  
  f->samples_written += samples;
  
  }

static gavl_sink_status_t
write_audio_packet_func_flac(void * priv, gavl_packet_t * packet)
  {
  flac_t * flac = priv;
  
  if(flac->data_start < 0)
    flac->data_start = flac->bytes_written;
  
  append_packet(flac, packet->duration);
  
  if(write_data(flac, packet->data, packet->data_len))
    return GAVL_SINK_OK;
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

  if(flac->compressed)
    {
    if(!(flac->psink_ext = bg_flac_start_compressed(flac->enc, &flac->format, &flac->ci,
                                                    &flac->m_stream)))
      return 0;
    }
  else
    {
    if(!(flac->sink = bg_flac_start_uncompressed(flac->enc, &flac->format, &flac->ci,
                                                 &flac->m_stream)))
      return 0;
    }
  
  flac->psink_int =
    gavl_packet_sink_create(NULL, write_audio_packet_func_flac, flac);
  bg_flac_set_sink(flac->enc, flac->psink_int);

  flac->data_start = -1;
  
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

static void build_seek_table(flac_t * flac)
  {
  int i;

  /* We encoded fewer frames than we have in the seektable: Placeholders will remain there */
  if(flac->frame_table_len <= flac->num_seektable_entries)
    {
    memcpy(flac->seektable, flac->frame_table,
           flac->frame_table_len * sizeof(*flac->seektable));
    }
  /* More common case: We have more frames than we will have in the seek table */
  else
    {
    int index = 0;
    int64_t next_seek_sample;
    
    /* First entry is always copied */
    memcpy(flac->seektable, flac->frame_table,
           sizeof(*flac->seektable));
    
    index = 1;
    next_seek_sample = (flac->samples_written * index) / flac->num_seektable_entries;
    
    for(i = 1; i < flac->frame_table_len; i++)
      {
      if(flac->frame_table[i].sample_number >= next_seek_sample)
        {
        memcpy(flac->seektable + index, flac->frame_table + i,
               sizeof(*flac->seektable));
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
  if(!flac->out)
    return;
  
  /* Update stream info */
  
  /* Seek table */
  if(flac->seektable) // Build seek table
    {
    build_seek_table(flac);
    fseek(flac->out, flac->seektable_start, SEEK_SET);
    write_seektable(flac->seektable,
                    flac->num_seektable_entries,
                    flac, 1);
    }
  }

static int close_flac(void * data, int do_delete)
  {
  flac_t * flac;
  flac = data;

  /* Flush and free the codec */
  if(flac->enc)
    {
    bg_flac_free(flac->enc);
    flac->enc = NULL;
    }

  /* Finalize output file */
  if(flac->out)
    {
    if(do_delete)
      {
      fclose(flac->out);
      flac->out = NULL;
      remove(flac->filename);
      }
    else
      {
      finalize(flac);
      fclose(flac->out);
      flac->out = NULL;
      }
    }

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
  if(flac->psink_int)
    {
    gavl_packet_sink_destroy(flac->psink_int);
    flac->psink_int = NULL;
    }
  if(flac->psink_ext)
    {
    gavl_packet_sink_destroy(flac->psink_ext);
    flac->psink_ext = NULL;
    }
  if(flac->sink)
    {
    gavl_audio_sink_destroy(flac->sink);
    flac->sink = NULL;
    }

  gavl_metadata_free(&flac->m_stream);
  
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
  flac_t * flac = priv;
  gavl_compression_info_copy(&flac->ci, info);
  gavl_audio_format_copy(&flac->format, format);
  gavl_metadata_copy(&flac->m_stream, m);
  flac->compressed = 1;
  return 0;
  }

static const bg_parameter_info_t * get_audio_parameters_flac(void * priv)
  {
  return bg_flac_get_parameters();
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
    
    .get_audio_parameters =    get_audio_parameters_flac,

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
