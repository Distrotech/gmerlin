/*****************************************************************
 
  e_faac.c
 
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

#include <faac.h>

#include <gmerlin_encoders.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

typedef struct
  {
  char * error_msg;
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
  faac = (faac_t*)priv;
  
  free(faac);
  }

static const char * get_error_faac(void * priv)
  {
  faac_t * faac;
  faac = (faac_t*)priv;
  return faac->error_msg;
  }


static bg_parameter_info_t audio_parameters[] =
  {
    {
      name:        "basic",
      long_name:   "Basic options",
      type:        BG_PARAMETER_SECTION
    },
    {
      name:        "object_type",
      long_name:   "Object type",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str:  "MPEG-4 Main profile" },
      multi_names: (char*[]){ "MPEG-2 Main profile",
                              "MPEG-2 Low Complexity profile (LC)",
                              "MPEG-4 Main profile",
                              "MPEG-4 Low Complexity profile (LC)",
                              "MPEG-4 Long Term Prediction (LTP)",
                              (char*)0 },
    },
    {
      name:        "bitrate",
      long_name:   "Bitrate (kbps)",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 0    },
      val_max:     { val_i: 1000 },
      help_string: "Average bitrate (0: VBR based on quality)",
    },
    {
      name:        "quality",
      long_name:   "Quality",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 10 },
      val_max:     { val_i: 500 },
      val_default: { val_i: 100 },
      help_string: "Quantizer quality",
    },
    {
      name:        "advanced",
      long_name:   "Advanced options",
      type:        BG_PARAMETER_SECTION
    },
    {
      name:        "block_types",
      long_name:   "Block types",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str:  "Both" },
      multi_names: (char*[]){ "Both",
                              "No short",
                              "No long",
                              (char*)0 },
    },
    {
      name:        "tns",
      type:        BG_PARAMETER_CHECKBUTTON,
      long_name:   "Use temporal noise shaping",
      val_default: { val_i: 0 }
    },
    {
      name:        "no_midside",
      long_name:   "Don\'t use mid/side coding",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 }
    },
    { /* End of parameters */ }
  };
static bg_parameter_info_t * get_audio_parameters_faac(void * data)
  {
  return audio_parameters;
  }

