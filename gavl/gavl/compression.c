/*****************************************************************
 * gavl - a general purpose audio/video processing library
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
#include <stdio.h>
#include <string.h>

#include <gavl/gavl.h>
#include <gavl/compression.h>

#define PACKET_PADDING 16

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

#define FLAG_SEPARATE           (1<<0)
#define FLAG_NEEDS_PIXELFORMAT  (1<<1)
#define FLAG_CFS                (1<<2) // Constant Frame Samples

struct
  {
  gavl_codec_id_t id;
  const char * extension;
  const char * short_name; // Has no spaces
  const char * long_name;
  int flags;
  int sample_size;
  }
compression_ids[] =
  {
    /* Audio */
    { GAVL_CODEC_ID_ALAW,      NULL,       "alaw",   "alaw",         0,       1 },
    { GAVL_CODEC_ID_ULAW,      NULL,       "ulaw",   "ulaw",         0,       1 },
    { GAVL_CODEC_ID_MP2,       "mp2",      "mp2",    "MPEG layer 3", FLAG_CFS },
    { GAVL_CODEC_ID_MP3,       "mp3",      "mp3",    "MPEG layer 3", FLAG_CFS },
    { GAVL_CODEC_ID_AC3,       "ac3",      "ac3",    "AC3",          FLAG_CFS },
    { GAVL_CODEC_ID_AAC,       NULL,       "aac",    "AAC",          FLAG_CFS },
    { GAVL_CODEC_ID_VORBIS,    NULL,       "vorbis", "Vorbis"       },
    { GAVL_CODEC_ID_FLAC,      NULL,       "flac",   "Flac"         },
    { GAVL_CODEC_ID_OPUS,      NULL,       "opus",   "Opus"         },
    
    /* Video */
    { GAVL_CODEC_ID_JPEG,      "jpg",      "jpeg",   "JPEG image",  FLAG_SEPARATE | FLAG_NEEDS_PIXELFORMAT },
    { GAVL_CODEC_ID_PNG,       "png",      "png",    "PNG image",   FLAG_SEPARATE | FLAG_NEEDS_PIXELFORMAT },
    { GAVL_CODEC_ID_TIFF,      "tif",      "tiff",   "TIFF image",  FLAG_SEPARATE | FLAG_NEEDS_PIXELFORMAT },
    { GAVL_CODEC_ID_TGA,       "tga",      "tga",    "TGA image",   FLAG_SEPARATE | FLAG_NEEDS_PIXELFORMAT },
    { GAVL_CODEC_ID_MPEG1,     "mpv",      "mpeg1",  "MPEG-1"       },
    { GAVL_CODEC_ID_MPEG2,     "mpv",      "mpeg2",  "MPEG-2",      FLAG_NEEDS_PIXELFORMAT },
    { GAVL_CODEC_ID_MPEG4_ASP, "m4v",      "mpeg4",  "MPEG-4 ASP"   },
    { GAVL_CODEC_ID_H264,      "h264",     "h264",   "H.264"        },
    { GAVL_CODEC_ID_THEORA,    NULL,       "theora", "Theora"       },
    { GAVL_CODEC_ID_DIRAC,     NULL,       "dirac",  "Dirac"        },
    { GAVL_CODEC_ID_DV,        "dv",       "dv",     "DV",          FLAG_NEEDS_PIXELFORMAT },
    
  };

#define NUM_CODEC_IDS (sizeof(compression_ids)/sizeof(compression_ids[0]))

