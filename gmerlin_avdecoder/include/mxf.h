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
typedef struct mxf_timecode_component_s mxf_timecode_component_t;
typedef struct mxf_identification_s mxf_identification_t;
typedef struct mxf_essence_container_data_s mxf_essence_container_data_t;
typedef struct mxf_preface_s mxf_preface_t;
typedef struct mxf_content_storage_s mxf_content_storage_t;

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
    MXF_TYPE_IDENTIFICATION,
    MXF_TYPE_ESSENCE_CONTAINER_DATA,
    MXF_TYPE_CONTENT_STORAGE,
    MXF_TYPE_PREFACE,
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

  mxf_ul_t essence_container_ref;
  mxf_ul_t essence_codec_ul;
  uint32_t sample_rate_num;
  uint32_t sample_rate_den;
  uint32_t aspect_ratio_num;
  uint32_t aspect_ratio_den;
  uint32_t width;
  uint32_t height;
  uint32_t channels;
  uint32_t bits_per_sample;
  uint32_t num_subdescriptor_refs;
  mxf_ul_t *subdescriptor_refs;
  uint32_t linked_track_id;
  uint8_t locked;

  /*
   * 0 FULL_FRAME:        frame consists of a full sample in progressive scan lines
   * 1 SEPARATE_FIELDS: sample consists of two fields, which when interlaced produce a full sample
   * 2 ONE_FIELD:      sample consists of two interlaced fields, but only one field is stored in the data stream
   * 3 MIXED_FIELDS
   * 4 SEGMENTED FRAME
   */
  
  uint8_t frame_layout;
  /* The number of the field which is considered temporally to come first */
  uint8_t field_dominance;

  uint32_t video_line_map_size;
  uint32_t * video_line_map;

  uint32_t active_bits_per_sample;
  uint32_t horizontal_subsampling;
  uint32_t vertical_subsampling;

  /* MPEG-4 extradata (in the future maybe others too) */
  uint8_t *ext_data;
  int ext_size;

  /* Secondary */
  uint32_t fourcc;
  
  };

void bgav_mxf_descriptor_dump(int indent, mxf_descriptor_t * d);
void bgav_mxf_descriptor_free(mxf_descriptor_t * d);

/* Sequence */

struct mxf_sequence_s
  {
  mxf_metadata_t common;

  mxf_ul_t data_definition_ul;
  uint32_t num_structural_component_refs;
  mxf_ul_t *structural_component_refs;
  int64_t duration;

  /* Secondary */
  bgav_stream_type_t stream_type;
  int num_structural_components;
  mxf_structural_component_t ** structural_components;
  };

void bgav_mxf_sequence_dump(int indent, mxf_sequence_t * s);
void bgav_mxf_sequence_free(mxf_sequence_t * s);

/* Track */

struct mxf_track_s
  {
  mxf_metadata_t common;
  mxf_ul_t sequence_ref;
  uint32_t track_id;
  uint8_t track_number[4];
  
  uint32_t edit_rate_num;
  uint32_t edit_rate_den;
  uint64_t origin;
    
  /* Secondary */
  mxf_sequence_t * sequence; /* mandatory, and only one */
  };

void bgav_mxf_track_dump(int indent, mxf_track_t * t);
void bgav_mxf_track_free(mxf_track_t * t);

/* Package */

typedef struct
  {
  mxf_metadata_t common;

  mxf_ul_t * track_refs;
  uint32_t num_track_refs;
  
  mxf_ul_t package_ul;
  mxf_ul_t descriptor_ref;

  uint64_t creation_date;
  uint64_t modification_date;
  
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
  mxf_ul_t source_package_ref;
  mxf_ul_t data_definition_ul;
  int64_t duration;
  int64_t start_position;
  uint32_t source_track_id;
  /* Secondary */
  mxf_package_t    * source_package;
  mxf_track_t      * source_track;
  mxf_descriptor_t * source_descriptor;
  };

