/*****************************************************************
 
  avdec_common.h
 
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

typedef struct
  {
  bg_track_info_t * track_info;
  bg_track_info_t * current_track;
  int num_tracks;
  bgav_t * dec;
  bgav_options_t * opt;
  
  bg_input_callbacks_t * bg_callbacks;
  
  } avdec_priv;

void * bg_avdec_create();

void bg_avdec_close(void * priv);
void bg_avdec_destroy(void * priv);
bg_track_info_t * bg_avdec_get_track_info(void * priv, int track);

int bg_avdec_read_video(void * priv,
                        gavl_video_frame_t * frame,
                        int stream);

int bg_avdec_read_audio(void * priv,
                        gavl_audio_frame_t * frame,
                        int stream,
                        int num_samples);

int bg_avdec_read_subtitle_overlay(void * priv,
                                   gavl_overlay_t * ovl, int stream);
  
int bg_avdec_read_subtitle_text(void * priv,
                                char ** text, int * text_alloc,
                                int64_t * start_time,
                                int64_t * duration,
                                int stream);

int bg_avdec_has_subtitle(void * priv, int stream);

int bg_avdec_set_audio_stream(void * priv,
                                  int stream,
                              bg_stream_action_t action);
int bg_avdec_set_video_stream(void * priv,
                              int stream,
                              bg_stream_action_t action);

int bg_avdec_set_subtitle_stream(void * priv,
                                 int stream,
                                 bg_stream_action_t action);
int bg_avdec_start(void * priv);
void bg_avdec_seek(void * priv, gavl_time_t * t);

int bg_avdec_init(avdec_priv * avdec);


void
bg_avdec_set_parameter(void * p, char * name,
                       bg_parameter_value_t * val);
int bg_avdec_get_num_tracks(void * p);

int bg_avdec_set_track(void * priv, int track);

void bg_avdec_set_callbacks(void * priv,
                            bg_input_callbacks_t * callbacks);

const char * bg_avdec_get_error(void * priv);

bg_device_info_t * bg_avdec_get_devices(bgav_device_info_t *);
