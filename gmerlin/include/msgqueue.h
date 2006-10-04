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
#include "streaminfo.h"


/** \defgroup messages Messages
 *  \brief Communication inside and between applications
 *
 *  Gmerlin messages are a universal method to do communication between
 *  processes or threads. Each message consists of an integer ID and
 *  a number of arguments. Arguments can be strings, numbers or complex types
 *  like \ref gavl_audio_format_t. For inter-thread comminucation, you can pass pointers
 *  as arguments as well.
 *
 *  For multithread applications, there are message queues (\ref bg_msg_queue_t). They
 *  are thread save FIFO structures, which allow asynchronous communication between threads.
 *
 *  For communication via sockets, there are the ultra-simple functions \ref bg_message_read_socket and
 *  \ref bg_message_write_socket, which can be used to build network protocols
 *  (e.g. remote control of applications)
 *  @{
 */

#define BG_MSG_NONE     -1 //!< Reserved ID for non valid message
#define BG_MSG_MAX_ARGS  4 //!< Maximum number of args

/** \brief Opaque message type, you don't want to know what's inside
 */

typedef struct bg_msg_s bg_msg_t;

/** \brief Create a message
 *  \returns A newly allocated message
 */

bg_msg_t * bg_msg_create();

/** \brief Destroy a message
 *  \param msg A message
 */

void bg_msg_destroy(bg_msg_t * msg);

/** \brief Free internal memory of the message
 *  \param msg A message
 *
 *  Use this, if you want to reuse the message with
 *  a different ID or args
 */

void bg_msg_free(bg_msg_t * msg);

/* Functions for messages */

/** \brief Set the ID of a message
 *  \param msg A message
 *  \param id The ID
 */

void bg_msg_set_id(bg_msg_t * msg, int id);

/** \brief Get the ID of a message
 *  \param msg A message
 *  \returns The ID
 */

int    bg_msg_get_id(bg_msg_t * msg);


/** \brief Set an integer argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param value Value
 */

void bg_msg_set_arg_int(bg_msg_t * msg, int arg, int value);

/** \brief Get an integer argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \returns Value
 */

int bg_msg_get_arg_int(bg_msg_t * msg, int arg);

/** \brief Set a time argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param value Value
 */

void bg_msg_set_arg_time(bg_msg_t * msg, int arg, gavl_time_t value);

/** \brief Get a time argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \returns Value
 */

gavl_time_t bg_msg_get_arg_time(bg_msg_t * msg, int arg);

/** \brief Set a string argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param value Value
 */

void bg_msg_set_arg_string(bg_msg_t * msg, int arg, const char * value);

/** \brief Get a string argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \returns The string
 *
 *  You can get the string value only once from each arg
 *  and must free() it, when you are done with it
 */

char * bg_msg_get_arg_string(bg_msg_t * msg, int arg);


/** \brief Set a float argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param value Value
 */
void bg_msg_set_arg_float(bg_msg_t * msg, int arg, float value);

/** \brief Get a float argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \returns Value
 */

float  bg_msg_get_arg_float(bg_msg_t * msg, int arg);


/** \brief Set a binary data argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param len Number of bytes requested
 *  \returns A pointer to a buffer of at least len bytes, to which you can copy the data
 */
void * bg_msg_set_arg_ptr(bg_msg_t * msg, int arg, int len);

/** \brief Set a binary data argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param len Returns the number of bytes
 *  \returns A pointer to a buffer of at least len bytes, where you find the data
 *
 *  You can get the buffer only once from each arg
 *  and must free() it, when you are done with it
 */
  
void * bg_msg_get_arg_ptr(bg_msg_t * msg, int arg, int * len);
  
/** \brief Set a pointer argument without copying data
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param ptr A pointer
 *
 *  Use this only for communication inside the same address space
 */

void bg_msg_set_arg_ptr_nocopy(bg_msg_t * msg, int arg, void * ptr);

/** \brief Get a pointer argument without copying data
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \returns A pointer
 *
 *  Use this only for communication inside the same address space
 */

void * bg_msg_get_arg_ptr_nocopy(bg_msg_t * msg, int arg);


/** \brief Set an audio format argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param format An audio format
 */

void bg_msg_set_arg_audio_format(bg_msg_t * msg, int arg,
                                 const gavl_audio_format_t * format);

/** \brief Get an audio format argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param format Returns the audio format
 */

void bg_msg_get_arg_audio_format(bg_msg_t * msg, int arg,
                                 gavl_audio_format_t * format);


/** \brief Set a video format argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param format A video format
 */

void bg_msg_set_arg_video_format(bg_msg_t * msg, int arg,
                                 const gavl_video_format_t * format);

/** \brief Get a video format argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param format Returns the video format
 */

void bg_msg_get_arg_video_format(bg_msg_t * msg, int arg,
                                 gavl_video_format_t * format);


/** \brief Set a matadata argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param m Metadata
 */

void bg_msg_set_arg_metadata(bg_msg_t * msg, int arg,
                             const bg_metadata_t * m);

