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

#ifndef GAVL_METADATA_H_INCLUDED
#define GAVL_METADATA_H_INCLUDED

#include <gavl/gavldefs.h>

// YYYY-MM-DD
#define GAVL_METADATA_DATE_STRING_LEN 11

// YYYY-MM-DD HH:MM:SS
#define GAVL_METADATA_DATE_TIME_STRING_LEN 20

typedef struct
  {
  char * key;
  char * val;
  } gavl_metadata_tag_t;

typedef struct
  {
  gavl_metadata_tag_t * tags;
  int tags_alloc;
  int num_tags;
  } gavl_metadata_t;

GAVL_PUBLIC void
gavl_metadata_free(gavl_metadata_t * m);

GAVL_PUBLIC void
gavl_metadata_init(gavl_metadata_t * m);


GAVL_PUBLIC void
gavl_metadata_set(gavl_metadata_t * m,
                  const char * key,
                  const char * val);

GAVL_PUBLIC void
gavl_metadata_set_nocpy(gavl_metadata_t * m,
                        const char * key,
                        char * val);

GAVL_PUBLIC void
gavl_metadata_set_int(gavl_metadata_t * m,
                      const char * key,
                      int val);
GAVL_PUBLIC void
gavl_metadata_set_date(gavl_metadata_t * m,
                       const char * key,
                       int year,
                       int month,
                       int day);

GAVL_PUBLIC void
gavl_metadata_date_to_string(int year,
                             int month,
                             int day, char * ret);

GAVL_PUBLIC void
gavl_metadata_date_time_to_string(int year,
                                  int month,
                                  int day,
                                  int hour,
                                  int minute,
                                  int second,
                                  char * ret);


GAVL_PUBLIC void
gavl_metadata_set_date_time(gavl_metadata_t * m,
                            const char * key,
                            int year,
                            int month,
                            int day,
                            int hour,
                            int minute,
                            int second);

GAVL_PUBLIC int
gavl_metadata_get_date(gavl_metadata_t * m,
                       const char * key,
                       int * year,
                       int * month,
                       int * day);

GAVL_PUBLIC int
gavl_metadata_get_date_time(gavl_metadata_t * m,
                            const char * key,
                            int * year,
                            int * month,
                            int * day,
                            int * hour,
                            int * minute,
                            int * second);



GAVL_PUBLIC 
const char * gavl_metadata_get(const gavl_metadata_t * m,
                               const char * key);

GAVL_PUBLIC 
int gavl_metadata_get_int(const gavl_metadata_t * m,
                          const char * key, int * ret);

GAVL_PUBLIC 
void gavl_metadata_merge(gavl_metadata_t * dst,
                         const gavl_metadata_t * src1,
                         const gavl_metadata_t * src2);

GAVL_PUBLIC
void gavl_metadata_merge2(gavl_metadata_t * dst,
                          const gavl_metadata_t * src);


GAVL_PUBLIC void
gavl_metadata_copy(gavl_metadata_t * dst,
                   const gavl_metadata_t * src);

GAVL_PUBLIC void
gavl_metadata_dump(const gavl_metadata_t * m, int indent);

GAVL_PUBLIC int
gavl_metadata_equal(const gavl_metadata_t * m1,
                    const gavl_metadata_t * m2);


#endif // GAVL_METADATA_H_INCLUDED
