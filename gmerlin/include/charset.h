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

typedef struct bg_charset_converter_s bg_charset_converter_t;

bg_charset_converter_t *
bg_charset_converter_create(const char * in_charset,
                              const char * out_charset);

void bg_charset_converter_destroy(bg_charset_converter_t *);

char * bg_convert_string(bg_charset_converter_t *,
                           const char * in_string, int in_len,
                           int * out_len);
