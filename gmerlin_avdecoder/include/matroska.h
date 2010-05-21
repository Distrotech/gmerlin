/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

int bgav_mkv_read_id(bgav_input_context_t * ctx, int * ret);
int bgav_mkv_read_size(bgav_input_context_t * ctx, int64_t * ret);


typedef struct
  {
  int id;
  int64_t size;
  int64_t end;
  } bgav_mkv_element_t;


int bgav_mkv_element_read(bgav_input_context_t * ctx, bgav_mkv_element_t * ret);
void bgav_mkv_element_dump(const bgav_mkv_element_t * ret);

typedef struct
  {
  int version;
  int read_version;
  int max_id_length;
  int max_size_length;
  char * doc_type;
  int doc_type_version;
  int doc_type_read_version;
  } bgav_mkv_ebml_header_t;

int bgav_mkv_ebml_header_read(bgav_input_context_t * ctx, bgav_mkv_ebml_header_t * ret);
void bgav_mkv_ebml_header_dump(const bgav_mkv_ebml_header_t * ret);


/* Known IDs */
#define MKV_ID_EBML                 0x1a45dfa3
#define MKV_ID_EBML_VERSION         0x4286
#define MKV_ID_EBML_READ_VERSION    0x42F7
#define MKV_ID_EBML_MAX_ID_LENGTH   0x42F2
#define MKV_ID_EBML_MAX_SIZE_LENGTH 0x42F3
#define MKV_ID_DOC_TYPE             0x4282
#define MKV_ID_DOC_VERSION          0x4287
#define MKV_ID_DOC_READ_VERSION     0x4285

#define MKV_ID_CRC32                0xbf
#define MKV_ID_VOID                 0xec

#define MKV_ID_SEGMENT              0x18538067
