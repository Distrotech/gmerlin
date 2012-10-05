/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <glob.h>

#include <avdec_private.h>

/* SRT format */

static int probe_srt(char * line)
  {
  int a1,a2,a3,a4,b1,b2,b3,b4,i;
  if(sscanf(line, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",
            &a1,&a2,&a3,(char *)&i,&a4,&b1,&b2,&b3,(char *)&i,&b4) == 10)
    return 1;
  return 0;
  }

static int init_srt(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  s->timescale = 1000;
  ctx = s->data.subtitle.subreader;
  ctx->scale_num = 1;
  ctx->scale_den = 1;
  return 1;
  }

static int read_srt(bgav_stream_t * s, bgav_packet_t * p)
  {
  int lines_read;
  uint32_t line_len;
  int a1,a2,a3,a4,b1,b2,b3,b4;
  int i,len;
  bgav_subtitle_reader_context_t * ctx;
  gavl_time_t start, end;
  
  ctx = s->data.subtitle.subreader;
  
  /* Read lines */
  while(1)
    {
    if(!bgav_input_read_convert_line(ctx->input, &ctx->line,
                                     &ctx->line_alloc, &line_len))
      return 0;

    if(ctx->line[0] == '@')
      {
      if(!strncasecmp(ctx->line, "@OFF=", 5))
        {
        ctx->time_offset += (int)(atof(ctx->line+5) * 1000);
        fprintf(stderr, "new time offset: %"PRId64"\n", ctx->time_offset);
        }
      else if(!strncasecmp(ctx->line, "@SCALE=", 7))
        {
        sscanf(ctx->line + 7, "%d:%d", &ctx->scale_num, &ctx->scale_den);
        fprintf(stderr, "new scale factor: %d:%d\n",
                ctx->scale_num, ctx->scale_den);
        }
      continue;
      }
    
    
    else if((len=sscanf (ctx->line,
                         "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",
                         &a1,&a2,&a3,(char *)&i,&a4,
                         &b1,&b2,&b3,(char *)&i,&b4)) == 10)
      {
      break;
      }
    }

  start  = a1;
  start *= 60;
  start += a2;
  start *= 60;
  start += a3;
  start *= 1000;
  start += a4;

  end  = b1;
  end *= 60;
  end += b2;
  end *= 60;
  end += b3;
  end *= 1000;
  end += b4;
  
  p->pts = start + ctx->time_offset;
  p->duration = end - start;

  p->pts = gavl_time_rescale(ctx->scale_den,
                             ctx->scale_num,
                             p->pts);

  p->duration = gavl_time_rescale(ctx->scale_den,
                                  ctx->scale_num,
                                  p->duration);
  
  p->data_size = 0;
  
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
        p->data[p->data_size] = '\0';
        // Terminator doesn't count for data size
        // p->data_size++;
        }
      return 1;
      }
    if(lines_read)
      {
      p->data[p->data_size] = '\n';
      p->data_size++;
      }
    
    lines_read++;
    bgav_packet_alloc(p, p->data_size + line_len + 2);
    memcpy(p->data + p->data_size, ctx->line, line_len);
    p->data_size += line_len;
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
  
  if(!strncmp(line, "FORMAT=TIME", 11) ||
     (sscanf(line, "FORMAT=%f", &f) == 1))
    return 1;
  return 0;
  }

