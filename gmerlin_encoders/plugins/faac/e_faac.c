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

#include <faac.h>

#include <config.h>

#include <gmerlin_encoders.h>

#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "e_faac"

#include "faac_codec.h"

#include <gmerlin/translation.h>

typedef struct
  {
  FILE * output;
  char * filename;
  
  gavl_audio_format_t format;
  
  bgen_id3v1_t * id3v1;
  int do_id3v1;
  int do_id3v2;
  int id3v2_charset;
  bg_encoder_callbacks_t * cb;
  gavl_audio_sink_t * sink;
  gavl_packet_sink_t * psink;
  
  bg_faac_t * codec;
  } faac_t;

static void * create_faac()
  {
  faac_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->codec = bg_faac_create();
  return ret;
  }

static void destroy_faac(void * priv)
  {
  faac_t * faac;
  faac = priv;
  if(faac->codec)
    bg_faac_destroy(faac->codec);

  
  free(faac);
  }

static void set_callbacks_faac(void * data, bg_encoder_callbacks_t * cb)
  {
  faac_t * faac = data;
  faac->cb = cb;
  }

static const bg_parameter_info_t * get_audio_parameters_faac(void * data)
  {
  faac_t * faac = data;
  return bg_faac_get_parameters(faac->codec);
  }

static void set_audio_parameter_faac(void * data, int stream, const char * name,
                                     const bg_parameter_value_t * v)
  {
  faac_t * faac;
  faac = data;
  if(stream)
    return;

  bg_faac_set_parameter(faac->codec, name, v);
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

static gavl_sink_status_t write_packet(void * data, gavl_packet_t * p)
  {
  faac_t * faac = data;
  if(fwrite(p->data, 1, p->data_len, faac->output) < p->data_len)
    return GAVL_SINK_ERROR;
  return GAVL_SINK_OK;
  }

static int write_audio_frame_faac(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  faac_t * faac = data;
  return (gavl_audio_sink_put_frame(faac->sink, frame) == GAVL_SINK_OK);
  }

static int add_audio_stream_faac(void * data,
                                 const gavl_metadata_t * m,
                                 const gavl_audio_format_t * format)
  {
  faac_t * faac = data;
  gavl_audio_format_copy(&faac->format, format);
  return 0;
  }

static int start_faac(void * data)
  {
  faac_t * faac = data;
  faac->sink = bg_faac_open(faac->codec,
                            NULL,
                            &faac->format,
                            NULL);
  if(!faac->sink)
    return 0;

  faac->psink = gavl_packet_sink_create(NULL, write_packet, faac);
  bg_faac_set_packet_sink(faac->codec, faac->psink);
  return 1;
  }

static void get_audio_format_faac(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  faac_t * faac = data;
  gavl_audio_format_copy(ret, &faac->format);
  }

static gavl_audio_sink_t * get_audio_sink_faac(void * data, int stream)
  {
  faac_t * faac = data;
  return faac->sink;
  }


static int close_faac(void * data, int do_delete)
  {
  int ret = 1;
  faac_t * faac;
  faac = data;
  
  /* Destroy codec, will also flush samples */
  
  bg_faac_destroy(faac->codec);
  faac->codec = NULL;
  
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

  if(faac->psink)
    {
    gavl_packet_sink_destroy(faac->psink);
    faac->psink = NULL;
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
    .start               =     start_faac,

    .get_audio_format =        get_audio_format_faac,
    .get_audio_sink =        get_audio_sink_faac,
    
    .write_audio_frame =   write_audio_frame_faac,
    .close =               close_faac
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
