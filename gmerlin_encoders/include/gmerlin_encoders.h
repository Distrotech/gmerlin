/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

#include <stdio.h>
#include <gmerlin/plugin.h>

/* ID3 V1.1 and V2.4 support */

typedef struct bgen_id3v1_s bgen_id3v1_t;
typedef struct bgen_id3v2_s bgen_id3v2_t;

bgen_id3v1_t * bgen_id3v1_create(const gavl_metadata_t*);
bgen_id3v2_t * bgen_id3v2_create(const gavl_metadata_t*);

#define ID3_ENCODING_LATIN1    0x00
#define ID3_ENCODING_UTF16_BOM 0x01
#define ID3_ENCODING_UTF16_BE  0x02
#define ID3_ENCODING_UTF8      0x03

int bgen_id3v1_write(FILE * output, const bgen_id3v1_t *);
int bgen_id3v2_write(FILE * output, const bgen_id3v2_t *, int encoding);

void bgen_id3v1_destroy(bgen_id3v1_t *);
void bgen_id3v2_destroy(bgen_id3v2_t *);
