/*****************************************************************
 
  transcoder.c
 
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

#include <pluginregistry.h>
#include <transcoder.h>

#define STREAM_STATUS_OFF      0
#define STREAM_STATUS_ON       1
#define STREAM_STATUS_FINISHED 2

#define STREAM_AUDIO           0
#define STREAM_VIDEO           1

typedef struct
  {
  int status;
  int type;

  int in_index;
  int out_index;
  gavl_time_t time;

  bg_plugin_handle_t * input_handle;
  bg_input_plugin_t  * input_plugin;

  bg_plugin_handle_t  * output_handle;
  bg_encoder_plugin_t * output_plugin;
  int do_convert;
  } stream_t;

typedef struct
  {
  stream_t com;

  gavl_audio_converter_t * cnv;
  gavl_audio_frame_t * in_frame;
  gavl_audio_frame_t * out_frame;
  
  } audio_stream_t;

typedef struct
  {
  stream_t com;
  
  gavl_video_converter_t * cnv;
  gavl_video_frame_t * in_frame;
  gavl_video_frame_t * out_frame;
  } video_stream_t;

struct bg_transcoder_s
  {
  int num_audio_streams;
  int num_video_streams;
  
  audio_stream_t * audio_streams;
  video_stream_t * video_streams;
  
  bg_transcoder_status_t status;
  
  bg_plugin_handle_t * input_handle;
  bg_input_plugin_t  * input_plugin;
  bg_track_info_t * track_info;
  };

static void create_audio_stream(audio_stream_t * ret, bg_transcoder_track_audio_t * s,
                                bg_plugin_handle_t * input_plugin, int input_index,
                                bg_plugin_handle_t * encoder_plugin, int output_index,
                                bg_track_info_t * track_info)
  {
  
  }

static void create_video_stream(video_stream_t * ret, bg_transcoder_track_video_t * s,
                                bg_plugin_handle_t * input_plugin, int input_index,
                                bg_plugin_handle_t * encoder_plugin, int output_index,
                                bg_track_info_t * track_info)
  {
  
  }

bg_transcoder_t * bg_transcoder_create(bg_plugin_registry_t * plugin_reg, bg_transcoder_track_t * track)
  {
  bg_transcoder_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  return ret;
  }

const bg_transcoder_status_t * bg_transcoder_get_status(bg_transcoder_t * t)
  {
  return &(t->status);
  }


/*
 *  Do one iteration (Will be called as an idle function in the GUI main loop)
 *  If return value is FALSE, we are done
 */

int bg_transcoder_iteration(bg_transcoder_t * t)
  {
  int i;
  gavl_time_t time;
  stream_t * stream;

  /* Find the stream with the smallest time */
  
  if(t->audio_streams)
    {
    time = t->audio_streams->com.time;
    stream = &(t->audio_streams->com);
    }
  else if(t->video_streams)
    {
    time = t->video_streams->com.time;
    stream = &(t->video_streams->com);
    }

  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].com.time < time)
      {
      time = t->audio_streams[i].com.time;
      stream = &(t->audio_streams[i].com);
      }
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].com.time < time)
      {
      time = t->video_streams[i].com.time;
      stream = &(t->video_streams[i].com);
      }
    }

  /* Do the transcoding */

  if(stream->type == STREAM_AUDIO)
    {
        
    }
  
  return 0;
  }