static void set_audio_parameter_faac(void * data, int stream, char * name,
                                       bg_parameter_value_t * v)
  {
  faac_t * faac;
  faac = (faac_t*)data;
  
  if(stream)
    return;
    
  else if(!name)
    {
    /* Set encoding parameters */
    
    if(!faacEncSetConfiguration(faac->enc, faac->enc_config))
      fprintf(stderr, "ERROR: faacEncSetConfiguration failed\n");
    }
    
  else if(!strcmp(name, "object_type"))
    {
    if(!strcmp(v->val_str, "MPEG-2 Main profile"))
      {
      faac->enc_config->mpegVersion = MPEG2;
      faac->enc_config->aacObjectType = MAIN;
      }
    else if(!strcmp(v->val_str, "MPEG-2 Low Complexity profile (LC)"))
      {
      faac->enc_config->mpegVersion = MPEG2;
      faac->enc_config->aacObjectType = LOW;
      }
    else if(!strcmp(v->val_str, "MPEG-4 Main profile"))
      {
      faac->enc_config->mpegVersion = MPEG4;
      faac->enc_config->aacObjectType = MAIN;
      }
    else if(!strcmp(v->val_str, "MPEG-4 Low Complexity profile (LC)"))
      {
      faac->enc_config->mpegVersion = MPEG4;
      faac->enc_config->aacObjectType = LOW;
      }
    else if(!strcmp(v->val_str, "MPEG-4 Long Term Prediction (LTP)"))
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
#if 0
  else
    {
    fprintf(stderr, "set_audio_parameter_faac %s\n", name);
    }
#endif
  }

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "do_id3v1",
      long_name:   "Write ID3V1.1 tag",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    {
      name:        "do_id3v2",
      long_name:   "Write ID3V2 tag",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_faac(void * data)
  {
  return parameters;
  }


static void set_parameter_faac(void * data, char * name,
                               bg_parameter_value_t * v)
  {
  faac_t * faac;
  faac = (faac_t*)data;
  
  if(!name)
    return;
  else if(!strcmp(name, "do_id3v1"))
    faac->do_id3v1 = v->val_i;
  else if(!strcmp(name, "do_id3v2"))
    faac->do_id3v2 = v->val_i;
  }

static int open_faac(void * data, const char * filename,
                    bg_metadata_t * metadata)
  {
  faac_t * faac;
  bgen_id3v2_t * id3v2;

  faac = (faac_t*)data;
  
  faac->output = fopen(filename, "wb");
  if(!faac->output)
    return 0;
  faac->filename = bg_strdup(faac->filename, filename);

  if(faac->do_id3v1)
    faac->id3v1 = bgen_id3v1_create(metadata);
  if(faac->do_id3v2)
    {
    id3v2 = bgen_id3v2_create(metadata);
    bgen_id3v2_write(faac->output, id3v2);
    bgen_id3v2_destroy(id3v2);
    }
  return 1;
  }

static char * faac_extension = ".aac";

static const char * get_extension_faac(void * data)
  {
  return faac_extension;
  }

static void add_audio_stream_faac(void * data, gavl_audio_format_t * format)
  {
  unsigned long input_samples;
  unsigned long output_bytes;
  
  faac_t * faac;
  
  faac = (faac_t*)data;

  /* Create encoder handle and get configuration */

  faac->enc = faacEncOpen(format->samplerate,
                          format->num_channels,
                          &input_samples,
                          &output_bytes);
  
  faac->enc_config = faacEncGetCurrentConfiguration(faac->enc);
  faac->enc_config->inputFormat = FAAC_INPUT_FLOAT;
  
  /* Copy and adjust format */

  gavl_audio_format_copy(&(faac->format), format);

  faac->format.interleave_mode = GAVL_INTERLEAVE_ALL;
  faac->format.sample_format   = GAVL_SAMPLE_FLOAT;
  faac->format.samples_per_frame = input_samples / format->num_channels;

  faac->frame = gavl_audio_frame_create(&(faac->format));
  
  faac->output_buffer = malloc(output_bytes);
  faac->output_buffer_size = output_bytes;
  
  return;
  }

static int flush_audio(faac_t * faac)
  {
  int i, imax;
  int bytes_encoded;

  /* First, we must scale the samples to -32767 .. 32767 */

  //  fprintf(stderr, "FLUSH %d\n", faac->frame->valid_samples);
  
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
    fwrite(faac->output_buffer, 1, bytes_encoded, faac->output);

  return bytes_encoded;
  }

static void write_audio_frame_faac(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  int samples_done = 0;
  int samples_copied;
    
  faac_t * faac;
  faac = (faac_t*)data;

  
  while(samples_done < frame->valid_samples)
    {
    //    fprintf(stderr, "ENCODE %d %d\n", frame->valid_samples, samples_done);

    /* Copy frame into our buffer */
    
    samples_copied =
      gavl_audio_frame_copy(&(faac->format),
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
      flush_audio(faac);
    }
  
  }

static void get_audio_format_faac(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  faac_t * faac;
  faac = (faac_t*)data;
  gavl_audio_format_copy(ret, &(faac->format));
  }


static void close_faac(void * data, int do_delete)
  {
  faac_t * faac;
  faac = (faac_t*)data;

  /* Flush remaining audio data */

  while(flush_audio(faac))
    ;

  /* Write id3v1 tag */
  if(faac->id3v1)
    {
    bgen_id3v1_write(faac->output, faac->id3v1);
    bgen_id3v1_destroy(faac->id3v1);
    faac->id3v1 = (bgen_id3v1_t*)0;    
    }
  
  fclose(faac->output);

  if(do_delete)
    remove(faac->filename);
  }

bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      name:            "e_faac",       /* Unique short name */
      long_name:       "Faac encoder",
      mimetypes:       NULL,
      extensions:      "ogg",
      type:            BG_PLUGIN_ENCODER_AUDIO,
      flags:           BG_PLUGIN_FILE,
      
      create:            create_faac,
      destroy:           destroy_faac,
      get_error:         get_error_faac,
      get_parameters:    get_parameters_faac,
      set_parameter:     set_parameter_faac,
    },
    max_audio_streams:   1,
    max_video_streams:   0,
    
    get_extension:       get_extension_faac,
    
    open:                open_faac,
    
    get_audio_parameters:    get_audio_parameters_faac,

    add_audio_stream:        add_audio_stream_faac,
    
    set_audio_parameter:     set_audio_parameter_faac,

    get_audio_format:        get_audio_format_faac,
    
    write_audio_frame:   write_audio_frame_faac,
    close:               close_faac
  };
