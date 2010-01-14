/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>

#include <linux/videodev.h>

#include <unistd.h>
#include <gmerlin/utils.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "i_v4l"

#include "pwc.h"

/* Debugging stuff */
#if 0

const struct
  {
  int id;
  char const * const name;
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
  FILE * out = stderr;
  for(i = 0; i < sizeof(palette_names)/sizeof(palette_names[0]); i++)
    {
    if(palette == palette_names[i].id)
      {
      fprintf(out, "%s", palette_names[i].name);
      break;
      }
    }
  }

static void dump_video_picture(struct video_picture * p)
  {
  FILE * out = stderr;
  fprintf(out , "Video picture:\n");
  fprintf(out , "  Brightness: %d\n", p->brightness);
  fprintf(out , "  Hue:        %d\n", p->hue);
  fprintf(out , "  Colour:     %d\n", p->colour);
  fprintf(out , "  Contrast:   %d\n", p->contrast);
  fprintf(out , "  Whiteness:  %d\n", p->whiteness);
  fprintf(out , "  Depth:      %d\n", p->depth);
  fprintf(out , "  Palette:    ");
  dump_palette(p->palette);
  fprintf(out , "\n");
  }
#endif

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

/* Input module */

typedef struct
  {
  bg_parameter_info_t * parameters;

  int have_pwc;            /* True is we have a Phillips webcam */
  int have_pwc_parameters; /* True is we have created parameters for
                              a Phillips webcam */

  gavl_video_format_t format;
  
  void * pwc_priv;
  
  int fd;

  struct video_picture pic, cfg_pic;
  struct video_window  win, cfg_win;

  int user_width;
  int user_height;
  int user_resolution;

  struct video_mbuf mbuf;

  struct video_mmap mmap_struct;

  int frame_index;
  uint8_t * mmap_buf;

  gavl_video_frame_t * frame;
  char * device;
  } v4l_t;

static int open_v4l(void * priv,
                    gavl_audio_format_t * audio_format,
                    gavl_video_format_t * format)
  {
  v4l_t * v4l;
  v4l = (v4l_t*)priv;

  /* Open device */

  if((v4l->fd = open(v4l->device, O_RDWR, 0)) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open device %s: %s",
                 v4l->device, strerror(errno));
    goto fail;
    }
  /* Check if we have a Phillips webcam */

  v4l->have_pwc = bg_pwc_probe(v4l->fd);

  if(v4l->have_pwc)
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Phillips webcam detected");
  
  /* Set Picture */
  
  if(ioctl(v4l->fd, VIDIOCGPICT, &(v4l->pic)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOCGPICT failed: %s",
           strerror(errno));
    goto fail;
    }
  format->pixelformat = get_gavl_pixelformat(v4l->pic.palette);
  /* If we have a nonsupported pixelformat we try YUV420 */

  if(format->pixelformat == GAVL_PIXELFORMAT_NONE)
    v4l->pic.palette = get_v4l_pixelformat(GAVL_YUV_420_P);
  
  /* Transfer values */

#if 1
  v4l->pic.brightness = v4l->cfg_pic.brightness;
  v4l->pic.hue        = v4l->cfg_pic.hue;
  v4l->pic.colour     = v4l->cfg_pic.colour;
  v4l->pic.contrast   = v4l->cfg_pic.contrast;
  v4l->pic.whiteness  = v4l->cfg_pic.whiteness;      /* Black and white only */
