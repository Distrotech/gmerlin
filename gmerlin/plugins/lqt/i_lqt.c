/*****************************************************************
 
  i_lqt.c
 
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

#include <string.h>
#include <ctype.h>
#include <plugin.h>
#include <utils.h>

#include "lqt_common.h"

#define PARAM_AUDIO 1
#define PARAM_VIDEO 3

static bg_parameter_info_t parameters[] = 
  {
    {
      name:      "audio",
      long_name: "Audio",
      type:      BG_PARAMETER_SECTION,
    },
    {
      name:      "audio_codecs",
      long_name: "Audio Codecs",
    },
    {
      name:      "video",
      long_name: "Video",
      type:      BG_PARAMETER_SECTION,
    },
    {
      name:      "video_codecs",
      long_name: "Video Codecs",
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
    int64_t total_samples;
    int64_t sample_position;
    } * audio_streams;
  
  struct
    {
    int quicktime_index;
    
    unsigned char ** rows;

    int64_t total_frames;
    int64_t frame_position;
    
    } * video_streams;
  
  } i_lqt_t;

static void * create_lqt()
  {
  i_lqt_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static int open_lqt(void * data, const char * arg)
  {
  char * tmp_string;
  int i;
  char * filename;
  gavl_time_t duration;
  gavl_audio_format_t * audio_format;
  gavl_video_format_t * video_format;
  int colormodel;
  int num_audio_streams = 0;
  int num_video_streams = 0;
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
        
        audio_format =
          &e->track_info.audio_streams[e->track_info.num_audio_streams].format;

        audio_format->num_channels = quicktime_track_channels(e->file, i);
        audio_format->samplerate   = quicktime_sample_rate(e->file, i);
        audio_format->sample_format = GAVL_SAMPLE_FLOAT;
        audio_format->interleave_mode = GAVL_INTERLEAVE_NONE;
        audio_format->samples_per_frame = 1024;
        
        gavl_set_channel_setup(audio_format);

        //        gavl_audio_format_dump(audio_format);

        e->audio_streams[e->track_info.num_audio_streams].total_samples
          = quicktime_audio_length(e->file, i);
        
        duration =
          gavl_samples_to_time(audio_format->samplerate,
                               e->audio_streams[e->track_info.num_audio_streams].total_samples);

        //        fprintf(stderr, "Audio Duration: %lld %d %lld\n", duration,
        //                audio_format->samplerate,
        //                audio_length);
        if(e->track_info.duration < duration)
          e->track_info.duration = duration;
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
        
        video_format =
          &e->track_info.video_streams[e->track_info.num_video_streams].format;

        video_format->image_width = quicktime_video_width(e->file, i);
        video_format->image_height = quicktime_video_height(e->file, i);

        video_format->frame_width = video_format->image_width;
        video_format->frame_height = video_format->image_height;

        video_format->pixel_width = 1;
        video_format->pixel_height = 1;
        
        
        video_format->timescale = lqt_video_time_scale(e->file, i);

        video_format->frame_duration =
          lqt_frame_duration(e->file, i, &(video_format->free_framerate));
        video_format->free_framerate = !video_format->free_framerate;

        colormodel = lqt_get_best_colormodel(e->file, i, bg_lqt_supported_colormodels);

        lqt_set_cmodel(e->file, i, colormodel);
        video_format->colorspace = bg_lqt_get_gavl_colorspace(colormodel);

        if(!gavl_colorspace_is_planar(video_format->colorspace))
          {
          e->video_streams[e->track_info.num_video_streams].rows =
            calloc(video_format->frame_height,
                   sizeof(*(e->video_streams[e->track_info.num_video_streams].rows)));
          }

        e->video_streams[e->track_info.num_video_streams].total_frames =
          quicktime_video_length(e->file, i);
          
        duration = gavl_samples_to_time(lqt_video_time_scale(e->file, i),
                                        lqt_video_duration(e->file, i));
#if 0
        fprintf(stderr, "Video Duration: %d %lld %lld, Total frames: %lld\n",
                lqt_video_time_scale(e->file, i),
                lqt_video_duration(e->file, i),
                duration,
                e->video_streams[e->track_info.num_video_streams].total_frames);
#endif
        if(e->track_info.duration < duration)
          e->track_info.duration = duration;
        
        e->track_info.num_video_streams++;
        }
      }
    }
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
  int samples_to_read;
  i_lqt_t * e = (i_lqt_t*)data;

  //  fprintf(stderr, "read audio\n");
  
  if(e->audio_streams[stream].sample_position + num_samples > 
     e->audio_streams[stream].total_samples)
    {
    samples_to_read = e->audio_streams[stream].total_samples -
      e->audio_streams[stream].sample_position;
    }
  else
    samples_to_read = num_samples;

  if(samples_to_read <= 0)
    return 0;
  
  if(e->track_info.audio_streams[stream].format.sample_format == GAVL_SAMPLE_FLOAT)
    lqt_decode_audio_track(e->file,
                           NULL,
                           f->channels.f,
                           samples_to_read,
                           e->audio_streams[stream].quicktime_index);

  e->audio_streams[stream].sample_position += samples_to_read;
  
  if(f)
    f->valid_samples = samples_to_read;
  return samples_to_read;
  }


/* Read one video frame (returns FALSE on EOF) */
static
int read_video_frame_lqt(void * data, gavl_video_frame_t * f, int stream)
  {
  int i;
  i_lqt_t * e = (i_lqt_t*)data;

  //  fprintf(stderr, "read video\n");

  if(e->video_streams[stream].frame_position >=
     e->video_streams[stream].total_frames)
    return 0;
  e->video_streams[stream].frame_position++;
    
  f->time_scaled =
    lqt_frame_time(e->file, e->video_streams[stream].quicktime_index);

  f->time =
    gavl_samples_to_time(e->track_info.video_streams[stream].format.timescale,
                         f->time_scaled);
  
  if(e->video_streams[stream].rows)
    {
    for(i = 0; i < e->track_info.video_streams[stream].format.frame_height; i++)
      e->video_streams[stream].rows[i] = f->planes[0] + i * f->strides[0];
    lqt_set_row_span(e->file, e->video_streams[stream].quicktime_index,
                     f->strides[0]);
    
    lqt_decode_video(e->file, e->video_streams[stream].rows,
                     e->video_streams[stream].quicktime_index);
    }
  else
    {
    lqt_set_row_span(e->file, e->video_streams[stream].quicktime_index,
                     f->strides[0]);
    lqt_set_row_span_uv(e->file, e->video_streams[stream].quicktime_index,
                        f->strides[1]);
    lqt_decode_video(e->file, f->planes,
                     e->video_streams[stream].quicktime_index);
    }
  return 1;
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

static void seek_lqt(void * data, gavl_time_t time)
  {
  int i;
  int64_t position;
  
  i_lqt_t * e = (i_lqt_t*)data;
  
  for(i = 0; i < e->track_info.num_audio_streams; i++)
    {
    position = gavl_time_to_samples(e->track_info.audio_streams[i].format.samplerate,
                                    time);
    quicktime_set_audio_position(e->file, position,
                                 e->audio_streams[i].quicktime_index);
    e->audio_streams[i].sample_position = position;
    }
  
  for(i = 0; i < e->track_info.num_video_streams; i++)
    {
    position = gavl_time_to_samples(e->track_info.video_streams[i].format.timescale,
                                    time);
    lqt_seek_video(e->file, e->video_streams[i].quicktime_index,
                   position);

    e->video_streams[i].frame_position =
      quicktime_video_position(e->file,
                               e->video_streams[i].quicktime_index);
    }

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

static void set_parameter_lqt(void * data, char * name,
                              bg_parameter_value_t * val)
  {
  i_lqt_t * e = (i_lqt_t*)data;

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
  
  }

bg_input_plugin_t the_plugin =
  {
    common:
    {
      name:            "i_lqt",       /* Unique short name */
      long_name:       "libquicktime input plugin",
      mimetypes:       NULL,
      extensions:      "mov",
      type:            BG_PLUGIN_INPUT,
      flags:           BG_PLUGIN_FILE,
      create:          create_lqt,
      destroy:         destroy_lqt,
      get_parameters:  get_parameters_lqt,
      set_parameter:   set_parameter_lqt,
    },
    
    open:              open_lqt,
    get_num_tracks:    get_num_tracks_lqt,
    get_track_info:    get_track_info_lqt,
    //    set_audio_stream:  set_audio_stream_lqt,
    //    set_video_stream:  set_audio_stream_lqt,
    //    start:             start_lqt,

    read_audio_samples: read_audio_samples_lqt,
    read_video_frame:   read_video_frame_lqt,
    seek:               seek_lqt,
    //    stop:               stop_lqt,
    close:              close_lqt
    
  };
