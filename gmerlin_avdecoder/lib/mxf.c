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

#define LOG_DOMAIN "mxf"

#define DUMP_UNKNOWN

static void do_indent(int i)
  {
  while(i--)
    bgav_dprintf(" ");
  }

static void dump_ul(const uint8_t * u)
  {
  bgav_dprintf("0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x",
               u[0], u[1], u[2], u[3], 
               u[4], u[5], u[6], u[7], 
               u[8], u[9], u[10], u[11], 
               u[12], u[13], u[14], u[15]);               
  }

static void dump_date(uint64_t d)
  {
  bgav_dprintf("%04d-%02d-%02d %02d:%02d:%02d:%03d",
               (int)(d>>48),
               (int)(d>>40 & 0xff),
               (int)(d>>32 & 0xff),
               (int)(d>>24 & 0xff),
               (int)(d>>16 & 0xff),
               (int)(d>>8 & 0xff),
               (int)(d & 0xff));
  
  }

static mxf_ul_t * read_refs(bgav_input_context_t * input, uint32_t * num)
  {
  mxf_ul_t * ret;
  if(!bgav_input_read_32_be(input, num))
    return (mxf_ul_t*)0;
  /* Skip size */
  bgav_input_skip(input, 4);
  if(!num)
    return (mxf_ul_t *)0;
  ret = malloc(sizeof(*ret) * *num);
  if(bgav_input_read_data(input, (uint8_t*)ret, sizeof(*ret) * *num) < sizeof(*ret) * *num)
    {
    free(ret);
    return (mxf_ul_t*)0;
    }
  return ret;
  }

/* Partial ULs */

static const uint8_t mxf_klv_key[] = { 0x06,0x0e,0x2b,0x34 };

/* Complete ULs */

static const uint8_t mxf_header_partition_pack_key[] =
  { 0x06,0x0e,0x2b,0x34,0x02,0x05,0x01,0x01,0x0d,0x01,0x02,0x01,0x01,0x02 };

static const uint8_t mxf_filler_key[] =
  {0x06,0x0e,0x2b,0x34,0x01,0x01,0x01,0x01,0x03,0x01,0x02,0x10,0x01,0x00,0x00,0x00};

static const uint8_t mxf_primer_pack_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x05,0x01,0x01,0x0d,0x01,0x02,0x01,0x01,0x05,0x01,0x00 };

static const uint8_t mxf_content_storage_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x18,0x00 };

static const uint8_t mxf_source_package_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x37,0x00 };

static const uint8_t mxf_material_package_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x36,0x00 };

static const uint8_t mxf_sequence_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x0F,0x00 };

static const uint8_t mxf_source_clip_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x11,0x00 };

static const uint8_t mxf_static_track_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x3B,0x00 };

static const uint8_t mxf_generic_track_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x04,0x01,0x02,0x02,0x00,0x00 };

static const uint8_t mxf_index_table_segment_key[] =
  { 0x06,0x0e,0x2b,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x02,0x01,0x01,0x10,0x01,0x00 };

static const uint8_t mxf_timecode_component_key[] =
  { 0x06,0x0e,0x2b,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x14,0x00 };

static const uint8_t mxf_identification_key[] =
  { 0x06,0x0e,0x2b,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x30,0x00 };

static const uint8_t mxf_essence_container_data_key[] =
{ 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0d, 0x01, 0x01, 0x01, 0x01, 0x01, 0x23, 0x00 };

static const uint8_t mxf_preface_key[] =
{ 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0d, 0x01, 0x01, 0x01, 0x01, 0x01, 0x2f, 0x00 };

/* Descriptors */

static const uint8_t mxf_descriptor_multiple_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x44,0x00 };

static const uint8_t mxf_descriptor_generic_sound_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x42,0x00 };

static const uint8_t mxf_descriptor_cdci_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x28,0x00 };

static const uint8_t mxf_descriptor_rgba_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x29,0x00 };

static const uint8_t mxf_descriptor_mpeg2video_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x51,0x00 };

static const uint8_t mxf_descriptor_wave_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x48,0x00 };

static const uint8_t mxf_descriptor_aes3_key[] =
  { 0x06,0x0E,0x2B,0x34,0x02,0x53,0x01,0x01,0x0d,0x01,0x01,0x01,0x01,0x01,0x47,0x00 };

/* MPEG-4 extradata */
static const uint8_t mxf_sony_mpeg4_extradata[] =
  { 0x06,0x0e,0x2b,0x34,0x04,0x01,0x01,0x01,0x0e,0x06,0x06,0x02,0x02,0x01,0x00,0x00 };

#define UL_MATCH(d, key) !memcmp(d, key, sizeof(key))

#define UL_MATCH_MOD_REGVER(d, key) (!memcmp(d, key, 7) && !memcmp(&d[8], &key[8], 8))

static int match_ul(const mxf_ul_t u1, const mxf_ul_t u2, int len)
  {
  int i;
  for (i = 0; i < len; i++)
    {
    if (i != 7 && u1[i] != u2[i])
      return 0;
    }
  return 1;
  }

typedef struct
  {
  mxf_ul_t ul;
  bgav_stream_type_t type;
  } stream_entry_t;

/* SMPTE RP224 http://www.smpte-ra.org/mdd/index.html */
static const stream_entry_t mxf_data_definition_uls[] = {
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x01,0x03,0x02,0x02,0x01,0x00,0x00,0x00 }, BGAV_STREAM_VIDEO },
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x01,0x03,0x02,0x02,0x02,0x00,0x00,0x00 }, BGAV_STREAM_AUDIO },
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x05,0x01,0x03,0x02,0x02,0x02,0x02,0x00,0x00 }, BGAV_STREAM_AUDIO },
    { { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }, BGAV_STREAM_UNKNOWN /* CODEC_TYPE_DATA */},
};

static const stream_entry_t * match_stream(const stream_entry_t * se, const mxf_ul_t u1)
  {
  int i = 0;

  //  fprintf(stderr, "Match stream: ");
  //  dump_ul(u1);
  //  fprintf(stderr, "\n");
  
  while(se[i].type != BGAV_STREAM_UNKNOWN)
    {
    if(match_ul(se[i].ul, u1, 16))
      return &se[i];
    i++;
    }
  return (stream_entry_t*)0;
  }

typedef struct
  {
  mxf_ul_t ul;
  int match_len;
  uint32_t fourcc;
  } codec_entry_t;

static const codec_entry_t mxf_codec_uls[] =
  {
    /* PictureEssenceCoding */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x03,0x04,0x01,0x02,0x02,0x01,0x01,0x11,0x00 }, 14, BGAV_MK_FOURCC('m', 'p', 'g', 'v')}, /* MP@ML Long GoP */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x01,0x02,0x02,0x01,0x02,0x01,0x01 }, 14, BGAV_MK_FOURCC('m', 'p', 'g', 'v') }, /* D-10 50Mbps PAL */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x03,0x04,0x01,0x02,0x02,0x01,0x03,0x03,0x00 }, 14, BGAV_MK_FOURCC('m', 'p', 'g', 'v') }, /* MP@HL Long GoP */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x03,0x04,0x01,0x02,0x02,0x01,0x04,0x02,0x00 }, 14, BGAV_MK_FOURCC('m', 'p', 'g', 'v') }, /* 422P@HL I-Frame */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x03,0x04,0x01,0x02,0x02,0x01,0x20,0x02,0x03 }, 14, BGAV_MK_FOURCC('m', 'p', '4', 'v') }, /* XDCAM proxy_pal030926.mxf */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x01,0x02,0x02,0x02,0x01,0x02,0x00 }, 13, BGAV_MK_FOURCC('d', 'v', 'c', 'p') }, /* DV25 IEC PAL */
      //    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x07,0x04,0x01,0x02,0x02,0x03,0x01,0x01,0x00 }, 14, CODEC_ID_JPEG2000 }, /* JPEG2000 Codestream */
      //    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x01,0x02,0x01,0x7F,0x00,0x00,0x00 }, 13, CODEC_ID_RAWVIDEO }, /* Uncompressed */
    /* SoundEssenceCompression */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x02,0x02,0x01,0x00,0x00,0x00,0x00 }, 13, BGAV_MK_FOURCC('s', 'o', 'w', 't') }, /* Uncompressed */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x02,0x02,0x01,0x7F,0x00,0x00,0x00 }, 13, BGAV_MK_FOURCC('s', 'o', 'w', 't') },
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x07,0x04,0x02,0x02,0x01,0x7E,0x00,0x00,0x00 }, 13, BGAV_MK_FOURCC('t', 'w', 'o', 's') }, /* From Omneon MXF file */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x04,0x04,0x02,0x02,0x02,0x03,0x01,0x01,0x00 }, 15, BGAV_MK_FOURCC('a', 'l', 'a', 'w') }, /* XDCAM Proxy C0023S01.mxf */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x02,0x02,0x02,0x03,0x02,0x01,0x00 }, 15, BGAV_MK_FOURCC('.', 'a', 'c', '3') },
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x02,0x02,0x02,0x03,0x02,0x05,0x00 }, 15, BGAV_MK_FOURCC('.', 'm', 'p', '3') }, /* MP2 or MP3 */
  //{ { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x02,0x02,0x02,0x03,0x02,0x1C,0x00 }, 15, CODEC_ID_DOLBY_E }, /* Dolby-E */
    { { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },  0, 0x00 },
  };

