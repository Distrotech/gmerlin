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

#include <sys/time.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>

#include <string.h>
#include <malloc.h> // memalign

#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "v4l2"

#include "v4l2_common.h"



static const struct
  {
  gavl_pixelformat_t gavl;
  uint32_t           v4l2;
  }
pixelformats[] =
  {
    /*      Pixel format         FOURCC                        depth  Description  */
    // #define V4L2_PIX_FMT_RGB332  v4l2_fourcc('R','G','B','1') /*  8  RGB-3-3-2     */
    // #define V4L2_PIX_FMT_RGB444  v4l2_fourcc('R','4','4','4') /* 16  xxxxrrrr ggggbbbb */
    // #define V4L2_PIX_FMT_RGB555  v4l2_fourcc('R','G','B','O') /* 16  RGB-5-5-5     */
    // #define V4L2_PIX_FMT_RGB565  v4l2_fourcc('R','G','B','P') /* 16  RGB-5-6-5     */
    // #define V4L2_PIX_FMT_RGB555X v4l2_fourcc('R','G','B','Q') /* 16  RGB-5-5-5 BE  */
    // #define V4L2_PIX_FMT_RGB565X v4l2_fourcc('R','G','B','R') /* 16  RGB-5-6-5 BE  */
    // #define V4L2_PIX_FMT_BGR24   v4l2_fourcc('B','G','R','3') /* 24  BGR-8-8-8     */
    { GAVL_BGR_24, V4L2_PIX_FMT_BGR24 },
    // #define V4L2_PIX_FMT_RGB24   v4l2_fourcc('R','G','B','3') /* 24  RGB-8-8-8     */
    { GAVL_RGB_24, V4L2_PIX_FMT_RGB24 },
    // #define V4L2_PIX_FMT_BGR32   v4l2_fourcc('B','G','R','4') /* 32  BGR-8-8-8-8   */
    { GAVL_BGR_32, V4L2_PIX_FMT_BGR32 },
    // #define V4L2_PIX_FMT_RGB32   v4l2_fourcc('R','G','B','4') /* 32  RGB-8-8-8-8   */
    { GAVL_RGB_32, V4L2_PIX_FMT_RGB32 },
    // #define V4L2_PIX_FMT_GREY    v4l2_fourcc('G','R','E','Y') /*  8  Greyscale     */
    { GAVL_GRAY_8, V4L2_PIX_FMT_GREY },
    // #define V4L2_PIX_FMT_PAL8    v4l2_fourcc('P','A','L','8') /*  8  8-bit palette */
    // #define V4L2_PIX_FMT_YVU410  v4l2_fourcc('Y','V','U','9') /*  9  YVU 4:1:0     */
    { GAVL_YUV_410_P, V4L2_PIX_FMT_YVU410 },
    // #define V4L2_PIX_FMT_YVU420  v4l2_fourcc('Y','V','1','2') /* 12  YVU 4:2:0     */
    { GAVL_YUV_420_P, V4L2_PIX_FMT_YVU420 },
    // #define V4L2_PIX_FMT_YUYV    v4l2_fourcc('Y','U','Y','V') /* 16  YUV 4:2:2     */
    { GAVL_YUY2, V4L2_PIX_FMT_YUYV },
    // #define V4L2_PIX_FMT_UYVY    v4l2_fourcc('U','Y','V','Y') /* 16  YUV 4:2:2     */
    { GAVL_UYVY, V4L2_PIX_FMT_UYVY },
    // #define V4L2_PIX_FMT_YUV422P v4l2_fourcc('4','2','2','P') /* 16  YVU422 planar */
    { GAVL_YUV_422_P, V4L2_PIX_FMT_YUV422P },
    // #define V4L2_PIX_FMT_YUV411P v4l2_fourcc('4','1','1','P') /* 16  YVU411 planar */
    { GAVL_YUV_411_P, V4L2_PIX_FMT_YUV411P },
    // #define V4L2_PIX_FMT_Y41P    v4l2_fourcc('Y','4','1','P') /* 12  YUV 4:1:1     */
    { GAVL_YUV_411_P, V4L2_PIX_FMT_Y41P },
    // #define V4L2_PIX_FMT_YUV444  v4l2_fourcc('Y','4','4','4') /* 16  xxxxyyyy uuuuvvvv */
    // #define V4L2_PIX_FMT_YUV555  v4l2_fourcc('Y','U','V','O') /* 16  YUV-5-5-5     */
    // #define V4L2_PIX_FMT_YUV565  v4l2_fourcc('Y','U','V','P') /* 16  YUV-5-6-5     */
    // #define V4L2_PIX_FMT_YUV32   v4l2_fourcc('Y','U','V','4') /* 32  YUV-8-8-8-8   */

/* two planes -- one Y, one Cr + Cb interleaved  */
    // #define V4L2_PIX_FMT_NV12    v4l2_fourcc('N','V','1','2') /* 12  Y/CbCr 4:2:0  */
    // #define V4L2_PIX_FMT_NV21    v4l2_fourcc('N','V','2','1') /* 12  Y/CrCb 4:2:0  */

/*  The following formats are not defined in the V4L2 specification */
    // #define V4L2_PIX_FMT_YUV410  v4l2_fourcc('Y','U','V','9') /*  9  YUV 4:1:0     */
    // #define V4L2_PIX_FMT_YUV420  v4l2_fourcc('Y','U','1','2') /* 12  YUV 4:2:0     */
    { GAVL_YUV_420_P, V4L2_PIX_FMT_YUV420 },
    
    // #define V4L2_PIX_FMT_YYUV    v4l2_fourcc('Y','Y','U','V') /* 16  YUV 4:2:2     */
    // #define V4L2_PIX_FMT_HI240   v4l2_fourcc('H','I','2','4') /*  8  8-bit color   */
    // #define V4L2_PIX_FMT_HM12    v4l2_fourcc('H','M','1','2') /*  8  YUV 4:2:0 16x16 macroblocks */

/* see http://www.siliconimaging.com/RGB%20Bayer.htm */
    // #define V4L2_PIX_FMT_SBGGR8  v4l2_fourcc('B','A','8','1') /*  8  BGBG.. GRGR.. */

/* compressed formats */
    // #define V4L2_PIX_FMT_MJPEG    v4l2_fourcc('M','J','P','G') /* Motion-JPEG   */
    // #define V4L2_PIX_FMT_JPEG     v4l2_fourcc('J','P','E','G') /* JFIF JPEG     */
    // #define V4L2_PIX_FMT_DV       v4l2_fourcc('d','v','s','d') /* 1394          */
    // #define V4L2_PIX_FMT_MPEG     v4l2_fourcc('M','P','E','G') /* MPEG-1/2/4    */

/*  Vendor-specific formats   */
    // #define V4L2_PIX_FMT_WNVA     v4l2_fourcc('W','N','V','A') /* Winnov hw compress */
    // #define V4L2_PIX_FMT_SN9C10X  v4l2_fourcc('S','9','1','0') /* SN9C10x compression */
    // #define V4L2_PIX_FMT_PWC1     v4l2_fourcc('P','W','C','1') /* pwc older webcam */
    // #define V4L2_PIX_FMT_PWC2     v4l2_fourcc('P','W','C','2') /* pwc newer webcam */
    // #define V4L2_PIX_FMT_ET61X251 v4l2_fourcc('E','6','2','5') /* ET61X251 compression */
    
  };

