/*****************************************************************
 * gavl - a general purpose audio/video processing library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <config.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

#ifdef HAVE_CLOCK_MONOTONIC
#include <time.h>
#include <errno.h>
#endif

#include <gavltime.h>
#include <arith128.h>


/* Sleep for a specified time */
#ifdef HAVE_CLOCK_MONOTONIC
void gavl_time_delay(gavl_time_t * t)
  {
  struct timespec tm;
  struct timespec rem;

  tm.tv_sec = *t / 1000000;
  tm.tv_nsec = (*t % 1000000)*1000;
  
  while(clock_nanosleep(CLOCK_MONOTONIC, 0,
                        &tm, &rem))
    {
    if(errno == EINTR)
      {
      tm.tv_sec = rem.tv_sec;
      tm.tv_nsec = rem.tv_nsec;
      }
     else
       break;
     }
  }
#else
void gavl_time_delay(gavl_time_t * t)
  {
  struct timeval tv;
  
  tv.tv_sec  = *t / 1000000;
  tv.tv_usec = *t % 1000000;
  select(0, NULL, NULL, NULL, &tv);
  }
#endif

/*
 *  Pretty print a time in the format:
 *  hhh:mm:ss
 */

GAVL_PUBLIC void
gavl_time_prettyprint_ms(gavl_time_t t, char str[GAVL_TIME_STRING_LEN_MS])
  {
  int hours, minutes, seconds, milliseconds;
  char * pos = str;
  
  if(t == GAVL_TIME_UNDEFINED)
    {
    strcpy(str, "-:--.---");
    return;
    }

  if(t < 0)
    {
    t = -t;
    *(pos++) = '-';
    }
  
  milliseconds = (t/1000) % 1000;
  t /= GAVL_TIME_SCALE;
  seconds = t % 60;
  t /= 60;

  minutes = t % 60;
  t /= 60;

  hours = t % 60;
  t /= 60;
  
  if(hours)
    sprintf(str, "%d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
  else
    sprintf(str, "%02d:%02d.%03d", minutes, seconds, milliseconds);

  }

void
gavl_time_prettyprint(gavl_time_t t, char str[GAVL_TIME_STRING_LEN])
  {
  int seconds;
  int minutes;
  int hours;
  
  char * pos = str;
  
  if(t == GAVL_TIME_UNDEFINED)
    {
    strcpy(str, "-:--");
    return;
    }
  
  if(t < 0)
    {
    t = -t;
    *(pos++) = '-';
    }
  
  t /= GAVL_TIME_SCALE;
  
  seconds = t % 60;
  t /= 60;
  minutes = t % 60;
  t /= 60;
  hours = t % 1000;
  
  /* Print hours */
  
  if(hours)
    sprintf(pos, "%d:%02d:%02d", hours, minutes, seconds);
  else
    sprintf(pos, "%d:%02d", minutes, seconds);
  }



/* Scan seconds: format is hhh:mm:ss with hh: hours, mm: minutes, ss: seconds. Seconds can be a fractional
   value (i.e. with decimal point) */

int gavl_time_parse(const char * str, gavl_time_t * ret)
  {
  double seconds_f;
  gavl_time_t seconds_t;
  int i_tmp;
  const char * start;
  const char * end_c;
  char * end;
  *ret = 0;

  start = str;
  if(!isdigit(*start))
    return 0;

  while(1)
    {
    end_c = start;
    while(isdigit(*end_c))
      end_c++;
    
    if(*end_c == '.')
      {
      *ret *= 60;
      /* Floating point seconds */
      seconds_f = strtod(start, &end);
      seconds_t = gavl_seconds_to_time(seconds_f);
      *ret *= GAVL_TIME_SCALE;
      *ret += seconds_t;
      end_c = end;
      return end_c - str;
      }
    else if(*end_c != ':')
      {
      /* Integer seconds */
      i_tmp = strtol(start, &end, 10);
      *ret *= 60;
      *ret += i_tmp;
      *ret *= GAVL_TIME_SCALE;
      end_c = end;
      return end_c - str;
      }
    else
      {
      i_tmp = strtol(start, &end, 10);
      *ret *= 60;
      *ret += i_tmp;
      end++; // ':'
      end_c = end;
      }
    if(*end_c == '\0')
      break;
    start = end_c;
    }
  return 0;
  }

/* Time scaling functions */

/* From the Linux kernel (drivers/scsi/sg.c): */

/*
 * Suppose you want to calculate the formula muldiv(x,m,d)=int(x * m / d)
 * Then when using 32 bit integers x * m may overflow during the calculation.
 * Replacing muldiv(x) by muldiv(x)=((x % d) * m) / d + int(x / d) * m
 * calculates the same, but prevents the overflow when both m and d
 * are "small" numbers (like HZ and USER_HZ).
 * Of course an overflow is inavoidable if the result of muldiv doesn't fit
 * in 32 bits.
 */

/* In our case, X is 64 bit while MUL and DIV are 32 bit, so we can use this here */

#define MULDIV(X,MUL,DIV) ((((X % DIV) * (int64_t)MUL) / DIV) + ((X / DIV) * (int64_t)MUL))

// #define gavl_samples_to_time(rate, samples) (((samples)*GAVL_TIME_SCALE)/(rate))

gavl_time_t gavl_samples_to_time(int samplerate, int64_t samples)
  {
  return MULDIV(samples, GAVL_TIME_SCALE, samplerate);
  }

int64_t gavl_time_to_samples(int samplerate, gavl_time_t time)
  {
  return MULDIV(time, samplerate, GAVL_TIME_SCALE);
  }

gavl_time_t gavl_time_unscale(int scale, int64_t time)
  {
  return MULDIV(time, GAVL_TIME_SCALE, scale);
  }

int64_t gavl_time_scale(int scale, gavl_time_t time)
  {
  return MULDIV(time, scale, GAVL_TIME_SCALE);
  }

int64_t gavl_time_rescale(int scale1, int scale2, int64_t time)
  {
  if(scale1 == scale2)
    return time;
  return MULDIV(time, scale2, scale1);
  }

int64_t gavl_time_to_frames(int rate_num, int rate_den, gavl_time_t time)
  {
  gavl_int128_t n, result;

  /* t * rate_num / (GAVL_TIME_SCALE * rate_den) */
  /* We know, that GAVL_TIME_SCALE * rate_den fits into 64 bit :) */
    
  gavl_int128_mult(time, rate_num, &n);
  gavl_int128_div(&n, (int64_t)rate_den * GAVL_TIME_SCALE,
                  &result);

  /* Assuming the result is smaller than 2^64 bit!! */
  return result.isneg ? -result.lo : result.lo;
  }

gavl_time_t gavl_frames_to_time(int rate_num, int rate_den, int64_t frames)
  {
  gavl_int128_t n, result;
  
  gavl_int128_mult(frames, (int64_t)rate_den * GAVL_TIME_SCALE, &n);
  gavl_int128_div(&n, rate_num, &result);
  
  /* Assuming the result is smaller than 2^64 bit!! */
  return result.isneg ? -result.lo : result.lo;
  }

// ((gavl_time_t)((GAVL_TIME_SCALE*((int64_t)frames)*((int64_t)rate_den))/((int64_t)rate_num)))
