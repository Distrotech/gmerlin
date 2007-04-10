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
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>


#include <avdec_private.h>
#include <yml.h>

/* SRT format */

static int probe_srt(char * line)
  {
  int a1,a2,a3,a4,b1,b2,b3,b4,i;
  if(sscanf(line, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",
            &a1,&a2,&a3,(char *)&i,&a4,&b1,&b2,&b3,(char *)&i,&b4) == 10)
    return 1;
  return 0;
  }

static int read_srt(bgav_stream_t * s)
  {
  int lines_read, line_len;
  int a1,a2,a3,a4,b1,b2,b3,b4;
  int i,len;
  bgav_subtitle_reader_context_t * ctx;

  gavl_time_t start, end;

  ctx = s->data.subtitle.subreader;

  /* Read lines */
  while(1)
    {
    if(!bgav_input_read_convert_line(ctx->input, &ctx->line, &ctx->line_alloc,
                                     &line_len))
      return 0;
    
    if ((len=sscanf (ctx->line, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",
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

  ctx->p->pts = start;
  ctx->p->duration = end - start;

  ctx->p->data_size = 0;
  
  /* Read lines until we are done */

  lines_read = 0;
  while(1)
    {
    if(!bgav_input_read_convert_line(ctx->input, &ctx->line, &ctx->line_alloc,
                                     &line_len))
      {
      line_len = 0;
      if(!lines_read)
        return 0;
      }
    
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
    memcpy(ctx->p->data + ctx->p->data_size, ctx->line, line_len);
    ctx->p->data_size += line_len;
    }
  
  return 0;
  }

/* MPSub */

typedef struct
  {
  int frame_based;
  int64_t frame_duration;

  gavl_time_t last_end_time;
  } mpsub_priv_t;

static int probe_mpsub(char * line)
  {
  float f;
  while(isspace(*line) && (*line != '\0'))
    line++;
  
  if(!strncmp(line, "FORMAT=TIME", 11) || (sscanf(line, "FORMAT=%f", &f) == 1))
    return 1;
  return 0;
  }

static int init_mpsub(bgav_stream_t * s)
  {
  int line_len;
  double framerate;
  char * ptr;
  bgav_subtitle_reader_context_t * ctx;
  mpsub_priv_t * priv = calloc(1, sizeof(*priv));
  ctx = s->data.subtitle.subreader;
  ctx->priv = priv;

  while(1)
    {
    if(!bgav_input_read_line(ctx->input, &ctx->line, &ctx->line_alloc, 0, &line_len))
      return 0;

    ptr = ctx->line;
    
    while(isspace(*ptr) && (*ptr != '\0'))
      ptr++;

    if(!strncmp(ptr, "FORMAT=TIME", 11))
      return 1;
    else if(sscanf(ptr, "FORMAT=%lf", &framerate))
      {
      priv->frame_duration = gavl_seconds_to_time(1.0 / framerate);
      priv->frame_based = 1;
      return 1;
      }
    }
  return 0;
  }

static int read_mpsub(bgav_stream_t * s)
  {
  int i1, i2;
  double d1, d2;
  gavl_time_t t1 = 0, t2 = 0;
  
  int line_len, lines_read;
  bgav_subtitle_reader_context_t * ctx;
  mpsub_priv_t * priv;
  char * ptr;
  
  ctx = s->data.subtitle.subreader;
  priv = (mpsub_priv_t*)(ctx->priv);
    
  while(1)
    {
    if(!bgav_input_read_line(ctx->input, &ctx->line, &ctx->line_alloc, 0, &line_len))
      return 0;

    ptr = ctx->line;
    
    while(isspace(*ptr) && (*ptr != '\0'))
      ptr++;

    /* The following will reset last_end_time whenever we cross a "FORMAT=" line */
    if(!strncmp(ptr, "FORMAT=", 7))
      {
      priv->last_end_time = 0;
      continue;
      }
    
    if(priv->frame_based)
      {
      if(sscanf(ptr, "%d %d\n", &i1, &i2) == 2)
        {
        t1 = i1 * priv->frame_duration;
        t2 = i2 * priv->frame_duration;
        break;
        }
      }
    else if(sscanf(ptr, "%lf %lf\n", &d1, &d2) == 2)
      {
      t1 = gavl_seconds_to_time(d1);
      t2 = gavl_seconds_to_time(d2);
      break;
      }
    }

  /* Set times */

  ctx->p->pts = priv->last_end_time + t1;
  ctx->p->duration  = t2;
  
  priv->last_end_time = ctx->p->pts + ctx->p->duration;
  
  /* Read the actual stuff */
  ctx->p->data_size = 0;
  
  /* Read lines until we are done */

  lines_read = 0;

  while(1)
    {
    if(!bgav_input_read_convert_line(ctx->input, &ctx->line, &ctx->line_alloc,
                                     &line_len))
      {
      line_len = 0;
      if(!lines_read)
        return 0;
      }
    
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
    memcpy(ctx->p->data + ctx->p->data_size, ctx->line, line_len);
    ctx->p->data_size += line_len;
    }
  return 0;
  }

static void close_mpsub(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  mpsub_priv_t * priv;
  
  ctx = s->data.subtitle.subreader;
  priv = (mpsub_priv_t *)(ctx->priv);
  free(priv);
  }

/* Spumux */

#ifdef HAVE_LIBPNG

#include <pngreader.h>

typedef struct
  {
  bgav_yml_node_t * yml;
  bgav_yml_node_t * cur;
  bgav_png_reader_t * reader;
  gavl_video_format_t format;
  int have_header;
  int need_header;
  
  int buffer_alloc;
  uint8_t * buffer;
  } spumux_t;

static int probe_spumux(char * line)
  {
  if(!strncasecmp(line, "<subpictures>", 13))
    return 1;
  return 0;
  }

static int init_current_spumux(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  spumux_t * priv;
  ctx = s->data.subtitle.subreader;
  priv = (spumux_t*)(ctx->priv);
  
  priv->cur = priv->yml->children;
  while(priv->cur && (!priv->cur->name || strcasecmp(priv->cur->name, "stream")))
    {
    priv->cur = priv->cur->next;
    }
  if(!priv->cur)
    return 0;
  priv->cur = priv->cur->children;
  while(priv->cur && (!priv->cur->name || strcasecmp(priv->cur->name, "spu")))
    {
    priv->cur = priv->cur->next;
    }
  if(!priv->cur)
    return 0;
  return 1;
  }

static int advance_current_spumux(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  spumux_t * priv;
  ctx = s->data.subtitle.subreader;
  priv = (spumux_t*)(ctx->priv);

  priv->cur = priv->cur->next;
  while(priv->cur && (!priv->cur->name || strcasecmp(priv->cur->name, "spu")))
    {
    priv->cur = priv->cur->next;
    }
  if(!priv->cur)
    return 0;
  return 1;
  }

static gavl_time_t parse_time_spumux(const char * str, int timescale, int frame_duration)
  {
  int h, m, s, f;
  gavl_time_t ret;
  if(sscanf(str, "%d:%d:%d.%d", &h, &m, &s, &f) < 4)
    return GAVL_TIME_UNDEFINED;
  ret = h;
  ret *= 60;
  ret += m;
  ret *= 60;
  ret += s;
  ret *= GAVL_TIME_SCALE;
  ret += gavl_frames_to_time(timescale, frame_duration, f);
  return ret;
  }

#define LOG_DOMAIN "spumux"

static int read_spumux(bgav_stream_t * s)
  {
  int size;
  const char * filename;
  const char * start_time;
  const char * tmp;
  char * error_msg = (char*)0;
  
  bgav_subtitle_reader_context_t * ctx;
  spumux_t * priv;
  ctx = s->data.subtitle.subreader;
  priv = (spumux_t*)(ctx->priv);

  if(!priv->cur)
    return 0;

  //  bgav_yml_dump(priv->cur);

  start_time = bgav_yml_get_attribute_i(priv->cur, "start");
  if(!start_time)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "yml node has no start attribute");
    return 0;
    }
  
  if(!priv->have_header)
    {
    filename = bgav_yml_get_attribute_i(priv->cur, "image");
    if(!filename)
      {
      bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "yml node has no filename attribute");
      return 0;
      }
    if(!bgav_slurp_file(filename, &priv->buffer, &priv->buffer_alloc,
                        &size, s->opt))
      {
      bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Reading file %s failed", filename);
      return 0;
      }
    if(!bgav_png_reader_read_header(priv->reader, priv->buffer, size, &priv->format, &error_msg))
      {
      if(error_msg)
        {
        bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Reading png header failed: %s", error_msg);
        free(error_msg);
        }
      else
        bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Reading png header failed");
        
      return 0;
      }
    if(priv->need_header)
      {
      priv->have_header = 1;
      return 1;
      }
    }
  
  /* Read the image */
  if((priv->format.image_width > s->data.subtitle.format.image_width) ||
     (priv->format.image_width > s->data.subtitle.format.image_height))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Overlay too large");
    return 0;
    }

  if(!bgav_png_reader_read_image(priv->reader, ctx->ovl.frame))
    return 0;
  
  ctx->ovl.frame->time_scaled =
    parse_time_spumux(start_time, s->data.subtitle.format.timescale,
                      s->data.subtitle.format.frame_duration);
  
  if(ctx->ovl.frame->time_scaled == GAVL_TIME_UNDEFINED)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Parsing time string %s failed", start_time);
    return 0;
    }
  tmp = bgav_yml_get_attribute_i(priv->cur, "end");
  if(tmp)
    {
    ctx->ovl.frame->duration_scaled =
      parse_time_spumux(tmp,
                        s->data.subtitle.format.timescale,
                        s->data.subtitle.format.frame_duration);
    if(ctx->ovl.frame->duration_scaled == GAVL_TIME_UNDEFINED)
      return 0;
    ctx->ovl.frame->duration_scaled -= ctx->ovl.frame->time_scaled;
    }
  else
    {
    ctx->ovl.frame->duration_scaled = -1;
    }

  tmp = bgav_yml_get_attribute_i(priv->cur, "xoffset");
  if(tmp)
    ctx->ovl.dst_x = atoi(tmp);
  else
    ctx->ovl.dst_x = 0;

  tmp = bgav_yml_get_attribute_i(priv->cur, "yoffset");
  if(tmp)
    ctx->ovl.dst_y = atoi(tmp);
  else
    ctx->ovl.dst_y = 0;
  
  ctx->ovl.ovl_rect.x = 0;
  ctx->ovl.ovl_rect.y = 0;

  ctx->ovl.ovl_rect.w = priv->format.image_width;
  ctx->ovl.ovl_rect.h = priv->format.image_height;
  
  priv->have_header = 0;
  advance_current_spumux(s);
  
  return 1;
  }

