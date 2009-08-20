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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>

#include <gavl/gavl.h>

#include <gmerlin/bg_sem.h>
#include <gmerlin/fifo.h>

/* Common frame structure */

typedef struct frame_s
  {
  void * frame;
  struct frame_s * next;

  sem_t produced;
  sem_t consumed;
  int eof;
  } frame_t;

static frame_t * create_frame(void * (*create_frame)(void*), void * data)
  {
  frame_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  sem_init(&(ret->produced), 0, 0);
  sem_init(&(ret->consumed), 0, 1);

  ret->frame = create_frame(data);
    
  return ret;
  }

static void destroy_frame(frame_t * f,
                          void (*_destroy_frame)(void*, void*),
                          void * data)
  {
  sem_destroy(&(f->produced));
  sem_destroy(&(f->consumed));
  _destroy_frame(data, f->frame);
  free(f);
  }

/* Common Fifo structure */

struct bg_fifo_s
  {
  frame_t * frames; /* Chained list */
  int num_frames;

  frame_t * input_frame;
  frame_t * output_frame;
  
  pthread_mutex_t input_frame_mutex;
  pthread_mutex_t output_frame_mutex;
  
  pthread_mutex_t state_mutex;
  bg_fifo_state_t state;

  /* Keep track of the waiting threads */

  //  int input_waiting;
  pthread_mutex_t input_waiting_mutex;
  
  //  int output_waiting;
  pthread_mutex_t output_waiting_mutex;

  bg_fifo_finish_mode_t finish_mode;

  int input_locked;
  int output_locked;
  };

static void set_input_waiting(bg_fifo_t * f, int waiting)
  {
  if(waiting)
    pthread_mutex_lock(&(f->input_waiting_mutex));
  else
    pthread_mutex_unlock(&(f->input_waiting_mutex));
  }

static void set_output_waiting(bg_fifo_t * f, int waiting)
  {
  if(waiting)
    pthread_mutex_lock(&(f->output_waiting_mutex));
  else
    pthread_mutex_unlock(&(f->output_waiting_mutex));
  }

static int get_input_waiting(bg_fifo_t * f)
  {
  int ret;
  ret = !!pthread_mutex_trylock(&(f->input_waiting_mutex));
  if(!ret)
    pthread_mutex_unlock(&(f->input_waiting_mutex));
  return ret;
  }

static int get_output_waiting(bg_fifo_t * f)
  {
  int ret;
  ret = !!pthread_mutex_trylock(&(f->output_waiting_mutex));
  if(!ret)
    pthread_mutex_unlock(&(f->output_waiting_mutex));
  return ret;
  }

static bg_fifo_state_t get_state(bg_fifo_t * f)
  {
  bg_fifo_state_t ret;
  pthread_mutex_lock(&(f->state_mutex));
  ret = f->state;
  pthread_mutex_unlock(&(f->state_mutex));
  return ret;
  }

bg_fifo_t * bg_fifo_create(int num_frames,
                           void * (*create_func)(void*), void * data,
                           bg_fifo_finish_mode_t finish_mode)
  {
  bg_fifo_t * ret;
  frame_t * tmp_frame;
  int i;
  ret = calloc(1, sizeof(*ret));
  ret->finish_mode = finish_mode;
  
  tmp_frame = create_frame(create_func, data);
  ret->frames = tmp_frame;

  for(i = 1; i < num_frames; i++)
    {
    tmp_frame->next = create_frame(create_func, data);
    tmp_frame = tmp_frame->next;
    }
  
  ret->num_frames = num_frames;
  tmp_frame->next = ret->frames;

  ret->input_frame = ret->frames;
  ret->output_frame = ret->frames;
  pthread_mutex_init(&(ret->state_mutex),NULL);
  pthread_mutex_init(&(ret->input_frame_mutex),NULL);
  pthread_mutex_init(&(ret->output_frame_mutex),NULL);
  pthread_mutex_init(&(ret->input_waiting_mutex),NULL);
  pthread_mutex_init(&(ret->output_waiting_mutex),NULL);
  return ret;
  }

void bg_fifo_destroy(bg_fifo_t * f,
                     void (*destroy_func)(void*, void*),
                     void * data)
  {
  int i;
  frame_t * tmp_frame;
  
  pthread_mutex_destroy(&(f->state_mutex));
  pthread_mutex_destroy(&(f->input_frame_mutex));
  pthread_mutex_destroy(&(f->output_frame_mutex));
  pthread_mutex_destroy(&(f->input_waiting_mutex));
  pthread_mutex_destroy(&(f->output_waiting_mutex));
  
  tmp_frame = f->frames->next;
  for(i = 0; i < f->num_frames; i++)
    {
    destroy_frame(f->frames, destroy_func, data);
    f->frames = tmp_frame;
    if(i < f->num_frames - 1)
      tmp_frame = tmp_frame->next;
    }
  free(f);
  }

void * bg_fifo_lock_read(bg_fifo_t*f, bg_fifo_state_t * state)
  {
  *state = get_state(f);
  if(*state != BG_FIFO_PLAYING)
    return (void*)0;
  
  set_output_waiting(f, 1);
  
  while(sem_wait(&(f->output_frame->produced)) == -1)
    {
    if(errno != EINTR)
      {
      set_output_waiting(f, 0);
      return (void*)0;
      }
    }
  
  set_output_waiting(f, 0);
  
  if(f->output_frame->eof)
    {
    *state = (f->finish_mode == BG_FIFO_FINISH_CHANGE) ?
      BG_FIFO_STOPPED : BG_FIFO_PAUSED;
    bg_fifo_set_state(f, *state);
    return (void*)0;
    }

  *state = get_state(f);

  if(*state == BG_FIFO_PLAYING)
    {
    f->output_locked = 1;
    return f->output_frame->frame;
    }
  else
    return  NULL;
  }

