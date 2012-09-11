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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>


#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/log.h>
#include <gmerlin/utils.h>

#include "v4l2_common.h"

#define LOG_DOMAIN "ov_v4l2"

#define MAX_BUFFERS  4  // TODO: Always enough?
#define MMAP_BUFFERS 4 // TODO: Always enough?

typedef struct
  {
  gavl_video_frame_t * f;
  void * buf;
  size_t size;
  int index;
  int queued;
  } buffer_t;

typedef struct
  {
  int fd;
  char * device;
  bg_parameter_info_t * parameters;
  gavl_video_format_t format;

  struct v4l2_queryctrl * controls;
  int num_controls;
  bgv4l2_io_method_t io;
  struct v4l2_format fmt;

  buffer_t buffers[MAX_BUFFERS];
  int num_buffers;
  int need_streamon;
  int num_queued;

  int buffer_index;
  
  int force_rw;

  gavl_video_sink_t * sink;
  
  } ov_v4l2_t;

static void cleanup_v4l(ov_v4l2_t *);

static void * create_v4l2()
  {
  ov_v4l2_t * v4l;

  v4l = calloc(1, sizeof(*v4l));
  v4l->fd = -1;
  return v4l;
  }

static void  destroy_v4l2(void * priv)
  {
  ov_v4l2_t * v4l = priv;

  /* Close the device just now */
  cleanup_v4l(v4l);
  if(v4l->parameters)
    bg_parameter_info_destroy_array(v4l->parameters);
  free(v4l);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "device_section",
      .long_name =   TRS("Device"),
      .type =        BG_PARAMETER_SECTION
    },
    {
      .name =        "device",
      .long_name =   TRS("V4L2 Device"),
      .type =        BG_PARAMETER_MULTI_MENU,
      .val_default = { .val_str = "/dev/video0" },
    },
    {
      .name =        "force_rw",
      .long_name =   TRS("Force write"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Don't use memory mapping")
    },
    { /* End */ }
  };

static void create_parameters(ov_v4l2_t * v4l)
  {
  bg_parameter_info_t * info;
  v4l->parameters = bg_parameter_info_copy_array(parameters);
  
  info = v4l->parameters + 1;
  bgv4l2_create_device_selector(info, V4L2_CAP_VIDEO_OUTPUT);
  }

static const bg_parameter_info_t * get_parameters_v4l2(void * priv)
  {
  ov_v4l2_t * v4l = priv;
  if(!v4l->parameters)
    create_parameters(v4l);
  return v4l->parameters;
  }


static void set_parameter_v4l2(void * priv, const char * name,
                               const bg_parameter_value_t * val)
  {
  ov_v4l2_t * v4l;
  v4l = priv;

  if(!name)
    {
    return;
    }
  else if(!strcmp(name, "device"))
    v4l->device = bg_strdup(v4l->device, val->val_str);
  else if(!strcmp(name, "force_rw"))
    v4l->force_rw = val->val_i;
  else if(v4l->controls && (v4l->fd >= 0))
    {
    bgv4l2_set_device_parameter(v4l->fd,
                                v4l->controls,
                                v4l->num_controls,
                                name, val);
    }
  }

static int get_parameter_v4l2(void * priv, const char * name,
                              bg_parameter_value_t * val)
  {
  ov_v4l2_t * v4l = priv;
  if(v4l->controls && (v4l->fd >= 0))
    {
    return bgv4l2_get_device_parameter(v4l->fd,
                                       v4l->controls,
                                       v4l->num_controls,
                                       name, val);
    }
  return 0;
  }

static gavl_pixelformat_t * get_pixelformats(int fd)
  {
  int index = 0;
  struct v4l2_fmtdesc desc;
  gavl_pixelformat_t fmt;
  gavl_pixelformat_t * ret = NULL;
  int num_ret = 0;
  int i;
  int present;
  CLEAR(desc);
  desc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT; 

  while(1)
    {
    desc.index = index;
    if(-1 == bgv4l2_ioctl (fd, VIDIOC_ENUM_FMT, &desc))
      {
      if(!index || (errno != EINVAL))
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOC_ENUM_FMT failed: %s", strerror(errno));
      break;
      }
    fmt = bgv4l2_pixelformat_v4l2_2_gavl(desc.pixelformat);
    
    if(fmt != GAVL_PIXELFORMAT_NONE)
      {
      present = 0;
      for(i = 0; i < num_ret; i++)
        {
        if(ret[i] == fmt)
          {
          present = 1;
          break;
          }
        }
      if(!present)
        {
        ret = realloc(ret, (num_ret+2) * sizeof(*ret));
        ret[num_ret] = fmt;
        ret[num_ret+1] = GAVL_PIXELFORMAT_NONE;
        num_ret++;
        // fprintf(stderr, "Device supports %s\n", gavl_pixelformat_to_string(fmt));
        }
      }
    index++;
    }
  return ret;
  }

