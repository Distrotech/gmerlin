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

#include <config.h>
#include <translation.h>

#include <plugin.h>
#include <utils.h>
#include <log.h>

#include "lqt_common.h"
#include "lqtgavl.h"

#define LOG_DOMAIN "e_lqt"

#define TEXT_TIME_SCALE GAVL_TIME_SCALE
// #define TEXT_TIME_SCALE 1000

static bg_parameter_info_t stream_parameters[] = 
  {
    {
      .name =      "codec",
      .long_name = TRS("Codec"),
    },
  };

typedef struct
  {
  int max_riff_size;
  char * filename;
  quicktime_t * file;
  
  //  bg_parameter_info_t * parameters;

  char * audio_codec_name;
  char * video_codec_name;

  bg_parameter_info_t * audio_parameters;
  bg_parameter_info_t * video_parameters;
  
  lqt_file_type_t file_type;
  int make_streamable;
  
  int num_video_streams;
  int num_audio_streams;
  int num_subtitle_text_streams;
  
  /* Needed for calculating the duration of the last chapter */
  gavl_time_t duration;
  
  struct
    {
    gavl_audio_format_t format;
    lqt_codec_info_t ** codec_info;
    char language[4];
    int64_t samples_written;
    } * audio_streams;
  
  struct
    {
    gavl_video_format_t format;
    uint8_t ** rows;
    lqt_codec_info_t ** codec_info;
    } * video_streams;
  
  struct
    {
    char language[4];
    gavl_time_t last_end_time;

    uint16_t text_box[4];

    uint16_t fg_color[4];
    uint16_t bg_color[4];
    } * subtitle_text_streams;
  
  const bg_chapter_list_t * chapter_list;
  
  int chapter_track_id;
  } e_lqt_t;

static void * create_lqt()
  {
  e_lqt_t * ret = calloc(1, sizeof(*ret));

  lqt_set_log_callback(bg_lqt_log, NULL);
  
  return ret;
  }


static struct
  {
  int type_mask;
  char * extension;
  }
extensions[] =
  {
    { LQT_FILE_QT  | LQT_FILE_QT_OLD,      ".mov" },
    { LQT_FILE_AVI | LQT_FILE_AVI_ODML,    ".avi" },
    { LQT_FILE_MP4,                        ".mp4" },
    { LQT_FILE_M4A,                        ".m4a" },
    { LQT_FILE_3GP,                        ".3gp" },
  };

static const char * get_extension_lqt(void * data)
  {
  int i;
  e_lqt_t * e = (e_lqt_t*)data;

  for(i = 0; i < sizeof(extensions)/sizeof(extensions[0]); i++)
    {
    if(extensions[i].type_mask & e->file_type)
      return extensions[i].extension;
    }
  return extensions[0].extension; /* ".mov" */
  }

static int open_lqt(void * data, const char * filename,
                    bg_metadata_t * metadata,
                    bg_chapter_list_t * chapter_list)
  {
  char * track_string;
  e_lqt_t * e = (e_lqt_t*)data;
  
  if(e->make_streamable && !(e->file_type & (LQT_FILE_AVI|LQT_FILE_AVI_ODML)))
    e->filename = bg_sprintf("%s.tmp", filename);
  else
    e->filename = bg_strdup(e->filename, filename);

  e->file = lqt_open_write(e->filename, e->file_type);
  if(!e->file)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open file %s", e->filename);
    return 0;
    }

  if(e->file_type == LQT_FILE_AVI_ODML)
    lqt_set_max_riff_size(e->file, e->max_riff_size);
    
  /* Set metadata */

  if(metadata->copyright)
    quicktime_set_copyright(e->file, metadata->copyright);
  if(metadata->title)
    quicktime_set_name(e->file, metadata->title);

  if(metadata->comment)
    lqt_set_comment(e->file, metadata->comment);
  if(metadata->artist)
    lqt_set_artist(e->file, metadata->artist);
  if(metadata->genre)
    lqt_set_genre(e->file, metadata->genre);
  if(metadata->track)
    {
    track_string = bg_sprintf("%d", metadata->track);
    lqt_set_track(e->file, track_string);
    free(track_string);
    }
  if(metadata->album)
    lqt_set_album(e->file, metadata->album);
  if(metadata->author)
    lqt_set_author(e->file, metadata->author);
  
  e->chapter_list = chapter_list;
  
  return 1;
  }

