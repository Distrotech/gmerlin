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


#include <config.h>
#include <avdec_private.h>
#include <bsf.h>
#include <bsf_private.h>
// #include <utils.h>

typedef struct
  {
  uint32_t fourcc;
  void (*init_func)(bgav_bsf_t*);
  } filter_t;

static const filter_t filters[] =
  {
    { BGAV_MK_FOURCC('a', 'v', 'c', '1'), bgav_bsf_init_avcC },
  };

void bgav_bsf_run(bgav_bsf_t * bsf, bgav_packet_t * in, bgav_packet_t * out)
  {
  /* Set packet fields now, so the filter has a chance
     to overwrite them */
  
  out->flags = in->flags;
  out->pts = in->pts;
  out->dts = in->dts;
  out->duration = in->duration;
    
  bsf->filter(bsf, in, out);
  }

gavl_source_status_t
bgav_bsf_get_packet(void * bsf_p, bgav_packet_t ** ret)
  {
  bgav_packet_t * in_packet;
  gavl_source_status_t st;
  bgav_bsf_t * bsf = bsf_p;

  if(bsf->out_packet)
    {
    *ret = bsf->out_packet;
    bsf->out_packet = NULL;
    return GAVL_SOURCE_OK;
    }

  in_packet = NULL;
  if((st = bsf->src.get_func(bsf->src.data, &in_packet)) != GAVL_SOURCE_OK)
    return st;
  
  *ret = bgav_packet_pool_get(bsf->s->pp);
  bgav_bsf_run(bsf, in_packet, *ret);

  bgav_packet_pool_put(bsf->s->pp, in_packet);
  return GAVL_SOURCE_OK;
  }

gavl_source_status_t
bgav_bsf_peek_packet(void * bsf_p, bgav_packet_t ** ret, int force)
  {
  gavl_source_status_t st;
  bgav_bsf_t * bsf = bsf_p;
  bgav_packet_t * in_packet;

  if(bsf->out_packet)
    {
    if(*ret)
      *ret = bsf->out_packet;
    return GAVL_SOURCE_OK;
    }

  if((st = bsf->src.peek_func(bsf->src.data, &in_packet, force)) !=
     GAVL_SOURCE_OK)
    return st;
  
  /* We are eating up this packet so we need to remove it from the
     packet buffer */
  bsf->src.get_func(bsf->src.data, &in_packet);
  
  if(!in_packet)
    return GAVL_SOURCE_EOF; // Impossible but who knows?
  
  bsf->out_packet = bgav_packet_pool_get(bsf->s->pp);
  bgav_bsf_run(bsf, in_packet, bsf->out_packet);
  bgav_packet_pool_put(bsf->s->pp, in_packet);

  if(ret)
    *ret = bsf->out_packet;
  return GAVL_SOURCE_OK;
  }

bgav_bsf_t * bgav_bsf_create(bgav_stream_t * s)
  {
  const filter_t * f = NULL;
  bgav_bsf_t * ret;
  int i;
  
  for(i = 0; i < sizeof(filters)/sizeof(filters[0]); i++)
    {
    if(s->fourcc == filters[i].fourcc)
      {
      f = &filters[i];
      break;
      }
    }
  if(!f)
    return NULL;

  ret = calloc(1, sizeof(*ret));
  ret->s = s;
  bgav_packet_source_copy(&ret->src, &s->src);
  
  s->src.get_func = bgav_bsf_get_packet;
  s->src.peek_func = bgav_bsf_peek_packet;
  s->src.data = ret;
  
  filters[i].init_func(ret);

  if(ret->ext_data)
    {
    ret->ext_data_orig = s->ext_data;
    ret->ext_size_orig = s->ext_size;

    s->ext_data = ret->ext_data;
    s->ext_size = ret->ext_size;
    }
  

  return ret;
  }

void bgav_bsf_destroy(bgav_bsf_t * bsf)
  {
  if(bsf->cleanup)
    bsf->cleanup(bsf);

  /* Restore extradata */
  if(bsf->ext_data_orig)
    {
    bsf->s->ext_data = bsf->ext_data_orig;
    bsf->s->ext_size = bsf->ext_size_orig;
    }

  /* Warning: This breaks when the bsf is *not* the last element
     in the processing chain */
  bgav_packet_source_copy(&bsf->s->src, &bsf->src);
  
  if(bsf->ext_data)
    free(bsf->ext_data);
  free(bsf);
  }

const uint8_t * bgav_bsf_get_header(bgav_bsf_t * bsf, int * size)
  {
  *size = bsf->ext_size;
  return bsf->ext_data;
  }