static const codec_entry_t  mxf_picture_essence_container_uls[] = {
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x02,0x0D,0x01,0x03,0x01,0x02,0x04,0x60,0x01 }, 14, BGAV_MK_FOURCC('m', 'p', 'g', 'v') }, /* MPEG-ES Frame wrapped */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x0D,0x01,0x03,0x01,0x02,0x02,0x41,0x01 }, 14, BGAV_MK_FOURCC('d', 'v', 'c', 'p') }, /* DV 625 25mbps */
    { { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },  0, 0x00 },
};

static const codec_entry_t  mxf_sound_essence_container_uls[] = {
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x0D,0x01,0x03,0x01,0x02,0x06,0x01,0x00 }, 14, BGAV_MK_FOURCC('s', 'o', 'w', 't') }, /* BWF Frame wrapped */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x02,0x0D,0x01,0x03,0x01,0x02,0x04,0x40,0x01 }, 14, BGAV_MK_FOURCC('.', 'm', 'p', '3') }, /* MPEG-ES Frame wrapped, 0x40 ??? stream id */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x0D,0x01,0x03,0x01,0x02,0x01,0x01,0x01 }, 14, BGAV_MK_FOURCC('s', 'o', 'w', 't') }, /* D-10 Mapping 50Mbps PAL Extended Template */
    { { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },  0, 0x00 },
};

static const codec_entry_t * match_codec(const codec_entry_t * ce, const mxf_ul_t u1)
  {
  int i = 0;
  //  fprintf(stderr, "Match codec: ");
  //  dump_ul(u1);
  //  fprintf(stderr, "\n");
  while(ce[i].fourcc)
    {
    if(match_ul(ce[i].ul, u1, ce[i].match_len))
      return &ce[i];
    i++;
    }
  return (codec_entry_t*)0;
  }

static char * read_utf16_string(bgav_input_context_t * input, int len)
  {
  bgav_charset_converter_t * cnv;
  char * str, * ret;
  cnv = bgav_charset_converter_create(input->opt, "UTF-16BE", "UTF-8");
  if(!cnv)
    {
    bgav_input_skip(input, len);
    return (char*)0;
    }
  str = malloc(len);
  if(bgav_input_read_data(input, (uint8_t*)str, len) < len)
    {
    bgav_charset_converter_destroy(cnv);
    return (char*)0;
    }

  ret = bgav_convert_string(cnv, str, len, (int *)0);
  bgav_charset_converter_destroy(cnv);
  return ret;
  }

int bgav_mxf_klv_read(bgav_input_context_t * input, mxf_klv_t * ret)
  {
  uint8_t c;
  int64_t len;
  /* Key */
  if(bgav_input_read_data(input, ret->key, 16) < 16)
    return 0;

  /* Length */
  if(!bgav_input_read_8(input, &c))
    return 0;

  if(c & 0x80) /* Long */
    {
    len = c & 0x0F;
    ret->length = 0;
    if(len > 8)
      return 0;
    while(len--)
      {
      if(!bgav_input_read_8(input, &c))
        return 0;
      ret->length = (ret->length<<8) | c;
      }
    }
  else
    ret->length = c;

  ret->endpos = input->position + ret->length;
  
  /* Value follows */
  return 1;
  }

void bgav_mxf_klv_dump(int indent, mxf_klv_t * ret)
  {
  do_indent(indent);
  
  bgav_dprintf("Key: ");
  dump_ul(ret->key);
  bgav_dprintf(", Length: %" PRId64 "\n", ret->length);
  }

int bgav_mxf_sync(bgav_input_context_t * input)
  {
  uint8_t data[4];
  while(1)
    {
    if(bgav_input_get_data(input, data, 4) < 4)
      return 0;
    if(UL_MATCH(data, mxf_klv_key))
      break;
    bgav_input_skip(input, 1);
    }
  return 1;
  }

/* Partition */

int bgav_mxf_partition_read(bgav_input_context_t * input,
                            mxf_klv_t * parent,
                            mxf_partition_t * ret)
  {
  int i;
  uint32_t len;
  if(!bgav_input_read_16_be(input, &ret->majorVersion) ||
     !bgav_input_read_16_be(input, &ret->minorVersion) ||
     !bgav_input_read_32_be(input, &ret->kagSize) ||
     !bgav_input_read_64_be(input, &ret->thisPartition) ||
     !bgav_input_read_64_be(input, &ret->previousPartition) ||
     !bgav_input_read_64_be(input, &ret->footerPartition) ||
     !bgav_input_read_64_be(input, &ret->headerByteCount) ||
     !bgav_input_read_64_be(input, &ret->indexByteCount) ||
     !bgav_input_read_32_be(input, &ret->indexSID) ||
     !bgav_input_read_64_be(input, &ret->bodyOffset) ||
     !bgav_input_read_32_be(input, &ret->bodySID) ||
     (bgav_input_read_data(input, ret->operationalPattern, 16) < 16) 
     )
    return 0;
                                         
  if(!bgav_input_read_32_be(input, &ret->num_essenceContainers) ||
     !bgav_input_read_32_be(input, &len))
    return 0;
  
  ret->essenceContainers = malloc(ret->num_essenceContainers *
                                   sizeof(*ret->essenceContainers));
  for(i = 0; i < ret->num_essenceContainers; i++)
    {
    if(bgav_input_read_data(input, ret->essenceContainers[i], 16) < 16)
      return 0;
    }
  return 1;
  }

void bgav_mxf_partition_dump(int indent, mxf_partition_t * ret)
  {
  int i;
  do_indent(indent);
  bgav_dprintf("Partition\n");
  do_indent(indent+2); bgav_dprintf("majorVersion:       %d\n", ret->majorVersion);
  do_indent(indent+2); bgav_dprintf("minorVersion:       %d\n", ret->minorVersion);
  do_indent(indent+2); bgav_dprintf("kagSize:            %d\n", ret->kagSize);
  do_indent(indent+2); bgav_dprintf("thisPartition:      %" PRId64 " \n", ret->thisPartition);
  do_indent(indent+2); bgav_dprintf("previousPartition:  %" PRId64 " \n", ret->previousPartition);
  do_indent(indent+2); bgav_dprintf("footerPartition:    %" PRId64 " \n", ret->footerPartition);
  do_indent(indent+2); bgav_dprintf("headerByteCount:    %" PRId64 " \n", ret->headerByteCount);
  do_indent(indent+2); bgav_dprintf("indexByteCount:     %" PRId64 " \n", ret->indexByteCount);
  do_indent(indent+2); bgav_dprintf("indexSID:           %d\n", ret->indexSID);
  do_indent(indent+2); bgav_dprintf("bodyOffset:         %" PRId64 " \n", ret->bodyOffset);
  do_indent(indent+2); bgav_dprintf("bodySID:            %d\n", ret->bodySID);
  do_indent(indent+2); bgav_dprintf("operationalPattern: ");
  dump_ul(ret->operationalPattern);
  bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Essence containers: %d\n", ret->num_essenceContainers);
  for(i = 0; i < ret->num_essenceContainers; i++)
    {
    do_indent(indent+4); bgav_dprintf("Essence container: ");
    dump_ul(ret->essenceContainers[i]);
    bgav_dprintf("\n");
    }

  
  }

void bgav_mxf_partition_free(mxf_partition_t * ret)
  {
  if(ret->essenceContainers) free(ret->essenceContainers);
  }

/* Primer pack */

int bgav_mxf_primer_pack_read(bgav_input_context_t * input,
                              mxf_primer_pack_t * ret)
  {
  int i;
  uint32_t len;
  if(!bgav_input_read_32_be(input, &ret->num_entries) ||
     !bgav_input_read_32_be(input, &len) ||
     (len != 18))
    return 0;
  ret->entries = malloc(ret->num_entries * sizeof(*ret->entries));

  for(i = 0; i < ret->num_entries; i++)
    {
    if(!bgav_input_read_16_be(input, &ret->entries[i].localTag) ||
       (bgav_input_read_data(input, ret->entries[i].uid, 16) < 16))
      return 0;
    }
  return 1;
  }

void bgav_mxf_primer_pack_dump(int indent, mxf_primer_pack_t * ret)
  {
  int i;
  do_indent(indent); bgav_dprintf("Primer pack (%d entries)\n", ret->num_entries);
  for(i = 0; i < ret->num_entries; i++)
    {
    do_indent(indent+2); bgav_dprintf("LocalTag: %04x, UID: ", ret->entries[i].localTag);
    dump_ul(ret->entries[i].uid);
    bgav_dprintf("\n");
    }
  }

void bgav_mxf_primer_pack_free(mxf_primer_pack_t * ret)
  {
  if(ret->entries) free(ret->entries);
  }

/* Header metadata */

