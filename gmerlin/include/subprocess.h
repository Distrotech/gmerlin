/*****************************************************************
 
  subprocess.h
 
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

typedef struct
  {
  //  FILE * stdin;
  //  FILE * stdout;
  //  FILE * stderr;
  int stdin;
  int stdout;
  int stderr;
  
  void * priv; /* Internals made opaque */
  } bg_subprocess_t;

bg_subprocess_t * bg_subprocess_create(const char * command, int do_stdin,
                                       int do_stdout, int do_stderr);

void bg_subprocess_kill(bg_subprocess_t*, int signal);

int bg_subprocess_close(bg_subprocess_t*);

/* For reading text strings from stdout or stderr */
int bg_subprocess_read_line(int fd, char ** ret, int * ret_alloc,
                            int milliseconds);

int bg_subprocess_read_data(int fd, uint8_t * ret, int len);
