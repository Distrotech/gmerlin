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

#include <gmerlin/bggavl.h>
#include <gmerlin/filters.h>
#include <gmerlin/encoder.h>

#include <player_thread.h>

#define NUM_THREADS 2

#define STREAM_ACTIVE       (1<<0)
#define STREAM_INPUT_OPEN   (1<<1)
#define STREAM_MONITOR      (1<<2)
#define STREAM_MONITOR_OPEN (1<<3)
#define STREAM_ENCODE       (1<<4)
#define STREAM_ENCODE_OPEN  (1<<5)

#define FLAG_RUNNING        (1<<0)     
#define FLAG_RECORDING      (1<<1) 
#define FLAG_DO_RECORD      (1<<2) 

typedef struct 
  {
  int flags;

  /* Options */
  bg_gavl_audio_options_t opt;
  
  /* Input section */
  bg_plugin_handle_t * input_handle;
  bg_recorder_plugin_t * input_plugin;
  
  bg_audio_filter_chain_t * fc;
  
  /* Output section */
  bg_plugin_handle_t * output_handle;
  gavl_audio_converter_t * output_cnv;
  gavl_audio_frame_t     * output_frame;

  /* Encoder section */
  gavl_audio_converter_t * enc_cnv;
  gavl_audio_frame_t     * enc_frame;
  
  bg_player_thread_t * th;
  
  bg_read_audio_func_t in_func;
  void * in_data;
  int in_stream;
  
  bg_parameter_info_t * parameters;
  
  int do_convert_enc;
  
  /* Formats */
  gavl_audio_format_t input_format;
  gavl_audio_format_t pipe_format;
  gavl_audio_format_t enc_format;
  
  gavl_audio_frame_t * pipe_frame;

  gavl_peak_detector_t * pd;

  int enc_index;

  char language[4];
  
  } bg_recorder_audio_stream_t;

typedef struct 
  {
  int flags;

  /* Options */
  bg_gavl_video_options_t opt;
  
  /* Input section */
  bg_plugin_handle_t * input_handle;
  bg_recorder_plugin_t * input_plugin;
  
  bg_video_filter_chain_t * fc;
  
  /* Monitor section */
  bg_plugin_handle_t     * monitor_handle;
  bg_ov_plugin_t         * monitor_plugin;
  gavl_video_converter_t * monitor_cnv;
  
  int do_convert_monitor;
  int do_convert_enc;

  //  int do_monitor;
  
  /* Output section */
  bg_plugin_handle_t * output_handle;
  gavl_video_converter_t * output_cnv;
  gavl_video_frame_t     * output_frame;

  /* Encoder section */
  gavl_video_converter_t * enc_cnv;
  gavl_video_frame_t     * enc_frame;
  
  bg_player_thread_t * th;

  bg_read_video_func_t in_func;
  void * in_data;
  int in_stream;
  
  bg_parameter_info_t * parameters;
  bg_parameter_info_t * monitor_parameters;

  /* Formats */
  gavl_video_format_t input_format;
  gavl_video_format_t pipe_format;
  gavl_video_format_t monitor_format;
  gavl_video_format_t enc_format;

  /* Frames */
  gavl_video_frame_t * pipe_frame;
  gavl_video_frame_t * monitor_frame;
  
  gavl_timer_t * timer;
  
  gavl_time_t fps_frame_time;
  int fps_frame_counter;
  
  gavl_time_t last_frame_time;
  gavl_time_t last_capture_duration;
  
  int limit_timescale;
  int limit_duration;
  
  int64_t frame_counter;
  
  pthread_mutex_t config_mutex;
  
  int enc_index;

  } bg_recorder_video_stream_t;

struct bg_recorder_s
  {
  bg_recorder_audio_stream_t as;
  bg_recorder_video_stream_t vs;
  
  bg_plugin_registry_t * plugin_reg;
  
  bg_player_thread_common_t * tc;
  
  bg_player_thread_t * th[NUM_THREADS];
  
  char * display_string;
  
  int flags;
  
  bg_msg_queue_list_t * msg_queues;

  bg_parameter_info_t * encoder_parameters;
  bg_cfg_section_t * encoder_section;
  
  bg_encoder_t * enc;
  
  pthread_mutex_t enc_mutex;
  
  char * output_directory;
  char * output_filename_mask;
  char * snapshot_directory;
  char * snapshot_filename_mask;

  bg_metadata_t m;
  bg_parameter_info_t * metadata_parameters;

  gavl_time_t recording_time;
  gavl_time_t last_recording_time;
  pthread_mutex_t time_mutex;
  
  };

void bg_recorder_create_audio(bg_recorder_t*);
void bg_recorder_destroy_audio(bg_recorder_t *);

void bg_recorder_create_video(bg_recorder_t*);
void bg_recorder_destroy_video(bg_recorder_t *);

void * bg_recorder_audio_thread(void * data);
void * bg_recorder_video_thread(void * data);

int bg_recorder_audio_init(bg_recorder_t *);
int bg_recorder_video_init(bg_recorder_t *);

void bg_recorder_audio_finalize_encode(bg_recorder_t *);
void bg_recorder_video_finalize_encode(bg_recorder_t *);


void bg_recorder_audio_cleanup(bg_recorder_t *);
void bg_recorder_video_cleanup(bg_recorder_t *);

void bg_recorder_update_time(bg_recorder_t *, gavl_time_t t);

/* Message stuff */
void bg_recorder_msg_framerate(bg_recorder_t * rec,
                               float framerate);

void bg_recorder_msg_audiolevel(bg_recorder_t * rec,
                                double * level, int samples);

void bg_recorder_msg_time(bg_recorder_t * rec, gavl_time_t t);
