/*****************************************************************
 
  i_vorbis.c
 
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

#include <stdlib.h>
#include <string.h>

#include <plugin.h>
#include <utils.h>

#include <vorbis/vorbisfile.h>

typedef struct vorbis_priv_s
  {
  OggVorbis_File vorbisfile; 
  bg_track_info_t * track_info;

  int num_bitstreams;
  int current_bitstream;
  
  int64_t * start_positions; /* Start positions for each bitstream */
  
  int streams_are_tracks_cfg;
  int streams_are_tracks;
  
  int bigendianp;
  int is_open;

  int64_t position;
    
  /* We can read integer and floating point audio */
    
  int (*read_audio_func)(struct vorbis_priv_s * p, gavl_audio_frame_t * frame,
                         int stream,int num_samples);

  } vorbis_priv;

static void * create_vorbis()
  {
  vorbis_priv * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_vorbis(void * data)
  {
  vorbis_priv * p;
  p = (vorbis_priv *)data;
  free(p);
  }

static char * _artist_key = "ARTIST=";
static char * _album_key = "ALBUM=";
static char * _title_key = "TITLE=";
// static char * _version_key = "VERSION=";
static char * _track_number_key = "TRACKNUMBER=";
// static char * _organization_key = "ORGANIZATION=";
static char * _genre_key = "GENRE=";
// static char * _description_key = "DESCRIPTION=";
// static char * _date_key = "DATE=";
// static char * _location_key = "LOCATION=";
// static char * _copyright_key = "COPYRIGHT=";

static int read_audio_int16(vorbis_priv * p, gavl_audio_frame_t * frame,
                            int stream,int num_samples)
  {
  int bitstream;
  long bytes_read;
  long total_bytes = 0;
  
  int num_channels =
    p->track_info[p->current_bitstream].audio_streams[0].format.num_channels;
  while(frame->valid_samples < num_samples)
    {
    bytes_read =
      ov_read(&(p->vorbisfile), &(frame->samples.s_8[total_bytes]),
              num_samples * 2 * num_channels - total_bytes, p->bigendianp,
              2, 1, &bitstream);
    total_bytes += bytes_read;
    frame->valid_samples += bytes_read / (2 * num_channels);
    }
  return 1;
  }
  
static int read_audio_float(vorbis_priv * p, gavl_audio_frame_t * frame,
                            int stream,int num_samples)
  {
  int bitstream;
  int samples_read = 0;
  float ** pcm_channels;
  int i;
  int num_channels =
    p->track_info[p->current_bitstream].audio_streams[0].format.num_channels;
  while(frame->valid_samples < num_samples)
    {
    samples_read =
      ov_read_float(&(p->vorbisfile), &pcm_channels,
                    num_samples - frame->valid_samples, &bitstream);
    for(i = 0; i < num_channels; i++)
      {
      memcpy(&(frame->channels.f[i][frame->valid_samples]),
             pcm_channels[i],
             samples_read * sizeof(float));
      }
    frame->valid_samples += samples_read;
    }
  return 1;
  }

static int read_audio_vorbis(void * priv, gavl_audio_frame_t * frame,
                             int stream,int num_samples)
  {
  int ret;
  vorbis_priv * p = (vorbis_priv *)priv;
  if(p->position + num_samples > p->start_positions[p->current_bitstream+1])
    {
    /* EOF */
    num_samples = p->start_positions[p->current_bitstream+1] - p->position;
    if(num_samples <= 0)
      return 0;
    }
  frame->valid_samples = 0;
  ret = p->read_audio_func(p, frame, stream, num_samples);
  p->position += frame-> valid_samples;
  return ret;
  }

static void seek_vorbis(void * priv, gavl_time_t t)
  {
  vorbis_priv * p = (vorbis_priv *)priv;

  fprintf(stderr, "Vorbis seek...");
  
  p->position =
    gavl_time_to_samples(p->track_info[p->current_bitstream].audio_streams[0].format.samplerate,
                         t);
  ov_pcm_seek(&(p->vorbisfile), p->position);
  fprintf(stderr, "Done\n");
  }

static int open_vorbis(void * data, const void * arg)
  {
  vorbis_comment * comment;
  vorbis_info    * info;
  vorbis_priv * p;
  int key_len;
  int i, j;
  int comment_added = 0;
  int seekable;
  p = (vorbis_priv *)data;
  FILE * input_file;
  memset(&(p->vorbisfile), 0, sizeof(p->vorbisfile));
  int64_t stream_duration = 0, last_stream_duration;
  /* Open File */
  
  input_file = fopen((const char*)arg, "r");
  if(!input_file)
    {
    fprintf(stderr, "Cannot open file %s\n", (const char*)arg);
    return 0;
    }

  if(ov_open(input_file, &(p->vorbisfile), NULL, 0))
    {
    fclose(input_file);
    fprintf(stderr, "File %s is no valid Ogg Vorbis stream\n",
            (const char*)arg);
    return 0;
    }

  seekable = ov_seekable(&(p->vorbisfile));
  
  p->num_bitstreams = ov_streams(&(p->vorbisfile));
  p->track_info = calloc(p->num_bitstreams, sizeof(bg_track_info_t));
  p->start_positions = calloc(p->num_bitstreams+1, sizeof(int64_t));

  p->streams_are_tracks = p->streams_are_tracks_cfg;
    
  for(i = 0; i < p->num_bitstreams; i++)
    {
    /* Read metadata */

    comment = ov_comment(&(p->vorbisfile), i);
    comment_added = 0;
    
    if(comment->comments)
      {
      for(j = 0; j < comment->comments; j++)
        {
        key_len = strlen(_artist_key);
        if(!strncasecmp(comment->user_comments[j], _artist_key,
                        key_len))
          {
          p->track_info[i].metadata.artist =
            bg_strdup(p->track_info[i].metadata.artist,
                      &(comment->user_comments[j][key_len]));
          continue;
          }
        key_len = strlen(_album_key);
        if(!strncasecmp(comment->user_comments[j], _album_key,
                        key_len))
          {
          p->track_info[i].metadata.album =
            bg_strdup(p->track_info[i].metadata.album,
                      &(comment->user_comments[j][key_len]));
          continue;
          }
        
        key_len = strlen(_title_key);
        if(!strncasecmp(comment->user_comments[j], _title_key,
                        key_len))
          {
          p->track_info[i].metadata.title =
            bg_strdup(p->track_info[i].metadata.title,
                      &(comment->user_comments[j][key_len]));
          continue;
          }
        
        key_len = strlen(_genre_key);
        if(!strncasecmp(comment->user_comments[j], _genre_key,
                        key_len))
          {
          p->track_info[i].metadata.genre =
            bg_strdup(p->track_info[i].metadata.genre,
                      &(comment->user_comments[j][key_len]));
          continue;
          }
        key_len = strlen(_track_number_key);
        if(!strncasecmp(comment->user_comments[j], _track_number_key,
                        key_len))
          {
          p->track_info[i].metadata.track =
            atoi(&(comment->user_comments[j][key_len]));
          continue;
          }
        
        if(!(comment_added) && !strchr(comment->user_comments[j], '='))
          {
          p->track_info[i].metadata.comment =
            bg_strdup(p->track_info[i].metadata.comment,
                      comment->user_comments[j]);
          comment_added = 1;
          continue;
          }
        }
      }
    
    /* Read audio Format */

    info = ov_info(&(p->vorbisfile), i);

    /* Read length */
    last_stream_duration = stream_duration;
    stream_duration = ov_pcm_total(&(p->vorbisfile), i);
    p->track_info[i].duration =
      gavl_samples_to_time(info->rate, stream_duration);
    
    if(!i)
      {
      p->start_positions[i] = 0;
      }
    else
      {
      p->start_positions[i] = p->start_positions[i-1] + last_stream_duration;
      }
    
    p->track_info[i].num_audio_streams = 1;
    p->track_info[i].audio_streams =
      calloc(1, sizeof(*(p->track_info[i].audio_streams)));
    p->track_info[i].audio_streams[0].format.sample_format
      = GAVL_SAMPLE_FLOAT;
    p->track_info[i].audio_streams[0].format.interleave_mode
      = GAVL_INTERLEAVE_NONE;
    p->read_audio_func = read_audio_float;

    p->track_info[i].audio_streams[0].format.num_channels = info->channels;

    gavl_set_channel_setup(&(p->track_info[i].audio_streams[0].format));
    p->track_info[i].audio_streams[0].format.samplerate = info->rate;
    p->track_info[i].audio_streams[0].format.samples_per_frame = 1024;

    /* Check if we still have the same format */

    if((i) && !(p->streams_are_tracks) &&
       ((p->track_info[i].audio_streams[0].format.samplerate != 
         p->track_info[0].audio_streams[0].format.samplerate) ||
        (p->track_info[i].audio_streams[0].format.num_channels != 
         p->track_info[0].audio_streams[0].format.num_channels)))
      {
      p->streams_are_tracks = 1;
      }
    p->track_info[i].seekable = seekable;
    }
  p->start_positions[p->num_bitstreams] =
    ov_pcm_total(&(p->vorbisfile), -1);
  
  p->is_open = 1;
  return 1;
  }

static bg_track_info_t * get_track_info_vorbis(void * priv, int track)
  {
  vorbis_priv * v;
  v = (vorbis_priv*)(priv);
  return &(v->track_info[track]);
  }

static int get_num_tracks_vorbis(void * priv)
  {
  vorbis_priv * v;
  v = (vorbis_priv*)(priv);
  return v->num_bitstreams;
  }

static int set_track_vorbis(void * priv, int track)
  {
  vorbis_priv * v;
  v = (vorbis_priv*)(priv);

  if((track < 0) || (track >= v->num_bitstreams))
    return 0;
  
  v->current_bitstream = track;
  if(!ov_pcm_seek(&(v->vorbisfile), v->start_positions[track]))
    return 1;
  return 0;
  }

static void close_vorbis(void * priv)
  {
  vorbis_priv * v;
  int i;
  v = (vorbis_priv*)(priv);
  if(v->is_open)
    {
    ov_clear(&(v->vorbisfile));
    free(v->start_positions);
    
    for(i = 0; i < v->num_bitstreams; i++)
      bg_track_info_free(&(v->track_info[i]));
    free(v->track_info);
    v->is_open = 0;
    }
  }

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "streams_are_tracks",
      long_name:   "Handle logical streams as separate tracks",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 }
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_vorbis(void * data)
  {
  return parameters;
  }

