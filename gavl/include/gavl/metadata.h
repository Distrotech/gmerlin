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

void
gavl_metadata_free(gavl_metadata_t * m);

void
gavl_metadata_init(gavl_metadata_t * m);


void
gavl_metadata_set(gavl_metadata_t * m,
                  const char * key,
                  const char * val);

void
gavl_metadata_set_int(gavl_metadata_t * m,
                      const char * key,
                      int val);


const char * gavl_metadata_get(const gavl_metadata_t * m,
                               const char * key);


int gavl_metadata_get_int(const gavl_metadata_t * m,
                          const char * key, int * ret);

void gavl_metadata_merge(gavl_metadata_t * dst,
                         const gavl_metadata_t * src1,
                         const gavl_metadata_t * src2);

void gavl_metadata_merge2(gavl_metadata_t * dst,
                          const gavl_metadata_t * src);


void
gavl_metadata_copy(gavl_metadata_t * dst,
                   const gavl_metadata_t * src);