static int add_audio_stream_lqt(void * data, const char * language,
                                gavl_audio_format_t * format)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  e->audio_streams =
    realloc(e->audio_streams,
            (e->num_audio_streams+1)*sizeof(*(e->audio_streams)));
  memset(&(e->audio_streams[e->num_audio_streams]), 0,
         sizeof(*(e->audio_streams)));
  gavl_audio_format_copy(&(e->audio_streams[e->num_audio_streams].format),
                         format);

  strncpy(e->audio_streams[e->num_audio_streams].language,
          language, 3);
  
  e->num_audio_streams++;
  return e->num_audio_streams-1;
  }

static int add_subtitle_text_stream_lqt(void * data, const char * language)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  e->subtitle_text_streams =
    realloc(e->subtitle_text_streams,
            (e->num_subtitle_text_streams+1)*
            sizeof(*(e->subtitle_text_streams)));

  memset(&(e->subtitle_text_streams[e->num_subtitle_text_streams]), 0,
         sizeof(*(e->subtitle_text_streams)));

  strncpy(e->subtitle_text_streams[e->num_subtitle_text_streams].language,
          language, 3);
  
  e->num_subtitle_text_streams++;
  return e->num_subtitle_text_streams-1;
  }


static int add_video_stream_lqt(void * data,
                                gavl_video_format_t* format)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  e->video_streams =
    realloc(e->video_streams,
            (e->num_video_streams+1)*sizeof(*(e->video_streams)));
  memset(&(e->video_streams[e->num_video_streams]), 0,
         sizeof(*(e->video_streams)));
  
  gavl_video_format_copy(&(e->video_streams[e->num_video_streams].format),
                         format);

#if 0  
  /* AVIs are made with constant framerates only */
  if((e->file_type & (LQT_FILE_AVI|LQT_FILE_AVI_ODML)))
    e->video_streams[e->num_video_streams].format.framerate_mode =
      GAVL_FRAMERATE_CONSTANT;
#endif // Done by lqtgavl
  
  e->num_video_streams++;
  return e->num_video_streams-1;
  }

static void get_audio_format_lqt(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  gavl_audio_format_copy(ret, &(e->audio_streams[stream].format));
  
  }
  
static void get_video_format_lqt(void * data, int stream,
                                 gavl_video_format_t * ret)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  gavl_video_format_copy(ret, &(e->video_streams[stream].format));
  }

static int start_lqt(void * data)
  {
  int i, tmp;
  e_lqt_t * e = (e_lqt_t*)data;

  for(i = 0; i < e->num_audio_streams; i++)
    {
     /* Ugly hack */
    tmp = e->audio_streams[i].format.samples_per_frame;
    lqt_gavl_get_audio_format(e->file,
                              i,
                              &(e->audio_streams[i].format));
    e->audio_streams[i].format.samples_per_frame = tmp;
    }
  for(i = 0; i < e->num_video_streams; i++)
    {
    lqt_gavl_get_video_format(e->file,
                              i,
                              &(e->video_streams[i].format), 1);
    }
  
  /* Add the subtitle tracks */
  for(i = 0; i < e->num_subtitle_text_streams; i++)
    {
    lqt_add_text_track(e->file, TEXT_TIME_SCALE);
    lqt_set_text_language(e->file, i, e->subtitle_text_streams[i].language);
    
    lqt_set_text_box(e->file, i,
                     e->subtitle_text_streams[i].text_box[0],
                     e->subtitle_text_streams[i].text_box[1],
                     e->subtitle_text_streams[i].text_box[2],
                     e->subtitle_text_streams[i].text_box[3]);

    lqt_set_text_fg_color(e->file, i,
                          e->subtitle_text_streams[i].fg_color[0],
                          e->subtitle_text_streams[i].fg_color[1],
                          e->subtitle_text_streams[i].fg_color[2],
                          e->subtitle_text_streams[i].fg_color[3]);

    lqt_set_text_bg_color(e->file, i,
                          e->subtitle_text_streams[i].bg_color[0],
                          e->subtitle_text_streams[i].bg_color[1],
                          e->subtitle_text_streams[i].bg_color[2],
                          e->subtitle_text_streams[i].bg_color[3]);
    
    }
  
  /* Add the chapter track */
  if(e->chapter_list)
    {
    lqt_add_text_track(e->file, GAVL_TIME_SCALE);
    e->chapter_track_id = e->num_subtitle_text_streams;
    lqt_set_chapter_track(e->file, e->chapter_track_id);
    }
  return 1;
  }


