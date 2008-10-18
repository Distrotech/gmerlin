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

#ifndef __BG_LOG_H_
#define __BG_LOG_H_

/* Gmerlin log facilities */

#include <gmerlin/parameter.h>
#include <gmerlin/msgqueue.h>

#include <libintl.h>

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

#define BG_LOG_LEVEL_MAX (1<<3)

/** \ingroup log
 *  \brief Send a message to the logger without translating it.
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

void bg_log_notranslate(bg_log_level_t level, const char * domain,
                        const char * format, ...) __attribute__ ((format (printf, 3, 4)));

/** \ingroup log
 *  \brief Send a message (as complete string) to the logger without translating it.
 *  \param level Level
 *  \param domain The name of the volume
 *  \param str Message string
 *
 *  All other arguments must match the format string.
 *
 *  This function either prints a message to stderr (if you didn't case
 *  \ref bg_log_set_dest and level is contained in the mask you passed to
 *  bg_log_set_verbose) or puts a message into the queue you passed to
 *  bg_log_set_dest.
 **/

void bg_logs_notranslate(bg_log_level_t level, const char * domain,
                         const char * str);



/** \ingroup log
 *  \brief Translate a message and send it to the logger.
 *  \param translation_domain Gettext domain (usually package name)
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

void bg_log_translate(const char * translation_domain,
                      bg_log_level_t level, const char * domain,
                      const char * format, ...) __attribute__ ((format (printf, 4, 5)));

/** \ingroup log
 *  \brief Translate a message and send it to the logger 
 */

#define bg_log(level, domain, ...) \
    bg_log_translate(PACKAGE, level, domain, __VA_ARGS__)


/** \ingroup log
 *  \brief Set the log destination
 *  \param q Message queue
 *
 *  This sets a global message queue to which log messages will be sent.
 *  The format of the logging messages is simple: The message id is equal
 *  to the log level (see \ref bg_msg_get_id).
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
 *  \param mask ORed log levels, which should be printed
 *
 *  Note, that this function is not thread save and has no effect
 *  if logging is done with a message queue.
 */

void bg_log_set_verbose(int mask);

#endif // __BG_LOG_H_
