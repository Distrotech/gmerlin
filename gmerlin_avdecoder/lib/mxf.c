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

static void do_indent(int i)
  {
  while(i--)
    bgav_dprintf(" ");
  }

static void dump_ul(uint8_t * u)
  {
  bgav_dprintf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
               u[0], u[1], u[2], u[3], 
               u[4], u[5], u[6], u[7], 
               u[8], u[9], u[10], u[11], 
               u[12], u[13], u[14], u[15]);               
  }

static mxf_ul_t * read_refs(bgav_input_context_t * input, uint32_t * num)
  {
  mxf_ul_t * ret;
  if(!bgav_input_read_32_be(input, num))
    return (mxf_ul_t*)0;
  /* Skip size */
  bgav_input_skip(input, 4);
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
    
#define UL_MATCH(d, key) !memcmp(d, key, sizeof(key))

#define UL_MATCH_MOD_REGVER(d, key) (!memcmp(d, key, 7) && !memcmp(&d[8], &key[8], 8))

static int match_uid(const mxf_ul_t u1, const mxf_ul_t u2, int len)
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
      //    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x02,0x02,0x01,0x00,0x00,0x00,0x00 }, 13, CODEC_ID_PCM_S16LE }, /* Uncompressed */
      //    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x02,0x02,0x01,0x7F,0x00,0x00,0x00 }, 13, CODEC_ID_PCM_S16LE },
      //    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x07,0x04,0x02,0x02,0x01,0x7E,0x00,0x00,0x00 }, 13, CODEC_ID_PCM_S16BE }, /* From Omneon MXF file */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x04,0x04,0x02,0x02,0x02,0x03,0x01,0x01,0x00 }, 15, BGAV_MK_FOURCC('a', 'l', 'a', 'w') }, /* XDCAM Proxy C0023S01.mxf */
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x02,0x02,0x02,0x03,0x02,0x01,0x00 }, 15, BGAV_MK_FOURCC('.', 'a', 'c', '3') },
    { { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x02,0x02,0x02,0x03,0x02,0x05,0x00 }, 15, BGAV_MK_FOURCC('.', 'm', 'p', '3') }, /* MP2 or MP3 */
  //{ { 0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x01,0x04,0x02,0x02,0x02,0x03,0x02,0x1C,0x00 }, 15, CODEC_ID_DOLBY_E }, /* Dolby-E */
    { { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },  0, 0x00 },
  };

static codec_entry_t * match_codec(codec_entry_t * ce, const mxf_ul_t u1)
  {
  int i = 0;
  while(ce[i].fourcc)
    {
    if(match_uid(ce[i].ul, u1, ce[i].match_len))
      return &ce[i];
    }
  return (codec_entry_t*)0;
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

static int read_content_storage(bgav_input_context_t * input,
                                mxf_file_t * ret, mxf_metadata_t * m,
                                int tag, int size, uint8_t * uid)
 {
 switch(tag)
   {
   case 0x1901:
     if(!(ret->packages_refs = read_refs(input, &ret->num_packages_refs)))
       return 0;
     break;
   default:
     break;
   }
 return 1;
 }

/* Header metadata */

void bgav_mxf_metadata_dump_common(int indent, mxf_metadata_t * m)
  {
  do_indent(indent); bgav_dprintf("UID: "); dump_ul(m->uid); bgav_dprintf("\n");
  }

/* Material package */

static int read_material_package(bgav_input_context_t * input,
                                 mxf_file_t * ret, mxf_metadata_t * m,
                                 int tag, int size, uint8_t * uid)
  {
  mxf_package_t * p = (mxf_package_t *)m;
  
  switch(tag)
    {
    case 0x4403:
      if(!(p->track_refs = read_refs(input, &p->num_track_refs)))
        return 0;
      break;
   default:
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
    case 0x4403:
      if(!(p->track_refs = read_refs(input, &p->num_track_refs)))
        return 0;
      break;
    case 0x4401:
      bgav_input_skip(input, 16);
      if(bgav_input_read_data(input, (uint8_t*)p->package_uid, 16) < 16)
        return 0;
      break;
    case 0x4701:
      if(bgav_input_read_data(input, (uint8_t*)p->descriptor_ref, 16) < 16)
        return 0;
      break;
    default:
      break;
   }
  return 1;
  }


void bgav_mxf_package_dump(int indent, mxf_package_t * p)
  {
  int i;
  do_indent(indent);   bgav_dprintf("Package:\n");
  bgav_mxf_metadata_dump_common(indent+2, &p->common);
  do_indent(indent+2); bgav_dprintf("Package UID: "); dump_ul(p->package_uid); bgav_dprintf("\n");
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
  }

/* source clip */

void bgav_mxf_structural_component_dump(int indent, mxf_structural_component_t * s)
  {
  do_indent(indent); bgav_dprintf("Structural component:\n");
  bgav_mxf_metadata_dump_common(indent+2, &s->common);
  do_indent(indent+2); bgav_dprintf("source_package_uid: "); dump_ul(s->source_package_uid);bgav_dprintf("\n");
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
      if(bgav_input_read_data(input, s->source_package_uid, 16) < 16)
        return 0;
      break;
    case 0x1102:
      if(!bgav_input_read_32_be(input, &s->source_track_id))
        return 0;
      break;
    }
  return 1;
  }

