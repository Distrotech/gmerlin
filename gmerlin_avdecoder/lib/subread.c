/*****************************************************************
 
  subread.c
 
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
#include <stdlib.h>

#include <avdec_private.h>

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

static int read_line_utf16(bgav_input_context_t * ctx,
                           int (*read_char)(bgav_input_context_t*,uint16_t*),
                           char ** _ret, int * ret_len,
                           int * ret_alloc)
  {
  uint16_t * ret;
  uint16_t c;
  int eof = 0;
  *ret_len = 0;

  ret = (uint16_t*)(*_ret);
  
  while(1)
    {
    if(!read_char(ctx, &c))
      {
      eof = 1;
      break;
      }
    if(c == 0x000a) /* \n' */
      break;
    else if((c != 0x000d) && (c != 0xfeff)) /* Skip '\r' and endian marker */
      {
      // fprintf(stderr, "Got 0x%04x\n", c);
      if(*ret_alloc <= (*ret_len)*2)
        {
        *ret_alloc += 256;
        //        fprintf(stderr, "Realloc 1...");
        ret = realloc(ret, *ret_alloc);
        //        fprintf(stderr, "done\n");
        }
      ret[*ret_len] = c;
      (*ret_len)++;
      }
    }

  /* Len is in bytes */
  *ret_len *= 2;

  *_ret = (char*)ret;
  
  if(eof)
    return !!(*ret_len);
  
  return 1;
  }

typedef struct
  {
  int (*read_char_16)(bgav_input_context_t * ctx, uint16_t*);

  char * buf_16;
  char * buf_8;
  int buf_16_alloc;
  int buf_8_alloc;

  uint8_t nl_16[2];
  
  } srt_priv_t;

static int init_srt(bgav_stream_t * s)
  {
  srt_priv_t * priv;
  uint8_t first_bytes[2];
  bgav_subtitle_reader_context_t * ctx;

  ctx = s->data.subtitle.subreader;

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  /* Check for utf-16 */
  if(bgav_input_get_data(ctx->input, first_bytes, 2) < 2)
    return 0;

  //  fprintf(stderr, "First bytes: ");
  //  bgav_hexdump(first_bytes, 2, 2);
  
  if((first_bytes[0] == 0xff) && (first_bytes[1] == 0xfe))
    {
    priv->read_char_16 = bgav_input_read_16_le;
    s->data.subtitle.encoding = bgav_strdup("UTF-16LE");
    priv->nl_16[0] = 0x0d;
    priv->nl_16[1] = 0x00;
    //    fprintf(stderr, "Detected UTF-16LE format\n");
    
    }
  else if((first_bytes[0] == 0xfe) && (first_bytes[1] == 0xff))
    {
    priv->read_char_16 = bgav_input_read_16_be;
    s->data.subtitle.encoding = bgav_strdup("UTF-16BE");
    priv->nl_16[0] = 0x00;
    priv->nl_16[1] = 0x0d;
    
    //    fprintf(stderr, "Detected UTF-16BE format\n");
    }
  return 1;
  }

static int read_line_srt(bgav_stream_t * s)
  {
  int len, out_len;
  srt_priv_t * priv;
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;
  priv = (srt_priv_t *)(ctx->priv);

  if(priv->read_char_16)
    {
    if(!read_line_utf16(ctx->input, priv->read_char_16,
                        &(priv->buf_16), &len,
                        &(priv->buf_16_alloc)))
      return 0;
    //    fprintf(stderr, "Read %p (%d bytes)\n", priv->buf_16, len);
    //    fprintf(stderr, "convert_string_realloc 1...");
    bgav_convert_string_realloc(s->data.subtitle.cnv,
                                priv->buf_16, len,
                                &out_len,
                                &(priv->buf_8), &(priv->buf_8_alloc));
    //    fprintf(stderr, "done\n");
    return 1;
    }
  else
    {
    return bgav_input_read_line(ctx->input, &(priv->buf_8), &(priv->buf_8_alloc), 0);
    }
  }

