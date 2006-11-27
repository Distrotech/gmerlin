/*****************************************************************
 
  log.h
 
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

#ifndef __BG_LOG_H_
#define __BG_LOG_H_

/* Gmerlin log facilities */

#include "parameter.h"
#include "msgqueue.h"

/** \defgroup log Logging
 *  \brief Global logging facilities
 *
 *  The logging mechanism is used for everything,
 *  which would be done with fprintf(stderr, ...) in
 *  simpler applications. It is the only mechanism, which
 *  used global variables, i.e. the functions \ref bg_log_set_dest
 *  and \ref bg_log_set_verbose. They should be called just once during
 *  application startup. The function \ref bg_log can, of course, be
 *  called from multiple threads simultaneously.
 */

/** \ingroup log
 *  \brief Log levels
 *
 *  These specify the type and severity of a message. They can also be 
 *  ORed for a call to \ref bg_log_set_verbose.
 **/

typedef enum
  {
    BG_LOG_DEBUG    = 1<<0, //!< Only for programmers, useless for users
    BG_LOG_WARNING  = 1<<1, //!< Something went wrong, but is not fatal
    BG_LOG_ERROR    = 1<<2, //!< Something went wrong, cannot continue
    BG_LOG_INFO     = 1<<3  //!< Something interesting the user might want to know
  } bg_log_level_t;

/** \ingroup log
 *  \brief Log levels
 *  \param level Level
 *  \param domain The name of the volume
 *  \param format Format like for printf
 *
 *  All other arguments must match the format string.
 *
 *  This function either prints a message to stderr (if you didn't case
 *  \ref bg_log_set_dest and level is contained in the mask you passed to
 *  bg_log_set_verbose) or puts a message into the queue you passed to
 *  bg_log_set_dest.
 **/

void bg_log(bg_log_level_t level, const char * domain,
            const char * format, ...);

/** \ingroup log
 *  \brief Set the log destination
 *  \param q Message queue
 *
 *  This sets a global message queue to which log messages will be sent.
 *  The format of the logging messages is simple: The message id (see \ref bg_msg_get_id).
 *  The first two arguments are strings for the domain and the actual message
 *  respectively (see \ref bg_msg_get_arg_string).
 *
 *  Note, that logging will become asynchronous with this method. Also, single threaded
 *  applications always must remember to handle messages from the log queue
 *  after they did something critical.
 */

void bg_log_set_dest(bg_msg_queue_t * q);

/** \ingroup log
 *  \brief Convert a log level to a human readable string
 *  \param level Log level
 *  \returns A human readable string describing the log level
 */

const char * bg_log_level_to_string(bg_log_level_t level);

/** \ingroup log
 *  \brief Set verbosity mask
 *  \param mask ORed log log levels, which should be printed
 *
 *  Note, that this function is not thread save and has no effect
 *  if logging is done with a message queue.
 */

void bg_log_set_verbose(int mask);

#endif // __BG_LOG_H_