static int read_header_metadata(bgav_input_context_t * input,
                                mxf_file_t * ret, mxf_klv_t * klv,
                                int (*read_func)(bgav_input_context_t * input,
                                                 mxf_file_t * ret, mxf_metadata_t * m,
                                                 int tag, int size, uint8_t * uid),
                                int struct_size, mxf_metadata_type_t type)
  {
  uint16_t tag, len;
  int64_t end_pos;
  mxf_ul_t uid = {0};

  mxf_metadata_t * m;
  if(struct_size)
    {
    m = calloc(1, struct_size);
    m->type = type;
    }
  else
    m = (mxf_metadata_t *)0;
  
  while(input->position < klv->endpos)
    {
    if(!bgav_input_read_16_be(input, &tag) ||
       !bgav_input_read_16_be(input, &len))
      return 0;
    end_pos = input->position + len;

    if(!len) continue;

    if(tag > 0x7FFF)
      {
      int i;
      for(i = 0; i < ret->header.primer_pack.num_entries; i++)
        {
        if(ret->header.primer_pack.entries[i].localTag == tag)
          memcpy(uid, ret->header.primer_pack.entries[i].uid, 16);
        }
      }
    else if((tag == 0x3C0A) && m)
      {
      if(bgav_input_read_data(input, m->uid, 16) < 16)
        return 0;
      //      fprintf(stderr, "Got UID: ");dump_ul(m->uid);fprintf(stderr, "\n");
      //      fprintf(stderr, "Skip UID:\n");
      //      bgav_input_skip_dump(input, 16);
      }
    else if(!read_func(input, ret, m, tag, len, uid))
      return 0;

    if(input->position < end_pos)
      bgav_input_skip(input, end_pos - input->position);
    }

  if(m)
    {
    ret->header.metadata =
      realloc(ret->header.metadata,
              (ret->header.num_metadata+1) * sizeof(*ret->header.metadata));
    ret->header.metadata[ret->header.num_metadata] = m;
    ret->header.num_metadata++;
    }
  
  return 1;
  }


/* Header metadata */

/* Content storage */

static int read_content_storage(bgav_input_context_t * input,
                                mxf_file_t * ret, mxf_metadata_t * m,
                                int tag, int size, uint8_t * uid)
 {
 mxf_content_storage_t * p = (mxf_content_storage_t *)m;
 switch(tag)
   {
   case 0x1902:
     if(!(p->essence_container_data_refs =
          read_refs(input, &p->num_essence_container_data_refs)))
       return 0;
     break;
     
   case 0x1901:
     if(!(p->package_refs = read_refs(input, &p->num_package_refs)))
       return 0;
     break;
   default:
#ifdef DUMP_UNKNOWN
     bgav_dprintf("Unknown local tag in content storage: %04x, %d bytes\n", tag, size);
     if(size)
       bgav_input_skip_dump(input, size);
#endif
     break;
   }
 return 1;
 }

void bgav_mxf_content_storage_dump(int indent, mxf_content_storage_t * s)
  {
  int i;
  do_indent(indent); bgav_dprintf("Content storage\n");
  do_indent(indent+2); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("UID:        "); dump_ul(s->common.uid); bgav_dprintf("\n");

  if(s->num_package_refs)
    {
    bgav_dprintf("Package refs: %d\n", s->num_package_refs);
    for(i = 0; i < s->num_package_refs; i++)
      {
      do_indent(2); dump_ul(s->package_refs[i]); bgav_dprintf("\n");
      }
    }
  if(s->num_essence_container_data_refs)
    {
    bgav_dprintf("Essence container refs: %d\n", s->num_essence_container_data_refs);
    for(i = 0; i < s->num_essence_container_data_refs; i++)
      {
      do_indent(2); dump_ul(s->essence_container_data_refs[i]); bgav_dprintf("\n");
      }
    }

  }

void bgav_mxf_content_storage_free(mxf_content_storage_t * s)
  {
  if(s->package_refs)
    free(s->package_refs);

  }



/* Material package */

static int read_material_package(bgav_input_context_t * input,
                                 mxf_file_t * ret, mxf_metadata_t * m,
                                 int tag, int size, uint8_t * uid)
  {
  mxf_package_t * p = (mxf_package_t *)m;
  
  switch(tag)
    {
    case 0x4401:
      bgav_input_skip(input, 16);
      if(bgav_input_read_data(input, (uint8_t*)p->package_ul, 16) < 16)
        return 0;
      break;
    case 0x4404:
      if(!bgav_input_read_64_be(input, &p->modification_date))
        return 0;
      break;
    case 0x4405:
      if(!bgav_input_read_64_be(input, &p->creation_date))
        return 0;
      break;
    case 0x4403:
      if(!(p->track_refs = read_refs(input, &p->num_track_refs)))
        return 0;
      break;
   default:
#ifdef DUMP_UNKNOWN
     bgav_dprintf("Unknown local tag in material package: %04x, %d bytes\n", tag, size);
     if(size)
       bgav_input_skip_dump(input, size);
#endif
     break;
   }
 return 1;
 }

static int read_source_package(bgav_input_context_t * input,
                               mxf_file_t * ret, mxf_metadata_t * m,
                               int tag, int size, uint8_t * uid)
  {
  mxf_package_t * p = (mxf_package_t *)m;
  
  switch(tag)
    {
    case 0x4404:
      if(!bgav_input_read_64_be(input, &p->modification_date))
        return 0;
      break;
    case 0x4405:
      if(!bgav_input_read_64_be(input, &p->creation_date))
        return 0;
      break;
    case 0x4403:
      if(!(p->track_refs = read_refs(input, &p->num_track_refs)))
        return 0;
      break;
    case 0x4401:
      bgav_input_skip(input, 16);
      if(bgav_input_read_data(input, (uint8_t*)p->package_ul, 16) < 16)
        return 0;
      break;
    case 0x4701:
      if(bgav_input_read_data(input, (uint8_t*)p->descriptor_ref, 16) < 16)
        return 0;
      break;
    default:
#ifdef DUMP_UNKNOWN
     bgav_dprintf("Unknown local tag in source package: %04x, %d bytes\n", tag, size);
     if(size)
       bgav_input_skip_dump(input, size);
#endif
      break;
   }
  return 1;
  }


void bgav_mxf_package_dump(int indent, mxf_package_t * p)
  {
  int i;
  do_indent(indent);   bgav_dprintf("Package:\n");
  do_indent(indent+2); bgav_dprintf("UID:           "); dump_ul(p->common.uid); bgav_dprintf("\n");

  do_indent(indent+2); bgav_dprintf("Package UID:       "); dump_ul(p->package_ul); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Creation date:     "); dump_date(p->creation_date); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Modification date: "); dump_date(p->modification_date); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("%d tracks\n", p->num_track_refs);
  for(i = 0; i < p->num_track_refs; i++)
    {
    do_indent(indent+4); bgav_dprintf("Track: "); dump_ul(p->track_refs[i]); bgav_dprintf("\n");
    }
  do_indent(indent+2); bgav_dprintf("Descriptor ref: "); dump_ul(p->descriptor_ref); bgav_dprintf("\n");
  
  }

void bgav_mxf_package_free(mxf_package_t * p)
  {
  if(p->track_refs) free(p->track_refs);
  if(p->tracks) free(p->tracks);
  }

/* source clip */

void bgav_mxf_structural_component_dump(int indent, mxf_structural_component_t * s)
  {
  do_indent(indent);   bgav_dprintf("Structural component:\n");
  do_indent(indent+2); bgav_dprintf("UID:                "); dump_ul(s->common.uid); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("source_package_ref: "); dump_ul(s->source_package_ref);bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("data_definition_ul: "); dump_ul(s->data_definition_ul);bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("duration:           %" PRId64 "\n", s->duration);
  do_indent(indent+2); bgav_dprintf("start_pos:          %" PRId64 "\n", s->start_position);
  
  }

static int read_source_clip(bgav_input_context_t * input,
                            mxf_file_t * ret, mxf_metadata_t * m,
                            int tag, int size, uint8_t * uid)
  {
  mxf_structural_component_t * s = (mxf_structural_component_t *)m;
  switch(tag)
    {
    case 0x0201:
      if(bgav_input_read_data(input, s->data_definition_ul, 16) < 16)
        return 0;
      break;
    case 0x0202:
      if(!bgav_input_read_64_be(input, (uint64_t*)&s->duration))
        return 0;
      break;
    case 0x1201:
      if(!bgav_input_read_64_be(input, (uint64_t*)&s->start_position))
        return 0;
      break;
    case 0x1101:
      /* UMID, only get last 16 bytes */
      bgav_input_skip(input, 16);
      if(bgav_input_read_data(input, s->source_package_ref, 16) < 16)
        return 0;
      break;
    case 0x1102:
      if(!bgav_input_read_32_be(input, &s->source_track_id))
        return 0;
      break;
    default:
#ifdef DUMP_UNKNOWN
     bgav_dprintf("Unknown local tag in source clip: %04x, %d bytes\n", tag, size);
     if(size)
       bgav_input_skip_dump(input, size);
#endif
     break;
    }
  return 1;
  }

/* Timecode component */

static int read_timecode_component(bgav_input_context_t * input,
                                   mxf_file_t * ret, mxf_metadata_t * m,
                                   int tag, int size, uint8_t * uid)
  {
  mxf_timecode_component_t * s = (mxf_timecode_component_t *)m;
  switch(tag)
    {
    case 0x0201:
      if(bgav_input_read_data(input, s->data_definition_ul, 16) < 16)
        return 0;
      break;
    case 0x0202:
      if(!bgav_input_read_64_be(input, &s->duration))
        return 0;
      break;
    case 0x1502:
      if(!bgav_input_read_16_be(input, &s->rounded_timecode_base))
        return 0;
      break;
    case 0x1501:
      if(!bgav_input_read_64_be(input, &s->start_timecode))
        return 0;
      break;
    case 0x1503:
      if(!bgav_input_read_8(input, &s->drop_frame))
        return 0;
      break;
    default:
#ifdef DUMP_UNKNOWN
     bgav_dprintf("Unknown local tag in timecode component: %04x, %d bytes\n", tag, size);
     if(size)
       bgav_input_skip_dump(input, size);
#endif
     break;
    }
  return 1;
  }