static int init_spumux(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  spumux_t * priv;
  ctx = s->data.subtitle.subreader;

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
    
  priv->yml = bgav_yml_parse(ctx->input);
  if(!priv->yml)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Parsing spumux file failed");
    return 0;
    }
  if(!priv->yml->name || strcasecmp(priv->yml->name, "subpictures"))
    return 0;
  
  if(!init_current_spumux(s))
    return 0;

  priv->reader = bgav_png_reader_create(0);

  gavl_video_format_copy(&s->data.subtitle.format,
                         &s->data.subtitle.video_stream->data.video.format);

  priv->need_header = 1;
  if(!read_spumux(s))
    return 0;
  priv->need_header = 0;
  
  s->data.subtitle.format.pixelformat = priv->format.pixelformat;
  
  return 1;
  }


static void seek_spumux(bgav_stream_t * s, gavl_time_t time)
  {
  const char * start_time, * end_time;
  bgav_subtitle_reader_context_t * ctx;
  spumux_t * priv;
  gavl_time_t start, end = 0;
    
  ctx = s->data.subtitle.subreader;
  priv = (spumux_t*)(ctx->priv);
  init_current_spumux(s);
  
  while(1)
    {
    start_time = bgav_yml_get_attribute_i(priv->cur, "start");
    if(!start_time)
      {
      bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "yml node has no start attribute");
      return;
      }
    end_time = bgav_yml_get_attribute_i(priv->cur, "end");

    start = parse_time_spumux(start_time,
                              s->data.subtitle.format.timescale,
                              s->data.subtitle.format.frame_duration);
    if(start == GAVL_TIME_UNDEFINED)
      {
      bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Error parsing start attribute");
      return;
      }
    
    if(end_time)
      end = parse_time_spumux(end_time,
                              s->data.subtitle.format.timescale,
                              s->data.subtitle.format.frame_duration);
    
    if(end == GAVL_TIME_UNDEFINED)
      {
      if(end > time)
        break;
      }
    else
      {
      if(start > time)
        break;
      }
    advance_current_spumux(s);
    }
  priv->have_header = 0;
  bgav_png_reader_reset(priv->reader);
  }

