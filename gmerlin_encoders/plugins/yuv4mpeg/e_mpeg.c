/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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
#include <yuv4mpeg.h>

#include <config.h>

#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/subprocess.h>
#include <gmerlin/translation.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "e_mpeg"

#include <gmerlin_encoders.h>
#include "mpa_common.h"
#include "mpv_common.h"

// #define DEBUG_MPLEX

#define FORMAT_MPEG1   0
#define FORMAT_VCD     1
#define FORMAT_MPEG2   3
#define FORMAT_SVCD    4
#define FORMAT_DVD_NAV 8
#define FORMAT_DVD     9

typedef struct
  {
  bg_mpa_common_t mpa;
  char * filename;
  gavl_audio_format_t format;
  const gavl_compression_info_t * ci;
    
  int64_t start_pts;
  } audio_stream_t;

typedef struct
  {
  bg_mpv_common_t mpv;
  char * filename;
  gavl_video_format_t format;
  const gavl_compression_info_t * ci;
    
  int64_t start_pts;
  } video_stream_t;

typedef struct
  {
  int is_open;
  char * filename;
  
  //  bg_parameter_info_t * parameters;
  
  int format; /* See defines above */

  int num_video_streams;
  int num_audio_streams;

  audio_stream_t * audio_streams;
  video_stream_t * video_streams;
  
  char * tmp_dir;
  char * aux_stream_1;
  char * aux_stream_2;
  char * aux_stream_3;
  
  bg_encoder_callbacks_t * cb;
  } e_mpeg_t;

static void * create_mpeg()
  {
  e_mpeg_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void set_callbacks_mpeg(void * data, bg_encoder_callbacks_t * cb)
  {
  e_mpeg_t * mpg = data;
  mpg->cb = cb;
  }

static int open_mpeg(void * data, const char * filename,
                     const bg_metadata_t * metadata,
                     const bg_chapter_list_t * chapter_list)
  {
  e_mpeg_t * e = data;
  
  e->filename = bg_filename_ensure_extension(filename, "mpg");

  if(!bg_encoder_cb_create_output_file(e->cb, e->filename))
    return 0;
 
  /* To make sure this will work, we check for the execuables of
     mpeg2enc, mplex and mp2enc */

  if(!bg_search_file_exec("mpeg2enc", (char**)0))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot find mpeg2enc exectuable");
    return 0;
    }
  if(!bg_search_file_exec("mp2enc", (char**)0))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot find mp2enc exectuable");
    return 0;
    }
  if(!bg_search_file_exec("mplex", (char**)0))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot find mplex exectuable");
    return 0;
    }
  return 1;
  }

static int add_audio_stream_mpeg(void * data,
                                 const char * language,
                                 const gavl_audio_format_t * format)
  {
  e_mpeg_t * e = data;

  e->audio_streams =
    realloc(e->audio_streams,
            (e->num_audio_streams+1)*sizeof(*(e->audio_streams)));
  memset(&e->audio_streams[e->num_audio_streams], 0,
         sizeof(*(e->audio_streams)));

  gavl_audio_format_copy(&e->audio_streams[e->num_audio_streams].format,
                         format);

  e->audio_streams[e->num_audio_streams].start_pts =
    GAVL_TIME_UNDEFINED;
  
  e->num_audio_streams++;
  return e->num_audio_streams - 1;
  }

static int add_audio_stream_compressed_mpeg(void * data,
                                            const char * language,
                                            const gavl_audio_format_t * format,
                                            const gavl_compression_info_t * ci)
  {
  e_mpeg_t * e = data;
  
  e->audio_streams =
    realloc(e->audio_streams,
            (e->num_audio_streams+1)*sizeof(*(e->audio_streams)));
  memset(&e->audio_streams[e->num_audio_streams], 0,
         sizeof(*(e->audio_streams)));
  gavl_audio_format_copy(&e->audio_streams[e->num_audio_streams].format,
                         format);
  e->audio_streams[e->num_audio_streams].ci = ci;

  e->audio_streams[e->num_audio_streams].start_pts =
    GAVL_TIME_UNDEFINED;

  e->num_audio_streams++;
  return e->num_audio_streams - 1;
  }


