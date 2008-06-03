/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#ifndef __BG_EDL_H_
#define __BG_EDL_H_

#include <parameter.h>
#include <streaminfo.h>

/** \defgroup edl EDL support
 *  \ingroup decoding
 *  \brief EDL support
 *
 *  Most media files contain one or more A/V streams. In addition however, there
 *  can be additional instructions, how the media should be played back. Basically
 *  you can have "logical" streams, where the EDL tells how they are composed from
 *  phyiscal streams.
 *
 *  To use EDLs with Gmerlin-avdecoder, note the following:
 *
 *  - If you do nothing, the streams are decoded as they are found in the file
 *  - If a media file contains an EDL, it is returned by \ref bgav_get_edl
 *  - The EDL references streams either in the file you opened, or in external
 *    files.
 *  - Some files contain only the EDL (with external references) but no actual media
 *    streams. In this case, \ref bgav_num_tracks will return 0.
 *  - To use an EDL from a decoder instance, make a local copy (\ref bgav_edl_copy),
 *    close the decoder (\ref bgav_close) and open a new decoder with \ref bgav_open_edl
 *    with the copied EDL
 *  - The opened decoder will behave the same as a regular decoder
 *  - You can also build EDLs yourself and pass them to \ref bgav_open_edl
 *
 * @{
 */

/** \brief Forward declaration
 */

typedef struct bg_edl_s bg_edl_t;


/** \brief One segment of a physical stream to appear in a logical stream
 */

typedef struct
  {
  char * url;   //!< Location of that segment. If NULL, the "master url" in bg_edl_t is valid.

  int track;        //!<  Track index for multitrack inputs
  int stream;       //!<  Index of the A/V stream
  int timescale;    //!<  Source timescale
    
  int64_t src_time; //!< Time within the source in source timescale
  
  /* Time and duration within the destination in destination
     timescale */
  int64_t dst_time;  //!< Time  within the destination in destination timescale
  int64_t dst_duration; //!< Duration within the destination in destination timescale

  /*  */
  int32_t speed_num; //!< Playback speed numerator
  int32_t speed_den; //!< Playback speed demoninator
  } bg_edl_segment_t;

/** \brief A locical stream
 */

typedef struct
  {
  bg_edl_segment_t * segments; //!< Segments
  int num_segments;              //!< Number of segments 
  int timescale;                 //!< Destination timescale
  } bg_edl_stream_t;

/** \brief A locical track
 */

typedef struct
  {
  int num_audio_streams;             //!< Number of logical audio streams
  bg_edl_stream_t * audio_streams; //!< Logical audio streams

  int num_video_streams;             //!< Number of logical video streams
  bg_edl_stream_t * video_streams; //!< Logical video streams

  int num_subtitle_text_streams;     //!< Number of logical text subtitle streams
  bg_edl_stream_t * subtitle_text_streams; //!< Logical text subtitle streams

  int num_subtitle_overlay_streams;  //!< Number of logical overlay subtitle streams
  bg_edl_stream_t * subtitle_overlay_streams; //!< Logical overlay subtitle streams
  
  } bg_edl_track_t;

/** \brief EDL structure
 */

struct bg_edl_s
  {
  int num_tracks;             //!< Number of logical tracks
  bg_edl_track_t * tracks;  //!< Logical tracks
  char * url;                 //!< Filename if all streams are from the same file
  };


bg_edl_t * bg_edl_create();

bg_edl_track_t * bg_edl_add_track(bg_edl_t * e);

bg_edl_stream_t * bg_edl_add_audio_stream(bg_edl_track_t * t);

bg_edl_stream_t * bg_edl_add_video_stream(bg_edl_track_t * t);

bg_edl_stream_t * bg_edl_add_subtitle_text_stream(bg_edl_track_t * t);

bg_edl_stream_t * bg_edl_add_subtitle_overlay_stream(bg_edl_track_t * t);

bg_edl_segment_t * bg_edl_add_segment(bg_edl_stream_t * s);

bg_edl_t * bg_edl_copy(const bg_edl_t * e);

void bg_edl_destroy(bg_edl_t * e);

void bg_edl_dump(const bg_edl_t * e);

bg_edl_t * bg_edl_load(const char * filename);
void bg_edl_save(const bg_edl_t * edl, const char * filename);

void bg_edl_append_track_info(bg_edl_t * e, const bg_track_info_t * info, const char * url, int index);

/**
 * @}
 */


#endif // __BG_EDL_H_