void bgav_mxf_timecode_component_dump(int indent, mxf_timecode_component_t * s)
  {
  do_indent(indent);   bgav_dprintf("Timecode component\n");
  do_indent(indent+2); bgav_dprintf("data_definition_ul:    "); dump_ul(s->data_definition_ul); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("duration:              %"PRId64"\n", s->duration);
  do_indent(indent+2); bgav_dprintf("rounded_timecode_base: %d\n",
                                    s->rounded_timecode_base);
  do_indent(indent+2); bgav_dprintf("start_timecode:        %"PRId64"\n", s->start_timecode);
  do_indent(indent+2); bgav_dprintf("drop_frame:            %d\n", s->drop_frame);
  }



/* track */

void bgav_mxf_track_dump(int indent, mxf_track_t * t)
  {
  do_indent(indent); bgav_dprintf("Track\n");
  do_indent(indent+2); bgav_dprintf("UID:          "); dump_ul(t->common.uid); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Track ID:     %d\n", t->track_id);
  do_indent(indent+2); bgav_dprintf("Track number: [%02x %02x %02x %02x]\n",
                                    t->track_number[0], t->track_number[1],
                                    t->track_number[2], t->track_number[3]);
  do_indent(indent+2); bgav_dprintf("Edit rate:    %d/%d\n", t->edit_rate_num, t->edit_rate_den);
  do_indent(indent+2); bgav_dprintf("Sequence ref: ");dump_ul(t->sequence_ref);bgav_dprintf("\n");
  }

void bgav_mxf_track_free(mxf_track_t * t)
  {
  
  }

static int read_track(bgav_input_context_t * input,
                      mxf_file_t * ret, mxf_metadata_t * m,
                      int tag, int size, uint8_t * uid)
  {
  mxf_track_t * t = (mxf_track_t *)m;

  switch(tag)
    {
    case 0x4801:
      if(!bgav_input_read_32_be(input, &t->track_id))
        return 0;
      break;
    case 0x4804:
      if(bgav_input_read_data(input, t->track_number, 4) < 4)
        return 0;
      break;
    case 0x4B01:
      if(!bgav_input_read_32_be(input, &t->edit_rate_den) ||
         !bgav_input_read_32_be(input, &t->edit_rate_num))
        return 0;
      break;
    case 0x4803:
      if(bgav_input_read_data(input, t->sequence_ref, 16) < 16)
        return 0;
      break;
    case 0x4b02:
      if(!bgav_input_read_64_be(input, &t->origin))
        return 0;
      break;
    default:
#ifdef DUMP_UNKNOWN
      bgav_dprintf("Unknown local tag in track: %04x, %d bytes\n", tag, size);
      if(size)
        bgav_input_skip_dump(input, size);
#endif
      break;
    }
  return 1; 
  }


/* Sequence */

void bgav_mxf_sequence_dump(int indent, mxf_sequence_t * s)
  {
  int i;
  do_indent(indent); bgav_dprintf("Sequence\n");
  do_indent(indent+2); bgav_dprintf("UID:                "); dump_ul(s->common.uid); bgav_dprintf("\n");

  do_indent(indent+2); bgav_dprintf("data_definition_ul: ");dump_ul(s->data_definition_ul);bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Structural components (%d):\n", s->num_structural_component_refs);
  for(i = 0; i < s->num_structural_component_refs; i++)
    {
    do_indent(indent+4); dump_ul(s->structural_component_refs[i]);bgav_dprintf("\n");
    }
  do_indent(indent+2); bgav_dprintf("Type: %s\n",
                                    (s->stream_type == BGAV_STREAM_AUDIO ? "Audio" :
                                     (s->stream_type == BGAV_STREAM_VIDEO ? "Video" : "Unknown" )));
  
  }

void bgav_mxf_sequence_free(mxf_sequence_t * s)
  {
  if(s->structural_component_refs)
    free(s->structural_component_refs);
  if(s->structural_components)
    free(s->structural_components);
  }


static int read_sequence(bgav_input_context_t * input,
                         mxf_file_t * ret, mxf_metadata_t * m,
                         int tag, int size, uint8_t * uid)
  {
  mxf_sequence_t * s = (mxf_sequence_t *)m;

  switch(tag)
    {
    case 0x0202:
      if(!bgav_input_read_64_be(input, (uint64_t*)&s->duration))
        return 0;
      break;
    case 0x0201:
      if(bgav_input_read_data(input, s->data_definition_ul, 16) < 16)
        return 0;
      break;
    case 0x1001:
      if(!(s->structural_component_refs = read_refs(input, &s->num_structural_component_refs)))
        return 0;
      break;
    default:
#ifdef DUMP_UNKNOWN
      bgav_dprintf("Unknown local tag in sequence : %04x, %d bytes\n", tag, size);
      if(size)
        bgav_input_skip_dump(input, size);
#endif
      break;
    }
  return 1;
  }

/* Identification */

static int read_identification(bgav_input_context_t * input,
                               mxf_file_t * ret, mxf_metadata_t * m,
                               int tag, int size, uint8_t * uid)
  {
  mxf_identification_t * s = (mxf_identification_t *)m;

  switch(tag)
    {
    case 0x3c01:
      s->company_name = read_utf16_string(input, size);
      break;
    case 0x3c02:
      s->product_name = read_utf16_string(input, size);
      break;
    case 0x3c04:
      s->version_string = read_utf16_string(input, size);
      break;
    case 0x3c09:
      if(bgav_input_read_data(input, s->this_generation_ul, 16) < 16)
        return 0;
      break;
    case 0x3c05:
      if(bgav_input_read_data(input, s->product_ul, 16) < 16)
        return 0;
      break;
    case 0x3c06:
      if(!bgav_input_read_64_be(input, &s->modification_date))
        return 0;
      break;
    default:
#ifdef DUMP_UNKNOWN
      bgav_dprintf("Unknown local tag in identification : %04x, %d bytes\n", tag, size);
      if(size)
        bgav_input_skip_dump(input, size);
#endif
      break;
    }
  return 1;
  }

#if 0
  {
  mxf_metadata_t common;
  mxf_ul_t this_generation_uid;
  char * company_name;
  char * product_name;
  char * version_string;
  mxf_ul_t product_uid;
  uint64_t modification_date;
  };
#endif

void bgav_mxf_identification_dump(int indent, mxf_identification_t * s)
  {
  do_indent(indent); bgav_dprintf("Identification\n");

  do_indent(indent+2); bgav_dprintf("UID:               "); dump_ul(s->common.uid); bgav_dprintf("\n");

  do_indent(indent+2); bgav_dprintf("thisGenerationUID: "); dump_ul(s->this_generation_ul);bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Company name:      %s\n",
                                    (s->company_name ? s->company_name :
                                     "(unknown"));
  do_indent(indent+2); bgav_dprintf("Product name:      %s\n",
                                    (s->product_name ? s->product_name :
                                     "(unknown"));
  do_indent(indent+2); bgav_dprintf("Version string:    %s\n",
                                    (s->version_string ? s->version_string :
                                     "(unknown"));

  do_indent(indent+2); bgav_dprintf("ProductUID:        "); dump_ul(s->product_ul);bgav_dprintf("\n");
  
  do_indent(indent+2); bgav_dprintf("ModificationDate:  "); dump_date(s->modification_date);bgav_dprintf("\n");
  
  }

void bgav_mxf_identification_free(mxf_identification_t * s)
  {
  
  }



/* Descriptor */

void bgav_mxf_descriptor_dump(int indent, mxf_descriptor_t * d)
  {
  int i;
  do_indent(indent);   bgav_dprintf("Descriptor\n");
  do_indent(indent+2); bgav_dprintf("UID:                    ");dump_ul(d->common.uid); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("essence_container_ref:  ");dump_ul(d->essence_container_ref);bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("essence_codec_ul:       ");dump_ul(d->essence_codec_ul);bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Sample rate:            %d/%d\n", d->sample_rate_num, d->sample_rate_den); 
  do_indent(indent+2); bgav_dprintf("Aspect ratio:           %d/%d\n", d->aspect_ratio_num, d->aspect_ratio_den);
  do_indent(indent+2); bgav_dprintf("Image size:             %dx%d\n", d->width, d->height);
  do_indent(indent+2); bgav_dprintf("Bits per sample:        %d\n", d->bits_per_sample);
  do_indent(indent+2); bgav_dprintf("Locked:                 %d\n", d->locked);
  do_indent(indent+2); bgav_dprintf("Frame layout:           %d\n", d->frame_layout);
  do_indent(indent+2); bgav_dprintf("Field dominance:        %d\n", d->field_dominance);
  do_indent(indent+2); bgav_dprintf("Active bits per sample: %d\n", d->active_bits_per_sample);
  do_indent(indent+2); bgav_dprintf("Horizontal subsampling: %d\n", d->horizontal_subsampling);
  do_indent(indent+2); bgav_dprintf("Vertical subsampling:   %d\n", d->vertical_subsampling);
  
  do_indent(indent+2); bgav_dprintf("Subdescriptor refs:     %d\n", d->num_subdescriptor_refs);

  for(i = 0; i < d->num_subdescriptor_refs; i++)
    {
    do_indent(indent+4); dump_ul(d->subdescriptor_refs[i]);bgav_dprintf("\n");
    }

  do_indent(indent+2); bgav_dprintf("Video line map: %d entries\n", d->video_line_map_size);
  
  for(i = 0; i < d->video_line_map_size; i++)
    {
    do_indent(indent+4); bgav_dprintf("Entry: %d\n", d->video_line_map[i]);
    }

  do_indent(indent+2); bgav_dprintf("linked track ID:      %d\n", d->linked_track_id);
  do_indent(indent+2); bgav_dprintf("fourcc:               ");bgav_dump_fourcc(d->fourcc);bgav_dprintf("\n");
  
  }

