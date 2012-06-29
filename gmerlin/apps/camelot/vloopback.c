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

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <pthread.h>
#include <sys/signal.h>
#include <string.h>

#include <gavl/gavl.h>
#include <linux/videodev.h>

#include <gmerlin/translation.h>
#include <gmerlin/utils.h>

#include <gmerlin/parameter.h>
#include "vloopback.h"

#include <gmerlin/log.h>

#define LOG_DOMAIN "vloopback"

#define VIDIOCSINVALID _IO('v',BASE_VIDIOCPRIVATE+1)

#define MAX_WIDTH 640
#define MAX_HEIGHT 480
#define MIN_WIDTH 32
#define MIN_HEIGHT 32

#define MAXIOCTL 1024

#define MAX_FRAMESIZE (MAX_WIDTH*MAX_HEIGHT*4)
#define NUM_FRAMES 2

#define DUMP_IOCTLS

#define MODE_WRITE 0
#define MODE_IOCTL 1

static void put_frame_ioctl(bg_vloopback_t * v, gavl_video_frame_t * frame);
static void put_frame_write(bg_vloopback_t * v, gavl_video_frame_t * frame);


struct bg_vloopback_s
  {
  int mode;
  int fd;
  gavl_video_converter_t * cnv;
  uint8_t * mmap;
  int mmap_len;

  gavl_video_format_t input_format;
  gavl_video_format_t output_format;

  /* write() */
  gavl_video_format_t write_format;
  int write_width;
  int write_height;
  int write_size_custom;
  gavl_video_frame_t * write_frame;
  int write_bytes;
  
  int out_palette;
  
  sigset_t oldset;
  int output_open;
  
  int out_depth;
  
  int do_grab;
  int grab_frame;
  
  int format_changed;
  gavl_video_frame_t * mmap_frames[NUM_FRAMES];

  int do_convert;

  char * device;

  int changed;

  void (*put_frame)(bg_vloopback_t * v, gavl_video_frame_t * frame);
  };


/* Colorspace translation stuff */

static const struct
  {
  int               v4l;
  gavl_pixelformat_t gavl;
  int depth;
  }
pixelformats[] =
  {
    { VIDEO_PALETTE_GREY, GAVL_GRAY_8, 8 }, /* Linear greyscale */
    // VIDEO_PALETTE_HI240     2       /* High 240 cube (BT848) */
    { VIDEO_PALETTE_RGB565, GAVL_RGB_16, 16 },       /* 565 16 bit RGB */
    { VIDEO_PALETTE_RGB24,  GAVL_BGR_24, 24 },       /* 24bit RGB */
    { VIDEO_PALETTE_RGB32,  GAVL_RGB_32, 32 },       /* 32bit RGB */
    { VIDEO_PALETTE_RGB555, GAVL_RGB_15, 15 },       /* 555 15bit RGB */
    { VIDEO_PALETTE_YUV422, GAVL_YUY2,   24 },  /* YUV422 capture */
    { VIDEO_PALETTE_YUYV,   GAVL_YUY2,   24 },
    { VIDEO_PALETTE_UYVY,   GAVL_UYVY,   24 },  /* The great thing about standards is ... */
    // VIDEO_PALETTE_YUV420    10
    // VIDEO_PALETTE_YUV411    11      /* YUV411 capture */
    // VIDEO_PALETTE_RAW       12      /* RAW capture (BT848) */
    { VIDEO_PALETTE_YUV422P, GAVL_YUV_422_P, 24 },  /* YUV 4:2:2 Planar */
    { VIDEO_PALETTE_YUV411P,  GAVL_YUV_411_P, 24 },  /* YUV 4:1:1 Planar */
    //    { VIDEO_PALETTE_YUV420P, GAVL_YUV_420_P, 12 },  /* YUV 4:2:0 Planar */
    { VIDEO_PALETTE_YUV420P, GAVL_YUV_420_P, 24 },  /* YUV 4:2:0 Planar */

    // VIDEO_PALETTE_YUV410P   16      /* YUV 4:1:0 Planar */
  };

static const int
num_pixelformats = sizeof(pixelformats)/sizeof(pixelformats[0]);

static gavl_pixelformat_t get_gavl_pixelformat(int csp)
  {
  int i;
  for(i = 0; i < num_pixelformats; i++)
    {
    if(pixelformats[i].v4l == csp)
      return pixelformats[i].gavl;
    }
  return GAVL_PIXELFORMAT_NONE;
  }

