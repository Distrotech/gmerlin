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

#include <pthread.h>
#include <config.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/fifo.h>
#include <gmerlin/utils.h>
#include <gmerlin/bggavl.h>
#include <gmerlin/textrenderer.h>
#include <gmerlin/osd.h>
#include <gmerlin/converters.h>
#include <gmerlin/filters.h>
#include <gmerlin/visualize.h>

#include <gmerlin/translation.h>

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

typedef enum
  {
    TIME_UPDATE_SECOND = 0,
    TIME_UPDATE_FRAME,
  } bg_player_time_update_mode_t;


/* Stream structures */

typedef struct
  {
  gavl_audio_converter_t * cnv_out;
  bg_fifo_t * fifo;

  bg_audio_filter_chain_t * fc;
  
  bg_read_audio_func_t in_func;
  void * in_data;
  int    in_stream;
  
  int do_convert_out;
  
  gavl_audio_frame_t * frame_out;
  
  pthread_mutex_t config_mutex;

  bg_gavl_audio_options_t options;

  //  gavl_audio_options_t opt;
  //  int fixed_channel_setup;
  //  int fixed_samplerate;
  //  gavl_channel_setup_t channel_setup;
  //  int samplerate;
  
  gavl_audio_format_t input_format;
  gavl_audio_format_t output_format;
  gavl_audio_format_t fifo_format;

  /* Volume control */
  gavl_volume_control_t * volume;
  pthread_mutex_t volume_mutex;

  gavl_peak_detector_t * peak_detector;
  
  /* If the playback was interrupted due to changed parameters */
  int interrupted;
  } bg_player_audio_stream_t;

typedef struct
  {
  bg_fifo_t * fifo;
  
  bg_video_filter_chain_t * fc;
  
  bg_read_video_func_t in_func;
  void * in_data;
  int    in_stream;
    
  pthread_mutex_t config_mutex;

  bg_gavl_video_options_t options;
  
  float still_framerate;
  int64_t still_pts;
  int still_pts_inc;
    
  gavl_video_format_t input_format;
  gavl_video_format_t output_format;

  /* If the playback was interrupted due to changed parameters */
  int interrupted;
  
  } bg_player_video_stream_t;

typedef struct
  {
  bg_text_renderer_t * renderer;
  gavl_video_converter_t * cnv;
  int do_convert;
  
  bg_fifo_t * fifo;

  pthread_mutex_t config_mutex;
  
  gavl_video_format_t input_format;
  gavl_video_format_t output_format;
  
  char * buffer;
  int buffer_alloc;
  
  gavl_overlay_t in_ovl;
  } bg_player_subtitle_stream_t;

typedef struct
  {
  int playing;
  gavl_time_t time;
  } bg_player_saved_state_t;

/* Player flags */

#define PLAYER_DO_AUDIO            (1<<0)
#define PLAYER_DO_VIDEO            (1<<1)
#define PLAYER_DO_SUBTITLE         (1<<2) /* Set by open() */
#define PLAYER_DO_SUBTITLE_OVERLAY (1<<3) /* Set by start() */
#define PLAYER_DO_SUBTITLE_TEXT    (1<<4) /* Set by start() */
#define PLAYER_DO_SUBTITLE_ONLY    (1<<5)
#define PLAYER_DO_VISUALIZE        (1<<6)
#define PLAYER_DO_REPORT_PEAK      (1<<16)

#define DO_SUBTITLE_TEXT(f) \
 (f & PLAYER_DO_SUBTITLE_TEXT)

#define DO_SUBTITLE_OVERLAY(f) \
 (f & PLAYER_DO_SUBTITLE_OVERLAY)

#define DO_SUBTITLE(f) \
 (f & (PLAYER_DO_SUBTITLE))

#define DO_SUBTITLE_ONLY(f) \
  (f & PLAYER_DO_SUBTITLE_ONLY)

#define DO_AUDIO(f) \
  (f & PLAYER_DO_AUDIO)

#define DO_VIDEO(f) \
  (f & PLAYER_DO_VIDEO)

#define DO_VISUALIZE(f) \
 (f & PLAYER_DO_VISUALIZE)

#define DO_PEAK(f) \
 (f & PLAYER_DO_REPORT_PEAK)

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
   *  Visualizer: Initialized and cleaned up by the player
   *  core, run by the audio thread
   */
  
  bg_visualizer_t * visualizer;
  
  /*
   *  Stream selection
   */
  
  /*
   * Flags are set by bg_player_input_select_streams() and
   * bg_player_input_start()
   */
  
  int flags;
  int old_flags; /* Flags from previous playback */
  
  int current_audio_stream;
  int current_video_stream;
  int current_still_stream;
  int current_subtitle_stream;
  int current_chapter;

  /* Can we seek? */

  int can_seek;
  int can_pause;
  int do_bypass; /* Bypass mode */
  
  /* Message stuff */

  bg_msg_queue_t      * command_queue;
  bg_msg_queue_list_t * message_queues;

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

  int mute;
  pthread_mutex_t mute_mutex;
  
  bg_player_saved_state_t saved_state;
  
  int visualizer_enabled;
  pthread_mutex_t config_mutex;
  
  bg_player_time_update_mode_t time_update_mode;
  
  bg_fifo_finish_mode_t finish_mode;
  gavl_time_t wait_time;
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

int bg_player_keep_going(bg_player_t * player,
                         void (*ping_func)(void*), void * data, int interrupt);

/* Get the current time (thread save) */