static const int
num_pixelformats = sizeof(pixelformats)/sizeof(pixelformats[0]);


/* Colorspace translation stuff */

gavl_pixelformat_t bgv4l2_pixelformat_v4l2_2_gavl(int csp)
  {
  int i;
  for(i = 0; i < num_pixelformats; i++)
    {
    if(pixelformats[i].v4l2 == csp)
      return pixelformats[i].gavl;
    }
  return GAVL_PIXELFORMAT_NONE;
  }

uint32_t bgv4l2_pixelformat_gavl_2_v4l2(gavl_pixelformat_t csp)
  {
  int i;
  for(i = 0; i < num_pixelformats; i++)
    {
    if(pixelformats[i].gavl == csp)
      return pixelformats[i].v4l2;
    }
  return 0;
  }

/* ioctl utility */

int bgv4l2_ioctl(int fd, int request, void * arg)
  {
  int r;
  
  do{
    r = ioctl (fd, request, arg);
  } while (-1 == r && EINTR == errno);
  
  return r;
  }


/* Config stuff */

static int append_param(bg_parameter_info_t ** ret, int * num,
                        int fd, struct v4l2_queryctrl * ctrl)
  {
  bg_parameter_info_t * info;

  switch(ctrl->type)
    {
    case V4L2_CTRL_TYPE_INTEGER:
    case V4L2_CTRL_TYPE_INTEGER64:
    case V4L2_CTRL_TYPE_BOOLEAN:
    case V4L2_CTRL_TYPE_BUTTON:
      break;
    case V4L2_CTRL_TYPE_MENU:
    default:
      return 0;
    }
  
  if(ctrl->flags & V4L2_CTRL_FLAG_DISABLED)
    return 0;
  
  *ret = realloc(*ret, ( (*num)+2 ) * sizeof(**ret));
  memset((*ret) + *num, 0, 2 * sizeof(**ret));

  info = &(*ret)[*num];

  info->name = bg_strdup(info->name, (char*)ctrl->name);
  info->long_name = bg_strdup(info->long_name, (char*)ctrl->name);
  info->flags = BG_PARAMETER_SYNC;
  
  switch(ctrl->type)
    {
    case V4L2_CTRL_TYPE_INTEGER:
      if(ctrl->maximum > ctrl->minimum)
        info->type = BG_PARAMETER_SLIDER_INT;
      else
        info->type = BG_PARAMETER_INT;
      info->val_min.val_i = ctrl->minimum;
      info->val_max.val_i = ctrl->maximum;
      info->val_default.val_i = ctrl->default_value;
      break;
    case V4L2_CTRL_TYPE_INTEGER64:
      info->type = BG_PARAMETER_INT;
      break;
    case V4L2_CTRL_TYPE_BOOLEAN:
      info->type = BG_PARAMETER_CHECKBUTTON;
      info->val_default.val_i = ctrl->default_value;
      break;
    case V4L2_CTRL_TYPE_BUTTON:
      info->type = BG_PARAMETER_BUTTON;
      break;
    case V4L2_CTRL_TYPE_MENU:
      info->type = BG_PARAMETER_STRINGLIST;
      break;
    default:
      break;
    }
  *num += 1;
  return 1;
  }

