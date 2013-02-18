/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <stdlib.h>
#include <string.h>


#include <config.h>
#include <avdec_private.h>
#include <bsf.h>
#include <bsf_private.h>
#include <adts_header.h>

#include AVCODEC_HEADER

static void
filter_adts(bgav_bsf_t* bsf, bgav_packet_t * in, bgav_packet_t * out)
  {
  bgav_adts_header_t h;
  int header_len = 7;
  
  if((in->data_size < 7) || !bgav_adts_header_read(in->data, &h))
    {
    out->data_size = 0;
    return;
    }

  if(!h.protection_absent)
    header_len += 2;

  bgav_packet_alloc(out, in->data_size - header_len);
  memcpy(out->data, in->data + header_len, in->data_size - header_len);
  out->data_size = in->data_size - header_len;
  }

static void
cleanup_adts(bgav_bsf_t * bsf)
  {

  }

int
bgav_bsf_init_adts(bgav_bsf_t * bsf)
  {
  int ret = 0;
  bgav_packet_t * p = NULL;
  AVCodecContext * ctx = NULL;
  AVBitStreamFilterContext * b;
  uint8_t * poutbuf;
  int poutbuf_size;
  
  /* Fire up a bitstream filter for getting the extradata.
     We'll do the rest by ourselfes */

  b = av_bitstream_filter_init("aac_adtstoasc");
  if(!b)
    goto fail;

  ctx = calloc(1, sizeof(*ctx));
  
  /* Get a first packet to obtain the extradata */

  if(bsf->src.peek_func(bsf->src.data, &p, 1) != GAVL_SOURCE_OK)
    goto fail;

  //  bgav_packet_dump(p);
  
  if(av_bitstream_filter_filter(b, ctx, NULL,
                                &poutbuf, &poutbuf_size,
                                p->data, p->data_size, 1))
    goto fail;

  /* Extract extradata */

  if(!ctx->extradata_size)
    goto fail;

  bsf->ext_data = malloc(ctx->extradata_size + GAVL_PACKET_PADDING );
  memcpy(bsf->ext_data, ctx->extradata, ctx->extradata_size);
  memset(bsf->ext_data + ctx->extradata_size, 0, GAVL_PACKET_PADDING);
  bsf->ext_size = ctx->extradata_size;

  bsf->filter = filter_adts;
  bsf->cleanup = cleanup_adts;
  
  ret = 1;
  fail:

  if(ctx)
    {
    if(ctx->extradata)
      av_free(ctx->extradata);
    free(ctx);
    }

  if(b)
    av_bitstream_filter_close(b);
  
  return ret;
  }
