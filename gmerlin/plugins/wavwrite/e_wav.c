/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

/* Simple Wav encoder. Supports only PCM (0x0001), but
   up to 32 bits, multi channel capable and with metadata */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <config.h>
#include <gmerlin/translation.h>


#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "e_wav"

#include <gavl/metatags.h>

/* Speaker configurations for WAVEFORMATEXTENSIBLE */

#define SPEAKER_FRONT_LEFT 	        0x1
#define SPEAKER_FRONT_RIGHT 	        0x2
#define SPEAKER_FRONT_CENTER 	        0x4
#define SPEAKER_LOW_FREQUENCY 	        0x8
#define SPEAKER_BACK_LEFT 	        0x10
#define SPEAKER_BACK_RIGHT 	        0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER 	0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER 	0x80
#define SPEAKER_BACK_CENTER 	        0x100
#define SPEAKER_SIDE_LEFT 	        0x200
#define SPEAKER_SIDE_RIGHT 	        0x400
#define SPEAKER_TOP_CENTER 	        0x800
#define SPEAKER_TOP_FRONT_LEFT 	        0x1000
#define SPEAKER_TOP_FRONT_CENTER 	0x2000
#define SPEAKER_TOP_FRONT_RIGHT 	0x4000
#define SPEAKER_TOP_BACK_LEFT 	        0x8000
#define SPEAKER_TOP_BACK_CENTER 	0x10000
#define SPEAKER_TOP_BACK_RIGHT 	        0x20000

#define RIFF_SIZE_OFFSET 4
// #define DATA_SIZE_OFFSET 40

#define FORMAT_WAV 0
#define FORMAT_RAW 1

typedef struct wav_s
  {
  // int bytes_per_sample;

  int block_align;
  int bits;

  int file_format;
  
  gavf_io_t * output;
  int data_size_offset;
  gavl_audio_format_t format;

  int write_info_chunk;
  gavl_metadata_t metadata;
  char * filename;
  uint32_t channel_mask;

  gavl_audio_frame_t * frame;
  
  uint8_t * buffer;
  int buffer_alloc;
  void (*convert_func)(struct wav_s*, uint8_t * samples, int num_samples);
  bg_encoder_callbacks_t * cb;
  gavl_audio_sink_t * sink;

  int be;
  int swap_endian;
  gavl_dsp_context_t * dsp;

  
  } wav_t;

static int write_8(gavf_io_t * output, uint32_t val)
  {
  uint8_t c = val;
  if(gavf_io_write_data(output, &c, 1) < 1)
    return 0;
  return 1;
  }

static int write_16(gavf_io_t * output, uint32_t val)
  {
  uint8_t c[2];
  
  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  
  if(gavf_io_write_data(output, c, 2) < 2)
    return 0;
  
  return 1;
  }

static int write_32(gavf_io_t* output, uint32_t val)
  {
  uint8_t c[4];
  
  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  c[2] = (val >> 16) & 0xff;
  c[3] = (val >> 24) & 0xff;
  if(gavf_io_write_data(output, c, 4) < 4)
    return 0;
  return 1;
  }

static int write_fourcc(gavf_io_t* output, char c1, char c2, char c3, char c4)
  {
  char c[4];

  c[0] = c1;
  c[1] = c2;
  c[2] = c3;
  c[3] = c4;
  if(gavf_io_write_data(output, (uint8_t*)c, 4) < 4)
    return 0;
  return 1;
  }

/* Functions for writing signed integers */

#ifdef WORDS_BIGENDIAN
static void convert_16_be(struct wav_s*w, uint8_t * samples, int num_samples)
  {
  int i;
  uint8_t * src = samples;
  uint8_t * dst = w->buffer;
  
  for(i = 0; i < num_samples; i++)
    {
    dst[0] = src[1];
    dst[1] = src[0];
    dst+=2;
    src+=2;
    }
  }

static void convert_32_be(struct wav_s*w, uint8_t * samples, int num_samples)
  {
  int i;
  uint8_t * src = samples;
  uint8_t * dst = w->buffer;
  for(i = 0; i < num_samples; i++)
    {
    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];
    dst+=4;
    src+=4;
    }
  }
#endif

static void convert_24_le(struct wav_s*w, uint8_t * samples, int num_samples)
  {
  int i;
  uint32_t * src = (uint32_t*)samples;
  uint8_t * dst = w->buffer;

  for(i = 0; i < num_samples; i++)
    {
    dst[0] = (src[0] >> 8) & 0xff;
    dst[1] = (src[0] >> 16) & 0xff;
    dst[2] = (src[0] >> 24) & 0xff;
    src++;
    dst+=3;
    }
  }

