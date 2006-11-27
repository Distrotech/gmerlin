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

/** \defgroup Subprocesses
 *  \ingroup utils
 *  \brief Subprocesses with pipable stdin, stdout and stderr
 *
 * @{
 */

/** \brief Subprocess handle
 */

typedef struct
  {
  int stdin; //!< Filedescriptor, which is a pipe to the stdin of the process  
  int stdout; //!< Filedescriptor, which is a pipe to the stdout of the process
  int stderr;//!< Filedescriptor, which is a pipe to the stderr of the process
  
  void * priv; //!< Internals made opaque
  } bg_subprocess_t;

/** \brief Create a subprocess
 *  \param command Command, will be passed to /bin/sh
 *  \param do_stdin 1 if stdin should be connected by a pipe, 0 else
 *  \param do_stdout 1 if stdout should be connected by a pipe, 0 else
 *  \param do_stderr 1 if stderr should be connected by a pipe, 0 else
 *
 *  A new handle with the runnig child program or NULL
 */

 
bg_subprocess_t * bg_subprocess_create(const char * command, int do_stdin,
                                       int do_stdout, int do_stderr);

/** \brief Send a signal to a process
 *  \param proc A subprocess
 *  \param signal Which signal to send
 *
 *  Types for signal are the same as in \<signal.h\>
 */

void bg_subprocess_kill(bg_subprocess_t* proc, int signal);

/** \brief Close a subprocess and free all associated memory
 *  \param proc A subprocess
 *  \returns The return code of the program
 */

int bg_subprocess_close(bg_subprocess_t* proc);

/** \brief Read a line from stdout or stderr of a process
 *  \param fd The filesecriptor
 *  \param ret String (will be realloced)
 *  \param ret_alloc Allocated size of the string (will be changed with each realloc)
 *  \param timeout Timeout in milliseconds
 *  \returns 1 if a line could be read, 0 else
 */

int bg_subprocess_read_line(int fd, char ** ret, int * ret_alloc,
                            int timeout);

/** \brief Read data from stdout or stderr of a process
 *  \param fd The filesecriptor
 *  \param ret Pointer to allocated memory, where the data will be placed
 *  \param len How many bytes to read
 *  \returns The number of bytes read
 *
 *  If the return value is smaller than the number of bytes you requested,
 *  you can assume, that the process finished and won't send more data.
 */

int bg_subprocess_read_data(int fd, uint8_t * ret, int len);

/** @} */
