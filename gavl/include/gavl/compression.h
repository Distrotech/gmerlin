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

#ifndef GAVL_COMPRESSION_H_INCLUDED
#define GAVL_COMPRESSION_H_INCLUDED

#include <gavl/gavldefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup compression Compressed stream support
 *  \brief Compressed stream support
 *
 *  gavl provides some structures and functions for handling
 *  compressed data packets. It is a completely independent API
 *  layer and has nothing to do with the uncompressed video and audio
 *  API. In particular the conversion between compressed and uncompressed
 *  data (i.e. codecs) are outside the scope of gavl. These are implemented
 *  in gmerlin-avdecoder and gmerlin encoding plugins.
 *
 *  @{
 */


/** \brief Set if not all frames are keyframes */
 
#define GAVL_COMPRESSION_HAS_P_FRAMES (1<<0) 

/** \brief Set if frames don't appear in presentation order */
#define GAVL_COMPRESSION_HAS_B_FRAMES (1<<1) //!<  

/** \brief Packets can consist of 2 consecutive fields */
#define GAVL_COMPRESSION_HAS_FIELD_PICTURES (1<<2)

/** \brief Samplerate got doubled by decoder, format and sample counts are for the upsampled rate */
#define GAVL_COMPRESSION_SBR                (1<<3)

/** \brief Only in uncompressed gavf streams: Specifies that multibyte numbers are big endian */
#define GAVL_COMPRESSION_BIG_ENDIAN         (1<<4)
  
/** \brief Codec ID
 *
 *  These are used as identifiers for the type of compression
 */

typedef enum
  {

    /** \brief Unknown/unsupported compression format. In gavf files this signals that the stream
     *  consists of uncompressed gavl frames
     */
    GAVL_CODEC_ID_NONE  = 0, 
    /* Audio */
    GAVL_CODEC_ID_ALAW  = 1, //!< alaw 2:1
    GAVL_CODEC_ID_ULAW,      //!< mu-law 2:1
    GAVL_CODEC_ID_MP2,       //!< MPEG-1 audio layer II
    GAVL_CODEC_ID_MP3,       //!< MPEG-1/2 audio layer 3 CBR/VBR
    GAVL_CODEC_ID_AC3,       //!< AC3
    GAVL_CODEC_ID_AAC,       //!< AAC as stored in quicktime/mp4
    GAVL_CODEC_ID_VORBIS,    //!< Vorbis (segmented extradata and packets)
    GAVL_CODEC_ID_FLAC,      //!< Flac (extradata contain a file header without comment and seektable)
    GAVL_CODEC_ID_OPUS,      //!< Opus
    GAVL_CODEC_ID_SPEEX,     //!< Speex
    
    /* Video */
    GAVL_CODEC_ID_JPEG = 0x10000, //!< JPEG image
    GAVL_CODEC_ID_PNG,            //!< PNG image
    GAVL_CODEC_ID_TIFF,           //!< TIFF image
    GAVL_CODEC_ID_TGA,            //!< TGA image
    GAVL_CODEC_ID_MPEG1,          //!< MPEG-1 video
    GAVL_CODEC_ID_MPEG2,          //!< MPEG-2 video
    GAVL_CODEC_ID_MPEG4_ASP,      //!< MPEG-4 ASP (a.k.a. Divx4)
    GAVL_CODEC_ID_H264,           //!< H.264 (Annex B)
    GAVL_CODEC_ID_THEORA,         //!< Theora (segmented extradata)
    GAVL_CODEC_ID_DIRAC,          //!< Complete DIRAC frames, sequence end code appended to last packet
    GAVL_CODEC_ID_DV,             //!< DV (several variants)
    GAVL_CODEC_ID_VP8,            //!< VP8 (as used in webm)
  } gavl_codec_id_t;

#define GAVL_BITRATE_VBR -1 //!< Use this to specify a VBR stream
  
  /** \brief Compression format
   *
   *  This defines parameters of the compression. The most important
   *  value is the \ref gavl_codec_id_t. Formats, which support a global
   *  header, store it here as well.
   *
   *  Usually there must be an associated audio or video format, because
   *  some containers need this as well.
   */
  
typedef struct
  {
  uint32_t flags; //!< ORed combination of GAVL_COMPRESSION_* flags
  gavl_codec_id_t id; //!< Codec ID
  
  uint8_t * global_header; //!< Global header
  uint32_t global_header_len;   //!< Length of global header
  
  int32_t bitrate;             //!< Needed by some codecs, negative values mean VBR
  int palette_size;             //!< Size of the embedded palette for image codecs
  uint32_t pre_skip;            //!< Samples to skip at the start

  uint32_t video_buffer_size;   //!< VBV buffer size for video (in BYTES)
  
  uint32_t max_packet_size;     //!< Maximum packet size or 0 if unknown
  } gavl_compression_info_t;

/** \brief Free all dynamically allocated memory of a compression info
 *  \param info A compression info
 */
  
GAVL_PUBLIC
void gavl_compression_info_free(gavl_compression_info_t* info);

/** \brief Dump a compression info to stderr
 *  \param info A compression info
 *
 *  Use this for debugging
 */
  
GAVL_PUBLIC
void gavl_compression_info_dump(const gavl_compression_info_t * info);

/** \brief Dump a compression info to stderr
 *  \param info A compression info
 *  \param indent Indentation
 *
 *  Same as \ref gavl_compression_info_dump but it puts indent whitespaces
 *  in front of each line.
 */

GAVL_PUBLIC
void gavl_compression_info_dumpi(const gavl_compression_info_t * info, int num);

/** \brief Copy a compression info
 *  \param dst Destination
 *  \param src Source
 *
 */

GAVL_PUBLIC
void gavl_compression_info_copy(gavl_compression_info_t * dst,
                                const gavl_compression_info_t * src);

/** \brief Append a Xiph packet to a global header
 *  \param global_header Global header (will be changed)
 *  \param global_header_len Global header length (will be changed)
 *  \param header header to append
 *  \param header_len length of header to append
 */

GAVL_PUBLIC
void gavl_append_xiph_header(uint8_t ** global_header,
                             int * global_header_len,
                             uint8_t * header,
                             int header_len);

/** \brief Extract a Xiph packet to a global header
 *  \param global_header Global header
 *  \param global_header_len Global header length
 *  \param idx Index of the packet (starting with 0)
 *  \param header_len Returns the length of the header
 *  \returns The data of the header packet
 */
  
GAVL_PUBLIC
uint8_t * gavl_extract_xiph_header(uint8_t * global_header,
                                   int global_header_len,
                                   int idx,
                                   int * header_len);
  
  
/** \brief Get the file extension of the corresponding raw format
 *  \param id A codec ID
 *  \param separate If non-null returns 1 if each packet should be in a separate file
 *  \returns The file extension of the raw file or NULL
 *
 *  This function can be used for writing elementary streams to files.
 *  It returns a suitable file extension. If separate is 1, each packet should be
 *  written to a separate file. This basically means, that the codec corresponds to an
 *  image format.
 *
 *  Not all compression formats have a suitable elementary stream format, in this
 *  case NULL is returned for the extension. Most prominent examples are Vorbis and
 *  Theora, which can hardly exist outside an OGG container.
 */
  
GAVL_PUBLIC
const char * gavl_compression_get_extension(gavl_codec_id_t id, int * separate);

/** \brief Check if the compression supports multiple pixelformats
 *  \param id A codec ID
 *  \returns 1 if the compression ID must be associated with a pixelformat, 0 else
 *
 *  This function can be used by decoding libraries to check if the compresison ID
 *  is sufficient or if the pixelformat must be valid in the associated video format.
 *
 */
  
GAVL_PUBLIC
int gavl_compression_need_pixelformat(gavl_codec_id_t id);

/** \brief Check if an audio compression constant frame samples 
 *  \param id A codec ID
 *  \returns 1 if the compression has the same number of samples in each frame, 0 else
 *
 */

GAVL_PUBLIC
int gavl_compression_constant_frame_samples(gavl_codec_id_t id);

/** \brief Check if an audio compression size for each samples 
 *  \param id A codec ID
 *  \returns The coded size of one sample or 0
 */

GAVL_PUBLIC
int gavl_compression_get_sample_size(gavl_codec_id_t id);

/** \brief Return the long name of the compression 
 *  \param id A codec ID
 *  \returns The long name or NULL
 */
  
GAVL_PUBLIC const char *
gavl_compression_get_long_name(gavl_codec_id_t id);

/** \brief Return the short name of the compression 
 *  \param id A codec ID
 *  \returns The short name or NULL
 */

GAVL_PUBLIC const char *
gavl_compression_get_short_name(gavl_codec_id_t id);

/** \brief Get a compression from the short name
 *  \param A short name
 *  \returns The codec ID (or GAVL_CODEC_ID_NONE)
 */

GAVL_PUBLIC gavl_codec_id_t 
gavl_compression_from_short_name(const char * name);

/** \brief Get the number of compression formats
 *  \returns The number of supported compresion formats
 *
 *  Use this with \ref gavl_get_compression to enumerate all
 *  compression formats
 */
  
GAVL_PUBLIC
int gavl_num_compressions();

/** \brief Get a compression format for a specified index
 *  \param index Index (starting with 0)
 *  \returns The compression format
 *
 *  Use this with \ref gavl_num_compressions to enumerate all
 *  compression formats
 */
  
GAVL_PUBLIC
gavl_codec_id_t gavl_get_compression(int index);

  
#define GAVL_PACKET_TYPE_I    0x01      //!< Packet is an I-frame
#define GAVL_PACKET_TYPE_P    0x02      //!< Packet is a P-frame
#define GAVL_PACKET_TYPE_B    0x03      //!< Packet is a B-frame
#define GAVL_PACKET_TYPE_MASK 0x03      //!< Mask for frame type

#define GAVL_PACKET_KEYFRAME (1<<2) //!< Packet is a keyframe
#define GAVL_PACKET_LAST     (1<<3) //!< Packet is the last in the stream (only Xiph codecs need this flag)
#define GAVL_PACKET_EXT      (1<<4) //!< Packet has extensions (used only in gavf files)
#define GAVL_PACKET_REF      (1<<5) //!< B-frame used as reference (can't savely be skipped)
#define GAVL_PACKET_NOOUTPUT (1<<6) //!< Packet will produce no decoder output (e.g. VP8 alternate reference)

#define GAVL_PACKET_PADDING  32 //!< Packets are padded with this many zero bytes
  
/** \brief Packet structure
 *
 *  This specifies one packet of compressed data.
 *  For video streams, each packet must correspond to a video frame.
 *  For audio streams, each packet must be the smallest unit, which
 *  can be decoded indepentently and for which a duration is known.
 *
 *  The typical usage of a packet is to memset() oit to zero in the
 *  beginning. Then for each packet call \ref gavl_packet_alloc
 *  to ensure that enough data is allocated. At the very end call
 *  \ref gavl_packet_free to free all memory.
 */
  
typedef struct
  {
  uint8_t * data; //!< Data
  int data_len;   //!< Length of data
  int data_alloc; //!< How many bytes got allocated

  uint32_t flags; //!< ORed combination of GAVL_PACKET_* flags

  int64_t pts;      //!< Presentation time
  int64_t duration; //!< Duration of the contained frame

  uint32_t field2_offset; //!< Offset of field 2 for field pictures
  uint32_t header_size;   //!< Size of a repeated global header (or 0)
  uint32_t sequence_end_pos;    //!< Position of sequence end code if any

  gavl_interlace_mode_t interlace_mode; //!< Interlace mode for mixed interlacing
  gavl_timecode_t timecode; //!< Timecode
  
  gavl_rectangle_i_t src_rect;  //!< Rectangle to take from a video frame
  int32_t dst_x;             //!< X-coordinate in the destination frame (for overlays)
  int32_t dst_y;             //!< Y-coordinate in the destination frame (for overlays)

  uint32_t id;    //!< ID of the gavf stream where this packet belongs
  
  } gavl_packet_t;

/** \brief Initialize a packet
 *  \param p A packet
 *
 */
  
GAVL_PUBLIC
void gavl_packet_init(gavl_packet_t * p);

  
/** \brief Allocate memory for a packet
 *  \param p A packet
 *  \param len Number of bytes you want to store in the packet
 *
 *  This function uses realloc(), which means that it preserves
 *  the data already contained in the packet.
 */
  
GAVL_PUBLIC
void gavl_packet_alloc(gavl_packet_t * p, int len);

/** \brief Free memory of a packet
 *  \param p A packet
 */
  
GAVL_PUBLIC
void gavl_packet_free(gavl_packet_t * p);

/** \brief Copy a packet
 *  \param dst Destination
 *  \param src Source
 */
  
GAVL_PUBLIC
void gavl_packet_copy(gavl_packet_t * dst,
                      const gavl_packet_t * src);

/** \brief Copy metadata of a packet
 *  \param dst Destination
 *  \param src Source
 *
 *  Like \ref gavl_packet_copy but leaves the
 *  payload of dst untouched.
 *
 *  Since 1.5.0
 */
  
GAVL_PUBLIC
void gavl_packet_copy_metadata(gavl_packet_t * dst,
                               const gavl_packet_t * src);

  
/** \brief Reset a packet
 *  \param p Destination
 *
 *  Set fields to their default values
 */
  
GAVL_PUBLIC
void gavl_packet_reset(gavl_packet_t * p);

  
/** \brief Dump a packet to stderr
 *  \param p A packet
 *
 *  Use this for debugging
 */
  
GAVL_PUBLIC
void gavl_packet_dump(const gavl_packet_t * p);

/** \brief Save a packet to a file
 *  \param p A packet
 *  \param file Filename
 *
 *  Use this for debugging
 */
  
GAVL_PUBLIC
void gavl_packet_save(const gavl_packet_t * p,
                      const char * filename);
  
/**
 *  @}
 */
  
#ifdef __cplusplus
}
#endif
 

#endif // GAVL_COMPRESSION_H_INCLUDED
