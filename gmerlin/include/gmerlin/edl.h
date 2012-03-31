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

#ifndef __BG_EDL_H_
#define __BG_EDL_H_

#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>

/** \defgroup edl EDL support
 *  \ingroup plugin_i
 *  \brief EDL support
 *
 *  Most media files contain one or more A/V streams. In addition however, there
 *  can be additional instructions, how the media should be played back. Basically
 *  you can have "logical" streams, where the EDL tells how they are composed from
 *  phyiscal streams.
 *
 *  To use EDLs with gmerlin, note the following:
 *
 *  - If you do nothing, the streams are decoded as they are found in the file
 *  - If a media file contains an EDL, it is returned by the get_edl() method of
 *    the input plugin.
 *  - The EDL references streams either in the file you opened, or in external
 *    files.
 *  - Some files contain only the EDL (with external references) but no actual media
 *    streams. In this case, the get_num_tracks() method of the input plugin will return 0.
 *  - The gmerlin library contains a builtin EDL decoder plugin, which opens the
 *    elementary streams and decodes the EDL as if it was a simple file. It can be used
 *    by calling \ref bg_input_plugin_load. It will fire up an EDL decoder for files, which
 *    contain only EDL data and no media. For other files, the behaviour is controlled by the
 *    prefer_edl argument.
 * @{
 */

/** \brief Forward declaration for the EDL
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
  char * name; //!< Optional name of that track

  bg_metadata_t metadata; //!< Metadata
  
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

/** \brief Create an empty EDL
 *  \returns A newly allocated EDL
 */

bg_edl_t * bg_edl_create();

/** \brief Append a track to the EDL
    \param e An EDL
 *  \returns The new track
 */

bg_edl_track_t * bg_edl_add_track(bg_edl_t * e);

/** \brief Append an audio stream to an EDL track
    \param t An EDL track
 *  \returns The new stream
 */

bg_edl_stream_t * bg_edl_add_audio_stream(bg_edl_track_t * t);

/** \brief Append a video stream to an EDL track
    \param t An EDL track
 *  \returns The new stream
 */

bg_edl_stream_t * bg_edl_add_video_stream(bg_edl_track_t * t);

/** \brief Append a text subtitle stream to an EDL track
    \param t An EDL track
 *  \returns The new stream
 */

bg_edl_stream_t * bg_edl_add_subtitle_text_stream(bg_edl_track_t * t);

/** \brief Append an overlay subtitle stream to an EDL track
    \param t An EDL track
 *  \returns The new stream
 */

bg_edl_stream_t * bg_edl_add_subtitle_overlay_stream(bg_edl_track_t * t);

/** \brief Append a segment to an EDL stream
    \param s An EDL stream
 *  \returns The new segment
 */

bg_edl_segment_t * bg_edl_add_segment(bg_edl_stream_t * s);

/** \brief Copy an entire EDL
    \param e An EDL
 *  \returns Copy of the EDL
 */

bg_edl_t * bg_edl_copy(const bg_edl_t * e);

/** \brief Destroy an EDL and free all memory
    \param e An EDL
 */

void bg_edl_destroy(bg_edl_t * e);

/** \brief Dump an EDL to stderr
    \param e An EDL

    Mainly used for debugging
 */

void bg_edl_dump(const bg_edl_t * e);

/** \brief Save an EDL to an xml file
    \param e An EDL
    \param filename Name of the file

 */


void bg_edl_save(const bg_edl_t * e, const char * filename);

/** \brief Load an EDL from an xml file
    \param filename Name of the file
    \returns The EDL or NULL.
*/

bg_edl_t * bg_edl_load(const char * filename);

/** \brief Append a \ref bg_track_info_t to the EDL
    \param e An EDL
    \param info A track info (see \ref bg_track_info_t)
    \param url The location of the track
    \param index The index of the track in the location
    \param num_tracks The total number of the tracks in the location
    \param name An optional name.

    This function takes a track info (e.g. from an opened input plugin)
    and creates an EDL track, which corresponds to that track.
    
    If name is NULL, the track name will be constructed from the filename.
    
 *
 */


void bg_edl_append_track_info(bg_edl_t * e,
                              const bg_track_info_t * info, const char * url,
                              int index, int num_tracks, const char * name);

/**
 * @}
 */


#endif // __BG_EDL_H_
