/*****************************************************************
 
  playerprivate.h
 
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

#include <pthread.h>
#include <pluginregistry.h>
#include <fifo.h>
#include <utils.h>
#include <bggavl.h>
#include <textrenderer.h>

/* Each thread get it's private context */

typedef struct bg_player_input_context_s bg_player_input_context_t;
typedef struct bg_player_ov_context_s    bg_player_ov_context_t;
typedef struct bg_player_oa_context_s    bg_player_oa_context_t;

typedef enum
  {
    SYNC_SOFTWARE,
    SYNC_SOUNDCARD,
    SYNC_INPUT,
  } bg_player_sync_mode_t;

/* Stream structures */

typedef struct
  {
  gavl_audio_converter_t * cnv;
  bg_fifo_t * fifo;
  int do_convert;
  gavl_audio_frame_t * frame;

  pthread_mutex_t config_mutex;

  bg_gavl_audio_options_t options;

  //  gavl_audio_options_t opt;
  //  int fixed_channel_setup;
  //  int fixed_samplerate;
  //  gavl_channel_setup_t channel_setup;
  //  int samplerate;
  
  gavl_audio_format_t input_format;
  gavl_audio_format_t output_format;
  const char * error_msg;
  } bg_player_audio_stream_t;

typedef struct
  {
  gavl_video_converter_t * cnv;
  bg_fifo_t * fifo;

  pthread_mutex_t config_mutex;

  bg_gavl_video_options_t options;

  //  gavl_video_options_t opt;
  int do_convert;
  gavl_video_frame_t * frame;

  gavl_video_format_t input_format;
  gavl_video_format_t output_format;
  const char * error_msg;
  } bg_player_video_stream_t;

typedef struct
  {
  bg_text_renderer_t * renderer;
  gavl_video_converter_t * cnv;
  
  bg_fifo_t * fifo;

  pthread_mutex_t config_mutex;

  gavl_video_format_t format;
  const char * error_msg;

  char * buffer;
  int buffer_alloc;
  
  } bg_player_subtitle_stream_t;

/* The player */

struct bg_player_s
  {
  pthread_t input_thread;
  pthread_t ov_thread;
  pthread_t oa_thread;
  pthread_t player_thread;

  /* Private contexts defined in .c files */
    
  bg_player_input_context_t * input_context;
  bg_player_oa_context_t    * oa_context;
  bg_player_ov_context_t    * ov_context;
  
  /* Stream Infos */
    
  bg_player_audio_stream_t    audio_stream;
  bg_player_video_stream_t    video_stream;
  bg_player_subtitle_stream_t subtitle_stream;
  
  /* What is currently played? */
  
  bg_track_info_t * track_info;

  /*
   *  Stream selection
   */
    
  int do_audio;

  /* Only one of do_still and do_video can be nonzero at the same time */
  int do_video;
  int do_still;
  
  int do_subtitle_overlay;
  int do_subtitle_text;
  
  int current_audio_stream;
  int current_video_stream;
  int current_still_stream;
  int current_subtitle_stream;
  
  /* Can we seek? */

  int can_seek;
  int do_bypass; /* Bypass mode */
  
  /* Message stuff */

  bg_msg_queue_t      * command_queue;
  bg_msg_queue_list_t * message_queues;

  bg_msg_queue_t * to_oa_queue;
  bg_msg_queue_t * to_ov_queue;
  bg_msg_queue_t * to_input_queue;

  /* Inernal state variables */

  int state;
  pthread_mutex_t state_mutex;

  /* Stuff for synchronous stopping and starting of the playback */

  pthread_cond_t  start_cond;
  pthread_mutex_t start_mutex;

  pthread_cond_t  stop_cond;
  pthread_mutex_t stop_mutex;

  int             waiting_plugin_threads;
  pthread_mutex_t waiting_plugin_threads_mutex;
  
  int total_plugin_threads;
  bg_plugin_handle_t * input_handle;

  float volume; /* Current volume in dB (0 == max) */

  
  };

int  bg_player_get_state(bg_player_t * player);
void bg_player_set_state(bg_player_t * player, int state, const void * arg1,
                         const void * arg2);

/* This function has 3 possible results:
   1. It returns TRUE immediately, meaning that normal playback
      is running
   2. It returnas FALSE immediately, meaning that playback should be stopped
   3. If playback is paused (or during a sek operation), this function blocks
      until playback can continue again
 */

int bg_player_keep_going(bg_player_t * player, void (*ping_func)(void*), void * data);

/* Get the current time (thread save) */

void bg_player_time_get(bg_player_t * player, int exact, gavl_time_t * ret);
void bg_player_time_stop(bg_player_t * player);
void bg_player_time_start(bg_player_t * player);
void bg_player_time_init(bg_player_t * player);
void bg_player_time_reset(bg_player_t * player);
void bg_player_time_set(bg_player_t * player, gavl_time_t time);

