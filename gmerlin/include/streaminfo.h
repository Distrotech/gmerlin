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

void bg_audio_info_copy(bg_audio_info_t * dst, const bg_audio_info_t * src);
void bg_audio_info_free(bg_audio_info_t * info);


typedef struct
  {
  gavl_video_format_t format;
  char * description;
  } bg_video_info_t;

void bg_video_info_copy(bg_video_info_t * dst, const bg_video_info_t * src);
void bg_video_info_free(bg_video_info_t * info);

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
      
  int track;
  char * date;
  char * genre;
  char * comment;

  char * author;
  char * copyright;
  } bg_metadata_t;

void bg_metadata_free(bg_metadata_t * m);
void bg_metadata_copy(bg_metadata_t * dst, const bg_metadata_t * src);

/*
 *  Get parameters for configuring metadata
 *  call bg_parameter_info_destroy_array(bg_parameter_info_t * info);
 *  to free the returned array
 */

bg_parameter_info_t * bg_metadata_get_parameters(bg_metadata_t * m);

void bg_metadata_set_parameter(void * data, char * name,
                               bg_parameter_value_t * v);

typedef struct
  {
  int seekable; /* 1 is track is seekable (duration MUST be > 0 then) */
  int redirector; /* 1 if we are a redirector, the url field must be
                     valid then */
    
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

  /* The following are only meaningful for redirectors */
  
  char * url;
  
  } bg_track_info_t;

/*
 *  This one can be called by plugins to free
 *  all strings contained in a stream info.
 *  Note, that you have to free() the structure
 *  itself after.
 */

void bg_track_info_free(bg_track_info_t *);

char * bg_create_track_name(const bg_track_info_t *, const char * format);

void bg_set_track_name_default(bg_track_info_t *,
                               const char * location);
char * bg_get_track_name_default(const char * location);

#endif // /__BG_STREAMINFO_H_