static int read_pixel_layout(bgav_input_context_t * input, mxf_descriptor_t * d)
  {
  uint8_t code;
  uint8_t w;
  
  do
    {
    if(!bgav_input_read_8(input, &code))
      return 0;
    
    switch (code)
      {
      case 0x52: /* R */
        if(!bgav_input_read_8(input, &w))
          return 0;
        d->bits_per_sample += w;
        break;
      case 0x47: /* G */
        if(!bgav_input_read_8(input, &w))
          return 0;
        d->bits_per_sample += w;
        break;
      case 0x42: /* B */
        if(!bgav_input_read_8(input, &w))
          return 0;
        d->bits_per_sample += w;
        break;
      default:
        bgav_input_skip(input, 1);
        break;
      }
    } while (code != 0); /* SMPTE 377M E.2.46 */
  return 1;
  }

static int read_descriptor(bgav_input_context_t * input,
                           mxf_file_t * ret, mxf_metadata_t * m,
                           int tag, int size, uint8_t * uid)
  {
  int i;
  mxf_descriptor_t * d = (mxf_descriptor_t *)m;
  
  switch(tag)
    {
    case 0x3F01:
      if(!(d->subdescriptor_refs = read_refs(input, &d->num_subdescriptor_refs)))
        return 0;
      break;
    case 0x3004:
      if(bgav_input_read_data(input, d->essence_container_ref, 16) < 16)
        return 0;
      break;
    case 0x3006:
      if(!bgav_input_read_32_be(input, &d->linked_track_id))
        return 0;
      break;
    case 0x3201: /* PictureEssenceCoding */
      if(bgav_input_read_data(input, d->essence_codec_ul, 16) < 16)
        return 0;
      break;
    case 0x3203:
      if(!bgav_input_read_32_be(input, &d->width))
        return 0;
      break;
    case 0x3202:
      if(!bgav_input_read_32_be(input, &d->height))
        return 0;
      break;
    case 0x3301:
      if(!bgav_input_read_32_be(input, &d->active_bits_per_sample))
        return 0;
      break;
    case 0x3302:
      if(!bgav_input_read_32_be(input, &d->horizontal_subsampling))
        return 0;
      break;
    case 0x3308:
      if(!bgav_input_read_32_be(input, &d->vertical_subsampling))
        return 0;
      break;
    case 0x320E:
      if(!bgav_input_read_32_be(input, &d->aspect_ratio_num) ||
         !bgav_input_read_32_be(input, &d->aspect_ratio_den))
        return 0;
      break;
    case 0x3D02:
      if(!bgav_input_read_8(input, &d->locked))
        return 0;
      break;
    case 0x3001:
    case 0x3D03:
      if(!bgav_input_read_32_be(input, &d->sample_rate_num) ||
         !bgav_input_read_32_be(input, &d->sample_rate_den))
        return 0;
      break;
    case 0x3D06: /* SoundEssenceCompression */
      if(bgav_input_read_data(input, d->essence_codec_ul, 16) < 16)
        return 0;
      break;
    case 0x3D07:
      if(!bgav_input_read_32_be(input, &d->channels))
        return 0;
      break;
    case 0x3D01:
      if(!bgav_input_read_32_be(input, &d->bits_per_sample))
        return 0;
      break;
    case 0x3401:
      if(!read_pixel_layout(input, d))
        return 0;
      break;
    case 0x320c:
      if(!bgav_input_read_8(input, &d->frame_layout))
        return 0;
      break;
    case 0x3212:
      if(!bgav_input_read_8(input, &d->field_dominance))
        return 0;
      break;
    case 0x320d:
      if(!bgav_input_read_32_be(input, &d->video_line_map_size))
        return 0;
      bgav_input_skip(input, 4);
      d->video_line_map = malloc(d->video_line_map_size * sizeof(*d->video_line_map));
      for(i = 0; i < d->video_line_map_size; i++)
        {
        if(!bgav_input_read_32_be(input, &d->video_line_map[i]))
          return 0;
        }
      break;
    default:
      /* Private uid used by SONY C0023S01.mxf */
      if(UL_MATCH(uid, mxf_sony_mpeg4_extradata))
        {
        d->ext_data = malloc(size);
        d->ext_size = size;
        if(!bgav_input_read_data(input, d->ext_data, size) < size)
          return 0;
        }
#ifdef DUMP_UNKNOWN
      else
        {
        bgav_dprintf("Unknown local tag in descriptor : %04x, %d bytes\n", tag, size);
        if(size)
          bgav_input_skip_dump(input, size);
        }
#endif
      break;
    }
  return 1;
  }

void bgav_mxf_descriptor_free(mxf_descriptor_t * d)
  {
  if(d->subdescriptor_refs) free(d->subdescriptor_refs);
  if(d->ext_data) free(d->ext_data);
  }

/*
 *  Essence Container Data
 */

void bgav_mxf_essence_container_data_dump(int indent, mxf_essence_container_data_t * s)
  {
  do_indent(indent);   bgav_dprintf("Essence Container Data:\n");
  do_indent(indent+2); bgav_dprintf("UID:                  ");dump_ul(s->common.uid); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Linked Package UID:    ");dump_ul(s->linked_package_ul); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("IndexSID:              %d", s->index_sid);
  do_indent(indent+2); bgav_dprintf("BodySID:               %d", s->body_sid);
  
  }

void bgav_mxf_essence_container_data_free(mxf_essence_container_data_t * s)
  {
  
  }


static int read_essence_container_data(bgav_input_context_t * input,
                           mxf_file_t * ret, mxf_metadata_t * m,
                           int tag, int size, uint8_t * uid)
  {
  mxf_essence_container_data_t * d = (mxf_essence_container_data_t *)m;

  switch(tag)
    {
    case 0x2701:
      /* UMID, only get last 16 bytes */
      bgav_input_skip(input, 16);
      if(bgav_input_read_data(input, d->linked_package_ul, 16) < 16)
        return 0;
      break;
    case 0x3f06:
      if(!bgav_input_read_32_be(input, &d->index_sid))
        return 0;
      break;
    case 0x3f07:
      if(!bgav_input_read_32_be(input, &d->body_sid))
        return 0;
      break;
#if 0
    case 0x3F01:
      if(!(d->subdescriptors_refs = read_refs(input, &d->num_subdescriptors)))
        return 0;
      break;
    case 0x3004:
      if(bgav_input_read_data(input, d->essence_container_ul, 16) < 16)
        return 0;
      break;
    case 0x3006:
      if(!bgav_input_read_32_be(input, &d->linked_track_id))
        return 0;
      break;
    case 0x3201: /* PictureEssenceCoding */
      if(bgav_input_read_data(input, d->essence_codec_ul, 16) < 16)
        return 0;
      break;
    case 0x3203:
      if(!bgav_input_read_32_be(input, &d->width))
        return 0;
      break;
    case 0x3202:
      if(!bgav_input_read_32_be(input, &d->height))
        return 0;
      break;
    case 0x320E:
      if(!bgav_input_read_32_be(input, &d->aspect_ratio_num) ||
         !bgav_input_read_32_be(input, &d->aspect_ratio_den))
        return 0;
      break;
    case 0x3D03:
      if(!bgav_input_read_32_be(input, &d->sample_rate_num) ||
         !bgav_input_read_32_be(input, &d->sample_rate_den))
        return 0;
      break;
    case 0x3D06: /* SoundEssenceCompression */
      if(bgav_input_read_data(input, d->essence_codec_ul, 16) < 16)
        return 0;
      break;
    case 0x3D07:
      if(!bgav_input_read_32_be(input, &d->channels))
        return 0;
      break;
    case 0x3D01:
      if(!bgav_input_read_32_be(input, &d->bits_per_sample))
        return 0;
      break;
    case 0x3401:
      if(!read_pixel_layout(input, d))
        return 0;
      break;
#endif
    default:
      /* Private uid used by SONY C0023S01.mxf */
#ifdef DUMP_UNKNOWN
        bgav_dprintf("Unknown local tag in essence container data : %04x, %d bytes\n", tag, size);
        if(size)
          bgav_input_skip_dump(input, size);
#endif
      break;
    }
  return 1;
  }

/* Preface */

void bgav_mxf_preface_dump(int indent, mxf_preface_t * s)
  {
  int i;
  do_indent(indent);   bgav_dprintf("Preface:\n");
  do_indent(indent+2); bgav_dprintf("UID:                  ");dump_ul(s->common.uid); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Last modified date:    ");dump_date(s->last_modified_date); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Version:               %d\n", s->version);
  do_indent(indent+2); bgav_dprintf("Identifications:       %d\n", s->num_identification_refs);
  for(i = 0; i < s->num_identification_refs; i++)
    {
    do_indent(indent+4); dump_ul(s->identification_refs[i]);bgav_dprintf("\n");
    }
  do_indent(indent+2); bgav_dprintf("Content storage:       ");dump_ul(s->content_storage_ref); bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Operational pattern:   ");dump_ul(s->operational_pattern); bgav_dprintf("\n");
  for(i = 0; i < s->num_essence_container_refs; i++)
    {
    do_indent(indent+4); dump_ul(s->essence_container_refs[i]);bgav_dprintf("\n");
    }
  }

