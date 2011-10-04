/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
#include <stdio.h>
#include <stdlib.h>

#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>
#include <gmerlin/msgqueue.h>
#include <gavl/gavl.h>

#define BUFFER_SIZE 1024

#define MSG_QUIT   0
#define MSG_VOID   1
#define MSG_INT    2
#define MSG_STRING 3
#define MSG_FLOAT  4

static void * thread1_func(void * data)
  {
  int keep_going = 1;
  bg_msg_t * msg;
  bg_msg_queue_t * queue;
  char * str;
    
  queue = (bg_msg_queue_t *)data;
    
  while(keep_going)
    {
    msg = bg_msg_queue_lock_read(queue);
    
    switch(bg_msg_get_id(msg))
      {
      case MSG_QUIT:
        fprintf(stderr, "Got message quit\n");
        keep_going = 0;
        break;
      case MSG_VOID:
        fprintf(stderr, "Got message void\n");
        break;
      case MSG_INT:
        fprintf(stderr, "Got message int %d\n", bg_msg_get_arg_int(msg, 0));
        break;
      case MSG_STRING:
        str = bg_msg_get_arg_string(msg, 0);
        fprintf(stderr, "Got message string %s\n", str);
        free(str);
        break;
      case MSG_FLOAT:
        fprintf(stderr, "Got message int %f\n", bg_msg_get_arg_float(msg, 0));
        break;
        
      }
    bg_msg_queue_unlock_read(queue);
    }
  return NULL;
  }

static int read_message(bg_msg_queue_t * q)
  {
  int num_messages;
  int id;
  int val_i;
  float val_f;
  char val_str[BUFFER_SIZE];
  bg_msg_t * msg;
  int i;
  printf("Enter ID [0-4]: ");
  scanf("%d", &id);

  num_messages = 1;
  
  switch(id)
    {
    case MSG_QUIT:
      break;
    case MSG_VOID:
      break;
    case MSG_INT:
      printf("Enter Integer: ");
      scanf("%d", &val_i);
      num_messages = 10;
      break;
    case MSG_STRING:
      printf("Enter String: ");
      fgets(val_str, BUFFER_SIZE, stdin);
      break;
    case MSG_FLOAT:
      printf("Enter float: ");
      scanf("%f", &val_f);
      break;
    }

  for(i = 0; i < num_messages; i++)
    {
    msg = bg_msg_queue_lock_write(q);
    bg_msg_set_id(msg, id);
    
    switch(id)
      {
      case MSG_QUIT:
      case MSG_VOID:
        break;
      case MSG_INT:
        bg_msg_set_arg_int(msg, 0, val_i);
        break;
      case MSG_STRING:
        bg_msg_set_arg_string(msg, 0, val_str);
        break;
      case MSG_FLOAT:
        bg_msg_set_arg_float(msg, 0, val_f);
        break;
      }
    bg_msg_queue_unlock_write(q);
    }
  if(id == MSG_QUIT)
    return 0;
  return 1;
  }


int main(int argc, char ** argv)
  {
  pthread_t read_thread;
  
  bg_msg_queue_t * queue;

  queue = bg_msg_queue_create();

  pthread_create(&read_thread,
                 NULL,
                 thread1_func, queue);

  while(1)
    {
    if(!read_message( queue))
      break;
    }

  pthread_join(read_thread,NULL);
  
  
  return 0;
  }
         