static int write_audio_frame_lqt(void * data, gavl_audio_frame_t* frame,
                                 int stream)
  {
  gavl_time_t test_time;
  e_lqt_t * e = (e_lqt_t*)data;

  e->audio_streams[stream].samples_written += frame->valid_samples;
  
  test_time = gavl_time_unscale(e->audio_streams[stream].format.samplerate,
                                e->audio_streams[stream].samples_written);
  if(e->duration < test_time)
    e->duration = test_time;
  
  return !!lqt_encode_audio_raw(e->file, frame->samples.s_8,
                                frame->valid_samples, stream);
  }

static int write_video_frame_lqt(void * data, gavl_video_frame_t* frame,
                                  int stream)
  {
  gavl_time_t test_time;
  e_lqt_t * e = (e_lqt_t*)data;
  
  test_time = gavl_time_unscale(e->video_streams[stream].format.timescale,
                                frame->timestamp);
  if(e->duration < test_time)
    e->duration = test_time;
  
  return !lqt_gavl_encode_video(e->file, stream, frame,
                                e->video_streams[stream].rows);
  }

static int write_subtitle_text_lqt(void * data,const char * text,
                                   gavl_time_t start,
                                   gavl_time_t duration, int stream)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  int64_t duration_scaled;
  
  /* Put empty subtitle if the last end time is not equal to
     this start time */
  if(e->subtitle_text_streams[stream].last_end_time < start)
    {
    duration_scaled =
      gavl_time_scale(TEXT_TIME_SCALE,
                      start - e->subtitle_text_streams[stream].last_end_time);
    if(lqt_write_text(e->file, stream, "",
                      duration_scaled))
      return 0;
    }

  duration_scaled = gavl_time_scale(TEXT_TIME_SCALE, duration);

  if(lqt_write_text(e->file, stream, text, duration_scaled))
    return 0;
  e->subtitle_text_streams[stream].last_end_time = start + duration;
  return 1;
  }


static int close_lqt(void * data, int do_delete)
  {
  int i;
  char * filename_final, *pos;
  e_lqt_t * e = (e_lqt_t*)data;

  if(!e->file)
    return 1;

  /* Write chapter information */
  if(e->chapter_list)
    {
    for(i = 0; i < e->chapter_list->num_chapters; i++)
      {
      if(e->chapter_list->chapters[i].time > e->duration)
        {
        bg_log(BG_LOG_WARNING, LOG_DOMAIN,
               "Omitting chapter %d: time (%f) > duration (%f)",
               i+1,
               gavl_time_to_seconds(e->chapter_list->chapters[i].time),
               gavl_time_to_seconds(e->duration));
        break;
        }
      if(i < e->chapter_list->num_chapters-1)
        {
        if(lqt_write_text(e->file, e->chapter_track_id,
                          e->chapter_list->chapters[i].name,
                          e->chapter_list->chapters[i+1].time -
                          e->chapter_list->chapters[i].time))
          return 0;
        }
      else
        {
        if(lqt_write_text(e->file, e->chapter_track_id,
                          e->chapter_list->chapters[i].name,
                          e->duration - e->chapter_list->chapters[i].time))
          return 0;
        }
      }
    }
  
  quicktime_close(e->file);
  e->file = (quicktime_t*)0;
  
  if(do_delete)
    remove(e->filename);

  else if(e->make_streamable && !(e->file_type & (LQT_FILE_AVI|LQT_FILE_AVI_ODML)))
    {
    filename_final = bg_strdup((char*)0, e->filename);
    pos = strrchr(filename_final, '.');
    *pos = '\0';
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Making streamable....");
    quicktime_make_streamable(e->filename, filename_final);
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Making streamable....done");
    remove(e->filename);
    free(filename_final);
    }
  
  if(e->filename)
    {
    free(e->filename);
    e->filename = (char*)0;
    }
  if(e->audio_streams)
    {
    for(i = 0; i < e->num_audio_streams; i++)
      {
      if(e->audio_streams[i].codec_info)
        lqt_destroy_codec_info(e->audio_streams[i].codec_info);
      }

    free(e->audio_streams);
    e->audio_streams = NULL;
    }
  if(e->video_streams)
    {
    for(i = 0; i < e->num_video_streams; i++)
      {
      if(e->video_streams[i].codec_info)
        lqt_destroy_codec_info(e->video_streams[i].codec_info);
      lqt_gavl_rows_destroy(e->video_streams[i].rows);
      }
    
    free(e->video_streams);
    e->video_streams = NULL;
    }
  e->num_audio_streams = 0;
  e->num_video_streams = 0;
  return 1;
  }