static int init_mpsub(bgav_stream_t * s)
  {
  uint32_t line_len;
  double framerate;
  char * ptr;
  bgav_subtitle_reader_context_t * ctx;
  mpsub_priv_t * priv = calloc(1, sizeof(*priv));
  ctx = s->data.subtitle.subreader;
  ctx->priv = priv;
  s->timescale = GAVL_TIME_SCALE;
  while(1)
    {
    if(!bgav_input_read_line(ctx->input, &ctx->line,
                             &ctx->line_alloc, 0, &line_len))
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

static int read_mpsub(bgav_stream_t * s, bgav_packet_t * p)
  {
  int i1, i2;
  double d1, d2;
  gavl_time_t t1 = 0, t2 = 0;
  
  uint32_t line_len;
  int lines_read;
  bgav_subtitle_reader_context_t * ctx;
  mpsub_priv_t * priv;
  char * ptr;
  
  ctx = s->data.subtitle.subreader;
  priv = ctx->priv;
    
  while(1)
    {
    if(!bgav_input_read_line(ctx->input, &ctx->line,
                             &ctx->line_alloc, 0, &line_len))
      return 0;

    ptr = ctx->line;
    
    while(isspace(*ptr) && (*ptr != '\0'))
      ptr++;
    
    /*
     * The following will reset last_end_time whenever we
     * cross a "FORMAT=" line
     */
    
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

  p->pts = priv->last_end_time + t1;
  p->duration  = t2;
  
  priv->last_end_time = p->pts + p->duration;
  
  /* Read the actual stuff */
  p->data_size = 0;
  
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
        // Terminator doesn't count to data size
        p->data[p->data_size] = '\0';
        //        p->data_size++;
        }
      return 1;
      }
    if(lines_read)
      {
      p->data[p->data_size] = '\n';
      p->data_size++;
      }
    
    lines_read++;
    bgav_packet_alloc(p, p->data_size + line_len + 2);
    memcpy(p->data + p->data_size, ctx->line, line_len);
    p->data_size += line_len;
    }
  return 0;
  }

static void close_mpsub(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  mpsub_priv_t * priv;
  
  ctx = s->data.subtitle.subreader;
  priv = ctx->priv;
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
  priv = ctx->priv;
  
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
  priv = ctx->priv;

  priv->cur = priv->cur->next;
  while(priv->cur && (!priv->cur->name || strcasecmp(priv->cur->name, "spu")))
    {
    priv->cur = priv->cur->next;
    }
  if(!priv->cur)
    return 0;
  return 1;
  }

static gavl_time_t parse_time_spumux(const char * str,
                                     int timescale, int frame_duration)
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
  if(f)
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
  char * error_msg = NULL;
  
  bgav_subtitle_reader_context_t * ctx;
  spumux_t * priv;
  ctx = s->data.subtitle.subreader;
  priv = ctx->priv;

  if(!priv->cur)
    return 0;

  //  bgav_yml_dump(priv->cur);

  start_time = bgav_yml_get_attribute_i(priv->cur, "start");
  if(!start_time)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "yml node has no start attribute");
    return 0;
    }
  
  if(!priv->have_header)
    {
    filename = bgav_yml_get_attribute_i(priv->cur, "image");
    if(!filename)
      {
      bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "yml node has no filename attribute");
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
  
  ctx->ovl.frame->timestamp =
    parse_time_spumux(start_time, s->data.subtitle.format.timescale,
                      s->data.subtitle.format.frame_duration);
  
  if(ctx->ovl.frame->timestamp == GAVL_TIME_UNDEFINED)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Parsing time string %s failed", start_time);
    return 0;
    }
  tmp = bgav_yml_get_attribute_i(priv->cur, "end");
  if(tmp)
    {
    ctx->ovl.frame->duration =
      parse_time_spumux(tmp,
                        s->data.subtitle.format.timescale,
                        s->data.subtitle.format.frame_duration);
    if(ctx->ovl.frame->duration == GAVL_TIME_UNDEFINED)
      return 0;
    ctx->ovl.frame->duration -= ctx->ovl.frame->timestamp;
    }
  else
    {
    ctx->ovl.frame->duration = -1;
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
  const char * tmp;
  bgav_subtitle_reader_context_t * ctx;
  spumux_t * priv;
  ctx = s->data.subtitle.subreader;
  s->timescale = GAVL_TIME_SCALE;
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

  /* Get duration */
  if(!init_current_spumux(s))
    return 0;
  
  do{
    tmp = bgav_yml_get_attribute_i(priv->cur, "end");
    s->duration = parse_time_spumux(tmp,
                                    s->data.subtitle.format.timescale,
                                    s->data.subtitle.format.frame_duration);
    } while(advance_current_spumux(s));
  
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
  s->data.subtitle.format.timescale   = GAVL_TIME_SCALE;
  
  return 1;
  }

