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

/* Generic time type: Microseconds */

#define GAVL_TIME_SCALE     1000000
#define GAVL_TIME_UNDEFINED 0x8000000000000000LL
#define GAVL_TIME_MAX       0x7fffffffffffffffLL

typedef int64_t gavl_time_t;

/* Utility functions */

#define gavl_samples_to_time(rate, samples) \
(((samples)*GAVL_TIME_SCALE)/(rate))

#define gavl_frames_to_time(rate_num, rate_den, frames) \
((gavl_time_t)((GAVL_TIME_SCALE*((int64_t)frames)*((int64_t)rate_den))/((int64_t)rate_num)))

#define gavl_time_to_samples(rate, t) \
  (((t)*(rate))/GAVL_TIME_SCALE)

#define gavl_time_to_frames(rate_num, rate_den, t) \
  (int64_t)(((t)*((int64_t)rate_num))/(GAVL_TIME_SCALE*((int64_t)rate_den)))


// #define GAVL_TIME_TO_SECONDS(t) (double)(t)/(double)(GAVL_TIME_SCALE)

#define gavl_seconds_to_time(s) \
(gavl_time_t)((s)*(double)(GAVL_TIME_SCALE))

#define gavl_time_to_seconds(t) \
((double)t/(double)(GAVL_TIME_SCALE))

/* Simple software timer */

typedef struct gavl_timer_s gavl_timer_t;

gavl_timer_t * gavl_timer_create();
void gavl_timer_destroy(gavl_timer_t *);

void gavl_timer_start(gavl_timer_t *);
void gavl_timer_stop(gavl_timer_t *);

gavl_time_t gavl_timer_get(gavl_timer_t *);
void gavl_timer_set(gavl_timer_t *, gavl_time_t);


/* Sleep for a specified time */

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