static int read_subtitle_srt(bgav_stream_t * s)
  {
  int lines_read, line_len;
  int a1,a2,a3,a4,b1,b2,b3,b4;
  int i,len;
  srt_priv_t * priv;
  bgav_subtitle_reader_context_t * ctx;

  gavl_time_t start, end;

  ctx = s->data.subtitle.subreader;
  priv = (srt_priv_t *)(ctx->priv);

  /* Read lines */
  while(1)
    {
    if(!read_line_srt(s))
      return 0;

    //    fprintf(stderr, "Got line: %s\n", priv->buf_8);
    if ((len=sscanf (priv->buf_8, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",
                     &a1,&a2,&a3,(char *)&i,&a4,&b1,&b2,&b3,(char *)&i,&b4)) == 10)
      {
      break;
      }
    }

  start  = a1;
  start *= 60;
  start += a2;
  start *= 60;
  start += a3;
  start *= GAVL_TIME_SCALE;
  start += a4 * (GAVL_TIME_SCALE/1000);

  end  = b1;
  end *= 60;
  end += b2;
  end *= 60;
  end += b3;
  end *= GAVL_TIME_SCALE;
  end += b4 * (GAVL_TIME_SCALE/1000);

  ctx->p->timestamp_scaled = start;
  ctx->p->duration_scaled = end - start;

  ctx->p->data_size = 0;
  
  lines_read = 0;
  /* Read lines until we are done */
  if(priv->read_char_16)
    {
    while(1)
      {
      if(!read_line_utf16(ctx->input, priv->read_char_16,
                          &(priv->buf_16), &line_len,
                          &(priv->buf_16_alloc)))
        line_len = 0;

      if(!line_len)
        {
        /* Zero terminate */
        if(lines_read)
          {
          ctx->p->data[ctx->p->data_size]   = 0x00;
          ctx->p->data[ctx->p->data_size+1] = 0x00;
          ctx->p->data_size+=2;
          }
        return !!lines_read;
        }

      if(lines_read)
        {
        ctx->p->data[ctx->p->data_size]   = priv->nl_16[0];
        ctx->p->data[ctx->p->data_size+1] = priv->nl_16[1];
        ctx->p->data_size+=2;
        }
      lines_read++;
      bgav_packet_alloc(ctx->p, ctx->p->data_size + line_len + 4);
      memcpy(ctx->p->data + ctx->p->data_size, priv->buf_16, line_len);
      ctx->p->data_size += line_len;
      //      fprintf(stderr, "Line: ");
      //      bgav_hexdump(priv->buf_16, line_len, 16);
      }
    }
  else
    {
    while(1)
      {
      if(!bgav_input_read_line(ctx->input, &(priv->buf_8),
                               &(priv->buf_8_alloc), 0))
        line_len = 0;
      else
        line_len = strlen(priv->buf_8);
      
      if(!line_len)
        {
        /* Zero terminate */
        if(lines_read)
          {
          ctx->p->data[ctx->p->data_size] = '\0';
          ctx->p->data_size++;
          }
        return 1;
        }
      if(lines_read)
        {
        ctx->p->data[ctx->p->data_size] = '\n';
        ctx->p->data_size++;
        }
      lines_read++;
      bgav_packet_alloc(ctx->p, ctx->p->data_size + line_len + 2);
      memcpy(ctx->p->data + ctx->p->data_size, priv->buf_8, line_len);
      ctx->p->data_size += line_len;
      }
    }
  
  return 0;
  }

static void close_srt(bgav_stream_t * s)
  {
  srt_priv_t * priv;
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;
  priv = (srt_priv_t *)(ctx->priv);

  if(priv->buf_8) free(priv->buf_8);
  if(priv->buf_16) free(priv->buf_16);
  free(priv);
  
  }

static bgav_subtitle_reader_t subtitle_readers[] =
  {
    {
      name: "Subrip (srt)",
      extensions: "srt",
      read_subtitle_text: read_subtitle_srt,
      init:               init_srt,
      close:              close_srt,
    },
    { /* End of subtitle readers */ }
    
  };

void bgav_subreaders_dump()
  {
  int i = 0;
  fprintf(stderr, "<h2>Subtitle readers</h2>\n<ul>\n");
  while(subtitle_readers[i].name)
    {
    fprintf(stderr, "<li>%s\n", subtitle_readers[i].name);
    i++;
    }
  fprintf(stderr, "</ul>\n");
  }


static bgav_subtitle_reader_t * find_subtitle_reader(const char * extension)
  {
  char ** extensions;
  int i = 0, j;
  while(subtitle_readers[i].name)
    {
    extensions = bgav_stringbreak(subtitle_readers[i].extensions, ' ');
    j = 0;
    while(extensions[j])
      {
      if(!strcasecmp(extension, extensions[j]))
        {
        bgav_stringbreak_free(extensions);
        return &subtitle_readers[i];
        }
      j++;
      }
    bgav_stringbreak_free(extensions);
    i++;
    }
  return (bgav_subtitle_reader_t*)0;
  }

