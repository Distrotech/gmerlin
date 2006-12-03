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

#include "transcoder_track.h"

typedef struct bg_transcoder_s bg_transcoder_t;

bg_transcoder_t * bg_transcoder_create();

bg_parameter_info_t * bg_transcoder_get_parameters();

void bg_transcoder_set_parameter(void * priv, char * name, bg_parameter_value_t * val);

int bg_transcoder_init(bg_transcoder_t * t,
                       bg_plugin_registry_t * plugin_reg, bg_transcoder_track_t * track);


const char * bg_transcoder_get_error(bg_transcoder_t * t);

/*
 *  Do one iteration (Will be called as an idle function in the GUI main loop)
 *  If return value is FALSE, we are done
 */

int bg_transcoder_iteration(bg_transcoder_t * t);

void bg_transcoder_destroy(bg_transcoder_t * t);

/* Messages (see transcodermsg.h) */

void bg_transcoder_add_message_queue(bg_transcoder_t * t,
                                     bg_msg_queue_t * message_queue);

/*
 *  Multithread interface
 */

void bg_transcoder_run(bg_transcoder_t * t);
void bg_transcoder_stop(bg_transcoder_t * t);
void bg_transcoder_finish(bg_transcoder_t * t);

/*
 *  Message broadcasting stuff used by both
 *  transcoder and postprocessor
 */

void bg_transcoder_send_msg_num_audio_streams(bg_msg_queue_list_t * l,
                                              int num);
void bg_transcoder_send_msg_num_video_streams(bg_msg_queue_list_t * l,
                                                int num);
void bg_transcoder_send_msg_audio_format(bg_msg_queue_list_t * l,
                                         int index,
                                         gavl_audio_format_t * input_format,
                                         gavl_audio_format_t * output_format);

void bg_transcoder_send_msg_video_format(bg_msg_queue_list_t * l,
                                         int index,
                                         gavl_video_format_t * input_format,
                                         gavl_video_format_t * output_format);

void bg_transcoder_send_msg_audio_file(bg_msg_queue_list_t * l,
                                       int index,
                                       const char * filename, int pp_only);

void bg_transcoder_send_msg_video_file(bg_msg_queue_list_t * l,
                                       int index,
                                       const char * filename, int pp_only);

void bg_transcoder_send_msg_file(bg_msg_queue_list_t * l,
                                 const char * filename, int pp_only);

void bg_transcoder_send_msg_progress(bg_msg_queue_list_t * l,
                                     float percentage_done,
                                     gavl_time_t remaining_time);

void bg_transcoder_send_msg_finished(bg_msg_queue_list_t * l);

void bg_transcoder_send_msg_start(bg_msg_queue_list_t * l, char * what);
void bg_transcoder_send_msg_error(bg_msg_queue_list_t * l, char * msg);

void bg_transcoder_send_msg_metadata(bg_msg_queue_list_t * l, bg_metadata_t * m);


/*
 *  Transcoder postprocessing.
 *  
 *  It's a module, which collects informations about transcoded tracks
 *  for doing something else with them after. (e.g. burning onto a CD).
 */

typedef struct bg_transcoder_pp_s bg_transcoder_pp_t;

bg_transcoder_pp_t * bg_transcoder_pp_create();
void bg_transcoder_pp_destroy(bg_transcoder_pp_t*);

int bg_transcoder_pp_init(bg_transcoder_pp_t*, bg_plugin_handle_t * pp_plugin);
void bg_transcoder_pp_update(bg_transcoder_pp_t * p);

void bg_transcoder_pp_connect(bg_transcoder_pp_t*,bg_transcoder_t*);

void bg_transcoder_pp_run(bg_transcoder_pp_t*);
void bg_transcoder_pp_stop(bg_transcoder_pp_t*);

void bg_transcoder_pp_finish(bg_transcoder_pp_t * t);

void bg_transcoder_pp_set_parameter(void * priv, char * name, bg_parameter_value_t * val);
void bg_transcoder_pp_add_message_queue(bg_transcoder_pp_t * p,
                                        bg_msg_queue_t * message_queue);