void bg_player_time_get(bg_player_t * player, int exact, gavl_time_t * ret);
void bg_player_time_stop(bg_player_t * player);
void bg_player_time_start(bg_player_t * player);
void bg_player_time_init(bg_player_t * player);
void bg_player_time_reset(bg_player_t * player);
void bg_player_time_set(bg_player_t * player, gavl_time_t time);
void bg_player_broadcast_time(bg_player_t * player, gavl_time_t time);

/* player_input.c */

void bg_player_input_create(bg_player_t * player);
void bg_player_input_destroy(bg_player_t * player);

void * bg_player_input_thread(void *);
void * bg_player_input_thread_bypass(void *);

int bg_player_input_init(bg_player_input_context_t * ctx,
                         bg_plugin_handle_t * handle,
                         int track_index);
int bg_player_input_set_track(bg_player_input_context_t * ctx);

void bg_player_input_select_streams(bg_player_input_context_t * ctx);
int bg_player_input_start(bg_player_input_context_t * ctx);
void bg_player_input_stop(bg_player_input_context_t * ctx);



void bg_player_input_cleanup(bg_player_input_context_t * ctx);

void bg_player_input_preload(bg_player_input_context_t * ctx);

int
bg_player_input_get_audio_format(bg_player_input_context_t * ctx);
int
bg_player_input_get_video_format(bg_player_input_context_t * ctx);

int
bg_player_input_read_audio(void * priv, gavl_audio_frame_t * frame, int stream, int samples);

int
bg_player_input_read_video(void * priv, gavl_video_frame_t * frame, int stream);

int
bg_player_input_read_video_subtitle_only(void * priv, gavl_video_frame_t * frame, int stream);

int
bg_player_input_get_subtitle_format(bg_player_input_context_t * ctx);

int
bg_player_input_set_still_stream(bg_player_input_context_t * ctx,
                                 int still_stream);

void bg_player_input_seek(bg_player_input_context_t * ctx,
                          gavl_time_t * time);

void bg_player_input_bypass_set_volume(bg_player_input_context_t * ctx,
                                       float volume);

void bg_player_input_bypass_set_pause(bg_player_input_context_t * ctx,
                                      int pause);

void bg_player_input_send_messages(bg_player_input_context_t * ctx);



/* player_ov.c */

void bg_player_ov_create(bg_player_t * player);

void bg_player_ov_reset(bg_player_t * player);

void bg_player_ov_destroy(bg_player_t * player);
int  bg_player_ov_init(bg_player_ov_context_t * ctx);

int  bg_player_ov_has_plugin(bg_player_ov_context_t * ctx);


void bg_player_ov_cleanup(bg_player_ov_context_t * ctx);
void * bg_player_ov_thread(void *);

void bg_player_ov_update_aspect(bg_player_ov_context_t * ctx,
                                int pixel_width, int pixel_height);

/* Update still image: To be called during pause */
void bg_player_ov_update_still(bg_player_ov_context_t * ctx);

void bg_player_ov_standby(bg_player_ov_context_t * ctx);

/* Create/destroy frames */
void * bg_player_ov_create_frame(void * data);
void bg_player_ov_destroy_frame(void * data, void * frame);

void * bg_player_ov_create_subtitle_overlay(void * data);
void bg_player_ov_destroy_subtitle_overlay(void * data, void * frame);

/* Set this extra because we must initialize subtitles after the video output */
void bg_player_ov_set_subtitle_format(void * data);

void bg_player_ov_set_plugin(bg_player_t * player,
                             bg_plugin_handle_t * handle);

/* Plugin handle is needed by the core to fire up the visualizer */

bg_plugin_handle_t * bg_player_ov_get_plugin(bg_player_ov_context_t * ctx);

/*
 *  This call will let the video plugin adjust the playertime from the
 *  next frame to be displayed
 */
void bg_player_ov_sync(bg_player_t * p);

/* player_video.c */

int bg_player_video_init(bg_player_t * p, int video_stream);
void bg_player_video_cleanup(bg_player_t * p);
void bg_player_video_create(bg_player_t * p, bg_plugin_registry_t * plugin_reg);
void bg_player_video_destroy(bg_player_t * p);


/* player_oa.c */

void * bg_player_oa_create_frame(void * data);
void bg_player_oa_destroy_frame(void * data, void * frame);

void bg_player_oa_create(bg_player_t * player);
void bg_player_oa_destroy(bg_player_t * player);

int  bg_player_oa_has_plugin(bg_player_oa_context_t * ctx);


int bg_player_oa_init(bg_player_oa_context_t * ctx);
int bg_player_oa_start(bg_player_oa_context_t * ctx);
void bg_player_oa_stop(bg_player_oa_context_t * ctx);

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

void bg_player_audio_create(bg_player_t * p, bg_plugin_registry_t * plugin_reg);
void bg_player_audio_destroy(bg_player_t * p);

/* player_subtitle.c */

int bg_player_subtitle_init(bg_player_t * player, int subtitle_stream);
void bg_player_subtitle_cleanup(bg_player_t * p);

void bg_player_subtitle_create(bg_player_t * p);
void bg_player_subtitle_destroy(bg_player_t * p);

void bg_player_subtitle_init_converter(bg_player_t * player);


void bg_player_accel_pressed(bg_player_t * player, int id);

void bg_player_set_track(bg_player_t *  player, int track);
void bg_player_set_track_name(bg_player_t * player, const char *);
void bg_player_set_duration(bg_player_t * player, gavl_time_t duration, int can_seek);
void bg_player_set_metadata(bg_player_t * player, const bg_metadata_t *);



/* Number of frames in the buffers */

#define NUM_AUDIO_FRAMES   16
#define NUM_VIDEO_FRAMES    8
#define NUM_SUBTITLE_FRAMES 4