static void close_spumux(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  spumux_t * priv;
  ctx = s->data.subtitle.subreader;
  priv = (spumux_t*)(ctx->priv);

  if(priv->yml)
    bgav_yml_free(priv->yml);
  if(priv->buffer)
    free(priv->buffer);
  
  if(priv->reader)
    bgav_png_reader_destroy(priv->reader);
  free(priv);
  
  }

#undef LOG_DOMAIN

#endif // HAVE_LIBPNG


static bgav_subtitle_reader_t subtitle_readers[] =
  {
    {
      name: "Subrip (srt)",
      probe:              probe_srt,
      read_subtitle_text: read_srt,
    },
    {
      name: "Mplayer mpsub",
      init:               init_mpsub,
      probe:              probe_mpsub,
      read_subtitle_text: read_mpsub,
      close:              close_mpsub,
    },
#ifdef HAVE_LIBPNG
    {
      name: "Spumux (xml/png)",
      probe:                 probe_spumux,
      init:                  init_spumux,
      read_subtitle_overlay: read_spumux,
      seek:                  seek_spumux,
      close:                 close_spumux,
    },
#endif // HAVE_LIBPNG
    { /* End of subtitle readers */ }
    
  };

void bgav_subreaders_dump()
  {
  int i = 0;
  bgav_dprintf( "<h2>Subtitle readers</h2>\n<ul>\n");
  while(subtitle_readers[i].name)
    {
    bgav_dprintf( "<li>%s\n", subtitle_readers[i].name);
    i++;
    }
  bgav_dprintf( "</ul>\n");
  }

