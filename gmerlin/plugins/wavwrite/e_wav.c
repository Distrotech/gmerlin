/*****************************************************************
 
  e_wav.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/* Simple Wav encoder. Supports only PCM (0x0001), but
   up to 32 bits, multi channel capable and with metadata */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <config.h>
#include <plugin.h>
#include <utils.h>

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

typedef struct
  {
  int bytes_per_sample;
  FILE * output;
  char * error_msg;
  int data_size_offset;
  gavl_audio_format_t format;

  int write_info_chunk;
  bg_metadata_t metadata;
  char * filename;
  uint32_t channel_mask;
  } wav_t;

static void write_8(FILE * output, uint32_t val)
  {
  uint8_t c = val;
  fwrite(&c, 1, 1, output);
  }

static void write_16(FILE * output, uint32_t val)
  {
  uint8_t c[2];
  
  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  
  fwrite(c, 1, 2, output);
  }
#if 0
static void write_24(FILE * output, uint32_t val)
  {
  uint8_t c[3];

  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  c[2] = (val >> 16) & 0xff;
  fwrite(c, 1, 3, output);
  }
#endif
static void write_32(FILE * output, uint32_t val)
  {
  uint8_t c[4];
  
  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  c[2] = (val >> 16) & 0xff;
  c[3] = (val >> 24) & 0xff;
  fwrite(c, 1, 4, output);
  }

/* Functions for writing signed integers */

static void write_16_s(FILE * output, int32_t val)
  {
  uint8_t c[2];
  
  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  
  fwrite(c, 1, 2, output);
  }

static void write_24_s(FILE * output, int32_t val)
  {
  uint8_t c[3];

  c[0] = (val >> 8) & 0xff;
  c[1] = (val >> 16) & 0xff;
  c[2] = (val >> 24) & 0xff;
  fwrite(c, 1, 3, output);
  }

static void write_32_s(FILE * output, int32_t val)
  {
  uint8_t c[4];

  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  c[2] = (val >> 16) & 0xff;
  c[3] = (val >> 24) & 0xff;
  fwrite(c, 1, 4, output);
  }


static void write_fourcc(FILE * output, char c1, char c2, char c3, char c4)
  {
  char c[4];

  c[0] = c1;
  c[1] = c2;
  c[2] = c3;
  c[3] = c4;
  fwrite(c, 1, 4, output);
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
      fwrite(#tag, 1, 4, output);\
      write_32(output, len); \
      fwrite(info.tag, 1, len, output); \
      if(len % 2) write_8(output, 0x00); \
      } \
    }

#define BUFFER_SIZE 256

static void write_info_chunk(FILE * output, bg_metadata_t * m)
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

  write_fourcc(output, 'L', 'I', 'S', 'T');
  write_32(output, total_len);
  write_fourcc(output, 'I', 'N', 'F', 'O');
  
  /* Write strings */

  WRITE_STRING(IART);
  WRITE_STRING(INAM);
  WRITE_STRING(ICMT);
  WRITE_STRING(ICOP);
  WRITE_STRING(IGNR);
  WRITE_STRING(ICRD);
  WRITE_STRING(ISFT);
  
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

static const char * get_error_wav(void * priv)
  {
  wav_t * wav;
  wav = (wav_t*)priv;
  return wav->error_msg;
  }


static bg_parameter_info_t audio_parameters[] =
  {
    {
      name:        "bits",
      long_name:   "Bits per sample",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "16" },
      multi_names:     (char*[]){ "8", "16", "24", "32", (char*)0 },
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "write_info_chunk",
      long_name:   "Write info chunk",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
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

static void write_PCMWAVEFORMAT(wav_t * wav)
  {
  write_32(wav->output, 16);                                               /* Size   */
  write_16(wav->output, 0x0001);                                           /* wFormatTag */
  write_16(wav->output, wav->format.num_channels);                         /* nChannels */
  write_32(wav->output, wav->format.samplerate);                           /* nSamplesPerSec */
  write_32(wav->output, wav->bytes_per_sample *
           wav->format.num_channels * wav->format.samplerate);             /* nAvgBytesPerSec */
  write_16(wav->output, wav->bytes_per_sample * wav->format.num_channels); /* nBlockAlign */
  write_16(wav->output, wav->bytes_per_sample * 8);                        /* wBitsPerSample */
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
  int i, j;
  uint32_t ret = 0;
  gavl_channel_id_t new_locations[GAVL_MAX_CHANNELS];
  int channel_index = 0;
  
  for(j = 0; j < sizeof(channel_flags)/sizeof(channel_flags[0]); j++)
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


static void write_WAVEFORMATEXTENSIBLE(wav_t * wav)
  {
  uint8_t guid[16] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                      0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };

  write_32(wav->output, 40);                                               /* Size   */
  write_16(wav->output, 0xfffe);                                           /* wFormatTag */
  write_16(wav->output, wav->format.num_channels);                         /* nChannels */
  write_32(wav->output, wav->format.samplerate);                           /* nSamplesPerSec */
  write_32(wav->output, wav->bytes_per_sample *
           wav->format.num_channels * wav->format.samplerate);             /* nAvgBytesPerSec */
  write_16(wav->output, wav->bytes_per_sample * wav->format.num_channels); /* nBlockAlign */
  write_16(wav->output, wav->bytes_per_sample * 8);                        /* wBitsPerSample */
  write_16(wav->output, 22);                                               /* cbSize         */
  write_16(wav->output, wav->bytes_per_sample * 8);                        /* wValidBitsPerSample */

  /* Write channel mask */

  write_32(wav->output, wav->channel_mask);            /* dwChannelMask */
  fwrite(guid, 1, 16, wav->output);
  
  }

static void set_audio_parameter_wav(void * data, int stream, char * name,
                                    bg_parameter_value_t * v)
  {
  wav_t * wav;
  wav = (wav_t*)data;
  
  if(stream)
    return;
    
  if(!name)
    return;

  //  fprintf(stderr, "set_audio_parameter_wav %s\n", name);
  
  if(!strcmp(name, "bits"))
    {
    wav->bytes_per_sample = atoi(v->val_str) / 8;
    
    /* Write the header and adjust format */

    write_fourcc(wav->output, 'R', 'I', 'F', 'F'); /* "RIFF" */
    write_32(wav->output, 0x00000000);             /*  size  */
    write_fourcc(wav->output, 'W', 'A', 'V', 'E'); /* "WAVE" */
    write_fourcc(wav->output, 'f', 'm', 't', ' '); /* "fmt " */

    /* Build channel mask and adjust channel locations */
    wav->channel_mask = format_2_channel_mask(&wav->format);
    
    if(wav->format.num_channels <= 2)
      write_PCMWAVEFORMAT(wav);
    else
      write_WAVEFORMATEXTENSIBLE(wav);
    
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
        break;
      case 3:
      case 4:
        wav->format.sample_format = GAVL_SAMPLE_S32;
        break;
      }
    }
  }