static int init_write(ov_v4l2_t * v4l)
  {
  v4l->num_buffers = 1;
  v4l->buffers[0].f = bgv4l2_create_frame(NULL, // Can be NULL
                                          &v4l->format,
                                          &v4l->fmt);
  return 1;
  }

static void cleanup_write(ov_v4l2_t * v4l)
  {
  if(v4l->buffers[0].f)
    gavl_video_frame_destroy(v4l->buffers[0].f);
  
  }

static int init_mmap(ov_v4l2_t * v4l)
  {
  int i;
  struct v4l2_requestbuffers req;
  struct v4l2_buffer buf;
  
  CLEAR (req);
  
  req.count               = MMAP_BUFFERS;
  req.type                = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  req.memory              = V4L2_MEMORY_MMAP;

  /* Request buffers */
  if (-1 == bgv4l2_ioctl (v4l->fd, VIDIOC_REQBUFS, &req))
    {
    if (EINVAL == errno)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "%s does not support "
               "memory mapping", v4l->device);
      return 0;
      }
    else
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOC_REQBUFS failed: %s", strerror(errno));
      return 0;
      }
    }

  if (req.count < 2)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Insufficient buffer memory on %s",
             v4l->device);
    return 0;
    }
  
  v4l->num_buffers = req.count;

  for(i = 0; i < v4l->num_buffers; i++)
    {
    CLEAR (buf);

    buf.type        = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = i;
    
    if (-1 == bgv4l2_ioctl (v4l->fd, VIDIOC_QUERYBUF, &buf))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOC_QUERYBUF failed: %s", strerror(errno));
      
      return 0;
      }
    v4l->buffers[i].size = buf.length;
    v4l->buffers[i].buf =
      mmap (NULL /* start anywhere */,
            buf.length,
            PROT_READ | PROT_WRITE /* required */,
            MAP_SHARED /* recommended */,
            v4l->fd, buf.m.offset);
    
    if (MAP_FAILED == v4l->buffers[i].buf)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "mmap failed: %s", strerror(errno));
      return 0;
      }
    v4l->buffers[i].index = i;
    
    v4l->buffers[i].f =
      bgv4l2_create_frame(v4l->buffers[i].buf, // Can be NULL
                          &v4l->format,
                          &v4l->fmt);
    v4l->buffers[i].f->user_data = &v4l->buffers[i];
    }
  v4l->need_streamon = 1;
  
  return 1;
  }

static void cleanup_mmap(ov_v4l2_t * v4l)
  {
  int i;
  enum v4l2_buf_type type;

  type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  if (-1 == bgv4l2_ioctl (v4l->fd, VIDIOC_STREAMOFF, &type))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOC_STREAMOFF failed: %s", strerror(errno));
    return;
    }
  for (i = 0; i < v4l->num_buffers; ++i)
    {
    if (-1 == munmap (v4l->buffers[i].buf, v4l->buffers[i].size))
      return;
    if(v4l->buffers[i].f)
      {
      gavl_video_frame_null(v4l->buffers[i].f);
      gavl_video_frame_destroy(v4l->buffers[i].f);
      }
    
    }
  }

static gavl_sink_status_t put_frame_write(ov_v4l2_t * v4l, gavl_video_frame_t * frame)
  {
  if(-1 == write(v4l->fd, frame->planes[0], v4l->fmt.fmt.pix.sizeimage))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "write failed: %s", strerror(errno));
    return GAVL_SINK_ERROR;
    }
  return GAVL_SINK_OK;
  }

static gavl_sink_status_t put_frame_mmap(ov_v4l2_t * v4l, gavl_video_frame_t * frame)
  {
  struct v4l2_buffer buf;
  buffer_t * b = frame->user_data;
  CLEAR (buf);
  
  buf.type        = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  buf.memory      = V4L2_MEMORY_MMAP;
  buf.index       = b->index;
  buf.bytesused   = v4l->fmt.fmt.pix.sizeimage;

  gettimeofday(&buf.timestamp, NULL);
  
  if (-1 == bgv4l2_ioctl (v4l->fd, VIDIOC_QBUF, &buf))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOC_QBUF failed: %s", strerror(errno));
    return GAVL_SINK_ERROR;
    }
  //  fprintf(stderr, "VIDIOC_QBUF %d\n", buf.index);

  v4l->num_queued++;
  
  if((v4l->num_queued >= v4l->num_buffers) && v4l->need_streamon)
    {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    v4l->need_streamon = 0;
    if (-1 == bgv4l2_ioctl (v4l->fd, VIDIOC_STREAMON, &type))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOC_STREAMON failed: %s", strerror(errno));
      return GAVL_SINK_ERROR;
      }
    //    fprintf(stderr, "STREAMON\n");
    }

  b->queued = 1;
  return GAVL_SINK_OK;
  }

