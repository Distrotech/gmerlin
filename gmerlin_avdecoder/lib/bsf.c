/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <stdlib.h>


#include <config.h>
#include <avdec_private.h>
#include <bsf.h>
#include <bsf_private.h>
// #include <utils.h>

static const struct
  {
  uint32_t fourcc;
  void (*init_func)(bgav_bsf_t*);
  }
parsers[] =
  {
    { BGAV_MK_FOURCC('a', 'v', 'c', '1'), bgav_bsf_init_avcC },
  };

bgav_bsf_t * bgav_bsf_create(bgav_stream_t * s)
  {
  bgav_bsf_t * ret;
  int i;
  
  for(i = 0; i < sizeof(parsers)/sizeof(parsers[0]); i++)
    {
    if(s->fourcc == parsers[i].fourcc)
      {
      ret = calloc(1, sizeof(*ret));
      ret->s = s;
      parsers[i].init_func(ret);
      return ret;
      }
    }
  return NULL;
  }

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

void bgav_bsf_destroy(bgav_bsf_t * bsf)
  {
  if(bsf->cleanup)
    bsf->cleanup(bsf);
  if(bsf->ext_data)
    free(bsf->ext_data);
  free(bsf);
  }

const uint8_t * bgav_bsf_get_header(bgav_bsf_t * bsf, int * size)
  {
  *size = bsf->ext_size;
  return bsf->ext_data;
  }
  