static void set_parameter_wav(void * data, char * name, bg_parameter_value_t * v)
  {
  wav_t * wav;
  wav = (wav_t*)data;
  
  if(!name)
    return;
  if(!strcmp(name, "write_info_chunk"))
    wav->write_info_chunk = v->val_i;
  }

static int open_wav(void * data, const char * filename,
                    bg_metadata_t * metadata)
  {
  int result;
  wav_t * wav;
  wav = (wav_t*)data;
  
  wav->filename = bg_strdup(wav->filename, filename);
  wav->output = fopen(wav->filename, "wb");

  if(!wav->output)
    {
    wav->error_msg = bg_sprintf("Cannot open output file: %s\n",
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

static void add_audio_stream_wav(void * data, gavl_audio_format_t * format)
  {
  wav_t * wav;
  
  wav = (wav_t*)data;

  gavl_audio_format_copy(&(wav->format), format);

  // fprintf(stderr, "add_audio_stream_wav\n");
  // gavl_audio_format_dump(&(wav->format));
  
  wav->format.interleave_mode = GAVL_INTERLEAVE_ALL;
  return;
  }

static void write_audio_frame_wav(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  int i, imax;
  wav_t * wav;
  
  wav = (wav_t*)data;

  imax = frame->valid_samples * wav->format.num_channels;
  
  switch(wav->bytes_per_sample)
    {
    case 1:
      fwrite(frame->samples.s_8, 1, imax, wav->output);
      break;
    case 2:
      for(i = 0; i < imax; i++)
        write_16_s(wav->output, frame->samples.s_16[i]);
      
      break;
    case 3:
      for(i = 0; i < imax; i++)
        write_24_s(wav->output, frame->samples.s_32[i]);
      break;
    case 4:
      for(i = 0; i < imax; i++)
        write_32_s(wav->output, frame->samples.s_32[i]);
      break;
    }
  }

static void get_audio_format_wav(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  wav_t * wav;
  wav = (wav_t*)data;
  gavl_audio_format_copy(ret, &(wav->format));
  }


static void close_wav(void * data, int do_delete)
  {
  wav_t * wav;
  int64_t total_bytes;
  wav = (wav_t*)data;

  if(!do_delete)
    {
    total_bytes = ftell(wav->output);
    /* Finalize data section */
    fseek(wav->output, wav->data_size_offset, SEEK_SET);
    write_32(wav->output, total_bytes - wav->data_size_offset - 4);
    /* Write info chunk */
    if(wav->write_info_chunk)
      {
      fseek(wav->output, total_bytes, SEEK_SET);
      write_info_chunk(wav->output, &(wav->metadata));
      total_bytes = ftell(wav->output);
      }
    /* Finalize RIFF */
    fseek(wav->output, RIFF_SIZE_OFFSET, SEEK_SET);
    write_32(wav->output, total_bytes - RIFF_SIZE_OFFSET - 4);
    
    }
  
  fclose(wav->output);

  if(do_delete)
    remove(wav->filename);

  bg_metadata_free(&(wav->metadata));
  
  wav->output = NULL;
  }

bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      name:              "e_wav", /* Unique short name */
      long_name:         "Simple wave writer",
      mimetypes:         NULL,
      extensions:        "wav",
      type:              BG_PLUGIN_ENCODER_AUDIO,
      flags:             BG_PLUGIN_FILE,
      priority:          BG_PLUGIN_PRIORITY_MAX,
      create:            create_wav,
      destroy:           destroy_wav,
      get_error:         get_error_wav,
      get_parameters:    get_parameters_wav,
      set_parameter:     set_parameter_wav,
    },
    max_audio_streams:   1,
    max_video_streams:   0,
    
    get_extension:       get_extension_wav,
    
    open:                open_wav,
    
    get_audio_parameters:    get_audio_parameters_wav,

    add_audio_stream:        add_audio_stream_wav,
    
    set_audio_parameter:     set_audio_parameter_wav,

    get_audio_format:        get_audio_format_wav,
    
    write_audio_frame:   write_audio_frame_wav,
    close:               close_wav
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
