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

struct
  {
  gavl_codec_id_t id;
  const char * extension;
  int separate;
  const char * name;
  }
compression_ids[] =
  {
    { GAVL_CODEC_ID_ALAW,      NULL,  0, "alaw"              },
    { GAVL_CODEC_ID_ULAW,      NULL,  0, "ulaw"              },
    { GAVL_CODEC_ID_MP2,       "mp2", 0, "MPEG layer 2"      },
    { GAVL_CODEC_ID_MP3_CBR,   "mp3", 0, "MPEG layer 3, CBR" },
    { GAVL_CODEC_ID_MP3_VBR,   "mp3", 0, "MPEG layer 3, VBR" },
    { GAVL_CODEC_ID_AC3,       "ac3", 0, "AC3"               },
    { GAVL_CODEC_ID_AAC_RAW,   NULL,  0, "AAC"               },
    { GAVL_CODEC_ID_VORBIS,    NULL,  0, "Vorbis"            },
    
    /* Video */
    { GAVL_CODEC_ID_JPEG,      "jpg", 1, "JPEG image"        },
    { GAVL_CODEC_ID_PNG,       "png", 1, "PNG image"         },
    { GAVL_CODEC_ID_TIFF,      "tif", 1, "TIFF image"        },
    { GAVL_CODEC_ID_TGA,       "tga", 1, "TGA image"         },
    { GAVL_CODEC_ID_MPEG1,     "mpv", 0, "MPEG-1"            },
    { GAVL_CODEC_ID_MPEG2,     "mpv", 0, "MPEG-2"            },
    { GAVL_CODEC_ID_MPEG4_ASP, "m4v", 0, "MPEG-4 ASP"        },
    { GAVL_CODEC_ID_H264,      "h264",0, "H.264"             },
    { GAVL_CODEC_ID_THEORA,    NULL,  0, "Theora"            },
    { GAVL_CODEC_ID_DIRAC,     NULL,  0, "Dirac"             },
    
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

  if(p->dts != GAVL_TIME_UNDEFINED)
    fprintf(stderr, "dts: %"PRId64" ", p->dts);
  else
    fprintf(stderr, "dts: None ");
  fprintf(stderr, "dur: %"PRId64"\n", p->duration);

  fprintf(stderr, "  head: %d, f2: %d",
          p->header_size, p->field2_offset);
  
  hexdump(p->data, p->data_len < 16 ? p->data_len : 16, 16);
  
  }