static void convert_24_be(struct wav_s*w, uint8_t * samples, int num_samples)
  {
  int i;
  uint32_t * src = (uint32_t*)samples;
  uint8_t * dst = w->buffer;

  for(i = 0; i < num_samples; i++)
    {
    dst[0] = (src[0] >> 24) & 0xff;
    dst[1] = (src[0] >> 16) & 0xff;
    dst[2] = (src[0] >> 8) & 0xff;
    src++;
    dst+=3;
    }
  }


/* Write info chunk */

typedef struct
  {
  const char * IART;
  const char * INAM;
  const char * ICMT;
  const char * ICOP;
  const char * IGNR;
  const char * ICRD;
  const char * ISFT;
  } info_chunk_t;

#define GET_LEN(tag) \
  if(info.tag) \
    { \
    len = strlen(info.tag)+1; \
    if(len % 2) \
      len++; \
    if(len) \
      total_len += 8 + len; \
    }

#define WRITE_STRING(tag) \
  if(info.tag) \
    { \
    len = strlen(info.tag)+1; \
    if(len > 1) \
      { \
      if(gavf_io_write_data(output, (const uint8_t*)#tag, 4) < 4)        \
        return 0; \
      write_32(output, len); \
      if(gavf_io_write_data(output, (const uint8_t*)info.tag, len) < len)     \
        return 0; \
      if(len % 2) \
        { \
        if(!write_8(output, 0x00)) \
          return 0; \
        } \
      } \
    }

#define BUFFER_SIZE 256

static int write_info_chunk(gavf_io_t * output, gavl_metadata_t * m)
  {
  int len;
  int total_len;
  struct tm time_date;
  time_t t;
  
  char date_string[BUFFER_SIZE];
  char software_string[BUFFER_SIZE];
  
  info_chunk_t info;
  memset(&info, 0, sizeof(info));

  /* Set up the structure */

  time(&t);
  localtime_r(&t, &time_date);
  strftime(date_string, BUFFER_SIZE, "%Y-%m-%d", &time_date);
  sprintf(software_string, "%s-%s", PACKAGE, VERSION);

  info.IART = gavl_metadata_get(m, GAVL_META_ARTIST);
  info.INAM = gavl_metadata_get(m, GAVL_META_TITLE);
  info.ICMT = gavl_metadata_get(m, GAVL_META_COMMENT);
  info.ICOP = gavl_metadata_get(m, GAVL_META_COPYRIGHT);
  info.IGNR = gavl_metadata_get(m, GAVL_META_GENRE);
  info.ICRD = date_string;
  info.ISFT = software_string;

  /* Write the thing */
  total_len = 4;

  GET_LEN(IART);
  GET_LEN(INAM);
  GET_LEN(ICMT);
  GET_LEN(ICOP);
  GET_LEN(IGNR);
  GET_LEN(ICRD);
  GET_LEN(ISFT);

  /* Write chunk header */

  if(!write_fourcc(output, 'L', 'I', 'S', 'T') ||
     !write_32(output, total_len) ||
     !write_fourcc(output, 'I', 'N', 'F', 'O'))
    return 0;
  
  /* Write strings */

  WRITE_STRING(IART);
  WRITE_STRING(INAM);
  WRITE_STRING(ICMT);
  WRITE_STRING(ICOP);
  WRITE_STRING(IGNR);
  WRITE_STRING(ICRD);
  WRITE_STRING(ISFT);

  return 1;
  }


static void * create_wav()
  {
  wav_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void set_callbacks_wav(void * data, bg_encoder_callbacks_t * cb)
  {
  wav_t * e = data;
  e->cb = cb;
  }

static void destroy_wav(void * priv)
  {
  wav_t * wav;
  wav = priv;

  if(wav->output)
    gavf_io_destroy(wav->output);

  if(wav->sink)
    gavl_audio_sink_destroy(wav->sink);
  
  free(wav);
  }

static const bg_parameter_info_t audio_parameters[] =
  {
    {
      .name =        "bits",
      .long_name =   TRS("Bits per sample"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "16" },
      .multi_names =     (char const *[]){ "8", "16", "24", "32", NULL },
    },
    {
      .name =        "be",
      .long_name =   TRS("Output big endian (Raw only)"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "format",
      .long_name =   TRS("Format"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "waw" },
      .multi_names = (char const*[]){ "wav", "raw", NULL},
      .multi_labels = (char const*[]){ "WAV", "Raw", NULL},
    },
    {
      .name =        "write_info_chunk",
      .long_name =   TRS("Write info chunk"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_audio_parameters_wav(void * data)
  {
  return audio_parameters;
  }

static const bg_parameter_info_t * get_parameters_wav(void * data)
  {
  return parameters;
  }

static int write_PCMWAVEFORMAT(wav_t * wav)
  {
  return
    (write_32(wav->output, 16) &&     /* Size   */
     write_16(wav->output, 0x0001) && /* wFormatTag */
     write_16(wav->output, wav->format.num_channels) && /* nChannels */
     write_32(wav->output, wav->format.samplerate) &&   /* nSamplesPerSec */
     write_32(wav->output, wav->block_align *
              wav->format.samplerate) &&                /* nAvgBytesPerSec */
     write_16(wav->output, wav->block_align) &&         /* nBlockAlign */
     write_16(wav->output, (wav->block_align / wav->format.num_channels) * 8)); /* wBitsPerSample */
  }

const struct
  {
  int flag;
  gavl_channel_id_t id;
  }
channel_flags[] =
  {
    { SPEAKER_FRONT_LEFT,            GAVL_CHID_FRONT_LEFT },
    { SPEAKER_FRONT_RIGHT,           GAVL_CHID_FRONT_RIGHT },
    { SPEAKER_FRONT_CENTER,          GAVL_CHID_FRONT_CENTER },
    { SPEAKER_LOW_FREQUENCY,         GAVL_CHID_LFE },
    { SPEAKER_BACK_LEFT,             GAVL_CHID_REAR_LEFT },
    { SPEAKER_BACK_RIGHT,            GAVL_CHID_REAR_RIGHT },
    { SPEAKER_FRONT_LEFT_OF_CENTER,  GAVL_CHID_FRONT_CENTER_LEFT },
    { SPEAKER_FRONT_RIGHT_OF_CENTER, GAVL_CHID_FRONT_CENTER_RIGHT },
    { SPEAKER_BACK_CENTER,           GAVL_CHID_REAR_CENTER },
    { SPEAKER_SIDE_LEFT,             GAVL_CHID_SIDE_LEFT },
    { SPEAKER_SIDE_RIGHT,            GAVL_CHID_SIDE_RIGHT },
  };

static uint32_t format_2_channel_mask(gavl_audio_format_t * format)
  {
  int i;
  uint32_t ret = 0;
  gavl_channel_id_t new_locations[GAVL_MAX_CHANNELS];
  int channel_index = 0;
  
  for(i = 0; i < sizeof(channel_flags)/sizeof(channel_flags[0]); i++)
    {
    if(gavl_channel_index(format, channel_flags[i].id) >= 0)
      {
      new_locations[channel_index++] = channel_flags[i].id;
      ret |= channel_flags[i].flag;
      }
    }
  memcpy(format->channel_locations,
         new_locations,
         format->num_channels * sizeof(new_locations[0]));
  return ret;
  }


static int write_WAVEFORMATEXTENSIBLE(wav_t * wav)
  {
  uint8_t guid[16] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                      0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };

  return(write_32(wav->output, 40) &&                       /* Size   */
         write_16(wav->output, 0xfffe) &&                   /* wFormatTag */
         write_16(wav->output, wav->format.num_channels) && /* nChannels */
         write_32(wav->output, wav->format.samplerate) &&   /* nSamplesPerSec */
         write_32(wav->output, wav->block_align *
                  wav->format.samplerate) &&                /* nAvgBytesPerSec */
         write_16(wav->output, wav->block_align) &&         /* nBlockAlign */
         write_16(wav->output, (wav->block_align / wav->format.num_channels) * 8) && /* wBitsPerSample */
         write_16(wav->output, 22) &&                        /* cbSize         */
         write_16(wav->output, (wav->block_align / wav->format.num_channels) * 8) && /* wValidBitsPerSample */

  /* Write channel mask */

         write_32(wav->output, wav->channel_mask) &&         /* dwChannelMask */
         (gavf_io_write_data(wav->output, guid, 16) == 16));
    }

static void set_audio_parameter_wav(void * data, int stream,
                                    const char * name,
                                    const bg_parameter_value_t * v)
  {
  wav_t * wav;
  wav = data;
  
  if(stream)
    return;
    
  if(!name)
    return;
  
  if(!strcmp(name, "bits"))
    wav->bits = atoi(v->val_str);
  else if(!strcmp(name, "be"))
    wav->be = v->val_i;
}

static void set_parameter_wav(void * data, const char * name,
                              const bg_parameter_value_t * v)
  {
  wav_t * wav;
  wav = data;
  
  if(!name)
    return;
  if(!strcmp(name, "write_info_chunk"))
    wav->write_info_chunk = v->val_i;
  if(!strcmp(name, "format"))
    {
    if(!strcmp(v->val_str, "wav"))
      wav->file_format = FORMAT_WAV;
    if(!strcmp(v->val_str, "raw"))
      wav->file_format = FORMAT_RAW;
    }
  }

static int open_io_wav(void * data, gavf_io_t * io,
                       const gavl_metadata_t * metadata,
                       const gavl_chapter_list_t * chapter_list)
  {
  wav_t * wav;
  wav = data;
  wav->output = io;

  if(metadata)
    gavl_metadata_copy(&wav->metadata, metadata);

  return 1;
  }

static int open_wav(void * data, const char * filename,
                    const gavl_metadata_t * metadata,
                    const gavl_chapter_list_t * chapter_list)
  {
  wav_t * wav;
  gavf_io_t * io;
  wav = data;

  if(!strcmp(filename, "-"))
    {
    if(!(wav->file_format != FORMAT_RAW))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Only raw audio can be written to a pipe");
      return 0;
      }
    io = gavf_io_create_file(stdout, 1, 0, 0);
    }
  else
    {
    FILE * f;
    wav->filename = bg_filename_ensure_extension(filename, "wav");

    if(!bg_encoder_cb_create_output_file(wav->cb, wav->filename))
      return 0;

    f = fopen(wav->filename, "wb");
    if(!f)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s: %s",
             wav->filename, strerror(errno));
      return 0;
      }
    io = gavf_io_create_file(f, 1, 1, 1);
    }

  return open_io_wav(data, io, metadata, chapter_list);
  }


static int add_audio_stream_wav(void * data,
                                const gavl_metadata_t * m,
                                const gavl_audio_format_t * format)
  {
  wav_t * wav;
  
  wav = data;

  gavl_audio_format_copy(&wav->format, format);

  
  wav->format.interleave_mode = GAVL_INTERLEAVE_ALL;
  return 0;
  }

static gavl_sink_status_t write_func_wav(void * data, gavl_audio_frame_t * frame)
  {
  int num_bytes;
  wav_t * wav = data;

  num_bytes = frame->valid_samples * wav->block_align;
  
  if(wav->convert_func)
    {
    if(wav->buffer_alloc < num_bytes)
      {
      wav->buffer_alloc = num_bytes + 1024;
      wav->buffer = realloc(wav->buffer, wav->buffer_alloc);
      }
    wav->convert_func(wav, frame->samples.u_8, frame->valid_samples * wav->format.samples_per_frame);
    if(gavf_io_write_data(wav->output, wav->buffer, num_bytes) < num_bytes)
      return GAVL_SINK_ERROR;
    }
  else
    {
    if(wav->swap_endian)
      gavl_dsp_audio_frame_swap_endian(wav->dsp,
                                       wav->frame,
                                       &wav->format);
    
    if(gavf_io_write_data(wav->output, frame->samples.u_8, num_bytes) < num_bytes)
      return GAVL_SINK_ERROR;
    }
  return GAVL_SINK_OK;
  }

static gavl_audio_frame_t * get_func_wav(void * data)
  {
  wav_t * wav = data;
  return wav->frame;
  }

static int write_wav_header(wav_t * wav)
  {
  /* Write the header and adjust format */
  
  if(!write_fourcc(wav->output, 'R', 'I', 'F', 'F') || /* "RIFF" */
     !write_32(wav->output, 0x00000000) ||             /*  size  */
     !write_fourcc(wav->output, 'W', 'A', 'V', 'E') || /* "WAVE" */
     !write_fourcc(wav->output, 'f', 'm', 't', ' ')) /* "fmt " */
    return 0;
  
  /* Build channel mask and adjust channel locations */
  wav->channel_mask = format_2_channel_mask(&wav->format);
  
  if(wav->format.num_channels <= 2)
    {
    if(!write_PCMWAVEFORMAT(wav))
      return 0;
    }
  else
    {
    if(!write_WAVEFORMATEXTENSIBLE(wav))
      return 0;
    }
  /* Start data section */
  
  write_fourcc(wav->output, 'd', 'a', 't', 'a'); /* "data" */
  
  wav->data_size_offset = gavf_io_position(wav->output);
  write_32(wav->output, 0x00000000);             /*  size  */
  return 1;
  }

static int start_wav(void * data)
  {
  wav_t * wav;
  wav = data;

  wav->block_align = (wav->bits / 8) * wav->format.num_channels;
  
  if((wav->file_format == FORMAT_WAV) &&
     !write_wav_header(wav))
    return 0;
  
  /* Adjust format */
  
  switch(wav->bits)
    {
    case 8:
      wav->format.sample_format = GAVL_SAMPLE_U8;
      break;
    case 16:
      wav->format.sample_format = GAVL_SAMPLE_S16;
#ifdef WORDS_BIGENDIAN
      if(!wav->be)
        wav->swap_endian = 1;
#else
      if(wav->be)
        wav->swap_endian = 1;
#endif
      break;
    case 24:
      if(wav->be)
        wav->convert_func = convert_24_be;
      else
        wav->convert_func = convert_24_le;
      
      wav->format.sample_format = GAVL_SAMPLE_S32;
      break;
    case 32:
#ifdef WORDS_BIGENDIAN
      if(!wav->be)
        wav->swap_endian = 1;
#else
      if(wav->be)
        wav->swap_endian = 1;
#endif
      wav->format.sample_format = GAVL_SAMPLE_S32;
      break;
    }

  if(wav->swap_endian)
    {
    wav->dsp = gavl_dsp_context_create();
    wav->format.samples_per_frame = 1024;
    wav->frame = gavl_audio_frame_create(&wav->format);
    }
  wav->sink = gavl_audio_sink_create(wav->frame ? get_func_wav : NULL,
                                     write_func_wav, wav,
                                     &wav->format);
  
  return 1;
  }

static int close_wav(void * data, int do_delete)
  {
  int ret = 1;
  wav_t * wav;
  int64_t total_bytes;
  wav = data;

  if(!do_delete)
    {
    if(wav->file_format == FORMAT_WAV)
      {
      total_bytes = gavf_io_position(wav->output);
      /* Finalize data section */
      gavf_io_seek(wav->output, wav->data_size_offset, SEEK_SET);
      write_32(wav->output, total_bytes - wav->data_size_offset - 4);
      /* Finalize RIFF */
      gavf_io_seek(wav->output, RIFF_SIZE_OFFSET, SEEK_SET);
      write_32(wav->output, total_bytes - RIFF_SIZE_OFFSET - 4);
    
      /* Write info chunk */
      if(wav->write_info_chunk)
        {
        gavf_io_seek(wav->output, total_bytes, SEEK_SET);
        ret = write_info_chunk(wav->output, &wav->metadata);
        total_bytes = gavf_io_position(wav->output);
        }
      }
    }
  if(wav->output)
    gavf_io_destroy(wav->output);
  
  if(do_delete && wav->filename)
    remove(wav->filename);
  
  gavl_metadata_free(&wav->metadata);
  
  if(wav->sink)
    {
    gavl_audio_sink_destroy(wav->sink);
    wav->sink = NULL;
    }

  if(wav->dsp)
    {
    gavl_dsp_context_destroy(wav->dsp);
    wav->dsp = NULL;
    }

  if(wav->frame)
    {
    gavl_audio_frame_destroy(wav->frame);
    wav->frame = NULL;
    }
  
  wav->output = NULL;
  return ret;
  }

static gavl_audio_sink_t * get_audio_sink_wav(void * data, int stream)
  {
  wav_t * wav = data;
  return wav->sink;
  }

const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =              "e_wav", /* Unique short name */
      .long_name =         TRS("Wave writer"),
      .description =       TRS("Simple writer for wave files, supports 8, 16, 24 and 32 bit PCM"),
      .type =              BG_PLUGIN_ENCODER_AUDIO,
      .flags =             BG_PLUGIN_FILE | BG_PLUGIN_PIPE | BG_PLUGIN_GAVF_IO,
      .priority =          BG_PLUGIN_PRIORITY_MAX,
      .create =            create_wav,
      .destroy =           destroy_wav,
      .get_parameters =    get_parameters_wav,
      .set_parameter =     set_parameter_wav,
    },
    .max_audio_streams =   1,
    .max_video_streams =   0,
    
    .set_callbacks =       set_callbacks_wav,
    
    .open =                open_wav,
    .open_io =             open_io_wav,
    
    .get_audio_parameters =    get_audio_parameters_wav,

    .add_audio_stream =        add_audio_stream_wav,
    
    .set_audio_parameter =     set_audio_parameter_wav,

    .get_audio_sink =          get_audio_sink_wav,

    .start =               start_wav,
    .close =               close_wav
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
