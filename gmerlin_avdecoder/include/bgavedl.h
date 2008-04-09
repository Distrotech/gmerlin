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

/* EDL Support */

typedef struct
  {
  char * url;            /* Location of that segment          */
  int track;             /* Track index for multitrack inputs */
  int stream;            /* Index of the A/V stream           */
  int timescale;         /* Source timescale                  */
  int64_t src_timescale;
  
  /* If source duration != dst duration, the playback speed of
     the stream must be changed */
  
  /* Time within the source in source timescale */
  int64_t src_time;
  int64_t src_duration;

  /* Time within the destination in stream timescale */
  int64_t dst_time;
  int64_t dst_duration;
  } bgav_edl_segment_t;

/* This is output as one A/V stream */
typedef struct
  {
  bgav_edl_segment_t * segments;
  int num_segments;
  int dst_timescale;
  } bgav_edl_stream_t;

typedef struct
  {
  int num_audio_streams;
  bgav_edl_stream_t * audio_streams;

  int num_video_streams;
  bgav_edl_stream_t * video_streams;

  int num_subtitle_streams;
  bgav_edl_stream_t * subtitle_streams;
  
  } bgav_edl_track_t;

typedef struct
  {
  int num_tracks;
  bgav_edl_track_t * tracks;
  } bgav_edl_t;

bgav_edl_t * bgav_edl_create();

bgav_edl_track_t * bgav_edl_add_track(bgav_edl_t *);

bgav_edl_stream_t * bgav_edl_add_audio_stram(bgav_edl_track_t * t);
bgav_edl_stream_t * bgav_edl_add_video_stram(bgav_edl_track_t * t);
bgav_edl_stream_t * bgav_edl_add_subtitle_stram(bgav_edl_track_t * t);

bgav_edl_segment_t * bgav_edl_add_segment(bgav_edl_stream_t * s);

bgav_edl_t * bgav_edl_copy(const bgav_edl_t * e);
void bgav_edl_destroy(bgav_edl_t * e);
