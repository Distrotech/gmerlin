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

#include <player_thread.h>

#define NUM_THREADS 2

typedef struct 
  {
  int active;
  /* Options */
  bg_gavl_audio_options_t opt;
  
  /* Input section */
  bg_plugin_handle_t * input_handle;
  bg_recorder_plugin_t * input_plugin;
  
  bg_audio_filter_chain_t * fc;
  
  int do_monitor;
  
  /* Output section */
  bg_plugin_handle_t * output_handle;
  gavl_audio_converter_t * output_cnv;
  gavl_audio_frame_t     * output_frame;
  
  bg_player_thread_t * th;
  
  bg_read_audio_func_t in_func;
  void * in_data;
  int in_stream;
  
  bg_parameter_info_t * parameters;

  /* Formats */
  gavl_audio_format_t input_format;
  gavl_audio_format_t pipe_format;
  
  gavl_audio_frame_t * pipe_frame;
  
  } bg_recorder_audio_stream_t;

typedef struct 
  {
  int active;
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
  int do_monitor;
  
  /* Output section */
  bg_plugin_handle_t * output_handle;
  gavl_video_converter_t * output_cnv;
  gavl_video_frame_t     * output_frame;
  
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

  /* Frames */
  gavl_video_frame_t * pipe_frame;
  gavl_video_frame_t * monitor_frame;
  
  } bg_recorder_video_stream_t;


struct bg_recorder_s
  {
  bg_recorder_audio_stream_t as;
  bg_recorder_video_stream_t vs;
  
  bg_plugin_registry_t * plugin_reg;
  
  bg_player_thread_common_t * tc;
  
  bg_player_thread_t * th[NUM_THREADS];
  
  char * display_string;
  
  int running;
  };

void bg_recorder_create_audio(bg_recorder_t*);
void bg_recorder_destroy_audio(bg_recorder_t *);

void bg_recorder_create_video(bg_recorder_t*);
void bg_recorder_destroy_video(bg_recorder_t *);

void * bg_recorder_audio_thread(void * data);
void * bg_recorder_video_thread(void * data);

int bg_recorder_audio_init(bg_recorder_t *);
int bg_recorder_video_init(bg_recorder_t *);
