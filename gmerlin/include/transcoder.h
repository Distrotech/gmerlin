/*****************************************************************
 
  transcoder.h

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

/*
 *  VERY simple transcoder class.
 *  You give readily initialized plugins
 *  to the transcoder and it will transcode the streams
 */

typedef struct bg_transcoder_s bg_transcoder_t;

typedef struct
  {
  float percentage_done;
  gavl_time_t remaining_time;
  } bg_transcoder_status_t;

bg_transcoder_t * bg_transcoder_create();
const bg_transcoder_status_t * bg_transcoder_get_status(bg_transcoder_t * t);

void bg_transcoder_set_input(bg_transcoder_t * t, bg_plugin_handle_t * h, int track);

/*
 *  Set the output plugins for audio and video streams
 *  You can pass the same handle in multiple function calls.
 *  The streams on the output side must be initialized by the caller.
 *  The formats for the output must be passed separately.
 */

void bg_transcoder_set_audio_stream(bg_transcoder_t * t,
                                    bg_plugin_handle_t * encoder,
                                    int in_index, int out_index,
                                    gavl_audio_format_t * format);
  

void bg_transcoder_set_video_stream(bg_transcoder_t * t,
                                    bg_plugin_handle_t * encoder,
                                    int in_index, int out_index,
                                    gavl_video_format_t * format);

/*
 *  Do one iteration (Will be called as an idle function in the GUI main loop)
 *  If return value is FALSE, we are done
 */

int bg_transcoder_iteration(bg_transcoder_t * t);