static bg_parameter_info_t * create_card_parameters(int fd)
  {
  int num = 0;
  int i;
  struct v4l2_queryctrl ctrl;
  bg_parameter_info_t * ret = NULL;

  CLEAR(ctrl);
  
  for(i = V4L2_CID_BASE; i < V4L2_CID_LASTP1; i++)
    {
    ctrl.id = i;
    if(bgv4l2_ioctl(fd, VIDIOC_QUERYCTRL, &ctrl) < 0)
      continue;
    append_param(&ret, &num, fd, &ctrl);
    }
  
  i = V4L2_CID_PRIVATE_BASE;
  while(1)
    {
    ctrl.id = i;
    if(bgv4l2_ioctl(fd, VIDIOC_QUERYCTRL, &ctrl) < 0)
      break;
    append_param(&ret, &num, fd, &ctrl);
    i++;
    }
  return ret;
  }


void bgv4l2_create_device_selector(bg_parameter_info_t * info,
                                   int capability)
  {
  int i, fd;
  struct v4l2_capability cap;
  int num_cards = 0;
  char * tmp_string;
  CLEAR(cap);
  
  for(i = 0; i < 64; i++)
    {
    tmp_string = bg_sprintf("/dev/video%d", i);
    
    fd = open(tmp_string, O_RDWR | O_NONBLOCK, 0);
    if(fd < 0)
      {
      free(tmp_string);
      continue;
      }

    if(-1 == bgv4l2_ioctl(fd, VIDIOC_QUERYCAP, &cap))
      {
      close(fd);
      free(tmp_string);
      continue;
      }

    if (!(cap.capabilities & capability))
      {
      close(fd);
      free(tmp_string);
      continue;
      }

    info->multi_names_nc = realloc(info->multi_names_nc, (num_cards + 2)*
                                sizeof(*info->multi_names));

    info->multi_labels_nc = realloc(info->multi_labels_nc, (num_cards + 2)*
                                sizeof(*info->multi_labels));

    info->multi_parameters_nc = realloc(info->multi_parameters_nc, (num_cards + 2)*
                                     sizeof(*info->multi_parameters));
    
    info->multi_names_nc[num_cards] = bg_strdup(NULL, tmp_string);
    info->multi_names_nc[num_cards+1] = NULL;

    info->multi_labels_nc[num_cards] = bg_strdup(NULL, (char*)cap.card);
    info->multi_labels_nc[num_cards+1] = NULL;

    info->multi_parameters_nc[num_cards] = create_card_parameters(fd);
    info->multi_parameters_nc[num_cards+1] = NULL;

    bg_parameter_info_set_const_ptrs(info);

    num_cards++;
    close(fd);
    free(tmp_string);
    }

  
  }

static void append_control(struct v4l2_queryctrl ** ret, int * num,
                           struct v4l2_queryctrl * ctrl)
  {
  *ret = realloc(*ret, ( (*num)+2 ) * sizeof(**ret));
  memcpy((*ret) + *num, ctrl, sizeof(**ret));
  *num += 1;
  }

struct v4l2_queryctrl * bgv4l2_create_device_controls(int fd, int * num)
  {
  int i;
  struct v4l2_queryctrl ctrl;
  struct v4l2_queryctrl * ret = NULL;
  *num = 0;

  CLEAR(ctrl);
  
  for(i = V4L2_CID_BASE; i < V4L2_CID_LASTP1; i++)
    {
    ctrl.id = i;
    if(bgv4l2_ioctl(fd, VIDIOC_QUERYCTRL, &ctrl) < 0)
      continue;
    append_control(&ret, num, &ctrl);
    }
  
  i = V4L2_CID_PRIVATE_BASE;
  while(1)
    {
    ctrl.id = i;
    if(bgv4l2_ioctl(fd, VIDIOC_QUERYCTRL, &ctrl) < 0)
      break;
    append_control(&ret, num, &ctrl);
    i++;
    }
  return ret;
  }


