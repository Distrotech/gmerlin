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
#include <errno.h>

#include <config.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "e_flac"

#include <bgflac.h>

typedef struct
  {
  bg_flac_t com; /* Must be first for bg_flac_set_audio_parameter */
  
  char * error_msg;
  char * filename;
  
  gavl_audio_format_t format;
  FLAC__FileEncoder * enc;

  /* Metadata stuff */
  
  FLAC__StreamMetadata * seektable; 

  FLAC__StreamMetadata * metadata[2];
  int num_metadata;
  
  /* Configuration stuff */
  int use_vorbis_comment;
  int use_seektable;
  int num_seektable_entries;

  int64_t samples_written;
  
  } flac_t;

static void * create_flac()
  {
  flac_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }


static const char * get_error_flac(void * priv)
  {
  flac_t * flac;
  flac = (flac_t*)priv;
  return flac->error_msg;
  }

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

static bg_parameter_info_t * get_parameters_flac(void * data)
  {
  return parameters;
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


static void create_seektable(flac_t * flac)
  {
  flac->seektable =
    FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);

  FLAC__metadata_object_seektable_template_append_placeholders(flac->seektable,
                                                               flac->num_seektable_entries);
  flac->metadata[flac->num_metadata++] = flac->seektable;
  }

static int open_flac(void * data, const char * filename,
                    bg_metadata_t * m, bg_chapter_list_t * chapter_list)
  {
  int result = 1;
  flac_t * flac;
  
  flac = (flac_t*)data;

  /* Create encoder instance */
  flac->enc = FLAC__file_encoder_new();
  
  flac->filename = bg_strdup(flac->filename, filename);
  FLAC__file_encoder_set_filename(flac->enc, flac->filename);

  /* Create vorbis comment */

  if(flac->use_vorbis_comment)
    {
    bg_flac_init_metadata(&flac->com, m);
    flac->metadata[flac->num_metadata++] = flac->com.vorbis_comment;
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

static int add_audio_stream_flac(void * data,
                                 const char * language,
                                 gavl_audio_format_t * format)
  {
  flac_t * flac;
  flac = (flac_t*)data;

  /* Copy and adjust format */

  gavl_audio_format_copy(&(flac->format), format);
  flac->com.format = &(flac->format);
  
  return 0;
  }

static int start_flac(void * data)
  {
  flac_t * flac;
  flac = (flac_t*)data;

  bg_flac_init_file_encoder(&flac->com, flac->enc);
  
  /* Initialize encoder */
  if(FLAC__file_encoder_init(flac->enc) != FLAC__FILE_ENCODER_OK)
    {
    if(errno)
      flac->error_msg = bg_sprintf("Initializing encoder failed: %s",
                                   strerror(errno));
    else
      flac->error_msg = bg_sprintf("Initializing encoder failed");
    
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, flac->error_msg);
    return 0;
    }
  return 1;
  }

static int write_audio_frame_flac(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  int ret = 1;
  flac_t * flac;
  
  flac = (flac_t*)data;

  bg_flac_prepare_audio_frame(&flac->com, frame);
  
  /* Encode */

  if(!FLAC__file_encoder_process(flac->enc, (const FLAC__int32 **) flac->com.buffer,
                                 frame->valid_samples))
    ret = 0;
  flac->samples_written += frame->valid_samples;
  return ret;
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
  }

static void seektable_metadata_callback(const FLAC__FileDecoder *decoder,
                                        const FLAC__StreamMetadata *metadata,
                                        void *client_data)
  {
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

static int close_flac(void * data, int do_delete)
  {
  flac_t * flac;
  flac = (flac_t*)data;

  if(flac->seektable && !do_delete)
    {
    FLAC__metadata_object_seektable_template_sort(flac->seektable, 1);
    }
  if(flac->enc)
    {
    FLAC__file_encoder_finish(flac->enc);
    FLAC__file_encoder_delete(flac->enc);
    flac->enc = NULL;
    }
  
  if(do_delete && flac->filename)
    remove(flac->filename);
  else if(flac->seektable)
    finalize_seektable(flac);
  
  free(flac->filename);
  flac->filename = (char*)0;
  
  if(flac->seektable)
    {
    FLAC__metadata_object_delete(flac->seektable);
    flac->seektable = NULL;
    }
  bg_flac_free(&flac->com);
  return 1;
  }

static void destroy_flac(void * priv)
  {
  flac_t * flac;
  flac = (flac_t*)priv;
  close_flac(priv, 1);
  free(flac);
  }

static void set_audio_parameter_flac(void * data, int stream, char * name,
                                     bg_parameter_value_t * val)
  {
  flac_t * flac;
  flac = (flac_t*)data;
  bg_flac_set_parameter(&(flac->com), name, val);
  }

bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      name:            "e_flac",       /* Unique short name */
      long_name:       "Flac encoder",
      mimetypes:       NULL,
      extensions:      "flac",
      type:            BG_PLUGIN_ENCODER_AUDIO,
      flags:           BG_PLUGIN_FILE,
      priority:        5,
      
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
    
    get_audio_parameters:    bg_flac_get_parameters,

    add_audio_stream:        add_audio_stream_flac,
    
    set_audio_parameter:     set_audio_parameter_flac,

    get_audio_format:        get_audio_format_flac,

    start:                   start_flac,
    
    write_audio_frame:   write_audio_frame_flac,
    close:               close_flac
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
