/*****************************************************************

  tcp.h

  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

int
bg_tcp_connect(const char * host, int port, int milliseconds,
                 char ** error_msg);

int bg_tcp_send(int fd, uint8_t * data, int len, char ** error_msg);
  
int bg_tcp_read_data(int fd, uint8_t * ret, int len, int milliseconds);
int bg_tcp_read_line(int fd, char ** ret, int * ret_alloc, int milliseconds);