void bgav_mxf_preface_free(mxf_preface_t * d)
  {
  if(d->identification_refs)
    free(d->identification_refs);
  if(d->essence_container_refs)
    free(d->essence_container_refs);
  if(d->dm_schemes)
    free(d->dm_schemes);
  }


static int read_preface(bgav_input_context_t * input,
                        mxf_file_t * ret, mxf_metadata_t * m,
                        int tag, int size, uint8_t * uid)
  {
  mxf_preface_t * d = (mxf_preface_t *)m;

  switch(tag)
    {
    case 0x3b02:
      if(!bgav_input_read_64_be(input, &d->last_modified_date))
        return 0;
      break;
    case 0x3b05:
      if(!bgav_input_read_16_be(input, &d->version))
        return 0;
      break;
    case 0x3b06:
      if(!(d->identification_refs = read_refs(input, &d->num_identification_refs)))
        return 0;
      break;
    case 0x3b03:
      if(bgav_input_read_data(input, d->content_storage_ref, 16) < 16)
        return 0;
      break;
    case 0x3b09:
      if(bgav_input_read_data(input, d->operational_pattern, 16) < 16)
        return 0;
      break;
    case 0x3b0a:
      if(!(d->essence_container_refs = read_refs(input, &d->num_essence_container_refs)))
        return 0;
      break;
    case 0x3b0b:
      if(!(d->dm_schemes = read_refs(input, &d->num_dm_schemes)))
        return 0;
      break;
  
#if 0
    case 0x3F01:
      if(!(d->subdescriptors_refs = read_refs(input, &d->num_subdescriptors)))
        return 0;
      break;
    case 0x3004:
      if(bgav_input_read_data(input, d->essence_container_ul, 16) < 16)
        return 0;
      break;
    case 0x3201: /* PictureEssenceCoding */
      if(bgav_input_read_data(input, d->essence_codec_ul, 16) < 16)
        return 0;
      break;
    case 0x3203:
      if(!bgav_input_read_32_be(input, &d->width))
        return 0;
      break;
    case 0x3202:
      if(!bgav_input_read_32_be(input, &d->height))
        return 0;
      break;
    case 0x320E:
      if(!bgav_input_read_32_be(input, &d->aspect_ratio_num) ||
         !bgav_input_read_32_be(input, &d->aspect_ratio_den))
        return 0;
      break;
    case 0x3D03:
      if(!bgav_input_read_32_be(input, &d->sample_rate_num) ||
         !bgav_input_read_32_be(input, &d->sample_rate_den))
        return 0;
      break;
    case 0x3D06: /* SoundEssenceCompression */
      if(bgav_input_read_data(input, d->essence_codec_ul, 16) < 16)
        return 0;
      break;
    case 0x3D07:
      if(!bgav_input_read_32_be(input, &d->channels))
        return 0;
      break;
    case 0x3D01:
      if(!bgav_input_read_32_be(input, &d->bits_per_sample))
        return 0;
      break;
    case 0x3401:
      if(!read_pixel_layout(input, d))
        return 0;
      break;
#endif
    default:
#ifdef DUMP_UNKNOWN
        bgav_dprintf("Unknown local tag in preface : %04x, %d bytes\n", tag, size);
        if(size)
          bgav_input_skip_dump(input, size);
#endif
      break;
    }
  return 1;
  }




/* Index table segment */

static int read_index_table_segment(bgav_input_context_t * input,
                                    mxf_file_t * ret, mxf_klv_t * klv)
  {
  uint16_t tag, len;
  int64_t end_pos;
  mxf_ul_t uid = {0};
  int i;
  
  mxf_index_table_segment_t * idx;

  idx = calloc(1, sizeof(*idx));
  
  while(input->position < klv->endpos)
    {
    if(!bgav_input_read_16_be(input, &tag) ||
       !bgav_input_read_16_be(input, &len))
      return 0;
    end_pos = input->position + len;
    
    if(!len) continue;
    
    if(tag > 0x7FFF)
      {
      int i;
      for(i = 0; i < ret->header.primer_pack.num_entries; i++)
        {
        if(ret->header.primer_pack.entries[i].localTag == tag)
          memcpy(uid, ret->header.primer_pack.entries[i].uid, 16);
        }
      }
    else if(tag == 0x3C0A)
      {
      if(bgav_input_read_data(input, idx->uid, 16) < 16)
        return 0;
      //      fprintf(stderr, "Got UID: ");dump_ul(m->uid);fprintf(stderr, "\n");
      //      fprintf(stderr, "Skip UID:\n");
      //      bgav_input_skip_dump(input, 16);
      }
    else if(tag == 0x3f0b) // IndexEditRate
      {
      if(!bgav_input_read_32_be(input, &idx->edit_rate_den) ||
         !bgav_input_read_32_be(input, &idx->edit_rate_num))
        return 0;
      }
    else if(tag == 0x3f0c) // IndexStartPosition
      {
      if(!bgav_input_read_64_be(input, &idx->start_position))
        return 0;
      }
    else if(tag == 0x3f0d) // IndexDuration
      {
      if(!bgav_input_read_64_be(input, &idx->duration))
        return 0;
      }
    else if(tag == 0x3f05) // EditUnitByteCount
      {
      if(!bgav_input_read_32_be(input, &idx->edit_unit_byte_count))
        return 0;
      }
    else if(tag == 0x3f06) // IndexSID
      {
      if(!bgav_input_read_32_be(input, &idx->index_sid))
        return 0;
      }
    else if(tag == 0x3f07) // BodySID
      {
      if(!bgav_input_read_32_be(input, &idx->body_sid))
        return 0;
      }
    else if(tag == 0x3f08) // SliceCount
      {
      if(!bgav_input_read_8(input, &idx->slice_count))
        return 0;
      }
    else if(tag == 0x3f09) // DeltaEntryArray
      {
      if(!bgav_input_read_32_be(input, &idx->num_delta_entries))
        return 0;
      bgav_input_skip(input, 4);

      idx->delta_entries =
        malloc(idx->num_delta_entries * sizeof(idx->delta_entries));
      for(i = 0; i < idx->num_delta_entries; i++)
        {
        if(!bgav_input_read_8(input, &idx->delta_entries[i].pos_table_index) ||
           !bgav_input_read_8(input, &idx->delta_entries[i].slice) ||
           !bgav_input_read_32_be(input, &idx->delta_entries[i].element_delta))
          return 0;
        }
      }
    else
      {
      fprintf(stderr, "Skipping unknown tag %04x (len %d) in index\n",
              tag, len);
      bgav_input_skip_dump(input, len);
      }
    if(input->position < end_pos)
      bgav_input_skip(input, end_pos - input->position);
    }

  ret->index_segments =
    realloc(ret->index_segments,
            (ret->num_index_segments+1) * sizeof(*ret->index_segments));
  ret->index_segments[ret->num_index_segments] = idx;
  ret->num_index_segments ++;
  
  return 1;
  }

void bgav_mxf_index_table_segment_dump(int indent, mxf_index_table_segment_t * idx)
  {
  int i;
  do_indent(indent); bgav_dprintf("Index table segment:\n");
  do_indent(indent+2); bgav_dprintf("UID: "); dump_ul(idx->uid), bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("edit_rate: %d/%d",
                                    idx->edit_rate_num, idx->edit_rate_den);
  do_indent(indent+2); bgav_dprintf("start_position: %"PRId64"\n",
                                    idx->start_position);
  do_indent(indent+2); bgav_dprintf("duration: %"PRId64"\n",
                                    idx->duration);
  do_indent(indent+2); bgav_dprintf("edit_unit_byte_count: %d\n",
                                    idx->edit_unit_byte_count);
  do_indent(indent+2); bgav_dprintf("index_sid: %d\n",
                                    idx->index_sid);
  do_indent(indent+2); bgav_dprintf("body_sid: %d\n",
                                    idx->body_sid);
  do_indent(indent+2); bgav_dprintf("slice_count: %d\n",
                                    idx->slice_count);

  if(idx->num_delta_entries)
    {
    do_indent(indent+2); bgav_dprintf("Delta entries: %d\n", idx->num_delta_entries);
    for(i = 0; i < idx->num_delta_entries; i++)
      {
      do_indent(indent+4); bgav_dprintf("I: %d, S: %d, delta: %d\n",
                                        idx->delta_entries[i].pos_table_index,
                                        idx->delta_entries[i].slice,
                                        idx->delta_entries[i].element_delta);
      }
    }
  }

void bgav_mxf_index_table_segment_free(mxf_index_table_segment_t * idx)
  {
  if(idx->delta_entries) free(idx->delta_entries);
  
  }


/* File */

static void *
resolve_strong_ref(mxf_file_t * ret, mxf_ul_t u, mxf_metadata_type_t type)
  {
  int i;
  for(i = 0; i < ret->header.num_metadata; i++)
    {
    if(!memcmp(u, ret->header.metadata[i]->uid, 16) &&
       ((type == MXF_TYPE_ALL) ||
        (type == ret->header.metadata[i]->type)))
      return ret->header.metadata[i];
    }
  return (void*)0;
  }

static mxf_package_t * get_source_package(mxf_file_t * file, mxf_structural_component_t * c)
  {
  int i;
  mxf_package_t * ret;
  for(i = 0; i < file->content_storage->num_package_refs; i++)
    {
    ret = resolve_strong_ref(file, file->content_storage->package_refs[i], MXF_TYPE_SOURCE_PACKAGE);
    if(ret && !memcmp(ret->package_ul, c->source_package_ref, 16))
      {
      return ret;
      break;
      }
    }
  return (mxf_package_t *)0;
  }