static void destroy_lqt(void * data)
  {
  e_lqt_t * e = (e_lqt_t*)data;

  close_lqt(data, 1);
  
  if(e->audio_parameters)
    bg_parameter_info_destroy_array(e->audio_parameters);
  if(e->video_parameters)
    bg_parameter_info_destroy_array(e->video_parameters);

  free(e);
  }

static void create_parameters(e_lqt_t * e)
  {
  e->audio_parameters = calloc(2, sizeof(*(e->audio_parameters)));
  e->video_parameters = calloc(2, sizeof(*(e->video_parameters)));

  bg_parameter_info_copy(&(e->audio_parameters[0]), &(stream_parameters[0]));
  bg_parameter_info_copy(&(e->video_parameters[0]), &(stream_parameters[0]));
  
  bg_lqt_create_codec_info(&(e->audio_parameters[0]),
                           1, 0, 1, 0);
  bg_lqt_create_codec_info(&(e->video_parameters[0]),
                           0, 1, 1, 0);
  
  }

static bg_parameter_info_t common_parameters[] =
  {
    {
      .name =      "format",
      .long_name = TRS("Format"),
      .type =      BG_PARAMETER_STRINGLIST,
      .multi_names =    (char*[]) { "quicktime", "avi", "avi_opendml",   "mp4", "m4a", "3gp", (char*)0 },
      .multi_labels =   (char*[]) { TRS("Quicktime"), TRS("AVI"), TRS("AVI (Opendml)"),
                                  TRS("MP4"), TRS("M4A"), TRS("3GP"), (char*)0 },
      .val_default = { .val_str = "quicktime" },
    },
    {
      .name =      "make_streamable",
      .long_name = TRS("Make streamable"),
      .type =      BG_PARAMETER_CHECKBUTTON,
      .help_string = TRS("Make the file streamable afterwards (uses twice the diskspace)"),
    },
    {
      .name =      "max_riff_size",
      .long_name = TRS("Maximum RIFF size"),
      .type =      BG_PARAMETER_INT,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 1024 },
      .val_default = { .val_i = 1024 },
      .help_string = TRS("Maximum RIFF size (in MB) for OpenDML AVIs. The default (1GB) is reasonable and should only be changed by people who know what they do."),
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_lqt(void * data)
  {
  return common_parameters;
  }

static void set_parameter_lqt(void * data, const char * name,
                              const bg_parameter_value_t * val)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  if(!name)
    return;

  else if(!strcmp(name, "format"))
    {
    if(!strcmp(val->val_str, "quicktime"))
      e->file_type = LQT_FILE_QT;
    else if(!strcmp(val->val_str, "avi"))
      e->file_type = LQT_FILE_AVI;
    else if(!strcmp(val->val_str, "avi_opendml"))
      e->file_type = LQT_FILE_AVI_ODML;
    else if(!strcmp(val->val_str, "mp4"))
      e->file_type = LQT_FILE_MP4;
    else if(!strcmp(val->val_str, "m4a"))
      e->file_type = LQT_FILE_M4A;
    else if(!strcmp(val->val_str, "3gp"))
      e->file_type = LQT_FILE_3GP;
    }
  else if(!strcmp(name, "make_streamable"))
    e->make_streamable = val->val_i;
  else if(!strcmp(name, "max_riff_size"))
    e->max_riff_size = val->val_i;
  
  }

