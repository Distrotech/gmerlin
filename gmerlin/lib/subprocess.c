/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <inttypes.h>

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include <config.h>

#include <gmerlin/subprocess.h>
#include <gmerlin/log.h>
#include <gmerlin/utils.h>
#define READ  0
#define WRITE 1

#define LOG_DOMAIN "subprocess"


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

static int create_pipe(pipe_t * p)
  {
  if(pipe(p->fd) == -1)
    return 0;
  p->use = 1;
  return 1;
  }

static int connect_pipe_parent(pipe_t * p)
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

static void connect_pipe_child(pipe_t * p, int fileno)
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
  pipe_t stdin_fd;
  pipe_t stdout_fd;
  pipe_t stderr_fd;
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

  ret_priv->stdin_fd.w = 1;
  
  if(do_stdin)
    create_pipe(&ret_priv->stdin_fd);
  if(do_stdout)
    create_pipe(&ret_priv->stdout_fd);
  if(do_stderr)
    create_pipe(&ret_priv->stderr_fd);
  
  pid = fork();
  if(pid == (pid_t) 0)
    {
    /*  Child */
    connect_pipe_child(&ret_priv->stdin_fd, STDIN_FILENO);
    connect_pipe_child(&ret_priv->stdout_fd, STDOUT_FILENO);
    connect_pipe_child(&ret_priv->stderr_fd, STDERR_FILENO);

    /* Close all open filedescriptors from parent */

    open_max = sysconf(_SC_OPEN_MAX);
    for(i = 3; i < open_max; i++)
      fcntl (i, F_SETFD, FD_CLOEXEC);
    
    /* Exec */
    execl("/bin/sh", "sh", "-c", command, NULL);
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
    ret->stdin_fd  = connect_pipe_parent(&ret_priv->stdin_fd);
    ret->stdout_fd = connect_pipe_parent(&ret_priv->stdout_fd);
    ret->stderr_fd = connect_pipe_parent(&ret_priv->stderr_fd);
    ret_priv->pid = pid;
    }
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Created process: %s [%d]",
         command, pid);
  
  return ret;
  fail:
  bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Creating process failed: %s",
         strerror(errno));
  
  free(ret_priv);
  free(ret);
  return NULL;
  }


void bg_subprocess_kill(bg_subprocess_t * p, int sig)
  {
  subprocess_priv_t * priv = (subprocess_priv_t*)(p->priv);
  kill(priv->pid, sig);
  
  }

int bg_subprocess_close(bg_subprocess_t*p)
  {
  int status, ret;
  subprocess_priv_t * priv = (subprocess_priv_t*)(p->priv);

  if(priv->stdin_fd.use)
    {
    my_close(&p->stdin_fd);
    }
  /* Some programs might rely on EOF in stdin */

  //  bg_subprocess_kill(p, SIGHUP);
  
  waitpid(priv->pid, &status, 0);

  ret = WEXITSTATUS(status);
  
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Finished process [%d] return value: %d",
         priv->pid, ret);
  
  if(priv->stdout_fd.use)
    my_close(&p->stdout_fd);
  if(priv->stderr_fd.use)
    my_close(&p->stderr_fd);
  
  free(priv);
  free(p);
  return ret;
  }

int bg_system(const char * command)
  {
  bg_subprocess_t * sp = bg_subprocess_create(command, 0, 0, 0);
  if(!sp)
    return -1;
  return bg_subprocess_close(sp);
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
      return bytes_read;
      }
    }
  
  while((c != '\n') && (c != '\r'))
    {
    if(!read(fd, &c, 1))
      {
      return 0;
      }
    if((c != '\n') && (c != '\r'))
      {
      if(bytes_read + 2 > *ret_alloc)
        {
        *ret_alloc += 256;
        *ret = realloc(*ret, *ret_alloc);
        }
      (*ret)[bytes_read] = c;
      bytes_read++;
      }
    }

  (*ret)[bytes_read] = '\0';
  return 1;
  }

int bg_subprocess_read_data(int fd, uint8_t * ret, int len)
  {
  int result;
  int bytes_read = 0;

  while(bytes_read < len)
    {
    result = read(fd, ret + bytes_read, len - bytes_read);
    if(result <= 0)
      return bytes_read;
    bytes_read += result;
    }
  return bytes_read;
  }

void bg_daemonize()
  {
  pid_t pid, sid;
  /* Fork off the parent process */
  pid = fork();
  if(pid < 0)
    {
    exit(EXIT_FAILURE);
    }
  /* If we got a good PID, then
     we can exit the parent process. */
  if(pid > 0)
    {
    exit(EXIT_SUCCESS);
    }
  
  /* Change the file mode mask */
  umask(0);
                
  /* Open any logs here */        
                
  /* Create a new SID for the child process */
  sid = setsid();
  if (sid < 0)
    {
    /* Log the failure */
    exit(EXIT_FAILURE);
    }
  
  /* Change the current working directory */
  if ((chdir("/")) < 0)
    {
    /* Log the failure */
    exit(EXIT_FAILURE);
    }
  
  /* Close out the standard file descriptors */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  }
