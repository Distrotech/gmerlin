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

#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <errno.h>

#include <gmerlin_encoders.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/translation.h>

#define USE_VBR
#include "bglame.h"

#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "e_lame"

#include <xing.h>

typedef struct
  {
  bg_lame_t * codec;

  //  lame_common_t com; // Must be first!!
  
  char * filename;
  
  FILE * output;

  int do_id3v1;
  int do_id3v2;
  int id3v2_charset;
  
  bgen_id3v1_t * id3v1;
  
  bg_encoder_callbacks_t * cb;

  gavl_compression_info_t ci;
  gavl_packet_sink_t * psink;
  gavl_audio_sink_t * asink;
  
  bg_xing_t * xing;
  uint32_t xing_pos;

  int compressed;
  gavl_audio_format_t fmt;
  } lame_priv_t;

static void * create_lame()
  {
  lame_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->codec = bg_lame_create();
  
  return ret;
  }

static void destroy_lame(void * priv)
  {
  lame_priv_t * lame;
  lame = priv;
  if(lame->codec)
    bg_lame_destroy(lame->codec);
  free(lame);
  }

static void set_callbacks_lame(void * data, bg_encoder_callbacks_t * cb)
  {
  lame_priv_t * lame = data;
  lame->cb = cb;
  }


static const bg_parameter_info_t * get_audio_parameters_lame(void * data)
  {
  return audio_parameters;
  }

static gavl_audio_sink_t * get_audio_sink_lame(void * data, int stream)
  {
  lame_priv_t * lame = data;
  return lame->asink;
  }

/* Global parameters */

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

static const bg_parameter_info_t * get_parameters_lame(void * data)
  {
  return parameters;
  }

static void set_parameter_lame(void * data, const char * name,
                               const bg_parameter_value_t * v)
  {
  lame_priv_t * lame;
  lame = data;
  
  if(!name)
    return;
  else if(!strcmp(name, "do_id3v1"))
    lame->do_id3v1 = v->val_i;
  else if(!strcmp(name, "do_id3v2"))
    lame->do_id3v2 = v->val_i;
  else if(!strcmp(name, "id3v2_charset"))
    lame->id3v2_charset = atoi(v->val_str);
  }

static int open_lame(void * data,
                     const char * filename,
                     const gavl_metadata_t * metadata,
                     const gavl_chapter_list_t * chapter_list)
  {
  int ret = 1;
  lame_priv_t * lame;
  bgen_id3v2_t * id3v2;

  lame = data;

  //  bg_lame_open(&lame->com);
  //  id3tag_init(lame->lame);

  if(!strcmp(filename, "-"))
    {
    lame->output = stdout;
    if(lame->do_id3v1)
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Disabling ID3V1 tags for output to stdout");
      lame->do_id3v1 = 0;
      }
    if(lame->do_id3v2)
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Disabling ID3V2 tags for output to stdout");
      lame->do_id3v2 = 0;
      }
    }
  else
    {
    lame->filename = bg_filename_ensure_extension(filename, "mp3");

    if(!bg_encoder_cb_create_output_file(lame->cb, lame->filename))
      return 0;
  
    lame->output = fopen(lame->filename, "wb+");
    if(!lame->output)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s: %s",
             filename, strerror(errno));
      return 0;
      }
    }

  // lame->com.write_callback = write_callback;
  // lame->com.write_priv = lame->output;
  
  if(lame->do_id3v2 && metadata)
    {
    id3v2 = bgen_id3v2_create(metadata);
    if(!bgen_id3v2_write(lame->output, id3v2, lame->id3v2_charset))
      ret = 0;
    bgen_id3v2_destroy(id3v2);
    }

  /* Create id3v1 tag. It will be appended to the file at the very end */

  if(lame->do_id3v1 && metadata)
    {
    lame->id3v1 = bgen_id3v1_create(metadata);
    }
  
  return ret;
  }

static int
writes_compressed_audio_lame(void * data, const gavl_audio_format_t * format,
                             const gavl_compression_info_t * ci)
  {
  lame_priv_t * lame = data;

  if(ci->id != GAVL_CODEC_ID_MP3)
    return 0;
  
  if((ci->bitrate == GAVL_BITRATE_VBR) && (lame->output == stdout))
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "VBR mp3 cannot be written to stdout");
    return 0;
    }
  return 1;
  }

static gavl_sink_status_t
write_audio_packet_func_lame(void * data, gavl_packet_t * p)
  {
  lame_priv_t * lame;
  
  lame = data;

  if((lame->ci.bitrate == GAVL_BITRATE_VBR) &&
     !lame->xing)
    {
    lame->xing = bg_xing_create(p->data, p->data_len);
    lame->xing_pos = ftell(lame->output);
    
    if(!bg_xing_write(lame->xing, lame->output))
      return GAVL_SINK_ERROR;
    }

  if(lame->xing)
    bg_xing_update(lame->xing, p->data_len);
  
  if(fwrite(p->data, 1, p->data_len, lame->output) < p->data_len)
    return GAVL_SINK_ERROR;
  return GAVL_SINK_OK;
  }