void bgav_mxf_structural_component_dump(int indent,
                                        mxf_structural_component_t * s);

/* Timecode component */

struct mxf_timecode_component_s
  {
  mxf_metadata_t common;
  mxf_ul_t data_definition_ul;
  uint16_t rounded_timecode_base;
  uint64_t start_timecode;
  uint64_t duration;
  uint8_t drop_frame;
  };

void bgav_mxf_timecode_component_dump(int indent,
                                      mxf_timecode_component_t * s);

/* Identification */

struct mxf_identification_s
  {
  mxf_metadata_t common;
  mxf_ul_t this_generation_ul;
  char * company_name;
  char * product_name;
  char * version_string;
  mxf_ul_t product_ul;
  uint64_t modification_date;
  };

void bgav_mxf_identification_dump(int indent, mxf_identification_t * s);
void bgav_mxf_identification_free(mxf_identification_t * s);

/* Essence container data */

struct mxf_essence_container_data_s
  {
  mxf_metadata_t common;
  mxf_ul_t linked_package_ul;
  uint32_t index_sid;
  uint32_t body_sid;
  
  };

void bgav_mxf_essence_container_data_dump(int indent, mxf_essence_container_data_t * s);
void bgav_mxf_essence_container_data_free(mxf_essence_container_data_t * s);

/* Preface */

struct mxf_preface_s
  {
  mxf_metadata_t common;
  uint64_t last_modified_date;
  uint16_t version;

  mxf_ul_t * identification_refs;
  uint32_t num_identification_refs;

  mxf_ul_t content_storage_ref;
  mxf_ul_t operational_pattern;

  mxf_ul_t * essence_container_refs;
  uint32_t num_essence_container_refs;

  mxf_ul_t * dm_schemes;
  uint32_t num_dm_schemes;
  
  };

void bgav_mxf_preface_dump(int indent, mxf_preface_t * s);
void bgav_mxf_preface_free(mxf_preface_t * s);

/* Content storage */

struct mxf_content_storage_s
  {
  mxf_metadata_t common;
  
  uint32_t num_package_refs;
  mxf_ul_t * package_refs;

  uint32_t num_essence_container_data_refs;
  mxf_ul_t * essence_container_data_refs;
  };

void bgav_mxf_content_storage_dump(int indent, mxf_content_storage_t * s);
void bgav_mxf_content_storage_free(mxf_content_storage_t * s);


/* Index table segment */

typedef struct 
  {
  mxf_ul_t uid;
  uint32_t edit_rate_num;
  uint32_t edit_rate_den;
  uint64_t start_position;
  uint64_t duration;
  uint32_t edit_unit_byte_count;
  uint32_t index_sid;
  uint32_t body_sid;
  uint8_t slice_count;

  struct
    {
    uint8_t pos_table_index;
    uint8_t slice;
    uint32_t element_delta;
    } * delta_entries;
  uint32_t num_delta_entries;
  
  } mxf_index_table_segment_t;

void bgav_mxf_index_table_segment_dump(int indent, mxf_index_table_segment_t * idx);
void bgav_mxf_index_table_segment_free(mxf_index_table_segment_t * idx);

/* Toplevel file structure */

typedef struct
  {
  mxf_partition_t header_partition;

  
  struct
    {
    mxf_primer_pack_t primer_pack;
    mxf_metadata_t ** metadata;
    int num_metadata;
    } header;

  /* Index table segments */
  mxf_index_table_segment_t ** index_segments;
  int num_index_segments;
  
  /* Convenience arrays and pointers */
  mxf_package_t ** material_packages;
  int num_material_packages;
  
  mxf_content_storage_t * content_storage;
  
  } mxf_file_t;

int bgav_mxf_file_read(bgav_input_context_t * input,
                       mxf_file_t * ret);

void bgav_mxf_file_dump(mxf_file_t * ret);
void bgav_mxf_file_free(mxf_file_t * ret);
