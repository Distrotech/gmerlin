/*****************************************************************
 
  msgqueue.h
 
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

#ifndef __BG_MSGQUEUE_H_
#define __BG_MSGQUEUE_H_

#include <gavl/gavl.h>

/* Reserved ID for non valid message */

#define BG_MSG_NONE     -1

/* Maximum number of args */

#define BG_MSG_MAX_ARGS  4

/* Thread save message queues */

typedef struct bg_msg_queue_s bg_msg_queue_t;
typedef struct bg_msg_s bg_msg_t;

void bg_msg_destroy(bg_msg_t * m);
bg_msg_t * bg_msg_create();



/* Functions for messages */

void bg_msg_set_id(bg_msg_t * msg, int id);

void bg_msg_set_arg_int(bg_msg_t * msg, int arg, int value);

void bg_msg_set_arg_time(bg_msg_t * msg, int arg, gavl_time_t time);

void bg_msg_set_arg_string(bg_msg_t * msg, int arg, const char * value);
void bg_msg_set_arg_float(bg_msg_t * msg, int arg, float value);
void * bg_msg_set_arg_ptr(bg_msg_t * msg, int arg, int len);

void bg_msg_set_arg_ptr_nocopy(bg_msg_t * msg, int arg, void * ptr);

int    bg_msg_get_id(bg_msg_t * msg);

int    bg_msg_get_arg_int(bg_msg_t * msg, int arg);
float  bg_msg_get_arg_float(bg_msg_t * msg, int arg);
void * bg_msg_get_arg_ptr(bg_msg_t * msg, int arg, int * len);

void * bg_msg_get_arg_ptr_nocopy(bg_msg_t * msg, int arg);
gavl_time_t bg_msg_get_arg_time(bg_msg_t * msg, int arg);

/* Get/set specific types */

void bg_msg_get_arg_audio_format(bg_msg_t * msg, int arg,
                                 gavl_audio_format_t * format);
void bg_msg_set_arg_audio_format(bg_msg_t * msg, int arg,
                                 gavl_audio_format_t * format);

void bg_msg_get_arg_video_format(bg_msg_t * msg, int arg,
                                 gavl_video_format_t * format);
void bg_msg_set_arg_video_format(bg_msg_t * msg, int arg,
                                 gavl_video_format_t * format);


/*
 *  You can get the string value only once from each arg
 *  and must free() it, when yor are done with it
 */

char * bg_msg_get_arg_string(bg_msg_t * msg, int arg);

bg_msg_queue_t * bg_msg_queue_create();
void bg_msg_queue_destroy(bg_msg_queue_t *);

/*
 *  This on will be used for remote controls,
 *  return FALSE on error
 */

int bg_message_read(bg_msg_t * ret,  int fd);
int bg_message_write(bg_msg_t * msg, int fd);

/*
 *  Lock message queue for reading, block until something arrives,
 *  return the message ID
 */

bg_msg_t * bg_msg_queue_lock_read(bg_msg_queue_t *);
bg_msg_t * bg_msg_queue_try_lock_read(bg_msg_queue_t *);
void bg_msg_queue_unlock_read(bg_msg_queue_t *);

bg_msg_t * bg_msg_queue_lock_write(bg_msg_queue_t *);
void bg_msg_queue_unlock_write(bg_msg_queue_t *);

/*
 *  Finally, we have a list of message queues 
 *  This can be used, if some informations have to be passed to
 *  multiple recipients
 */

typedef struct bg_msg_queue_list_s bg_msg_queue_list_t;

bg_msg_queue_list_t * bg_msg_queue_list_create();

void bg_msg_queue_list_destroy(bg_msg_queue_list_t *);

void 
bg_msg_queue_list_send(bg_msg_queue_list_t *,
                       void (*set_message)(bg_msg_t * message, void * data),
                       void * data);

void bg_msg_queue_list_add(bg_msg_queue_list_t * list,
                        bg_msg_queue_t * queue);

void bg_msg_queue_list_remove(bg_msg_queue_list_t * list,
                           bg_msg_queue_t * queue);



#endif /* __BG_MSGQUEUE_H_ */
