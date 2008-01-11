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

#include <string.h>
#include <ctype.h>

#include <config.h>
#include <translation.h>

#include <plugin.h>
#include <utils.h>
#include <log.h>

#include "lqt_common.h"
#include "lqtgavl.h"

#define LOG_DOMAIN "i_lqt"

#define PARAM_AUDIO 1
#define PARAM_VIDEO 3

static bg_parameter_info_t parameters[] = 
  {
    {
      .name =      "audio",
      .long_name = TRS("Audio"),
      .type =      BG_PARAMETER_SECTION,
    },
    {
      .name =      "audio_codecs",
      .opt =       "ac",
      .long_name = TRS("Audio Codecs"),
      .help_string = TRS("Sort and configure audio codecs"),
    },
    {
      .name =      "video",
      .long_name = TRS("Video"),
      .type =      BG_PARAMETER_SECTION,
    },
    {
      .name =      "video_codecs",
      .opt =       "vc",
      .long_name = TRS("Video Codecs"),
      .help_string = TRS("Sort and configure video codecs"),
    },
    { /* End of parameters */ }
  };

typedef struct
  {
  quicktime_t * file;
  bg_parameter_info_t * parameters;

  char * audio_codec_string;
  char * video_codec_string;

  bg_track_info_t track_info;

  struct
    {
    int quicktime_index;
    } * audio_streams;
  
  struct
    {
    int quicktime_index;
    unsigned char ** rows;
    } * video_streams;
  struct
    {
    int quicktime_index;
    int timescale;
    } * subtitle_streams;
  } i_lqt_t;

static void * create_lqt()
  {
  i_lqt_t * ret = calloc(1, sizeof(*ret));

  lqt_set_log_callback(bg_lqt_log, NULL);

  return ret;
  }

static void setup_chapters(i_lqt_t * e, int track)
  {
  int i, num;
  int timescale;
  int64_t timestamp, duration;
  char * text = (char*)0;
  int text_alloc = 0;
  gavl_time_t chapter_time;
  
  e->track_info.chapter_list = bg_chapter_list_create(0);
  timescale = lqt_text_time_scale(e->file, track);

  num = lqt_text_samples(e->file, track);
  
  for(i = 0; i < num; i++)
    {
    if(lqt_read_text(e->file, track, &text, &text_alloc, &timestamp, &duration))
      {
      chapter_time = gavl_time_unscale(timescale, timestamp);
      bg_chapter_list_insert(e->track_info.chapter_list, i, chapter_time, text);
      }
    else
      break;
    }
  if(text) free(text);
  }

