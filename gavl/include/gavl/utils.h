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

/** \defgroup utils Utilities
 *  \brief Utility functions
 *
 *  These are some utility functions, which should be used, whereever
 *  possible, instead of their libc counterparts (if any). That's because
 *  they also handle portability issues, ans it's easier to make one single
 *  function portable (with \#ifdefs if necessary) and use it throughout
 *  the code.
 *
 *  @{
 */

/** \brief Dump to stderr
 *  \param format Format (printf compatible)
 */

GAVL_PUBLIC
void gavl_dprintf(const char * format, ...)
  __attribute__ ((format (printf, 1, 2)));

/** \brief Dump to stderr with intendation
 *  \param indent How many spaces to prepend
 *  \param format Format (printf compatible)
 */

GAVL_PUBLIC
void gavl_diprintf(int indent, const char * format, ...)
  __attribute__ ((format (printf, 2, 3)));

/** \brief Do a hexdump of binary data
 *  \param data Data
 *  \param len Length
 *  \param linebreak How many bytes to print in each line before a linebreak
 *  \param indent How many spaces to prepend
 *
 *  This is mostly for debugging
 */

GAVL_PUBLIC
void gavl_hexdumpi(const uint8_t * data, int len, int linebreak, int indent);

/** \brief Do a hexdump of binary data
 *  \param data Data
 *  \param len Length
 *  \param linebreak How many bytes to print in each line before a linebreak
 *
 *  This is mostly for debugging
 */

GAVL_PUBLIC
void gavl_hexdump(const uint8_t * data, int len, int linebreak);


/** \brief Print into a string
 *  \param format printf like format
 *
 *  All other arguments must match the format like in printf.
 *  This function allocates the returned string, thus it must be
 *  freed by the caller.
 */

GAVL_PUBLIC
char * gavl_sprintf(const char * format,...)
  __attribute__ ((format (printf, 1, 2)));


/** \brief Replace a string
 *  \param old_string Old string (will be freed)
 *  \param new_string How the new string will look like
 *  \returns A copy of new_string
 *
 *  This function handles correctly the cases where either argument
 *  is NULL. The emtpy string is treated like a NULL (non-existant)
 *  string.
 */


GAVL_PUBLIC
char * gavl_strrep(char * old_string,
                   const char * new_string);

/** \brief Replace a string
 *  \param old_string Old string (will be freed)
 *  \param new_string How the new string will look like
 *  \param new_string_end End pointer of the new string
 *  \returns A copy of new_string
 *
 *  Like \ref gavl_strrep but the end of the string is also given
 */


GAVL_PUBLIC
char * gavl_strnrep(char * old_string,
                    const char * new_string_start,
                    const char * new_string_end);

/** \brief Duplicate a string
 *  \param new_string How the new string will look like
 *  \returns A copy of new_string
 *
 *  This function handles correctly the cases where either argument
 *  is NULL. The emtpy string is treated like a NULL (non-existant)
 *  string.
 */

GAVL_PUBLIC
char * gavl_strdup(const char * new_string);

/** \brief Duplicate a string
 *  \param new_string How the new string will look like
 *  \param new_string_end End pointer of the new string
 *  \returns A copy of new_string
 *
 *  Like \ref gavl_strdup but the end of the string is also given
 */


GAVL_PUBLIC
char * gavl_strndup(const char * new_string,
                    const char * new_string_end);

/** \brief Concatenate two strings
 *  \param old Old string (will be freed)
 *  \param tail Will be appended to old_string
 *  \returns A newly allocated string.
 */

GAVL_PUBLIC
char * gavl_strcat(char * old, const char * tail);

/** \brief Append a part of a string to another string
 *  \param old Old string (will be freed)
 *  \param start Start of the string to be appended
 *  \param end Points to the first character after the end of the string to be appended
 *  \returns A newly allocated string.
 */

GAVL_PUBLIC
char * gavl_strncat(char * old, const char * start, const char * end);

/* @} */
