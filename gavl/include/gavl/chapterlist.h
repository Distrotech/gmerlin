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

#ifndef GAVL_CHAPTERLIST_H_INCLUDED
#define GAVL_CHAPTERLIST_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <gavl/gavldefs.h>

/** \defgroup chapterlist Chapter list
 *  \brief Chapter list
 *
 *  Chapters in gavl are simply 
 *  seekpoints with (optionally) associated names.
 *
 *  Since 1.5.0
 *
 * @{
 */

/** \brief Chapter list
 *
 */
  
typedef struct
  {
  uint32_t num_chapters;       //!< Number of chapters
  uint32_t timescale;          //!< Scale of the timestamps
  struct
    {
    int64_t time;         //!< Start time (seekpoint) of this chapter
    char * name;          //!< Name for this chapter (or NULL if unavailable)
    } * chapters;         //!< Chapters
  } gavl_chapter_list_t;

/** \brief Create chapter list
 *  \param num_chapters Initial number of chapters
 */

GAVL_PUBLIC
gavl_chapter_list_t * gavl_chapter_list_create(int num_chapters);

/** \brief Copy chapter list
 *  \param list Chapter list
 */

GAVL_PUBLIC
gavl_chapter_list_t * gavl_chapter_list_copy(const gavl_chapter_list_t * list);


/** \brief Destroy chapter list
 *  \param list A chapter list
 */

GAVL_PUBLIC
void gavl_chapter_list_destroy(gavl_chapter_list_t * list);
/** \brief Insert a chapter into a chapter list
 *  \param list A chapter list
 *  \param index Position (starting with 0) where the new chapter will be placed
 *  \param time Start time of the chapter
 *  \param name Chapter name (or NULL)
 */

GAVL_PUBLIC
void gavl_chapter_list_insert(gavl_chapter_list_t * list, int index,
                            int64_t time, const char * name);

/** \brief Delete a chapter from a chapter list
 *  \param list A chapter list
 *  \param index Position (starting with 0) of the chapter to delete
 */

GAVL_PUBLIC
void gavl_chapter_list_delete(gavl_chapter_list_t * list, int index);


/** \brief Get current chapter
 *  \param list A chapter list
 *  \param time Playback time
 *  \returns The current chapter index
 *
 *  Use this function after seeking to signal a
 *  chapter change
 */

GAVL_PUBLIC
int gavl_chapter_list_get_current(gavl_chapter_list_t * list,
                                 gavl_time_t time);

/** \brief Get current chapter
 *  \param list A chapter list
 *  \param time Playback time
 *  \param current_chapter Returns the current chapter
 *  \returns 1 if the chapter changed, 0 else
 *
 *  Use this function during linear playback to signal a
 *  chapter change
 */

GAVL_PUBLIC
int gavl_chapter_list_changed(gavl_chapter_list_t * list,
                              gavl_time_t time, int * current_chapter);

/** \brief Dump a chapter list to stderr
 *  \param list A chapter list
 *
 *  Use this for debugging
 */

GAVL_PUBLIC
void gavl_chapter_list_dump(const gavl_chapter_list_t * list);
  
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // GAVL_CHAPTERLIST_H_INCLUDED