static int add_video_stream_mpeg(void * data, const gavl_video_format_t* format)
  {
  e_mpeg_t * e = data;

  e->video_streams =
    realloc(e->video_streams,
            (e->num_video_streams+1)*sizeof(*(e->video_streams)));
  memset(&e->video_streams[e->num_video_streams], 0,
         sizeof(*(e->video_streams)));
  
  gavl_video_format_copy(&e->video_streams[e->num_video_streams].format,
                         format);
  e->video_streams[e->num_video_streams].start_pts =
    GAVL_TIME_UNDEFINED;
  e->num_video_streams++;
  return (e->num_video_streams - 1);
  }

static int add_video_stream_compressed_mpeg(void * data,
                                            const gavl_video_format_t* format,
                                            const gavl_compression_info_t * ci)
  {
  e_mpeg_t * e = data;

  e->video_streams =
    realloc(e->video_streams,
            (e->num_video_streams+1)*sizeof(*(e->video_streams)));
  memset(&e->video_streams[e->num_video_streams], 0,
         sizeof(*(e->video_streams)));

  gavl_video_format_copy(&e->video_streams[e->num_video_streams].format,
                         format);
  e->video_streams[e->num_video_streams].ci = ci;
  e->video_streams[e->num_video_streams].start_pts =
    GAVL_TIME_UNDEFINED;
  e->num_video_streams++;
  return (e->num_video_streams - 1);
  }

static int writes_compressed_video_mpeg(void * priv,
                                 const gavl_video_format_t * format,
                                 const gavl_compression_info_t * info)
  {
  e_mpeg_t * e = priv;
  switch(info->id)
    {
    case GAVL_CODEC_ID_MPEG1:
      switch(e->format)
        {
        case FORMAT_MPEG1:
          return 1;
          break;
        /* TODO: Check for valid MPEG-1 resolutions on VCDs */
        case FORMAT_VCD:
          return 1;
          break;
        case FORMAT_MPEG2:
          return 0;
          break;
        case FORMAT_SVCD:
          return 0;
          break;
        /* TODO: Check for valid MPEG-1 resolutions on DVDs */
        case FORMAT_DVD_NAV:
          return 0;
          break;
        case FORMAT_DVD:
          return 0;
          break;
        }
      break;
    case GAVL_CODEC_ID_MPEG2:
      switch(e->format)
        {
        case FORMAT_MPEG1:
          return 0;
          break;
        case FORMAT_VCD:
          return 0;
          break;
        case FORMAT_MPEG2:
          return 1;
          break;
        /* TODO: Check for valid MPEG-2 resolutions on SVCDs */
        case FORMAT_SVCD:
          return 1;
          break;
        /* TODO: Check for valid MPEG-2 resolutions on DVDs */
        case FORMAT_DVD_NAV:
          return 1;
          break;
        case FORMAT_DVD:
          return 1;
          break;
        }
      break;
    default:
      return 0;
    }
  return 0;
  }

static int writes_compressed_audio_mpeg(void * priv,
                                        const gavl_audio_format_t * format,
                                        const gavl_compression_info_t * info)
  {
  e_mpeg_t * e = priv;
  switch(info->id)
    {
    case GAVL_CODEC_ID_MP2:
      {
      switch(e->format)
        {
        case FORMAT_MPEG1:
          return 1;
          break;
        case FORMAT_VCD:
          return 1;
          break;
        case FORMAT_MPEG2:
          return 1;
          break;
        case FORMAT_SVCD:
          return 1;
          break;
        case FORMAT_DVD_NAV:
          return 1;
          break;
        case FORMAT_DVD:
          return 1;
          break;
        }
      default:
        break;
      }
    }
  return 0;
  }
  
static void get_audio_format_mpeg(void * data, int stream,
                                  gavl_audio_format_t * ret)
  {
  e_mpeg_t * e = data;
  bg_mpa_get_format(&e->audio_streams[stream].mpa, ret);
  }

