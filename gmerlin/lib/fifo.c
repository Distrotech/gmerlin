/*****************************************************************
 
  fifo.c
 
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

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include <gavl/gavl.h>
#include <fifo.h>

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
  //  fprintf(stderr, "Create_frame\n");
  
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

  frame_t *  input_frame;
  frame_t * output_frame;
  pthread_mutex_t input_frame_mutex;
  pthread_mutex_t output_frame_mutex;
  
  pthread_mutex_t state_mutex;
  bg_fifo_state_t state;

  /* Keep track of the waiting threads */

  int input_waiting;
  pthread_mutex_t input_waiting_mutex;
  
  int output_waiting;
  pthread_mutex_t output_waiting_mutex;

  };

static void set_input_waiting(bg_fifo_t * f, int waiting)
  {
  pthread_mutex_lock(&(f->input_waiting_mutex));
  f->input_waiting = waiting;
  pthread_mutex_unlock(&(f->input_waiting_mutex));
  }

static void set_output_waiting(bg_fifo_t * f, int waiting)
  {
  pthread_mutex_lock(&(f->output_waiting_mutex));
  f->output_waiting = waiting;
  pthread_mutex_unlock(&(f->output_waiting_mutex));
  }

static int get_input_waiting(bg_fifo_t * f)
  {
  int ret;
  pthread_mutex_lock(&(f->input_waiting_mutex));
  ret = f->input_waiting;
  pthread_mutex_unlock(&(f->input_waiting_mutex));
  return ret;
  }

static int get_output_waiting(bg_fifo_t * f)
  {
  int ret;
  pthread_mutex_lock(&(f->output_waiting_mutex));
  ret = f->output_waiting;
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
                           void * (*create_func)(void*), void * data)
  {
  bg_fifo_t * ret;
  frame_t * tmp_frame;
  int i;
  ret = calloc(1, sizeof(*ret));

  //  fprintf(stderr, "bg_fifo_create\n");
  
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
  pthread_mutex_init(&(ret->state_mutex),(pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ret->input_frame_mutex),(pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ret->output_frame_mutex),(pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ret->input_waiting_mutex),(pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ret->output_waiting_mutex),(pthread_mutexattr_t *)0);
  return ret;
  }

void bg_fifo_destroy(bg_fifo_t * f,
                     void (*destroy_func)(void*, void*),
                     void * data)
  {
  int i;
  frame_t * tmp_frame;
  //  fprintf(stderr, "bg_fifo_destroy\n");
  
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
  //  fprintf(stderr, "bg_fifo_lock_read %d\n", *state);
  if(*state != BG_FIFO_PLAYING)
    {
    //    fprintf(stderr, "FIFO isn't playing\n");
    return (void*)0;
    }
  set_output_waiting(f, 1);
  //  fprintf(stderr, "sem wait %p %d\n", f->output_frame, *state);
  sem_wait(&(f->output_frame->produced));
  //  fprintf(stderr, "sem wait done\n");
  
  set_output_waiting(f, 0);
  
  if(f->output_frame->eof)
    {
    //    fprintf(stderr, "FIFO EOF\n");
    *state = BG_FIFO_STOPPED;
    bg_fifo_set_state(f, *state);
    return (void*)0;
    }

  *state = get_state(f);

  return (*state == BG_FIFO_PLAYING) ? f->output_frame->frame : (void*)0;
  }

void * bg_fifo_get_read(bg_fifo_t * f)
  {
  return f->output_frame->frame;
  }


void bg_fifo_unlock_read(bg_fifo_t*f)
  {
  pthread_mutex_lock(&(f->output_frame_mutex));
  //  fprintf(stderr, "unlock read..");
  
  sem_post(&(f->output_frame->consumed));

  if(f->output_frame->eof)
    {
    /* Post the procuced semaphore also so we can
       report EOF more than once */
    sem_post(&(f->output_frame->produced));
    }
  else
    f->output_frame = f->output_frame->next;
  pthread_mutex_unlock(&(f->output_frame_mutex));
  //  fprintf(stderr, "done\n");
  }

void * bg_fifo_lock_write(bg_fifo_t*f, bg_fifo_state_t * state)
  {
  *state = get_state(f);

  if(*state != BG_FIFO_PLAYING)
    return (void *)0;
  
  set_input_waiting(f, 1);
  sem_wait(&(f->input_frame->consumed));
  set_input_waiting(f, 0);

  *state = get_state(f);

  if(*state != BG_FIFO_PLAYING)
    return (void *)0;
  
  return f->input_frame->frame;
  }

void * bg_fifo_try_lock_write(bg_fifo_t*f, bg_fifo_state_t * state)
  {
  *state = get_state(f);
  
  if(*state != BG_FIFO_PLAYING)
    return (void *)0;

  if(sem_trywait(&(f->input_frame->consumed)))
    return (void *)0;
  //  fprintf(stderr, "done\n");
  return f->input_frame->frame;
  }


/*
 *  Unlock frame for writing. If eof == 1,
 *  the frame is invalid and playback will stop
 *  as soon as the fifos are emtpy
 */

void bg_fifo_unlock_write(bg_fifo_t*f, int eof)
  {
  //  fprintf(stderr, "unlock write %p\n", f->input_frame);
  pthread_mutex_lock(&(f->input_frame_mutex));
  f->input_frame->eof = eof;
  sem_post(&(f->input_frame->produced));
  f->input_frame = f->input_frame->next;
  //  fprintf(stderr, "done %d\n", eof);
  pthread_mutex_unlock(&(f->input_frame_mutex));
  }

void bg_fifo_set_state(bg_fifo_t * f, bg_fifo_state_t state)
  {
  pthread_mutex_lock(&(f->state_mutex));
  f->state = state;

  if(f->state != BG_FIFO_PLAYING)
    {
    /* Check whether output thread waits for the semaphore */
    if(get_output_waiting(f))
      {
      sem_post(&(f->output_frame->produced));
      }
    /* Check whether input thread waits for the semaphore */
    if(get_input_waiting(f))
      {
      sem_post(&(f->input_frame->consumed));
      }
    } 

  pthread_mutex_unlock(&(f->state_mutex));
  }

void bg_fifo_clear(bg_fifo_t * f)
  {
  int i;
  frame_t * tmp_frame;
  
  tmp_frame = f->frames;
  for(i = 0; i < f->num_frames; i++)
    {
    sem_destroy(&(tmp_frame->produced));
    sem_destroy(&(tmp_frame->consumed));
    
    sem_init(&(tmp_frame->produced), 0, 0);
    sem_init(&(tmp_frame->consumed), 0, 1);
    tmp_frame = tmp_frame->next;
    tmp_frame->eof = 0;
    }
  f->output_frame = f->input_frame;
  }
