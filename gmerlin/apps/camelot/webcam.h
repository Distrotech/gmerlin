#include <msgqueue.h>

/* Messages from the webcam */

#define MSG_FRAMERATE   0 /* Current capture rate (float) */
#define MSG_FRAME_COUNT 1 /* Frame counter (int)          */

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

typedef struct gmerlin_webcam_s gmerlin_webcam_t;

gmerlin_webcam_t * gmerlin_webcam_create();

void gmerlin_webcam_get_message_queues(gmerlin_webcam_t *,
                                       bg_msg_queue_t ** cmd_queue,
                                       bg_msg_queue_t ** msg_queue);
                 

void gmerlin_webcam_run(gmerlin_webcam_t *);
void gmerlin_webcam_quit(gmerlin_webcam_t *);

void gmerlin_webcam_destroy(gmerlin_webcam_t *);