static void get_video_format_mpeg(void * data, int stream,
                                  gavl_video_format_t * ret)
  {
  e_mpeg_t * e = data;
  bg_mpv_get_format(&e->video_streams[stream].mpv, ret);
  }

static char * get_filename(e_mpeg_t * e, const char * extension, int is_audio)
  {
  char * start, *end;
  char * template, * ret;
  
  if(!e->tmp_dir || (*e->tmp_dir == '\0'))
    {
    start = e->filename;
    end = strrchr(e->filename, '.');
    if(!end)
      {
      end = start + strlen(start);
      }
    template = bg_strndup((char*)0, start, end);
    }
  else
    {
    template = bg_sprintf("%s/", e->tmp_dir);
    start = strrchr(e->filename, '/');
    if(!start)
      start = e->filename;
    else
      start++;

    end = strrchr(e->filename, '.');
    if(!end)
      end = start + strlen(start);

    template = bg_strncat(template, start, end);
    }
  if(is_audio)
    {
    template = bg_strcat(template, "_audio_%04d.");
    }
  else
    {
    template = bg_strcat(template, "_video_%04d.");
    }
  template = bg_strcat(template, extension);
  ret = bg_create_unique_filename(template);
  free(template);
  return ret;
    
  } 

static int start_mpeg(void * data)
  {
  
  int i;
  e_mpeg_t * e = data;
  e->is_open = 1;
  
  for(i = 0; i < e->num_audio_streams; i++)
    {
    e->audio_streams[i].filename =
      get_filename(e, bg_mpa_get_extension(&e->audio_streams[i].mpa), 1);

    if(!e->audio_streams[i].filename)
      return 0;

    if(e->audio_streams[i].ci)
      bg_mpa_set_ci(&e->audio_streams[i].mpa, e->audio_streams[i].ci);
    else
      bg_mpa_set_format(&e->audio_streams[i].mpa, &e->audio_streams[i].format);
    if(!bg_mpa_start(&e->audio_streams[i].mpa, e->audio_streams[i].filename))
      return 0;
    }
  for(i = 0; i < e->num_video_streams; i++)
    {
    e->video_streams[i].filename =
      get_filename(e, bg_mpv_get_extension(&e->video_streams[i].mpv), 0);

    if(!e->video_streams[i].filename)
      return 0;

    if(e->video_streams[i].ci)
      bg_mpv_set_ci(&e->video_streams[i].mpv, e->video_streams[i].ci);
    
    bg_mpv_open(&e->video_streams[i].mpv, e->video_streams[i].filename);

    if(!e->video_streams[i].ci)
      bg_mpv_set_format(&e->video_streams[i].mpv, &e->video_streams[i].format);
    
    if(!bg_mpv_start(&e->video_streams[i].mpv))
      return 0;
    }
  return 1;
  }

static int write_audio_frame_mpeg(void * data, gavl_audio_frame_t* frame,
                                  int stream)
  {
  audio_stream_t * s;
  e_mpeg_t * e = data;
  s = &e->audio_streams[stream];
  if(s->start_pts == GAVL_TIME_UNDEFINED)
    s->start_pts = frame->timestamp;
  return bg_mpa_write_audio_frame(&s->mpa, frame);
  }

static int write_video_frame_mpeg(void * data, gavl_video_frame_t* frame,
                                  int stream)
  {
  video_stream_t * s;
  e_mpeg_t * e = data;
  s = &e->video_streams[stream];
  if(s->start_pts == GAVL_TIME_UNDEFINED)
    s->start_pts = frame->timestamp;
  return bg_mpv_write_video_frame(&s->mpv, frame);
  }

static int write_audio_packet_mpeg(void * data, gavl_packet_t* p,
                                   int stream)
  {
  audio_stream_t * s;
  e_mpeg_t * e = data;
  s = &e->audio_streams[stream];
  if(s->start_pts == GAVL_TIME_UNDEFINED)
    s->start_pts = p->pts;
  return bg_mpa_write_audio_packet(&s->mpa, p);
  }

