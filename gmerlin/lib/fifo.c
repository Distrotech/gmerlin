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
                          void (*destroy_frame)(void*, void*),
                          void * data)
  {
  sem_destroy(&(f->produced));
  sem_destroy(&(f->consumed));
  destroy_frame(data, f->frame);
  free(f);
  }

/* Common Fifo structure */

struct bg_fifo_s
  {
  frame_t * frames; /* Chained list */
  int num_frames;

  frame_t *  input_frame;
  frame_t * output_frame;
  pthread_mutex_t  input_frame_mutex;
  pthread_mutex_t output_frame_mutex;
  
  pthread_mutex_t state_mutex;
  bg_fifo_state_t state;
  };

bg_fifo_t * bg_fifo_create(int num_frames,
                           void * (*create_func)(void*), void * data)
  {
  bg_fifo_t * ret;
  frame_t * tmp_frame;
  int i;
  ret = calloc(1, sizeof(*ret));

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
  return ret;
  }

void bg_fifo_destroy(bg_fifo_t * f,
                     void (*destroy_func)(void*, void*),
                     void * data)
  {
  int i;
  frame_t * tmp_frame;
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
  pthread_mutex_lock(&(f->state_mutex));
  if(f->state != BG_FIFO_NORMAL)
    {
    *state = f->state;
    pthread_mutex_unlock(&(f->state_mutex));
    return (void*)0;
    }
  pthread_mutex_unlock(&(f->state_mutex));
  pthread_mutex_lock(&(f->output_frame_mutex));
  sem_wait(&(f->output_frame->produced));
  pthread_mutex_unlock(&(f->output_frame_mutex));
  if(f->output_frame->eof)
    {
    //    fprintf(stderr, "FIFO EOF\n");
    *state = BG_FIFO_STOPPED;
    return (void*)0;
    }
  else
    {
    *state = BG_FIFO_NORMAL;
    return f->output_frame->frame;
    }
  }

void * bg_fifo_get_read(bg_fifo_t * f)
  {
  return f->output_frame->frame;
  }

  //  fprintf(stderr, "done\n");

void bg_fifo_unlock_read(bg_fifo_t*f)
  {
  pthread_mutex_lock(&(f->output_frame_mutex));
  //  fprintf(stderr, "unlock read..");
  
  sem_post(&(f->output_frame->consumed));
  f->output_frame = f->output_frame->next;
  pthread_mutex_unlock(&(f->output_frame_mutex));

  //  fprintf(stderr, "done\n");
  }

void * bg_fifo_lock_write(bg_fifo_t*f, bg_fifo_state_t * state)
  {
  pthread_mutex_lock(&(f->state_mutex));
  if(f->state != BG_FIFO_NORMAL)
    {
    *state = f->state;
    pthread_mutex_unlock(&(f->state_mutex));
    return (void *)0;
    }
  
  pthread_mutex_unlock(&(f->state_mutex));
  //  fprintf(stderr, "lock write...");
  sem_wait(&(f->input_frame->consumed));
  //  fprintf(stderr, "done\n");
  return f->input_frame->frame;
  }

void * bg_fifo_try_lock_write(bg_fifo_t*f, bg_fifo_state_t * state)
  {
  pthread_mutex_lock(&(f->state_mutex));
  if(f->state != BG_FIFO_NORMAL)
    {
    *state = f->state;
    pthread_mutex_unlock(&(f->state_mutex));
    return (void *)0;
    }
  
  pthread_mutex_unlock(&(f->state_mutex));
  //  fprintf(stderr, "lock write...");
  if(sem_trywait(&(f->input_frame->consumed)))
    return (void *)0;
  //  fprintf(stderr, "done\n");
  return f->input_frame->frame;
  }


/*
 *  Unlock frame for writing, if eof == 1,
 *  the frame is invalid and playback will stop
 *  as soon as the fifos are emtpy
 */

void   bg_fifo_unlock_write(bg_fifo_t*f, int eof)
  {
  //  fprintf(stderr, "unlock write...");
  pthread_mutex_lock(&(f->input_frame_mutex));
  f->input_frame->eof = eof;
  sem_post(&(f->input_frame->produced));
  f->input_frame = f->input_frame->next;
  //  fprintf(stderr, "done\n");
  pthread_mutex_unlock(&(f->input_frame_mutex));
  }

void bg_fifo_set_state(bg_fifo_t * f, bg_fifo_state_t state)
  {
  pthread_mutex_lock(&(f->state_mutex));
  f->state = state;

  if(f->state != BG_FIFO_NORMAL)
    {
    /* Check whether output thread waits for the semaphore */

    pthread_mutex_lock(&(f->input_frame_mutex));

    if(pthread_mutex_trylock(&(f->output_frame_mutex)))
      {
      sem_post(&(f->input_frame->produced));
      }
    else
      pthread_mutex_unlock(&(f->output_frame_mutex));
    /* */
    pthread_mutex_unlock(&(f->input_frame_mutex));
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
    }
  f->output_frame = f->input_frame;
  }
