/*****************************************************************
 
  fifo.h
 
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

#ifndef __BG_FIFO_H_
#define __BG_FIFO_H_

/* Thread save fifo classes */

/*
 *  They also allow save suspension and termination of threads 
 *  through the state values returned by the bg_fifo_lock_*
 *  functions
 */

typedef enum
  {
    BG_FIFO_NORMAL,  /* Normal operation */
    BG_FIFO_STOPPED, /* Playback is stopped, threads should be terminated */
    BG_FIFO_PAUSED,   /* Playback is paused for a time, threads should wait until
                        playback starts again */
  } bg_fifo_state_t;

typedef struct bg_fifo_s bg_fifo_t;

/*
 *  Create fifos,
 *  cnv_output: if 1 format conversion will happen in the output thread
 *  input_format: Input format
 *  output_format: Output format
 */

bg_fifo_t * bg_fifo_create(int num_frames,
                           void * (*create_func)(void*), void * data);

void bg_fifo_destroy(bg_fifo_t * , void (*destroy_func)(void*, void*), void * data);

void * bg_fifo_lock_read(bg_fifo_t*, bg_fifo_state_t * state);
void * bg_fifo_get_read(bg_fifo_t * f);

void   bg_fifo_unlock_read(bg_fifo_t*);

void * bg_fifo_lock_write(bg_fifo_t*, bg_fifo_state_t * state);
void * bg_fifo_try_lock_write(bg_fifo_t*, bg_fifo_state_t * state);

void   bg_fifo_unlock_write(bg_fifo_t*, int eof);

void bg_fifo_set_state(bg_fifo_t *, bg_fifo_state_t state);

/* This can only be called, if there are no other threads are running */

void bg_fifo_clear(bg_fifo_t *);

  

#endif //  __BG_FIFO_H_
