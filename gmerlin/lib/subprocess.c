/*****************************************************************
 
  subprocess.c
 
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include <subprocess.h>
#include <log.h>

#define READ  0
#define WRITE 1

#define LOG_DOMAIN "subprocess"

#if 0
#define CLOSE(f) \
  if(close(f)) \
    perror(# f); \
  else           \
    fprintf(stderr, "close %s (%d)\n", # f, f)
#endif

static int my_close(int * fd)
  {
  int result;
  if(*fd >= 0)
    {
    result = close(*fd);
    *fd = -1;
    }
  return result;
  }

typedef struct
  {
  int fd[2]; /* fd */
  int use;
  int w; /*  1 if parent writes */
  } pipe_t;

int create_pipe(pipe_t * p)
  {
  if(pipe(p->fd) == -1)
    return 0;
  p->use = 1;
  return 1;
  }

int connect_pipe_parent(pipe_t * p)
  {
  if(!p->use)
    return -1;

  /* Close unused end */
  my_close(p->w ? &p->fd[READ] : &p->fd[WRITE]);

  if(p->w)
    return p->fd[WRITE];
  else
    return p->fd[READ];
  }

void connect_pipe_child(pipe_t * p, int fileno)
  {
  if(!p->use)
    return;

  /* Close unused end */
  my_close(p->w ? &p->fd[WRITE] : &p->fd[READ]);

  close(fileno);
  
  if(p->w)
    {
    dup2(p->fd[READ], fileno);
    my_close(&p->fd[READ]);
    }
  else
    {
    dup2(p->fd[WRITE], fileno);
    my_close(&p->fd[WRITE]);
    }
  }

typedef struct
  {
  pid_t pid;
  pipe_t stdin;
  pipe_t stdout;
  pipe_t stderr;
  } subprocess_priv_t;

bg_subprocess_t * bg_subprocess_create(const char * command, int do_stdin,
                                       int do_stdout, int do_stderr)
  {
  bg_subprocess_t * ret;
  subprocess_priv_t * ret_priv;
  pid_t pid;
  int open_max, i;
  
  ret = calloc(1, sizeof(*ret));
  ret_priv = calloc(1, sizeof(*ret_priv));
  ret->priv = ret_priv;

  ret_priv->stdin.w = 1;
  
  if(do_stdin)
    create_pipe(&ret_priv->stdin);
  if(do_stdout)
    create_pipe(&ret_priv->stdout);
  if(do_stderr)
    create_pipe(&ret_priv->stderr);
  
  pid = fork();
  if(pid == (pid_t) 0)
    {
    /*  Child */
    connect_pipe_child(&ret_priv->stdin, STDIN_FILENO);
    connect_pipe_child(&ret_priv->stdout, STDOUT_FILENO);
    connect_pipe_child(&ret_priv->stderr, STDERR_FILENO);

    /* Close all open filedescriptors from parent */

    open_max = sysconf(_SC_OPEN_MAX);
    for(i = 3; i < open_max; i++)
      fcntl (i, F_SETFD, FD_CLOEXEC);
    
    /* Exec */
    execl("/bin/sh", "sh", "-c", command, (char*)0);
    /* Never get here */
    _exit(1);
    }
  else if(pid < 0)
    {
    goto fail;
    }
  else
    {
    /*  Parent */
    ret->stdin  = connect_pipe_parent(&ret_priv->stdin);
    ret->stdout = connect_pipe_parent(&ret_priv->stdout);
    ret->stderr = connect_pipe_parent(&ret_priv->stderr);
    ret_priv->pid = pid;
    }
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Created process: %s [%d]",
         command, pid);
  
  return ret;
  fail:
  bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Creating process failed: %s",
         strerror(errno));
  
  free(ret_priv);
  free(ret);
  return (bg_subprocess_t*)0;
  }


void bg_subprocess_kill(bg_subprocess_t * p, int sig)
  {
  subprocess_priv_t * priv = (subprocess_priv_t*)(p->priv);
  kill(priv->pid, sig);
  
  }

int bg_subprocess_close(bg_subprocess_t*p)
  {
  int status;
  subprocess_priv_t * priv = (subprocess_priv_t*)(p->priv);

  if(priv->stdin.use)
    {
    //    fflush(p->stdin);
    my_close(&p->stdin);
    }
  /* Some programs might rely on EOF in stdin */


  //  bg_subprocess_kill(p, SIGHUP);
  
  fprintf(stderr, "waitpid...");
  waitpid(priv->pid, &status, 0);
  fprintf(stderr, "done\n");

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Finished process [%d]",
         priv->pid);

  
  if(priv->stdout.use)
    my_close(&p->stdout);
  if(priv->stderr.use)
    my_close(&p->stderr);
  
  free(priv);
  free(p);
  return WEXITSTATUS(status);
  }

/* Read line without trailing '\r' or '\n' */
int bg_subprocess_read_line(int fd, char ** ret, int * ret_alloc,
                            int milliseconds)
  {
  int bytes_read = 0;
  char c = 0;
  fd_set rset;
  struct timeval timeout;
  int result;
  if(milliseconds >= 0)
    { 
    FD_ZERO (&rset);
    FD_SET  (fd, &rset);
    
    timeout.tv_sec  = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;
    
    if((result = select (fd+1, &rset, NULL, NULL, &timeout)) <= 0)
      {
      //        fprintf(stderr, "select returned %d\n", result);
      return bytes_read;
      }
    //      fprintf(stderr, "select returned %d\n", result);
    }
  
  while((c != '\n') && (c != '\r'))
    {
    if(!read(fd, &c, 1))
      return 0;
    
    if((c != '\n') && (c != '\r'))
      {
      if(bytes_read + 1 > *ret_alloc)
        {
        *ret_alloc += 256;
        *ret = realloc(*ret, *ret_alloc);
        //        fprintf(stderr, "Ret: %p, ret_alloc: %d\n", *ret, *ret_alloc);
        }
      (*ret)[bytes_read] = c;
      bytes_read++;
      }
    }

  if(bytes_read + 1 > *ret_alloc)
    {
    *ret_alloc += 256;
    *ret = realloc(*ret, *ret_alloc);
    //    fprintf(stderr, "Ret: %p, ret_alloc: %d\n", *ret, *ret_alloc);
    }
  (*ret)[bytes_read] = '\0';
  return 1;
  }
