/*****************************************************************
 * gavl - a general purpose audio/video processing library
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

#include <gavl/gavl.h>
#include <gavl/compression.h>

#include <stdlib.h>

void gavl_compression_info_free(gavl_compression_info_t * info)
  {
  if(info->global_header)
    free(info->global_header);
  }

struct
  {
  gavl_codec_id_t id;
  const char * extension;
  int separate;
  }
compression_ids[] =
  {
    { GAVL_CODEC_ID_ALAW,      NULL,  0 },
    { GAVL_CODEC_ID_ULAW,      NULL,  0 },
    { GAVL_CODEC_ID_MP2,       "mp2", 0 },
    { GAVL_CODEC_ID_MP3_CBR,   "mp3", 0 },
    { GAVL_CODEC_ID_MP3_VBR,   "mp3", 0 },
    { GAVL_CODEC_ID_AC3,       NULL,  0 },
    { GAVL_CODEC_ID_AAC_RAW,   NULL,  0 },
    { GAVL_CODEC_ID_VORBIS,    NULL,  0 },
    
    /* Video */
    { GAVL_CODEC_ID_JPEG,      "jpg", 1 },
    { GAVL_CODEC_ID_PNG,       "png", 1 },
    { GAVL_CODEC_ID_TIFF,      "tif", 1 },
    { GAVL_CODEC_ID_TGA,       "tga", 0 },
    { GAVL_CODEC_ID_MPEG1,     "mpv", 0 },
    { GAVL_CODEC_ID_MPEG2,     "mpv", 0 },
    { GAVL_CODEC_ID_MPEG4_ASP, "m4v", 0 },
    { GAVL_CODEC_ID_H264,      "h264",0 },
    { GAVL_CODEC_ID_THEORA,    NULL,  0 },
    { GAVL_CODEC_ID_DIRAC,     NULL,  0 },
    
  };

const char * gavl_compression_get_extension(gavl_codec_id_t id, int * separate)
  {
  int i;
  for(i = 0; i < sizeof(compression_ids)/sizeof(compression_ids[0]); i++)
    {
    if(compression_ids[i].id == id)
      {
      if(separate)
        *separate = compression_ids[i].separate;
      return compression_ids[i].extension;
      }
    }
  return NULL;
  }

GAVL_PUBLIC
void gavl_packet_alloc(gavl_packet_t * p, int len)
  {
  if(len > p->data_alloc)
    {
    p->data_alloc = len + 1024;
    p->data = realloc(p->data, p->data_alloc);
    }
  }

GAVL_PUBLIC
void gavl_packet_free(gavl_packet_t * p)
  {
  if(p->data)
    free(p->data);
  }
