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

#include <gmerlin/msgqueue.h>

/* Messages from the webcam */

#define MSG_FRAMERATE   0 /* Current capture rate (float) */
#define MSG_FRAME_COUNT 1 /* Frame counter (int)          */
#define MSG_ERROR       2 /* Something went wrong         */

/* Commands to the webcam */

#define CMD_SET_MONITOR                0 /* ARG 0: int */
#define CMD_SET_MONITOR_PLUGIN         1 /* ARG 0: plugin_handle */

#define CMD_SET_INPUT_PLUGIN           3 /* ARG 0: plugin_handle */

#define CMD_SET_CAPTURE_PLUGIN         4 /* ARG 0: plugin_handle */
#define CMD_SET_CAPTURE_AUTO           5 /* ARG 0: int   */
#define CMD_SET_CAPTURE_INTERVAL       6 /* ARG 0: float */
#define CMD_SET_CAPTURE_DIRECTORY      7 /* ARG 0: string */
#define CMD_SET_CAPTURE_NAMEBASE       8 /* ARG 0: string */

#define CMD_SET_CAPTURE_FRAME_COUNTER  9 /* ARG 0: int */

#define CMD_DO_CAPTURE                10 /* No args */
#define CMD_INPUT_REOPEN              11 /* No args */

#define CMD_QUIT                      12 /* No args */

#define CMD_SET_VLOOPBACK             13 /* ARG 0: int */
                                         /* ARG 1: string */

typedef struct gmerlin_webcam_s gmerlin_webcam_t;

gmerlin_webcam_t * gmerlin_webcam_create(bg_plugin_registry_t * plugin_reg);

void gmerlin_webcam_get_message_queues(gmerlin_webcam_t *,
                                       bg_msg_queue_t ** cmd_queue,
                                       bg_msg_queue_t ** msg_queue);
                 

void gmerlin_webcam_run(gmerlin_webcam_t *);
void gmerlin_webcam_quit(gmerlin_webcam_t *);

void gmerlin_webcam_destroy(gmerlin_webcam_t *);

const bg_parameter_info_t * gmerlin_webcam_get_filter_parameters(gmerlin_webcam_t *);
void gmerlin_webcam_set_filter_parameter(void * data, const char * name,
                                         const bg_parameter_value_t * val);

#ifdef HAVE_V4L
const bg_parameter_info_t * gmerlin_webcam_get_vloopback_parameters(gmerlin_webcam_t *);
void gmerlin_webcam_set_vloopback_parameter(void * data, const char * name,
                                            const bg_parameter_value_t * val);
#endif
