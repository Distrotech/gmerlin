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
  } stream_t;

typedef struct
  {
  stream_t com;

  gavl_audio_converter_t * cnv;
  gavl_audio_frame_t * in_frame;
  gavl_audio_frame_t * out_frame;
  int do_convert;
  } audio_stream_t;

typedef struct
  {
  stream_t com;
  
  gavl_video_converter_t * cnv;
  gavl_video_frame_t * in_frame;
  gavl_video_frame_t * out_frame;
  int do_convert;
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

bg_transcoder_t * bg_transcoder_create()
  {
  bg_transcoder_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  return ret;
  }

const bg_transcoder_status_t * bg_transcoder_get_status(bg_transcoder_t * t)
  {
  return &(t->status);
  }

void bg_transcoder_set_input(bg_transcoder_t * t, bg_plugin_handle_t * h, int track)
  {
  int i;

  t->input_handle = h;
  t->input_plugin  = (bg_input_plugin_t*)(h->plugin);
  t->track_info    = t->input_plugin->get_track_info(h->priv, track);

  if(t->track_info->num_audio_streams)
    {
    t->num_audio_streams = t->track_info->num_audio_streams;
    t->audio_streams = calloc(t->num_audio_streams, sizeof(*(t->audio_streams)));
    
    for(i = 0; i < t->track_info->num_audio_streams; i++)
      {
      t->audio_streams[i].com.type = STREAM_AUDIO;
      }
    }
  
  }

/*
 *  Set the output plugins for audio and video streams
 *  You can pass the same handle in multiple function calls.
 *  The streams on the output side must be initialized by the caller.
 *  The formats for the output must be passed separately.
 */

void bg_transcoder_set_audio_stream(bg_transcoder_t * t,
                                    bg_plugin_handle_t * encoder,
                                    int in_index, int out_index,
                                    gavl_audio_format_t * format)
  {

  }

void bg_transcoder_set_video_stream(bg_transcoder_t * t,
                                    bg_plugin_handle_t * encoder,
                                    int in_index, int out_index,
                                    gavl_video_format_t * format)
  {
  
  }

/*
 *  Do one iteration (Will be called as an idle function in the GUI main loop)
 *  If return value is FALSE, we are done
 */

int bg_transcoder_iteration(bg_transcoder_t * t)
  {
  return 0;
  }
