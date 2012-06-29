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

#include <inttypes.h>
#include <config.h>
#include <float_cast.h>

#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>


#include <stdlib.h>
#include <assert.h>

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <gmerlin/bg_sem.h>

#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>
#include <gmerlin/msgqueue.h>

#include <gmerlin/utils.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/serialize.h>

#define TYPE_INT            0
#define TYPE_FLOAT          1
#define TYPE_POINTER        2
#define TYPE_POINTER_NOCOPY 3
#define TYPE_TIME           4
#define TYPE_COLOR_RGB      5
#define TYPE_COLOR_RGBA     6
#define TYPE_POSITION       7

struct bg_msg_s
  {
  uint32_t id;

  struct
    {
    union
      {
      int val_i;
      double val_f;
      void * val_ptr;
      gavl_time_t val_time;
      float val_color[4];
      double val_pos[2];
      } value;
    uint8_t type;
    uint32_t size;
    } args[BG_MSG_MAX_ARGS];

  int num_args;

  sem_t produced;
  
  bg_msg_t * prev;
  bg_msg_t * next;
  };

void bg_msg_set_id(bg_msg_t * msg, int id)
  {
  msg->id = id;
  msg->num_args = 0;

  /* Zero everything */

  memset(&msg->args, 0, sizeof(msg->args));
  
  }

static int check_arg(int arg)
  {
  if(arg < 0)
    return 0;
  assert(arg < BG_MSG_MAX_ARGS);
  return 1;
  }

/* Set argument to a basic type */

void bg_msg_set_arg_int(bg_msg_t * msg, int arg, int value)
  {
  if(!check_arg(arg))
    return;
  msg->args[arg].value.val_i = value;
  msg->args[arg].type = TYPE_INT;
  if(arg+1 > msg->num_args)
    msg->num_args = arg + 1;
  }

void bg_msg_set_arg_time(bg_msg_t * msg, int arg, gavl_time_t value)
  {
  if(!check_arg(arg))
    return;
  msg->args[arg].value.val_time = value;
  msg->args[arg].type = TYPE_TIME;
  if(arg+1 > msg->num_args)
    msg->num_args = arg + 1;
  }


void * bg_msg_set_arg_ptr(bg_msg_t * msg, int arg, int len)
  {
  if(!check_arg(arg))
    return NULL;
  
  msg->args[arg].value.val_ptr = calloc(1, len);
  msg->args[arg].size = len;
  msg->args[arg].type = TYPE_POINTER;
  if(arg+1 > msg->num_args)
    msg->num_args = arg + 1;
  return msg->args[arg].value.val_ptr;
  }

void bg_msg_set_arg_ptr_nocopy(bg_msg_t * msg, int arg, void * value)
  {
  if(!check_arg(arg))
    return;
  msg->args[arg].value.val_ptr = value;
  msg->args[arg].type = TYPE_POINTER_NOCOPY;
  if(arg+1 > msg->num_args)
    msg->num_args = arg + 1;
  }


void bg_msg_set_arg_string(bg_msg_t * msg, int arg, const char * value)
  {
  int length;
  void * dst;
  if(!value)
    return;
  length = strlen(value)+1;
  dst = bg_msg_set_arg_ptr(msg, arg, length);
  memcpy(dst, value, length);
  
  if(arg+1 > msg->num_args)
    msg->num_args = arg + 1;
  }

void bg_msg_set_arg_float(bg_msg_t * msg, int arg, double value)
  {
  if(!check_arg(arg))
    return;
  msg->args[arg].value.val_f = value;
  msg->args[arg].type = TYPE_FLOAT;
  if(arg+1 > msg->num_args)
    msg->num_args = arg + 1;
  }

void bg_msg_set_arg_color_rgb(bg_msg_t * msg, int arg, const float * value)
  {
  if(!check_arg(arg))
    return;
  msg->args[arg].value.val_color[0] = value[0];
  msg->args[arg].value.val_color[1] = value[1];
  msg->args[arg].value.val_color[2] = value[2];
  msg->args[arg].type = TYPE_COLOR_RGB;
  if(arg+1 > msg->num_args)
    msg->num_args = arg + 1;
  }

void bg_msg_set_arg_color_rgba(bg_msg_t * msg, int arg, const float * value)
  {
  if(!check_arg(arg))
    return;
  msg->args[arg].value.val_color[0] = value[0];
  msg->args[arg].value.val_color[1] = value[1];
  msg->args[arg].value.val_color[2] = value[2];
  msg->args[arg].value.val_color[3] = value[3];
  msg->args[arg].type = TYPE_COLOR_RGBA;
  if(arg+1 > msg->num_args)
    msg->num_args = arg + 1;
  }

void bg_msg_set_arg_position(bg_msg_t * msg, int arg, const double * value)
  {
  if(!check_arg(arg))
    return;
  msg->args[arg].value.val_pos[0] = value[0];
  msg->args[arg].value.val_pos[1] = value[1];
  msg->args[arg].type = TYPE_POSITION;
  if(arg+1 > msg->num_args)
    msg->num_args = arg + 1;
  }


/* Get basic types */