static int get_v4l_pixelformat(gavl_pixelformat_t csp, int * depth)
  {
  int i;
  for(i = 0; i < num_pixelformats; i++)
    {
    if(pixelformats[i].gavl == csp)
      {
      if(depth)
        *depth = pixelformats[i].depth;
      return pixelformats[i].v4l;
      }
    }
  return -1;
  }

/* ioctl handler shamelessly stolen from effectv */

static int v4l_ioctlhandler(bg_vloopback_t * v,
                            unsigned long int cmd, void *arg)
  {
  gavl_pixelformat_t pfmt;
  switch(cmd)
    {
    case VIDIOCGCAP:
      {
      struct video_capability *vidcap = arg;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCGCAP");
#endif
      snprintf(vidcap->name, 32, "Camelot vloopback output");
      vidcap->type = VID_TYPE_CAPTURE;
      vidcap->channels = 1;
      vidcap->audios = 0;
      vidcap->maxwidth = MAX_WIDTH;
      vidcap->maxheight = MAX_HEIGHT;
      vidcap->minwidth = MIN_WIDTH;
      vidcap->minheight = MIN_HEIGHT;
      return 0;
      }
    case VIDIOCGCHAN:
      {
      struct video_channel *vidchan = arg;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCGCHAN");
#endif
      if(vidchan->channel != 0)
        return EINVAL;
      vidchan->flags = 0;
      vidchan->tuners = 0;
      vidchan->type = VIDEO_TYPE_CAMERA;
      strncpy(vidchan->name, "Camelot", 32);
      //      strncpy(vidchan->name, "Webcam", 32);
      return 0;
      }
    case VIDIOCSCHAN:
      {
      int *v = arg;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCSCHAN");
#endif
      if(v[0] != 0)
        return EINVAL;
      return 0;
      }
    case VIDIOCCAPTURE:
      {
      int * start = arg;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCCAPTURE %d", *start);
#endif
      return 0;
      }
    case VIDIOCGPICT:
      {
      struct video_picture *vidpic = arg;
      vidpic->colour = 0xffff;
      vidpic->hue = 0xffff;
      vidpic->brightness = 0xffff;
      vidpic->contrast = 0xffff;
      vidpic->whiteness = 0xffff;
      //      vidpic->depth = v->out_depth;
      vidpic->depth = 12;
      vidpic->palette = v->out_palette;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCGPICT %d %d",
             vidpic->depth, vidpic->palette);
#endif
      return 0;
      }
    case VIDIOCSPICT:
      {
      struct video_picture *vidpic = arg;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCSPICT");
#endif
      if(vidpic->palette != v->out_palette)
        {
        pfmt = get_gavl_pixelformat(vidpic->palette);
        if(pfmt == GAVL_PIXELFORMAT_NONE)
          {
          bg_log(BG_LOG_WARNING, LOG_DOMAIN,
                 "unsupported pixel format (%d) is requested in VIDIOCSPICT",
                 vidpic->palette);
          return EINVAL;
          }
        v->out_palette = vidpic->palette;
        v->output_format.pixelformat = pfmt;
        v->format_changed = 1;
        }
      return 0;
      }

    case VIDIOCGWIN:
      {
      struct video_window *vidwin = arg;
      vidwin->x = 0;
      vidwin->y = 0;
      vidwin->width = v->output_format.image_width;
      vidwin->height = v->output_format.image_height;
      vidwin->chromakey = 0;
      vidwin->flags = 655360;
      //      vidwin->flags = 0;
      vidwin->clipcount = 0;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCGWIN %dx%d", vidwin->width, vidwin->height);
#endif
      return 0;
      }
      
    case VIDIOCSWIN:
      {
      struct video_window *vidwin = arg;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCSWIN");
#endif
      if(vidwin->width > MAX_WIDTH || vidwin->height > MAX_HEIGHT)
        {
        bg_log(BG_LOG_WARNING, LOG_DOMAIN, "width %d out of range", vidwin->width);
        return EINVAL;
        }
      if(vidwin->width < MIN_WIDTH || vidwin->height < MIN_HEIGHT)
        {
        bg_log(BG_LOG_WARNING, LOG_DOMAIN, "height %d out of range", vidwin->height);
        return EINVAL;
        }
      if(vidwin->flags)
        {
        bg_log(BG_LOG_WARNING, LOG_DOMAIN, "invalid flags %08x in VIDIOCSWIN", vidwin->flags);
        return EINVAL;
        }
      v->output_format.image_width = vidwin->width;
      v->output_format.image_height = vidwin->height;
      return 0;
      }

    case VIDIOCSYNC:
      {
      int frame = *(int *)arg;
      int ret = 0;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCSYNC");
#endif
      if(frame < 0 || frame >= NUM_FRAMES)
        {
        bg_log(BG_LOG_WARNING, LOG_DOMAIN, "frame %d out of range in VIDIOCSYNC", frame);
        return EINVAL;
        }
      v->do_grab = 0;
      return ret;
      }

    case VIDIOCMCAPTURE:
      {
      struct video_mmap *vidmmap = arg;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCMCAPTURE");
#endif
      if((vidmmap->width  != v->output_format.image_width) ||
         (vidmmap->height != v->output_format.image_height))
        {
        if(vidmmap->width > MAX_WIDTH || vidmmap->height > MAX_HEIGHT)
          {
          bg_log(BG_LOG_WARNING, LOG_DOMAIN, "VIDIOCMCAPTURE: requested capture size is too big: %dx%d",
                 vidmmap->width, vidmmap->height);
          return EINVAL;
          }
        if(vidmmap->width < MIN_WIDTH || vidmmap->height < MIN_HEIGHT)
          {
          bg_log(BG_LOG_WARNING, LOG_DOMAIN, "VIDIOCMCAPTURE: requested capture size is too small: %dx%d",
                 vidmmap->width, vidmmap->height);
          return EINVAL;
          }
#ifdef DUMP_IOCTLS
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCMCAPTURE %dx%d, format: %d",
               vidmmap->width, vidmmap->height, vidmmap->format);
#endif
        v->output_format.image_width = vidmmap->width;
        v->output_format.image_height = vidmmap->height;
        v->format_changed = 1;
        }
      if(vidmmap->format != v->out_palette)
        {
        pfmt = get_gavl_pixelformat(vidmmap->format);
        if(pfmt == GAVL_PIXELFORMAT_NONE)
          {
          bg_log(BG_LOG_WARNING, LOG_DOMAIN,
                 "unsupported pixel format (%d) is requested in VIDIOCMCAPTURE",
                 vidmmap->format);
          return EINVAL;
          }
        v->out_palette = vidmmap->format;
        v->output_format.pixelformat = pfmt;
        v->format_changed = 1;
        }
      if(vidmmap->frame >= NUM_FRAMES || vidmmap->frame < 0)
        {
        bg_log(BG_LOG_WARNING, LOG_DOMAIN,
               "Invalid frame (%d) in VIDIOCMCAPTURE",
               vidmmap->frame);
        return EINVAL;
        }
      v->do_grab = 1;
      v->grab_frame = vidmmap->frame;
      return 0;
      }
      
    case VIDIOCGMBUF:
      {
      struct video_mbuf *vidmbuf = arg;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCGMBUF");
#endif
      vidmbuf->size = v->mmap_len;
      vidmbuf->frames = NUM_FRAMES;
      vidmbuf->offsets[0] = 0;
      vidmbuf->offsets[1] = MAX_FRAMESIZE;
      return 0;
      }

    default:
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN,
             "Unknown ioctl %lx", cmd);
          
      return EPERM;
      }
    }
  }