static void seek_spumux(bgav_stream_t * s, int64_t time1, int scale)
  {
  const char * start_time, * end_time;
  bgav_subtitle_reader_context_t * ctx;
  spumux_t * priv;
  gavl_time_t start, end = 0;

  gavl_time_t time = gavl_time_unscale(scale, time1);
  
  ctx = s->data.subtitle.subreader;
  priv = ctx->priv;
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
  priv = ctx->priv;

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


static const bgav_subtitle_reader_t subtitle_readers[] =
  {
    {
      .name = "Subrip (srt)",
      .init =               init_srt,
      .probe =              probe_srt,
      .read_subtitle_text = read_srt,
    },
    {
      .name = "Mplayer mpsub",
      .init =               init_mpsub,
      .probe =              probe_mpsub,
      .read_subtitle_text = read_mpsub,
      .close =              close_mpsub,
    },
#ifdef HAVE_LIBPNG
    {
      .name = "Spumux (xml/png)",
      .probe =                 probe_spumux,
      .init =                  init_spumux,
      .read_subtitle_overlay = read_spumux,
      .seek =                  seek_spumux,
      .close =                 close_spumux,
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

static char const * const extensions[] =
  {
    "srt",
    "sub",
    "xml",
    NULL
  };

static const bgav_subtitle_reader_t *
find_subtitle_reader(const char * filename,
                     const bgav_options_t * opt)
  {
  int i;
  bgav_input_context_t * input;
  const char * extension;
  const bgav_subtitle_reader_t* ret = NULL;
  
  char * line = NULL;
  uint32_t line_alloc = 0;
  uint32_t line_len;
  /* 1. Check if we have a supported extension */
  extension = strrchr(filename, '.');
  if(!extension)
    return NULL;

  extension++;
  i = 0;
  while(extensions[i])
    {
    if(!strcasecmp(extension, extensions[i]))
      break;
    i++;
    }
  if(!extensions[i])
    return NULL;

  /* 2. Open the file and do autodetection */
  input = bgav_input_create(opt);
  if(!bgav_input_open(input, filename))
    {
    bgav_input_destroy(input);
    return NULL;
    }

  bgav_input_detect_charset(input);
  while(bgav_input_read_convert_line(input, &line, &line_alloc, &line_len))
    {
    i = 0;

        
    while(subtitle_readers[i].name)
      {
      if(subtitle_readers[i].probe(line))
        {
        ret = &subtitle_readers[i];
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
  char *pattern, *pos, *name;
  const bgav_subtitle_reader_t * r;
  bgav_subtitle_reader_context_t * ret = NULL;
  bgav_subtitle_reader_context_t * end = NULL;
  bgav_subtitle_reader_context_t *new;
  glob_t glob_buf;
  int i;
  int base_len;
  
  /* Check if input is a regular file */
  if((input_ctx->input != &bgav_input_file) || !input_ctx->filename)
    {
    return NULL;
    }

  pattern = bgav_strdup(input_ctx->filename);
  pos = strrchr(pattern, '.');
  if(!pos)
    {
    free(pattern);
    return NULL;
    }

  base_len = pos - pattern;
  
  pos[0] = '*';
  pos[1] = '\0';

  if(glob(pattern, 0, NULL, &glob_buf))
    return NULL;

  free(pattern);
  
  for(i = 0; i < glob_buf.gl_pathc; i++)
    {
    if(!strcmp(glob_buf.gl_pathv[i], input_ctx->filename))
      continue;
    //    fprintf(stderr, "Found %s\n", glob_buf.gl_pathv[i]);

    r = find_subtitle_reader(glob_buf.gl_pathv[i], input_ctx->opt);
    if(!r)
      continue;
    
    new = calloc(1, sizeof(*new));
    new->filename = bgav_strdup(glob_buf.gl_pathv[i]);
    new->input    = bgav_input_create(input_ctx->opt);
    new->reader   = r;

    name = glob_buf.gl_pathv[i] + base_len;
    
    while(!isalnum(*name) && (*name != '\0'))
      name++;

    if(*name != '\0')
      {
      pos = strrchr(name, '.');
      new->info = bgav_strndup(name, pos);
      }
    
    /* Apped to list */
    
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
  globfree(&glob_buf);
  return ret;
  }

void bgav_subtitle_reader_stop(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;

  if(ctx->reader->close)
    ctx->reader->close(s);

  if(ctx->ovl.frame)
    {
    gavl_video_frame_destroy(ctx->ovl.frame);
    ctx->ovl.frame = NULL;
    }
  
  if(ctx->input)
    bgav_input_close(ctx->input);
  }

void bgav_subtitle_reader_destroy(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;
  if(ctx->info)
    free(ctx->info);
  if(ctx->filename)
    free(ctx->filename);
  if(ctx->line)
    free(ctx->line);
  if(ctx->input)
    bgav_input_destroy(ctx->input);
  free(ctx);
  }

int bgav_subtitle_reader_start(bgav_stream_t * s)
  {
  bgav_subtitle_reader_context_t * ctx;
  ctx = s->data.subtitle.subreader;
  if(!bgav_input_open(ctx->input, ctx->filename))
    return 0;

  bgav_input_detect_charset(ctx->input);
  if(ctx->input->charset) /* We'll do charset conversion by the input */
    s->data.subtitle.charset = bgav_strdup(BGAV_UTF8);
  
  if(ctx->reader->init && !ctx->reader->init(s))
    return 0;

  if(s->type == BGAV_STREAM_SUBTITLE_OVERLAY)
    {
    ctx->ovl.frame = gavl_video_frame_create(&s->data.subtitle.format);
    }
  return 1;
  }

void bgav_subtitle_reader_seek(bgav_stream_t * s,
                               int64_t time1, int scale)
  {
  bgav_subtitle_reader_context_t * ctx;
  int64_t time = gavl_time_rescale(scale, s->timescale, time1);
  
  ctx = s->data.subtitle.subreader;

  if(ctx->reader->seek)
    ctx->reader->seek(s, time, scale);
    
  else if(ctx->input->input->seek_byte)
    {
    bgav_input_seek(ctx->input, ctx->data_start, SEEK_SET);
    
    ctx->time_offset = 0;
    if(ctx->reader->read_subtitle_text)
      {
      if(!ctx->out_packet)
        ctx->out_packet = bgav_packet_pool_get(s->pp);
      
      while(ctx->reader->read_subtitle_text(s, ctx->out_packet))
        {
        if(ctx->out_packet->pts + ctx->out_packet->duration < time)
          continue;
        else
          break;
        }
      }
    else if(ctx->reader->read_subtitle_overlay)
      {
      while(ctx->reader->read_subtitle_overlay(s))
        {
        if(ctx->ovl.frame->timestamp + ctx->ovl.frame->duration < time)
          continue;
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
    gavl_video_format_copy(&copy_format, &s->data.subtitle.format);
    copy_format.image_width = ctx->ovl.ovl_rect.w;
    copy_format.frame_width = ctx->ovl.ovl_rect.w;

    copy_format.image_height = ctx->ovl.ovl_rect.h;
    copy_format.frame_height = ctx->ovl.ovl_rect.h;
    gavl_video_frame_copy(&copy_format, ovl->frame, ctx->ovl.frame);
    ovl->frame->timestamp     = ctx->ovl.frame->timestamp;
    ovl->frame->duration = ctx->ovl.frame->duration;
    ovl->dst_x = ctx->ovl.dst_x;
    ovl->dst_y = ctx->ovl.dst_y;
    gavl_rectangle_i_copy(&ovl->ovl_rect, &ctx->ovl.ovl_rect);
    return 1;
    }
  else
    return 0;
  }

#if 0
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
    return NULL;
  }
#endif

/* Generic functions */

bgav_packet_t *
bgav_subtitle_reader_read_text_packet(void * subreader)
  {
  bgav_subtitle_reader_context_t * ctx = subreader;

  bgav_packet_t * ret;
  if(ctx->out_packet)
    {
    ret = ctx->out_packet;
    ctx->out_packet = NULL;
    return ret;
    }
  ret = bgav_packet_pool_get(ctx->s->pp);
  
  if(ctx->reader->read_subtitle_text(ctx->s, ret))
    return ret;
  else
    {
    bgav_packet_pool_put(ctx->s->pp, ret);
    return NULL;
    }
  
  }

bgav_packet_t *
bgav_subtitle_reader_peek_text_packet(void * subreader, int force)
  {
  bgav_subtitle_reader_context_t * ctx = subreader;

  if(!ctx->out_packet)
    {
    ctx->out_packet = bgav_packet_pool_get(ctx->s->pp);
    
    if(!ctx->reader->read_subtitle_text(ctx->s, ctx->out_packet))
      {
      bgav_packet_pool_put(ctx->s->pp, ctx->out_packet);
      ctx->out_packet = NULL;
      }
    }
  
  return ctx->out_packet;
  }