#endif

  //  dump_video_picture(&(v4l->pic));
  
  if(ioctl(v4l->fd, VIDIOCSPICT, &(v4l->pic)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOCSPICT failed: %s",
                                    strerror(errno));
    goto fail;
    }
  format->pixelformat = get_gavl_pixelformat(v4l->pic.palette);
  
  /* Set window */

  if(ioctl(v4l->fd, VIDIOCGWIN, &(v4l->win)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOCGWIN failed: %s",
           strerror(errno));
    goto fail;
    }
  v4l->win.x = 0;
  v4l->win.y = 0;

  v4l->win.width  = v4l->cfg_win.width;
  v4l->win.height = v4l->cfg_win.height;
  v4l->win.flags = 0;
  
  if(ioctl(v4l->fd, VIDIOCSWIN, &(v4l->win)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOCSWIN failed: %s (invalaid picture dimensions?)",
           strerror(errno));
    goto fail;
    }
  /* Setup format */
      
  format->image_width  = v4l->win.width;
  format->image_height = v4l->win.height;

  format->frame_width  = v4l->win.width;
  format->frame_height = v4l->win.height;

  format->pixel_width  = 1;
  format->pixel_height = 1;

  format->framerate_mode = GAVL_FRAMERATE_VARIABLE;
  format->frame_duration = 0;
  format->timescale = 1;

  gavl_video_format_copy(&v4l->format, format);
  
  
  /* Setup frame */

  gavl_video_frame_set_strides(v4l->frame, format);
  
  /* Setup mmap */
  
  if(ioctl(v4l->fd, VIDIOCGMBUF, &(v4l->mbuf)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOCGMBUF failed: %s",
           strerror(errno));
    goto fail;
    }  
  v4l->mmap_buf = (uint8_t*)(mmap(0, v4l->mbuf.size, PROT_READ|PROT_WRITE,
                                  MAP_SHARED,v4l->fd,0));
  if((unsigned char*)-1 == v4l->mmap_buf)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "mmap failed: %s", strerror(errno));
    goto fail;
    }
  v4l->mmap_struct.width = format->image_width;
  v4l->mmap_struct.height = format->image_height;
  v4l->mmap_struct.format = v4l->pic.palette;

  v4l->frame_index = 0;

  gavl_video_format_copy(&(v4l->format), format);
  
  return 1;
  fail:
  if(v4l->fd >= 0)
    {
    close(v4l->fd);
    v4l->fd = -1;
    }
  return 0;
  
  }

static void close_v4l(void * priv)
  {
  v4l_t * v4l;
  v4l = (v4l_t*)priv;

  if(v4l->fd >= 0)
    {
    close(v4l->fd);
    if(v4l->mmap_buf)
      munmap(v4l->mmap_buf, v4l->mbuf.size);
    }
  v4l->fd = -1;
  }

static int read_frame_v4l(void * priv, gavl_video_frame_t * frame, int stream)
  {
  v4l_t * v4l;
  v4l = (v4l_t*)priv;

  v4l->mmap_struct.frame = v4l->frame_index;
  
  if(ioctl(v4l->fd, VIDIOCMCAPTURE, &(v4l->mmap_struct)))
    return 0;

  if(ioctl(v4l->fd,VIDIOCSYNC, &v4l->frame_index))
    return 0;

  gavl_video_frame_set_planes(v4l->frame,
                              &v4l->format,
                              &v4l->mmap_buf[v4l->mbuf.offsets[v4l->frame_index]]);
  
  gavl_video_frame_copy(&(v4l->format), frame, v4l->frame);
  //  if(ioctl(v4l->fd,VIDIOCSYNC, &(v4l->mmap[v4l->frame_index])))
  
  v4l->frame_index++;
  if(v4l->frame_index >= v4l->mbuf.frames)
    v4l->frame_index = 0;
  
  return 1;
  }

static void * create_v4l()
  {
  v4l_t * v4l;

  v4l = calloc(1, sizeof(*v4l));

  v4l->frame = gavl_video_frame_create(NULL);
  v4l->fd = -1;
  return v4l;
  }

static void  destroy_v4l(void * priv)
  {
  v4l_t * v4l;
  v4l = (v4l_t*)priv;
  gavl_video_frame_null(v4l->frame);
  gavl_video_frame_destroy(v4l->frame);
  close_v4l(priv);

  if(v4l->parameters)
    bg_parameter_info_destroy_array(v4l->parameters);
  
  if(v4l->pwc_priv)
    {
    bg_pwc_destroy(v4l->pwc_priv);
    v4l->pwc_priv = (void*)0;
    }
    free(v4l);
  
  }

