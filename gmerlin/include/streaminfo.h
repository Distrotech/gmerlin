/*****************************************************************
 
  streaminfo.h
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#ifndef __BG_STREAMINFO_H_
#define __BG_STREAMINFO_H_

#include <gavl/gavl.h>

/************************************************
 * Types for describing media streams
 ************************************************/

typedef struct
  {
  gavl_audio_format_t format;
  char * description; /* Something line MPEG-1 audio layer 3, 128 kbps */
  char * language;
  } bg_audio_info_t;

typedef struct
  {
  gavl_video_format_t format;
  char * description;
  } bg_video_info_t;

typedef struct
  {
  char * language;
  char * description;
  } bg_subpicture_info_t;

typedef struct
  {
  /* Strings here MUST be in UTF-8 */
  char * artist;
  char * title;
  char * album;
  int year;
  int track;
  char * genre;
  char * comment;
  } bg_metadata_t;

typedef struct
  {
  int seekable; /* 1 is track is seekable (duration MUST be > 0 then) */
  int num_audio_streams;
  int num_video_streams;
  int num_subpicture_streams;
  int num_programs;
  char * name;
  char * description;
  int64_t duration;

  bg_audio_info_t *    audio_streams;
  bg_video_info_t *    video_streams;
  bg_subpicture_info_t * subpicture_streams;

  /* Metadata (optional) */
  
  bg_metadata_t metadata;

  } bg_track_info_t;

/*
 *  This one can be called by plugins to free
 *  all strings contained in a stream info.
 *  Note, that you have to free() the structure
 *  itself after.
 */

void bg_track_info_free(bg_track_info_t *);
char * bg_create_track_name(const bg_track_info_t *, const char * format);

#endif // /__BG_STREAMINFO_H_
