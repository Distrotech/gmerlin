/*****************************************************************
 * gavl - a general purpose audio/video processing library
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
#include <gavl/metadata.h>
#include <gavl/metatags.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char * my_strdup(const char * str)
  {
  char * ret;
  int len = strlen(str) + 1;

  ret = malloc(len);
  strncpy(ret, str, len);
  return ret;
  }


void
gavl_metadata_free(gavl_metadata_t * m)
  {
  int i;
  for(i = 0; i < m->num_tags; i++)
    {
    free(m->tags[i].key);
    free(m->tags[i].val);
    }
  if(m->tags)
    free(m->tags);
  gavl_metadata_init(m);
  }

void
gavl_metadata_copy(gavl_metadata_t * dst,
                   const gavl_metadata_t * src)
  {
  int i;

  if(!src->tags_alloc)
    return;

  for(i = 0; i < src->num_tags; i++)
    gavl_metadata_set(dst, src->tags[i].key, src->tags[i].val); 
  }

void
gavl_metadata_init(gavl_metadata_t * m)
  {
  memset(m, 0, sizeof(*m));
  }


static int find_tag(const gavl_metadata_t * m, const char * key)
  {
  int i;
  for(i = 0; i < m->num_tags; i++)
    {
    if(!strcmp(m->tags[i].key, key))
      return i;
    }
  return -1;
  }

void
gavl_metadata_set(gavl_metadata_t * m,
                  const char * key,
                  const char * val_c)
  {
  char * val;
  if(val_c && (*val_c != '\0'))
    val = my_strdup(val_c);
  else
    val = NULL;
  gavl_metadata_set_nocpy(m, key, val);
  }


void
gavl_metadata_set_nocpy(gavl_metadata_t * m,
                        const char * key,
                        char * val)
  {
  int idx = find_tag(m, key);

  if(idx >= 0) // Tag exists
    {
    if(m->tags[idx].val)
      free(m->tags[idx].val);
    if(val && (*val != '\0')) // Replace tag
      m->tags[idx].val = val;
    else // Delete tag
      {
      if(idx < (m->num_tags - 1))
        {
        memmove(m->tags + idx, m->tags + idx + 1,
                (m->num_tags - 1 - idx) * sizeof(*m->tags));
        }
      m->num_tags--;
      }
    }
  else
    {
    if(val && (*val != '\0')) // Add new tag
      {
      if(m->num_tags + 1 > m->tags_alloc)
        {
        m->tags_alloc = m->num_tags + 16;
        m->tags = realloc(m->tags,
                          m->tags_alloc * sizeof(*m->tags));
        }
      m->tags[m->num_tags].key = my_strdup(key);
      m->tags[m->num_tags].val = val;
      m->num_tags++;
      }
    }
  }

#define STR_SIZE 128

void
gavl_metadata_set_int(gavl_metadata_t * m,
                      const char * key,
                      int val)
  {
  char str[STR_SIZE];
  snprintf(str, STR_SIZE, "%d", val);
  gavl_metadata_set(m, key, str);
  }

void
gavl_metadata_set_long(gavl_metadata_t * m,
                       const char * key,
                       int64_t val)
  {
  char str[STR_SIZE];
  snprintf(str, STR_SIZE, "%"PRId64, val);
  gavl_metadata_set(m, key, str);
  }


#undef STR_SIZE

const char * gavl_metadata_get(const gavl_metadata_t * m,
                               const char * key)
  {
  int idx = find_tag(m, key);
  if(idx < 0)
    return NULL;
  return m->tags[idx].val;
  }


int gavl_metadata_get_int(const gavl_metadata_t * m,
                          const char * key, int * ret)
  {
  char * rest;
  const char * val_str = gavl_metadata_get(m, key);
  if(!val_str)
    return 0;
  *ret = strtol(val_str, &rest, 10);
  if(*rest != '\0')
    return 0;
  return 1;
  }

int gavl_metadata_get_long(const gavl_metadata_t * m,
                           const char * key, int64_t * ret)
  {
  char * rest;
  const char * val_str = gavl_metadata_get(m, key);
  if(!val_str)
    return 0;
  *ret = strtoll(val_str, &rest, 10);
  if(*rest != '\0')
    return 0;
  return 1;
  }

/* Time <-> String */

void
gavl_metadata_date_to_string(int year,
                             int month,
                             int day, char * ret)
  {
  snprintf(ret, GAVL_METADATA_DATE_STRING_LEN,
           "%04d-%02d-%02d", year, month, day);
  }

void
gavl_metadata_date_time_to_string(int year,
                                  int month,
                                  int day,
                                  int hour,
                                  int minute,
                                  int second,
                                  char * ret)
  {
  snprintf(ret, GAVL_METADATA_DATE_TIME_STRING_LEN,
           "%04d-%02d-%02d %02d:%02d:%02d",
           year, month, day, hour, minute, second);
  }