/* Configuration stuff */

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "device_section",
      .long_name =   TRS("Device"),
      .type =        BG_PARAMETER_SECTION
    },
    {
      .name =        "device",
      .long_name =   TRS("V4L Device"),
      .type =        BG_PARAMETER_DEVICE,
      .val_default = { .val_str = "/dev/video0" },
    },
    {
      .name =        "res",
      .long_name =   TRS("Resolution"),
      .type =        BG_PARAMETER_SECTION,
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
                              (char*)0 },
      .multi_labels =     (char const *[]){ TRS("QSIF (160x112)"),
                                   TRS("QCIF (176x144)"), 
                                   TRS("QVGA (320x240)"), 
                                   TRS("SIF(352x240)"), 
                                   TRS("CIF (352x288)"), 
                                   TRS("VGA (640x480)"), 
                                   TRS("User defined"),
                                   (char*)0 },
    },
    {
      .name =        "user_width",
      .long_name =   TRS("User defined width"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 720 },
      .val_min =     { .val_i = 160 },
      .val_max =     { .val_i = 1024 },
    },
    {
      .name =        "user_height",
      .long_name =   TRS("User defined height"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 576 },
      .val_min =     { .val_i = 112 },
      .val_max =     { .val_i = 768 },
    },
    {
      .name =        "settings",
      .long_name =   TRS("Settings"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
      .name =        "brightness",
      .long_name =   TRS("Brightness"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 40000 },
    },
    {
      .name =        "hue",
      .long_name =   TRS("Hue"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 65535 },
    },
    {
      .name =        "colour",
      .long_name =   TRS("Colour"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 30000 },
    },
    {
      .name =        "contrast",
      .long_name =   TRS("Contrast"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 30000 },
    },
    {
      .name =        "whiteness",
      .long_name =   TRS("Whiteness"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =       BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 65535 },
      .val_default = { .val_i = 30000 },
    },
    { /* End of parameters */ }
  };

static void create_parameters(v4l_t * v4l)
  {
  if(v4l->parameters)
    bg_parameter_info_destroy_array(v4l->parameters);
  v4l->parameters = bg_parameter_info_copy_array(parameters);

#if 0  
  if((v4l->fd < 0) &&
     ((v4l->fd = open(v4l->device, O_RDWR, 0)) >= 0))
    {
    v4l->have_pwc = bg_pwc_probe(v4l->fd);
    close(v4l->fd);
    v4l->fd = -1;
    }
#endif
  
  if(v4l->have_pwc)
    {
    v4l->pwc_priv = bg_pwc_get_parameters(v4l->fd, &(v4l->parameters));
    v4l->have_pwc_parameters = 1;
    }
  }

static const bg_parameter_info_t * get_parameters_v4l(void * priv)
  {
  v4l_t * v4l;
  v4l = (v4l_t*)priv;

  if(!v4l->parameters ||
     (v4l->have_pwc != v4l->have_pwc_parameters))
    {
    create_parameters(v4l);
    }
  
  return v4l->parameters;
  }