static mxf_track_t * get_source_track(mxf_file_t * file, mxf_package_t * p,
                                      mxf_structural_component_t * c)
  {
  int i;
  mxf_track_t * ret;
  for(i = 0; i < p->num_track_refs; i++)
    {
    if(!(ret = resolve_strong_ref(file, p->track_refs[i], MXF_TYPE_TRACK)))
      {
      return (mxf_track_t *)0;
      }
    else if(ret->track_id == c->source_track_id)
      {
      return ret;
      break;
      }
    } 
  return (mxf_track_t*)0;
  }

static mxf_descriptor_t * get_source_descriptor(mxf_file_t * file, mxf_package_t * p, mxf_structural_component_t * c)
  {
  int i;
  mxf_descriptor_t * desc;
  mxf_descriptor_t * sub_desc;

  desc = resolve_strong_ref(file, p->descriptor_ref, MXF_TYPE_ALL);
  if(!desc)
    return desc;
  else if(desc->common.type == MXF_TYPE_DESCRIPTOR)
    return desc;
  else if(desc->common.type == MXF_TYPE_MULTIPLE_DESCRIPTOR)
    {
    for(i = 0; i < desc->num_subdescriptor_refs; i++)
      {
      if(!(sub_desc = resolve_strong_ref(file, desc->subdescriptor_refs[i], MXF_TYPE_DESCRIPTOR)))
        {
        return (mxf_descriptor_t *)0;
        }
      else if(sub_desc->linked_track_id == c->source_track_id)
        {
        return sub_desc;
        break;
        }
      } 
    }
  return (mxf_descriptor_t*)0;
  }


#define APPEND_ARRAY(arr, new, num) \
  arr = realloc(arr, (num+1)*sizeof(*arr));arr[num]=new;num++;

