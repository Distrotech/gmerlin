/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
#include <lqt.h>

/*! \mainpage

 \section Introduction
 This is the API documentation for lqtgavl, a gavl wrapper for libquicktime.
 Click Modules (on top of the page) to get to the main API index.
 Here, you find just some general blabla :)

 \section Usage
 The recommended usage is to copy the files include/lqtgavl.h
 and lib/lqtgavl.c into your sourcetree. This is to small to distribute as
 an external library.
*/

/** \defgroup encode Encoding related functions
 */

/** \defgroup decode Decoding related functions
 */

/** \defgroup rows Frame rows pointers
 *
 *  Video frames in libquicktime and gavl are compatible for planar formats.
 *  For packed formats however, we need an additional array for storing the
 *  rows. We could allocate and free them dynamically, causing a lot of overhead.
 *  Therefore, we supply 2 functions for creating and freeing the row pointers so
 *  they can be reused.
 */

/** \ingroup rows
 *  \brief Creeate a row pointer array.
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \returns A row pointer array, you can pass to \ref lqt_gavl_encode_video or
 *  \ref lqt_gavl_decode_video.
 */

uint8_t ** lqt_gavl_rows_create(quicktime_t * file, int track);

/** \ingroup rows
    \brief Free rows array
    \param rows Row array crated with \ref lqt_gavl_rows_create.
*/

void lqt_gavl_rows_destroy(uint8_t** rows);


/** \ingroup encode
 *  \brief Set up an audio stream for encoding
 *  \param file A quicktime handle
 *  \param format Audio format
 *  \param codec The codec to use
 *
 * This function sets up an audio stream for encoding.
 * This function will change the format parameter according to
 * what you must pass to the encode calls. If the format is different
 * from the source format, you need a \ref gavl_audio_converter_t.
 *
 * You can pass NULL for the codec. In this case you must call
 * \ref lqt_gavl_set_audio_codec later on.
 * 
 */

void lqt_gavl_add_audio_track(quicktime_t * file,
                               gavl_audio_format_t * format,
                               lqt_codec_info_t * codec);

/** \ingroup encode
 *  \brief Set up an audio stream for encoding
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param format Audio format
 *  \param codec The codec to use
 */

void lqt_gavl_set_audio_codec(quicktime_t * file,
                              int track,
                              gavl_audio_format_t * format,
                              lqt_codec_info_t * codec);


/** \ingroup encode
 *  \brief Set up a video stream for encoding
 *  \param file A quicktime handle
 *  \param format Video format
 *  \param codec The codec to use
 *
 * This function sets up a video stream for encoding.
 * This function will change the format parameter according to
 * what you must pass to the encode calls. If the format is different
 * from the source format, you need a \ref gavl_video_converter_t.
 *
 * If the video format contains a valid timecode format, a timecode track
 * will be added and attached to the video stream. If you don't like this,
 * set format->timecode_format.int_framerate to 0 before calling this function.
 *
 * You can pass NULL for the codec. In this case you must call
 * \ref lqt_gavl_set_video_codec later on.
 */

void lqt_gavl_add_video_track(quicktime_t * file,
                              gavl_video_format_t * format,
                              lqt_codec_info_t * codec);

/** \ingroup encode
 *  \brief Set up a video stream for encoding
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param format Video format (will be updated according to codec)
 *  \param codec The codec to use
 */

void lqt_gavl_set_video_codec(quicktime_t * file,
                              int track,
                              gavl_video_format_t * format,
                              lqt_codec_info_t * codec);


/* Encode audio/video */

/** \ingroup encode
 *  \brief Encode a video frame
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param frame Video frame
 *  \param rows Rows (created with \ref lqt_gavl_rows_create)
 *  \returns 1 if a frame could be encoded, 0 else
 *
 *  Pass one audio frame to libquicktime for encoding.
 *  The format must be the same as returned by \ref lqt_gavl_add_audio_track.
 *  The samples_per_frame member of the frame can be larger than
 *  the samples_per_frame member of the format.
 *
 *  If the frame contains a valid timecode and timecodes are enabled for this
 *  track, it is written to the file as well.
 */

int lqt_gavl_encode_video(quicktime_t * file, int track,
                          gavl_video_frame_t * frame, uint8_t ** rows, int64_t pts_offset);

/** \ingroup encode
 *  \brief Encode a video frame
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param frame Video frame
 *
 *  Pass one video frame to libquicktime for encoding.
 *  The format must be the same as returned by \ref lqt_gavl_add_video_track.
 *  If the time_scaled member of the frame is no multiple of the frame_duration
 *  the framerate is variable.
 */


void lqt_gavl_encode_audio(quicktime_t * file, int track,
                           gavl_audio_frame_t * frame);

/* Get formats for decoding */

/** \ingroup decode
 *  \brief Get the audio format for decoding
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param format Returns the format.
 *  \returns 1 on success, 0 if there is no such track.
 */

int lqt_gavl_get_audio_format(quicktime_t * file,
                              int track,
                              gavl_audio_format_t * format);

