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
#include <gavl/metadata.h>

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
  dst->tags = calloc(src->tags_alloc, sizeof(*dst->tags));

  for(i = 0; i < src->num_tags; i++)
    {
    dst->tags[i].key = my_strdup(src->tags[i].key);
    dst->tags[i].val = my_strdup(src->tags[i].val);
    }
  dst->num_tags = src->num_tags;
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
                  const char * val)
  {
  int idx = find_tag(m, key);

  if(idx >= 0) // Tag exists
    {
    free(m->tags[idx].val);
    if(val && (*val != '\0')) // Replace tag
      m->tags[idx].val = my_strdup(val);
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
      if(m->num_tags + 1 < m->tags_alloc)
        {
        m->tags_alloc = m->num_tags + 16;
        m->tags = realloc(m->tags,
                          m->tags_alloc * sizeof(*m->tags));
        }
      m->tags[m->num_tags].key = my_strdup(key);
      m->tags[m->num_tags].val = my_strdup(val);
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
  *ret = strtol(val_str, &rest, 10);
  if(*rest != '\0')
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
