/*****************************************************************
 
  i_v4l.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <plugin.h>

#include <linux/videodev.h>

#include <unistd.h>
#include <utils.h>

extern int bg_pwc_probe(int fd);

extern void * bg_pwc_get_parameters(int fd,
                                  bg_parameter_info_t ** parameters);

extern void bg_pwc_destroy(void*);

extern void bg_pwc_set_parameter(int fd, void * priv, char * name,
                                 bg_parameter_value_t * val);

/* Debugging stuff */
#if 0

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
#endif

/* Colorspace translation stuff */

static struct
  {
  int               v4l;
  gavl_colorspace_t gavl;
  }
colorspaces[] =
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

static int num_colorspaces = sizeof(colorspaces)/sizeof(colorspaces[0]);

static gavl_colorspace_t get_gavl_colorspace(int csp)
  {
  int i;
  for(i = 0; i < num_colorspaces; i++)
    {
    if(colorspaces[i].v4l == csp)
      return colorspaces[i].gavl;
    }
  return GAVL_COLORSPACE_NONE;
  }

static int get_v4l_colorspace(gavl_colorspace_t csp)
  {
  int i;
  for(i = 0; i < num_colorspaces; i++)
    {
    if(colorspaces[i].gavl == csp)
      return colorspaces[i].v4l;
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
  gavl_video_format_t format;
  int luma_size;
  int chroma_size;
  char * device;
  int flip;
  char * error_message;
  } v4l_t;

static int open_v4l(void * priv, gavl_video_format_t * format)
  {
  int sub_h, sub_v;
  //  int i;
  v4l_t * v4l;
  v4l = (v4l_t*)priv;
  if(v4l->error_message)
    free(v4l->error_message);

  /* Open device */

  fprintf(stderr, "Opening %s\n", v4l->device);  
  if((v4l->fd = open(v4l->device, O_RDWR, 0)) < 0)
    {
    v4l->error_message = 
      bg_sprintf("Cannot open device %s: %s",
                 v4l->device, strerror(errno));
    goto fail;
    }
  /* Check if we have a Phillips webcam */

  v4l->have_pwc = bg_pwc_probe(v4l->fd);

  if(v4l->have_pwc)
    fprintf(stderr, "Phillips webcam detected\n");
  
  /* Set Picture */
  
  if(ioctl(v4l->fd, VIDIOCGPICT, &(v4l->pic)))
    {
    v4l->error_message = bg_sprintf("VIDIOCGPICT failed: %s",
                                    strerror(errno));
    goto fail;
    }
  format->colorspace = get_gavl_colorspace(v4l->pic.palette);
  /* If we have a nonsupported colorspace we try YUV420 */

  if(format->colorspace == GAVL_COLORSPACE_NONE)
    v4l->pic.palette = get_v4l_colorspace(GAVL_YUV_420_P);
  
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
    v4l->error_message = bg_sprintf("VIDIOCSPICT failed: %s",
                                    strerror(errno));

    goto fail;
    }
  format->colorspace = get_gavl_colorspace(v4l->pic.palette);
  
  /* Set window */

  if(ioctl(v4l->fd, VIDIOCGWIN, &(v4l->win)))
    {
    v4l->error_message = bg_sprintf("VIDIOCGWIN failed: %s",
                                    strerror(errno));

    goto fail;
    }
  v4l->win.x = 0;
  v4l->win.y = 0;

  v4l->win.width  = v4l->cfg_win.width;
  v4l->win.height = v4l->cfg_win.height;

  if(ioctl(v4l->fd, VIDIOCSWIN, &(v4l->win)))
    {
    v4l->error_message = bg_sprintf("VIDIOCSWIN failed: %s",
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

  format->free_framerate = 1;
  format->frame_duration = 1;
  format->timescale = 1;

  /* Setup frame */

  gavl_colorspace_chroma_sub(format->colorspace, &sub_h, &sub_v);
  
  v4l->frame->strides[0] = format->image_width;
  v4l->frame->strides[1] = format->image_width / sub_h;
  v4l->frame->strides[2] = format->image_width / sub_h;

  v4l->luma_size   = format->image_width * format->image_height;
  v4l->chroma_size = v4l->luma_size / (sub_h * sub_v);
  
  /* Setup mmap */
  
  if(ioctl(v4l->fd, VIDIOCGMBUF, &(v4l->mbuf)))
    {
    v4l->error_message = bg_sprintf("VIDIOCGMBUF failed: %s",
                                    strerror(errno));

    goto fail;
    }  
  v4l->mmap_buf = (uint8_t*)(mmap(0, v4l->mbuf.size, PROT_READ|PROT_WRITE,
                                  MAP_SHARED,v4l->fd,0));
  if((unsigned char*)-1 == v4l->mmap_buf)
    {
    v4l->error_message = bg_sprintf("mmap failed");
    goto fail;
    }
  v4l->mmap_struct.width = format->image_width;
  v4l->mmap_struct.height = format->image_height;
  v4l->mmap_struct.format = v4l->pic.palette;

  v4l->frame_index = 0;

  gavl_video_format_copy(&(v4l->format), format);
  
  return 1;
  fail:
  return 0;
  
  }

static void close_v4l(void * priv)
  {
  v4l_t * v4l;
  v4l = (v4l_t*)priv;

  if(v4l->pwc_priv)
    {
    bg_pwc_destroy(v4l->pwc_priv);
    v4l->pwc_priv = (void*)0;
    }
  if(v4l->fd != -1)
    {
    close(v4l->fd);
    if(v4l->mmap_buf)
      munmap(v4l->mmap_buf, v4l->mbuf.size);
    }
  if(v4l->error_message)
    {
    free(v4l->error_message);
    v4l->error_message = (char*)0;
    }
  
  v4l->fd = -1;
  }

static int read_frame_v4l(void * priv, gavl_video_frame_t * frame)
  {
  v4l_t * v4l;
  v4l = (v4l_t*)priv;

  v4l->mmap_struct.frame = v4l->frame_index;
  
  if(ioctl(v4l->fd, VIDIOCMCAPTURE, &(v4l->mmap_struct)))
    return 0;

  if(ioctl(v4l->fd,VIDIOCSYNC, &v4l->frame_index))
    return 0;

  v4l->frame->planes[0] =
    &(v4l->mmap_buf[v4l->mbuf.offsets[v4l->frame_index]]);
  v4l->frame->planes[1] = v4l->frame->planes[0] + v4l->luma_size;
  v4l->frame->planes[2] = v4l->frame->planes[0] + v4l->luma_size + v4l->chroma_size;

  //  memset(v4l->frame->planes[0], 0, v4l->luma_size);
  //  memset(v4l->frame->planes[1], 0, v4l->chroma_size);
  //  memset(v4l->frame->planes[2], 0, v4l->chroma_size);

  if(v4l->flip)
    gavl_video_frame_copy_flip_x(&(v4l->format), frame, v4l->frame);
  else
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
  free(v4l);
  
  }

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "device_section",
      long_name:   "Device",
      type:        BG_PARAMETER_SECTION
    },
    {
      name:        "device",
      long_name:   "V4L Device",
      type:        BG_PARAMETER_DEVICE,
      val_default: { val_str: "/dev/video0" },
    },
    {
      name:        "res",
      long_name:   "Resolution",
      type:        BG_PARAMETER_SECTION,
    },
    {
      name:      "resolution",
      long_name: "Resolution",
      type:      BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "QVGA (320x240)" },
      multi_names:     (char*[]){ "QSIF (160x112)",
                              "QCIF (176x144)", 
                              "QVGA (320x240)", 
                              "SIF(352x240)", 
                              "CIF (352x288)", 
                              "VGA (640x480)", 
                              "User defined",
                              (char*)0 },
    },
    {
      name:        "user_width",
      long_name:   "User defined width",
      type:        BG_PARAMETER_INT,
      val_default: { val_i: 720 },
      val_min:     { val_i: 160 },
      val_max:     { val_i: 1024 },
    },
    {
      name:        "user_height",
      long_name:   "User defined height",
      type:        BG_PARAMETER_INT,
      val_default: { val_i: 576 },
      val_min:     { val_i: 112 },
      val_max:     { val_i: 768 },
    },
    {
      name:        "settings",
      long_name:   "Settings",
      type:        BG_PARAMETER_SECTION,
    },
    {
      name:        "brightness",
      long_name:   "Brightness",
      type:        BG_PARAMETER_SLIDER_INT,
      flags:       BG_PARAMETER_SYNC,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 40000 },
    },
    {
      name:        "hue",
      long_name:   "Hue",
      type:        BG_PARAMETER_SLIDER_INT,
      flags:       BG_PARAMETER_SYNC,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 65535 },
    },
    {
      name:        "colour",
      long_name:   "Colour",
      type:        BG_PARAMETER_SLIDER_INT,
      flags:       BG_PARAMETER_SYNC,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 30000 },
    },
    {
      name:        "contrast",
      long_name:   "Contrast",
      type:        BG_PARAMETER_SLIDER_INT,
      flags:       BG_PARAMETER_SYNC,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 30000 },
    },
    {
      name:        "whiteness",
      long_name:   "Whiteness",
      type:        BG_PARAMETER_SLIDER_INT,
      flags:       BG_PARAMETER_SYNC,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 65535 },
      val_default: { val_i: 30000 },
    },
    {
      name:        "flip",
      long_name:   "Flip Image",
      type:        BG_PARAMETER_CHECKBUTTON,
      flags:       BG_PARAMETER_SYNC,
      val_default: { val_i: 0 },
    },
    { /* End of parameters */ }
  };