static void cleanup_v4l(ov_v4l2_t * v4l)
  {
  if(v4l->fd < 0)
    return;
  
  switch(v4l->io)
    {
    case BGV4L2_IO_METHOD_RW:
      cleanup_write(v4l);
      break;
    case BGV4L2_IO_METHOD_MMAP:
      cleanup_mmap(v4l);
      break;
    default:
      return;
    }

  if(v4l->controls)
    {
    free(v4l->controls);
    v4l->controls = NULL;
    }
  v4l->num_queued = 0;
  close(v4l->fd);
  v4l->fd = -1;

  if(v4l->sink)
    {
    gavl_video_sink_destroy(v4l->sink);
    v4l->sink = NULL;
    }
  
  return;
  }


static gavl_video_frame_t * get_frame_dqbuf(ov_v4l2_t * v4l, int mode)
  {
  int i;
  struct v4l2_buffer buf;

  for(i = 0; i < v4l->num_buffers; i++)
    {
    if(!v4l->buffers[i].queued)
      return v4l->buffers[i].f;
    }

#if 0

  CLEAR(buf);
  buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  buf.index = v4l->buffer_index;
  buf.memory = mode;
  
  if (0 == bgv4l2_ioctl (v4l->fd, VIDIOC_QUERYBUF, &buf))
    {
    fprintf(stderr, "VIDIOC_QUERY_BUF %d done: %d, queued: %d\n",
            buf.index,
            !!(buf.flags & V4L2_BUF_FLAG_DONE),
            !!(buf.flags & V4L2_BUF_FLAG_QUEUED));
        
    }
#endif
  
  CLEAR(buf);
  buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  buf.memory = mode;
  
  if (-1 == bgv4l2_ioctl (v4l->fd, VIDIOC_DQBUF, &buf))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOC_DQBUF failed: %s",
           strerror(errno));
    return NULL;
    }

  v4l->buffer_index++;
  if(v4l->buffer_index >= v4l->num_buffers)
    v4l->buffer_index = 0;
  
  //  fprintf(stderr, "VIDIOC_DQBUF %d done: %d, queued: %d\n",
  //          buf.index,
  //          !!(buf.flags & V4L2_BUF_FLAG_DONE),
  //          !!(buf.flags & V4L2_BUF_FLAG_QUEUED));

  return v4l->buffers[buf.index].f;
  }

static gavl_video_frame_t * get_frame_v4l2(void * data)
  {

  ov_v4l2_t * v4l = data;
  
  switch(v4l->io)
    {
    case BGV4L2_IO_METHOD_RW:
      return v4l->buffers[0].f;
      break;
    case BGV4L2_IO_METHOD_MMAP:
      return get_frame_dqbuf(v4l, V4L2_MEMORY_MMAP);
      break;
    default:
      return NULL;
    }
  }


static gavl_sink_status_t put_func_v4l2(void * data, gavl_video_frame_t * frame)
  {
  ov_v4l2_t * v4l = data;

  //  fprintf(stderr, "Put video v4l2\n");
  
  switch(v4l->io)
    {
    case BGV4L2_IO_METHOD_RW:
      put_frame_write(v4l, frame);
      break;
    case BGV4L2_IO_METHOD_MMAP:
      put_frame_mmap(v4l, frame);
      break;
    default:
      break;
    }
  //  fprintf(stderr, "Put video v4l2 done\n");
  return GAVL_SINK_ERROR;
  }

static void put_frame_v4l2(void * data, gavl_video_frame_t * frame)
  {
  ov_v4l2_t * v4l = data;
  gavl_video_sink_put_frame(v4l->sink, frame);
  }