void
gavl_metadata_set_date(gavl_metadata_t * m,
                       const char * key,
                       int year,
                       int month,
                       int day)
  {
  // YYYY-MM-DD
  char buf[GAVL_METADATA_DATE_STRING_LEN];
  gavl_metadata_date_to_string(year, month, day, buf);
  gavl_metadata_set(m, key, buf);
  }

void
gavl_metadata_set_date_time(gavl_metadata_t * m,
                            const char * key,
                            int year,
                            int month,
                            int day,
                            int hour,
                            int minute,
                            int second)
  {
  // YYYY-MM-DD HH:MM:SS
  char buf[GAVL_METADATA_DATE_TIME_STRING_LEN];
  gavl_metadata_date_time_to_string(year,
                                    month,
                                    day,
                                    hour,
                                    minute,
                                    second,
                                    buf);
  gavl_metadata_set(m, key, buf);
  }

GAVL_PUBLIC int
gavl_metadata_get_date(gavl_metadata_t * m,
                       const char * key,
                       int * year,
                       int * month,
                       int * day)
  {
  const char * val = gavl_metadata_get(m, key);
  if(!val)
    return 0;

  if(sscanf(val, "%04d-%02d-%02d", year, month, day) < 3)
    return 0;
  return 1;
  }

GAVL_PUBLIC int
gavl_metadata_get_date_time(gavl_metadata_t * m,
                            const char * key,
                            int * year,
                            int * month,
                            int * day,
                            int * hour,
                            int * minute,
                            int * second)
  {
  const char * val = gavl_metadata_get(m, key);
  if(!val)
    return 0;

  if(sscanf(val, "%04d-%02d-%02d %02d:%02d:%02d",
            year, month, day, hour, minute, second) < 6)
    return 0;
  return 1;
  }



void gavl_metadata_merge(gavl_metadata_t * dst,
                         const gavl_metadata_t * src1,
                         const gavl_metadata_t * src2)
  {
  int i;
  /* src1 has priority */
  for(i = 0; i < src1->num_tags; i++)
    gavl_metadata_set(dst, src1->tags[i].key, src1->tags[i].val);

  /* From src2 we take only the tags, which aren't available */
  for(i = 0; i < src2->num_tags; i++)
    {
    if(!gavl_metadata_get(dst, src2->tags[i].key))
      gavl_metadata_set(dst, src2->tags[i].key,
                        src2->tags[i].val);
    }
  }

void gavl_metadata_merge2(gavl_metadata_t * dst,
                          const gavl_metadata_t * src)
  {
  int i;
  for(i = 0; i < src->num_tags; i++)
    {
    if(!gavl_metadata_get(dst, src->tags[i].key))
      gavl_metadata_set(dst,
                        src->tags[i].key,
                        src->tags[i].val);
    }
  }

GAVL_PUBLIC void
gavl_metadata_dump(const gavl_metadata_t * m, int indent)
  {
  int len, i, j;
  int max_key_len = 0;
  
  for(i = 0; i < m->num_tags; i++)
    {
    len = strlen(m->tags[i].key);
    if(len > max_key_len)
      max_key_len = len;
    }
  
  for(i = 0; i < m->num_tags; i++)
    {
    len = strlen(m->tags[i].key);

    for(j = 0; j < indent; j++)
      fprintf(stderr, " ");

    fprintf(stderr, "%s: ", m->tags[i].key);

    for(j = 0; j < max_key_len - len; j++)
      fprintf(stderr, " ");

    fprintf(stderr, "%s\n", m->tags[i].val);
    }
  }

int
gavl_metadata_equal(const gavl_metadata_t * m1,
                    const gavl_metadata_t * m2)
  {
  int i;
  const char * val;

  /* Check if tags from m1 are present in m2 */
  for(i = 0; i < m1->num_tags; i++)
    {
    val = gavl_metadata_get(m2, m1->tags[i].key);
    if(!val || strcmp(val, m1->tags[i].val))
      return 0;
    }
  
  /* Check if tags from m2 are present in m1 */
  for(i = 0; i < m2->num_tags; i++)
    {
    if(!gavl_metadata_get(m1, m2->tags[i].key))
      return 0;
    }
  return 1;
  }

static const char * compression_fields[] =
  {
    GAVL_META_SOFTWARE,
    GAVL_META_FORMAT,
    GAVL_META_BITRATE,
    GAVL_META_AUDIO_BITS,
    GAVL_META_VIDEO_BPP,
    GAVL_META_VENDOR,
    NULL,
  };

static void
delete_fields(gavl_metadata_t * m, const char * fields[])
  {
  int found;
  int i, j;

  i = 0;
  while(i < m->num_tags)
    {
    j = 0;

    found = 0;
    
    while(fields[j])
      {
      if(!strcmp(fields[j], m->tags[i].key))
        {
        gavl_metadata_set(m, fields[j], NULL);
        found = 1;
        break;
        }
      j++;
      }
    if(!found)
      i++;
    }
  }

void
gavl_metadata_delete_compression_fields(gavl_metadata_t * m)
  {
  delete_fields(m, compression_fields);
  }