static void set_parameter_v4l(void * priv, const char * name,
                              const bg_parameter_value_t * val)
  {
  char * pos;
  v4l_t * v4l;
  v4l = (v4l_t*)priv;

  if(!name)
    {
    if(v4l->user_resolution)
      {
      v4l->cfg_win.width  = v4l->user_width;
      v4l->cfg_win.height = v4l->user_height;
      }
    bg_pwc_set_parameter(v4l->fd, v4l->pwc_priv, NULL, val);
    return;
    }
  
  if(!strncmp(name, "pwc_", 4))
    {
    bg_pwc_set_parameter(v4l->fd, v4l->pwc_priv, name, val);
    }
  else if(!strcmp(name, "resolution"))
    {
    if(!strcmp(val->val_str, "QSIF (160x112)"))
      {
      v4l->cfg_win.width  = 160;
      v4l->cfg_win.height = 112;
      v4l->user_resolution = 0;
      }
    else if(!strcmp(val->val_str, "QCIF (176x144)"))
      {
      v4l->cfg_win.width  = 176;
      v4l->cfg_win.height = 144;
      v4l->user_resolution = 0;
      }
    else if(!strcmp(val->val_str, "QVGA (320x240)"))
      {
      v4l->cfg_win.width  = 320;
      v4l->cfg_win.height = 240;
      v4l->user_resolution = 0;
      }
    else if(!strcmp(val->val_str, "SIF(352x240)"))
      {
      v4l->cfg_win.width  = 352;
      v4l->cfg_win.height = 240;
      v4l->user_resolution = 0;
      }
    else if(!strcmp(val->val_str, "CIF (352x288)"))
      {
      v4l->cfg_win.width  = 352;
      v4l->cfg_win.height = 288;
      v4l->user_resolution = 0;
      }
    else if(!strcmp(val->val_str, "VGA (640x480)"))
      {
      v4l->cfg_win.width  = 640;
      v4l->cfg_win.height = 480;
      v4l->user_resolution = 0;
      }
    else if(!strcmp(val->val_str, "User defined"))
      {
      v4l->user_resolution = 1;
      }
    }
  else if(!strcmp(name, "device"))
    {
    v4l->device = bg_strdup(v4l->device, val->val_str);
    pos = strchr(v4l->device, ' '); if(pos) *pos = '\0';
    }
  else if(!strcmp(name, "user_width"))
    {
    v4l->user_width = val->val_i;
    }
  else if(!strcmp(name, "user_height"))
    {
    v4l->user_height = val->val_i;
    }
  else if(!strcmp(name, "brightness"))
    {
    v4l->cfg_pic.brightness = val->val_i;

    if(v4l->fd > 0)
      {
      if(ioctl(v4l->fd, VIDIOCGPICT, &(v4l->pic)))
        return;
      v4l->pic.brightness = val->val_i;
      if(ioctl(v4l->fd, VIDIOCSPICT, &(v4l->pic)))
        return;
      }
    }
  else if(!strcmp(name, "hue"))
    {
    v4l->cfg_pic.hue = val->val_i;
    if(v4l->fd > 0)
      {
      if(ioctl(v4l->fd, VIDIOCGPICT, &(v4l->pic)))
        return;
      v4l->pic.hue = val->val_i;
      if(ioctl(v4l->fd, VIDIOCSPICT, &(v4l->pic)))
        return;
      }
    }
  else if(!strcmp(name, "colour"))
    {
    v4l->cfg_pic.colour = val->val_i;
    if(v4l->fd > 0)
      {
      if(ioctl(v4l->fd, VIDIOCGPICT, &(v4l->pic)))
        return;
      v4l->pic.colour = val->val_i;
      if(ioctl(v4l->fd, VIDIOCSPICT, &(v4l->pic)))
        return;
      }
    }
  else if(!strcmp(name, "contrast"))
    {
    v4l->cfg_pic.contrast = val->val_i;
    if(v4l->fd > 0)
      {
      if(ioctl(v4l->fd, VIDIOCGPICT, &(v4l->pic)))
        return;
      v4l->pic.contrast = val->val_i;
      if(ioctl(v4l->fd, VIDIOCSPICT, &(v4l->pic)))
        return;
      }

    }
  else if(!strcmp(name, "whiteness"))
    {
    v4l->cfg_pic.whiteness = val->val_i;
    if(v4l->fd > 0)
      {
      if(ioctl(v4l->fd, VIDIOCGPICT, &(v4l->pic)))
        return;
      v4l->pic.whiteness = val->val_i;
      if(ioctl(v4l->fd, VIDIOCSPICT, &(v4l->pic)))
        return;
      }
    }
  }


const bg_recorder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "i_v4l",
      .long_name =     TRS("V4L"),
      .description =   TRS("video4linux recording plugin. Supports only video and no tuner decives."),
      .type =          BG_PLUGIN_RECORDER_VIDEO,
      .flags =         BG_PLUGIN_RECORDER,
      .priority =      BG_PLUGIN_PRIORITY_MAX,
      .create =        create_v4l,
      .destroy =       destroy_v4l,

      .get_parameters = get_parameters_v4l,
      .set_parameter =  set_parameter_v4l,
    },
    
    .open =       open_v4l,
    .close =      close_v4l,
    .read_video = read_frame_v4l,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