/** \ingroup decode
 *  \brief Get the video format for decoding
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param format Returns the format.
 *  \param encode Set to 1 if encoding, 0 for decoding
 *  \returns 1 on success, 0 if there is no such track.
 */

int lqt_gavl_get_video_format(quicktime_t * file,
                              int track,
                              gavl_video_format_t * format, int encode);

/* Decode audio/video */

/** \ingroup decode
 *  \brief Decode one video frame
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param frame The frame
 *  \param rows Rows (created with \ref lqt_gavl_rows_create)
 *  \returns 1 on success, 0 for EOF.
 **/

int lqt_gavl_decode_video(quicktime_t * file,
                          int track,
                          gavl_video_frame_t * frame, uint8_t ** rows);

/** \ingroup decode
 *  \brief Decode one audio frame
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param frame The frame
 *  \param samples Number of samples to decode.
 *  \returns The real number of decoded samples, 0 for EOF.
 **/

int lqt_gavl_decode_audio(quicktime_t * file,
                          int track,
                          gavl_audio_frame_t * frame, int samples);

/** \ingroup decode
 *  \brief Seek all tracks to a specific point
 *  \param file A quicktime handle
 *  \param time The time to seek to.
 *
 * This call sets the time argument to the real time, we have now.
 * It might be changed if you seek between 2 video frames.
 **/

void lqt_gavl_seek(quicktime_t * file, gavl_time_t * time);

/** \ingroup decode
 *  \brief Seek all tracks to a specific point given as a scaled time
 *  \param file A quicktime handle
 *  \param time The scaled time to seek to
 *  \param scale The timescale
 *
 * This call sets the time argument to the real time, we have now.
 * It might be changed if you seek between 2 video frames.
 **/

void lqt_gavl_seek_scaled(quicktime_t * file, gavl_time_t * time, int scale);

                   
/** \ingroup decode
 *  \brief Get the total duration
 *  \param file A quicktime handle
 *  \returns The duration
 *
 * Return the whole duration of the file as a \ref gavl_time_t
 **/

gavl_time_t lqt_gavl_duration(quicktime_t * file);

/** \defgroup compression Functions for compressed frame I/O
 */

/** \ingroup compression
 *  \brief Get audio compression info
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param ci Returns compression info
 *  \returns 1 if a compression info was returned, 0 else
 *
 *  Free the returned info structure with \ref gavl_compression_info_free
 */

int lqt_gavl_get_audio_compression_info(quicktime_t * file, int track,
                                        gavl_compression_info_t * ci);

/** \ingroup compression
 *  \brief Get video compression info
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param ci Returns compression info
 *  \returns 1 if a compression info was returned, 0 else
 *
 *  Free the returned info structure with \ref gavl_compression_info_free
 */

int lqt_gavl_get_video_compression_info(quicktime_t * file, int track,
                                        gavl_compression_info_t * ci);

/** \ingroup compression
 *  \brief Read an audio packet
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param p Packet
 *  \returns 1 if a packet was read, 0 else
 *
 */

int lqt_gavl_read_audio_packet(quicktime_t * file, int track, gavl_packet_t * p);

/** \ingroup compression
 *  \brief Read a video packet
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param p Packet
 *  \returns 1 if a packet was read, 0 else
 *
 */

int lqt_gavl_read_video_packet(quicktime_t * file, int track, gavl_packet_t * p);

/** \ingroup compression
 *  \brief Check if a compressed audio stream is supported 
 *  \param type File type
 *  \param format Audio format
 *  \param ci Compression info
 *  \returns 1 if writing compressed packets is supported, 0 else
 */

int lqt_gavl_writes_compressed_audio(lqt_file_type_t type,
                                     const gavl_audio_format_t * format,
                                     const gavl_compression_info_t * ci);

/** \ingroup compression
 *  \brief Check if a compressed video stream is supported 
 *  \param type File type
 *  \param format Video format
 *  \param ci Compression info
 *  \returns 1 if writing compressed packets is supported, 0 else
 */

int lqt_gavl_writes_compressed_video(lqt_file_type_t type,
                                     const gavl_video_format_t * format,
                                     const gavl_compression_info_t * ci);

/** \ingroup compression
 *  \brief Add an audio stream for compressed writing 
 *  \param file A quicktime handle
 *  \param format Audio format
 *  \param ci Compression info
 */

int lqt_gavl_add_audio_track_compressed(quicktime_t * file,
                                        const gavl_audio_format_t * format,
                                        const gavl_compression_info_t * ci);

/** \ingroup compression
 *  \brief Add a video stream for compressed writing 
 *  \param file A quicktime handle
 *  \param format Video format
 *  \param ci Compression info
 */

int lqt_gavl_add_video_track_compressed(quicktime_t * file,
                                        const gavl_video_format_t * format,
                                        const gavl_compression_info_t * ci);

/** \ingroup compression
 *  \brief Write an audio packet 
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param p Packet
 */

int lqt_gavl_write_audio_packet(quicktime_t * file, int track, gavl_packet_t * p);

/** \ingroup compression
 *  \brief Write a video packet 
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param p Packet
 */

int lqt_gavl_write_video_packet(quicktime_t * file, int track, gavl_packet_t * p);