static bg_parameter_info_t * get_audio_parameters_lqt(void * data)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  
  if(!e->audio_parameters)
    create_parameters(e);
  
  return e->audio_parameters;
  }

static bg_parameter_info_t * get_video_parameters_lqt(void * data)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  
  if(!e->video_parameters)
    create_parameters(e);
  
  return e->video_parameters;
  }

static void set_audio_parameter_lqt(void * data, int stream, const char * name,
                                    const bg_parameter_value_t * val)
  {
  e_lqt_t * e = (e_lqt_t*)data;
    
  if(!name)
    return;

  if(!strcmp(name, "codec"))
    {
    /* Now we can add the stream */

    e->audio_streams[stream].codec_info = lqt_find_audio_codec_by_name(val->val_str);
    
    lqt_gavl_add_audio_track(e->file, &e->audio_streams[stream].format,
                             *e->audio_streams[stream].codec_info);
    lqt_set_audio_language(e->file, stream, e->audio_streams[stream].language);
    }
  else
    {

    bg_lqt_set_audio_parameter(e->file,
                               stream,
                               name,
                               val,
                               e->audio_streams[stream].codec_info[0]->encoding_parameters);
      
    }

  

  }

static int set_video_pass_lqt(void * data, int stream, int pass,
                              int total_passes, const char * stats_file)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  return lqt_set_video_pass(e->file, pass, total_passes, stats_file, stream);
  }

static void set_video_parameter_lqt(void * data, int stream, const char * name,
                                    const bg_parameter_value_t * val)
  {
  e_lqt_t * e = (e_lqt_t*)data;
  
  if(!name)
    return;

  
  if(!strcmp(name, "codec"))
    {
    /* Now we can add the stream */

    e->video_streams[stream].codec_info =
      lqt_find_video_codec_by_name(val->val_str);
    
    if(e->file_type == LQT_FILE_AVI)
      {
      //      e->video_streams[stream].format.image_width *=
      //        e->video_streams[stream].format.pixel_width;
      
      //      e->video_streams[stream].format.image_width /=
      //        e->video_streams[stream].format.pixel_height;

      e->video_streams[stream].format.pixel_width = 1;
      e->video_streams[stream].format.pixel_height = 1;

      //      e->video_streams[stream].format.frame_width =
      //        e->video_streams[stream].format.image_width;
      }
    
    lqt_gavl_add_video_track(e->file, &e->video_streams[stream].format,
                             *e->video_streams[stream].codec_info);
    
    e->video_streams[stream].rows = lqt_gavl_rows_create(e->file, stream);
    }
  else
    {

    bg_lqt_set_video_parameter(e->file,
                               stream,
                               name,
                               val,
                               e->video_streams[stream].codec_info[0]->encoding_parameters);

    }
  }

/* Subtitle parameters */

static bg_parameter_info_t subtitle_text_parameters[] =
  {
    {
      .name =      "box_top",
      .long_name = TRS("Text box (top)"),
      .type =      BG_PARAMETER_INT,
      .val_min =   { .val_i = 0 },
      .val_max =   { .val_i = 0xffff },
    },
    {
      .name =      "box_left",
      .long_name = TRS("Text box (left)"),
      .type =      BG_PARAMETER_INT,
      .val_min =   { .val_i = 0 },
      .val_max =   { .val_i = 0xffff },
    },
    {
      .name =      "box_bottom",
      .long_name = TRS("Text box (bottom)"),
      .type =      BG_PARAMETER_INT,
      .val_min =   { .val_i = 0 },
      .val_max =   { .val_i = 0xffff },
    },
    {
      .name =      "box_right",
      .long_name = TRS("Text box (right)"),
      .type =      BG_PARAMETER_INT,
      .val_min =   { .val_i = 0 },
      .val_max =   { .val_i = 0xffff },
    },
    {
      .name =        "fg_color",
      .long_name =   TRS("Text color"),
      .type =        BG_PARAMETER_COLOR_RGBA,
      .val_default = { .val_color = { 1.0, 1.0, 1.0, 1.0 }},
    },
    {
      .name =        "bg_color",
      .long_name =   TRS("Background color"),
      .type =        BG_PARAMETER_COLOR_RGBA,
      .val_default = { .val_color = { 0.0, 0.0, 0.0, 1.0 }},
    },
    { /* End of parameters */ },
  };

