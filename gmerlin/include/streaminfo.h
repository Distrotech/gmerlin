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

#include <libxml/tree.h>
#include <libxml/parser.h>

/** \defgroup streaminfo Track information
 *  \ingroup plugin_i
 *
 *  These structures describe media tracks with their streams.
 *  They are returned by the input plugin.
 */

/************************************************
 * Types for describing media streams
 ************************************************/

/* Languages are ISO 639-2 (3 character code) */

/** \ingroup streaminfo
 *  \brief Description of an audio stream
 *
 *  Unknown fields can be NULL.
 */

typedef struct
  {
  gavl_audio_format_t format; //!< Format (invalid until after the start function was called)
  char * description; //!< Something like MPEG-1 audio layer 3, 128 kbps
  char   language[4]; //!< The language in ISO 639-2 (3 character code+'\\0')
  char * info;        //!< Directors comments etc...
  } bg_audio_info_t;

/** \ingroup streaminfo
 *  \brief Description of a video stream
 *
 *  Unknown fields can be NULL.
 */

typedef struct
  {
  gavl_video_format_t format; //!< Format (invalid until after the start function was called)
  char * description; //!< Something like MPEG-1 video 1150 kbps
  char language[4]; //!< The language in ISO 639-2 (3 character code+'\\0')
  char * info;        //!< Info about this stream
  } bg_video_info_t;

/** \ingroup streaminfo
 *  \brief Description of a still image stream
 *
 *  Unknown fields can be NULL.
 */

typedef struct
  {
  gavl_video_format_t format;//!< Format (invalid until after the start function was called)
  char * description; //!< Something like JPEG encoded
  } bg_still_info_t;

/** \ingroup streaminfo
 *  \brief Description of a subtitle stream
 *
 *  Unknown fields can be NULL.
 */

typedef struct
  {
  char * description; //!< Something like subrip format
  char language[4]; //!< The language in ISO 639-2 (3 character code+'\\0')
  char * info;        //!< Info about this stream

  int is_text; //!< 1 if subtitles are in text format (0 for overlay subtitles)
  gavl_video_format_t format; //!< Format of overlay subtitles
  } bg_subtitle_info_t;

/** \ingroup streaminfo
 *  \brief Description of metadata
 *
 *  Unknown fields can be NULL. Strings here MUST be in UTF-8
 */

typedef struct
  {
  char * artist; //!< Artist
  char * title; //!< Title
  char * album; //!< Album
      
  int track; //!< Track number
  char * date; //!< Date
  char * genre; //!< Genre
  char * comment;//!< Comment

  char * author;//!< Author
  char * copyright;//!< Copyright
  } bg_metadata_t;

/** \ingroup streaminfo
 *  \brief Free all strings in a metadata structure
 *  \param m Metadata
 */

void bg_metadata_free(bg_metadata_t * m);

/** \ingroup streaminfo
 *  \brief Copy metadata
 *  \param dst Destination
 *  \param src Source
 *
 *  Make sure, that dst is either memset to 0 before the call or
 *  contains only strings, which can savely be freed.
 */

void bg_metadata_copy(bg_metadata_t * dst, const bg_metadata_t * src);

/** \ingroup streaminfo
 *  \brief Create trackname from metadata
 *  \param m Metadata
 *  \param format Format string
 *  \returns A newly allocated track name or NULL
 *
 *  The format string can contain arbitrary characters and the
 *  following placeholders
 *
 *  - %p: Artist
 *  - %a: Album
 *  - %g: Genre
 *  - %t: Track name
 *  - %c: Comment
 *  - %y: Year
 *  - %\<d\>n: Track number with \<d\> digits
 *
 *  If the string corresponding to a placeholder is NULL, the whole
 *  function is aborted and NULL is returned.
 *  
 */

char * bg_create_track_name(const bg_metadata_t * m, const char * format);

/** \ingroup streaminfo
 *  \brief Convert metadata to a humanized string
 *  \param m Metadata
 *  \param use_tabs Indicate, that tabs (\\t) should be used in the output
 *  \returns A newly allocated string
 */

char * bg_metadata_to_string(const bg_metadata_t * m, int use_tabs);