static void set_parameter_vorbis(void * priv, char * name,
                                 bg_parameter_value_t * val)
  {
  vorbis_priv * v;
  v = (vorbis_priv*)(priv);

  if(!name)
    return;
  
  else if(!strcmp(name, "streams_are_tracks"))
    v->streams_are_tracks_cfg = val->val_i;
  }

bg_input_plugin_t the_plugin =
  {
    common:
    {
      name:          "i_vorbis",
      long_name:     "Ogg Vorbis plugin",
      mimetypes:     (char*)0,
      extensions:    "ogg",
      type:          BG_PLUGIN_INPUT,
      flags:         BG_PLUGIN_FILE,
      create:        create_vorbis,
      destroy:       destroy_vorbis,
      get_parameters: get_parameters_vorbis,
      set_parameter:  set_parameter_vorbis
    },
    /* Open file/device */
    open: open_vorbis,
    //    set_callbacks: set_callbacks_vorbis,
    /* For file and network plugins, this is NULL */
    
    get_num_tracks: get_num_tracks_vorbis,
    /* Return track information */

    get_track_info: get_track_info_vorbis,

    set_track: set_track_vorbis,
    
    /* Set streams */
    //    set_audio_stream:      bg_vorbis_set_audio_stream,
    
    /*
     *  Start decoding.
     */

    //    start: start_vorbis,
    /* Read one audio frame (returns FALSE on EOF) */
    read_audio_samples: read_audio_vorbis,
        
    /*
     *  Do percentage seeking (can be NULL)
     *  Media streams are supposed to be seekable, if this
     *  function is non-NULL AND the duration field of the track info
     *  is > 0
     */
    seek: seek_vorbis,

    /* Stop playback, close all decoders */
    stop: NULL,
    close: close_vorbis,
  };
