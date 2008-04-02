/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <string.h>
#include <stdlib.h>

#include <avdec_private.h>
#include <mxf.h>

/* TODO: Find a better way */
static int probe_mxf(bgav_input_context_t * input)
  {
  char * pos;
  if(input->filename)
    {
    pos = strrchr(input->filename, '.');
    if(!pos)
      return 0;
    if(!strcasecmp(pos, ".mxf"))
      return 1;
    }
  return 0;
  }

typedef struct
  {
  mxf_file_t mxf;
  } mxf_t;

static int open_mxf(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  mxf_t * priv;

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  if(!bgav_mxf_file_read(ctx->input, &priv->mxf))
    {
    fprintf(stderr, "bgav_mxf_file_read failed\n"); 
    return 0;
    }
  bgav_mxf_file_dump(&priv->mxf);
  return 0;
  }

static int next_packet_mxf(bgav_demuxer_context_t * ctx)
  {
  return 0;
  
  }

static void seek_mxf(bgav_demuxer_context_t * ctx, int64_t time,
                    int scale)
  {
  
  }

static int select_track_mxf(bgav_demuxer_context_t * ctx, int track)
  {
  return 0;
  
  }

static void close_mxf(bgav_demuxer_context_t * ctx)
  {
  mxf_t * priv;
  priv = (mxf_t*)ctx->priv;
  bgav_mxf_file_free(&priv->mxf);
  free(priv);
  }

static void resync_mxf(bgav_demuxer_context_t * ctx, bgav_stream_t * s)
  {
  
  }

const bgav_demuxer_t bgav_demuxer_mxf =
  {
    .probe        = probe_mxf,
    .open         = open_mxf,
    .select_track = select_track_mxf,
    .next_packet  =  next_packet_mxf,
    .resync       = resync_mxf,
    .seek         = seek_mxf,
    .close        = close_mxf
  };