int bg_msg_get_id(bg_msg_t * msg)
  {
  return msg->id;
  }

int bg_msg_get_arg_int(bg_msg_t * msg, int arg)
  {
  if(!check_arg(arg))
    return 0;
  return msg->args[arg].value.val_i;
  }

gavl_time_t bg_msg_get_arg_time(bg_msg_t * msg, int arg)
  {
  if(!check_arg(arg))
    return 0;
  return msg->args[arg].value.val_time;
  }

double bg_msg_get_arg_float(bg_msg_t * msg, int arg)
  {
  if(!check_arg(arg))
    return 0.0;
  return msg->args[arg].value.val_f;
  }

void bg_msg_get_arg_color_rgb(bg_msg_t * msg, int arg, float * val)
  {
  if(!check_arg(arg))
    return;
  val[0] = msg->args[arg].value.val_color[0];
  val[1] = msg->args[arg].value.val_color[1];
  val[2] = msg->args[arg].value.val_color[2];
  }

void bg_msg_get_arg_color_rgba(bg_msg_t * msg, int arg, float * val)
  {
  if(!check_arg(arg))
    return;
  val[0] = msg->args[arg].value.val_color[0];
  val[1] = msg->args[arg].value.val_color[1];
  val[2] = msg->args[arg].value.val_color[2];
  val[3] = msg->args[arg].value.val_color[3];
  }

void bg_msg_get_arg_position(bg_msg_t * msg, int arg, double * val)
  {
  if(!check_arg(arg))
    return;
  val[0] = msg->args[arg].value.val_pos[0];
  val[1] = msg->args[arg].value.val_pos[1];
  }

void * bg_msg_get_arg_ptr(bg_msg_t * msg, int arg, int * length)
  {
  void * ret;
  
  if(!check_arg(arg))
    return NULL;

  ret = msg->args[arg].value.val_ptr;
  msg->args[arg].value.val_ptr = NULL;
  if(length)
    *length = msg->args[arg].size;
  return ret;
  }

void * bg_msg_get_arg_ptr_nocopy(bg_msg_t * msg, int arg)
  {
  if(!check_arg(arg))
    return NULL;
  return msg->args[arg].value.val_ptr;
  }

char * bg_msg_get_arg_string(bg_msg_t * msg, int arg)
  {
  char * ret;
  if(!check_arg(arg))
    return NULL;
  ret = msg->args[arg].value.val_ptr;
  msg->args[arg].value.val_ptr = NULL;
  return ret;
  }


/* Get/Set routines for structures */

static inline uint8_t * get_8(uint8_t * data, uint32_t * val)
  {
  *val = *data;
  data++;
  return data;
  }

static inline uint8_t * get_16(uint8_t * data, uint32_t * val)
  {
  *val = ((data[0] << 8) | data[1]);
  data+=2;
  return data;
  }