static int open_lqt(void * data, const char * arg)
  {
  char * tmp_string;
  int i;
  char * filename;
  int num_audio_streams = 0;
  int num_video_streams = 0;
  int num_text_streams = 0;
  i_lqt_t * e = (i_lqt_t*)data;

  lqt_codec_info_t ** codec_info;

  
  /* We want to keep the thing const-clean */
  filename = bg_strdup((char*)0, arg);
  e->file = quicktime_open(filename, 1, 0);
  free(filename);

  if(!e->file)
    return 0;

  bg_set_track_name_default(&(e->track_info), arg);

  /* Set metadata */

  e->track_info.metadata.title =
    bg_strdup((char*)0, quicktime_get_name(e->file));
  e->track_info.metadata.copyright =
    bg_strdup((char*)0, quicktime_get_copyright(e->file));

  e->track_info.metadata.comment =
    bg_strdup((char*)0, lqt_get_comment(e->file));
  if(!e->track_info.metadata.comment)
    e->track_info.metadata.comment =
      bg_strdup((char*)0, quicktime_get_info(e->file));

  tmp_string = lqt_get_track(e->file);
  if(tmp_string && isdigit(*tmp_string))
    e->track_info.metadata.track = atoi(tmp_string);

  e->track_info.metadata.artist = bg_strdup((char*)0, lqt_get_artist(e->file));
  e->track_info.metadata.album  = bg_strdup((char*)0, lqt_get_album(e->file));
  e->track_info.metadata.genre  = bg_strdup((char*)0, lqt_get_genre(e->file));
  e->track_info.metadata.author  = bg_strdup((char*)0, lqt_get_author(e->file));
  
  /* Query streams */

  num_audio_streams = quicktime_audio_tracks(e->file);
  num_video_streams = quicktime_video_tracks(e->file);
  num_text_streams  = lqt_text_tracks(e->file);

  e->track_info.duration = 0;
  e->track_info.seekable = 1;
  if(num_audio_streams)
    {
    e->audio_streams = calloc(num_audio_streams, sizeof(*e->audio_streams));
    e->track_info.audio_streams =
      calloc(num_audio_streams, sizeof(*e->track_info.audio_streams));
    
    for(i = 0; i < num_audio_streams; i++)
      {
      if(quicktime_supported_audio(e->file, i))
        {
        e->audio_streams[e->track_info.num_audio_streams].quicktime_index = i;

        codec_info = lqt_audio_codec_from_file(e->file, i);
        e->track_info.audio_streams[e->track_info.num_audio_streams].description =
          bg_strdup(e->track_info.audio_streams[e->track_info.num_audio_streams].description,
                    codec_info[0]->long_name);
        lqt_destroy_codec_info(codec_info);

        lqt_get_audio_language(e->file, i,
                               e->track_info.audio_streams[e->track_info.num_audio_streams].language);
        e->track_info.num_audio_streams++;
        }
      }
    }

  if(num_video_streams)
    {
    e->video_streams = calloc(num_video_streams, sizeof(*e->video_streams));
    e->track_info.video_streams =
      calloc(num_video_streams, sizeof(*e->track_info.video_streams));
    
    
    for(i = 0; i < num_video_streams; i++)
      {
      if(quicktime_supported_video(e->file, i))
        {
        e->video_streams[e->track_info.num_video_streams].quicktime_index = i;
        
        codec_info = lqt_video_codec_from_file(e->file, i);
        e->track_info.video_streams[e->track_info.num_video_streams].description =
          bg_strdup(e->track_info.video_streams[e->track_info.num_video_streams].description,
                    codec_info[0]->long_name);
        lqt_destroy_codec_info(codec_info);

                
        e->video_streams[e->track_info.num_video_streams].rows = lqt_gavl_rows_create(e->file, i);
        
        e->track_info.num_video_streams++;
        }
      }
    }
  if(num_text_streams)
    {
    e->subtitle_streams = calloc(num_text_streams, sizeof(*e->subtitle_streams));
    e->track_info.subtitle_streams =
      calloc(num_text_streams, sizeof(*e->track_info.subtitle_streams));
    
    for(i = 0; i < num_text_streams; i++)
      {
      if(lqt_is_chapter_track(e->file, i))
        {
        if(e->track_info.chapter_list)
          {
          bg_log(BG_LOG_WARNING, LOG_DOMAIN,
                 "More than one chapter track found, using first one");
          }
        else
          setup_chapters(e, i);
        }
      else
        {
        e->subtitle_streams[e->track_info.num_subtitle_streams].quicktime_index = i;
        e->subtitle_streams[e->track_info.num_subtitle_streams].timescale =
          lqt_text_time_scale(e->file, i);
        
        lqt_get_text_language(e->file, i,
                              e->track_info.subtitle_streams[e->track_info.num_subtitle_streams].language);
        
        e->track_info.subtitle_streams[e->track_info.num_subtitle_streams].is_text = 1;
        
        e->track_info.num_subtitle_streams++;
        }
      }

    
    }

  e->track_info.duration = lqt_gavl_duration(e->file);

  if(lqt_is_avi(e->file))
     e->track_info.description = bg_strdup(e->track_info.description,
                                           "AVI (lqt)");
  else
    e->track_info.description = bg_strdup(e->track_info.description,
                                            "Quicktime (lqt)");
  
  //  if(!e->track_info.num_audio_streams && !e->track_info.num_video_streams)
  //    return 0;
  return 1;
  }

static int get_num_tracks_lqt(void * data)
  {
  return 1;
  }

static bg_track_info_t * get_track_info_lqt(void * data, int track)
  {
  i_lqt_t * e = (i_lqt_t*)data;
  return &(e->track_info);
  }

/* Read one audio frame (returns FALSE on EOF) */
static  
int read_audio_samples_lqt(void * data, gavl_audio_frame_t * f, int stream,
                          int num_samples)
  {
  i_lqt_t * e = (i_lqt_t*)data;

  lqt_gavl_decode_audio(e->file, e->audio_streams[stream].quicktime_index,
                        f, num_samples);
  return f->valid_samples;
  }


static int has_subtitle_lqt(void * data, int stream)
  {
  return 1;
  }

static int read_subtitle_text_lqt(void * priv,
                                  char ** text, int * text_alloc,
                                  int64_t * start_time,
                                  int64_t * duration, int stream)
  {
  int64_t start_time_scaled, duration_scaled;
  i_lqt_t * e = (i_lqt_t*)priv;

  if(lqt_read_text(e->file, stream, text, text_alloc,
                   &start_time_scaled, &duration_scaled))
    {
    *start_time = gavl_time_unscale(e->subtitle_streams[stream].timescale,
                                    start_time_scaled);
    *duration   = gavl_time_unscale(e->subtitle_streams[stream].timescale,
                                    duration_scaled);
    return 1;
    }
  return 0;
  }


/* Read one video frame (returns FALSE on EOF) */
static
int read_video_frame_lqt(void * data, gavl_video_frame_t * f, int stream)
  {
  i_lqt_t * e = (i_lqt_t*)data;
  return lqt_gavl_decode_video(e->file,
                               e->video_streams[stream].quicktime_index,
                               f, e->video_streams[stream].rows);
  }


