/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#ifndef __BG_STREAMINFO_H_
#define __BG_STREAMINFO_H_

#include <gavl/gavl.h>
#include <gavl/metadata.h>
#include <gavl/chapterlist.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

/** \defgroup streaminfo Track information
 *  \ingroup plugin_i
 *
 *  These structures describe media tracks with their streams.
 *  They are returned by the input plugin.
 *
 *  @{
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

  gavl_metadata_t m; //!< Metadata
  
  int64_t duration;   //!< Duration in timescale tics
  int64_t pts_offset; //!< First timestamp
  } bg_audio_info_t;

/** \brief Description of a video stream
 *
 *  Unknown fields can be NULL.
 */

typedef struct
  {
  gavl_video_format_t format; //!< Format (invalid before the start function was called)
  gavl_metadata_t m; //!< Metadata

  int64_t duration;   //!< Duration in timescale tics
  int64_t pts_offset; //!< First timestamp
  } bg_video_info_t;

/** \brief Description of a subtitle stream
 *
 *  Unknown fields can be NULL.
 */

typedef struct
  {
  gavl_metadata_t m; //!< Metadata
  int is_text; //!< 1 if subtitles are in text format (0 for overlay subtitles)
  gavl_video_format_t format; //!< Format of overlay subtitles
  int64_t duration;   //!< Duration in timescale tics
  } bg_subtitle_info_t;

/** \brief Create trackname from metadata
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

char * bg_create_track_name(const gavl_metadata_t * m, const char * format);

/** \brief Convert metadata to a humanized string
 *  \param m Metadata
 *  \param use_tabs Indicate, that tabs (\\t) should be used in the output
 *  \returns A newly allocated string
 */

char * bg_metadata_to_string(const gavl_metadata_t * m, int use_tabs);

/** \brief Try to get the year from the metadata
 *  \param m Metadata
 *  \returns The year as int
 *
 *  The date string can be in multiple formats. This function
 *  tries to extract the year and return it as int.
 */

int bg_metadata_get_year(const gavl_metadata_t * m);


/* XML Interface */

/** \brief Convert a libxml2 node into a metadata struct
 *  \param xml_doc Pointer to the xml document
 *  \param xml_metadata Pointer to the xml node containing the metadata
 *  \param ret Metadata container, where the info will be stored
 *
 *  See the libxml2 documentation for more infos
 */

void bg_xml_2_metadata(xmlDocPtr xml_doc, xmlNodePtr xml_metadata,
                       gavl_metadata_t * ret);

/** \brief Convert a metadata struct into a libxml2 node
 *  \param ret Metadata
 *  \param xml_metadata Pointer to the xml node for the metadata
 *
 *  See the libxml2 documentation for more infos
 */

void bg_metadata_2_xml(xmlNodePtr xml_metadata,
                       gavl_metadata_t * ret);

/** \brief Get parameters for editing metadata
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

bg_parameter_info_t * bg_metadata_get_parameters(gavl_metadata_t * m);

/** \brief Get parameters for editing metadata
 *  \param m Metadata
 *  \returns A NULL-terminated array of parameter descriptions
 *
 *  This function works exactly like \ref bg_metadata_get_parameters
 *  but it returns only the parameters suitable for mass tagging.
 *  Using this function and \ref bg_metadata_set_parameter lets
 *  you set metadata with the usual configuration methods.
 *  The default values of the returned descriptions are set from
 *  the Metadata.
 *
 *  Call \ref bg_parameter_info_destroy_array to free the returned array
 */

bg_parameter_info_t * bg_metadata_get_parameters_common(gavl_metadata_t * m);


/** \brief Change metadata by setting parameters
 *  \param data Metadata casted to void
 *  \param name Name of the parameter
 *  \param v Value
 */

void bg_metadata_set_parameter(void * data, const char * name,
                               const bg_parameter_value_t * v);

/** \brief Set default chapter names
 *  \param list A chapter list
 *
 *  If no names for the chapters are avaiable, this function will
 *  set them to "Chapter 1", "Chapter 2" etc.
 */

void bg_chapter_list_set_default_names(gavl_chapter_list_t * list);

/** \brief Convert a chapter list into a libxml2 node
 *  \param list Chapter list
 *  \param xml_list Pointer to the xml node for the chapter list
 *
 *  See the libxml2 documentation for more infos
 */

void bg_chapter_list_2_xml(gavl_chapter_list_t * list, xmlNodePtr xml_list);

/** \brief Convert libxml2 node into a chapter list
 *  \param xml_doc Pointer to the xml document
 *  \param xml_list Pointer to the xml node for chapter list
 *  \returns The chapter list from the xml node
 *
 *  See the libxml2 documentation for more infos
 */

gavl_chapter_list_t *
bg_xml_2_chapter_list(xmlDocPtr xml_doc, xmlNodePtr xml_list);

/** \brief Save a chapter list to a file
 *  \param list A chapter list
 *  \param filename Where to save the list
 */

void bg_chapter_list_save(gavl_chapter_list_t * list, const char * filename);

/** \brief Load a chapter list from a file
 *  \param filename From where to load the list
 *  \returns A newly created chapter list or NULL
 */

gavl_chapter_list_t * bg_chapter_list_load(const char * filename);

#define BG_TRACK_SEEKABLE (1<<0) //!< Track is seekable
#define BG_TRACK_PAUSABLE (1<<1) //!< Track is pausable

/** \brief Track info
 */

typedef struct
  {
  int flags;             //!< 1 if track is seekable (duration must be > 0 then)
  int64_t duration;      //!< Duration
  
  int num_audio_streams;   //!< Number of audio streams
  int num_video_streams;   //!< Number of video streams
  int num_subtitle_streams;//!< Number of subtitle streams
  
  bg_audio_info_t *    audio_streams; //!< Audio streams
  bg_video_info_t *    video_streams; //!< Video streams
  bg_subtitle_info_t * subtitle_streams; //!< Subtitle streams

  gavl_metadata_t metadata; //!< Metadata (optional)
  
  /* The following are only meaningful for redirectors */
  
  char * url; //!< URL (needed if is_redirector field is nonzero)

  gavl_chapter_list_t * chapter_list; //!< Chapter list (or NULL)
  
  } bg_track_info_t;

/** \brief Free all allocated memory in a track info
 *  \param info Track info
 *
 *  This one can be called by plugins to free
 *  all allocated memory contained in a track info.
 *  Note, that you have to free() the structure
 *  itself after.
 */

void bg_track_info_free(bg_track_info_t * info);

/** \brief Set the track name from the filename/URL
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

/** \brief Get a track name from the filename/URL
 *  \param location filename or URL
 *  \returns A newly allocated track name which must be freed by the caller
 *  \param track Track index
 *  \param num_tracks Total number of tracks of the location
 *
 *  If location is an URL, the whole URL will be copied into the name field.
 *  If location is a local filename, the path and extension will be removed.
 */

char * bg_get_track_name_default(const char * location, int track, int num_tracks);

/**
 *  @}
 */


#endif // /__BG_STREAMINFO_H_
