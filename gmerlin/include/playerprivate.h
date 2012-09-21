/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
#include <gmerlin/utils.h>
#include <gmerlin/bggavl.h>
#include <gmerlin/textrenderer.h>
#include <gmerlin/osd.h>
#include <gmerlin/converters.h>
#include <gmerlin/filters.h>
#include <gmerlin/visualize.h>

#include <gmerlin/translation.h>
#include <gmerlin/ov.h>

#include <player_thread.h>

/* Each thread get it's private context */


typedef enum
  {
    SYNC_SOFTWARE,
    SYNC_SOUNDCARD,
  } bg_player_sync_mode_t;

typedef enum
  {
    TIME_UPDATE_SECOND = 0,
    TIME_UPDATE_FRAME,
  } bg_player_time_update_mode_t;

typedef enum
  {
    BG_PLAYER_FINISH_CHANGE = 0,
    BG_PLAYER_FINISH_PAUSE,
  } bg_player_finish_mode_t;

/* Stream structures */

typedef struct
  {
  /* Pipeline */
  
  bg_audio_filter_chain_t * fc;
  gavl_audio_converter_t * cnv_out;
  
  bg_read_audio_func_t in_func;
  void * in_data;
  int    in_stream;
  
  int do_convert_out;
  
  gavl_audio_frame_t * fifo_frame;
  gavl_audio_frame_t * output_frame;
  
  pthread_mutex_t config_mutex;
  bg_gavl_audio_options_t options;
  
  gavl_audio_format_t input_format;
  gavl_audio_format_t output_format;
  gavl_audio_format_t fifo_format;

  /* Volume control */
  gavl_volume_control_t * volume;
  pthread_mutex_t volume_mutex;

  /* Mute */
  int mute;
  pthread_mutex_t mute_mutex;
  
  int send_silence;
  gavl_peak_detector_t * peak_detector;
  /* If the playback was interrupted due to changed parameters */
  int interrupted;
  
  /* Output plugin */
  bg_plugin_handle_t * plugin_handle;
  bg_oa_plugin_t     * plugin;
  void               * priv;

  /*
   *  Sync mode, set ONLY in init function,
   *  read by various threads
   */
  
  bg_player_sync_mode_t sync_mode;

  int output_open;
  /* Timing stuff */
  
  pthread_mutex_t time_mutex;
  gavl_time_t     current_time;
  gavl_timer_t *  timer;

  int64_t samples_written;
  int64_t samples_read;

  int has_first_timestamp_o;
  int has_first_timestamp_i;

  pthread_mutex_t eof_mutex;
  int eof;

  bg_player_thread_t * th;

  gavl_audio_sink_t * sink_out;
  gavl_audio_sink_t * sink_intern;
  
  } bg_player_audio_stream_t;

typedef struct
  {
  bg_text_renderer_t * renderer;
  gavl_video_converter_t * cnv;
  int do_convert;

  pthread_mutex_t config_mutex;
  
  gavl_video_format_t input_format;
  gavl_video_format_t output_format;

  /* For text subtitles */
  char * buffer;
  int buffer_alloc;
  
  gavl_overlay_t input_subtitle;
  
  gavl_overlay_t * current_subtitle;
  gavl_overlay_t * next_subtitle;

  gavl_overlay_t * subtitles[2];
  
  int eof;

  bg_parameter_info_t * parameters;
  
  int64_t time_offset;
  } bg_player_subtitle_stream_t;

typedef struct
  {
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

  bg_ov_t * ov;
  bg_ov_callbacks_t callbacks;
  gavl_time_t frame_time;
  
  int subtitle_id; /* Stream id for subtitles in the output plugin */

  gavl_video_format_t osd_format;
  
  bg_osd_t * osd;
  int osd_id;
  gavl_overlay_t * osd_ovl;
  
  bg_msg_queue_t * msg_queue;
  
  int64_t frames_written;
  int64_t frames_read;
  
  bg_accelerator_map_t * accel_map;
  
  bg_player_subtitle_stream_t * ss;

  gavl_video_frame_t * still_frame_in;
  int do_still;
  
  int eof;
  pthread_mutex_t eof_mutex;
  
  float bg_color[4];

  bg_player_thread_t * th;

  /* 1 if we are currently displaying a subtitle */
  int subtitle_active;

  /* 1 if we are currently displaying OSD */
  int osd_active;
    
  int64_t skip;
  int64_t last_frame_time;
  
  } bg_player_video_stream_t;


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
#define PLAYER_FREEZE_FRAME        (1<<17)
#define PLAYER_FREEZE_VIS          (1<<18)

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

#define PLAYER_MAX_THREADS 2

struct bg_player_s
  {
  pthread_t player_thread;

  bg_player_thread_common_t * thread_common;

  bg_player_thread_t * threads[PLAYER_MAX_THREADS];
  
  /* Input plugin and stuff */

  bg_input_callbacks_t input_callbacks;

  bg_plugin_handle_t * input_handle;
  bg_input_plugin_t  * input_plugin;
  void * input_priv;
  
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

  int current_track;
  
  /* Can we seek? */

  int can_seek;
  int can_pause;
  
  /* Message stuff */

  bg_msg_queue_t      * command_queue;
  bg_msg_queue_list_t * message_queues;

  /* Inernal state variables */

  int state;
  pthread_mutex_t state_mutex;

  /* Stuff for synchronous stopping and starting of the playback */
  
  float volume; /* Current volume in dB (0 == max) */
  
  bg_player_saved_state_t saved_state;
  
  int visualizer_enabled;
  
  pthread_mutex_t config_mutex;
  float still_framerate;
  gavl_time_t sync_offset;
  
  bg_player_time_update_mode_t time_update_mode;
  
  bg_player_finish_mode_t finish_mode;
  gavl_time_t wait_time;
  };

