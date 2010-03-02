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

#ifndef GAVL_COMPRESSION_H_INCLUDED
#define GAVL_COMPRESSION_H_INCLUDED

#include <gavl/gavldefs.h>

#ifdef __cplusplus
extern "C" {
#endif


#define GAVL_COMPRESSION_HAS_P_FRAMES (1<<0) // Not all frames are keyframes
#define GAVL_COMPRESSION_HAS_B_FRAMES (1<<1) // Frames don't appear in presentation order 

typedef enum
  {
    GAVL_CODEC_ID_NONE  = 0,
    /* Audio */
    GAVL_CODEC_ID_ALAW  = 1,
    GAVL_CODEC_ID_ULAW,
    GAVL_CODEC_ID_MP2,
    GAVL_CODEC_ID_MP3_CBR,
    GAVL_CODEC_ID_MP3_VBR,
    GAVL_CODEC_ID_AC3,
    GAVL_CODEC_ID_AAC_RAW, /* Like in quicktime/mp4 */
    GAVL_CODEC_ID_VORBIS,  /* http://wiki.xiph.org/Oggless */
    
    /* Video */
    GAVL_CODEC_ID_JPEG = 0x10000, /* JPEG image */
    GAVL_CODEC_ID_PNG,            /* PNG image  */
    GAVL_CODEC_ID_TIFF,           /* TIFF image */
    GAVL_CODEC_ID_TGA,            /* TGA image  */
    GAVL_CODEC_ID_MPEG1,          /* MPEG-1 */
    GAVL_CODEC_ID_MPEG2,          /* MPEG-2 */
    GAVL_CODEC_ID_MPEG4_ASP,      /* MPEG-4 ASP (a.k.a. Divx4) */
    GAVL_CODEC_ID_H264,           /* H.264 */
    GAVL_CODEC_ID_THEORA,         /* http://wiki.xiph.org/Oggless */
    GAVL_CODEC_ID_DIRAC,          /* Complete frames, sequence end appended to last packet */
    
  } gavl_codec_id_t;

typedef struct
  {
  int flags;
  gavl_codec_id_t id;
  
  uint8_t * global_header;
  int global_header_len;
  } gavl_compression_info_t;

GAVL_PUBLIC
void gavl_compression_info_free(gavl_compression_info_t*);

GAVL_PUBLIC
const char * gavl_compression_get_extension(gavl_codec_id_t id, int * separate);


#define GAVL_PACKET_TYPE_I 'I'
#define GAVL_PACKET_TYPE_P 'P'
#define GAVL_PACKET_TYPE_B 'B'
#define GAVL_PACKET_TYPE_MASK 0xff

#define GAVL_PACKET_KEYFRAME (1<<8)

typedef struct
  {
  uint8_t * data;
  int data_len;
  int data_alloc;

  int flags;

  int64_t pts;
  int64_t dts;
  } gavl_packet_t;

GAVL_PUBLIC
void gavl_packet_alloc(gavl_packet_t * , int len);

GAVL_PUBLIC
void gavl_packet_free(gavl_packet_t *);

#ifdef __cplusplus
}
#endif
 

#endif // GAVL_COMPRESSION_H_INCLUDED
