/*****************************************************************
 
  gavltime.h
 
  Copyright (c) 2001-2002 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/*! \defgroup time Time
 */

/*! \ingroup time
 */

#define GAVL_TIME_SCALE     1000000 /*!< Generic time scale: Microsecond tics */

/*! \ingroup time
 */

#define GAVL_TIME_UNDEFINED 0x8000000000000000LL /*!< Unknown or undefined time */
/*! \ingroup time
 */
#define GAVL_TIME_MAX       0x7fffffffffffffffLL /*!< Maximum possible value */

/*! \ingroup time
 * \brief Times in gavl are 64 bit signed integers
 * 
 */

typedef int64_t gavl_time_t;

/* Utility functions */

/*! \ingroup time
 * \brief Convert a number of samples to a time for a given samplerate
 */

#define gavl_samples_to_time(rate, samples) \
(((samples)*GAVL_TIME_SCALE)/(rate))

/*! \ingroup time
 * \brief Convert a number of video frames to a time for a given framerate
 */

#define gavl_frames_to_time(rate_num, rate_den, frames) \
((gavl_time_t)((GAVL_TIME_SCALE*((int64_t)frames)*((int64_t)rate_den))/((int64_t)rate_num)))

/*! \ingroup time
 * \brief Convert a time to a number of audio samples for a given samplerate
 * \param rate Samplerate
 * \param t Time
 * \returns Number of audio samples
 */

#define gavl_time_to_samples(rate, t) \
  (((t)*(rate))/GAVL_TIME_SCALE)

/*! \ingroup time
 * \brief Convert a time to a number of video frames for a given framerate
 * \param rate_num Numerator of the framerate
 * \param rate_den Denominator of the framerate
 * \param t Time
 * \returns Number of frames
 */

#define gavl_time_to_frames(rate_num, rate_den, t) \
  (int64_t)(((t)*((int64_t)rate_num))/(GAVL_TIME_SCALE*((int64_t)rate_den)))

/*! \ingroup time
 * \brief Convert a time scaled by another base to a gavl time
 * \param scale Time scale
 * \param time Time scaled by scale
 * \returns Time scaled by \ref GAVL_TIME_SCALE
 */

#define gavl_time_unscale(scale, time) \
(((time)*GAVL_TIME_SCALE)/(scale))

/*! \ingroup time
 * \brief Convert a gavl time to a time scaled by another base 
 * \param scale Time scale
 * \param time Time scaled by \ref GAVL_TIME_SCALE
 * \returns Time scaled by scale
 */

#define gavl_time_scale(scale, time)          \
(((time)*(scale))/GAVL_TIME_SCALE)

/*! \ingroup time
 * \brief Convert seconds (as double) to a gavl time
 * \param s Seconds as double
 * \returns Integer time scaled by \ref GAVL_TIME_SCALE
 */

#define gavl_seconds_to_time(s) \
(gavl_time_t)((s)*(double)(GAVL_TIME_SCALE))

/*! \ingroup time
 * \brief Convert a gavl time to seconds (as double)
 * \param t Integer time scaled by \ref GAVL_TIME_SCALE
 * \returns Seconds as double
 */

#define gavl_time_to_seconds(t) \
((double)t/(double)(GAVL_TIME_SCALE))

/*! \ingroup time
 * \brief Sleep for a specified time
 * \param time Time after which execution of the current thread is resumed
 */

void gavl_time_delay(gavl_time_t * time);

/*
 *  Pretty print a time in the format:
 *  -hhh:mm:ss
 */

#define GAVL_TIME_STRING_LEN 11

void
gavl_time_prettyprint(gavl_time_t time, char string[GAVL_TIME_STRING_LEN]);

void
gavl_time_prettyprint_seconds(int seconds, char string[GAVL_TIME_STRING_LEN]);

/* Simple software timer */

/*! \defgroup timer
 * \ingroup time
 * \brief Software timer
 *
 *  This is a simple software timer, which can be used for synchronization
 *  purposes for cases wherer there is no synchronization with hardware devices
 *  available.
 */

/*! \ingroup timer
 * \brief Opaque timer structure
 *
 * You don't want to know what's inside.
 */

typedef struct gavl_timer_s gavl_timer_t;

/*! \ingroup timer
 * \brief Create a timer
 * \returns A newly allocated timer
 */

gavl_timer_t * gavl_timer_create();

/*! \ingroup timer
 * \brief Destroy a timer
 * \param timer A timer
 *
 * Destroys a timer and frees all associated memory
 */

void gavl_timer_destroy(gavl_timer_t * timer);

/*! \ingroup timer
 * \brief Start a timer
 * \param timer A timer
 */

void gavl_timer_start(gavl_timer_t * timer);

/*! \ingroup timer
 * \brief Stop a timer
 * \param timer A timer
 */

void gavl_timer_stop(gavl_timer_t * timer);

/*! \ingroup timer
 * \brief Get the current time of the timer
 * \param timer A timer
 * \returns Current time
 */

gavl_time_t gavl_timer_get(gavl_timer_t * timer);

/*! \ingroup timer
 * \brief Set the current time of the timer
 * \param timer A timer
 * \param t New time
 */

void gavl_timer_set(gavl_timer_t * timer, gavl_time_t t);