int  bg_player_get_state(bg_player_t * player);
void bg_player_set_state(bg_player_t * player, int state, const void * arg1,
                         const void * arg2);


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


int bg_player_input_init(bg_player_t * p,
                         bg_plugin_handle_t * handle,
                         int track_index);

int bg_player_input_set_track(bg_player_t * p);

void bg_player_input_select_streams(bg_player_t * p);
int bg_player_input_start(bg_player_t * p);
void bg_player_input_stop(bg_player_t * p);

void bg_player_input_cleanup(bg_player_t * p);


int
bg_player_input_get_audio_format(bg_player_t * ctx);
int
bg_player_input_get_video_format(bg_player_t * ctx);

int
bg_player_input_read_audio(void * priv, gavl_audio_frame_t * frame, int stream, int samples);

int
bg_player_input_read_video(void * priv, gavl_video_frame_t * frame, int stream);

int
bg_player_input_read_video_still(void * priv, gavl_video_frame_t * frame, int stream);

int
bg_player_input_read_video_subtitle_only(void * priv, gavl_video_frame_t * frame, int stream);

int
bg_player_input_get_subtitle_format(bg_player_t * ctx);


void bg_player_input_seek(bg_player_t * ctx,
                          gavl_time_t * time, int scale);


void bg_player_input_send_messages(bg_player_t * p);



/* player_ov.c */

void bg_player_ov_create(bg_player_t * player);

void bg_player_ov_reset(bg_player_t * player);

void bg_player_ov_destroy(bg_player_t * player);
int bg_player_ov_init(bg_player_video_stream_t * vs);

void bg_player_ov_cleanup(bg_player_video_stream_t * ctx);
void * bg_player_ov_thread(void *);

void bg_player_ov_update_aspect(bg_player_video_stream_t * ctx,
                                int pixel_width, int pixel_height);

/* Update still image: To be called during pause */
void bg_player_ov_update_still(bg_player_t * p);
void bg_player_ov_handle_events(bg_player_video_stream_t * s);


void bg_player_ov_standby(bg_player_video_stream_t * ctx);


/* Set this extra because we must initialize subtitles after the video output */
void bg_player_ov_set_subtitle_format(bg_player_video_stream_t * ctx);


void bg_player_ov_set_plugin(bg_player_t * player,
                             bg_plugin_handle_t * handle);

/* Plugin handle is needed by the core to fire up the visualizer */

// bg_plugin_handle_t * bg_player_ov_get_plugin(bg_player_ov_context_t * ctx);

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

int bg_player_read_video(bg_player_t * p,
                         gavl_video_frame_t * frame);

void bg_player_video_set_eof(bg_player_t * p);


/* player_oa.c */

int bg_player_oa_init(bg_player_audio_stream_t * ctx);
int bg_player_oa_start(bg_player_audio_stream_t * ctx);
void bg_player_oa_stop(bg_player_audio_stream_t * ctx);

void bg_player_oa_cleanup(bg_player_audio_stream_t * ctx);
void * bg_player_oa_thread(void *);

void bg_player_oa_set_plugin(bg_player_t * player,
                             bg_plugin_handle_t * handle);

void bg_player_oa_set_volume(bg_player_audio_stream_t * ctx,
                             float volume);

void bg_player_peak_callback(void * priv, int num_samples,
                             const double * min,
                             const double * max,
                             const double * abs);


/*
 *  Audio output must be locked, since the sound latency is
 *  obtained from here by other threads
 */

int  bg_player_oa_get_latency(bg_player_audio_stream_t * ctx);

/* player_audio.c */

int  bg_player_audio_init(bg_player_t * p, int audio_stream);
void bg_player_audio_cleanup(bg_player_t * p);

void bg_player_audio_create(bg_player_t * p, bg_plugin_registry_t * plugin_reg);
void bg_player_audio_destroy(bg_player_t * p);

int bg_player_read_audio(bg_player_t * p, gavl_audio_frame_t * frame);

/* Returns 1 if the thread should be finished, 0 if silence should be sent */
int bg_player_audio_set_eof(bg_player_t * p);


/* player_subtitle.c */

int bg_player_subtitle_init(bg_player_t * player, int subtitle_stream);
void bg_player_subtitle_cleanup(bg_player_t * p);

void bg_player_subtitle_create(bg_player_t * p);
void bg_player_subtitle_destroy(bg_player_t * p);

void bg_player_subtitle_init_converter(bg_player_t * player);

int bg_player_has_subtitle(bg_player_t * p);
int bg_player_read_subtitle(bg_player_t * p, gavl_overlay_t * ovl);



void bg_player_accel_pressed(bg_player_t * player, int id);

void bg_player_set_track_name(bg_player_t * player, const char *);
void bg_player_set_duration(bg_player_t * player, gavl_time_t duration, int can_seek);

void bg_player_set_metadata(bg_player_t * player, const gavl_metadata_t *);