static gavl_packet_sink_t *
get_packet_sink_lame(void * data, int stream)
  {
  lame_priv_t * lame = data;
  return lame->psink;
  }

static int
add_audio_stream_lame(void * data,
                      const gavl_metadata_t * m,
                      const gavl_audio_format_t * format)
  {
  lame_priv_t * lame = data;
  gavl_audio_format_copy(&lame->fmt, format);
  return 0;
  }

static int
add_audio_stream_compressed_lame(void * data,
                                 const gavl_metadata_t * m,
                                 const gavl_audio_format_t * format,
                                 const gavl_compression_info_t * ci)
  {
  lame_priv_t * lame = data;

  add_audio_stream_lame(data, m, format);
  gavl_compression_info_copy(&lame->ci, ci);
  lame->compressed = 1;
  return 0;
  }

static void set_audio_parameter_lame(void * data, int stream, const char * name,
                                     const bg_parameter_value_t * val)
  {
  lame_priv_t * lame = data;
  bg_lame_set_parameter(lame->codec, name, val);
  }


static int start_lame(void * data)
  {
  lame_priv_t * lame = data;

  /* Create sink */
  lame->psink = gavl_packet_sink_create(NULL, write_audio_packet_func_lame,
                                        lame);

  if(!lame->compressed)
    {
    lame->asink = bg_lame_open(lame->codec,
                               &lame->ci,
                               &lame->fmt,
                               NULL);

    if((lame->ci.bitrate == GAVL_BITRATE_VBR) && (lame->output == stdout))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Won't write VBR mp3 to stdout");
      return 0;
      }

    bg_lame_set_packet_sink(lame->codec, lame->psink);
    }
  
  return 1;
  }

static int close_lame(void * data, int do_delete)
  {
  int ret = 1;
  lame_priv_t * lame = data;

  bg_lame_destroy(lame->codec);
  lame->codec = NULL;
  
  /* Write xing tag */  
  if(lame->xing)
    {
    uint64_t pos = ftell(lame->output);
    fseek(lame->output, lame->xing_pos, SEEK_SET);
    bg_xing_write(lame->xing, lame->output);
    fseek(lame->output, pos, SEEK_SET);
    }
  
  /* Write ID3V1 tag */

  if(lame->output && (lame->output != stdout))
    {
    if(lame->output == stdout)
      {
      fflush(lame->output);
      }
    else
      {
      if(ret && lame->id3v1)
        {
        fseek(lame->output, 0, SEEK_END);
        if(!bgen_id3v1_write(lame->output, lame->id3v1))
          ret = 0;
        bgen_id3v1_destroy(lame->id3v1);
        lame->id3v1 = NULL;
        }
    /* 4. Close output file */
      fclose(lame->output);
      lame->output = NULL;
      }
    }
  /* Clean up */
  //  bg_lame_close(&lame->com);
  
  if(lame->filename)
    {
    /* Remove if necessary */
    if(do_delete)
      remove(lame->filename);
    free(lame->filename);
    lame->filename = NULL;
    }

  if(lame->psink)
    gavl_packet_sink_destroy(lame->psink);
  
  return 1;
  }

const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            "e_lame",       /* Unique short name */
      .long_name =       TRS("Lame mp3 encoder"),
      .description =     TRS("Encoder for mp3 files. Based on lame (http://www.mp3dev.org). Supports CBR, ABR and VBR as well as ID3V1 and ID3V2 tags."),
      .type =            BG_PLUGIN_ENCODER_AUDIO,
      .flags =           BG_PLUGIN_FILE | BG_PLUGIN_PIPE,
      .priority =        5,
      .create =            create_lame,
      .destroy =           destroy_lame,
      .get_parameters =    get_parameters_lame,
      .set_parameter =     set_parameter_lame,
    },
    .max_audio_streams =   1,
    .max_video_streams =   0,
    
    .set_callbacks =       set_callbacks_lame,
    
    .open =                open_lame,
    .writes_compressed_audio = writes_compressed_audio_lame,
    .get_audio_parameters =    get_audio_parameters_lame,

    .add_audio_stream =        add_audio_stream_lame,
    .add_audio_stream_compressed =        add_audio_stream_compressed_lame,
    
    .set_audio_parameter =     set_audio_parameter_lame,

    .start = start_lame,
    
    .get_audio_sink =          get_audio_sink_lame,
    .get_audio_packet_sink =   get_packet_sink_lame,
    
    .close =                   close_lame
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