int bg_fifo_test_read(bg_fifo_t * f, bg_fifo_state_t * state, int * eof)
  {
  int semval;
  int valid = 0;
  *eof = 0;
  
  *state = get_state(f);
  if(f->output_locked)
    {
    sem_getvalue(&f->output_frame->next->produced, &semval);
    if(semval > 0)
      {
      valid = !f->output_frame->next->eof;
      *eof = f->output_frame->next->eof;
      }
    }
  else
    {
    sem_getvalue(&f->output_frame->produced, &semval);
    if(semval > 0)
      {
      valid = !f->output_frame->eof;
      *eof = f->output_frame->eof;
      }
    }
  return valid;
  }

int bg_fifo_test_write(bg_fifo_t * f, bg_fifo_state_t * state)
  {
  int semval;
  *state = get_state(f);
  if(f->input_locked)
    sem_getvalue(&f->input_frame->next->consumed, &semval);
  else
    sem_getvalue(&f->input_frame->consumed, &semval);
  return (semval > 0) ? 1 : 0;
  }

void * bg_fifo_try_lock_read(bg_fifo_t*f, bg_fifo_state_t * state)
  {
  *state = get_state(f);
  
  if(*state != BG_FIFO_PLAYING)
    return (void *)0;

  if(sem_trywait(&(f->output_frame->produced)))
    return (void *)0;

  if(f->output_frame->eof)
    {
    *state = (f->finish_mode == BG_FIFO_FINISH_CHANGE) ?
      BG_FIFO_STOPPED : BG_FIFO_PAUSED;
    bg_fifo_set_state(f, *state);
    return (void*)0;
    }
  f->output_locked = 1;
  return f->output_frame->frame;
  }

void bg_fifo_unlock_read(bg_fifo_t*f)
  {
  pthread_mutex_lock(&(f->output_frame_mutex));
  
  sem_post(&(f->output_frame->consumed));

  if(f->output_frame->eof)
    {
    /* FIXME: Probably unneccesary */
    
    /* Post the procuced semaphore also so we can
       report EOF more than once */
    sem_post(&(f->output_frame->produced));
    }
  else
    {
    f->output_frame = f->output_frame->next;
    }
  pthread_mutex_unlock(&(f->output_frame_mutex));
  f->output_locked = 0;
  }

void * bg_fifo_lock_write(bg_fifo_t*f, bg_fifo_state_t * state)
  {
  *state = get_state(f);
  
  if(*state != BG_FIFO_PLAYING)
    return (void *)0;
  
  set_input_waiting(f, 1);
  while(sem_wait(&(f->input_frame->consumed)) == -1)
    {
    if(errno != EINTR)
      {
      set_input_waiting(f, 0);
      return (void*)0;
      }
    }
  set_input_waiting(f, 0);

  *state = get_state(f);

  if(*state != BG_FIFO_PLAYING)
    return (void *)0;

  f->input_locked = 1;
  return f->input_frame->frame;
  }

void * bg_fifo_try_lock_write(bg_fifo_t*f, bg_fifo_state_t * state)
  {
  *state = get_state(f);
  
  if(*state != BG_FIFO_PLAYING)
    return (void *)0;

  if(sem_trywait(&(f->input_frame->consumed)))
    return (void *)0;

  f->input_locked = 1;
  
  return f->input_frame->frame;
  }


/*
 *  Unlock frame for writing. If eof == 1,
 *  the frame is invalid and playback will stop
 *  as soon as the fifo is emtpy
 */

void bg_fifo_unlock_write(bg_fifo_t*f, int eof)
  {
  pthread_mutex_lock(&(f->input_frame_mutex));
  f->input_frame->eof = eof;
  sem_post(&(f->input_frame->produced));
  f->input_frame = f->input_frame->next;
  pthread_mutex_unlock(&(f->input_frame_mutex));
  f->input_locked = 0;
  }

void bg_fifo_set_state(bg_fifo_t * f, bg_fifo_state_t state)
  {
  pthread_mutex_lock(&(f->state_mutex));
  f->state = state;

  if(f->state != BG_FIFO_PLAYING)
    {
    /* Check whether output thread waits for the semaphore */
    if(get_output_waiting(f))
      sem_post(&(f->output_frame->produced));
    
    /* Check whether input thread waits for the semaphore */
    if(get_input_waiting(f))
      sem_post(&(f->input_frame->consumed));
    }
  pthread_mutex_unlock(&(f->state_mutex));
  }

void bg_fifo_clear(bg_fifo_t * f)
  {
  int i;
  frame_t * tmp_frame;
  
  //  fprintf(stderr, "fifo clear\n");
  
  tmp_frame = f->frames;
  for(i = 0; i < f->num_frames; i++)
    {
    sem_destroy(&(tmp_frame->produced));
    sem_destroy(&(tmp_frame->consumed));
    
    sem_init(&(tmp_frame->produced), 0, 0);
    sem_init(&(tmp_frame->consumed), 0, 1);
    tmp_frame->eof = 0;
    tmp_frame = tmp_frame->next;
    }
  f->output_frame = f->input_frame;
  //  f->input_waiting = 0;
  //  f->output_waiting = 0;
  
  }
