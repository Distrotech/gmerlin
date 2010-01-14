/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <gmerlin/plugin.h>

#ifdef HAVE_V4L
#include "vloopback.h"
#endif

typedef struct video_stream_s
  {
  pthread_t thread;

  gavl_video_format_t pipe_format;
  gavl_video_format_t monitor_format;
  gavl_video_format_t out_format;
  
  bg_plugin_handle_t * input_handle;
  bg_plugin_handle_t * monitor_handle;
  bg_plugin_handle_t * output_handle;

  /* For encoding or output */
  void (*put_frame)(struct video_stream_s * s, gavl_video_frame_t * f);
  
  } video_stream_t;

struct gmerlin_webcam_s
  {
  gavl_video_frame_t     * monitor_frame;
  gavl_video_converter_t * monitor_cnv;
  bg_plugin_handle_t     * monitor_handle;
  bg_ov_plugin_t         * monitor;

  gavl_video_frame_t       * capture_frame;
  gavl_video_converter_t   * capture_cnv;
  bg_plugin_handle_t       * capture_handle;
  bg_image_writer_plugin_t * capture;
  
  int do_convert_monitor;
  int do_convert_capture;
  
  int do_monitor;
  
  int monitor_open;
  int input_open;
  int capture_open;

  int capture_initialized;

  gavl_timer_t * timer;
  gavl_time_t next_capture_time;
  gavl_time_t capture_interval;
  
  int auto_capture;
  
  uint32_t capture_frame_counter;

  uint32_t frame_counter;

  gavl_video_format_t cam_format;
  
  gavl_video_format_t input_format;
  gavl_video_format_t monitor_format;
  gavl_video_format_t capture_format;
  
  bg_plugin_handle_t * input_handle;

  bg_recorder_plugin_t     * input;
  
  gavl_video_frame_t * input_frame;

  pthread_t thread;
  pthread_mutex_t mutex;

  bg_msg_queue_t * cmd_queue;
  bg_msg_queue_t * msg_queue;

  char * capture_directory;
  char * capture_namebase;
  
  bg_video_filter_chain_t * fc;

  bg_plugin_registry_t * plugin_reg;

  bg_gavl_video_options_t opt;
  
  int do_quit;

  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;

  int running;
  int interrupted;

#ifdef HAVE_V4L
  bg_vloopback_t * vloopback;
  int do_vloopback;
  pthread_mutex_t vloopback_mutex;
#endif  
  };

void * video_thread(void * priv);

