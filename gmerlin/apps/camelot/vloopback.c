
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
    { VIDEO_PALETTE_RGB24,  GAVL_RGB_24 },       /* 24bit RGB */
    { VIDEO_PALETTE_RGB32,  GAVL_RGB_32 },       /* 32bit RGB */
    { VIDEO_PALETTE_RGB555, GAVL_RGB_15 },       /* 555 15bit RGB */
    //    VIDEO_PALETTE_YUV422    7       /* YUV422 capture */
    { VIDEO_PALETTE_YUYV,   GAVL_YUY2 },
    { VIDEO_PALETTE_UYVY,   GAVL_UYVY },  /* The great thing about standards is ... */
    // VIDEO_PALETTE_YUV420    10
    // VIDEO_PALETTE_YUV411    11      /* YUV411 capture */
    // VIDEO_PALETTE_RAW       12      /* RAW capture (BT848) */
    { VIDEO_PALETTE_YUV422P, GAVL_YUV_422_P },  /* YUV 4:2:2 Planar */
    // VIDEO_PALETTE_YUV411P   14      /* YUV 4:1:1 Planar */
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


struct bg_vloopback_s
  {
  int fd;
  gavl_video_converter_t * cnv;
  uint8_t * mmap;
  int mmap_len;

  gavl_video_format_t input_format;
  gavl_video_format_t output_format;

  sigset_t oldset;
  int output_open;
  };

bg_vloopback_t * bg_vloopback_create()
  {
  sigset_t newset;
  bg_vloopback_t * ret = calloc(1, sizeof(*ret));
  
  ret->cnv = gavl_video_converter_create();

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
  }

int bg_vloopback_open(bg_vloopback_t * v, const char * device)
  {
  v->fd = open(device, O_RDWR);

  if(v->fd < 0)
    {
    fprintf(stderr, "vloopback open failed: %s\n", strerror(errno));
    return 0;
    }
  v->mmap_len = 640 * 480 * 4;

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
    gavl_video_format_copy(&v->output_format, format);
  }

