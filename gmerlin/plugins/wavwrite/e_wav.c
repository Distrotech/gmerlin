#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <plugin.h>
#include <utils.h>

#define RIFF_SIZE_OFFSET 4
#define DATA_SIZE_OFFSET 40

typedef struct
  {
  int bytes_per_sample;
  FILE * output;
  char * error_msg;

  gavl_audio_format_t format;
  } wav_t;

#if 0
static void write_8(FILE * output, uint32_t val)
  {
  uint8_t c = val;
  fwrite(&c, 1, 1, output);
  }
#endif

static void write_16(FILE * output, uint32_t val)
  {
  uint8_t c[2];
  
  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  
  fwrite(c, 1, 2, output);
  }

static void write_24(FILE * output, uint32_t val)
  {
  uint8_t c[3];

  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  c[2] = (val >> 16) & 0xff;
  fwrite(c, 1, 3, output);
  }

static void write_32(FILE * output, uint32_t val)
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


static bg_parameter_info_t parameters[] =
  {
    {
      name:        "bits",
      long_name:   "Bits per sample",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "16" },
      multi_names:     (char*[]){ "8", "16", "24", (char*)0 },
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_audio_parameters_wav(void * data)
  {
  return parameters;
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

  if(!strcmp(name, "bits"))
    {
    wav->bytes_per_sample = atoi(v->val_str) / 8;
    }
  }

static int open_wav(void * data, const char * filename_base,
                    bg_metadata_t * metadata)
  {
  char * filename;
  int result;
  wav_t * wav;
  wav = (wav_t*)data;

  filename = bg_sprintf("%s.wav");
  wav->output = fopen(filename, "wb");

  if(!wav->output)
    {
    wav->error_msg = bg_sprintf("Cannot open output file: %s\n",
                               strerror(errno));
    result = 0;
    }
  else
    result = 1;
  
  free(filename);
  return result;
  }


static void add_audio_stream_wav(void * data, bg_audio_info_t * audio_info)
  {
  wav_t * wav;
  
  wav = (wav_t*)data;

  gavl_audio_format_copy(&(wav->format), &(audio_info->format));
  
  /* Write the header and adjust format */

  write_fourcc(wav->output, 'R', 'I', 'F', 'F'); /* "RIFF" */
  write_32(wav->output, 0x00000000);             /*  size  */
  write_fourcc(wav->output, 'W', 'A', 'V', 'E'); /* "WAVE" */
  write_fourcc(wav->output, 'f', 'm', 't', ' '); /* "fmt " */
  write_16(wav->output, 16);                     /* Size   */
  write_16(wav->output, 0x0001);                 /* WAV ID */

  write_16(wav->output, wav->format.num_channels);   /* nch        */
  write_32(wav->output, wav->format.samplerate);     /* Samplerate */
    
  /* Bytes per second */
  write_32(wav->output, wav->bytes_per_sample * wav->format.num_channels * wav->format.samplerate);

  /* Block align */
  write_16(wav->output, wav->bytes_per_sample * wav->format.num_channels);

  /* Bits */

  write_16(wav->output, wav->bytes_per_sample * 8);

  /* Start data section */

  write_fourcc(wav->output, 'd', 'a', 't', 'a'); /* "data" */
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
      wav->format.sample_format = GAVL_SAMPLE_S32;
      break;
    }
  
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
        write_16(wav->output, frame->samples.s_16[i]);
      
      break;
    case 3:
      for(i = 0; i < imax; i++)
        write_24(wav->output, frame->samples.s_32[i]);
      break;
    }
  }

static void get_audio_format_wav(void * data, int stream, gavl_audio_format_t * ret)
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
  total_bytes = ftell(wav->output);
  
  fseek(wav->output, RIFF_SIZE_OFFSET, SEEK_SET);
  write_32(wav->output, total_bytes - RIFF_SIZE_OFFSET - 4);

  fseek(wav->output, DATA_SIZE_OFFSET, SEEK_SET);
  write_32(wav->output, total_bytes - DATA_SIZE_OFFSET - 4);

  fclose(wav->output);
  wav->output = NULL;
  }

bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      name:            "e_wav",       /* Unique short name */
      long_name:       "Simple wave writer",
      mimetypes:       NULL,
      extensions:      "wav",
      type:            BG_PLUGIN_ENCODER_AUDIO,
      flags:           BG_PLUGIN_FILE,
      
      create:            create_wav,
      destroy:           destroy_wav,
      get_error:         get_error_wav

    },
    max_audio_streams:   1,
    max_video_streams:   0,
    
    open:                open_wav,
    
    get_audio_parameters:    get_audio_parameters_wav,

    add_audio_stream:        add_audio_stream_wav,
    
    set_audio_parameter:     set_audio_parameter_wav,

    get_audio_format:        get_audio_format_wav,
    
    write_audio_frame:   write_audio_frame_wav,
    close:               close_wav
  };