static void init(bg_vloopback_t * v)
  {
  int i;
  gavl_rectangle_i_t out_rect;
  gavl_rectangle_f_t in_rect;
  
  v->output_format.frame_width = v->output_format.image_width;
  v->output_format.frame_height = v->output_format.image_height;

  for(i = 0; i < NUM_FRAMES; i++)
    {
    v->mmap_frames[i]->strides[0] = 0;
    gavl_video_frame_set_planes(v->mmap_frames[i],
                                &v->output_format, v->mmap + i * MAX_FRAMESIZE);
    }
  
  gavl_rectangle_i_set_all(&out_rect, &v->output_format);
  gavl_rectangle_f_set_all(&in_rect, &v->input_format);

  gavl_video_options_set_rectangles(gavl_video_converter_get_options(v->cnv),
                                    &in_rect, &out_rect);
  
  v->do_convert = gavl_video_converter_init(v->cnv, &v->input_format,
                                            &v->output_format);

  //  fprintf(stderr, "Initializing vloopback, in format:\n");
  //  gavl_video_format_dump(&v->input_format);
  //  fprintf(stderr, "Out format:\n");
  //  gavl_video_format_dump(&v->output_format);
  
  }


bg_vloopback_t * bg_vloopback_create()
  {
  int i;
  sigset_t newset;
  bg_vloopback_t * ret = calloc(1, sizeof(*ret));

  /* Block SIGIO */
  sigemptyset(&newset);
  sigaddset(&newset, SIGIO);
  pthread_sigmask(SIG_BLOCK, &newset, &ret->oldset);
  
  ret->fd = -1;
  for(i = 0; i < NUM_FRAMES; i++)
    {
    ret->mmap_frames[i] = gavl_video_frame_create(NULL);
    }
  ret->cnv = gavl_video_converter_create();
  
  return ret;
  }

