/*****************************************************************
 
  utils.h
 
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

#ifndef __BG_UTILS_H_
#define __BG_UTILS_H_

#include <gavl/gavl.h>

/* Append a trailing '/' if it's missing. Argument must be free()able */

char * bg_fix_path(char * path);

/*
 *  Searchpath support
 */

char * bg_search_file_read(const char * directory, const char * file);

char * bg_search_file_write(const char * directory, const char * file);

/* 
 *  Returns TRUE if an executeable is found anywhere in $PATH 
 *  If path is non NULL, it will contain the path to the executable,
 *  which must be freed after
 */

int bg_search_file_exec(const char * file, char ** path);

/* 
 *  String utilities
 */

char * bg_strdup(char * old_string, const char * new_string);

char * bg_strndup(char * old_string,
                  const char * new_start,
                  const char * new_end);

char * bg_strcat(char * old_string, const char * tail);

char * bg_strncat(char * old_string, const char * start, const char * end);

int bg_string_is_url(const char * str);

/* Like sprintf, but allocates memory (free with free())*/

char * bg_sprintf(const char * format,...);

/*
 * Break a string into a NULL terminated array of
 * substrings separated by delimiter. The delimiters
 * are removed from the returned array
 */

char ** bg_strbreak(const char * str, char delim);
void bg_strbreak_free(char ** retval);

/*
 * Scramble/descramble string. These are taken from gftp
 * and just prevent reading passowrds using cat.
 */

char * bg_scramble_string(const char * str);
char * bg_descramble_string(const char * str);

/* Convert uri to binary string and vice versa */

char * bg_string_to_uri(const char * pos1, int len);
char * bg_uri_to_string(const char * pos1, int len);


/*
 *  This one decodes a string of MIME type text/urilist into
 *  a gmerlin usable array of location strings.
 *  The array is NULL terminated, everything must be freed by the
 *  caller
 */



char ** bg_urilist_decode(const char * str, int len);

void bg_urilist_free(char ** uri_list);



/*
 *  Create a "system charset" (as returned by nl_langinfo())
 *  to utf8 and back
 *
 *  Used mostly for filenames
 */

char * bg_system_to_utf8(const char * str, int len);
char * bg_utf8_to_system(const char * str, int len);

/* This is mostly for debugging */

void bg_hexdump(const void * data, int len);

/*
 *  Convert formats to strings, free return value with
 *  free()
 *  Implemented in formats.c
 */

char * bg_audio_format_to_string(gavl_audio_format_t * format, int use_tabs);
char * bg_video_format_to_string(gavl_video_format_t * format, int use_tabs);

/*
 *  Read a single line from a filedescriptor
 *
 *  ret will be reallocated if neccesary and ret_alloc will
 *  be updated then
 *
 *  The string will be 0 terminated, a trailing \r or \n will
 *  be removed
 */

int bg_read_line_fd(int fd, char ** ret, int * ret_alloc);

/*
 *  Create a unique filename, create an empty file
 *  Template MUST contain the printf forma %08x
 */

char * bg_create_unique_filename(char * format);

/* 
 *  Macro, which calls strcmp, but casts the first argument to char*
 *  This is needed because libxml strings are uint8_t*
 */

#define BG_XML_STRCMP(a, b) strcmp((char*)a, b)
#define BG_XML_GET_PROP(a, b) (char*)xmlGetProp(a, (xmlChar*)b)
#define BG_XML_SET_PROP(a, b, c) xmlSetProp(a, (xmlChar*)b, (xmlChar*)c)
#define BG_XML_NEW_TEXT(a) xmlNewText((xmlChar*)a)

/* iso-639 3cc -> Language name */

const char * bg_get_language_name(const char * iso);


#endif // __BG_UTILS_H_
