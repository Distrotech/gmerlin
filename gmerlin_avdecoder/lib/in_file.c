/*****************************************************************
 
  in_file.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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

#include <errno.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_FSEEKO
#define BGAV_FSEEK fseeko
#else
#define BGAV_FSEEK fseek
#endif

#ifdef HAVE_FTELLO
#define BGAV_FTELL ftello
#else
#define BGAV_FSEEK ftell
#endif

static int open_file(bgav_input_context_t * ctx, const char * url)
  {
  FILE * f = fopen(url, "rb");
  if(!f)
    {
    ctx->error_msg = bgav_sprintf(strerror(errno));
    return 0;
    }
  ctx->priv = f;

  BGAV_FSEEK((FILE*)(ctx->priv), 0, SEEK_END);
  ctx->total_bytes = BGAV_FTELL((FILE*)(ctx->priv));
    
  BGAV_FSEEK((FILE*)(ctx->priv), 0, SEEK_SET);

  //  fprintf(stderr, "Total bytes: %lld\n", ctx->total_bytes);
  
  ctx->filename = bgav_strdup(url);
  return 1;
  }

static int     read_file(bgav_input_context_t* ctx,
                         uint8_t * buffer, int len)
  {
  return fread(buffer, 1, len, (FILE*)(ctx->priv)); 
  }

static int64_t seek_byte_file(bgav_input_context_t * ctx,
                              int64_t pos, int whence)
  {
  //  BGAV_FSEEK((FILE*)(ctx->priv), pos, whence);
  BGAV_FSEEK((FILE*)(ctx->priv), ctx->position, SEEK_SET);
  return BGAV_FTELL((FILE*)(ctx->priv));
  }

static void    close_file(bgav_input_context_t * ctx)
  {
  if(ctx->priv)
    fclose((FILE*)(ctx->priv));
  }

bgav_input_t bgav_input_file =
  {
    name:      "file",
    open:      open_file,
    read:      read_file,
    seek_byte: seek_byte_file,
    close:     close_file
  };