/* player1_input.c */

void bg_player_input_create(bg_player_t * player);
void bg_player_input_destroy(bg_player_t * player);

void * bg_player_input_thread(void *);
void * bg_player_input_thread_bypass(void *);

int bg_player_input_init(bg_player_input_context_t * ctx,
                         bg_plugin_handle_t * handle,
                         int track_index);

const char * bg_player_input_get_error(bg_player_input_context_t * ctx);

void bg_player_input_cleanup(bg_player_input_context_t * ctx);

void bg_player_input_preload(bg_player_input_context_t * ctx);

int
bg_player_input_set_audio_stream(bg_player_input_context_t * ctx,
                                 int audio_stream);
int
bg_player_input_set_video_stream(bg_player_input_context_t * ctx,
                                 int video_stream);

int
bg_player_input_set_subtitle_stream(bg_player_input_context_t * ctx,
                                    int subtitle_stream);

int
bg_player_input_set_still_stream(bg_player_input_context_t * ctx,
                                 int still_stream);

void bg_player_input_seek(bg_player_input_context_t * ctx,
                          gavl_time_t * time);

void bg_player_input_bypass_set_volume(bg_player_input_context_t * ctx,
                                       float volume);

void bg_player_input_bypass_set_pause(bg_player_input_context_t * ctx,
                                      int pause);



/* player1_ov.c */

void bg_player_ov_create(bg_player_t * player);
void bg_player_ov_destroy(bg_player_t * player);
int  bg_player_ov_init(bg_player_ov_context_t * ctx);
void bg_player_ov_cleanup(bg_player_ov_context_t * ctx);
void * bg_player_ov_thread(void *);
void * bg_player_ov_still_thread(void *);

/* Update still image: To be called during pause */
void bg_player_ov_update_still(bg_player_ov_context_t * ctx);

void bg_player_ov_standby(bg_player_ov_context_t * ctx);

/* Create/destroy frames */
void * bg_player_ov_create_frame(void * data);
void bg_player_ov_destroy_frame(void * data, void * frame);

/* Set this extra because we must initialize subtitles after the video output */
void bg_player_ov_set_subtitle_format(void * data, const gavl_video_format_t * format);

void bg_player_ov_set_plugin(bg_player_t * player,
                             bg_plugin_handle_t * handle);

const char * bg_player_ov_get_error(bg_player_ov_context_t * ctx);

/*
 *  This call will let the video plugin adjust the playertime from the
 *  next frame to be displayed
 */
void bg_player_ov_sync(bg_player_t * p);

/* player_video.c */

int bg_player_video_init(bg_player_t * p, int video_stream);
void bg_player_video_cleanup(bg_player_t * p);
void bg_player_video_create(bg_player_t * p);
void bg_player_video_destroy(bg_player_t * p);


/* player_oa.c */

void * bg_player_oa_create_frame(void * data);
void bg_player_oa_destroy_frame(void * data, void * frame);

void bg_player_oa_create(bg_player_t * player);
void bg_player_oa_destroy(bg_player_t * player);

int bg_player_oa_init(bg_player_oa_context_t * ctx);
int bg_player_oa_start(bg_player_oa_context_t * ctx);
void bg_player_oa_stop(bg_player_oa_context_t * ctx);
const char * bg_player_oa_get_error(bg_player_oa_context_t * ctx);

void bg_player_oa_cleanup(bg_player_oa_context_t * ctx);
void * bg_player_oa_thread(void *);

void bg_player_oa_set_plugin(bg_player_t * player,
                             bg_plugin_handle_t * handle);

void bg_player_oa_set_volume(bg_player_oa_context_t * ctx,
                             float volume);


/*
 *  Audio output must be locked, since the sound latency is
 *  obtained from here by other threads
 */

int  bg_player_oa_get_latency(bg_player_oa_context_t * ctx);

/* player_audio.c */

int  bg_player_audio_init(bg_player_t * p, int audio_stream);
void bg_player_audio_cleanup(bg_player_t * p);

void bg_player_audio_create(bg_player_t * p);
void bg_player_audio_destroy(bg_player_t * p);

/* player_subtitle.c */

int bg_player_subtitle_init(bg_player_t * player, int subtitle_stream);
void bg_player_subtitle_cleanup(bg_player_t * p);

void bg_player_subtitle_create(bg_player_t * p);
void bg_player_subtitle_destroy(bg_player_t * p);

/* Number of frames in the buffers */

#define NUM_AUDIO_FRAMES 8
#define NUM_VIDEO_FRAMES 8