/* track */

void bgav_mxf_track_dump(int indent, mxf_track_t * t)
  {
  do_indent(indent); bgav_dprintf("Track\n");
  bgav_mxf_metadata_dump_common(indent+2, &t->common);
  do_indent(indent+2); bgav_dprintf("Track ID:     %d\n", t->track_id);
  do_indent(indent+2); bgav_dprintf("Track number: { %02x %02x %02x %02x }\n",
                                    t->track_number[0], t->track_number[1],
                                    t->track_number[2], t->track_number[3]);
  do_indent(indent+2); bgav_dprintf("Edit rate: %d/%d\n", t->edit_rate_num, t->edit_rate_den);
  do_indent(indent+2); bgav_dprintf("Sequence ref: ");dump_ul(t->sequence_ref);bgav_dprintf("\n");
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
    }
  return 1; 
  }


/* Sequence */

void bgav_mxf_sequence_dump(int indent, mxf_sequence_t * s)
  {
  int i;
  do_indent(indent); bgav_dprintf("Sequence\n");
  bgav_mxf_metadata_dump_common(indent+2, &s->common);

  do_indent(indent+2); bgav_dprintf("data_definition_ul: ");dump_ul(s->data_definition_ul);bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Structural components (%d):\n", s->num_structural_components_refs);
  for(i = 0; i < s->num_structural_components_refs; i++)
    {
    do_indent(indent+4); dump_ul(s->structural_components_refs[i]);bgav_dprintf("\n");
    }
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
      if(!(s->structural_components_refs = read_refs(input, &s->num_structural_components_refs)))
        return 0;
      break;
    }
  return 1;
  }

/* Descriptor */

void bgav_mxf_descriptor_dump(int indent, mxf_descriptor_t * d)
  {
  int i;
  do_indent(indent);   bgav_dprintf("Descriptor\n");
  bgav_mxf_metadata_dump_common(indent+2, &d->common);
  do_indent(indent+2); bgav_dprintf("essence_container_ul: ");dump_ul(d->essence_container_ul);bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("essence_codec_ul:     ");dump_ul(d->essence_codec_ul);bgav_dprintf("\n");
  do_indent(indent+2); bgav_dprintf("Sample rate:          %d/%d\n", d->sample_rate_num, d->sample_rate_den); 
  do_indent(indent+2); bgav_dprintf("Aspect ratio:         %d/%d\n", d->aspect_ratio_num, d->aspect_ratio_den);
  do_indent(indent+2); bgav_dprintf("Image size:           %dx%d\n", d->width, d->height);
  do_indent(indent+2); bgav_dprintf("Bits per sample:      %d\n", d->bits_per_sample);
  do_indent(indent+2); bgav_dprintf("Subdescriptor refs: %d\n", d->num_subdescriptors);

  for(i = 0; i < d->num_subdescriptors; i++)
    {
    do_indent(indent+4); dump_ul(d->subdescriptors_refs[i]);bgav_dprintf("\n");
    }
  do_indent(indent+2); bgav_dprintf("linked track ID:      %d\n", d->linked_track_id);
  
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
  mxf_descriptor_t * d = (mxf_descriptor_t *)m;

  switch(tag)
    {
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
    default:
#if 0
      /* Private uid used by SONY C0023S01.mxf */
      if (IS_KLV_KEY(uid, mxf_sony_mpeg4_extradata))
        {
        descriptor->extradata = av_malloc(size);
        if (!descriptor->extradata)
          return -1;
        descriptor->extradata_size = size;
        get_buffer(pb, descriptor->extradata, size);
        }
#endif
      break;
    }
  return 1;
  }

void bgav_mxf_descriptor_free(mxf_descriptor_t * d)
  {
  if(d->subdescriptors_refs) free(d->subdescriptors_refs);
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
  fprintf(stderr, "resolve_strong_ref failed\n");
  return (void*)0;
  }


#define APPEND_ARRAY(arr, new, num) \
  arr = realloc(arr, (num+1)*sizeof(*arr));arr[num]=new;num++;