static void create_parameters(v4l_t * v4l)
  {
  if(v4l->parameters)
    bg_parameter_info_destroy_array(v4l->parameters);
  v4l->parameters = bg_parameter_info_copy_array(parameters);

  if(v4l->have_pwc)
    {
    v4l->pwc_priv = bg_pwc_get_parameters(v4l->fd, &(v4l->parameters));
    v4l->have_pwc_parameters = 1;
    }
  }

static bg_parameter_info_t * get_parameters_v4l(void * priv)
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

static void set_parameter_v4l(void * priv, char * name,
                       bg_parameter_value_t * val)
  {
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

  fprintf(stderr, "Set parameter %s\n", name);

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
  else if(!strcmp(name, "flip"))
    {
    v4l->flip = val->val_i;
    }
  else if(!strcmp(name, "device"))
    {
    v4l->device = bg_strdup(v4l->device, val->val_str);
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
  //  fprintf(stderr, "cfg pic:\n");
  //  dump_video_picture(&(v4l->cfg_pic));
  }

const char * get_error_v4l(void * priv)
  {
  v4l_t * v4l = (v4l_t*)priv;
  return v4l->error_message;
  }

bg_rv_plugin_t the_plugin =
  {
    common:
    {
      name:          "i_v4l",
      long_name:     "Video4Linux input",
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      type:          BG_PLUGIN_RECORDER_VIDEO,
      flags:         BG_PLUGIN_RECORDER,
      create:        create_v4l,
      destroy:       destroy_v4l,

      get_parameters: get_parameters_v4l,
      set_parameter:  set_parameter_v4l,
      get_error:      get_error_v4l
    },
    
    open:       open_v4l,
    close:      close_v4l,
    read_frame: read_frame_v4l,
    
  };
