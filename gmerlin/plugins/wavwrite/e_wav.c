/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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
#include <translation.h>


#include <plugin.h>
#include <utils.h>
#include <log.h>
#define LOG_DOMAIN "e_wav"

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

typedef struct wav_s
  {
  int bytes_per_sample;
  FILE * output;
  int data_size_offset;
  gavl_audio_format_t format;

  int write_info_chunk;
  bg_metadata_t metadata;
  char * filename;
  uint32_t channel_mask;

  uint8_t * buffer;
  int buffer_alloc;
  void (*convert_func)(struct wav_s*, uint8_t * samples, int num_samples);
  } wav_t;

static int write_8(FILE * output, uint32_t val)
  {
  uint8_t c = val;
  if(fwrite(&c, 1, 1, output) < 1)
    return 0;
  return 1;
  }

static int write_16(FILE * output, uint32_t val)
  {
  uint8_t c[2];
  
  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  
  if(fwrite(c, 1, 2, output) < 2)
    return 0;
  return 1;
  }

static int write_32(FILE * output, uint32_t val)
  {
  uint8_t c[4];
  
  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  c[2] = (val >> 16) & 0xff;
  c[3] = (val >> 24) & 0xff;
  if(fwrite(c, 1, 4, output) < 4)
    return 0;
  return 1;
  }

static int write_fourcc(FILE * output, char c1, char c2, char c3, char c4)
  {
  char c[4];

  c[0] = c1;
  c[1] = c2;
  c[2] = c3;
  c[3] = c4;
  if(fwrite(c, 1, 4, output) < 4)
    return 0;
  return 1;
  }

/* Functions for writing signed integers */

#ifdef GAVL_PROCESSOR_BIG_ENDIAN
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

static void convert_24(struct wav_s*w, uint8_t * samples, int num_samples)
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
      if(fwrite(#tag, 1, 4, output) < 4)\
        return 0; \
      write_32(output, len); \
      if(fwrite(info.tag, 1, len, output) < len) \
        return 0; \
      if(len % 2) \
        { \
        if(!write_8(output, 0x00)) \
          return 0; \
        } \
      } \
    }

#define BUFFER_SIZE 256

static int write_info_chunk(FILE * output, bg_metadata_t * m)
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

  info.IART = m->artist;
  info.INAM = m->title;
  info.ICMT = m->comment;
  info.ICOP = m->copyright;
  info.IGNR = m->genre;
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

static void destroy_wav(void * priv)
  {
  wav_t * wav;
  wav = (wav_t*)priv;

  if(wav->output)
    fclose(wav->output);

  free(wav);
  }