void bg_vloopback_destroy(bg_vloopback_t * v)
  {
  int i;
  if(v->fd >= 0)
    bg_vloopback_close(v);

  if(v->device)
    free(v->device);
  
  /* Restore signal mask */
  //  pthread_sigmask(SIG_SETMASK, &v->oldset, NULL);
  
  gavl_video_converter_destroy(v->cnv);

  for(i = 0; i < NUM_FRAMES; i++)
    {
    gavl_video_frame_null(v->mmap_frames[i]);
    gavl_video_frame_destroy(v->mmap_frames[i]);
    }
  free(v);
  }

int bg_vloopback_open(bg_vloopback_t * v)
  {
  struct video_capability vid_caps;
  struct video_window vid_win;
  struct video_picture vid_pic;
  
  //  fprintf(stderr, "vloopback open\n");
  v->fd = open(v->device, O_RDWR);

  if(v->fd < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Opening %s failed: %s",
           v->device, strerror(errno));
    return 0;
    }

  if(v->write_frame)
    {
    gavl_video_frame_destroy(v->write_frame);
    v->write_frame = NULL;
    }
  
  if(v->mode == MODE_WRITE)
    {
    int depth;
    if(v->write_size_custom)
      {
      v->write_format.image_width = v->write_width;
      v->write_format.image_height = v->write_height;
      }
    v->write_format.frame_width  = v->write_format.image_width;
    v->write_format.frame_height = v->write_format.image_height;

    v->write_format.pixel_width  = 1;
    v->write_format.pixel_height = 1;

    v->format_changed = 1;
    
    gavl_video_format_copy(&v->output_format, 
                           &v->write_format);
    
    v->write_frame = gavl_video_frame_create(&v->write_format);

    v->write_bytes = gavl_video_format_get_image_size(&v->write_format);

    //    fprintf(stderr, "Write format %d bytes:\n", v->write_bytes);
    //    gavl_video_format_dump(&v->write_format);
    
    
    /* The following is from the vloopback example code */
    if (ioctl(v->fd, VIDIOCGCAP, &vid_caps) == -1)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOCGCAP failed %s",strerror(errno));
      return 0;
      }
  
    if (ioctl(v->fd, VIDIOCGPICT, &vid_pic) == -1)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOCGPICT failed: %s",strerror(errno));
      return 0;
      }

    vid_pic.palette = get_v4l_pixelformat(v->write_format.pixelformat,
                                          &depth);
    vid_pic.depth = depth;
    
    if (ioctl(v->fd, VIDIOCSPICT, &vid_pic) == -1)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOCSPICT failed: %s",strerror(errno));
      return 0;
      }
  
    if (ioctl(v->fd, VIDIOCGWIN, &vid_win) == -1)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "ioctl VIDIOCGWIN failed");
      return 0;
      }

    vid_win.width  = v->write_format.image_width;
    vid_win.height = v->write_format.image_height;
  
    if (ioctl(v->fd, VIDIOCSWIN, &vid_win) == -1)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "ioctl VIDIOCSWIN failed");
      return (1);
      } 

    v->put_frame = put_frame_write;

    v->output_open = 1;
    
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Opened vloopback in write mode");
    }
  else
    {
    v->mmap_len = MAX_FRAMESIZE * NUM_FRAMES;
  
    v->mmap = mmap(NULL, v->mmap_len, PROT_READ | PROT_WRITE /* required */,
                   MAP_SHARED /* recommended */, v->fd, 0);
  
    if(MAP_FAILED == v->mmap)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "vloopback mmap failed: %s",
             strerror(errno));
      return 0;
      }

    v->put_frame = put_frame_ioctl;

    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Opened vloopback in ioctl mode");
    }

  v->changed = 0;
  
  /* */
  
  return 1;
  }