static int write_video_packet_mpeg(void * data, gavl_packet_t* p,
                                   int stream)
  {
  video_stream_t * s;
  e_mpeg_t * e = data;
  s = &e->video_streams[stream];
  if(s->start_pts == GAVL_TIME_UNDEFINED)
    s->start_pts = p->pts;
  return bg_mpv_write_video_packet(&s->mpv, p);
  }

static int close_mpeg(void * data, int do_delete)
  {
#ifndef DEBUG_MPLEX
  bg_subprocess_t * proc;
#endif
  char * commandline;
  char * tmp_string;
  int ret = 1;
  int i;
  e_mpeg_t * e = data;

  int64_t sync_offset   = 0;
  int64_t audio_pts_max = 0, audio_pts_min = 0, pts;
  
  if(!e->is_open)
    return 1;
  e->is_open = 0;
  
  /* 1. Step: Close all streams */

  for(i = 0; i < e->num_audio_streams; i++)
    {
    if(!bg_mpa_close(&e->audio_streams[i].mpa))
      {
      ret = 0;
      break;
      }

    pts = gavl_time_rescale(e->audio_streams[i].format.samplerate,
                            90000, e->audio_streams[i].start_pts);

    if(!i)
      {
      audio_pts_min = pts;
      audio_pts_max = pts;
      }
    else
      {
      if(audio_pts_min > pts)
        audio_pts_min = pts;
      if(audio_pts_max < pts)
        audio_pts_max = pts;
      }
    }
  for(i = 0; i < e->num_video_streams; i++)
    {
    if(!bg_mpv_close(&e->video_streams[i].mpv))
      {
      ret = 0;
      break;
      }
    } 

  /* Get the pts offset between audio and video */
  if((e->num_video_streams == 1) && (e->num_audio_streams))
    {
    sync_offset = gavl_time_rescale(e->video_streams[0].format.timescale,
                                    90000, e->video_streams[0].start_pts) -
      (audio_pts_max - audio_pts_min) / 2; // Minimize maximum error
    }
  
  if(!do_delete && ret)
    {
    /* 2. Step: Build mplex commandline */
    
    if(!bg_search_file_exec("mplex", &commandline))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Cannot find mplex exectuable");
      return 0;
      }
    /* Options */

    tmp_string = bg_sprintf(" -f %d", e->format);
    commandline = bg_strcat(commandline, tmp_string);
    free(tmp_string);

    if(sync_offset)
      {
      bg_log(BG_LOG_DEBUG, LOG_DOMAIN,
             "Video sync offset: %"PRId64, sync_offset);

      tmp_string = bg_sprintf(" --sync-offset %"PRId64"mpt", sync_offset);
      commandline = bg_strcat(commandline, tmp_string);
      free(tmp_string);
      }
    
    commandline = bg_strcat(commandline, " -v 0 -o \"");
    
    commandline = bg_strcat(commandline, e->filename);
    commandline = bg_strcat(commandline, "\"");
    
    /* Audio and video streams */
    for(i = 0; i < e->num_video_streams; i++)
      {
      tmp_string = bg_sprintf(" \"%s\"", e->video_streams[i].filename);
      commandline = bg_strcat(commandline, tmp_string);
      free(tmp_string);
      }
    for(i = 0; i < e->num_audio_streams; i++)
      {
      tmp_string = bg_sprintf(" \"%s\"", e->audio_streams[i].filename);
      commandline = bg_strcat(commandline, tmp_string);
      free(tmp_string);
      }
    /* Other streams */
    if(e->aux_stream_1)
      {
      tmp_string = bg_sprintf(" \"%s\"", e->aux_stream_1);
      commandline = bg_strcat(commandline, tmp_string);
      free(tmp_string);
      }
    if(e->aux_stream_2)
      {
      tmp_string = bg_sprintf(" \"%s\"", e->aux_stream_2);
      commandline = bg_strcat(commandline, tmp_string);
      free(tmp_string);
      }
    if(e->aux_stream_3)
      {
      tmp_string = bg_sprintf(" \"%s\"", e->aux_stream_3);
      commandline = bg_strcat(commandline, tmp_string);
      free(tmp_string);
      }
    
    /* 3. Step: Execute mplex */
#ifndef DEBUG_MPLEX
    proc = bg_subprocess_create(commandline, 0, 0, 0);
    if(bg_subprocess_close(proc))
      ret = 0;
#else
    bg_dprintf("Mplex command: %s", commandline);
#endif
    free(commandline);
    }
  /* 4. Step: Clean up */

  if(e->num_audio_streams)
    {
    for(i = 0; i < e->num_audio_streams; i++)
      {
      if(e->audio_streams[i].filename)
        {
        bg_log(BG_LOG_INFO, LOG_DOMAIN,
               "Removing %s", e->audio_streams[i].filename);
#ifndef DEBUG_MPLEX
        remove(e->audio_streams[i].filename);
#endif
        free(e->audio_streams[i].filename);
        }
      }
    free(e->audio_streams);
    }
  if(e->num_video_streams)
    {
    for(i = 0; i < e->num_video_streams; i++)
      {
      if(e->video_streams[i].filename)
        {
        bg_log(BG_LOG_INFO, LOG_DOMAIN,
               "Removing %s", e->video_streams[i].filename);
#ifndef DEBUG_MPLEX
        remove(e->video_streams[i].filename);
#endif
        free(e->video_streams[i].filename);
        }
      }
    free(e->video_streams);
    }
  e->num_audio_streams = 0;
  e->num_video_streams = 0;
  return ret;
  }