/** \ingroup streaminfo
 *  \brief Try to get the year from the metadata
 *  \param m Metadata
 *  \returns The year as int
 *
 *  The date string can be in multiple formats. This function
 *  tries to extract the year and return it as int.
 */

int bg_metadata_get_year(const bg_metadata_t * m);

/* XML Interface */

/** \ingroup streaminfo
 *  \brief Convert a libxml2 node into a metadata struct
 *  \param xml_doc Pointer to the xml document
 *  \param xml_metadata Pointer to the xml node containing the metadata
 *  \param ret Metadata container, where the info will be stored
 *
 *  See the libxml2 documentation for more infos
 */


void bg_xml_2_metadata(xmlDocPtr xml_doc, xmlNodePtr xml_metadata,
                       bg_metadata_t * ret);

/** \ingroup streaminfo
 *  \brief Convert a metadata struct into a libxml2 node
 *  \param ret Metadata
 *  \param xml_metadata Pointer to the xml node for the metadata
 *
 *  See the libxml2 documentation for more infos
 */

void bg_metadata_2_xml(xmlNodePtr xml_metadata,
                       bg_metadata_t * ret);

/** \ingroup streaminfo
 *  \brief Get parameters for editing metadata
 *  \param m Metadata
 *  \returns A NULL-terminated array of parameter descriptions
 *
 *  Using this function and \ref bg_metadata_set_parameter lets
 *  you set metadata with the usual configuration methods.
 *  The default values of the returned descriptions are set from
 *  the Metadata.
 *
 *  Call \ref bg_parameter_info_destroy_array to free the returned array
 */

bg_parameter_info_t * bg_metadata_get_parameters(bg_metadata_t * m);

/** \ingroup streaminfo
 *  \brief Change metadata by setting parameters
 *  \param data Metadata casted to void
 *  \param name Name of the parameter
 *  \param v Value
 */

void bg_metadata_set_parameter(void * data, char * name,
                               bg_parameter_value_t * v);


/** \ingroup streaminfo
 *  \brief Chapter list
 *
 *  Chapters in gmerlin are simply an array of
 *  seekpoints with (optionally) associated names.
 *  They are valid as soon as the file is opened
 */

typedef struct
  {
  int num_chapters;       //!< Number of chapters
  struct
    {
    gavl_time_t time;     //!< Start time (seekpoint) of this chapter
    char * name;          //!< Name for this chapter (or NULL if unavailable)
    } * chapters;
  } bg_chapter_list_t;

/** \ingroup streaminfo
 *  \brief Create chapter list
 *  \param num_chapters Initial number of chapters
 */


bg_chapter_list_t * bg_chapter_list_create(int num_chapters);

/** \ingroup streaminfo
 *  \brief Copy chapter list
 *  \param list Chapter list
 */

bg_chapter_list_t * bg_chapter_list_copy(const bg_chapter_list_t * list);


/** \ingroup streaminfo
 *  \brief Destroy chapter list
 *  \param list A chapter list
 */

void bg_chapter_list_destroy(bg_chapter_list_t * list);
/** \ingroup streaminfo
 *  \brief Insert a chapter into a chapter list
 *  \param list A chapter list
 *  \param index Position (starting with 0) where the new chapter will be placed
 */

void bg_chapter_list_insert(bg_chapter_list_t * list, int index,
                            int64_t time, const char * name);

/** \ingroup streaminfo
 *  \brief Delete a chapter from a chapter list
 *  \param list A chapter list
 *  \param index Position (starting with 0) of the chapter to delete
 */

void bg_chapter_list_delete(bg_chapter_list_t * list, int index);

/** \ingroup streaminfo
 *  \brief Set default chapter names
 *  \param list A chapter list
 *
 *  If no names for the chapters are avaiable, this function will
 *  set them to "Chapter 1", "Chapter 2" etc.
 */

void bg_chapter_list_set_default_names(bg_chapter_list_t * list);

/** \ingroup streaminfo
 *  \brief Get current chapter
 *  \param list A chapter list
 *  \param time Playback time
 *  \returns The current chapter index
 *
 *  Use this function after seeking to signal a
 *  chapter change
 */