void bg_vloopback_close(bg_vloopback_t * v)
  {
  if(v->mmap)
    {
    munmap(v->mmap, v->mmap_len);
    v->mmap = NULL;
    }
  close(v->fd);
  v->fd = -1;
  v->output_open = 0;
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Closed vloopback");
  
  }

void bg_vloopback_set_format(bg_vloopback_t * v, const gavl_video_format_t * format)
  {
  //  fprintf(stderr, "vloopback set format\n");
  gavl_video_format_copy(&v->input_format, format);
  if(!v->output_open)
    {
    gavl_video_format_copy(&v->output_format, format);
    v->out_palette =
      get_v4l_pixelformat(v->output_format.pixelformat, &v->out_depth);
    
    if(v->out_palette == -1)
      {
      /* Works for sure */
      v->out_palette = VIDEO_PALETTE_YUV420P;
      v->output_format.pixelformat = get_v4l_pixelformat(v->out_palette, &v->out_depth);
      }
    
    }
  v->format_changed = 1;
  }

static void put_frame_ioctl(bg_vloopback_t * v, gavl_video_frame_t * frame)
  {
  struct pollfd pfd;
  uint8_t buffer[MAXIOCTL];
  int len;
  unsigned long ioctl_nr;
  uint8_t * arg;
  int result;
  
  if(v->fd < 0)
    return;
  
  pfd.fd = v->fd;
  pfd.events = POLLIN;

  while(poll(&pfd, 1, 0))
    {
    if(!v->output_open)
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Client connected");
    v->output_open = 1;
    len = read(v->fd, buffer, MAXIOCTL);
    if(len < 8)
      return;
    
    v->output_open = 1;
    
    memcpy(&ioctl_nr, buffer, sizeof(ioctl_nr));
    arg = &buffer[sizeof(unsigned long)];

    if(!ioctl_nr)
      {
      /* Client disconnected */
      v->output_open = 0;
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Client disconnected");
      break;
      }
    
    result = v4l_ioctlhandler(v, ioctl_nr, arg);
    if(result)
      {
      /* new vloopback patch supports a way to return EINVAL to
       * a client. */
      memset(arg, 0xff, MAXIOCTL-sizeof(unsigned long int));
      ioctl(v->fd, VIDIOCSINVALID);
      }
    else if(ioctl(v->fd, ioctl_nr, arg))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "ioctl %lx unsuccessfull.", ioctl_nr);
      //      fprintf(stderr, "vloopback: ioctl %lx unsuccessfull.\n", ioctl_nr);
      }
    if(v->do_grab)
      {
      if(v->format_changed)
        {
        init(v);
        v->format_changed = 0;
        }

      if(v->do_convert)
        gavl_video_convert(v->cnv, frame, v->mmap_frames[v->grab_frame]);
      else
        gavl_video_frame_copy(&v->output_format, v->mmap_frames[v->grab_frame], frame);
      
      v->do_grab = 0;
      }
    }
  
  }

static void put_frame_write(bg_vloopback_t * v, gavl_video_frame_t * frame)
  {
  if(v->format_changed)
    {
    init(v);
    v->format_changed = 0;
    }
  
  if(v->do_convert)
    gavl_video_convert(v->cnv, frame, v->write_frame);
  else
    gavl_video_frame_copy(&v->write_format, v->write_frame, frame);
  write(v->fd, v->write_frame->planes[0], v->write_bytes);
  }

