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
#include <stdlib.h>
#include <stdio.h>

#include <gavl/gavl.h>
#include <gavl/compression.h>


static void hexdump(const uint8_t * data, int len, int linebreak)
  {
  int i;
  int bytes_written = 0;
  int imax;
  
  while(bytes_written < len)
    {
    imax = (bytes_written + linebreak > len) ? len - bytes_written : linebreak;
    for(i = 0; i < imax; i++)
      fprintf(stderr, "%02x ", data[bytes_written + i]);
    for(i = imax; i < linebreak; i++)
      fprintf(stderr, "   ");
    for(i = 0; i < imax; i++)
      {
      if((data[bytes_written + i] < 0x7f) && (data[bytes_written + i] >= 32))
        fprintf(stderr, "%c", data[bytes_written + i]);
      else
        fprintf(stderr, ".");
      }
    bytes_written += imax;
    fprintf(stderr, "\n");
    }
  }


void gavl_compression_info_free(gavl_compression_info_t * info)
  {
  if(info->global_header)
    free(info->global_header);
  }

#define FLAG_SEPARATE          (1<<0)
#define FLAG_NEEDS_PIXELFORMAT (1<<1)

struct
  {
  gavl_codec_id_t id;
  const char * extension;
  const char * name;
  int flags;
  }
compression_ids[] =
  {
    /* Audio */
    { GAVL_CODEC_ID_ALAW,      NULL,  "alaw"         },
    { GAVL_CODEC_ID_ULAW,      NULL,  "ulaw"         },
    { GAVL_CODEC_ID_MP2,       "mp2", "MPEG layer 2" },
    { GAVL_CODEC_ID_MP3,       "mp3", "MPEG layer 3" },
    { GAVL_CODEC_ID_AC3,       "ac3", "AC3"          },
    { GAVL_CODEC_ID_AAC_RAW,   NULL,  "AAC"          },
    { GAVL_CODEC_ID_VORBIS,    NULL,  "Vorbis"       },
    
    /* Video */
    { GAVL_CODEC_ID_JPEG,      "jpg", "JPEG image",  FLAG_SEPARATE | FLAG_NEEDS_PIXELFORMAT },
    { GAVL_CODEC_ID_PNG,       "png", "PNG image",   FLAG_SEPARATE | FLAG_NEEDS_PIXELFORMAT },
    { GAVL_CODEC_ID_TIFF,      "tif", "TIFF image",  FLAG_SEPARATE | FLAG_NEEDS_PIXELFORMAT },
    { GAVL_CODEC_ID_TGA,       "tga", "TGA image",   FLAG_SEPARATE | FLAG_NEEDS_PIXELFORMAT },
    { GAVL_CODEC_ID_MPEG1,     "mpv", "MPEG-1"       },
    { GAVL_CODEC_ID_MPEG2,     "mpv", "MPEG-2",      FLAG_NEEDS_PIXELFORMAT },
    { GAVL_CODEC_ID_MPEG4_ASP, "m4v", "MPEG-4 ASP"   },
    { GAVL_CODEC_ID_H264,      "h264","H.264"        },
    { GAVL_CODEC_ID_THEORA,    NULL,  "Theora"       },
    { GAVL_CODEC_ID_DIRAC,     NULL,  "Dirac"        },
    
  };

const char * gavl_compression_get_extension(gavl_codec_id_t id, int * separate)
  {
  int i;
  for(i = 0; i < sizeof(compression_ids)/sizeof(compression_ids[0]); i++)
    {
    if(compression_ids[i].id == id)
      {
      if(separate)
        *separate = !!(compression_ids[i].flags & FLAG_SEPARATE);
      return compression_ids[i].extension;
      }
    }
  return NULL;
  }

int gavl_compression_need_pixelformat(gavl_codec_id_t id)
  {
  int i;
  for(i = 0; i < sizeof(compression_ids)/sizeof(compression_ids[0]); i++)
    {
    if(compression_ids[i].id == id)
      return !!(compression_ids[i].flags & FLAG_NEEDS_PIXELFORMAT);
    }
  return 0;
  }


static const char *
get_name(gavl_codec_id_t id)
  {
  int i;
  for(i = 0; i < sizeof(compression_ids)/sizeof(compression_ids[0]); i++)
    {
    if(compression_ids[i].id == id)
      {
      return compression_ids[i].name;
      }
    }
  return NULL;
  }

void gavl_compression_info_dump(const gavl_compression_info_t * info)
  {
  fprintf(stderr, "Compression info\n");
  fprintf(stderr, "  Codec:   %s\n", get_name(info->id));
  fprintf(stderr, "  Bitrate: %d bps\n", info->bitrate);

  fprintf(stderr, "  Frame types: I");
  if(info->flags & GAVL_COMPRESSION_HAS_P_FRAMES)
    fprintf(stderr, ",P");
  if(info->flags & GAVL_COMPRESSION_HAS_B_FRAMES)
    fprintf(stderr, ",B");
  fprintf(stderr, "\n");
  
  fprintf(stderr, "  Global header %d bytes", info->global_header_len);
  if(info->global_header_len)
    {
    fprintf(stderr, " (hexdump follows)\n");
    hexdump(info->global_header,
            info->global_header_len, 16);
    }
  else
    fprintf(stderr, "\n");
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

void gavl_packet_dump(const gavl_packet_t * p)
  {
  fprintf(stderr, "Packet: sz: %d ", p->data_len);

  if(p->pts != GAVL_TIME_UNDEFINED)
    fprintf(stderr, "pts: %"PRId64" ", p->pts);
  else
    fprintf(stderr, "pts: None ");

  fprintf(stderr, "dur: %"PRId64, p->duration);

  fprintf(stderr, " head: %d, f2: %d\n",
          p->header_size, p->field2_offset);
  
  hexdump(p->data, p->data_len < 16 ? p->data_len : 16, 16);
  
  }