void bg_vloopback_put_frame(bg_vloopback_t * v, gavl_video_frame_t * frame)
  {
  struct pollfd pfd;
  uint8_t buffer[1032];
  int len;
  unsigned long ioctl_nr;

  if(v->fd < 0)
    return;
  
  pfd.fd = v->fd;
  pfd.events = POLLIN;

  while(poll(&pfd, 1, 0))
    {
    v->output_open = 1;
    len = read(v->fd, buffer, 1032);
    if(len < 8)
      return;
    
    ioctl_nr = *(unsigned long*)(buffer);

    switch(ioctl_nr)
      {
      case VIDIOCGCAP:
        {
        struct video_capability cap;
        fprintf(stderr, "VIDIOCGCAP\n");
        memset(&cap, 0, sizeof(cap));
        cap.type= VID_TYPE_CAPTURE;
        cap.channels = 1;
        cap.audios = 0;
        cap.maxwidth = 640;
        cap.maxheight = 320;
        cap.minwidth=20;
        cap.minheight=20;
        strcpy(cap.name, "Gmerlin loopback");
        if(ioctl(v->fd, VIDIOCGCAP, &cap) < 0)
          fprintf(stderr, "ioctl failed %s\n", strerror(errno));
        }
        break;
      case VIDIOCGCHAN:      /* Get channel info (sources) */
        fprintf(stderr, "VIDIOCGCHAN\n");
        break;
      case VIDIOCSCHAN:      /* Set channel  */
        fprintf(stderr, "VIDIOCSCHAN\n");
        break;
      case VIDIOCGTUNER:     /* Get tuner abilities */
        fprintf(stderr, "VIDIOCGTUNER\n");
        break;
      case VIDIOCSTUNER:     /* Tune the tuner for the current channel */
        fprintf(stderr, "VIDIOCSTUNER\n");
        break;
      case VIDIOCGPICT:      /* Get picture properties */
        {
        struct video_picture pic;
        memset(&pic, 0, sizeof(pic));
        fprintf(stderr, "VIDIOCGPICT\n");
        pic.palette = get_v4l_pixelformat(v->output_format.pixelformat);
        pic.depth = 1;
        if(ioctl(v->fd, VIDIOCGPICT, &pic) < 0)
          fprintf(stderr, "ioctl failed %s\n", strerror(errno));
        }
        break;
      case VIDIOCSPICT:      /* Set picture properties */
        fprintf(stderr, "VIDIOCSPICT\n");
        break;
      case VIDIOCCAPTURE:    /* Start, end capture */
        fprintf(stderr, "VIDIOCCAPTURE\n");
        break;
      case VIDIOCGWIN:       /* Get the video overlay window */
        {
        struct video_window win;
        fprintf(stderr, "VIDIOCGWIN\n");
        memset(&win, 0, sizeof(win));
        win.x = 0;
        win.y = 0;
        win.width  = v->output_format.image_width;
        win.height = v->output_format.image_height;
        if(ioctl(v->fd, VIDIOCGWIN, &win) < 0)
          fprintf(stderr, "ioctl failed %s\n", strerror(errno));
        }
        break;
      case VIDIOCSWIN:       /* Set the video overlay window - passes clip list
                                for hardware smarts , chromakey etc */
        {
        struct video_window * win;
        win = (struct video_window*)(buffer + sizeof(unsigned long));

        fprintf(stderr, "VIDIOCSWIN %dx%d+%d+%d\n",
                win->width, win->height, win->x, win->y);
        if(ioctl(v->fd, VIDIOCSWIN, win) < 0)
          fprintf(stderr, "ioctl failed %s\n", strerror(errno));

        v->output_format.image_width = win->width;
        v->output_format.image_height = win->height;
        }
        break;
      case VIDIOCGFBUF:      /* Get frame buffer */
        fprintf(stderr, "VIDIOCGFBUF\n");
        break;
      case VIDIOCSFBUF:      /* Set frame buffer - root only */
        fprintf(stderr, "VIDIOCSFBUF\n");
        break;
      case VIDIOCKEY:        /* Video key event - to dev 255 is to all -
                                cuts capture on all DMA windows with this
                                key (0xFFFFFFFF == all) */
        fprintf(stderr, "VIDIOCKEY\n");
        break;
      case VIDIOCGFREQ:      /* Set tuner */
        fprintf(stderr, "VIDIOCGFREQ\n");
        break;
      case VIDIOCSFREQ:      /* Set tuner */
        fprintf(stderr, "VIDIOCSFREQ\n");
        break;
      case VIDIOCGAUDIO:     /* Get audio info */
        fprintf(stderr, "VIDIOCGAUDIO\n");
        break;
      case VIDIOCSAUDIO:     /* Audio source, mute etc */
        fprintf(stderr, "VIDIOCSAUDIO\n");
        break;
      case VIDIOCSYNC:       /* Sync with mmap grabbing */
        fprintf(stderr, "VIDIOCSYNC\n");
        break;
      case VIDIOCMCAPTURE:   /* Grab frames */
        fprintf(stderr, "VIDIOCMCAPTURE\n");
        break;
      case VIDIOCGMBUF:      /* Memory map buffer info */
        {
        struct video_mbuf buf;
        fprintf(stderr, "VIDIOCGMBUF\n");
        memset(&buf, 0, sizeof(buf));

        buf.size = v->mmap_len;
        buf.frames = 1;
        buf.offsets[0] = 0;
        
        if(ioctl(v->fd, VIDIOCGMBUF, &buf) < 0)
          fprintf(stderr, "ioctl failed %s\n", strerror(errno));
        }
        break;
      case VIDIOCGUNIT:      /* Get attached units */
        fprintf(stderr, "VIDIOCGUNIT\n");
        break;
      case VIDIOCGCAPTURE:   /* Get subcapture */
        fprintf(stderr, "VIDIOCGCAPTURE\n");
        break;
      case VIDIOCSCAPTURE:   /* Set subcapture */
        fprintf(stderr, "VIDIOCSCAPTURE\n");
        break;
      case VIDIOCSPLAYMODE:  /* Set output video mode/feature */
        fprintf(stderr, "VIDIOCSPLAYMODE\n");
        break;
      case VIDIOCSWRITEMODE: /* Set write mode */
        fprintf(stderr, "VIDIOCSWRITEMODE\n");
        break;
      case VIDIOCGPLAYINFO:  /* Get current playback info from hardware */
        fprintf(stderr, "VIDIOCGPLAYINFO\n");
        break;
      case VIDIOCSMICROCODE: /* Load microcode into hardware */
        fprintf(stderr, "VIDIOCSMICROCODE\n");
        break;
      case VIDIOCGVBIFMT:    /* Get VBI information */
        fprintf(stderr, "VIDIOCGVBIFMT\n");
        break;
      case VIDIOCSVBIFMT:    /* Set VBI information */
        fprintf(stderr, "VIDIOCSVBIFMT\n");
        break;
      default:
        fprintf(stderr, "Unknown ioctl %ld\n", ioctl_nr);
        break;
      }
    }
  }