void bg_vloopback_put_frame(bg_vloopback_t * v, gavl_video_frame_t * frame)
  {
  v->put_frame(v, frame);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name        = "device",
      .long_name   = TRS("Device"),
      .type        = BG_PARAMETER_FILE,
      .val_default = { .val_str = "/dev/video1" },
    },
    {
      .name         = "mode",
      .long_name    = TRS("Mode"),
      .type         = BG_PARAMETER_STRINGLIST,
      .val_default  = { .val_str = "ioctl" },
      .multi_names  = (char const *[]){ "write", "ioctl", NULL },
      .multi_labels = (char const *[]){ TRS("Write"), TRS("ioctl"),NULL },
      .help_string = TRS("Set the operating mode. Ioctl is more flexible on the clients side but doesn't work with all applications. Write works with more applications but has a larger overhead"),
      
    },
    {
      .name =      "resolution",
      .long_name = TRS("Resolution"),
      .type =      BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "QVGA (320x240)" },
      .multi_names =     (char const *[]){ "QSIF (160x112)",
                              "QCIF (176x144)", 
                              "QVGA (320x240)", 
                              "SIF(352x240)", 
                              "CIF (352x288)", 
                              "VGA (640x480)", 
                              "User defined",
                              NULL },
      .multi_labels =     (char const *[]){ TRS("QSIF (160x112)"),
                                   TRS("QCIF (176x144)"), 
                                   TRS("QVGA (320x240)"), 
                                   TRS("SIF(352x240)"), 
                                   TRS("CIF (352x288)"), 
                                   TRS("VGA (640x480)"), 
                                   TRS("User defined"),
                                   NULL },
      .help_string = TRS("Fixed resolution for write mode"),
    },
    {
      .name =        "user_width",
      .long_name =   TRS("User defined width"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 720 },
      .val_min =     { .val_i = 160 },
      .val_max =     { .val_i = 1024 },
      .help_string = TRS("User defined width for write mode"),
    },
    {
      .name =        "user_height",
      .long_name =   TRS("User defined height"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 576 },
      .val_min =     { .val_i = 112 },
      .val_max =     { .val_i = 768 },
      .help_string = TRS("User defined height for write mode"),
    },
    {
      .name =      "pixelformat",
      .long_name = TRS("Pixelformat"),
      .type =      BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "yuv420p" },
      .multi_names =     (char const *[]){ "yuv420p", "rgb24",
                                          NULL },
      .multi_labels =     (char const *[]){ TRS("Y'CbCr 4:2:0"),
                                            TRS("RGB (24 bit)"), 
                                            NULL },
      .help_string = TRS("Pixelformat for write mode"),
      
    },
    { /* End of parameters */ }
  };

const bg_parameter_info_t * bg_vloopback_get_parameters(bg_vloopback_t * v)
  {
  return parameters;
  }

void bg_vloopback_set_parameter(void * data, const char * name,
                                const bg_parameter_value_t * val)
  {
  bg_vloopback_t * v = data;

  //  fprintf(stderr, "vloopback_set_parameter %s\n", name); 

  if(!name)
    return;
  else if(!strcmp(name, "device"))
    {
    if(!v->device || strcmp(val->val_str, v->device))
      {
      v->device = bg_strdup(v->device, val->val_str);
      v->changed = 1;
      }
    }
  else if(!strcmp(name, "mode"))
    {
    int new_mode;
    if(!strcmp(val->val_str, "write"))
      new_mode = MODE_WRITE;
    else
      new_mode = MODE_IOCTL;
    if(v->mode != new_mode)
      {
      v->mode = new_mode;
      v->changed = 1;
      }
    }
  else if(!strcmp(name, "resolution"))
    {
    int width = 0, height = 0, custom = 0;
    
    if(!strcmp(val->val_str, "QSIF (160x112)"))
      {
      width  = 160;
      height = 112;
      }
    else if(!strcmp(val->val_str, "QCIF (176x144)"))
      {
      width  = 176;
      height = 144;
      }
    else if(!strcmp(val->val_str, "QVGA (320x240)"))
      {
      width  = 320;
      height = 240;
      }
    else if(!strcmp(val->val_str, "SIF(352x240)"))
      {
      width  = 352;
      height = 240;
      }
    else if(!strcmp(val->val_str, "CIF (352x288)"))
      {
      width  = 352;
      height = 288;
      }
    else if(!strcmp(val->val_str, "VGA (640x480)"))
      {
      width  = 640;
      height = 480;
      }
    else if(!strcmp(val->val_str, "User defined"))
      {
      custom = 1;
      width = v->write_format.image_width;
      height = v->write_format.image_height;
      }
    if((v->write_size_custom != custom) ||
       (v->write_format.image_width != width) ||
       (v->write_format.image_height != height))
      {
      v->write_size_custom = custom;
      v->write_format.image_width = width;
      v->write_format.image_height = height;
      v->changed = 1;
      }
    }
  else if(!strcmp(name, "user_width"))
    {
    if(v->write_width != val->val_i)
      {
      v->write_width = val->val_i;
      v->changed = 1;
      }
    }
  else if(!strcmp(name, "user_height"))
    {
    if(v->write_height != val->val_i)
      {
      v->write_height = val->val_i;
      v->changed = 1;
      }
    }
  else if(!strcmp(name, "pixelformat"))
    {
    if(!strcmp(val->val_str, "yuv420p"))
      v->write_format.pixelformat = GAVL_YUV_420_P;
    else
      v->write_format.pixelformat = GAVL_RGB_24;
    }
  }

int bg_vloopback_changed(bg_vloopback_t * v)
  {
  return v->changed;
  }