static bg_parameter_info_t audio_parameters[] =
  {
    {
      .name =        "bits",
      .long_name =   TRS("Bits per sample"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "16" },
      .multi_names =     (char*[]){ "8", "16", "24", "32", (char*)0 },
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t parameters[] =
  {
    {
      .name =        "write_info_chunk",
      .long_name =   TRS("Write info chunk"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_audio_parameters_wav(void * data)
  {
  return audio_parameters;
  }

static bg_parameter_info_t * get_parameters_wav(void * data)
  {
  return parameters;
  }

static int write_PCMWAVEFORMAT(wav_t * wav)
  {
  return
    (write_32(wav->output, 16) &&                                               /* Size   */
     write_16(wav->output, 0x0001) &&                                            /* wFormatTag */
     write_16(wav->output, wav->format.num_channels) &&                          /* nChannels */
     write_32(wav->output, wav->format.samplerate) &&                           /* nSamplesPerSec */
     write_32(wav->output, wav->bytes_per_sample *
              wav->format.num_channels * wav->format.samplerate) &&             /* nAvgBytesPerSec */
     write_16(wav->output, wav->bytes_per_sample * wav->format.num_channels) && /* nBlockAlign */
     write_16(wav->output, wav->bytes_per_sample * 8));                        /* wBitsPerSample */
  }

struct
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

  return(write_32(wav->output, 40) &&                                               /* Size   */
         write_16(wav->output, 0xfffe) &&                                           /* wFormatTag */
         write_16(wav->output, wav->format.num_channels) &&                         /* nChannels */
         write_32(wav->output, wav->format.samplerate) &&                           /* nSamplesPerSec */
         write_32(wav->output, wav->bytes_per_sample *
                  wav->format.num_channels * wav->format.samplerate) &&             /* nAvgBytesPerSec */
         write_16(wav->output, wav->bytes_per_sample * wav->format.num_channels) && /* nBlockAlign */
         write_16(wav->output, wav->bytes_per_sample * 8) &&                        /* wBitsPerSample */
         write_16(wav->output, 22) &&                                               /* cbSize         */
         write_16(wav->output, wav->bytes_per_sample * 8) &&                        /* wValidBitsPerSample */

  /* Write channel mask */

         write_32(wav->output, wav->channel_mask) &&            /* dwChannelMask */
         (fwrite(guid, 1, 16, wav->output) == 16));
    }

static void set_audio_parameter_wav(void * data, int stream,
                                    const char * name,
                                    const bg_parameter_value_t * v)
  {
  wav_t * wav;
  wav = (wav_t*)data;
  
  if(stream)
    return;
    
  if(!name)
    return;

  
  if(!strcmp(name, "bits"))
    {
    wav->bytes_per_sample = atoi(v->val_str) / 8;
    }
  }

static void set_parameter_wav(void * data, const char * name,
                              const bg_parameter_value_t * v)
  {
  wav_t * wav;
  wav = (wav_t*)data;
  
  if(!name)
    return;
  if(!strcmp(name, "write_info_chunk"))
    wav->write_info_chunk = v->val_i;
  }

static int open_wav(void * data, const char * filename,
                    bg_metadata_t * metadata,
                    bg_chapter_list_t * chapter_list)
  {
  int result;
  wav_t * wav;
  wav = (wav_t*)data;
  
  wav->filename = bg_strdup(wav->filename, filename);
  wav->output = fopen(wav->filename, "wb");

  if(!wav->output)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open output file: %s",
           strerror(errno));
    result = 0;
    }
  else
    result = 1;

  bg_metadata_copy(&(wav->metadata), metadata);
  
  return result;
  }

static char * wav_extension = ".wav";

static const char * get_extension_wav(void * data)
  {
  return wav_extension;
  }

static int add_audio_stream_wav(void * data, const char * language, gavl_audio_format_t * format)
  {
  wav_t * wav;
  
  wav = (wav_t*)data;

  gavl_audio_format_copy(&(wav->format), format);

  
  wav->format.interleave_mode = GAVL_INTERLEAVE_ALL;
  return 0;
  }

static int write_audio_frame_wav(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  int num_samples, num_bytes;
  wav_t * wav;
  
  wav = (wav_t*)data;
  
  num_samples = frame->valid_samples * wav->format.num_channels;
  num_bytes = num_samples * wav->bytes_per_sample;
  
  if(wav->convert_func)
    {
    if(wav->buffer_alloc < num_bytes)
      {
      wav->buffer_alloc = num_bytes + 1024;
      wav->buffer = realloc(wav->buffer, wav->buffer_alloc);
      }
    wav->convert_func(wav, frame->samples.u_8, num_samples);
    if(fwrite(wav->buffer, 1, num_bytes, wav->output) < num_bytes)
      return 0;
    }
  else
    {
    if(fwrite(frame->samples.s_8, 1, num_bytes, wav->output) < num_bytes)
      return 0;
    }
  return 1;
  }

static void get_audio_format_wav(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  wav_t * wav;
  wav = (wav_t*)data;
  gavl_audio_format_copy(ret, &(wav->format));
  }

static int start_wav(void * data)
  {
  wav_t * wav;
  wav = (wav_t*)data;

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
  
  wav->data_size_offset = ftell(wav->output);
  write_32(wav->output, 0x00000000);             /*  size  */
  
  /* Adjust format */
  
  switch(wav->bytes_per_sample)
    {
    case 1:
      wav->format.sample_format = GAVL_SAMPLE_U8;
      break;
    case 2:
      wav->format.sample_format = GAVL_SAMPLE_S16;
#ifdef GAVL_PROCESSOR_BIG_ENDIAN
      wav->convert_func = convert_16_be;
#endif
      break;
    case 3:
      wav->convert_func = convert_24;
      wav->format.sample_format = GAVL_SAMPLE_S32;
      break;
    case 4:
#ifdef GAVL_PROCESSOR_BIG_ENDIAN
      wav->convert_func = convert_32_be;
#endif
      wav->format.sample_format = GAVL_SAMPLE_S32;
      break;
    }
  return 1;
  }


static int close_wav(void * data, int do_delete)
  {
  int ret = 1;
  wav_t * wav;
  int64_t total_bytes;
  wav = (wav_t*)data;

  if(!do_delete)
    {
    total_bytes = ftell(wav->output);
    /* Finalize data section */
    fseek(wav->output, wav->data_size_offset, SEEK_SET);
    write_32(wav->output, total_bytes - wav->data_size_offset - 4);
    /* Finalize RIFF */
    fseek(wav->output, RIFF_SIZE_OFFSET, SEEK_SET);
    write_32(wav->output, total_bytes - RIFF_SIZE_OFFSET - 4);

    /* Write info chunk */
    if(wav->write_info_chunk)
      {
      fseek(wav->output, total_bytes, SEEK_SET);
      ret = write_info_chunk(wav->output, &(wav->metadata));
      total_bytes = ftell(wav->output);
      }
    }
  if(wav->output)
    fclose(wav->output);
  
  if(do_delete)
    remove(wav->filename);

  bg_metadata_free(&(wav->metadata));
  
  wav->output = NULL;
  return ret;
  }

bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =              "e_wav", /* Unique short name */
      .long_name =         TRS("Wave writer"),
      .description =       TRS("Simple writer for wave files, supports 8, 16, 24 and 32 bit PCM"),
      .mimetypes =         NULL,
      .extensions =        "wav",
      .type =              BG_PLUGIN_ENCODER_AUDIO,
      .flags =             BG_PLUGIN_FILE,
      .priority =          BG_PLUGIN_PRIORITY_MAX,
      .create =            create_wav,
      .destroy =           destroy_wav,
      .get_parameters =    get_parameters_wav,
      .set_parameter =     set_parameter_wav,
    },
    .max_audio_streams =   1,
    .max_video_streams =   0,
    
    .get_extension =       get_extension_wav,
    
    .open =                open_wav,
    
    .get_audio_parameters =    get_audio_parameters_wav,

    .add_audio_stream =        add_audio_stream_wav,
    
    .set_audio_parameter =     set_audio_parameter_wav,

    .get_audio_format =        get_audio_format_wav,

    .start =               start_wav,
    .write_audio_frame =   write_audio_frame_wav,
    .close =               close_wav
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