static int bgav_mxf_finalize(mxf_file_t * ret, const bgav_options_t * opt)
  {
  int i, j, k;
  mxf_package_t * mp;
  mxf_track_t * mt;
  mxf_track_t * st;
  mxf_structural_component_t * component;

  for(i = 0; i < ret->header.num_metadata; i++)
    {
    if(ret->header.metadata[i]->type == MXF_TYPE_CONTENT_STORAGE)
      {
      ret->content_storage = (mxf_content_storage_t*)(ret->header.metadata[i]);
      break;
      }
    }
  if(!ret->content_storage)
    return 0; 
  /*
   *  Loop over all Material (= output) packages. Each material package will
   *  become a bgav_track_t
   */
  
  for(i = 0; i < ret->content_storage->num_package_refs; i++)
    {
    mp = resolve_strong_ref(ret, ret->content_storage->package_refs[i], MXF_TYPE_MATERIAL_PACKAGE);
    if(!mp)
      continue;
    
    APPEND_ARRAY(ret->material_packages, mp, ret->num_material_packages);
    
    for(j = 0; j < mp->num_track_refs; j++)
      {
      /* Get track */
      mt = resolve_strong_ref(ret, mp->track_refs[j], MXF_TYPE_TRACK);

      if(!mt)
        {
        bgav_log(opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Could not resolve track %d for material package", j);
        continue;
        }
      APPEND_ARRAY(mp->tracks, mt, mp->num_tracks);

      if(!mt->sequence)
        {
        if(!(mt->sequence =
             resolve_strong_ref(ret, mt->sequence_ref, MXF_TYPE_SEQUENCE)))
          {
          bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not resolve sequence for material track %d", j);
          return 0;
          }
        }
      /* Loop over clips */
      for(k = 0; k < mt->sequence->num_structural_component_refs; k++)
        {
        //        fprintf(stderr, "Find component: ");
        //        dump_ul(mt->sequence->structural_components_refs[k]);
        //        fprintf(stderr, "\n");
        
        component = resolve_strong_ref(ret,
                                       mt->sequence->structural_component_refs[k], MXF_TYPE_SOURCE_CLIP);
        if(!component)
          continue;
        
        APPEND_ARRAY(mt->sequence->structural_components,
                     component, mt->sequence->num_structural_components);

        /* Get source package */
        if(!component->source_package)
          component->source_package = get_source_package(ret, component);

        if(!component->source_package)
          {
          bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not resolve source package for clip %d of material track %d", k, j);
          continue;
          }

        /* Get source track */
        if(!component->source_track)
          component->source_track = get_source_track(ret, component->source_package, component);

        if(!component->source_track)
          {
          bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not resolve source track for clip %d of material track %d", k, j);
          continue;
          }

        st = component->source_track;
        
        /* From the sequence of the source track, we get the track type */
        if(!st->sequence)
          {
          if(!(st->sequence =
               resolve_strong_ref(ret, st->sequence_ref, MXF_TYPE_SEQUENCE)))
            {
            bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not resolve sequence for source track");
            return 0;
            }
          }
        if(st->sequence->stream_type == BGAV_STREAM_UNKNOWN)
          {
          const stream_entry_t * se =
            match_stream(mxf_data_definition_uls, st->sequence->data_definition_ul);
          if(se)
            st->sequence->stream_type = se->type;
          }
        
        /* Get source descriptor */
        if(!component->source_descriptor)
          component->source_descriptor = get_source_descriptor(ret, component->source_package, component);

        if(!component->source_descriptor)
          {
          bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not resolve source descriptor for clip %d of material track %d", k, j);
          continue;
          }
        if(st->sequence->stream_type != BGAV_STREAM_UNKNOWN)
          {
          if(!component->source_descriptor->fourcc)
            {
            const codec_entry_t * ce;
            ce = match_codec(mxf_codec_uls,
                             component->source_descriptor->essence_codec_ul);
            if(ce)
              component->source_descriptor->fourcc = ce->fourcc;
            else if(st->sequence->stream_type == BGAV_STREAM_AUDIO)
              {
              ce = match_codec(mxf_picture_essence_container_uls,
                               component->source_descriptor->essence_container_ref);
              
              if(ce)
                component->source_descriptor->fourcc = ce->fourcc;
              }
            else if(st->sequence->stream_type == BGAV_STREAM_VIDEO)
              {
              ce = match_codec(mxf_sound_essence_container_uls,
                               component->source_descriptor->essence_container_ref);
              
              if(ce)
                component->source_descriptor->fourcc = ce->fourcc;
              }
            }
          }
        }
      }
    }
  
  return 1;
  }


int bgav_mxf_file_read(bgav_input_context_t * input,
                       mxf_file_t * ret)
  {
  mxf_klv_t klv;
  int64_t last_pos, header_start_pos;
  /* Read header partition pack */
  if(!bgav_mxf_sync(input))
    {
    fprintf(stderr, "End of file reached\n");
    return 0;
    }
  if(!bgav_mxf_klv_read(input, &klv))
    return 0;

  if(UL_MATCH(klv.key, mxf_header_partition_pack_key))
    {
    if(!bgav_mxf_partition_read(input, &klv, &ret->header_partition))
      return 0;
    //    fprintf(stderr, "Got header partition\n");
    }
  else
    {
    bgav_log(input->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not find header partition");
    return 0;
    }
  header_start_pos = 0;

  while(1) /* Look for primer pack */
    {
    last_pos = input->position;
    
    if(!bgav_mxf_klv_read(input, &klv))
      break;

    if(UL_MATCH(klv.key, mxf_primer_pack_key))
      {
      if(!bgav_mxf_primer_pack_read(input, &ret->header.primer_pack))
        return 0;
      //      fprintf(stderr, "Got primer pack\n");
      header_start_pos = last_pos;
      break;
      }
    else if(UL_MATCH_MOD_REGVER(klv.key, mxf_filler_key))
      {
      //      fprintf(stderr, "Filler key: %ld\n", klv.length);
      bgav_input_skip(input, klv.length);
      }
    else
      {
#ifdef DUMP_UNKNOWN
      bgav_dprintf("Unknown header chunk:\n");
      bgav_mxf_klv_dump(0, &klv);
      bgav_input_skip_dump(input, klv.length);
#else
      bgav_input_skip(input, klv.length);
#endif
      }
    }
  
  /* Read header metadata */
  while((input->position - header_start_pos < ret->header_partition.headerByteCount))
    {
    last_pos = input->position;
    
    if(!bgav_mxf_klv_read(input, &klv))
      break;
    if(1)
      {
      if(UL_MATCH_MOD_REGVER(klv.key, mxf_filler_key))
        {
        //        fprintf(stderr, "Filler key: %ld\n", klv.length);
        bgav_input_skip(input, klv.length);
        }
      else if(UL_MATCH(klv.key, mxf_content_storage_key))
        {
        if(!read_header_metadata(input, ret, &klv,
                                 read_content_storage,
                                 sizeof(mxf_content_storage_t),
                                 MXF_TYPE_CONTENT_STORAGE))
          return 0;
        }
      else if(UL_MATCH(klv.key, mxf_source_package_key))
        {
        //        fprintf(stderr, "mxf_source_package_key\n");
        // bgav_input_skip_dump(input, klv.length);
        if(!read_header_metadata(input, ret, &klv,
                                 read_source_package, sizeof(mxf_package_t),
                                 MXF_TYPE_SOURCE_PACKAGE))
          return 0;
        }
      else if(UL_MATCH(klv.key, mxf_essence_container_data_key))
        {
        //        fprintf(stderr, "mxf_essence_container_data_key\n");
        // bgav_input_skip_dump(input, klv.length);
        if(!read_header_metadata(input, ret, &klv,
                                 read_essence_container_data, sizeof(mxf_essence_container_data_t),
                                 MXF_TYPE_ESSENCE_CONTAINER_DATA))
          return 0;
        }
      else if(UL_MATCH(klv.key, mxf_material_package_key))
        {
        //        fprintf(stderr, "mxf_material_package_key\n");
        //        bgav_input_skip_dump(input, klv.length);
        if(!read_header_metadata(input, ret, &klv,
                                 read_material_package, sizeof(mxf_package_t),
                                 MXF_TYPE_MATERIAL_PACKAGE))
          return 0;

        }
      else if(UL_MATCH(klv.key, mxf_sequence_key))
        {
        //        fprintf(stderr, "mxf_sequence_key\n");
        if(!read_header_metadata(input, ret, &klv,
                                 read_sequence, sizeof(mxf_sequence_t),
                                 MXF_TYPE_SEQUENCE))
          return 0;
        
        }
      else if(UL_MATCH(klv.key, mxf_source_clip_key))
        {
        //        bgav_input_skip_dump(input, klv.length);
        if(!read_header_metadata(input, ret, &klv,
                                 read_source_clip, sizeof(mxf_structural_component_t),
                                 MXF_TYPE_SOURCE_CLIP))
          return 0;
        
        }
      else if(UL_MATCH(klv.key, mxf_timecode_component_key))
        {
        //        fprintf(stderr, "mxf_timecode_component_key\n");
        // bgav_input_skip_dump(input, klv.length);
        if(!read_header_metadata(input, ret, &klv,
                                 read_timecode_component, sizeof(mxf_timecode_component_t),
                                 MXF_TYPE_TIMECODE_COMPONENT))
          return 0;
        }
      else if(UL_MATCH(klv.key, mxf_static_track_key))
        {
        //  fprintf(stderr, "mxf_static_track_key\n");
        //  bgav_input_skip_dump(input, klv.length);

        if(!read_header_metadata(input, ret, &klv,
                                 read_track, sizeof(mxf_track_t),
                                 MXF_TYPE_TRACK))
          return 0;
        
        }
      else if(UL_MATCH(klv.key, mxf_preface_key))
        {
        //  fprintf(stderr, "mxf_static_track_key\n");
        //  bgav_input_skip_dump(input, klv.length);

        if(!read_header_metadata(input, ret, &klv,
                                 read_preface, sizeof(mxf_preface_t),
                                 MXF_TYPE_PREFACE))
          return 0;
        
        }
      else if(UL_MATCH(klv.key, mxf_generic_track_key))
        {
        //        fprintf(stderr, "mxf_generic_track_key\n");
        //        bgav_input_skip_dump(input, klv.length);
        if(!read_header_metadata(input, ret, &klv,
                                 read_source_clip, sizeof(mxf_track_t),
                                 MXF_TYPE_TRACK))
          return 0;
        }
      else if(UL_MATCH(klv.key, mxf_descriptor_multiple_key))
        {
        if(!read_header_metadata(input, ret, &klv,
                                 read_descriptor, sizeof(mxf_descriptor_t),
                                 MXF_TYPE_MULTIPLE_DESCRIPTOR))
          return 0;
        
        }
      else if(UL_MATCH(klv.key, mxf_identification_key))
        {
        if(!read_header_metadata(input, ret, &klv,
                                 read_identification, sizeof(mxf_identification_t),
                                 MXF_TYPE_IDENTIFICATION))
          return 0;
        
        }
      else if(UL_MATCH(klv.key, mxf_descriptor_generic_sound_key) ||
              UL_MATCH(klv.key, mxf_descriptor_cdci_key) ||
              UL_MATCH(klv.key, mxf_descriptor_rgba_key) ||
              UL_MATCH(klv.key, mxf_descriptor_mpeg2video_key) ||
              UL_MATCH(klv.key, mxf_descriptor_wave_key) ||
              UL_MATCH(klv.key, mxf_descriptor_aes3_key))
        {
        if(!read_header_metadata(input, ret, &klv,
                                 read_descriptor, sizeof(mxf_descriptor_t),
                                 MXF_TYPE_DESCRIPTOR))
          return 0;
        }
      else
        {
#ifdef DUMP_UNKNOWN
        bgav_dprintf("Unknown metadata chunk:\n");
        bgav_mxf_klv_dump(0, &klv);
        bgav_input_skip_dump(input, klv.length);
#else
        bgav_input_skip(input, klv.length);
#endif
        }
      }
    }
  
  fprintf(stderr, "Header done\n");

  if(!bgav_mxf_finalize(ret, input->opt))
    {
    fprintf(stderr, "Finalizing failed");
    return 0;
    }
  /* Read rest */
  while(1)
    {
    if(!bgav_mxf_klv_read(input, &klv))
      break;
    
    if(UL_MATCH_MOD_REGVER(klv.key, mxf_filler_key))
      {
      //      fprintf(stderr, "Filler key: %ld\n", klv.length);
      bgav_input_skip(input, klv.length);
      }
    else if(UL_MATCH(klv.key, mxf_index_table_segment_key))
      {
      if(!read_index_table_segment(input, ret, &klv))
        return 0;
      }
    else
      {
      bgav_input_skip(input, klv.length);
      bgav_mxf_klv_dump(0, &klv);
      }
    }

  return 1;
  }

void bgav_mxf_file_free(mxf_file_t * ret)
  {
  int i;

  bgav_mxf_partition_free(&ret->header_partition);
  bgav_mxf_primer_pack_free(&ret->header.primer_pack);
  
  if(ret->index_segments)
    {
    for(i = 0; i < ret->num_index_segments; i++)
      {
      bgav_mxf_index_table_segment_free(ret->index_segments[i]);
      free(ret->index_segments[i]);
      }
    free(ret->index_segments);
    }
  
  if(ret->header.metadata)
    {
    for(i = 0; i < ret->header.num_metadata; i++)
      {
      switch(ret->header.metadata[i]->type)
        {
        case MXF_TYPE_MATERIAL_PACKAGE:
        case MXF_TYPE_SOURCE_PACKAGE:
          bgav_mxf_package_free((mxf_package_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_PREFACE:
          bgav_mxf_preface_free((mxf_preface_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_CONTENT_STORAGE:
          bgav_mxf_content_storage_free((mxf_content_storage_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_SOURCE_CLIP:
          break;
        case MXF_TYPE_TIMECODE_COMPONENT:
          break;
        case MXF_TYPE_SEQUENCE:
          bgav_mxf_sequence_free((mxf_sequence_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_MULTIPLE_DESCRIPTOR:
        case MXF_TYPE_DESCRIPTOR:
          bgav_mxf_descriptor_free((mxf_descriptor_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_ESSENCE_CONTAINER_DATA:
          bgav_mxf_essence_container_data_free((mxf_essence_container_data_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_TRACK:
          break;
        case MXF_TYPE_CRYPTO_CONTEXT:
          break;
        case MXF_TYPE_IDENTIFICATION:
          bgav_mxf_identification_free((mxf_identification_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_NONE:
        case MXF_TYPE_ALL:
          break;
        }
      if(ret->header.metadata[i])
        free(ret->header.metadata[i]);
      }
    free(ret->header.metadata);
    }

  }

void bgav_mxf_file_dump(mxf_file_t * ret)
  {
  int i;
  bgav_dprintf("\nMXF File structure\n"); 

  bgav_dprintf("  Header "); bgav_mxf_partition_dump(2, &ret->header_partition);

  bgav_mxf_primer_pack_dump(2, &ret->header.primer_pack);

  if(ret->header.metadata)
    {
    for(i = 0; i < ret->header.num_metadata; i++)
      {
      switch(ret->header.metadata[i]->type)
        {
        case MXF_TYPE_MATERIAL_PACKAGE:
          bgav_dprintf("  Material");
          bgav_mxf_package_dump(2, (mxf_package_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_SOURCE_PACKAGE:
          bgav_dprintf("  Source");
          bgav_mxf_package_dump(2, (mxf_package_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_SOURCE_CLIP:
          bgav_dprintf("  Source clip");
          bgav_mxf_structural_component_dump(2, (mxf_structural_component_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_TIMECODE_COMPONENT:
          bgav_mxf_timecode_component_dump(2, (mxf_timecode_component_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_PREFACE:
          bgav_mxf_preface_dump(2, (mxf_preface_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_CONTENT_STORAGE:
          bgav_mxf_content_storage_dump(2, (mxf_content_storage_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_SEQUENCE:
          bgav_mxf_sequence_dump(2, (mxf_sequence_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_MULTIPLE_DESCRIPTOR:
          bgav_mxf_descriptor_dump(2, (mxf_descriptor_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_DESCRIPTOR:
          bgav_mxf_descriptor_dump(2, (mxf_descriptor_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_TRACK:
          bgav_mxf_track_dump(2, (mxf_track_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_IDENTIFICATION:
          bgav_mxf_identification_dump(2, (mxf_identification_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_ESSENCE_CONTAINER_DATA:
          bgav_mxf_essence_container_data_dump(2, (mxf_essence_container_data_t*)(ret->header.metadata[i]));
          break;

        case MXF_TYPE_CRYPTO_CONTEXT:
          break;
        case MXF_TYPE_NONE:
        case MXF_TYPE_ALL:
          break;
        }
      }
    }
  if(ret->num_index_segments)
    {
    for(i = 0; i < ret->num_index_segments; i++)
      {
      bgav_mxf_index_table_segment_dump(0, ret->index_segments[i]);
      }
    }
  }
