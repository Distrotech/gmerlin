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

typedef uint8_t mxf_ul_t[16];

typedef struct mxf_descriptor_s mxf_descriptor_t;
typedef struct mxf_sequence_s mxf_sequence_t;
typedef struct mxf_track_s mxf_track_t;
typedef struct mxf_structural_component_s mxf_structural_component_t;

/* Move to the next startcode */
int bgav_mxf_sync(bgav_input_context_t * input);

/* Structures found in MXF files and functions to operate on them */

/* KLV */
typedef struct
  {
  mxf_ul_t key;
  uint64_t length;
  int64_t endpos;
  } mxf_klv_t;

int bgav_mxf_klv_read(bgav_input_context_t * input, mxf_klv_t * ret);
void bgav_mxf_klv_dump(int indent, mxf_klv_t * ret);

/* Partition */

typedef struct
  {
  uint16_t majorVersion;
  uint16_t minorVersion;
  uint32_t kagSize;
  uint64_t thisPartition;
  uint64_t previousPartition;
  uint64_t footerPartition;
  uint64_t headerByteCount;
  uint64_t indexByteCount;
  uint32_t indexSID;
  uint64_t bodyOffset;
  uint32_t bodySID;
  uint8_t operationalPattern[16];

  uint32_t num_essenceContainers;
  mxf_ul_t * essenceContainers;
  
  } mxf_partition_t;

int bgav_mxf_partition_read(bgav_input_context_t * input,
                            mxf_klv_t * parent,
                            mxf_partition_t * ret);
void bgav_mxf_partition_dump(int indent, mxf_partition_t * ret);

void bgav_mxf_partition_free(mxf_partition_t * ret);

/* Primer pack */

typedef struct
  {
  uint32_t num_entries;

  struct
    {
    uint16_t localTag;
    mxf_ul_t uid;
    } * entries;
  
  } mxf_primer_pack_t;

int bgav_mxf_primer_pack_read(bgav_input_context_t * input,
                              mxf_primer_pack_t * ret);

void bgav_mxf_primer_pack_dump(int indent, mxf_primer_pack_t * ret);
void bgav_mxf_primer_pack_free(mxf_primer_pack_t * ret);

/* Header Metadata */

typedef enum
  {
    MXF_TYPE_ALL,
    MXF_TYPE_MATERIAL_PACKAGE,
    MXF_TYPE_SOURCE_PACKAGE,
    MXF_TYPE_SOURCE_CLIP,
    MXF_TYPE_TIMECODE_COMPONENT,
    MXF_TYPE_SEQUENCE,
    MXF_TYPE_MULTIPLE_DESCRIPTOR,
    MXF_TYPE_DESCRIPTOR,
    MXF_TYPE_TRACK,
    MXF_TYPE_CRYPTO_CONTEXT,
    MXF_TYPE_NONE,
  } mxf_metadata_type_t;

typedef struct
  {
  mxf_metadata_type_t type;
  mxf_ul_t uid;
  } mxf_metadata_t;

void bgav_mxf_metadata_dump_common(int indent, mxf_metadata_t * m);

/* Descriptor */

struct  mxf_descriptor_s
  {
  mxf_metadata_t common;

  mxf_ul_t essence_container_ul;
  mxf_ul_t essence_codec_ul;
  uint32_t sample_rate_num;
  uint32_t sample_rate_den;
  uint32_t aspect_ratio_num;
  uint32_t aspect_ratio_den;
  uint32_t width;
  uint32_t height;
  uint32_t channels;
  uint32_t bits_per_sample;
  uint32_t num_subdescriptors;
  mxf_ul_t *subdescriptors_refs;
  uint32_t linked_track_id;
  uint8_t *extradata;
  int extradata_size;
  
  };

void bgav_mxf_descriptor_dump(int indent, mxf_descriptor_t * d);
void bgav_mxf_descriptor_free(mxf_descriptor_t * d);

/* Sequence */

struct mxf_sequence_s
  {
  mxf_metadata_t common;

  mxf_ul_t data_definition_ul;
  uint32_t num_structural_components_refs;
  mxf_ul_t *structural_components_refs;
  int64_t duration;

  /* Secondary */
  bgav_stream_type_t stream_type;
  int num_structural_components;
  mxf_structural_component_t ** structural_components;
  };

void bgav_mxf_sequence_dump(int indent, mxf_sequence_t * s);

/* Track */

struct mxf_track_s
  {
  mxf_metadata_t common;
  mxf_ul_t sequence_ref;
  uint32_t track_id;
  uint8_t track_number[4];
  
  uint32_t edit_rate_num;
  uint32_t edit_rate_den;

  /* Secondary */
  mxf_sequence_t * sequence; /* mandatory, and only one */
  };

void bgav_mxf_track_dump(int indent, mxf_track_t * s);

/* Package */

typedef struct
  {
  mxf_metadata_t common;

  mxf_ul_t * track_refs;
  uint32_t num_track_refs;
  
  mxf_ul_t package_uid;
  mxf_ul_t descriptor_ref;
  /* Secondary */
  mxf_descriptor_t * descriptor;

  int num_tracks;
  mxf_track_t ** tracks;
  
  } mxf_package_t;

void bgav_mxf_package_dump(int indent, mxf_package_t * p);
void bgav_mxf_package_free(mxf_package_t * p);

/* Structural component */

struct mxf_structural_component_s
  {
  mxf_metadata_t common;
  mxf_ul_t source_package_uid;
  mxf_ul_t data_definition_ul;
  int64_t duration;
  int64_t start_position;
  uint32_t source_track_id;
  
  mxf_package_t * source_package;
  };

void bgav_mxf_structural_component_dump(int indent, mxf_structural_component_t * s);

/* Toplevel file structure */

typedef struct
  {
  mxf_partition_t header_partition;

  uint32_t num_packages_refs;
  mxf_ul_t * packages_refs;
  
  
  struct
    {
    mxf_primer_pack_t primer_pack;
    mxf_metadata_t ** metadata;
    int num_metadata;
    } header;

  /* Convenience arrays */
  mxf_package_t ** material_packages;
  int num_material_packages;
  
  } mxf_file_t;

int bgav_mxf_file_read(bgav_input_context_t * input,
                       mxf_file_t * ret);

void bgav_mxf_file_dump(mxf_file_t * ret);
void bgav_mxf_file_free(mxf_file_t * ret);
