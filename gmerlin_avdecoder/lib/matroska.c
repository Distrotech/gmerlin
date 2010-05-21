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

#include <stdlib.h>

#include <avdec_private.h>
#include <matroska.h>
  
static int mkv_read_num(bgav_input_context_t * ctx, int64_t * ret, int do_mask)
  {
  int shift = 0;

  uint8_t byte;
  uint8_t mask = 0x80;

  /* Get first byte */
  if(!bgav_input_read_8(ctx, &byte))
    return 0;

  while(!(mask & byte) && mask)
    {
    mask >>= 1;
    shift++;
    }

  if(!mask)
    return 0;

  *ret = byte;

  if(do_mask)
    *ret &= (0xff >> (shift+1));
  
  while(shift--)
    {
    if(!bgav_input_read_8(ctx, &byte))
      return 0;

    *ret <<= 8;
    *ret |= byte;
    }
  return 1;
  }

static int mkv_read_uint(bgav_input_context_t * ctx, uint64_t * ret, int bytes)
  {
  uint8_t byte;
  *ret = 0;

  while(bytes--)
    {
    if(!bgav_input_read_8(ctx, &byte))
      return 0;
    *ret <<= 8;
    *ret |= byte;
    }
  return 1;
  }

static int mkv_read_string(bgav_input_context_t * ctx, char ** ret, int bytes)
  {
  *ret = calloc(bytes+1, 1);
  if(bgav_input_read_data(ctx, (uint8_t*)(*ret), bytes) < bytes)
    return 0;
  return 1;
  }
  
int bgav_mkv_read_size(bgav_input_context_t * ctx, int64_t * ret)
  {
  return mkv_read_num(ctx, ret, 1);
  }

int bgav_mkv_read_id(bgav_input_context_t * ctx, int * ret)
  {
  int64_t ret1;
  if(!mkv_read_num(ctx, &ret1, 0))
    return 0;
  *ret = ret1;
  return 1;
  }
  
int bgav_mkv_element_read(bgav_input_context_t * ctx, bgav_mkv_element_t * ret)
  {
  if(!bgav_mkv_read_id(ctx, &ret->id) ||
     !bgav_mkv_read_size(ctx, &ret->size))
    return 0;
  ret->end = ctx->position + ret->size;
  return 1;
  }

void bgav_mkv_element_dump(const bgav_mkv_element_t * ret)
  {
  bgav_dprintf("Matroska element\n");
  bgav_dprintf("  ID:   %x\n", ret->id);
  bgav_dprintf("  Size: %"PRId64"\n", ret->size);
  bgav_dprintf("  End:  %"PRId64"\n", ret->end);
  }

int bgav_mkv_ebml_header_read(bgav_input_context_t * ctx,
                              bgav_mkv_ebml_header_t * ret)
  {
  bgav_mkv_element_t e;
  bgav_mkv_element_t e1;
  uint64_t tmp_64;

  // Set defaults
  ret->version      = 1;
  ret->read_version = 1;
  ret->max_id_length     = 4;
  ret->max_size_length   = 8;
  
  ret->doc_type_version      = 1;
  ret->doc_type_read_version = 1;

  /* Read stuff */

  if(!bgav_mkv_element_read(ctx, &e))
    return 0;

  if(e.id != MKV_ID_EBML)
    return 0;

  bgav_mkv_element_dump(&e);

  while(ctx->position < e.end)
    {
    if(!bgav_mkv_element_read(ctx, &e1))
      return 0;

    switch(e1.id)
      {
      case MKV_ID_EBML_VERSION:
        if(!mkv_read_uint(ctx, &tmp_64, e1.size))
          return 0;
        ret->version = tmp_64;
        break;
      case MKV_ID_EBML_READ_VERSION:
        if(!mkv_read_uint(ctx, &tmp_64, e1.size))
          return 0;
        ret->read_version = tmp_64;
        break;
      case MKV_ID_EBML_MAX_ID_LENGTH:
        if(!mkv_read_uint(ctx, &tmp_64, e1.size))
          return 0;
        ret->max_id_length = tmp_64;
        break;
      case MKV_ID_EBML_MAX_SIZE_LENGTH:
        if(!mkv_read_uint(ctx, &tmp_64, e1.size))
          return 0;
        ret->max_size_length = tmp_64;
        break;
      case MKV_ID_DOC_TYPE:
        if(!mkv_read_string(ctx, &ret->doc_type, e1.size)) 
          return 0;
        break;
      case MKV_ID_DOC_VERSION:
        if(!mkv_read_uint(ctx, &tmp_64, e1.size))
          return 0;
        ret->doc_type_version = tmp_64;
        break;
      case MKV_ID_DOC_READ_VERSION:
        if(!mkv_read_uint(ctx, &tmp_64, e1.size))
          return 0;
        ret->doc_type_read_version = tmp_64;
        break;
      default:
        bgav_input_skip(ctx, e1.size);
      }
    }
  return 1;
  }

void bgav_mkv_ebml_header_dump(const bgav_mkv_ebml_header_t * ret)
  {
  bgav_dprintf("EBML Header\n");
  bgav_dprintf("  EBMLVersion:        %d\n", ret->version);
  bgav_dprintf("  EBMLReadVersion:    %d\n", ret->read_version);
  bgav_dprintf("  EBMLMaxIDLength:    %d\n", ret->max_id_length);
  bgav_dprintf("  EBMLMaxSizeLength:  %d\n", ret->max_size_length);
  bgav_dprintf("  DocType:            %s\n", ret->doc_type);
  bgav_dprintf("  DocTypeVersion:     %d\n", ret->doc_type_version);
  bgav_dprintf("  DocTypeReadVersion: %d\n", ret->doc_type_read_version);
  
  }
