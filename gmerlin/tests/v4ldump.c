/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <linux/videodev.h>
#include <unistd.h>
#include <stdio.h>

static void dump_video_capability(struct video_capability * c)
  {
  fprintf(stderr, "Video Capability:\n");
  fprintf(stderr, "  Name: %s\n", c->name);
  fprintf(stderr, "  Type: ");

  if(c->type & VID_TYPE_CAPTURE)       /* Can capture */
    fprintf(stderr, "CAPTURE ");
  if(c->type & VID_TYPE_TUNER)         /* Can tune */
    fprintf(stderr, "TUNER ");
  if(c->type & VID_TYPE_TELETEXT)      /* Does teletext */
    fprintf(stderr, "TELETEXT ");
  if(c->type & VID_TYPE_OVERLAY)       /* Overlay onto frame buffer */
    fprintf(stderr, "OVERLAY ");
  if(c->type & VID_TYPE_CHROMAKEY)     /* Overlay by chromakey */
    fprintf(stderr, "CHROMAKEY ");
  if(c->type & VID_TYPE_CLIPPING)      /* Can clip */
    fprintf(stderr, "CLIPPING ");
  if(c->type & VID_TYPE_FRAMERAM)      /* Uses the frame buffer memory */
    fprintf(stderr, "FRAMERAM ");
  if(c->type & VID_TYPE_SCALES)        /* Scalable */
    fprintf(stderr, "SCALES ");
  if(c->type & VID_TYPE_MONOCHROME)    /* Monochrome only */
    fprintf(stderr, "MONOCHROME ");
  if(c->type & VID_TYPE_SUBCAPTURE)    /* Can capture subareas of the image */
    fprintf(stderr, "SUBCAPTURE ");
  if(c->type & VID_TYPE_MPEG_DECODER)  /* Can decode MPEG streams */
    fprintf(stderr, "MPEG_DECODER ");
  if(c->type & VID_TYPE_MPEG_ENCODER)  /* Can encode MPEG streams */
    fprintf(stderr, "MPEG_ENCODER ");
  if(c->type & VID_TYPE_MJPEG_DECODER) /* Can decode MJPEG streams */
    fprintf(stderr, "MJPEG_DECODER ");
  if(c->type & VID_TYPE_MJPEG_ENCODER) /* Can encode MJPEG streams */
    fprintf(stderr, "MJPEG_ENCODER ");
  fprintf(stderr, "\n");
  fprintf(stderr, "  Channels: %d\n", c->channels);
  fprintf(stderr, "  Audios:   %d\n", c->audios);
  fprintf(stderr, "  Min size: %d x %d\n", c->minwidth, c->minheight);
  fprintf(stderr, "  Max size: %d x %d\n", c->maxwidth, c->maxheight);
  }

struct
  {
  int id;
  char * name;
  }
palette_names[] =
  {
    { VIDEO_PALETTE_GREY,    "Linear greyscale" },
    { VIDEO_PALETTE_HI240,   "High 240 cube (BT848)" },
    { VIDEO_PALETTE_RGB565,  "565 16 bit RGB" },
    { VIDEO_PALETTE_RGB24,   "24bit RGB" },
    { VIDEO_PALETTE_RGB32,   "32bit RGB" },
    { VIDEO_PALETTE_RGB555,  "555 15bit RGB" },
    { VIDEO_PALETTE_YUV422,  "YUV422 capture" },
    { VIDEO_PALETTE_YUYV,    "YUYV" },
    { VIDEO_PALETTE_UYVY,    "UYVY" },
    { VIDEO_PALETTE_YUV420,  "YUV 420" },
    { VIDEO_PALETTE_YUV411,  "YUV 411" },
    { VIDEO_PALETTE_RAW,     "RAW capture (BT848)" },
    { VIDEO_PALETTE_YUV422P, "YUV 4:2:2 Planar" },
    { VIDEO_PALETTE_YUV411P, "YUV 4:1:1 Planar" },
    { VIDEO_PALETTE_YUV420P, "YUV 4:2:0 Planar" },
    { VIDEO_PALETTE_YUV410P, "YUV 4:1:0 Planar" },
  };

static void dump_palette(int palette)
  {
  int i;
  for(i = 0; i < sizeof(palette_names)/sizeof(palette_names[0]); i++)
    {
    if(palette == palette_names[i].id)
      {
      fprintf(stderr, "%s", palette_names[i].name);
      break;
      }
    }
  }

static void dump_video_picture(struct video_picture * p)
  {
  fprintf(stderr , "Video picture:\n");
  fprintf(stderr , "  Brightness: %d\n", p->brightness);
  fprintf(stderr , "  Hue:        %d\n", p->hue);
  fprintf(stderr , "  Colour:     %d\n", p->colour);
  fprintf(stderr , "  Contrast:   %d\n", p->contrast);
  fprintf(stderr , "  Whiteness:  %d\n", p->whiteness);
  fprintf(stderr , "  Depth:      %d\n", p->depth);
  fprintf(stderr , "  Palette:    ");
  dump_palette(p->palette);
  fprintf(stderr , "\n");
  }

static void dump_video_window(struct video_window * win)
  {
  fprintf(stderr, "Video window:\n");
  fprintf(stderr, "  x:         %d\n", win->x);
  fprintf(stderr, "  y:         %d\n", win->y);
  fprintf(stderr, "  width:     %d\n", win->width);
  fprintf(stderr, "  height:    %d\n", win->height);
  fprintf(stderr, "  chromakey: %d\n", win->chromakey);
  fprintf(stderr, "  flags:     %d\n", win->flags);
  }

int main(int argc, char ** argv)
  {
  struct video_capability capability;
  struct video_picture    picture;
  struct video_window     win;
  
  int fd;
  fd = open("/dev/video0", O_RDONLY  | O_NONBLOCK);

  if(fd < 0)
    {
    fprintf(stderr, "Cannot open device\n");
    return -1;
    }

  if(ioctl(fd, VIDIOCGCAP, &capability) < 0)
    {
    fprintf(stderr, "Cannot get capabilities\n");
    return -1;
    }
  dump_video_capability(&capability);

  if(ioctl(fd, VIDIOCGPICT, &picture) < 0)
    {
    fprintf(stderr, "Cannot get picture\n");
    return -1;
    }
  dump_video_picture(&picture);

  if(ioctl(fd, VIDIOCGWIN, &win) < 0)
    {
    fprintf(stderr, "Cannot get window\n");
    return -1;
    }
  dump_video_window(&win);
  
  close(fd);
  return 0;
  }