static int bgav_mxf_finalize(mxf_file_t * ret)
  {
  int i, j, k;
  mxf_package_t * mp;
  mxf_track_t * mt;
  mxf_structural_component_t * component;
  
  /*
   *  Loop over all Material (= output) packages. Each material package will
   *  become a bgav_track_t
   */
  
  for(i = 0; i < ret->num_packages_refs; i++)
    {
    mp = resolve_strong_ref(ret, ret->packages_refs[i], MXF_TYPE_MATERIAL_PACKAGE);
    if(!mp)
      continue;
    
    APPEND_ARRAY(ret->material_packages, mp, ret->num_material_packages);
    
    for(j = 0; j < mp->num_track_refs; j++)
      {
      /* Get track */
      mt = resolve_strong_ref(ret, mp->track_refs[i], MXF_TYPE_TRACK);

      if(!mt)
        continue;

      APPEND_ARRAY(mp->tracks, mt, mp->num_tracks);
      
      if(!(mt->sequence =
           resolve_strong_ref(ret, mt->sequence_ref, MXF_TYPE_SEQUENCE)))
        {
        return 0;
        }
      /* Loop over clips */
      for(k = 0; k < mt->sequence->num_structural_components_refs; k++)
        {
        component = resolve_strong_ref(ret,
                                       mt->sequence->structural_components_refs[k], MXF_TYPE_SOURCE_CLIP);
        if(!component)
          continue;
        
        APPEND_ARRAY(mt->sequence->structural_components,
                     component, mt->sequence->num_structural_components);
        
        }
      /* From the sequence, we get the track type */
      
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
    fprintf(stderr, "Got header partition\n");
    }
  else
    return 0;

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
      fprintf(stderr, "Got primer pack\n");
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
      bgav_input_skip(input, klv.length);
      bgav_mxf_klv_dump(0, &klv);
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
                                 read_content_storage, 0, MXF_TYPE_NONE))
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
        //        fprintf(stderr, "mxf_source_clip_key\n");
        //        bgav_input_skip_dump(input, klv.length);
        if(!read_header_metadata(input, ret, &klv,
                                 read_source_clip, sizeof(mxf_structural_component_t),
                                 MXF_TYPE_SOURCE_CLIP))
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
        bgav_input_skip(input, klv.length);
        bgav_mxf_klv_dump(0, &klv);
        }
      }
    }
  
  fprintf(stderr, "Header done\n");

  if(!bgav_mxf_finalize(ret))
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

  if(ret->header.metadata)
    {
    for(i = 0; i < ret->header.num_metadata; i++)
      {
      switch(ret->header.metadata[i]->type)
        {
        case MXF_TYPE_MATERIAL_PACKAGE:
          bgav_mxf_package_free((mxf_package_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_SOURCE_PACKAGE:
          bgav_mxf_package_free((mxf_package_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_SOURCE_CLIP:
          break;
        case MXF_TYPE_TIMECODE_COMPONENT:
          break;
        case MXF_TYPE_SEQUENCE:
          break;
        case MXF_TYPE_MULTIPLE_DESCRIPTOR:
          break;
        case MXF_TYPE_DESCRIPTOR:
          break;
        case MXF_TYPE_TRACK:
          break;
        case MXF_TYPE_CRYPTO_CONTEXT:
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
  bgav_dprintf("Header "); bgav_mxf_partition_dump(0, &ret->header_partition);

  bgav_mxf_primer_pack_dump(0, &ret->header.primer_pack);

  if(ret->header.metadata)
    {
    for(i = 0; i < ret->header.num_metadata; i++)
      {
      switch(ret->header.metadata[i]->type)
        {
        case MXF_TYPE_MATERIAL_PACKAGE:
          bgav_dprintf("Material ");
          bgav_mxf_package_dump(0, (mxf_package_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_SOURCE_PACKAGE:
          bgav_dprintf("Source ");
          bgav_mxf_package_dump(0, (mxf_package_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_SOURCE_CLIP:
          bgav_dprintf("Source clip ");
          bgav_mxf_structural_component_dump(0, (mxf_structural_component_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_TIMECODE_COMPONENT:
          break;
        case MXF_TYPE_SEQUENCE:
          bgav_mxf_sequence_dump(0, (mxf_sequence_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_MULTIPLE_DESCRIPTOR:
          bgav_mxf_descriptor_dump(0, (mxf_descriptor_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_DESCRIPTOR:
          bgav_mxf_descriptor_dump(0, (mxf_descriptor_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_TRACK:
          bgav_mxf_track_dump(0, (mxf_track_t*)(ret->header.metadata[i]));
          break;
        case MXF_TYPE_CRYPTO_CONTEXT:
          break;
        case MXF_TYPE_NONE:
        case MXF_TYPE_ALL:
          break;
        }
      }
    }
  
  if(ret->num_packages_refs)
    {
    bgav_dprintf("Package refs (%d)\n", ret->num_packages_refs);
    for(i = 0; i < ret->num_packages_refs; i++)
      {
      do_indent(2); dump_ul(ret->packages_refs[i]);; bgav_dprintf("\n");
      }
    }
  }
