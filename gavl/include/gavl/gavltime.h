
/* Generic time type: Microseconds */

#define GAVL_TIME_SCALE 1000000

typedef int64_t gavl_time_t;

/* Utility functions */

#if 0
void gavl_samples_to_time(gavl_time_t * ret,
                          int samplerate, int64_t * samples);

void gavl_frames_to_time(gavl_time_t * ret,
                         double framerate, int64_t * frames);

void gavl_time_to_samples(int64_t * ret,
                          int samplerate, gavl_time_t * time);

void gavl_time_to_frames(int64_t * ret,
                         double framerate, gavl_time_t * time);
#endif

#define gavl_samples_to_time(rate, samples) \
(((samples)*1000000)/(rate))

#define gavl_frames_to_time(rate_num, rate_den, frames) \
((gavl_time_t)((1000000*(frames)*(rate_den))/(rate_num)))

#define gavl_time_to_samples(rate, t) \
  (((t)*(rate))/1000000)

#define gavl_time_to_frames(rate_num, rate_den, t) \
  ((int64_t)(((t)*(rate_num))/(1000000*(rate_den)))


#define GAVL_TIME_TO_SECONDS(t) (double)(t)/(double)(GAVL_TIME_SCALE)

#define GAVL_SECONDS_TO_TIME(s) \
(gavl_time_t)((s)*(double)(GAVL_TIME_SCALE))

#define gavl_time_to_seconds(t) \
(double)t/(double)(GAVL_TIME_SCALE)

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