int bg_chapter_list_get_current(bg_chapter_list_t * list,
                                 gavl_time_t time);

/** \ingroup streaminfo
 *  \brief Get current chapter
 *  \param list A chapter list
 *  \param time Playback time
 *  \param current_chapter Returns the current chapter
 *  \returns 1 if the chapter changed, 0 else
 *
 *  Use this function during linear playback to signal a
 *  chapter change
 */

int bg_chapter_list_changed(bg_chapter_list_t * list,
                            gavl_time_t time, int * current_chapter);


/** \ingroup streaminfo
 *  \brief Convert a chapter list into a libxml2 node
 *  \param list Chapter list
 *  \param xml_list Pointer to the xml node for the chapter list
 *
 *  See the libxml2 documentation for more infos
 */

void bg_chapter_list_2_xml(bg_chapter_list_t * list, xmlNodePtr xml_list);

/** \ingroup streaminfo
 *  \brief Convert libxml2 node into a chapter list
 *  \param xml_doc Pointer to the xml document
 *  \param xml_list Pointer to the xml node for chapter list
 *  \returns The chapter list from the xml node
 *
 *  See the libxml2 documentation for more infos
 */

bg_chapter_list_t *
bg_xml_2_chapter_list(xmlDocPtr xml_doc, xmlNodePtr xml_list);

/** \ingroup streaminfo
 *  \brief Save a chapter list to a file
 *  \param list A chapter list
 *  \param filename Where to save the list
 */

void bg_chapter_list_save(bg_chapter_list_t * list, const char * filename);

/** \ingroup streaminfo
 *  \brief Load a chapter list from a file
 *  \param filename From where to load the list
 *  \returns A newly created chapter list or NULL
 */

bg_chapter_list_t * bg_chapter_list_load(const char * filename);


/** \ingroup streaminfo
 *  \brief Track info
 */

typedef struct
  {
  int seekable; //!< 1 if track is seekable (duration must be > 0 then)
  //  int redirector; //!< 1 if we are a redirector, the url field must be valid then
  char * name;             //!< Name of the track (can be NULL)
  char * description;      //!< Technical desription of the format
  int64_t duration;        //!< Duration
  
  int num_audio_streams;   //!< Number of audio streams
  int num_video_streams;   //!< Number of video streams
  int num_still_streams;   //!< Number of still image streams
  int num_subtitle_streams;//!< Number of subtitle streams
  
  bg_audio_info_t *    audio_streams; //!< Audio streams
  bg_video_info_t *    video_streams; //!< Video streams
  bg_still_info_t *    still_streams; //!< Still streams
  bg_subtitle_info_t * subtitle_streams; //!< Subtitle streams

  bg_metadata_t metadata; //!< Metadata (optional)
  
  /* The following are only meaningful for redirectors */
  
  char * url; //!< URL (needed if is_redirector field is nonzero)

  bg_chapter_list_t * chapter_list; //!< Chapter list (or NULL)
  
  } bg_track_info_t;

/** \ingroup streaminfo
 *  \brief Free all allocated memory in a track info
 *  \param info Track info
 *
 *  This one can be called by plugins to free
 *  all allocated memory contained in a track info.
 *  Note, that you have to free() the structure
 *  itself after.
 */

void bg_track_info_free(bg_track_info_t * info);

/** \ingroup streaminfo
 *  \brief Set the track name from the filename/URL
 *  \param info Track info
 *  \param location filename or URL
 *
 *  This is used for cases, where the input plugin didn't set a track name,
 *  and the name cannot (or shouldn't) be set from the metadata.
 *  If location is an URL, the whole URL will be copied into the name field.
 *  If location is a local filename, the path and extension will be removed.
 */

void bg_set_track_name_default(bg_track_info_t * info,
                               const char * location);

/** \ingroup streaminfo
 *  \brief Get a track name from the filename/URL
 *  \param location filename or URL
 *  \returns A newly allocated track name which must be freed by the caller
 *
 *  If location is an URL, the whole URL will be copied into the name field.
 *  If location is a local filename, the path and extension will be removed.
 */

char * bg_get_track_name_default(const char * location);

#endif // /__BG_STREAMINFO_H_
