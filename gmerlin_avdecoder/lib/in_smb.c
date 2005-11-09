/*****************************************************************

  in_smb.c

  Copyright (c) 2003-2005 by Michael Gruenert - one78@web.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <avdec_private.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <pthread.h>
#include <libsmbclient.h>


typedef struct
  {
  int fd;
  int64_t bytes_read;
  } smb_priv_t;


static bgav_input_context_t * auth_ctx;

static pthread_mutex_t auth_mutex = PTHREAD_MUTEX_INITIALIZER;

static void __cleanup() __attribute__ ((destructor));

static void __cleanup()
  {
  pthread_mutex_destroy(&auth_mutex);
  }

#define FREE(ptr) if(ptr) free(ptr);

static void smb_auth_fn(const char *server, const char *share,
             char *workgroup, int wgmaxlen, char *user, int unmaxlen,
             char *pass, int pwmaxlen)
  {
  char * server_share;
  char *password= (char*)0;
  char *username= (char*)0;

  server_share = bgav_sprintf("%s/%s", server, share);
  if(!auth_ctx->opt->user_pass_callback ||
     !auth_ctx->opt->user_pass_callback(auth_ctx->opt->user_pass_callback_data,
                                        server_share, &username, &password))
    {
    free(server_share);
    return;
    }
  if(username)
    strncpy(user, username, unmaxlen - 1);
  
  if(password)
    strncpy(pass, password, pwmaxlen - 1);

  FREE(username);
  FREE(password);
  free(server_share);
  }

/* Connect and open File */

static int open_smb(bgav_input_context_t * ctx, const char * url)
  {
  smb_priv_t * p;
  uint64_t len;
  int err;

  p = calloc(1, sizeof(*p));
  ctx->priv = p;
  
  pthread_mutex_lock(&auth_mutex);
  auth_ctx = ctx;
  err = smbc_init(smb_auth_fn, 0);
  
  if (err < 0)
    {
    ctx->error_msg = bgav_sprintf("Initialization of samba failed(error: %d)", err);
    pthread_mutex_unlock(&auth_mutex);
    goto fail;
    }

  p->fd = smbc_open((char*)url, O_RDONLY, 0644);
  if (p->fd < 0)
    { 
    ctx->error_msg = bgav_sprintf("Open file failed (error: %d)", p->fd);
    pthread_mutex_unlock(&auth_mutex);
    goto fail;
    }
  pthread_mutex_unlock(&auth_mutex);
    
  len = smbc_lseek(p->fd,0,SEEK_END);
  smbc_lseek (p->fd, 0, SEEK_SET);
 
  if (len <= 0)
    {
    ctx->error_msg = bgav_sprintf("Can't get filesize");
    goto fail;
    }
  
  ctx->total_bytes = len;

  return 1;
  
  fail:
  return 0;
  }

/* File read */

static int read_smb(bgav_input_context_t* ctx,
                    uint8_t * buffer, int len)
  {
  int len_read, err;
  smb_priv_t * p;
  p = (smb_priv_t*)(ctx->priv);
  
  if(len + p->bytes_read > ctx->total_bytes)
    len = ctx->total_bytes - p->bytes_read;
  
  if(!len)
    {
    return 0;
    }
  err = smbc_read(p->fd, buffer, len);

  if(err <= 0)
    len_read = 0;
  else
    len_read = len;
  
  p->bytes_read += len_read;
  
  return len_read;
  }

/* File seek */

static int64_t seek_byte_smb(bgav_input_context_t * ctx,
                              int64_t pos, int whence)
  {
  smb_priv_t * p;
  p = (smb_priv_t*)(ctx->priv);
  int64_t len;

  len = smbc_lseek(p->fd, pos, SEEK_SET);
  
  return len;
  }


/* close the smb connection */

static void    close_smb(bgav_input_context_t * ctx)
  {
  smb_priv_t * p;
  p = (smb_priv_t*)(ctx->priv);
  smbc_close(p->fd);
  free(p);
  }


bgav_input_t bgav_input_smb =
  {
    name:      "samba",
    open:      open_smb,
    read:      read_smb,
    seek_byte: seek_byte_smb,
    close:     close_smb
  };