/** \brief Get a matadata argument
 *  \param msg A message
 *  \param arg Argument index (starting with 0)
 *  \param m Returns metadata
 *
 *  Don't pass uninitalized memory as metadata.
 */

void bg_msg_get_arg_metadata(bg_msg_t * msg, int arg,
                             bg_metadata_t * m);


/*
 *  This on will be used for remote controls,
 *  return FALSE on error
 */

/** \brief Read a message from a socket
 *  \param ret Where the message will be copied
 *  \param fd A socket
 *  \param milliseconds Read timeout
 *  \returns 1 on success, 0 on error
 */

int bg_message_read_socket(bg_msg_t * ret,  int fd, int milliseconds);

/** \brief Write a message to a socket
 *  \param msg Message
 *  \param fd A socket
 *  \returns 1 on success, 0 on error
 */

int bg_message_write_socket(bg_msg_t * msg, int fd);

/** @} */

/** \defgroup message_queues Message queues
/*  \ingroup messages
 *  \brief Thread save message queues
 *
 *  @{
 */

/** \brief Opaque message queue type. You don't want to know what's inside.
 */

typedef struct bg_msg_queue_s bg_msg_queue_t;

/** \brief Create a message queue
 *  \returns A newly allocated message queue
 */

bg_msg_queue_t * bg_msg_queue_create();

/** \brief Destroy a message queue
 *  \param mq A message queue
 */

void bg_msg_queue_destroy(bg_msg_queue_t * mq);

/*
 *  Lock message queue for reading, block until something arrives,
 *  return the message ID
 */

/** \brief Lock a message queue for reading
 *  \param mq A message queue
 *  \returns A new message or NULL
 *
 *  This function blocks until a message arrives and returns the message.
 *  Use this function with caution to avoid deadlocks.
 *
 *  When you are done with the message, call \ref bg_msg_queue_unlock_read.
 *  The message is owned by the queue and must not be freed.
 */

bg_msg_t * bg_msg_queue_lock_read(bg_msg_queue_t * mq);

/** \brief Try to lock a message queue for reading
 *  \param mq A message queue
 *  \returns A new message or NULL
 *
 *  This function immediately returns NULL if there is no message for reading.
 *  When you are done with the message, call \ref bg_msg_queue_unlock_read.
 *  The message is owned by the queue and must not be freed.
 */

bg_msg_t * bg_msg_queue_try_lock_read(bg_msg_queue_t * mq);

/** \brief Unlock a message queue for reading
 *  \param mq A message queue
 *
 *  Call this to signal, that you are done with a message.
 */

void bg_msg_queue_unlock_read(bg_msg_queue_t * mq);

/** \brief Lock a message queue for writing
 *  \param mq A message queue
 *  \returns An empty message, where you can place your information.
 *
 *  When you are done setting the ID and arguments, call \ref bg_msg_queue_unlock_write.
 */

bg_msg_t * bg_msg_queue_lock_write(bg_msg_queue_t * mq);

/** \brief Unlock a message queue for writing
 *  \param mq A message queue
 *
 *  Call this to signal, that you are done with a message.
 */

void bg_msg_queue_unlock_write(bg_msg_queue_t * mq);

/** \brief Check, if there is a message for readinbg available and get the ID.
 *  \param mq A message queue
 *  \param id Might return the ID
 *  \returns 1 if there is a message (and id is valid), 0 else
 */

int bg_msg_queue_peek(bg_msg_queue_t * mq, uint32_t * id);

/** @} */

/** \defgroup message_queue_list Lists of message queues
 *  \ingroup messages
 *  \brief Send messages to multiple message queues
 
 *  Lists of message queues can be used, if some informations have to be passed to
 *  multiple recipients. Each listener adds a message queue to the list and will get
 *  all messages, which are broadcasted with \ref bg_msg_queue_list_send from the writing end.
 *  @{
 */

/** \brief Opaque message queue list type. You don't want to know what's inside.
 */

typedef struct bg_msg_queue_list_s bg_msg_queue_list_t;

/** \brief Create a message queue list
 *  \returns A newly allocated message queue list
 */

bg_msg_queue_list_t * bg_msg_queue_list_create();

/** \brief Destroy a message queue list
 *  \param list A message queue list
 */

void bg_msg_queue_list_destroy(bg_msg_queue_list_t * list);

/** \brief Send a message to all queues in the list
 *  \param list A message queue list
 *  \param set_message Function to set ID and arguments of a message
 *  \param data Data to pass to set_message
 */

void 
bg_msg_queue_list_send(bg_msg_queue_list_t * list,
                       void (*set_message)(bg_msg_t * message,
                                           const void * data),
                       const void * data);

/** \brief Add a queue to the list
 *  \param list A message queue list
 *  \param queue A message queue
 */

void bg_msg_queue_list_add(bg_msg_queue_list_t * list,
                        bg_msg_queue_t * queue);

/** \brief Remove a queue from the list
 *  \param list A message queue list
 *  \param queue A message queue
 */

void bg_msg_queue_list_remove(bg_msg_queue_list_t * list,
                           bg_msg_queue_t * queue);

/** @} */


#endif /* __BG_MSGQUEUE_H_ */