static int open_v4l2(void * priv,
                    gavl_video_format_t * format, int keep_aspect)
  {
  struct v4l2_capability cap;
  gavl_pixelformat_t * pixelformats;
  
  ov_v4l2_t * v4l = priv;
  
  if(v4l->fd >= 0)
    {
    /* We accept the framerate settings but nothing else */
    v4l->format.framerate_mode = format->framerate_mode;
    v4l->format.timescale      = format->timescale;
    v4l->format.frame_duration = format->frame_duration;
    
    gavl_video_format_copy(format, &v4l->format);
    return 1;
    }

  /* For now we accept only square pixels */
  if((format->pixel_width != format->pixel_height) && keep_aspect)
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Forcing square pixels");
    format->image_width *= format->pixel_width;
    format->image_width /= format->pixel_height;
    format->pixel_width = 1;
    format->pixel_height = 1;
    if(format->frame_width < format->image_width)
      format->frame_width = format->image_width;
    }
  
  CLEAR(v4l->fmt);
  
  v4l->fd = bgv4l2_open_device(v4l->device, V4L2_CAP_VIDEO_OUTPUT,
                                 &cap);
  if(v4l->fd < 0)
    return 0;
    
  if(v4l->controls)
    free(v4l->controls);
    
  v4l->controls =
    bgv4l2_create_device_controls(v4l->fd, &v4l->num_controls);
  
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Device name: %s", cap.card);

  /* Get the I/O method */
  if ((cap.capabilities & V4L2_CAP_STREAMING) && !v4l->force_rw)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Trying mmap i/o");
    v4l->io = BGV4L2_IO_METHOD_MMAP;
    }
  else if(cap.capabilities & V4L2_CAP_READWRITE)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Trying write i/o");
    v4l->io = BGV4L2_IO_METHOD_RW;
    }
  
  memset(&v4l->fmt, 0, sizeof(v4l->fmt));
  
  /* Set up pixelformat */
  pixelformats = get_pixelformats(v4l->fd);

  format->pixelformat = gavl_pixelformat_get_best(format->pixelformat,
                                                  pixelformats, NULL);

  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Using %s", gavl_pixelformat_to_string(format->pixelformat));

  /* Set up image size */
  v4l->fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  v4l->fmt.fmt.pix.width  = format->image_width;
  v4l->fmt.fmt.pix.height = format->image_height;
  v4l->fmt.fmt.pix.pixelformat = bgv4l2_pixelformat_gavl_2_v4l2(format->pixelformat);

  if (-1 == bgv4l2_ioctl (v4l->fd, VIDIOC_S_FMT, &v4l->fmt))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOC_S_FMT failed: %s", strerror(errno));
    return 0;
    }
  
  gavl_video_format_copy(&v4l->format, format);

  /* Set up buffers */
  switch(v4l->io)
    {
    case BGV4L2_IO_METHOD_RW:
      return init_write(v4l);
      break;
    case BGV4L2_IO_METHOD_MMAP:
      return init_mmap(v4l);
    default:
      return 0;
    }

  v4l->sink = gavl_video_sink_create(get_frame_v4l2,
                                     put_func_v4l2,
                                     v4l, format);
  
  return 1;
  }

static gavl_video_sink_t * get_sink_v4l2(void * data)
  {
  ov_v4l2_t * v4l = data;
  return v4l->sink;
  }

static void show_window_v4l2(void * data, int show)
  {
  ov_v4l2_t * v4l = data;
  if(!show)
    cleanup_v4l(v4l);
  }

static void close_v4l2(void * data)
  {
  // Noop (for now)
  }

const bg_ov_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "ov_v4l2",
      .long_name =     TRS("V4L2"),
      .description =   TRS("V4L2 output driver"),
      .type =          BG_PLUGIN_OUTPUT_VIDEO,
      .flags =         BG_PLUGIN_PLAYBACK,
      .priority =      1,
      .create =        create_v4l2,
      .destroy =       destroy_v4l2,

      .get_parameters = get_parameters_v4l2,
      .set_parameter =  set_parameter_v4l2,
      .get_parameter =  get_parameter_v4l2
    },
    //  .set_callbacks =  set_callbacks_v4l2,
    
    //    .set_window =         set_window_v4l2,
    //    .get_window =         get_window_v4l2,
    //    .set_window_options =   set_window_options_v4l2,
    //    .set_window_title =   set_window_title_v4l2,
    .open =               open_v4l2,
    .get_sink = get_sink_v4l2,
    .get_frame =    get_frame_v4l2,

    //    .add_overlay_stream = add_overlay_stream_v4l2,
    //    .create_overlay =    create_overlay_v4l2,
    //    .set_overlay =        set_overlay_v4l2,

    .put_frame =          put_frame_v4l2,

    //    .handle_events =  handle_events_v4l2,
    
    //    .destroy_overlay =   destroy_overlay_v4l2,
    .close =          close_v4l2,
    //    .update_aspect =  update_aspect_v4l2,
    
    .show_window =    show_window_v4l2
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