extern bgav_input_t bgav_input_file;

bgav_subtitle_reader_context_t *
bgav_subtitle_reader_open(bgav_input_context_t * input_ctx)
  {
  char * file, *directory, *pos, *extension, *name;
  int file_len;
  DIR * dir;
  struct dirent * res;
  bgav_subtitle_reader_t * r;

  bgav_subtitle_reader_context_t * ret = (bgav_subtitle_reader_context_t *)0;
  bgav_subtitle_reader_context_t * end, *new;
  
  union
    {
    struct dirent d;
    char b[sizeof (struct dirent) + NAME_MAX];
    } u;
  
  /* Check if input is a regular file */
  if((input_ctx->input != &bgav_input_file) || !input_ctx->filename)
    {
    fprintf(stderr, "No subtitle searching, no regular file\n");
    return (bgav_subtitle_reader_context_t*)0;
    }

  /* Get directory name and open directory */
  directory = bgav_strdup(input_ctx->filename);
  pos = strrchr(directory, '/');
  if(!pos)
    {
    free(directory);
    fprintf(stderr, "No subtitle searching, Invalid filename\n");
    return (bgav_subtitle_reader_context_t*)0;
    }
  *pos = '\0';

  file = pos + 1;
  /* Cut off extension */
  pos = strrchr(file, '.');
  if(pos)
    file_len = pos - file;
  else
    file_len = strlen(file);
  dir = opendir(directory);
  if(!dir)
    {
    fprintf(stderr, "No subtitle searching, failed to open directory\n");
    return (bgav_subtitle_reader_context_t*)0;
    }

  while(!readdir_r(dir, &u.d, &res))
    {
    if(!res)
      break;
    
    //    fprintf(stderr, "Trying %s\n", u.d.d_name);
    /* Check, if the filenames match */
    if(strncasecmp(u.d.d_name, file, file_len) ||
       !strcmp(u.d.d_name, file))
      continue;
      
    /* Check if there is an extension for this */
    pos = strrchr(u.d.d_name, '.');
    if(pos)
      extension = pos+1;
    else
      continue;
    r = find_subtitle_reader(extension);
    if(!r)
      continue;
    
    new = calloc(1, sizeof(*new));
    new->filename = bgav_sprintf("%s/%s", directory, u.d.d_name);
    new->input    = bgav_input_create(input_ctx->opt);
    new->reader   = r;
    new->p = bgav_packet_create();
    
    name = u.d.d_name + file_len;

    
    while(!isalnum(*name) && (*name != '\0'))
      name++;

    if(*name != '\0')
      {
      pos = strrchr(name, '.');
      new->info = bgav_strndup(name, pos);
      }

    fprintf(stderr, "Found subtitle file %s (reader: %s, info: %s)\n",
            new->filename, new->reader->name, new->info);
    
    if(!ret)
      {
      ret = new;
      end = ret;
      }
    else
      {
      end->next = new;
      end = end->next;
      }
    }
  closedir(dir);
  free(directory);
  return ret;
  }

void bgav_subtitle_reader_close(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;

  if(ctx->reader->close)
    ctx->reader->close(s);
  if(ctx->info) free(ctx->info);
  if(ctx->filename) free(ctx->filename);
  if(ctx->p) bgav_packet_destroy(ctx->p);
    
  if(ctx->input)
    bgav_input_destroy(ctx->input);
  free(ctx);
  }

bgav_packet_t * bgav_subtitle_reader_read_text(bgav_stream_t * s)
  {
  bgav_subtitle_reader_t * r;
  r = s->data.subtitle.subreader->reader;
  
  if(r->read_subtitle_text(s))
    return s->data.subtitle.subreader->p;
  else
    return (bgav_packet_t *)0;
  }

int bgav_subtitle_reader_start(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;
  if(!bgav_input_open(ctx->input, ctx->filename))
    return 0;

  if(ctx->reader->init && !ctx->reader->init(s))
    return 0;
  
  return 1;
  }

void bgav_subtitle_reader_seek(bgav_stream_t * s,
                               gavl_time_t time)
  {
  
  }