static bg_parameter_info_t * get_subtitle_text_parameters_lqt(void * priv)
  {
  return subtitle_text_parameters;
  }

static void set_subtitle_text_parameter_lqt(void * priv, int stream,
                                            const char * name,
                                            const bg_parameter_value_t * val)
  {
  e_lqt_t * e = (e_lqt_t*)priv;
  
  if(!name)
    return;
  
  if(!strcmp(name, "box_top"))
    e->subtitle_text_streams[stream].text_box[0] = val->val_i;
  else if(!strcmp(name, "box_left"))
    e->subtitle_text_streams[stream].text_box[1] = val->val_i;
  else if(!strcmp(name, "box_bottom"))
    e->subtitle_text_streams[stream].text_box[2] = val->val_i;
  else if(!strcmp(name, "box_right"))
    e->subtitle_text_streams[stream].text_box[3] = val->val_i;
  else if(!strcmp(name, "fg_color"))
    {
    e->subtitle_text_streams[stream].fg_color[0] = (int)(val->val_color[0] * 65535.0 + 0.5);
    e->subtitle_text_streams[stream].fg_color[1] = (int)(val->val_color[1] * 65535.0 + 0.5);
    e->subtitle_text_streams[stream].fg_color[2] = (int)(val->val_color[2] * 65535.0 + 0.5);
    e->subtitle_text_streams[stream].fg_color[3] = (int)(val->val_color[3] * 65535.0 + 0.5);
    }
  else if(!strcmp(name, "bg_color"))
    {
    e->subtitle_text_streams[stream].bg_color[0] = (int)(val->val_color[0] * 65535.0 + 0.5);
    e->subtitle_text_streams[stream].bg_color[1] = (int)(val->val_color[1] * 65535.0 + 0.5);
    e->subtitle_text_streams[stream].bg_color[2] = (int)(val->val_color[2] * 65535.0 + 0.5);
    e->subtitle_text_streams[stream].bg_color[3] = (int)(val->val_color[3] * 65535.0 + 0.5);
    }
  }

bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "e_lqt",       /* Unique short name */
      .long_name =      TRS("Quicktime encoder"),
      .description =    TRS("Encoder based on libquicktime (http://libquicktime.sourceforge.net)\
 Writes Quicktime, AVI (optionally ODML), MP4, M4A and 3GPP. Supported codecs range from \
high quality uncompressed formats for professional applications to consumer level formats \
like H.264/AVC, AAC, MP3, Divx compatible etc. Also supported are chapters and text subtitles"),
      .mimetypes =      NULL,
      .extensions =     "mov",
      .type =           BG_PLUGIN_ENCODER,
      .flags =          BG_PLUGIN_FILE,
      .priority =       BG_PLUGIN_PRIORITY_MAX,
      .create =         create_lqt,
      .destroy =        destroy_lqt,
      .get_parameters = get_parameters_lqt,
      .set_parameter =  set_parameter_lqt,
    },
    
    .max_audio_streams =         -1,
    .max_video_streams =         -1,
    .max_subtitle_text_streams = -1,

    .get_audio_parameters =         get_audio_parameters_lqt,
    .get_video_parameters =         get_video_parameters_lqt,
    .get_subtitle_text_parameters = get_subtitle_text_parameters_lqt,

    .get_extension =        get_extension_lqt,
    
    .open =                 open_lqt,

    .add_audio_stream =     add_audio_stream_lqt,
    
    .add_subtitle_text_stream = add_subtitle_text_stream_lqt,

    .add_video_stream =     add_video_stream_lqt,
    .set_video_pass =       set_video_pass_lqt,
    
    .set_audio_parameter =          set_audio_parameter_lqt,
    .set_video_parameter =          set_video_parameter_lqt,
    .set_subtitle_text_parameter =  set_subtitle_text_parameter_lqt,
    
    .get_audio_format =     get_audio_format_lqt,
    .get_video_format =     get_video_format_lqt,

    .start =                start_lqt,
    
    .write_audio_frame =    write_audio_frame_lqt,
    .write_video_frame =    write_video_frame_lqt,
    .write_subtitle_text =  write_subtitle_text_lqt,
    .close =                close_lqt,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