static void destroy_mpeg(void * data)
  {
  e_mpeg_t * e = data;

  close_mpeg(data, 1);
  
  free(e);
  }

/*

#define FORMAT_MPEG1   0
#define FORMAT_VCD     1
#define FORMAT_MPEG2   3
#define FORMAT_SVCD    4
#define FORMAT_DVD_NAV 8
#define FORMAT_DVD     9

*/

static const bg_parameter_info_t common_parameters[] =
  {
    {
      .name =      "format",
      .long_name = TRS("Format"),
      .type =      BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "mpeg1" },
      .multi_names =    (char const *[]) { "mpeg1",            "vcd",          "mpeg2",            "svcd",         "dvd_nav",   "dvd", (char*)0 },
      .multi_labels =   (char const *[]) { TRS("MPEG-1 (generic)"), TRS("MPEG-1 (VCD)"),
                                  TRS("MPEG-2 (generic)"), TRS("MPEG-2 (SVCD)"),
                                  TRS("DVD (NAV)"), TRS("DVD"), (char*)0 },
      .help_string = TRS("Output format. Note that for some output formats (e.g. VCD), you MUST use proper settings for the audio and video streams also, since this isn't done automatically")
    },
    {
      .name =        "tmp_dir",
      .long_name =   TRS("Directory for temporary files"),
      .type =        BG_PARAMETER_DIRECTORY,
      .help_string = TRS("Leave empty to use the same directory as the final output file"),
    },
    {
      .name =        "aux_stream_1",
      .long_name =   TRS("Additional stream 1"),
      .type =        BG_PARAMETER_FILE,
      .help_string = TRS("Additional stream to multiplex into the final output file. Use this if you \
want e.g. create mp3 or AC3 audio with some other encoder"),
    },
    {
      .name =        "aux_stream_2",
      .long_name =   TRS("Additional stream 2"),
      .type =        BG_PARAMETER_FILE,
      .help_string = TRS("Additional stream to multiplex into the final output file. Use this if you \
want e.g. create mp3 or AC3 audio with some other encoder"),
    },
    {
      .name =        "aux_stream_3",
      .long_name =   TRS("Additional stream 3"),
      .type =        BG_PARAMETER_FILE,
      .help_string = TRS("Additional stream to multiplex into the final output file. Use this if you \
want e.g. create mp3 or AC3 audio with some other encoder"),
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_mpeg(void * data)
  {
  return common_parameters;
  }

#define SET_ENUM(ret, key, v) if(!strcmp(val->val_str, key)) ret = v;

#define SET_STRING(key) if(!strcmp(# key, name)) e->key = bg_strdup(e->key, val->val_str);

static void set_parameter_mpeg(void * data, const char * name,
                               const bg_parameter_value_t * val)
  {
  e_mpeg_t * e = data;
  if(!name)
    return;

  else if(!strcmp(name, "format"))
    {
    SET_ENUM(e->format, "mpeg1",   FORMAT_MPEG1);
    SET_ENUM(e->format, "vcd",     FORMAT_VCD);
    SET_ENUM(e->format, "mpeg2",   FORMAT_MPEG2);
    SET_ENUM(e->format, "svcd",    FORMAT_SVCD);
    SET_ENUM(e->format, "dvd_nav", FORMAT_DVD_NAV);
    SET_ENUM(e->format, "dvd",     FORMAT_DVD);
    }

  SET_STRING(tmp_dir);
  SET_STRING(aux_stream_1);
  SET_STRING(aux_stream_2);
  SET_STRING(aux_stream_3);
  }


static const bg_parameter_info_t * get_audio_parameters_mpeg(void * data)
  {
  return bg_mpa_get_parameters();
  }

static const bg_parameter_info_t * get_video_parameters_mpeg(void * data)
  {
  return bg_mpv_get_parameters();
  }

static void set_audio_parameter_mpeg(void * data, int stream,
                                     const char * name,
                                     const bg_parameter_value_t * val)
  {
  e_mpeg_t * e = data;
  
  if(!name)
    return;
  bg_mpa_set_parameter(&e->audio_streams[stream].mpa, name, val);
  
  }


static void set_video_parameter_mpeg(void * data, int stream,
                                     const char * name,
                                     const bg_parameter_value_t * val)
  {
  e_mpeg_t * e = data;
  if(!name)
    return;
  bg_mpv_set_parameter(&e->video_streams[stream].mpv, name, val);
  }


const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "e_mpeg",       /* Unique short name */
      .long_name =      TRS("MPEG 1/2 program/system stream encoder"),
      .description =      TRS("Encoder for regular .mpg files as well as VCD and DVD streams.\
 Based on mjpegtools (http://mjpeg.sourceforge.net)"),
      .type =           BG_PLUGIN_ENCODER,
      .flags =          BG_PLUGIN_FILE,
      .priority =       5,
      .create =         create_mpeg,
      .destroy =        destroy_mpeg,
      .get_parameters = get_parameters_mpeg,
      .set_parameter =  set_parameter_mpeg,
    },

    .max_audio_streams = -1,
    .max_video_streams = -1,

    .get_audio_parameters = get_audio_parameters_mpeg,
    .get_video_parameters = get_video_parameters_mpeg,

    .writes_compressed_audio = writes_compressed_audio_mpeg,
    .writes_compressed_video = writes_compressed_video_mpeg,
        
    .set_callbacks =        set_callbacks_mpeg,

    .open =                 open_mpeg,

    .add_audio_stream =     add_audio_stream_mpeg,
    .add_video_stream =     add_video_stream_mpeg,
    .add_audio_stream_compressed =     add_audio_stream_compressed_mpeg,
    .add_video_stream_compressed =     add_video_stream_compressed_mpeg,

    .set_audio_parameter =  set_audio_parameter_mpeg,
    .set_video_parameter =  set_video_parameter_mpeg,

    .get_audio_format =     get_audio_format_mpeg,
    .get_video_format =     get_video_format_mpeg,

    .start =                start_mpeg,

    .write_audio_frame = write_audio_frame_mpeg,
    .write_video_frame = write_video_frame_mpeg,

    .write_audio_packet = write_audio_packet_mpeg,
    .write_video_packet = write_video_packet_mpeg,

    .close =             close_mpeg,

  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
