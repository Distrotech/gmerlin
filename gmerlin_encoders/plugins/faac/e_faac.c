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

#include <faac.h>

#include <config.h>

#include <gmerlin_encoders.h>

#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "e_faac"

#include <gmerlin/translation.h>

typedef struct
  {
  FILE * output;
  char * filename;
  
  faacEncHandle enc;
  faacEncConfigurationPtr enc_config;
  
  gavl_audio_format_t format;

  gavl_audio_frame_t * frame;
  uint8_t            * output_buffer;
  unsigned int output_buffer_size;
    
  bgen_id3v1_t * id3v1;
  
  int do_id3v1;
  int do_id3v2;
  int id3v2_charset;

  int64_t samples_read;
  bg_encoder_callbacks_t * cb;
  
  } faac_t;

static void * create_faac()
  {
  faac_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_faac(void * priv)
  {
  faac_t * faac;
  faac = priv;
  
  free(faac);
  }

static void set_callbacks_faac(void * data, bg_encoder_callbacks_t * cb)
  {
  faac_t * faac = data;
  faac->cb = cb;
  }

static const bg_parameter_info_t audio_parameters[] =
  {
    {
      .name =        "basic",
      .long_name =   TRS("Basic options"),
      .type =        BG_PARAMETER_SECTION
    },
    {
      .name =        "object_type",
      .long_name =   TRS("Object type"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str =  "mpeg4_main" },
      .multi_names = (char const *[]){ "mpeg2_main",
                              "mpeg2_lc",
                              "mpeg4_main",
                              "mpeg4_lc",
                              "mpeg4_ltp",
                              NULL },
      .multi_labels = (char const *[]){ TRS("MPEG-2 Main profile"),
                               TRS("MPEG-2 Low Complexity profile (LC)"),
                               TRS("MPEG-4 Main profile"),
                               TRS("MPEG-4 Low Complexity profile (LC)"),
                               TRS("MPEG-4 Long Term Prediction (LTP)"),
                               NULL },
    },
    {
      .name =        "bitrate",
      .long_name =   TRS("Bitrate (kbps)"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0    },
      .val_max =     { .val_i = 1000 },
      .help_string = TRS("Average bitrate (0: VBR based on quality)"),
    },
    {
      .name =        "quality",
      .long_name =   TRS("Quality"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 10 },
      .val_max =     { .val_i = 500 },
      .val_default = { .val_i = 100 },
      .help_string = TRS("Quantizer quality"),
    },
    {
      .name =        "advanced",
      .long_name =   TRS("Advanced options"),
      .type =        BG_PARAMETER_SECTION
    },
    {
      .name =        "block_types",
      .long_name =   TRS("Block types"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str =  "Both" },
      .multi_names = (char const *[]){ "Both",
                              "No short",
                              "No long",
                              NULL },
      .multi_labels = (char const *[]){ TRS("Both"),
                               TRS("No short"),
                               TRS("No long"),
                               NULL },
    },
    {
      .name =        "tns",
      .type =        BG_PARAMETER_CHECKBUTTON,
      .long_name =   TRS("Use temporal noise shaping"),
      .val_default = { .val_i = 0 }
    },
    {
      .name =        "no_midside",
      .long_name =   TRS("Don\'t use mid/side coding"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 }
    },
    { /* End of parameters */ }
  };
static const bg_parameter_info_t * get_audio_parameters_faac(void * data)
  {
  return audio_parameters;
  }

static void set_audio_parameter_faac(void * data, int stream, const char * name,
                                     const bg_parameter_value_t * v)
  {
  faac_t * faac;
  faac = data;
  
  if(stream)
    return;
    
  else if(!name)
    {
    /* Set encoding parameters */
    
    if(!faacEncSetConfiguration(faac->enc, faac->enc_config))
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "faacEncSetConfiguration failed");
    }
    
  else if(!strcmp(name, "object_type"))
    {
    if(!strcmp(v->val_str, "mpeg2_main"))
      {
      faac->enc_config->mpegVersion = MPEG2;
      faac->enc_config->aacObjectType = MAIN;
      }
    else if(!strcmp(v->val_str, "mpeg2_lc"))
      {
      faac->enc_config->mpegVersion = MPEG2;
      faac->enc_config->aacObjectType = LOW;
      }
    else if(!strcmp(v->val_str, "mpeg4_main"))
      {
      faac->enc_config->mpegVersion = MPEG4;
      faac->enc_config->aacObjectType = MAIN;
      }
    else if(!strcmp(v->val_str, "mpeg4_lc"))
      {
      faac->enc_config->mpegVersion = MPEG4;
      faac->enc_config->aacObjectType = LOW;
      }
    else if(!strcmp(v->val_str, "mpeg4_ltp"))
      {
      faac->enc_config->mpegVersion = MPEG4;
      faac->enc_config->aacObjectType = LTP;
      }
    }
  else if(!strcmp(name, "bitrate"))
    {
    faac->enc_config->bitRate =
      (v->val_i * 1000) / faac->format.num_channels;
    }
  else if(!strcmp(name, "quality"))
    {
    faac->enc_config->quantqual = v->val_i;
    }
  else if(!strcmp(name, "block_types"))
    {
    if(!strcmp(v->val_str, "Both"))
      {
      faac->enc_config->shortctl = SHORTCTL_NORMAL;
      }
    else if(!strcmp(v->val_str, "No short"))
      {
      faac->enc_config->shortctl = SHORTCTL_NOSHORT;
      }
    else if(!strcmp(v->val_str, "No long"))
      {
      faac->enc_config->shortctl = SHORTCTL_NOLONG;
      }
    }
  else if(!strcmp(name, "tns"))
    {
    faac->enc_config->useTns = v->val_i;
    }
  else if(!strcmp(name, "no_midside"))
    {
    faac->enc_config->allowMidside = !v->val_i;
    }
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "do_id3v1",
      .long_name =   TRS("Write ID3V1.1 tag"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      .name =        "do_id3v2",
      .long_name =   TRS("Write ID3V2 tag"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      .name =        "id3v2_charset",
      .long_name =   TRS("ID3V2 Encoding"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "3" },
      .multi_names = (char const *[]){ "0", "1",
                               "2", "3", NULL },
      .multi_labels = (char const *[]){ TRS("ISO-8859-1"), TRS("UTF-16 LE"),
                               TRS("UTF-16 BE"), TRS("UTF-8"), NULL },
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_faac(void * data)
  {
  return parameters;
  }


static void set_parameter_faac(void * data, const char * name,
                               const bg_parameter_value_t * v)
  {
  faac_t * faac;
  faac = data;
  
  if(!name)
    return;
  else if(!strcmp(name, "do_id3v1"))
    faac->do_id3v1 = v->val_i;
  else if(!strcmp(name, "do_id3v2"))
    faac->do_id3v2 = v->val_i;
  else if(!strcmp(name, "id3v2_charset"))
    faac->id3v2_charset = atoi(v->val_str);
  }

static int open_faac(void * data, const char * filename,
                     const gavl_metadata_t * metadata,
                     const gavl_chapter_list_t * chapter_list)
  {
  faac_t * faac;
  bgen_id3v2_t * id3v2;

  faac = data;

  faac->filename = bg_filename_ensure_extension(filename, "aac");

  if(!bg_encoder_cb_create_output_file(faac->cb, faac->filename))
    return 0;
  
  faac->output = fopen(faac->filename, "wb");

  if(!faac->output)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s: %s",
           filename, strerror(errno));
    return 0;
    }
  

  if(faac->do_id3v1 && metadata)
    faac->id3v1 = bgen_id3v1_create(metadata);
  if(faac->do_id3v2 && metadata)
    {
    id3v2 = bgen_id3v2_create(metadata);
    bgen_id3v2_write(faac->output, id3v2, faac->id3v2_charset);
    bgen_id3v2_destroy(id3v2);
    }
  return 1;
  }

static int add_audio_stream_faac(void * data,
                                 const gavl_metadata_t * m,
                                 const gavl_audio_format_t * format)
  {
  unsigned long input_samples;
  unsigned long output_bytes;
  
  faac_t * faac;
  
  faac = data;

  /* Create encoder handle and get configuration */

  faac->enc = faacEncOpen(format->samplerate,
                          format->num_channels,
                          &input_samples,
                          &output_bytes);
  
  faac->enc_config = faacEncGetCurrentConfiguration(faac->enc);
  faac->enc_config->inputFormat = FAAC_INPUT_FLOAT;
  
  /* Copy and adjust format */

  gavl_audio_format_copy(&faac->format, format);

  faac->format.interleave_mode = GAVL_INTERLEAVE_ALL;
  faac->format.sample_format   = GAVL_SAMPLE_FLOAT;
  faac->format.samples_per_frame = input_samples / format->num_channels;

  switch(faac->format.num_channels)
    {
    case 1:
      faac->format.channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      break;
    case 2:
      faac->format.channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      faac->format.channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      break;
    case 3:
      faac->format.channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      faac->format.channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      faac->format.channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      break;
    case 4:
      faac->format.channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      faac->format.channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      faac->format.channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      faac->format.channel_locations[3] = GAVL_CHID_REAR_CENTER;
      break;
    case 5:
      faac->format.channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      faac->format.channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      faac->format.channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      faac->format.channel_locations[3] = GAVL_CHID_REAR_LEFT;
      faac->format.channel_locations[4] = GAVL_CHID_REAR_RIGHT;
      break;
    case 6:
      faac->format.channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      faac->format.channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      faac->format.channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      faac->format.channel_locations[3] = GAVL_CHID_REAR_LEFT;
      faac->format.channel_locations[4] = GAVL_CHID_REAR_RIGHT;
      faac->format.channel_locations[5] = GAVL_CHID_LFE;
      break;

    }

  
  faac->frame = gavl_audio_frame_create(&faac->format);
  
  faac->output_buffer = malloc(output_bytes);
  faac->output_buffer_size = output_bytes;
  
  return 0;
  }

static int flush_audio(faac_t * faac)
  {
  int i, imax;
  int bytes_encoded;

  /* First, we must scale the samples to -32767 .. 32767 */

  imax = faac->frame->valid_samples * faac->format.num_channels;
  
  for(i = 0; i < imax; i++)
    faac->frame->samples.f[i] *= 32767.0;
  
  /* Encode the stuff */

  bytes_encoded = faacEncEncode(faac->enc, (int32_t*)faac->frame->samples.f,
                                faac->frame->valid_samples * faac->format.num_channels,
                                faac->output_buffer, faac->output_buffer_size);
  faac->frame->valid_samples = 0;

  /* Write this to the file */

  if(bytes_encoded)
    {
    if(fwrite(faac->output_buffer, 1, bytes_encoded, faac->output) < bytes_encoded)
      return -1;
    }
  return bytes_encoded;
  }

static int write_audio_frame_faac(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  int samples_done = 0;
  int samples_copied;
    
  faac_t * faac;
  faac = data;

  
  while(samples_done < frame->valid_samples)
    {

    /* Copy frame into our buffer */
    
    samples_copied =
      gavl_audio_frame_copy(&faac->format,
                            faac->frame,                                                 /* dst */
                            frame,                                                       /* src */
                            faac->frame->valid_samples,                                  /* dst_pos */
                            samples_done,                                                /* src_pos */
                            faac->format.samples_per_frame - faac->frame->valid_samples, /* dst_size */
                            frame->valid_samples - samples_done);                        /* src_size */
    
    samples_done += samples_copied;
    faac->frame->valid_samples += samples_copied;
    
    /* Encode buffer */

    if(faac->frame->valid_samples == faac->format.samples_per_frame)
      {
      if(flush_audio(faac) < 0)
        return 0;
      }
    }
  
  faac->samples_read += frame->valid_samples;
  return 1;
  }

static void get_audio_format_faac(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  faac_t * faac;
  faac = data;
  gavl_audio_format_copy(ret, &faac->format);
  }


static int close_faac(void * data, int do_delete)
  {
  int ret = 1, result;
  faac_t * faac;
  faac = data;

  /* Flush remaining audio data */

  if(faac->samples_read)
    {
    while(1)
      {
      result = flush_audio(faac);
      if(result > 0)
        continue;
      else if(result < 0)
        ret = 0;
      break;
      }
    }
  
  if(faac->enc)
    {
    faacEncClose(faac->enc);
    faac->enc = NULL;
    }
  
  if(faac->output)
    {
    /* Write id3v1 tag */
    if(faac->id3v1)
      {
      if(ret)
        ret = bgen_id3v1_write(faac->output, faac->id3v1);
      bgen_id3v1_destroy(faac->id3v1);
      faac->id3v1 = NULL;    
      }
    fclose(faac->output);
    faac->output = NULL;
    }
  
  if(faac->filename)
    {
    if(do_delete)
      remove(faac->filename);
    free(faac->filename);
    faac->filename = NULL;
    }
  return ret;
  }
  
const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            "e_faac",       /* Unique short name */
      .long_name =       TRS("Faac encoder"),
      .description =     TRS("Plugin for encoding AAC streams (with ADTS headers). Based on faac (http://faac.sourceforge.net)."),
      .type =            BG_PLUGIN_ENCODER_AUDIO,
      .flags =           BG_PLUGIN_FILE,
      .priority =        5,
      .create =            create_faac,
      .destroy =           destroy_faac,
      .get_parameters =    get_parameters_faac,
      .set_parameter =     set_parameter_faac,
    },
    .max_audio_streams =   1,
    .max_video_streams =   0,
    
    .set_callbacks =       set_callbacks_faac,
    
    .open =                open_faac,
    
    .get_audio_parameters =    get_audio_parameters_faac,

    .add_audio_stream =        add_audio_stream_faac,
    
    .set_audio_parameter =     set_audio_parameter_faac,

    .get_audio_format =        get_audio_format_faac,
    
    .write_audio_frame =   write_audio_frame_faac,
    .close =               close_faac
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
