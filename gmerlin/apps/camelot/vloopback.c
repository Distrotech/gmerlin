
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

#include "vloopback.h"

#define VIDIOCSINVALID _IO('v',BASE_VIDIOCPRIVATE+1)

#define MAX_WIDTH 640
#define MAX_HEIGHT 480
#define MIN_WIDTH 32
#define MIN_HEIGHT 32

#define MAXIOCTL 1024

struct bg_vloopback_s
  {
  int fd;
  gavl_video_converter_t * cnv;
  uint8_t * mmap;
  int mmap_len;

  gavl_video_format_t input_format;
  gavl_video_format_t output_format;
  
  int out_palette;
  
  sigset_t oldset;
  int output_open;
  
  int out_depth;
  
  int do_grab;
  int format_changed;
  gavl_video_frame_t * frame;
  int do_convert;
  };


/* Colorspace translation stuff */

static const struct
  {
  int               v4l;
  gavl_pixelformat_t gavl;
  }
pixelformats[] =
  {
    // VIDEO_PALETTE_GREY /* Linear greyscale */
    // VIDEO_PALETTE_HI240     2       /* High 240 cube (BT848) */
    { VIDEO_PALETTE_RGB565, GAVL_RGB_16 },       /* 565 16 bit RGB */
    { VIDEO_PALETTE_RGB24,  GAVL_BGR_24 },       /* 24bit RGB */
    { VIDEO_PALETTE_RGB32,  GAVL_RGB_32 },       /* 32bit RGB */
    { VIDEO_PALETTE_RGB555, GAVL_RGB_15 },       /* 555 15bit RGB */
    { VIDEO_PALETTE_YUV422, GAVL_YUY2 },  /* YUV422 capture */
    { VIDEO_PALETTE_YUYV,   GAVL_YUY2 },
    { VIDEO_PALETTE_UYVY,   GAVL_UYVY },  /* The great thing about standards is ... */
    // VIDEO_PALETTE_YUV420    10
    // VIDEO_PALETTE_YUV411    11      /* YUV411 capture */
    // VIDEO_PALETTE_RAW       12      /* RAW capture (BT848) */
    { VIDEO_PALETTE_YUV422P, GAVL_YUV_422_P },  /* YUV 4:2:2 Planar */
    { VIDEO_PALETTE_YUV411P,  GAVL_YUV_411_P },  /* YUV 4:1:1 Planar */
    { VIDEO_PALETTE_YUV420P, GAVL_YUV_420_P },  /* YUV 4:2:0 Planar */
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

static int get_v4l_pixelformat(gavl_pixelformat_t csp)
  {
  int i;
  for(i = 0; i < num_pixelformats; i++)
    {
    if(pixelformats[i].gavl == csp)
      return pixelformats[i].v4l;
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

      if(vidchan->channel != 0)
        return EINVAL;
      vidchan->flags = 0;
      vidchan->tuners = 0;
      vidchan->type = VIDEO_TYPE_CAMERA;
      strncpy(vidchan->name, "Camelot", 32);
      return 0;
      }
    case VIDIOCSCHAN:
      {
      int *v = arg;

      if(v[0] != 0)
        return EINVAL;
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
      vidpic->depth = v->out_depth;
      vidpic->palette = v->out_palette;
      return 0;
      }
    case VIDIOCSPICT:
      {
      struct video_picture *vidpic = arg;
      if(vidpic->palette != v->out_palette)
        {
        pfmt = get_gavl_pixelformat(vidpic->palette);
        if(pfmt == GAVL_PIXELFORMAT_NONE)
          {
          fprintf(stderr, "vloopback: unsupported pixel format(%d) is requested.\n",vidpic->palette);
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
      vidwin->flags = 0;
      vidwin->clipcount = 0;
      }

    case VIDIOCSWIN:
      {
      struct video_window *vidwin = arg;

      if(vidwin->width > MAX_WIDTH || vidwin->height > MAX_HEIGHT)
        return EINVAL;
      if(vidwin->width < MIN_WIDTH || vidwin->height < MIN_HEIGHT)
        return EINVAL;
      if(vidwin->flags)
        return EINVAL;
      v->output_format.image_width = vidwin->width;
      v->output_format.image_height = vidwin->height;
      return 0;
      }

    case VIDIOCSYNC:
      {
      int frame = *(int *)arg;
      int ret = 0;

      if(frame < 0 || frame > 0)
        return EINVAL;
      v->do_grab = 0;
      return ret;
      }

    case VIDIOCMCAPTURE:
      {
      struct video_mmap *vidmmap = arg;

      if((vidmmap->width  != v->output_format.image_width) ||
         (vidmmap->height != v->output_format.image_height))
        {
        if(vidmmap->width > MAX_WIDTH || vidmmap->height > MAX_HEIGHT)
          {
          fprintf(stderr, "vloopback: requested capture size is too big(%dx%d).\n",vidmmap->width, vidmmap->height);
          return EINVAL;
          }
        if(vidmmap->width < MIN_WIDTH || vidmmap->height < MIN_HEIGHT)
          {
          fprintf(stderr, "vloopback: requested capture size is to small(%dx%d).\n",vidmmap->width, vidmmap->height);
          return EINVAL;
          }
        v->output_format.image_width = vidmmap->width;
        v->output_format.image_height = vidmmap->height;
        v->format_changed = 1;
        }
      
      if(vidmmap->format != v->out_palette)
        {
        pfmt = get_gavl_pixelformat(vidmmap->format);
        if(pfmt == GAVL_PIXELFORMAT_NONE)
          {
          fprintf(stderr, "vloopback: unsupported pixel format(%d) is requested.\n",vidmmap->format);
          return EINVAL;
          }
        v->out_palette = vidmmap->format;
        v->output_format.pixelformat = pfmt;
        v->format_changed = 1;
        }
      if(vidmmap->frame > 0 || vidmmap->frame < 0)
        return EINVAL;
      v->do_grab = 1;
      return 0;
      }
      
    case VIDIOCGMBUF:
      {
      struct video_mbuf *vidmbuf = arg;

      vidmbuf->size = v->mmap_len;
      vidmbuf->frames = 1;
      vidmbuf->offsets[0] = 0;
      return 0;
      }

    default:
      {
      return EPERM;
      }
    }
  }

static void init(bg_vloopback_t * v)
  {
  gavl_rectangle_i_t out_rect;
  gavl_rectangle_f_t in_rect;
  
  v->frame->strides[0] = 0;

  v->output_format.frame_width = v->output_format.image_width;
  v->output_format.frame_height = v->output_format.image_height;

  gavl_video_frame_set_planes(v->frame,
                              &v->output_format, v->mmap);

  gavl_rectangle_i_set_all(&out_rect, &v->output_format);
  gavl_rectangle_f_set_all(&in_rect, &v->input_format);

  gavl_video_options_set_rectangles(gavl_video_converter_get_options(v->cnv),
                                    &in_rect, &out_rect);
  
  v->do_convert = gavl_video_converter_init(v->cnv, &v->input_format,
                                            &v->output_format);

  fprintf(stderr, "Initializing vloopback, in format:\n");
  gavl_video_format_dump(&v->input_format);
  fprintf(stderr, "Out format:\n");
  gavl_video_format_dump(&v->output_format);
  
  }


bg_vloopback_t * bg_vloopback_create()
  {
  sigset_t newset;
  bg_vloopback_t * ret = calloc(1, sizeof(*ret));
  
  ret->cnv = gavl_video_converter_create();
  ret->frame = gavl_video_frame_create(NULL);
  
  /* Block SIGIO */
  sigemptyset(&newset);
  sigaddset(&newset, SIGIO);
  pthread_sigmask(SIG_BLOCK, &newset, &ret->oldset);
  
  return ret;
  }

void bg_vloopback_destroy(bg_vloopback_t * v)
  {
  /* Restore signal mask */
  pthread_sigmask(SIG_SETMASK, &v->oldset, NULL);

  gavl_video_converter_destroy(v->cnv);
  gavl_video_frame_null(v->frame);
  gavl_video_frame_destroy(v->frame);
  free(v);
  }

int bg_vloopback_open(bg_vloopback_t * v, const char * device)
  {
  v->fd = open(device, O_RDWR);

  if(v->fd < 0)
    {
    fprintf(stderr, "vloopback open failed: %s\n", strerror(errno));
    return 0;
    }
  v->mmap_len = MAX_WIDTH * MAX_HEIGHT * 4;
  
  v->mmap = mmap((void*)0, v->mmap_len, PROT_READ | PROT_WRITE /* required */,
                 MAP_SHARED /* recommended */, v->fd, 0);
  
  if(MAP_FAILED == v->mmap)
    {
    fprintf(stderr, "vloopback mmap failed: %s\n", strerror(errno));
    return 0;
    }
  fprintf(stderr, "Opened vloopback\n");
  return 1;
  }

void bg_vloopback_close(bg_vloopback_t * v)
  {
  close(v->fd);
  v->fd = -1;
  v->output_open = 0;
  }

void bg_vloopback_set_format(bg_vloopback_t * v, const gavl_video_format_t * format)
  {
  gavl_video_format_copy(&v->input_format, format);
  if(!v->output_open)
    {
    gavl_video_format_copy(&v->output_format, format);
    v->out_palette = get_v4l_pixelformat(v->output_format.pixelformat);

    if(v->out_palette == -1)
      {
      /* Works for sure */
      v->out_palette = VIDEO_PALETTE_YUV420P;
      v->output_format.pixelformat = get_v4l_pixelformat(v->out_palette);
      }
    }
  v->format_changed = 1;
  }




void bg_vloopback_put_frame(bg_vloopback_t * v, gavl_video_frame_t * frame)
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
      fprintf(stderr, "Client connected\n");
    v->output_open = 1;
    len = read(v->fd, buffer, MAXIOCTL);
    if(len < 8)
      return;
    
    v->output_open = 1;
    
    ioctl_nr = *(unsigned long*)(buffer);
    arg = &buffer[sizeof(unsigned long)];

    if(!ioctl_nr)
      {
      /* Client disconnected */
      v->output_open = 0;
      fprintf(stderr, "Client disconnected\n");
      break;
      }
    
    result = v4l_ioctlhandler(v, ioctl_nr, arg);
    if(result)
      {
      /* new vloopback patch supports a way to return EINVAL to
       * a client. */
      memset(arg, 0xff, MAXIOCTL-sizeof(unsigned long int));
      fprintf(stderr, "vloopback: ioctl %lx unsuccessfully handled.\n", ioctl_nr);
      ioctl(v->fd, VIDIOCSINVALID);
      }
    if(ioctl(v->fd, ioctl_nr, arg))
      {
      fprintf(stderr, "vloopback: ioctl %lx unsuccessfull.\n", ioctl_nr);
      }
    if(v->do_grab)
      {
      if(v->format_changed)
        {
        init(v);
        v->format_changed = 0;
        }

      if(v->do_convert)
        gavl_video_convert(v->cnv, frame, v->frame);
      else
        gavl_video_frame_copy(&v->output_format, v->frame, frame);
      
      v->do_grab = 0;
      }
    }
  
  }