static char * extensions[] =
  {
    "srt",
    "sub",
    "xml",
    (char*)0
  };

static bgav_subtitle_reader_t * find_subtitle_reader(const char * filename,
                                                     const bgav_options_t * opt)
  {
  int i;
  bgav_input_context_t * input;
  const char * extension;
  bgav_subtitle_reader_t* ret = (bgav_subtitle_reader_t*)0;
  
  char * line = (char*)0;
  int line_alloc = 0;
  int line_len;
  /* 1. Check if we have a supported extension */
  extension = strrchr(filename, '.');
  if(!extension)
    return (bgav_subtitle_reader_t*)0;

  extension++;
  i = 0;
  while(extensions[i])
    {
    if(!strcasecmp(extension, extensions[i]))
      break;
    i++;
    }
  if(!extensions[i])
    return (bgav_subtitle_reader_t*)0;

  /* 2. Open the file and do autodetection */
  input = bgav_input_create(opt);
  if(!bgav_input_open(input, filename))
    {
    bgav_input_destroy(input);
    return (bgav_subtitle_reader_t*)0;
    }

  bgav_input_detect_charset(input);
  while(bgav_input_read_convert_line(input, &line, &line_alloc, &line_len))
    {
    i = 0;

        
    while(subtitle_readers[i].name)
      {
      if(subtitle_readers[i].probe(line))
        {
        ret = &(subtitle_readers[i]);
        break;
        }
      i++;
      }
    if(ret)
      break;
    }
  bgav_input_destroy(input);
  free(line);
  
  return ret;
  }

extern bgav_input_t bgav_input_file;

