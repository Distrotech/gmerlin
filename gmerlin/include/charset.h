/*****************************************************************
 
  charset.h
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

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
