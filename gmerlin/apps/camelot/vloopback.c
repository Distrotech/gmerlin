
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

// #define DUMP_IOCTLS

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
  int grab_frame;
  
  int format_changed;
  gavl_video_frame_t * frames[NUM_FRAMES];

  int do_convert;
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
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCCAPTURE");
#endif
      return 0;
      
    case VIDIOCGPICT:
      {
      struct video_picture *vidpic = arg;
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCGPICT");
#endif
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
#ifdef DUMP_IOCTLS
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "VIDIOCGWIN");
#endif
      vidwin->x = 0;
      vidwin->y = 0;
      vidwin->width = v->output_format.image_width;
      vidwin->height = v->output_format.image_height;
      vidwin->chromakey = 0;
      vidwin->flags = 655360;
      //      vidwin->flags = 0;
      vidwin->clipcount = 0;
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
          bg_log(BG_LOG_WARNING, LOG_DOMAIN, "requested capture size is too big: %dx%d",
                 vidmmap->width, vidmmap->height);
          return EINVAL;
          }
        if(vidmmap->width < MIN_WIDTH || vidmmap->height < MIN_HEIGHT)
          {
          bg_log(BG_LOG_WARNING, LOG_DOMAIN, "requested capture size is too small: %dx%d",
                 vidmmap->width, vidmmap->height);
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
    v->frames[i]->strides[0] = 0;
    gavl_video_frame_set_planes(v->frames[i],
                                &v->output_format, v->mmap + i * MAX_FRAMESIZE);
    }
  
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
    ret->frames[i] = gavl_video_frame_create(NULL);
    }
  ret->cnv = gavl_video_converter_create();
  
  return ret;
  }

void bg_vloopback_destroy(bg_vloopback_t * v)
  {
  int i;
  if(v->fd >= 0)
    bg_vloopback_close(v);

  /* Restore signal mask */
  //  pthread_sigmask(SIG_SETMASK, &v->oldset, NULL);
  
  gavl_video_converter_destroy(v->cnv);

  for(i = 0; i < NUM_FRAMES; i++)
    {
    gavl_video_frame_null(v->frames[i]);
    gavl_video_frame_destroy(v->frames[i]);
    }
  free(v);
  }

int bg_vloopback_open(bg_vloopback_t * v, const char * device)
  {
  
  v->fd = open(device, O_RDWR);

  if(v->fd < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "vloopback open failed: %s",
           strerror(errno));
    return 0;
    }
  v->mmap_len = MAX_FRAMESIZE * NUM_FRAMES;
  
  v->mmap = mmap((void*)0, v->mmap_len, PROT_READ | PROT_WRITE /* required */,
                 MAP_SHARED /* recommended */, v->fd, 0);
  
  if(MAP_FAILED == v->mmap)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "vloopback mmap failed: %s",
           strerror(errno));
    return 0;
    }
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Opened vloopback");
  return 1;
  }

void bg_vloopback_close(bg_vloopback_t * v)
  {
  munmap(v->mmap, v->mmap_len);
  
  close(v->fd);
  v->fd = -1;
  v->output_open = 0;
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Closed vloopback");
  
  }

void bg_vloopback_set_format(bg_vloopback_t * v, const gavl_video_format_t * format)
  {
  gavl_video_format_copy(&v->input_format, format);
  if(!v->output_open)
    {
    gavl_video_format_copy(&v->output_format, format);
    v->out_palette = get_v4l_pixelformat(v->output_format.pixelformat, &v->out_depth);

    if(v->out_palette == -1)
      {
      /* Works for sure */
      v->out_palette = VIDEO_PALETTE_YUV420P;
      v->output_format.pixelformat = get_v4l_pixelformat(v->out_palette, &v->out_depth);
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
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Client connected");
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
    if(ioctl(v->fd, ioctl_nr, arg))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "ioctl %lx unsuccessfull.", ioctl_nr);
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
        gavl_video_convert(v->cnv, frame, v->frames[v->grab_frame]);
      else
        gavl_video_frame_copy(&v->output_format, v->frames[v->grab_frame], frame);
      
      v->do_grab = 0;
      }
    }
  
  }