static inline uint8_t * get_32(uint8_t * data, uint32_t * val)
  {
  *val = ((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
  data+=4;
  return data;
  }

static inline uint8_t * get_str(uint8_t * data, char ** val)
  {
  uint32_t len;
  uint8_t * ret;

  ret = get_32(data, &len);

  if(len)
    {
    *val = malloc(len+1);
    memcpy(*val, ret, len);
    (*val)[len] = '\0';
    }
  return ret + len;
  }

static inline uint8_t * set_8(uint8_t * data, uint8_t val)
  {
  *data = val;
  data++;
  return data;
  }

static inline uint8_t * set_16(uint8_t * data, uint16_t val)
  {
  data[0] = (val & 0xff00) >> 8;
  data[1] = (val & 0xff);
  data+=2;
  return data;
  }

static inline uint8_t * set_32(uint8_t * data, uint32_t val)
  {
  data[0] = (val & 0xff000000) >> 24;
  data[1] = (val & 0xff0000) >> 16;
  data[2] = (val & 0xff00) >> 8;
  data[3] = (val & 0xff);
  data+=4;
  return data;
  }

static int str_len(const char * str)
  {
  int ret = 4;
  if(str)
    ret += strlen(str);
  return ret;
  }

static inline uint8_t * set_str(uint8_t * data, const char * val)
  {
  uint32_t len;
  if(val)
    len = strlen(val);
  else
    len = 0;
  
  data = set_32(data, len);
  if(len)
    memcpy(data, val, len);
  return data + len;
  }

/*
  int samples_per_frame; 
  int samplerate;
  int num_channels;
  gavl_sample_format_t   sample_format;
  gavl_interleave_mode_t interleave_mode;
  gavl_channel_setup_t   channel_setup;
  int lfe;
*/

void bg_msg_set_arg_audio_format(bg_msg_t * msg, int arg,
                                 const gavl_audio_format_t * format)
  {
  uint8_t * ptr;
  int len;
  len = bg_serialize_audio_format(format, NULL, 0);
  ptr = bg_msg_set_arg_ptr(msg, arg, len);
  bg_serialize_audio_format(format, ptr, len);
  }

void bg_msg_get_arg_audio_format(bg_msg_t * msg, int arg,
                                 gavl_audio_format_t * format, int * big_endian)
  {
  uint8_t * ptr;
  int len, be;
  
  ptr = bg_msg_get_arg_ptr(msg, arg, &len);
  bg_deserialize_audio_format(format, ptr, len, &be);

  if(big_endian)
    *big_endian = be;
  
  free(ptr);
  }

/* Video format */

/*
  int frame_width;
  int frame_height;
  int image_width;
  int image_height;
  int pixel_width;
  int pixel_height;
  gavl_pixelformat_t pixelformat;
  int framerate_num;
  int frame_duration;
  
  int free_framerate;
*/

void bg_msg_set_arg_video_format(bg_msg_t * msg, int arg,
                                 const gavl_video_format_t * format)
  {
  uint8_t * ptr;
  int len;
  len = bg_serialize_video_format(format, NULL, 0);
  ptr = bg_msg_set_arg_ptr(msg, arg, len);
  bg_serialize_video_format(format, ptr, len);
  }

void bg_msg_get_arg_video_format(bg_msg_t * msg, int arg,
                                 gavl_video_format_t * format, int * big_endian)
  {
  uint8_t * ptr;
  int len, be;
  
  ptr = bg_msg_get_arg_ptr(msg, arg, &len);
  bg_deserialize_video_format(format, ptr, len, &be);
  
  if(big_endian)
    *big_endian = be;
  
  free(ptr);
  }

/*
  char * artist;
  char * title;
  char * album;
      
  int track;
  char * date;
  char * genre;
  char * comment;

  char * author;
  char * copyright;
*/


void bg_msg_set_arg_metadata(bg_msg_t * msg, int arg,
                             const gavl_metadata_t * m)
  {
  int i;
  
  uint8_t * ptr;
  uint8_t * pos;

  int len = 4;

  for(i = 0; i < m->num_tags; i++)
    {
    len += str_len(m->tags[i].key);
    len += str_len(m->tags[i].val);
    }
  
  ptr = bg_msg_set_arg_ptr(msg, arg, len);
  pos = ptr;

  pos = set_32(pos, m->num_tags);
  for(i = 0; i < m->num_tags; i++)
    {
    pos = set_str(pos, m->tags[i].key);
    pos = set_str(pos, m->tags[i].val);
    }
  }

void bg_msg_get_arg_metadata(bg_msg_t * msg, int arg,
                             gavl_metadata_t * m)
  {
  int i;
  uint32_t num;
  uint8_t * ptr;
  uint8_t * pos;
  char * key;
  char * val;
  
  ptr = bg_msg_get_arg_ptr(msg, arg, NULL);
  
  pos = ptr;

  pos = get_32(pos, &num);

  for(i = 0; i < num; i++)
    {
    pos = get_str(pos, &key);
    pos = get_str(pos, &val);
    gavl_metadata_set_nocpy(m, key, val);
    free(key);
    }
  free(ptr);
  }

bg_msg_t * bg_msg_create()
  {
  bg_msg_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  sem_init(&ret->produced, 0, 0);
    
  return ret;
  }

void bg_msg_free(bg_msg_t * m)
  {
  int i;
  for(i = 0; i < m->num_args; i++)
    {
    if((m->args[i].type == TYPE_POINTER) &&
       (m->args[i].value.val_ptr))
      {
      free(m->args[i].value.val_ptr);
      m->args[i].value.val_ptr = NULL;
      }
    }
  }

void bg_msg_destroy(bg_msg_t * m)
  {
  bg_msg_free(m);
  sem_destroy(&m->produced);
  free(m);
  }

/* Audio frame:
   .arg_0 = valid samples
   .arg_1 = timestamp
   .arg_2 = big endian
*/

int bg_msg_write_audio_frame(bg_msg_t * msg,
                             const gavl_audio_format_t * format,
                             const gavl_audio_frame_t * frame,
                             bg_msg_write_callback_t cb,
                             void * cb_data)
  {
  uint8_t * ptr;
  int len;

  len = bg_serialize_audio_frame_header(format, frame, NULL, 0);
  ptr = bg_msg_set_arg_ptr(msg, 0, len);
  bg_serialize_audio_frame_header(format, frame, ptr, len);
  
  if(!bg_msg_write(msg, cb, cb_data))
    return 0;
  return bg_serialize_audio_frame(format, frame, cb, cb_data);
  }

/** \brief Read an audio frame from a socket
 *  \param msg Message containing the frame header
 *  \param format Audio format
 *  \param frame An audio frame
 *  \param fd A socket
 *  \returns 1 on success, 0 on error
 *
 *  Before you can use this function, msg must contain
 *  a valid audio frame header
 */

int bg_msg_read_audio_frame(gavl_dsp_context_t * ctx,
                            bg_msg_t * msg,
                            const gavl_audio_format_t * format,
                            gavl_audio_frame_t * frame,
                            bg_msg_read_callback_t cb,
                            void * cb_data, int big_endian)
  {
  uint8_t * ptr;
  int len;
  
  ptr = bg_msg_get_arg_ptr(msg, 0, &len);
  bg_deserialize_audio_frame_header(format, frame, ptr, len);
  free(ptr);
  
  return bg_deserialize_audio_frame(ctx, format, frame,
                                    cb, cb_data, big_endian);
  }

/* .parameters =
   .arg_0 = name
   .arg_1 = type (int)
   .arg_2 = value
*/

void bg_msg_set_parameter(bg_msg_t * msg,
                          const char * name,
                          bg_parameter_type_t type,
                          const bg_parameter_value_t * val)
  {
  if(!name)
    return;
  
  bg_msg_set_arg_string(msg, 0, name);
  bg_msg_set_arg_int(msg, 1, type);
  switch(type)
    {
    case BG_PARAMETER_SECTION: //!< Dummy type. It contains no data but acts as a separator in notebook style configuration windows
    case BG_PARAMETER_BUTTON: /* Button (just tell we are called) */
      break;
      
    case BG_PARAMETER_CHECKBUTTON: //!< Bool
    case BG_PARAMETER_INT:         //!< Integer spinbutton
    case BG_PARAMETER_SLIDER_INT:  //!< Integer slider
      bg_msg_set_arg_int(msg, 2, val->val_i);
      break;
    case BG_PARAMETER_FLOAT: // spinbutton
    case BG_PARAMETER_SLIDER_FLOAT: //!< Float slider
      bg_msg_set_arg_float(msg, 2, val->val_f);
      break;
    case BG_PARAMETER_STRING:      //!< String (one line only)
    case BG_PARAMETER_STRING_HIDDEN: //!< Encrypted string (displays as ***)
    case BG_PARAMETER_STRINGLIST:  //!< Popdown menu with string values
    case BG_PARAMETER_FONT:        //!< Font (contains fontconfig compatible fontname)
    case BG_PARAMETER_DEVICE:      //!< Device
    case BG_PARAMETER_FILE:        //!< File
    case BG_PARAMETER_DIRECTORY:   //!< Directory
    case BG_PARAMETER_MULTI_MENU:  //!< Menu with config- and infobutton
    case BG_PARAMETER_MULTI_LIST:  //!< List with config- and infobutton
    case BG_PARAMETER_MULTI_CHAIN: //!< Several subitems (including suboptions) can be arranged in a chain
      bg_msg_set_arg_string(msg, 2, val->val_str);
      break;
    case BG_PARAMETER_COLOR_RGB:   //!< RGB Color
      bg_msg_set_arg_color_rgb(msg, 2, val->val_color);
      break;
    case BG_PARAMETER_COLOR_RGBA:  //!< RGBA Color
      bg_msg_set_arg_color_rgba(msg, 2, val->val_color);
      break;
    case BG_PARAMETER_POSITION:  //!< RGBA Color
      bg_msg_set_arg_position(msg, 2, val->val_pos);
      break;
    case BG_PARAMETER_TIME:         //!< Time
      bg_msg_set_arg_time(msg, 2, val->val_time);
      break;
    }
  }

void bg_msg_get_parameter(bg_msg_t * msg,
                          char ** name,
                          bg_parameter_type_t * type,
                          bg_parameter_value_t * val)
  {
  *name = bg_msg_get_arg_string(msg, 0);
  
  if(!*name)
    return;
  
  *type = bg_msg_get_arg_int(msg, 1);
  switch(*type)
    {
    case BG_PARAMETER_SECTION: //!< Dummy type. It contains no data but acts as a separator in notebook style configuration windows
    case BG_PARAMETER_BUTTON:
      break;
      
    case BG_PARAMETER_CHECKBUTTON: //!< Bool
    case BG_PARAMETER_INT:         //!< Integer spinbutton
    case BG_PARAMETER_SLIDER_INT:  //!< Integer slider
      val->val_i = bg_msg_get_arg_int(msg, 2);
      break;
    case BG_PARAMETER_FLOAT: // spinbutton
    case BG_PARAMETER_SLIDER_FLOAT: //!< Float slider
      val->val_f = bg_msg_get_arg_float(msg, 2);
      break;
    case BG_PARAMETER_STRING:      //!< String (one line only)
    case BG_PARAMETER_STRING_HIDDEN: //!< Encrypted string (displays as ***)
    case BG_PARAMETER_STRINGLIST:  //!< Popdown menu with string values
    case BG_PARAMETER_FONT:        //!< Font (contains fontconfig compatible fontname)
    case BG_PARAMETER_DEVICE:      //!< Device
    case BG_PARAMETER_FILE:        //!< File
    case BG_PARAMETER_DIRECTORY:   //!< Directory
    case BG_PARAMETER_MULTI_MENU:  //!< Menu with config- and infobutton
    case BG_PARAMETER_MULTI_LIST:  //!< List with config- and infobutton
    case BG_PARAMETER_MULTI_CHAIN: //!< Several subitems (including suboptions) can be arranged in a chain
      val->val_str = bg_msg_get_arg_string(msg, 2);
      break;
    case BG_PARAMETER_COLOR_RGB:   //!< RGB Color
      bg_msg_get_arg_color_rgb(msg, 2, val->val_color);
      break;
    case BG_PARAMETER_COLOR_RGBA:  //!< RGBA Color
      bg_msg_get_arg_color_rgba(msg, 2, val->val_color);
      break;
    case BG_PARAMETER_POSITION:  //!< RGBA Color
      bg_msg_get_arg_position(msg, 2, val->val_pos);
      break;
    case BG_PARAMETER_TIME:         //!< Time
      bg_msg_set_arg_time(msg, 2, val->val_time);
      break;
    }
  }

struct bg_msg_queue_s
  {
  bg_msg_t * msg_input;
  bg_msg_t * msg_output;
  bg_msg_t * msg_last;

  pthread_mutex_t chain_mutex;
  pthread_mutex_t write_mutex;
  
  int num_messages; /* Number of total messages */
  };

bg_msg_queue_t * bg_msg_queue_create()
  {
  bg_msg_queue_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  /* Allocate at least 2 messages */
  
  ret->msg_output       = bg_msg_create();
  ret->msg_output->next = bg_msg_create();
  
  /* Set in- and output messages */

  ret->msg_input = ret->msg_output;
  ret->msg_last  = ret->msg_output->next;
  
  /* Initialize chain mutex */

  pthread_mutex_init(&ret->chain_mutex, NULL);
  pthread_mutex_init(&ret->write_mutex, NULL);
  
  return ret;
  }

void bg_msg_queue_destroy(bg_msg_queue_t * m)
  {
  bg_msg_t * tmp_message;
  while(m->msg_output)
    {
    tmp_message = m->msg_output->next;
    bg_msg_destroy(m->msg_output);
    m->msg_output = tmp_message;
    }
  free(m);
  }

/* Lock message queue for reading, block until something arrives */

bg_msg_t * bg_msg_queue_lock_read(bg_msg_queue_t * m)
  {
  while(sem_wait(&m->msg_output->produced) == -1)
    {
    if(errno != EINTR)
      return NULL;
    }
  return m->msg_output;
  
  // sem_ret->msg_output
  }

bg_msg_t * bg_msg_queue_try_lock_read(bg_msg_queue_t * m)
  {
  if(!sem_trywait(&m->msg_output->produced))
    return m->msg_output;
  else
    return NULL;
  }

int bg_msg_queue_peek(bg_msg_queue_t * m, uint32_t * id)
  {
  int sem_val;
  sem_getvalue(&m->msg_output->produced, &sem_val);
  if(sem_val)
    {
    if(id)
      *id = m->msg_output->id;
    return 1;
    }
  else
    return 0;
  }

void bg_msg_queue_unlock_read(bg_msg_queue_t * m)
  {

  bg_msg_t * old_out_message;

  pthread_mutex_lock(&m->chain_mutex);
  old_out_message = m->msg_output;
  
  bg_msg_free(old_out_message);
  
  m->msg_output = m->msg_output->next;
  m->msg_last->next = old_out_message;
  m->msg_last = m->msg_last->next;
  m->msg_last->next = NULL;

  pthread_mutex_unlock(&m->chain_mutex);
  }

/*
 *  Lock queue for writing
 */

bg_msg_t * bg_msg_queue_lock_write(bg_msg_queue_t * m)
  {
  pthread_mutex_lock(&m->write_mutex);
  return m->msg_input;
  }

void bg_msg_queue_unlock_write(bg_msg_queue_t * m)
  {
  bg_msg_t * message = m->msg_input;
    
  pthread_mutex_lock(&m->chain_mutex);
  if(!m->msg_input->next)
    {
    m->msg_input->next = bg_msg_create();
    m->msg_last = m->msg_input->next;
    }
  
  m->msg_input = m->msg_input->next;
  sem_post(&message->produced);
  pthread_mutex_unlock(&m->chain_mutex);
  pthread_mutex_unlock(&m->write_mutex);

  }

typedef struct list_entry_s
  {
  bg_msg_queue_t * q;
  struct list_entry_s * next;
  } list_entry_t;

struct bg_msg_queue_list_s
  {
  list_entry_t * entries;
  
  pthread_mutex_t mutex;
  };

bg_msg_queue_list_t * bg_msg_queue_list_create()
  {
  bg_msg_queue_list_t * ret = calloc(1, sizeof(*ret));
  pthread_mutex_init(&ret->mutex, NULL);
  return ret;
  }

void bg_msg_queue_list_destroy(bg_msg_queue_list_t * l)
  {
  list_entry_t * tmp_entry;

  while(l->entries)
    {
    tmp_entry = l->entries->next;
    free(l->entries);
    l->entries = tmp_entry;
    }
  free(l);
  }

void 
bg_msg_queue_list_send(bg_msg_queue_list_t * l,
                       void (*set_message)(bg_msg_t * message,
                                           const void * data),
                       const void * data)
  {
  bg_msg_t * msg;
  list_entry_t * entry;
  
  pthread_mutex_lock(&l->mutex);
  entry = l->entries;
  
  while(entry)
    {
    msg = bg_msg_queue_lock_write(entry->q);
    set_message(msg, data);
    bg_msg_queue_unlock_write(entry->q);
    entry = entry->next;
    }
  pthread_mutex_unlock(&l->mutex);
  }

void bg_msg_queue_list_add(bg_msg_queue_list_t * list,
                        bg_msg_queue_t * queue)
  {
  list_entry_t * new_entry;

  new_entry = calloc(1, sizeof(*new_entry));
  
  pthread_mutex_lock(&list->mutex);

  new_entry->next = list->entries;
  new_entry->q = queue;
  list->entries = new_entry;
  
  pthread_mutex_unlock(&list->mutex);
  }

void bg_msg_queue_list_remove(bg_msg_queue_list_t * list,
                           bg_msg_queue_t * queue)
  {
  list_entry_t * tmp_entry;
  list_entry_t * entry_before;
  
  pthread_mutex_lock(&list->mutex);

  if(list->entries->q == queue)
    {
    tmp_entry = list->entries->next;
    free(list->entries);
    list->entries = tmp_entry;
    }
  else
    {
    entry_before = list->entries;

    while(entry_before->next->q != queue)
      entry_before = entry_before->next;

    tmp_entry = entry_before->next;
    entry_before->next = tmp_entry->next;
    free(tmp_entry);
    }
  pthread_mutex_unlock(&list->mutex);
  }

/*
 *  This on will be used for remote controls,
 *  return FALSE on error
 */

/* This is how a message is passed through pipes and sockets:
 *
 * content [   id  |nargs|<arg1><arg2>....
 * bytes   |0|1|2|3|  4  |5....
 *
 * An int argument is encoded as:
 * content | type | value |
 * bytes   | 0    |1|2|3|4|
 *
 * Type is TYPE_INT, byte order is big endian (network byte order)
 *
 *  Floating point arguments are coded with type TYPE_FLOAT and the
 *  value as an integer
 *
 *  String arguments are coded as:
 *  content |length |string including final '\0'|
 *  bytes   |0|1|2|3|4 .. 4 + len               |
 *
 *  Pointer messages cannot be transmited!
 */

static int read_uint32(uint32_t * ret, bg_msg_read_callback_t cb, void * cb_data)
  {
  uint8_t buf[4];

  if(cb(cb_data, buf, 4) < 4)
    return 0;
  
  //  bg_hexdump(buf, 4);
    
  *ret = (buf[0]<<24) | (buf[1]<<16) |
    (buf[2]<<8) | buf[3];
  return 1;
  }

static int read_uint64(uint64_t * ret, bg_msg_read_callback_t cb, void * cb_data)
  {
  uint8_t buf[8];
  
  if(cb(cb_data, buf, 8) < 8)
    return 0;

  *ret =
    ((gavl_time_t)(buf[0])<<56) |
    ((gavl_time_t)(buf[1])<<48) |
    ((gavl_time_t)(buf[2])<<40) |
    ((gavl_time_t)(buf[3])<<32) |
    ((gavl_time_t)(buf[4])<<24) |
    ((gavl_time_t)(buf[5])<<16) |
    ((gavl_time_t)(buf[6])<<8) |
    ((gavl_time_t)(buf[7]));
  return 1;
  }

static float
float32_be_read (unsigned char *cptr)
{       int             exponent, mantissa, negative ;
        float   fvalue ;

        negative = cptr [0] & 0x80 ;
        exponent = ((cptr [0] & 0x7F) << 1) | ((cptr [1] & 0x80) ? 1 : 0) ;
        mantissa = ((cptr [1] & 0x7F) << 16) | (cptr [2] << 8) | (cptr [3]) ;

        if (! (exponent || mantissa))
                return 0.0 ;

        mantissa |= 0x800000 ;
        exponent = exponent ? exponent - 127 : 0 ;

        fvalue = mantissa ? ((float) mantissa) / ((float) 0x800000) : 0.0 ;

        if (negative)
                fvalue *= -1 ;

        if (exponent > 0)
                fvalue *= (1 << exponent) ;
        else if (exponent < 0)
                fvalue /= (1 << abs (exponent)) ;

        return fvalue ;
} /* float32_be_read */


static int read_float(float * ret, bg_msg_read_callback_t cb, void * cb_data)
  {
  uint8_t buf[4];
  if(cb(cb_data, buf, 4) < 4)
    return 0;
  *ret = float32_be_read(buf);
  return 1;
  }

static double
double64_be_read (unsigned char *cptr)
{       int             exponent, negative ;
        double  dvalue ;

        negative = (cptr [0] & 0x80) ? 1 : 0 ;
        exponent = ((cptr [0] & 0x7F) << 4) | ((cptr [1] >> 4) & 0xF) ;

        /* Might not have a 64 bit long, so load the mantissa into a double. */
        dvalue = (((cptr [1] & 0xF) << 24) | (cptr [2] << 16) | (cptr [3] << 8) | cptr [4]) ;
        dvalue += ((cptr [5] << 16) | (cptr [6] << 8) | cptr [7]) / ((double) 0x1000000) ;

        if (exponent == 0 && dvalue == 0.0)
                return 0.0 ;

        dvalue += 0x10000000 ;

        exponent = exponent - 0x3FF ;

        dvalue = dvalue / ((double) 0x10000000) ;

        if (negative)
                dvalue *= -1 ;

        if (exponent > 0)
                dvalue *= (1 << exponent) ;
        else if (exponent < 0)
                dvalue /= (1 << abs (exponent)) ;

        return dvalue ;
} /* double64_be_read */


static int read_double(double * ret, bg_msg_read_callback_t cb, void * cb_data)
  {
  uint8_t buf[8];
  if(cb(cb_data, buf, 8) < 8)
    return 0;
  *ret = double64_be_read(buf);
  return 1;
  }

static int read_time(gavl_time_t * ret, bg_msg_read_callback_t cb, void * cb_data)
  {
  return read_uint64((uint64_t*)ret, cb, cb_data);
  }

static int write_uint32(uint32_t * val,
                        bg_msg_write_callback_t cb, void * cb_data)
  {
  uint8_t buf[4];
  
  buf[0] = (*val & 0xff000000) >> 24;
  buf[1] = (*val & 0x00ff0000) >> 16;
  buf[2] = (*val & 0x0000ff00) >> 8;
  buf[3] = (*val & 0x000000ff);

  //  bg_hexdump(buf, 4);
    
  if(cb(cb_data, buf, 4) < 4)
    return 0;
  return 1;
  }

static int write_uint64(uint64_t val,
                        bg_msg_write_callback_t cb, void * cb_data)
  {
  uint8_t buf[8];

  buf[0] = (val & 0xff00000000000000LL) >> 56;
  buf[1] = (val & 0x00ff000000000000LL) >> 48;
  buf[2] = (val & 0x0000ff0000000000LL) >> 40;
  buf[3] = (val & 0x000000ff00000000LL) >> 32;

  buf[4] = (val & 0x00000000ff000000LL) >> 24;
  buf[5] = (val & 0x0000000000ff0000LL) >> 16;
  buf[6] = (val & 0x000000000000ff00LL) >> 8;
  buf[7] = (val & 0x00000000000000ffLL);
  
  if(cb(cb_data, buf, 8) < 8)
    return 0;
  return 1;
  }

static void
float32_be_write (float in, unsigned char *out)
{       int             exponent, mantissa, negative = 0 ;

        memset (out, 0, sizeof (int)) ;

        if (in == 0.0)
                return ;

        if (in < 0.0)
        {       in *= -1.0 ;
                negative = 1 ;
                } ;

        in = frexp (in, &exponent) ;

        exponent += 126 ;

        in *= (float) 0x1000000 ;
        mantissa = (((int) in) & 0x7FFFFF) ;

        if (negative)
                out [0] |= 0x80 ;

        if (exponent & 0x01)
                out [1] |= 0x80 ;

        out [3] = mantissa & 0xFF ;
        out [2] = (mantissa >> 8) & 0xFF ;
        out [1] |= (mantissa >> 16) & 0x7F ;
        out [0] |= (exponent >> 1) & 0x7F ;

        return ;
} /* float32_be_write */


static int write_float(float val, bg_msg_write_callback_t cb, void * cb_data)
  {
  uint8_t buf[4];
  float32_be_write(val, buf);
  if(cb(cb_data, buf, 4) < 4)
    return 0;
  return 1;
  }

static void
double64_be_write (double in, unsigned char *out)
{       int             exponent, mantissa ;

        memset (out, 0, sizeof (double)) ;

        if (in == 0.0)
                return ;

        if (in < 0.0)
        {       in *= -1.0 ;
                out [0] |= 0x80 ;
                } ;

        in = frexp (in, &exponent) ;

        exponent += 1022 ;

        out [0] |= (exponent >> 4) & 0x7F ;
        out [1] |= (exponent << 4) & 0xF0 ;

        in *= 0x20000000 ;
        mantissa = lrint (floor (in)) ;

        out [1] |= (mantissa >> 24) & 0xF ;
        out [2] = (mantissa >> 16) & 0xFF ;
        out [3] = (mantissa >> 8) & 0xFF ;
        out [4] = mantissa & 0xFF ;

        in = fmod (in, 1.0) ;
        in *= 0x1000000 ;
        mantissa = lrint (floor (in)) ;
        out [5] = (mantissa >> 16) & 0xFF ;
        out [6] = (mantissa >> 8) & 0xFF ;
        out [7] = mantissa & 0xFF ;

        return ;
} /* double64_be_write */


static int write_double(double val, bg_msg_write_callback_t cb, void * cb_data)
  {
  uint8_t buf[8];
  double64_be_write(val, buf);
  if(cb(cb_data, buf, 8) < 8)
    return 0;
  return 1;
  }

static int write_time(gavl_time_t val,
                      bg_msg_write_callback_t cb, void * cb_data)
  {
  return write_uint64((uint64_t)val, cb, cb_data);
  }


int bg_msg_read(bg_msg_t * ret, bg_msg_read_callback_t cb, void * cb_data)
  {
  int i;
  void * ptr;
  
  int32_t val_i;

  gavl_time_t val_time;

  uint8_t val_u8;
  memset(ret, 0, sizeof(*ret));
  
  /* Message ID */

  if(!read_uint32((uint32_t*)(&val_i), cb, cb_data))
    return 0;
  
  ret->id = val_i;

  /* Number of arguments */

  if(!cb(cb_data, &val_u8, 1))
    return 0;
  
  ret->num_args = val_u8;

  for(i = 0; i < ret->num_args; i++)
    {
    if(!cb(cb_data, &val_u8, 1))
      return 0;
    ret->args[i].type = val_u8;
    
    switch(ret->args[i].type)
      {
      case TYPE_INT:
        if(!read_uint32((uint32_t*)(&val_i), cb, cb_data))
          return 0;
        
        ret->args[i].value.val_i = val_i;
        break;
      case TYPE_FLOAT:
        if(!read_double(&ret->args[i].value.val_f, cb, cb_data))
          return 0;
        break;
      case TYPE_POINTER:
        if(!read_uint32((uint32_t*)(&val_i), cb, cb_data)) /* Length */
          return 0;
        ptr = bg_msg_set_arg_ptr(ret, i, val_i);
        if(cb(cb_data, ret->args[i].value.val_ptr, val_i) < val_i)
          return 0;
        break;
      case TYPE_TIME:
        if(!read_time(&val_time, cb, cb_data))
          return 0;
        ret->args[i].value.val_time = val_time;
        break;
      case TYPE_POINTER_NOCOPY:
        break;
      case TYPE_COLOR_RGB:
        if(!read_float(&ret->args[i].value.val_color[0], cb, cb_data) ||
           !read_float(&ret->args[i].value.val_color[1], cb, cb_data) ||
           !read_float(&ret->args[i].value.val_color[2], cb, cb_data))
          return 0;
        break;
      case TYPE_COLOR_RGBA:
        if(!read_float(&ret->args[i].value.val_color[0], cb, cb_data) ||
           !read_float(&ret->args[i].value.val_color[1], cb, cb_data) ||
           !read_float(&ret->args[i].value.val_color[2], cb, cb_data) ||
           !read_float(&ret->args[i].value.val_color[3], cb, cb_data))
          return 0;
        break;
      }
    }
  return 1;
  }

int bg_msg_write(bg_msg_t * msg, bg_msg_write_callback_t cb,
                     void * cb_data)
  {
  int i;
  uint8_t val_u8;
    
  /* Message id */

  if(!write_uint32(&msg->id, cb, cb_data))
    return 0;

  /* Number of arguments */
  val_u8 = msg->num_args;
  
  if(!cb(cb_data, &val_u8, 1))
    return 0;
  
  /* Arguments */

  for(i = 0; i < msg->num_args; i++)
    {
    cb(cb_data, &msg->args[i].type, 1);

    switch(msg->args[i].type)
      {
      case TYPE_INT:
        if(!write_uint32((uint32_t*)(&msg->args[i].value.val_i), cb, cb_data))
          return 0;
        break;
      case TYPE_TIME:
        if(!write_time(msg->args[i].value.val_time, cb, cb_data))
          return 0;
        break;
      case TYPE_FLOAT:
        if(!write_double(msg->args[i].value.val_f, cb, cb_data))
          return 0;
        break;
      case TYPE_POINTER:
        if(!write_uint32(&msg->args[i].size, cb, cb_data))
          return 0;
        if(cb(cb_data, msg->args[i].value.val_ptr, msg->args[i].size) <
           msg->args[i].size)
          return 0;
        break;
      case TYPE_POINTER_NOCOPY:
        break;
      case TYPE_COLOR_RGB:
        if(!write_float(msg->args[i].value.val_color[0], cb, cb_data) ||
           !write_float(msg->args[i].value.val_color[1], cb, cb_data) ||
           !write_float(msg->args[i].value.val_color[2], cb, cb_data))
          return 0;
        break;
      case TYPE_COLOR_RGBA:
        if(!write_float(msg->args[i].value.val_color[0], cb, cb_data) ||
           !write_float(msg->args[i].value.val_color[1], cb, cb_data) ||
           !write_float(msg->args[i].value.val_color[2], cb, cb_data) ||
           !write_float(msg->args[i].value.val_color[3], cb, cb_data))
          return 0;
        break;
      }
    }
  return 1;
  }

typedef struct
  {
  int timeout;
  int fd;
  } socket_data;

static int socket_read_cb(void * data, uint8_t * ptr, int len)
  {
  int result;
  socket_data * cb_data = (socket_data *)data;
  result = bg_socket_read_data(cb_data->fd,
                               ptr, len, cb_data->timeout);
  
  /* After a successful read, timeout must be disabled */
  if(result && cb_data->timeout >= 0)
    cb_data->timeout = -1;
  return result;
  }

static int socket_write_cb(void * data, const uint8_t * ptr, int len)
  {
  socket_data * cb_data = (socket_data *)data;
  return bg_socket_write_data(cb_data->fd, ptr, len);
  
  }

int bg_msg_read_socket(bg_msg_t * ret, int fd, int milliseconds)
  {
  socket_data cb_data;
  cb_data.fd = fd;
  cb_data.timeout = milliseconds;
  return bg_msg_read(ret, socket_read_cb, &cb_data);
  }

int bg_msg_write_socket(bg_msg_t * msg, int fd)
  {
  socket_data cb_data;
  cb_data.fd = fd;
  cb_data.timeout = 0;
  return bg_msg_write(msg, socket_write_cb, &cb_data);
  }