bgav_subtitle_reader_context_t *
bgav_subtitle_reader_open(bgav_input_context_t * input_ctx)
  {
  char * file, *directory, *pos, *name;
  int file_len;
  DIR * dir;
  struct dirent * res;
  bgav_subtitle_reader_t * r;
  char * subtitle_filename;
  bgav_subtitle_reader_context_t * ret = (bgav_subtitle_reader_context_t *)0;
  bgav_subtitle_reader_context_t * end = (bgav_subtitle_reader_context_t *)0;
  bgav_subtitle_reader_context_t *new;
  
  union
    {
    struct dirent d;
    char b[sizeof (struct dirent) + NAME_MAX];
    } u;
  
  /* Check if input is a regular file */
  if((input_ctx->input != &bgav_input_file) || !input_ctx->filename)
    {
    return (bgav_subtitle_reader_context_t*)0;
    }

  /* Get directory name and open directory */
  directory = bgav_strdup(input_ctx->filename);
  pos = strrchr(directory, '/');
  if(!pos)
    {
    free(directory);
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
    return (bgav_subtitle_reader_context_t*)0;
    }

  while(!readdir_r(dir, &u.d, &res))
    {
    if(!res)
      break;
    
    /* Check, if the filenames match */
    if(strncasecmp(u.d.d_name, file, file_len) ||
       !strcmp(u.d.d_name, file))
      continue;
    
    subtitle_filename = bgav_sprintf("%s/%s", directory, u.d.d_name);
    
    r = find_subtitle_reader(subtitle_filename, input_ctx->opt);
    if(!r)
      {
      free(subtitle_filename);
      continue;
      }
    new = calloc(1, sizeof(*new));
    new->filename = subtitle_filename;
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
  if(ctx->ovl.frame) gavl_video_frame_destroy(ctx->ovl.frame);

  
  if(ctx->input)
    bgav_input_destroy(ctx->input);
  free(ctx);
  }

int bgav_subtitle_reader_has_subtitle(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;

  if(!ctx->has_subtitle)
    {
    if(ctx->reader->read_subtitle_text && ctx->reader->read_subtitle_text(s))
      ctx->has_subtitle = 1;
    else if(ctx->reader->read_subtitle_overlay && ctx->reader->read_subtitle_overlay(s))
      ctx->has_subtitle = 1;
    }
  return ctx->has_subtitle;
  }

bgav_packet_t * bgav_subtitle_reader_read_text(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;

  if(!ctx->has_subtitle && ctx->reader->read_subtitle_text(s))
    ctx->has_subtitle = 1;

  if(ctx->has_subtitle)
    {
    ctx->has_subtitle = 0;
    return s->data.subtitle.subreader->p;
    }
  else
    return (bgav_packet_t *)0;
  }

int bgav_subtitle_reader_start(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;
  if(!bgav_input_open(ctx->input, ctx->filename))
    return 0;

  bgav_input_detect_charset(ctx->input);
  if(ctx->input->charset) /* We'll do charset conversion by the input */
    s->data.subtitle.charset = bgav_strdup("UTF-8");
  
  if(ctx->reader->init && !ctx->reader->init(s))
    return 0;

  if(s->type == BGAV_STREAM_SUBTITLE_OVERLAY)
    {
    ctx->ovl.frame = gavl_video_frame_create(&s->data.subtitle.format);
    }
  return 1;
  }

void bgav_subtitle_reader_seek(bgav_stream_t * s,
                               gavl_time_t time)
  {
  int titles_skipped;
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;

  if(ctx->reader->seek)
    ctx->reader->seek(s, time);
    
  else if(ctx->input->input->seek_byte)
    {
    bgav_input_seek(ctx->input, ctx->data_start, SEEK_SET);

    titles_skipped = 0;
    
    if(ctx->reader->read_subtitle_text)
      {
      while(ctx->reader->read_subtitle_text(s))
        {
        if(ctx->p->pts + ctx->p->duration < time)
          {
          titles_skipped++;
        continue;
          }
        else
          break;
        }
      }
    else if(ctx->reader->read_subtitle_overlay)
      {
      while(ctx->reader->read_subtitle_overlay(s))
        {
        if(ctx->ovl.frame->time_scaled + ctx->ovl.frame->duration_scaled < time)
          {
          titles_skipped++;
          continue;
          }
        else
          break;
        }
      }
    ctx->has_subtitle = 1;
    }
  }

int bgav_subtitle_reader_read_overlay(bgav_stream_t * s, gavl_overlay_t * ovl)
  {
  gavl_video_format_t copy_format;
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;

  if(!ctx->has_subtitle && ctx->reader->read_subtitle_overlay(s))
    ctx->has_subtitle = 1;

  if(ctx->has_subtitle)
    {
    ctx->has_subtitle = 0;
    gavl_video_format_copy(&copy_format, &(s->data.subtitle.format));
    copy_format.image_width = ctx->ovl.ovl_rect.w;
    copy_format.frame_width = ctx->ovl.ovl_rect.w;

    copy_format.image_height = ctx->ovl.ovl_rect.h;
    copy_format.frame_height = ctx->ovl.ovl_rect.h;
    gavl_video_frame_copy(&copy_format, ovl->frame, ctx->ovl.frame);
    ovl->frame->time_scaled     = ctx->ovl.frame->time_scaled;
    ovl->frame->duration_scaled = ctx->ovl.frame->duration_scaled;
    ovl->dst_x = ctx->ovl.dst_x;
    ovl->dst_y = ctx->ovl.dst_y;
    gavl_rectangle_i_copy(&(ovl->ovl_rect), &(ctx->ovl.ovl_rect));
    return 1;
    }
  else
    return 0;
  }
