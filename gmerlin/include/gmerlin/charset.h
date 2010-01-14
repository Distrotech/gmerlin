/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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


/* Charset utilities (charset.c) */

/** \defgroup charset Charset utilities
 *  \ingroup utils
 *
 *  @{
 */

/** \brief Opaque charset converter
 *
 * You don't want to know, what's inside
 */

typedef struct bg_charset_converter_s bg_charset_converter_t;

/** \brief Create a charset converter
 *  \param in_charset Input character set
 *  \param out_charset Output character set
 *  \returns A newly allocated charset converte
 *
 *  in_charset and out_charset must be supported by iconv
 *  (type iconv -l for a list).
 */

bg_charset_converter_t *
bg_charset_converter_create(const char * in_charset,
                            const char * out_charset);

/** \brief Destroy a charset converter
 *  \param cnv A charset converter
 */

void bg_charset_converter_destroy(bg_charset_converter_t * cnv);

/** \brief Convert a string
 *  \param cnv A charset converter
 *  \param in_string Input string
 *  \param in_len Length of input string or -1
 *  \param out_len If non NULL, returns the length of the output string
 *  \returns A newly allocated string
 */

char * bg_convert_string(bg_charset_converter_t * cnv,
                           const char * in_string, int in_len,
                           int * out_len);

/** @} */