int bgv4l2_set_device_parameter(int fd,
                                struct v4l2_queryctrl * controls,
                                int num_controls,
                                const char * name,
                                const bg_parameter_value_t * val)
  {
  int i;
  struct v4l2_control ctrl;
  
  for(i = 0; i < num_controls; i++)
    {
    if(!strcmp(name, (char*)controls[i].name))
      {
      if(!val)
        {
        // fprintf(stderr, "Set button: %s", name);
        ctrl.value = 0;
        }
      else
        {
        // fprintf(stderr, "Set parameter: %s %d [%d]", name, val->val_i, v4l->controls[i].id);
        ctrl.value = val->val_i;
        }
      ctrl.id = controls[i].id;
      
      if(bgv4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl))
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOC_S_CTRL Failed");
      return 1;
      }
    }
  return 0;
  }


int bgv4l2_get_device_parameter(int fd,
                                struct v4l2_queryctrl * controls,
                                int num_controls,
                                const char * name,
                                bg_parameter_value_t * val)
  {
  int i;
  struct v4l2_control ctrl;
  CLEAR(ctrl);
  
  for(i = 0; i < num_controls; i++)
      {
      if(!strcmp(name, (char*)controls[i].name))
        {
        if(!val)
          return 0;

        ctrl.id = controls[i].id;
        
        //        fprintf(stderr, "Get parameter: %s \n", controls[i].name);
        
        if(!bgv4l2_ioctl(fd, VIDIOC_G_CTRL, &ctrl))
          {
          //          fprintf(stderr, " Success %d\n", ctrl.value);
          val->val_i = ctrl.value;
          return 1;
          }
        else
          bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOC_G_CTRL Failed");
        return 0;
        }
      }
  return 0;
  }

int bgv4l2_open_device(const char * device, int capability,
                       struct v4l2_capability * cap)
  {
  int ret = -1;
  
  /* Open device */
  ret = open(device, O_RDWR /* required */ | O_NONBLOCK, 0);
  
  if(ret < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Opening %s failed: %s",
           device, strerror(errno));
    goto fail;
    }
  
  if (-1 == bgv4l2_ioctl (ret, VIDIOC_QUERYCAP, cap))
    {
    if (EINVAL == errno)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "%s is no V4L2 device",
               device);
      goto fail;
      }
    else
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "VIDIOC_QUERYCAP failed: %s",
             strerror(errno));
      goto fail;
      }
    }

  if (!(cap->capabilities & capability))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "%s is no video %s device",
           device,
           capability == V4L2_CAP_VIDEO_CAPTURE ? "capture" : "output");
    goto fail;
    }
  return ret;
  fail:
  if(ret >= 0)
    close(ret);
  return -1;
  }

int bgv4l2_set_strides(const gavl_video_format_t * gavl,
                       const struct v4l2_format * v4l2, int * ret)
  {
  ret[0] = v4l2->fmt.pix.bytesperline;
  
  if(gavl_pixelformat_is_planar(gavl->pixelformat))
    {
    int sub_h, sub_v;
    gavl_pixelformat_chroma_sub(gavl->pixelformat, &sub_h, &sub_v);
    /* Might crash with odd sizes */
    ret[1] = v4l2->fmt.pix.bytesperline / sub_h;
    ret[2] = v4l2->fmt.pix.bytesperline / sub_h;
    return 3;
    }
  else
    return 1;
  }

gavl_video_frame_t * bgv4l2_create_frame(uint8_t * data, // Can be NULL
                                         const gavl_video_format_t * gavl,
                                         const struct v4l2_format * v4l2)
  {
  gavl_video_frame_t * ret = gavl_video_frame_create(NULL);
  bgv4l2_set_strides(gavl, v4l2, ret->strides);
  
  /* We allocate the frame here because sometimes sizeimage is
     padded to the pagesize */
  if(!data)
    data = memalign(16, v4l2->fmt.pix.sizeimage);
  gavl_video_frame_set_planes(ret, gavl, data);
  return ret;
  }

int bgv4l2_strides_match(const gavl_video_frame_t * f, int * strides, int num_strides)
  {
  int i;
  
  for(i = 0; i < num_strides; i++)
    {
    if(i && (f->planes[i] < f->planes[i-1]))
      return 0;
    
    if(f->strides[i] != strides[i])
      return 0;
    }
  return 1;
  }
