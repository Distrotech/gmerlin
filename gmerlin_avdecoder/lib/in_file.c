/*****************************************************************
 
  in_file.c
 
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

#include <stdio.h>
#include <avdec_private.h>

static int open_file(bgav_input_context_t * ctx, const char * url)
  {
  FILE * f = fopen(url, "rb");
  if(!f)
    return 0;
  ctx->priv = f;

  fseek((FILE*)(ctx->priv), 0, SEEK_END);
  ctx->total_bytes = ftell((FILE*)(ctx->priv));
    
  fseek((FILE*)(ctx->priv), 0, SEEK_SET);
  
  ctx->filename = bgav_strndup(url, NULL);
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
  //  fseek((FILE*)(ctx->priv), pos, whence);
  fseek((FILE*)(ctx->priv), ctx->position, SEEK_SET);
  return ftell((FILE*)(ctx->priv));
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

