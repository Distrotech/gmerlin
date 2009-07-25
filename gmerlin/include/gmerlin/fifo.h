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

#ifndef __BG_FIFO_H_
#define __BG_FIFO_H_

/** \defgroup fifo Fifos
 *  \brief Thread save fifo buffers for A/V data
 *
 *  Fifos are used in multithreaded realtime applications to transfer
 *  A/V frames from the source to the destination. They have a fixed number of
 *  frames.
 *  
 *  They also allow save suspension and termination of threads 
 *  through the state values returned by the bg_fifo_lock_*
 *  functions.
 *
 *  @{
 */

typedef enum
  {
    BG_FIFO_FINISH_CHANGE = 0,
    BG_FIFO_FINISH_PAUSE,
  } bg_fifo_finish_mode_t;

/** \brief Operation states a fifo can have
 *  
 */

typedef enum
  {
    BG_FIFO_PLAYING, //!< Normal operation
    BG_FIFO_STOPPED, //!< Playback is stopped, threads should be terminated
    BG_FIFO_PAUSED,  //!< Playback is paused, threads should wait until playback is resumed
  } bg_fifo_state_t;

/** \brief Opaque fifo handle
 *
 * You don't want to know what's inside
 */

typedef struct bg_fifo_s bg_fifo_t;

/** \brief Create a fifo
 *  \param num_frames Number of frames
 *  \param create_func Function to be called for each created frame
 *  \param data Argument for create_func
 *  \returns A newly allocated fifo
 */

bg_fifo_t * bg_fifo_create(int num_frames,
                           void * (*create_func)(void*), void * data,
                           bg_fifo_finish_mode_t finish_mode);

/** \brief Destroy a fifo
 *  \param f A fifo
 *  \param destroy_func Function to be called for each frame to destroy it
 *  \param data Argument for destroy_func
 */

void bg_fifo_destroy(bg_fifo_t * f, void (*destroy_func)(void*, void*), void * data);

/** \brief Lock a fifo for reading
 *  \param f A fifo
 *  \param state If non NULL, returns the state of the fifo
 *  \returns A frame or NULL
 *
 *  This function blocks until a new frame is available
 */

void * bg_fifo_lock_read(bg_fifo_t * f, bg_fifo_state_t * state);

/** \brief Lock a fifo for reading (nonblocking)
 *  \param f A fifo
 *  \param state If non NULL, returns the state of the fifo
 *  \returns A frame or NULL
 */


void * bg_fifo_try_lock_read(bg_fifo_t*f, bg_fifo_state_t * state);

/** \brief Check if locking a fifo for reading would succeed
 *  \param f A fifo
 *  \param state Returns the state of the fifo
 *  \param returns 1 if eof is reached
 *  \returns 1 if \ref bg_fifo_lock_read() would return immediately,
 *        0 if it might fail or block.
 *
 *  This function  returning zero is no garantuee, that to further
 *  frames are available. Test the returned state to make sure.
 */

int bg_fifo_test_read(bg_fifo_t * f, bg_fifo_state_t * state,
                      int * eof);

/** \brief Unlock a fifo for reading
 *  \param f A fifo
 */
void   bg_fifo_unlock_read(bg_fifo_t*f);

/** \brief Lock a fifo for writing
 *  \param f A fifo
 *  \param state If non NULL, returns the state of the fifo
 *  \returns A frame or NULL
 *
 *  This function blocks until a new frame is available
 */

void * bg_fifo_lock_write(bg_fifo_t*f, bg_fifo_state_t * state);

/** \brief Lock a fifo for writing (nonblocking)
 *  \param f A fifo
 *  \param state If non NULL, returns the state of the fifo
 *  \returns A frame or NULL
 */

void * bg_fifo_try_lock_write(bg_fifo_t*f, bg_fifo_state_t * state);

/** \brief Check if locking a fifo for writing would succeed
 *  \param f A fifo
 *  \param state Returns the state of the fifo
 *  \returns 1 if \ref bg_fifo_lock_write() would return immediately,
 *        0 if it might fail or block.
 *
 *  This function  returning zero is no garantuee, that to further
 *  frames are available. Test the returned state to make sure.
 */

int bg_fifo_test_write(bg_fifo_t * f, bg_fifo_state_t * state);

/** \brief Unlock a fifo for writing
 *  \param f A fifo
 *  \param eof Set to 1 if EOF (also means, the last frame is invalid)
 */

void   bg_fifo_unlock_write(bg_fifo_t*f, int eof);

/** \brief Set the state of a fifo
 *  \param f A fifo
 *  \param state The new state
 */

void bg_fifo_set_state(bg_fifo_t * f, bg_fifo_state_t state);

/** \brief Clear a fifo
 * \param f A fifo
 *
 * This flushes the fifo but keeps all buffers.
 * It should only be called, if there are no other threads accessing the fifo
 */



void bg_fifo_clear(bg_fifo_t *f );

/** @} */
  

#endif //  __BG_FIFO_H_
