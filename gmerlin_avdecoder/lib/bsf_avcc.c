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
#include <string.h>


#include <config.h>
#include <avdec_private.h>
#include <bsf.h>
#include <bsf_private.h>

static const uint8_t nal_header[4] = { 0x00, 0x00, 0x00, 0x01 };

typedef struct
  {
  int nal_size_length;
  int header_sent;
  } avcc_t;

static void append_data(bgav_packet_t * p, uint8_t * data, int len,
                        int header_len)
  {
  switch(header_len)
    {
    case 0:
      bgav_packet_alloc(p, p->data_size);
      break;
    case 3:
      bgav_packet_alloc(p, p->data_size + 3);
      memcpy(p->data + p->data_size, &nal_header[1], 3);
      p->data_size += 3;
      break;
    case 4:
      bgav_packet_alloc(p, p->data_size + 4);
      memcpy(p->data + p->data_size, nal_header, 4);
      p->data_size += 4;
      break;
    }
  memcpy(p->data + p->data_size, data, len);
  p->data_size += len;
  }

static void
filter_avcc(bgav_bsf_t* bsf, bgav_packet_t * in, bgav_packet_t * out)
  {
  uint8_t * ptr, *end;
  int len;
  int nals_sent = 0;
  avcc_t * priv = bsf->priv;
  
  ptr = in->data;
  end = in->data + in->data_size;

  out->data_size = 0;

  if(!priv->header_sent)
    {
    append_data(out, bsf->s->ext_data, bsf->s->ext_size, 0);
    }
  
  while(ptr < end - priv->nal_size_length)
    {
    switch(priv->nal_size_length)
      {
      case 1:
        len = *ptr;
        ptr++;
        break;
      case 2:
        len = BGAV_PTR_2_16BE(ptr);
        ptr += 2;
        break;
      case 4:
        len = BGAV_PTR_2_32BE(ptr);
        ptr += 4;
        break;
      default:
        break;
      }
    append_data(out, ptr, len, nals_sent ? 3 : 4);
    nals_sent++;
    ptr += len;
    }
  }

static void
cleanup_avcc(bgav_bsf_t * bsf)
  {
  free(bsf->priv);
  }

static void append_extradata(bgav_stream_t * s, uint8_t * data,
                             int len)
  {
  s->ext_data = realloc(s->ext_data, s->ext_size + len + 4);
  memcpy(s->ext_data + s->ext_size, nal_header, 4);
  s->ext_size += 4;
  memcpy(s->ext_data + s->ext_size, data, len);
  s->ext_size += len;
  }

void
bgav_bsf_init_avcC(bgav_bsf_t * bsf)
  {
  uint8_t * ptr;
  avcc_t * priv;
  uint8_t * old_ext_data;
  int old_ext_size;
  int num_units;
  int i;
  int len;
  
  bsf->filter = filter_avcc;
  bsf->cleanup = cleanup_avcc;
  priv = calloc(1, sizeof(*priv));
  bsf->priv = priv;

  /* Parse and replace extradata */
  old_ext_data = bsf->s->ext_data;
  old_ext_size = bsf->s->ext_size;

  bsf->s->ext_data = NULL;
  bsf->s->ext_size = 0;
  
  ptr = bsf->s->ext_data;

  
  ptr += 4; // Version, profile, profile compat, level
  priv->nal_size_length = (*ptr & 0x3) + 1;
  ptr++;

  /* SPS */
  num_units = *ptr & 0x1f; ptr++;
  for(i = 0; i < num_units; i++)
    {
    len = BGAV_PTR_2_16BE(ptr); ptr += 2;
    append_extradata(bsf->s, ptr, len);
    ptr += len;
    }

  /* PPS */
  num_units = *ptr; ptr++;
  for(i = 0; i < num_units; i++)
    {
    len = BGAV_PTR_2_16BE(ptr); ptr += 2;
    append_extradata(bsf->s, ptr, len);
    ptr += len;
    }
  
  }