static void close_lqt(void * data)
  {
  int i;
  i_lqt_t * e = (i_lqt_t*)data;
  
  if(e->file)
    {
    quicktime_close(e->file);
    e->file = (quicktime_t*)0;
    }
  if(e->audio_streams)
    {
    free(e->audio_streams);
    e->audio_streams = NULL;
    }
  if(e->video_streams)
    {
    for(i = 0; i < e->track_info.num_video_streams; i++)
      {
      if(e->video_streams[i].rows)
        free(e->video_streams[i].rows);
      }
    free(e->video_streams);
    e->video_streams = NULL;
    }
  bg_track_info_free(&(e->track_info));  
  }

static void seek_lqt(void * data, gavl_time_t * time)
  {
  i_lqt_t * e = (i_lqt_t*)data;
  lqt_gavl_seek(e->file, time);
  }

static void destroy_lqt(void * data)
  {
  i_lqt_t * e = (i_lqt_t*)data;
  close_lqt(data);

  if(e->parameters)
    bg_parameter_info_destroy_array(e->parameters);

  if(e->audio_codec_string)
    free(e->audio_codec_string);

  if(e->video_codec_string)
    free(e->video_codec_string);
      
  
  free(e);
  }

static void create_parameters(i_lqt_t * e)
  {

  e->parameters = bg_parameter_info_copy_array(parameters);
    
  bg_lqt_create_codec_info(&(e->parameters[PARAM_AUDIO]),
                           1, 0, 0, 1);
  bg_lqt_create_codec_info(&(e->parameters[PARAM_VIDEO]),
                           0, 1, 0, 1);

  
  }

static bg_parameter_info_t * get_parameters_lqt(void * data)
  {
  i_lqt_t * e = (i_lqt_t*)data;
  
  if(!e->parameters)
    create_parameters(e);
  
  return e->parameters;
  }

static void set_parameter_lqt(void * data, const char * name,
                              const bg_parameter_value_t * val)
  {
  i_lqt_t * e = (i_lqt_t*)data;
  char * pos;
  char * tmp_string;
  if(!name)
    return;

  
  if(!e->parameters)
    create_parameters(e);
#if 0
  if(bg_lqt_set_parameter(name, val, &(e->parameters[PARAM_AUDIO])) ||
     bg_lqt_set_parameter(name, val, &(e->parameters[PARAM_VIDEO])))
    return;
#endif
  if(!strcmp(name, "audio_codecs"))
    {
    e->audio_codec_string = bg_strdup(e->audio_codec_string, val->val_str);
    }
  else if(!strcmp(name, "video_codecs"))
    {
    e->video_codec_string = bg_strdup(e->video_codec_string, val->val_str);
    }
  else if(!strncmp(name, "audio_codecs.", 13))
    {
    tmp_string = bg_strdup(NULL, name+13);
    pos = strchr(tmp_string, '.');
    *pos = '\0';
    pos++;
    
    bg_lqt_set_audio_decoder_parameter(tmp_string, pos, val);
    free(tmp_string);
    
    }
  else if(!strncmp(name, "video_codecs.", 13))
    {
    tmp_string = bg_strdup(NULL, name+13);
    pos = strchr(tmp_string, '.');
    *pos = '\0';
    pos++;
    
    bg_lqt_set_video_decoder_parameter(tmp_string, pos, val);
    free(tmp_string);
    
    }
  }

static int start_lqt(void * data)
  {
  int i;
  i_lqt_t * e = (i_lqt_t*)data;

  for(i = 0; i < e->track_info.num_audio_streams; i++)
    {
    lqt_gavl_get_audio_format(e->file,
                              e->audio_streams[i].quicktime_index,
                              &(e->track_info.audio_streams[i].format));
    }
  for(i = 0; i < e->track_info.num_video_streams; i++)
    {
    lqt_gavl_get_video_format(e->file,
                              e->video_streams[i].quicktime_index,
                              &(e->track_info.video_streams[i].format), 0);
    }
  return 1;
  }


bg_input_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            "i_lqt",       /* Unique short name */
      .long_name =       TRS("libquicktime input plugin"),
      .description =     TRS("Input plugin based on libquicktime"),
      .mimetypes =       NULL,
      .extensions =      "mov",
      .type =            BG_PLUGIN_INPUT,
      .flags =           BG_PLUGIN_FILE,
      .priority =        5,
      .create =          create_lqt,
      .destroy =         destroy_lqt,
      .get_parameters =  get_parameters_lqt,
      .set_parameter =   set_parameter_lqt,
    },
    
    .open =              open_lqt,
    .get_num_tracks =    get_num_tracks_lqt,
    .get_track_info =    get_track_info_lqt,
    //    .set_audio_stream =  set_audio_stream_lqt,
    //    .set_video_stream =  set_audio_stream_lqt,
    .start =             start_lqt,

    .read_audio_samples = read_audio_samples_lqt,
    .read_video_frame =   read_video_frame_lqt,

    .has_subtitle =       has_subtitle_lqt,
    .read_subtitle_text = read_subtitle_text_lqt,
    
    .seek =               seek_lqt,
    //    .stop =               stop_lqt,
    .close =              close_lqt
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