const char * gavl_compression_get_extension(gavl_codec_id_t id, int * separate)
  {
  int i;
  for(i = 0; i < NUM_CODEC_IDS; i++)
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

int gavl_compression_get_sample_size(gavl_codec_id_t id)
  {
  int i;
  for(i = 0; i < NUM_CODEC_IDS; i++)
    {
    if(compression_ids[i].id == id)
      {
      return compression_ids[i].sample_size;
      }
    }
  return 0;
  }

int gavl_compression_need_pixelformat(gavl_codec_id_t id)
  {
  int i;
  for(i = 0; i < NUM_CODEC_IDS; i++)
    {
    if(compression_ids[i].id == id)
      return !!(compression_ids[i].flags & FLAG_NEEDS_PIXELFORMAT);
    }
  return 0;
  }

int gavl_compression_constant_frame_samples(gavl_codec_id_t id)
  {
  int i;
  for(i = 0; i < NUM_CODEC_IDS; i++)
    {
    if(compression_ids[i].id == id)
      return !!(compression_ids[i].flags & FLAG_CFS);
    }
  return 0;
  }


const char *
gavl_compression_get_long_name(gavl_codec_id_t id)
  {
  int i;
  for(i = 0; i < NUM_CODEC_IDS; i++)
    {
    if(compression_ids[i].id == id)
      return compression_ids[i].long_name;
    }
  return NULL;
  }

const char *
gavl_compression_get_short_name(gavl_codec_id_t id)
  {
  int i;
  for(i = 0; i < NUM_CODEC_IDS; i++)
    {
    if(compression_ids[i].id == id)
      return compression_ids[i].short_name;
    }
  return NULL;
  
  }

gavl_codec_id_t 
gavl_compression_from_short_name(const char * name)
  {
  int i;
  for(i = 0; i < NUM_CODEC_IDS; i++)
    {
    if(!strcmp(compression_ids[i].short_name, name))
      return compression_ids[i].id;
    }
  return GAVL_CODEC_ID_NONE;
  }

int gavl_num_compressions()
  {
  return NUM_CODEC_IDS;
  }

gavl_codec_id_t gavl_get_compression(int index)
  {
  return compression_ids[index].id;
  }


void gavl_compression_info_dump(const gavl_compression_info_t * info)
  {
  fprintf(stderr, "Compression info\n");
  fprintf(stderr, "  Codec:        %s [%s]\n",
          gavl_compression_get_long_name(info->id),
          gavl_compression_get_short_name(info->id));
  fprintf(stderr, "  Bitrate:      %d bps\n", info->bitrate);

  if(info->id >= 0x10000)
    {
    fprintf(stderr, "  Palette size: %d\n", info->palette_size);
    fprintf(stderr, "  Frame types:  I");
    if(info->flags & GAVL_COMPRESSION_HAS_P_FRAMES)
      fprintf(stderr, ",P");
    if(info->flags & GAVL_COMPRESSION_HAS_B_FRAMES)
      fprintf(stderr, ",B");
    fprintf(stderr, "\n");
    }
  else
    {
    fprintf(stderr, "  SBR:          %s\n",
            (info->flags & GAVL_COMPRESSION_SBR ? "Yes" : "No"));
    }
  
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

void gavl_compression_info_copy(gavl_compression_info_t * dst,
                                const gavl_compression_info_t * src)
  {
  if(!src)
    {
    memset(dst, 0, sizeof(*dst));
    return;
    }
  
  memcpy(dst, src, sizeof(*dst));
  if(src->global_header)
    {
    dst->global_header = malloc(src->global_header_len);
    memcpy(dst->global_header, src->global_header, src->global_header_len);
    }
  }


void gavl_packet_alloc(gavl_packet_t * p, int len)
  {
  if(len + PACKET_PADDING > p->data_alloc)
    {
    p->data_alloc = len + PACKET_PADDING + 1024;
    p->data = realloc(p->data, p->data_alloc);
    }
  memset(p->data + len, 0, PACKET_PADDING);
  }

void gavl_packet_free(gavl_packet_t * p)
  {
  if(p->data)
    free(p->data);
  }

void gavl_packet_reset(gavl_packet_t * p)
  {
  int data_alloc_save;
  uint8_t * data_save;

  data_alloc_save = p->data_alloc;
  data_save       = p->data;

  memset(p, 0, sizeof(*p));
  
  p->data_alloc = data_alloc_save;
  p->data       = data_save;
  
  }

void gavl_packet_copy(gavl_packet_t * dst,
                      const gavl_packet_t * src)
  {
  int data_alloc_save;
  uint8_t * data_save;

  data_alloc_save = dst->data_alloc;
  data_save       = dst->data;

  memcpy(dst, src, sizeof(*src));

  dst->data_alloc = data_alloc_save;
  dst->data       = data_save;

  gavl_packet_alloc(dst, src->data_len);
  memcpy(dst->data, src->data, src->data_len);
  }

void gavl_packet_copy_metadata(gavl_packet_t * dst,
                               const gavl_packet_t * src)
  {
  int data_alloc_save;
  int data_len_save;
  uint8_t * data_save;
  
  data_alloc_save = dst->data_alloc;
  data_len_save   = dst->data_len;
  data_save       = dst->data;

  memcpy(dst, src, sizeof(*src));

  dst->data_alloc = data_alloc_save;
  dst->data_len   = data_len_save;
  dst->data       = data_save;
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

void gavl_packet_init(gavl_packet_t * p)
  {
  memset(p, 0, sizeof(*p));
  }
