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

#include <gmerlin/pluginregistry.h>
#include <gmerlin/msgqueue.h>

extern const uint32_t bg_recorder_stream_mask;
extern const uint32_t bg_recorder_plugin_mask;


typedef struct bg_recorder_s bg_recorder_t;

bg_recorder_t * bg_recorder_create(bg_plugin_registry_t * plugin_reg);

void bg_recorder_destroy(bg_recorder_t *);

/* Run the threads, everything should be initialitzed then */
int bg_recorder_run(bg_recorder_t *);

/* Stop threads, start again with bg_recorder_run() */
void bg_recorder_stop(bg_recorder_t *);


void bg_recorder_add_message_queue(bg_recorder_t *,
                                   bg_msg_queue_t * msg_queue);

void bg_recorder_remove_message_queue(bg_recorder_t *,
                                      bg_msg_queue_t * msg_queue);

/* Commands */

// void bg_recorder_restart(bg_recorder_t*);

void bg_recorder_set_display_string(bg_recorder_t*, const char * str);

/* Messages */

/* Framerate (arg0: float) */
#define BG_RECORDER_MSG_FRAMERATE      0

/* Audio level (arg0: float, arg1: float) */
#define BG_RECORDER_MSG_AUDIOLEVEL     1

/* Recording (arg0: time) */
#define BG_RECORDER_MSG_TIME           2

/* Recording (arg0: audio (int), arg1: video (int) ) */

#define BG_RECORDER_MSG_RUNNING        3

/* Button press (arg0: x, arg1: y, arg2: button, arg3: mask) */
#define BG_RECORDER_MSG_BUTTON_PRESS   4

/* Button release (arg0: x, arg1: y, arg2: button, arg3: mask) */
#define BG_RECORDER_MSG_BUTTON_RELEASE 5

/* Button release (arg0: x, arg1: y, arg2: mask) */
#define BG_RECORDER_MSG_MOTION         6

/*
 *  Record = 1: Start recording
 *  Record = 0: Stop recording
 */
void bg_recorder_record(bg_recorder_t*, int record);

void bg_recorder_snapshot(bg_recorder_t*);

/* Parameter stuff */

/* Audio */

const bg_parameter_info_t *
bg_recorder_get_audio_parameters(bg_recorder_t *);

void
bg_recorder_set_audio_parameter(void * data,
                                const char * name,
                                const bg_parameter_value_t * val);

const bg_parameter_info_t *
bg_recorder_get_audio_filter_parameters(bg_recorder_t *);

void
bg_recorder_set_audio_filter_parameter(void * data,
                                       const char * name,
                                       const bg_parameter_value_t * val);
/* Video */

const bg_parameter_info_t *
bg_recorder_get_video_parameters(bg_recorder_t *);

void
bg_recorder_set_video_parameter(void * data,
                                const char * name,
                                const bg_parameter_value_t * val);

const bg_parameter_info_t *
bg_recorder_get_video_filter_parameters(bg_recorder_t *);

void
bg_recorder_set_video_filter_parameter(void * data,
                                       const char * name,
                                       const bg_parameter_value_t * val);

const bg_parameter_info_t *
bg_recorder_get_video_monitor_parameters(bg_recorder_t *);

void
bg_recorder_set_video_monitor_parameter(void * data,
                                        const char * name,
                                        const bg_parameter_value_t * val);

const bg_parameter_info_t *
bg_recorder_get_video_snapshot_parameters(bg_recorder_t *);

void
bg_recorder_set_video_snapshot_parameter(void * data,
                                         const char * name,
                                         const bg_parameter_value_t * val);


/* Encoders */
const bg_parameter_info_t *
bg_recorder_get_encoder_parameters(bg_recorder_t *);

void bg_recorder_set_encoder_section(bg_recorder_t *, bg_cfg_section_t *);


/* Output */
const bg_parameter_info_t *
bg_recorder_get_output_parameters(bg_recorder_t *);

void
bg_recorder_set_output_parameter(void * data,
                                 const char * name,
                                 const bg_parameter_value_t * val);

/* Metadata */

const bg_parameter_info_t *
bg_recorder_get_metadata_parameters(bg_recorder_t *);

void
bg_recorder_set_metadata_parameter(void * data,
                                   const char * name,
                                   const bg_parameter_value_t * val);

int bg_recorder_ping(bg_recorder_t * rec);
