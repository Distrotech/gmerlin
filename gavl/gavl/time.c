#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <inttypes.h>
#include <gavltime.h>
#include <string.h>

/* Sleep for a specified time */

void gavl_time_delay(gavl_time_t * t)
  {
  struct timeval tv;
  
  tv.tv_sec  = *t / 1000000;
  tv.tv_usec = *t % 1000000;
  select(0, NULL, NULL, NULL, &tv);
  }

/*
 *  Pretty print a time in the format:
 *  hhh:mm:ss
 */

static char digit_to_char_array[] = "0123456789 ";

static char digit_to_char(int digit)
  {
  if((digit > 9) || (digit < 0))
    return ' ';
  return digit_to_char_array[digit];
  }

void
gavl_time_prettyprint_seconds(int total_seconds,
                              char ret[GAVL_TIME_STRING_LEN])
  {
  char * pos;
  int seconds;
  int minutes;
  int hours;
  int negative;
  int digits_started;
  if(total_seconds < 0)
    {
    negative = 1;
    total_seconds = -total_seconds;
    }
  else
    {
    negative = 0;
    }
  
  seconds = total_seconds % 60;
  total_seconds /= 60;
  minutes = total_seconds % 60;
  total_seconds /= 60;
  hours = total_seconds;

  pos = ret;
  digits_started = 0;
    
  if(negative)
    {
    *(pos++) = '-';
    }

  /* Print hours */
  
  if(hours / 100)
    {
    *(pos++) = digit_to_char(hours/100);
    digits_started = 1;
    }
  if(digits_started || (hours % 100) / 10)
    {
    *(pos++) = digit_to_char((hours % 100) / 10);
    digits_started = 1;
    }
  if(digits_started || (hours % 10))
    {
    *(pos++) = digit_to_char(hours % 10);
    digits_started = 1;
    }

  if(digits_started)
    *(pos++) = ':';


  if(digits_started || (minutes / 10))
    {
    *(pos++) = digit_to_char(minutes / 10);
    digits_started = 1;
    }

  *(pos++) = digit_to_char(minutes % 10);
  
  *(pos++) = ':';
  *(pos++) =   digit_to_char(seconds / 10);
  *(pos++) =   digit_to_char(seconds % 10);
  *pos = '\0';
 
  }

void
gavl_time_prettyprint(gavl_time_t time, char ret[GAVL_TIME_STRING_LEN])
  {
  int total_seconds;

  if(time == GAVL_TIME_UNDEFINED)
    {
    strcpy(ret, "-:--");
    }
  else
    {
    total_seconds = time / 1000000;
    gavl_time_prettyprint_seconds(total_seconds, ret);
    }
  }
