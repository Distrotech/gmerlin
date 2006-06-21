/*****************************************************************

  colorspace.c

  Copyright (c) 2001-2002 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <gavl.h>
#include <config.h>
#include <video.h>
#include <colorspace.h>
#include <accel.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct
  {
  gavl_pixelformat_t pixelformat;
  char * name;
  } pixelformat_tab_t;

pixelformat_tab_t pixelformat_tab[] =
  {
    { GAVL_RGB_15, "15 bpp RGB" },
    { GAVL_BGR_15, "15 bpp BGR" },
    { GAVL_RGB_16, "16 bpp RGB" },
    { GAVL_BGR_16, "16 bpp BGR" },
    { GAVL_RGB_24, "24 bpp RGB" },
    { GAVL_BGR_24, "24 bpp BGR" },
    { GAVL_RGB_32, "32 bpp RGB" },
    { GAVL_BGR_32, "32 bpp BGR" },
    { GAVL_RGBA_32, "32 bpp RGBA" },
    { GAVL_RGB_48, "48 bpp RGB" },
    { GAVL_RGBA_64, "64 bpp RGBA" },
    { GAVL_RGB_FLOAT, "Float RGB" },
    { GAVL_RGBA_FLOAT, "Float RGBA" },
    { GAVL_YUY2, "YUV 422 (YUY2)" },
    { GAVL_UYVY, "YUV 422 (UYVY)" },
    { GAVL_YUVA_32, "YUVA 4444" },
    { GAVL_YUV_420_P, "YUV 420 Planar" },
    { GAVL_YUV_410_P, "YUV 410 Planar" },
    { GAVL_YUV_411_P, "YUV 411 Planar" },
    { GAVL_YUV_422_P, "YUV 422 Planar" },
    { GAVL_YUV_422_P_16, "YUV 422 Planar (16 bit)" },
    { GAVL_YUV_444_P, "YUV 444 Planar" },
    { GAVL_YUV_444_P_16, "YUV 444 Planar (16 bit)" },
    { GAVL_YUVJ_420_P, "YUVJ 420 Planar" },
    { GAVL_YUVJ_422_P, "YUVJ 422 Planar" },
    { GAVL_YUVJ_444_P, "YUVJ 444 Planar" },
    { GAVL_PIXELFORMAT_NONE, "Undefined" }
  };

static int num_pixelformats = sizeof(pixelformat_tab)/sizeof(pixelformat_tab_t);

int gavl_pixelformat_num_planes(gavl_pixelformat_t csp)
  {
  switch(csp)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
    case GAVL_RGB_24:
    case GAVL_BGR_24:
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGBA_32:
    case GAVL_YUY2:
    case GAVL_UYVY:
    case GAVL_YUVA_32:
    case GAVL_RGB_48:
    case GAVL_RGBA_64:
    case GAVL_RGB_FLOAT:
    case GAVL_RGBA_FLOAT:
      return 1;
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_444_P:
    case GAVL_YUV_422_P_16:
    case GAVL_YUV_444_P_16:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      return 3;
      break;
    case GAVL_PIXELFORMAT_NONE:
      return 0;
      break;
    }
  return 0;
  }

void gavl_pixelformat_chroma_sub(gavl_pixelformat_t csp, int * sub_h,
                                int * sub_v)
  {
  switch(csp)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
    case GAVL_RGB_24:
    case GAVL_BGR_24:
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGBA_32:
    case GAVL_YUV_444_P:
    case GAVL_YUV_444_P_16:
    case GAVL_YUVJ_444_P:
    case GAVL_YUVA_32:
    case GAVL_RGB_48:
    case GAVL_RGBA_64:
    case GAVL_RGB_FLOAT:
    case GAVL_RGBA_FLOAT:
      *sub_h = 1;
      *sub_v = 1;
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUVJ_420_P:
      *sub_h = 2;
      *sub_v = 2;
      break;
    case GAVL_YUV_422_P:
    case GAVL_YUV_422_P_16:
    case GAVL_YUVJ_422_P:
    case GAVL_YUY2:
    case GAVL_UYVY:
      *sub_h = 2;
      *sub_v = 1;
      break;
    case GAVL_YUV_411_P:
      *sub_h = 4;
      *sub_v = 1;
      break;
    case GAVL_YUV_410_P:
      *sub_h = 4;
      *sub_v = 4;
      break;
    case GAVL_PIXELFORMAT_NONE:
      *sub_h = 0;
      *sub_v = 0;
      break;
    }
  }

int gavl_num_pixelformats()
  {
  return num_pixelformats - 1;
  }

gavl_pixelformat_t gavl_get_pixelformat(int index)
  {
  return pixelformat_tab[index].pixelformat;
  }

const char * gavl_pixelformat_to_string(gavl_pixelformat_t pixelformat)
  {
  int i;
  for(i = 0; i < num_pixelformats; i++)
    {
    if(pixelformat_tab[i].pixelformat == pixelformat)
      return pixelformat_tab[i].name;
    }
  return (const char*)0;
  }

gavl_pixelformat_t gavl_string_to_pixelformat(const char * name)
  {
  int i;
  for(i = 0; i < num_pixelformats; i++)
    {
    if(!strcmp(pixelformat_tab[i].name, name))
      return pixelformat_tab[i].pixelformat;
    }
  return GAVL_PIXELFORMAT_NONE;
  }


static gavl_pixelformat_function_table_t *
create_pixelformat_function_table(const gavl_video_options_t * opt,
                                 int width, int height)
  {
  gavl_pixelformat_function_table_t * csp_tab;
    
  csp_tab =
    calloc(1,sizeof(gavl_pixelformat_function_table_t));

  //  fprintf(stderr, "create_pixelformat_function_table, flags: %08x, q: %d\n",
  //          opt->accel_flags, opt->quality);
  
  /* Standard C-routines, always complete */
  if(!opt->accel_flags || (opt->accel_flags & GAVL_ACCEL_C))
    {
    //    fprintf(stderr, "Init C functions %08x\n", real_accel_flags);
    gavl_init_rgb_rgb_funcs_c(csp_tab, opt);
    gavl_init_rgb_yuv_funcs_c(csp_tab, opt);
    gavl_init_yuv_rgb_funcs_c(csp_tab, opt);
    gavl_init_yuv_yuv_funcs_c(csp_tab, opt);
    }
  
#ifdef HAVE_MMX
  if(opt->accel_flags & GAVL_ACCEL_MMX)
    {
    //    fprintf(stderr, "Init MMX functions %08x\n", real_accel_flags);
    gavl_init_rgb_rgb_funcs_mmx(csp_tab, width, opt);
    gavl_init_rgb_yuv_funcs_mmx(csp_tab, width, opt);
    gavl_init_yuv_yuv_funcs_mmx(csp_tab, width, opt);
    gavl_init_yuv_rgb_funcs_mmx(csp_tab, width, opt);
    }
  if(opt->accel_flags & GAVL_ACCEL_MMXEXT)
    {
    //    fprintf(stderr, "Init MMXEXT functions %08x\n", real_accel_flags);
    gavl_init_rgb_rgb_funcs_mmxext(csp_tab, width, opt);
    gavl_init_rgb_yuv_funcs_mmxext(csp_tab, width, opt);
    gavl_init_yuv_yuv_funcs_mmxext(csp_tab, width, opt);
    gavl_init_yuv_rgb_funcs_mmxext(csp_tab, width, opt);
    }
#endif

  /* High quality */
  
  if((!opt->accel_flags && (opt->quality > 3)) ||
     (opt->accel_flags & GAVL_ACCEL_C_HQ))
    {
    //    fprintf(stderr, "Init HQ\n");
    gavl_init_rgb_rgb_funcs_hq(csp_tab, opt);
    gavl_init_rgb_yuv_funcs_hq(csp_tab, opt);
    gavl_init_yuv_rgb_funcs_hq(csp_tab, opt);
    gavl_init_yuv_yuv_funcs_hq(csp_tab, opt);
    }


  return csp_tab;
  }

gavl_video_func_t
gavl_find_pixelformat_converter(const gavl_video_options_t * opt,
                               gavl_pixelformat_t input_pixelformat,
                               gavl_pixelformat_t output_pixelformat,
                               int width,
                               int height)
  {
  gavl_video_func_t ret = NULL;
  gavl_pixelformat_function_table_t * tab =
    create_pixelformat_function_table(opt, width, height);

  switch(input_pixelformat)
    {
    case GAVL_RGB_15:
      switch(output_pixelformat)
        {
        case GAVL_BGR_15:
          ret = tab->swap_rgb_15;
          break;
        case GAVL_RGB_16:
          ret = tab->rgb_15_to_16;
          break;
        case GAVL_BGR_16:
          ret = tab->rgb_15_to_16_swap;
          break;
        case GAVL_RGB_24:
          ret = tab->rgb_15_to_24;
          break;
        case GAVL_BGR_24:
          ret = tab->rgb_15_to_24_swap;
          break;
        case GAVL_RGB_32:
          ret = tab->rgb_15_to_32;
          break;
        case GAVL_BGR_32:
          ret = tab->rgb_15_to_32_swap;
          break;
        case GAVL_RGBA_32:
          ret = tab->rgb_15_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgb_15_to_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->rgb_15_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgb_15_to_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->rgb_15_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->rgb_15_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgb_15_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->rgb_15_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->rgb_15_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->rgb_15_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->rgb_15_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->rgb_15_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->rgb_15_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgb_15_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->rgb_15_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->rgb_15_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->rgb_15_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->rgb_15_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGB_15:
          break;
        }
      break;
    case GAVL_BGR_15:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->swap_rgb_15;
          break;
        case GAVL_RGB_16:
          ret = tab->rgb_15_to_16_swap;
          break;
        case GAVL_BGR_16:
          ret = tab->rgb_15_to_16;
          break;
        case GAVL_RGB_24:
          ret = tab->rgb_15_to_24_swap;
          break;
        case GAVL_BGR_24:
          ret = tab->rgb_15_to_24;
          break;
        case GAVL_RGB_32:
          ret = tab->rgb_15_to_32_swap;
          break;
        case GAVL_BGR_32:
          ret = tab->rgb_15_to_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->bgr_15_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgb_15_to_48_swap;
          break;
        case GAVL_RGBA_64:
          ret = tab->bgr_15_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgb_15_to_float_swap;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->bgr_15_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->bgr_15_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->bgr_15_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->bgr_15_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->bgr_15_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->bgr_15_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->bgr_15_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->bgr_15_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->bgr_15_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->bgr_15_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->bgr_15_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->bgr_15_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->bgr_15_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->bgr_15_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_BGR_15:
          break;
        }
      break;
    case GAVL_RGB_16:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->rgb_16_to_15;
          break;
        case GAVL_BGR_15:
          ret = tab->rgb_16_to_15_swap;
          break;
        case GAVL_BGR_16:
          ret = tab->swap_rgb_16;
          break;
        case GAVL_RGB_24:
          ret = tab->rgb_16_to_24;
          break;
        case GAVL_BGR_24:
          ret = tab->rgb_16_to_24_swap;
          break;
        case GAVL_RGB_32:
          ret = tab->rgb_16_to_32;
          break;
        case GAVL_BGR_32:
          ret = tab->rgb_16_to_32_swap;
          break;
        case GAVL_RGBA_32:
          ret = tab->rgb_16_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgb_16_to_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->rgb_16_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgb_16_to_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->rgb_16_to_rgba_float;
          break;

        case GAVL_YUY2:
          ret = tab->rgb_16_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgb_16_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->rgb_16_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->rgb_16_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->rgb_16_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->rgb_16_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->rgb_16_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->rgb_16_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgb_16_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->rgb_16_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->rgb_16_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->rgb_16_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->rgb_16_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGB_16:
          break;
        }
      break;
    case GAVL_BGR_16:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->rgb_16_to_15_swap;
          break;
        case GAVL_BGR_15:
          ret = tab->rgb_16_to_15;
          break;
        case GAVL_RGB_16:
          ret = tab->swap_rgb_16;
          break;
        case GAVL_RGB_24:
          ret = tab->rgb_16_to_24_swap;
          break;
        case GAVL_BGR_24:
          ret = tab->rgb_16_to_24;
          break;
        case GAVL_RGB_32:
          ret = tab->rgb_16_to_32_swap;
          break;
        case GAVL_BGR_32:
          ret = tab->rgb_16_to_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->bgr_16_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgb_16_to_48_swap;
          break;
        case GAVL_RGBA_64:
          ret = tab->bgr_16_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgb_16_to_float_swap;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->bgr_16_to_rgba_float;
          break;

        case GAVL_YUY2:
          ret = tab->bgr_16_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->bgr_16_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->bgr_16_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->bgr_16_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->bgr_16_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->bgr_16_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->bgr_16_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->bgr_16_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->bgr_16_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->bgr_16_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->bgr_16_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->bgr_16_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->bgr_16_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_BGR_16:
          break;
        }
      break;
    case GAVL_RGB_24:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->rgb_24_to_15;
          break;
        case GAVL_BGR_15:
          ret = tab->rgb_24_to_15_swap;
          break;
        case GAVL_RGB_16:
          ret = tab->rgb_24_to_16;
          break;
        case GAVL_BGR_16:
          ret = tab->rgb_24_to_16_swap;
          break;
        case GAVL_BGR_24:
          ret = tab->swap_rgb_24;
          break;
        case GAVL_RGB_32:
          ret = tab->rgb_24_to_32;
          break;
        case GAVL_BGR_32:
          ret = tab->rgb_24_to_32_swap;
          break;
        case GAVL_RGBA_32:
          ret = tab->rgb_24_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgb_24_to_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->rgb_24_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgb_24_to_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->rgb_24_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->rgb_24_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgb_24_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->rgb_24_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->rgb_24_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->rgb_24_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->rgb_24_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->rgb_24_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->rgb_24_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgb_24_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->rgb_24_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->rgb_24_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->rgb_24_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->rgb_24_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGB_24:
          break;
        }
      break;
    case GAVL_BGR_24:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->rgb_24_to_15_swap;
          break;
        case GAVL_BGR_15:
          ret = tab->rgb_24_to_15;
          break;
        case GAVL_RGB_16:
          ret = tab->rgb_24_to_16_swap;
          break;
        case GAVL_BGR_16:
          ret = tab->rgb_24_to_16;
          break;
        case GAVL_RGB_24:
          ret = tab->swap_rgb_24;
          break;
        case GAVL_RGB_32:
          ret = tab->rgb_24_to_32_swap;
          break;
        case GAVL_BGR_32:
          ret = tab->rgb_24_to_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->bgr_24_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgb_24_to_48_swap;
          break;
        case GAVL_RGBA_64:
          ret = tab->bgr_24_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgb_24_to_float_swap;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->bgr_24_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->bgr_24_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->bgr_24_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->bgr_24_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->bgr_24_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->bgr_24_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->bgr_24_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->bgr_24_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->bgr_24_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->bgr_24_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->bgr_24_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->bgr_24_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->bgr_24_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->bgr_24_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_BGR_24:
          break;
        }
      break;
    case GAVL_RGB_32:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->rgb_32_to_15;
          break;
        case GAVL_BGR_15:
          ret = tab->rgb_32_to_15_swap;
          break;
        case GAVL_RGB_16:
          ret = tab->rgb_32_to_16;
          break;
        case GAVL_BGR_16:
          ret = tab->rgb_32_to_16_swap;
          break;
        case GAVL_RGB_24:
          ret = tab->rgb_32_to_24;
          break;
        case GAVL_BGR_24:
          ret = tab->rgb_32_to_24_swap;
          break;
        case GAVL_BGR_32:
          ret = tab->swap_rgb_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->rgb_32_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgb_32_to_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->rgb_32_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgb_32_to_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->rgb_32_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->rgb_32_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgb_32_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->rgb_32_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->rgb_32_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->rgb_32_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->rgb_32_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->rgb_32_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->rgb_32_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgb_32_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->rgb_32_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->rgb_32_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->rgb_32_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->rgb_32_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGB_32:
          break;
        }
      break;
    case GAVL_BGR_32:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->rgb_32_to_15_swap;
          break;
        case GAVL_BGR_15:
          ret = tab->rgb_32_to_15;
          break;
        case GAVL_RGB_16:
          ret = tab->rgb_32_to_16_swap;
          break;
        case GAVL_BGR_16:
          ret = tab->rgb_32_to_16;
          break;
        case GAVL_RGB_24:
          ret = tab->rgb_32_to_24_swap;
          break;
        case GAVL_BGR_24:
          ret = tab->rgb_32_to_24;
          break;
        case GAVL_RGB_32:
          ret = tab->swap_rgb_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->bgr_32_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgb_32_to_48_swap;
          break;
        case GAVL_RGBA_64:
          ret = tab->bgr_32_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgb_32_to_float_swap;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->bgr_32_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->bgr_32_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->bgr_32_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->bgr_32_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->bgr_32_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->bgr_32_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->bgr_32_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->bgr_32_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->bgr_32_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->bgr_32_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->bgr_32_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->bgr_32_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->bgr_32_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->bgr_32_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_BGR_32:
          break;
        }
      break;
    case GAVL_RGBA_32:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->rgba_32_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->rgba_32_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->rgba_32_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->rgba_32_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->rgba_32_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->rgba_32_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->rgba_32_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->rgba_32_to_bgr_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgba_32_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->rgba_32_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgba_32_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->rgba_32_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->rgba_32_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgba_32_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->rgba_32_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->rgba_32_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->rgba_32_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->rgba_32_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->rgba_32_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->rgba_32_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgba_32_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->rgba_32_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->rgba_32_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->rgba_32_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->rgba_32_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGBA_32:
          break;
        }
      break;

    case GAVL_RGBA_64:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->rgba_64_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->rgba_64_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->rgba_64_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->rgba_64_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->rgba_64_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->rgba_64_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->rgba_64_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->rgba_64_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->rgba_64_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgba_64_to_rgb_48;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgba_64_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->rgba_64_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->rgba_64_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgba_64_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->rgba_64_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->rgba_64_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->rgba_64_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->rgba_64_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->rgba_64_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->rgba_64_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgba_64_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->rgba_64_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->rgba_64_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->rgba_64_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->rgba_64_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGBA_64:
          break;
        }
      break;

    case GAVL_RGBA_FLOAT:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->rgba_float_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->rgba_float_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->rgba_float_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->rgba_float_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->rgba_float_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->rgba_float_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->rgba_float_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->rgba_float_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->rgba_float_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgba_float_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->rgba_float_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgba_float_to_rgb_float;
          break;
        case GAVL_YUY2:
          ret = tab->rgba_float_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgba_float_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->rgba_float_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->rgba_float_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->rgba_float_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->rgba_float_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->rgba_float_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->rgba_float_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgba_float_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->rgba_float_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->rgba_float_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->rgba_float_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->rgba_float_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGBA_FLOAT:
          break;
        }
      break;

      
    case GAVL_RGB_48:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->rgb_48_to_15;
          break;
        case GAVL_BGR_15:
          ret = tab->rgb_48_to_15_swap;
          break;
        case GAVL_RGB_16:
          ret = tab->rgb_48_to_16;
          break;
        case GAVL_BGR_16:
          ret = tab->rgb_48_to_16_swap;
          break;
        case GAVL_RGB_24:
          ret = tab->rgb_48_to_24;
          break;
        case GAVL_BGR_24:
          ret = tab->rgb_48_to_24_swap;
          break;
        case GAVL_RGB_32:
          ret = tab->rgb_48_to_32;
          break;
        case GAVL_BGR_32:
          ret = tab->rgb_48_to_32_swap;
          break;
        case GAVL_RGBA_32:
          ret = tab->rgb_48_to_rgba_32;
          break;
        case GAVL_RGBA_64:
          ret = tab->rgb_48_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->rgb_48_to_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->rgb_48_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->rgb_48_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgb_48_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->rgb_48_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->rgb_48_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->rgb_48_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->rgb_48_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->rgb_48_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->rgb_48_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgb_48_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->rgb_48_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->rgb_48_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->rgb_48_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->rgb_48_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGB_48:
          break;
        }
      break;
    case GAVL_RGB_FLOAT:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->rgb_float_to_15;
          break;
        case GAVL_BGR_15:
          ret = tab->rgb_float_to_15_swap;
          break;
        case GAVL_RGB_16:
          ret = tab->rgb_float_to_16;
          break;
        case GAVL_BGR_16:
          ret = tab->rgb_float_to_16_swap;
          break;
        case GAVL_RGB_24:
          ret = tab->rgb_float_to_24;
          break;
        case GAVL_BGR_24:
          ret = tab->rgb_float_to_24_swap;
          break;
        case GAVL_RGB_32:
          ret = tab->rgb_float_to_32;
          break;
        case GAVL_BGR_32:
          ret = tab->rgb_float_to_32_swap;
          break;
        case GAVL_RGBA_32:
          ret = tab->rgb_float_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->rgb_float_to_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->rgb_float_to_rgba_64;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->rgb_float_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->rgb_float_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgb_float_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->rgb_float_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->rgb_float_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->rgb_float_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->rgb_float_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->rgb_float_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->rgb_float_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgb_float_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->rgb_float_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->rgb_float_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->rgb_float_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->rgb_float_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGB_FLOAT:
          break;
        }
      break;

      

    case GAVL_YUY2:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuy2_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuy2_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuy2_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuy2_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuy2_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuy2_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuy2_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuy2_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuy2_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuy2_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuy2_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuy2_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuy2_to_rgba_float;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuy2_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuy2_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuy2_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->yuy2_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuy2_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuy2_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->yuy2_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->yuy2_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->yuy2_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuy2_to_yuvj_444_p;
          break;
        case GAVL_UYVY:
          ret = tab->uyvy_to_yuy2;
          break;
        case GAVL_YUVA_32:
          ret = tab->yuy2_to_yuva_32;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUY2:
          break;
        }
      break;
    case GAVL_UYVY:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->uyvy_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->uyvy_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->uyvy_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->uyvy_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->uyvy_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->uyvy_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->uyvy_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->uyvy_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->uyvy_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->uyvy_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->uyvy_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->uyvy_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->uyvy_to_rgba_float;
          break;
        case GAVL_YUV_420_P:
          ret = tab->uyvy_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->uyvy_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->uyvy_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->uyvy_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->uyvy_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->uyvy_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->uyvy_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->uyvy_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->uyvy_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->uyvy_to_yuvj_444_p;
          break;
        case GAVL_YUY2:
          ret = tab->uyvy_to_yuy2;
          break;
        case GAVL_YUVA_32:
          ret = tab->uyvy_to_yuva_32;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_UYVY:
          break;
        }
      break;

    case GAVL_YUVA_32:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuva_32_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuva_32_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuva_32_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuva_32_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuva_32_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuva_32_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuva_32_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuva_32_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuva_32_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuva_32_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuva_32_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuva_32_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuva_32_to_rgba_float;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuva_32_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuva_32_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuva_32_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->yuva_32_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuva_32_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuva_32_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->yuva_32_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->yuva_32_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->yuva_32_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuva_32_to_yuvj_444_p;
          break;
        case GAVL_YUY2:
          ret = tab->yuva_32_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuva_32_to_uyvy;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUVA_32:
          break;
        }
      break;


    case GAVL_YUV_420_P:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuv_420_p_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuv_420_p_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuv_420_p_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuv_420_p_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuv_420_p_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuv_420_p_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuv_420_p_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuv_420_p_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuv_420_p_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuv_420_p_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuv_420_p_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuv_420_p_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuv_420_p_to_rgba_float;
          break;

        case GAVL_YUY2:
          ret = tab->yuv_420_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_420_p_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->yuv_420_p_to_yuva_32;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuv_420_p_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuv_420_p_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->yuv_420_p_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuv_420_p_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuv_420_p_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->yuv_420_p_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->yuv_420_p_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->yuv_420_p_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuv_420_p_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUV_420_P:
          break;
        }
      break;
    case GAVL_YUV_410_P:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuv_410_p_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuv_410_p_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuv_410_p_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuv_410_p_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuv_410_p_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuv_410_p_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuv_410_p_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuv_410_p_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuv_410_p_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuv_410_p_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuv_410_p_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuv_410_p_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuv_410_p_to_rgba_float;
          break;

        case GAVL_YUY2:
          ret = tab->yuv_410_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_410_p_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->yuv_410_p_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuv_410_p_to_yuv_420_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuv_410_p_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->yuv_410_p_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuv_410_p_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuv_410_p_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->yuv_410_p_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->yuv_410_p_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->yuv_410_p_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuv_410_p_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUV_410_P:
          break;
        }
      break;
    case GAVL_YUV_422_P:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuv_422_p_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuv_422_p_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuv_422_p_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuv_422_p_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuv_422_p_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuv_422_p_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuv_422_p_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuv_422_p_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuv_422_p_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuv_422_p_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuv_422_p_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuv_422_p_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuv_422_p_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->yuv_422_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_422_p_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->yuv_422_p_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuv_422_p_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuv_422_p_to_yuv_410_p;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuv_422_p_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuv_422_p_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->yuv_422_p_to_yuv_444_p_16;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->yuv_422_p_to_yuv_422_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->yuv_422_p_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->yuv_422_p_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuv_422_p_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUV_422_P:
          break;
        }
      break;

    case GAVL_YUV_422_P_16:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuv_422_p_16_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuv_422_p_16_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuv_422_p_16_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuv_422_p_16_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuv_422_p_16_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuv_422_p_16_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuv_422_p_16_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuv_422_p_16_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuv_422_p_16_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuv_422_p_16_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuv_422_p_16_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuv_422_p_16_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuv_422_p_16_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->yuv_422_p_16_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_422_p_16_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->yuv_422_p_16_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuv_422_p_16_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuv_422_p_16_to_yuv_410_p;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuv_422_p_16_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuv_422_p_16_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->yuv_422_p_16_to_yuv_444_p_16;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuv_422_p_16_to_yuv_422_p;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->yuv_422_p_16_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->yuv_422_p_16_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuv_422_p_16_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUV_422_P_16:
          break;
        }
      break;

    case GAVL_YUV_411_P:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuv_411_p_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuv_411_p_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuv_411_p_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuv_411_p_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuv_411_p_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuv_411_p_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuv_411_p_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuv_411_p_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuv_411_p_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuv_411_p_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuv_411_p_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuv_411_p_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuv_411_p_to_rgba_float;
          break;

        case GAVL_YUY2:
          ret = tab->yuv_411_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_411_p_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->yuv_411_p_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuv_411_p_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuv_411_p_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuv_411_p_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->yuv_411_p_to_yuv_422_p_16;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuv_411_p_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->yuv_411_p_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->yuv_411_p_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->yuv_411_p_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuv_411_p_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUV_411_P:
          break;
        }
      break;
    case GAVL_YUV_444_P:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuv_444_p_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuv_444_p_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuv_444_p_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuv_444_p_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuv_444_p_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuv_444_p_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuv_444_p_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuv_444_p_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuv_444_p_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuv_444_p_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuv_444_p_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuv_444_p_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuv_444_p_to_rgba_float;
          break;
         
        case GAVL_YUY2:
          ret = tab->yuv_444_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_444_p_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->yuv_444_p_to_yuva_32;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuv_444_p_to_yuv_410_p;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuv_444_p_to_yuv_420_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuv_444_p_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->yuv_444_p_to_yuv_422_p_16;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->yuv_444_p_to_yuv_444_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuv_444_p_to_yuv_411_p;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->yuv_444_p_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->yuv_444_p_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuv_444_p_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUV_444_P:
          break;
        }
      break;

    case GAVL_YUV_444_P_16:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuv_444_p_16_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuv_444_p_16_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuv_444_p_16_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuv_444_p_16_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuv_444_p_16_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuv_444_p_16_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuv_444_p_16_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuv_444_p_16_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuv_444_p_16_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuv_444_p_16_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuv_444_p_16_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuv_444_p_16_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuv_444_p_16_to_rgba_float;
          break;
         
        case GAVL_YUY2:
          ret = tab->yuv_444_p_16_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_444_p_16_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->yuv_444_p_16_to_yuva_32;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuv_444_p_16_to_yuv_410_p;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuv_444_p_16_to_yuv_420_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuv_444_p_16_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->yuv_444_p_16_to_yuv_422_p_16;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuv_444_p_16_to_yuv_444_p;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuv_444_p_16_to_yuv_411_p;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->yuv_444_p_16_to_yuvj_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->yuv_444_p_16_to_yuvj_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuv_444_p_16_to_yuvj_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUV_444_P_16:
          break;
        }
      break;

    case GAVL_YUVJ_420_P:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuvj_420_p_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuvj_420_p_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuvj_420_p_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuvj_420_p_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuvj_420_p_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuvj_420_p_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuvj_420_p_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuvj_420_p_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuvj_420_p_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuvj_420_p_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuvj_420_p_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuvj_420_p_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuvj_420_p_to_rgba_float;
          break;

        case GAVL_YUY2:
          ret = tab->yuvj_420_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuvj_420_p_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->yuvj_420_p_to_yuva_32;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuvj_420_p_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->yuvj_420_p_to_yuv_422_p_16;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuvj_420_p_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->yuvj_420_p_to_yuv_444_p_16;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuvj_420_p_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuvj_420_p_to_yuv_410_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->yuv_420_p_to_yuv_422_p;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuv_420_p_to_yuv_411_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuv_420_p_to_yuv_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUVJ_420_P:
          break;
        }
      break;
    case GAVL_YUVJ_422_P:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuvj_422_p_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuvj_422_p_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuvj_422_p_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuvj_422_p_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuvj_422_p_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuvj_422_p_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuvj_422_p_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuvj_422_p_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuvj_422_p_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuvj_422_p_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuvj_422_p_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuvj_422_p_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuvj_422_p_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->yuvj_422_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuvj_422_p_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->yuvj_422_p_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuvj_422_p_to_yuv_420_p;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuvj_422_p_to_yuv_411_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuvj_422_p_to_yuv_410_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuvj_422_p_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->yuvj_422_p_to_yuv_444_p_16;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->yuv_422_p_to_yuv_420_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuvj_422_p_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->yuvj_422_p_to_yuv_422_p_16;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuv_422_p_to_yuv_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUVJ_422_P:
          break;
        }
      break;
    case GAVL_YUVJ_444_P:
      switch(output_pixelformat)
        {
        case GAVL_RGB_15:
          ret = tab->yuvj_444_p_to_rgb_15;
          break;
        case GAVL_BGR_15:
          ret = tab->yuvj_444_p_to_bgr_15;
          break;
        case GAVL_RGB_16:
          ret = tab->yuvj_444_p_to_rgb_16;
          break;
        case GAVL_BGR_16:
          ret = tab->yuvj_444_p_to_bgr_16;
          break;
        case GAVL_RGB_24:
          ret = tab->yuvj_444_p_to_rgb_24;
          break;
        case GAVL_BGR_24:
          ret = tab->yuvj_444_p_to_bgr_24;
          break;
        case GAVL_RGB_32:
          ret = tab->yuvj_444_p_to_rgb_32;
          break;
        case GAVL_BGR_32:
          ret = tab->yuvj_444_p_to_bgr_32;
          break;
        case GAVL_RGBA_32:
          ret = tab->yuvj_444_p_to_rgba_32;
          break;
        case GAVL_RGB_48:
          ret = tab->yuvj_444_p_to_rgb_48;
          break;
        case GAVL_RGBA_64:
          ret = tab->yuvj_444_p_to_rgba_64;
          break;
        case GAVL_RGB_FLOAT:
          ret = tab->yuvj_444_p_to_rgb_float;
          break;
        case GAVL_RGBA_FLOAT:
          ret = tab->yuvj_444_p_to_rgba_float;
          break;
        case GAVL_YUY2:
          ret = tab->yuvj_444_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuvj_444_p_to_uyvy;
          break;
        case GAVL_YUVA_32:
          ret = tab->yuvj_444_p_to_yuva_32;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuvj_444_p_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuvj_444_p_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuvj_444_p_to_yuv_422_p;
          break;
        case GAVL_YUV_422_P_16:
          ret = tab->yuvj_444_p_to_yuv_422_p_16;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuvj_444_p_to_yuv_411_p;
          break;
        case GAVL_YUVJ_420_P:
          ret = tab->yuv_444_p_to_yuv_420_p;
          break;
        case GAVL_YUVJ_422_P:
          ret = tab->yuv_444_p_to_yuv_422_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuvj_444_p_to_yuv_444_p;
          break;
        case GAVL_YUV_444_P_16:
          ret = tab->yuvj_444_p_to_yuv_444_p_16;
          break;
        /* Keep GCC happy */
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_YUVJ_444_P:
          break;
        }
      break;

      
    case GAVL_PIXELFORMAT_NONE:
      break;
    }
  free(tab);  
  return ret;
  }

/* bytes_per_component is only valid for planar formats */
  
int gavl_pixelformat_bytes_per_component(gavl_pixelformat_t csp)
  {
  switch(csp)
    {
    case GAVL_PIXELFORMAT_NONE:
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
    case GAVL_RGB_24:
    case GAVL_BGR_24:
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGBA_32:
    case GAVL_RGB_48:
    case GAVL_RGBA_64:
    case GAVL_RGB_FLOAT:
    case GAVL_RGBA_FLOAT:
    case GAVL_YUY2:
    case GAVL_UYVY:
    case GAVL_YUVA_32:
      return 0;
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_444_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_410_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      return 1;
      break;
    case GAVL_YUV_444_P_16:
    case GAVL_YUV_422_P_16:
      return 2;
    }
  return 0;
  }

/* bytes_per_pixel is only valid for packed formats */

int gavl_pixelformat_bytes_per_pixel(gavl_pixelformat_t csp)
  {
  switch(csp)
    {
    case GAVL_PIXELFORMAT_NONE:
      return 0;
      break;
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
      return 2;
      break;
    case GAVL_RGB_24:
    case GAVL_BGR_24:
      return 3;
      break;
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGBA_32:
    case GAVL_YUVA_32:
      return 4;
      break;
    case GAVL_RGB_48:
      return 6;
      break;
    case GAVL_RGBA_64:
      return 8;
      break;
    case GAVL_RGB_FLOAT:
      return 3*sizeof(float);
      break;
    case GAVL_RGBA_FLOAT:
      return 4*sizeof(float);
      break;
    case GAVL_YUY2:
    case GAVL_UYVY:
      return 2;
    case GAVL_YUV_420_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_422_P_16:
    case GAVL_YUV_444_P:
    case GAVL_YUV_444_P_16:
    case GAVL_YUV_411_P:
    case GAVL_YUV_410_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      return 0;
    }
  return 0;
  }

/* Check if a pixelformat can be converted by simple scaling */

int gavl_pixelformat_can_scale(gavl_pixelformat_t in_csp, gavl_pixelformat_t out_csp)
  {
    int sub_v_in,  sub_h_in;
  int sub_v_out, sub_h_out;
  if(gavl_pixelformat_is_rgb(in_csp) ||
     gavl_pixelformat_is_rgb(out_csp))
    {
    return 0;
    }

  if(gavl_pixelformat_is_jpeg_scaled(in_csp) !=
     gavl_pixelformat_is_jpeg_scaled(out_csp))
    {
    return 0;
    }
  if(gavl_pixelformat_has_alpha(in_csp) !=
     gavl_pixelformat_has_alpha(out_csp))
    {
    return 0;
    }


  
  gavl_pixelformat_chroma_sub(in_csp, &sub_h_in, &sub_v_in);
  gavl_pixelformat_chroma_sub(out_csp, &sub_h_out, &sub_v_out);
  
  if((sub_h_in == sub_h_out) && (sub_v_in == sub_v_out))
    {
    return 0;
    }

  if(!gavl_pixelformat_is_planar(in_csp))
    {
    // fprintf(stderr, "BLUPPPP: %d %d\n", gavl_pixelformat_is_planar(out_csp),
    //         gavl_pixelformat_bytes_per_component(out_csp));
    
    if(gavl_pixelformat_is_planar(out_csp) &&
       (gavl_pixelformat_bytes_per_component(out_csp) == 1))
      return 1;
    else
      return 0;
    }
  else
    {
    if(!gavl_pixelformat_is_planar(out_csp) &&
       (gavl_pixelformat_bytes_per_component(in_csp) == 1))
      return 1;
    else if(gavl_pixelformat_bytes_per_component(in_csp) ==
            gavl_pixelformat_bytes_per_component(out_csp))
      return 1;
    else
      return 0;
    }
  return 0;
  }

/*
 *  Return a pixelformat (or GAVL_PIXELFORMAT_NONE) as an intermediate pixelformat
 *  for which the conversion quality can be improved. E.g. instead of
 *  RGB -> YUV420P, we can do RGB -> YUV444P -> YUV420P with proper chroma scaling
 */

gavl_pixelformat_t gavl_pixelformat_get_intermediate(gavl_pixelformat_t in_csp,
                                                   gavl_pixelformat_t out_csp)
  {
  switch(in_csp)
    {
    case GAVL_PIXELFORMAT_NONE: return GAVL_PIXELFORMAT_NONE; break;
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
    case GAVL_RGB_24:
    case GAVL_BGR_24:
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGBA_32:
    case GAVL_RGB_48:
    case GAVL_RGBA_64:
    case GAVL_RGB_FLOAT:
    case GAVL_RGBA_FLOAT:
    case GAVL_YUVA_32:
      /*4:4:4 -> */
      switch(out_csp)
        {
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGB_15:
        case GAVL_BGR_15:
        case GAVL_RGB_16:
        case GAVL_BGR_16:
        case GAVL_RGB_24:
        case GAVL_BGR_24:
        case GAVL_RGB_32:
        case GAVL_BGR_32:
        case GAVL_RGB_48:
        case GAVL_RGBA_64:
        case GAVL_RGB_FLOAT:
        case GAVL_RGBA_FLOAT:
        case GAVL_RGBA_32:
        case GAVL_YUVA_32:
        case GAVL_YUV_444_P:
        case GAVL_YUVJ_444_P:
        case GAVL_YUV_444_P_16:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_YUY2:
        case GAVL_UYVY:
        case GAVL_YUV_420_P:
        case GAVL_YUV_422_P:
        case GAVL_YUV_411_P:
        case GAVL_YUV_410_P:
          return GAVL_YUV_444_P; break;
        case GAVL_YUVJ_420_P:
        case GAVL_YUVJ_422_P:
          return GAVL_YUVJ_444_P; break;
        case GAVL_YUV_422_P_16:
          return GAVL_YUV_444_P_16; break;
        }
      break;
    case GAVL_YUY2:
    case GAVL_UYVY:
    case GAVL_YUV_422_P:
      switch(out_csp)
        {
        case GAVL_PIXELFORMAT_NONE:
          return GAVL_PIXELFORMAT_NONE; break;
          /* YUV422 -> RGB */
        case GAVL_RGB_15:
        case GAVL_BGR_15:
        case GAVL_RGB_16:
        case GAVL_BGR_16:
        case GAVL_RGB_24:
        case GAVL_BGR_24:
        case GAVL_RGB_32:
        case GAVL_BGR_32:
        case GAVL_RGB_48:
        case GAVL_RGBA_64:
        case GAVL_RGB_FLOAT:
        case GAVL_RGBA_FLOAT:
        case GAVL_RGBA_32:
        case GAVL_YUVA_32:
          return GAVL_YUV_444_P; break;
        case GAVL_YUV_422_P:
        case GAVL_YUV_422_P_16:
        case GAVL_YUVJ_422_P:
        case GAVL_YUY2:
        case GAVL_UYVY:
        case GAVL_YUV_420_P:
        case GAVL_YUV_411_P:
        case GAVL_YUV_410_P:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_YUV_444_P_16:
          return GAVL_YUV_422_P_16; break;
        case GAVL_YUV_444_P:
          return GAVL_YUV_422_P; break;
        case GAVL_YUVJ_444_P:
          return GAVL_YUVJ_422_P; break;
        case GAVL_YUVJ_420_P:
          return GAVL_YUVJ_422_P; break;
        }
      break;
    case GAVL_YUV_420_P:
      switch(out_csp)
        {
        case GAVL_PIXELFORMAT_NONE:
          return GAVL_PIXELFORMAT_NONE; break;
          /* YUV420 -> RGB */
        case GAVL_RGB_15:
        case GAVL_BGR_15:
        case GAVL_RGB_16:
        case GAVL_BGR_16:
        case GAVL_RGB_24:
        case GAVL_BGR_24:
        case GAVL_RGB_32:
        case GAVL_BGR_32:
        case GAVL_RGB_48:
        case GAVL_RGBA_64:
        case GAVL_RGB_FLOAT:
        case GAVL_RGBA_FLOAT:
        case GAVL_RGBA_32:
        case GAVL_YUVA_32:
          return GAVL_YUV_444_P; break;
        case GAVL_YUV_422_P:
        case GAVL_YUY2:
        case GAVL_UYVY:
        case GAVL_YUV_420_P:
        case GAVL_YUV_411_P:
        case GAVL_YUV_410_P:
        case GAVL_YUV_444_P:
        case GAVL_YUVJ_420_P:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_YUV_422_P_16:
          return GAVL_YUV_422_P; break;
        case GAVL_YUVJ_422_P:
          return GAVL_YUVJ_420_P; break;
        case GAVL_YUV_444_P_16:
          return GAVL_YUV_444_P; break;
        case GAVL_YUVJ_444_P:
          return GAVL_YUVJ_420_P; break;
        }
      break;
    case GAVL_YUV_444_P:
      switch(out_csp)
        {
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGB_15:
        case GAVL_BGR_15:
        case GAVL_RGB_16:
        case GAVL_BGR_16:
        case GAVL_RGB_24:
        case GAVL_BGR_24:
        case GAVL_RGB_32:
        case GAVL_BGR_32:
        case GAVL_RGB_48:
        case GAVL_RGBA_64:
        case GAVL_RGB_FLOAT:
        case GAVL_RGBA_FLOAT:
        case GAVL_RGBA_32:
        case GAVL_YUVA_32:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_YUV_422_P:
        case GAVL_YUY2:
        case GAVL_UYVY:
        case GAVL_YUV_420_P:
        case GAVL_YUV_411_P:
        case GAVL_YUV_410_P:
        case GAVL_YUV_444_P:
        case GAVL_YUVJ_444_P:
        case GAVL_YUV_444_P_16:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_YUV_422_P_16:
          return GAVL_YUV_422_P; break;
        case GAVL_YUVJ_422_P:
          return GAVL_YUVJ_420_P; break;
        case GAVL_YUVJ_420_P:
          return GAVL_YUVJ_444_P; break;
        }
      break;
      
    case GAVL_YUV_411_P:
    case GAVL_YUV_410_P:
      switch(out_csp)
        {
        case GAVL_PIXELFORMAT_NONE:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_RGB_15:
        case GAVL_BGR_15:
        case GAVL_RGB_16:
        case GAVL_BGR_16:
        case GAVL_RGB_24:
        case GAVL_BGR_24:
        case GAVL_RGB_32:
        case GAVL_BGR_32:
        case GAVL_RGB_48:
        case GAVL_RGBA_64:
        case GAVL_RGB_FLOAT:
        case GAVL_RGBA_FLOAT:
        case GAVL_RGBA_32:
        case GAVL_YUVA_32:
          return GAVL_YUV_444_P; break;
        case GAVL_YUV_422_P:
        case GAVL_YUY2:
        case GAVL_UYVY:
        case GAVL_YUV_420_P:
        case GAVL_YUV_411_P:
        case GAVL_YUV_410_P:
        case GAVL_YUV_444_P:
          return GAVL_PIXELFORMAT_NONE; break;

        case GAVL_YUVJ_444_P:
          return GAVL_YUV_444_P; break;
        case GAVL_YUV_422_P_16:
          return GAVL_YUV_422_P; break;
        case GAVL_YUVJ_422_P:
          return GAVL_YUV_422_P; break;
        case GAVL_YUV_444_P_16:
          return GAVL_YUV_444_P; break;
        case GAVL_YUVJ_420_P:
          return GAVL_YUV_420_P; break;
        }
      break;
    case GAVL_YUVJ_420_P:
      switch(out_csp)
        {
        case GAVL_PIXELFORMAT_NONE:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_RGB_15:
        case GAVL_BGR_15:
        case GAVL_RGB_16:
        case GAVL_BGR_16:
        case GAVL_RGB_24:
        case GAVL_BGR_24:
        case GAVL_RGB_32:
        case GAVL_BGR_32:
        case GAVL_RGB_48:
        case GAVL_RGBA_64:
        case GAVL_RGB_FLOAT:
        case GAVL_RGBA_FLOAT:
        case GAVL_RGBA_32:
        case GAVL_YUVA_32:
          return GAVL_YUVJ_444_P; break;
        case GAVL_YUV_422_P:
        case GAVL_YUY2:
        case GAVL_UYVY:
        case GAVL_YUV_411_P:
        case GAVL_YUV_410_P:
        case GAVL_YUV_444_P:
          return GAVL_YUV_420_P; break;
        case GAVL_YUV_420_P:
        case GAVL_YUVJ_444_P:
        case GAVL_YUVJ_422_P:
        case GAVL_YUVJ_420_P:
          return GAVL_PIXELFORMAT_NONE; break;

        case GAVL_YUV_422_P_16:
          return GAVL_YUVJ_422_P; break;
        case GAVL_YUV_444_P_16:
          return GAVL_YUVJ_444_P; break;
        }
      break;
      
    case GAVL_YUVJ_422_P:
      switch(out_csp)
        {
        case GAVL_PIXELFORMAT_NONE:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_RGB_15:
        case GAVL_BGR_15:
        case GAVL_RGB_16:
        case GAVL_BGR_16:
        case GAVL_RGB_24:
        case GAVL_BGR_24:
        case GAVL_RGB_32:
        case GAVL_BGR_32:
        case GAVL_RGB_48:
        case GAVL_RGBA_64:
        case GAVL_RGB_FLOAT:
        case GAVL_RGBA_FLOAT:
        case GAVL_RGBA_32:
        case GAVL_YUVA_32:
          return GAVL_YUVJ_444_P; break;
        case GAVL_YUV_411_P:
        case GAVL_YUV_410_P:
        case GAVL_YUV_444_P:
        case GAVL_YUV_420_P:
          return GAVL_YUV_422_P; break;
        case GAVL_YUV_422_P:
        case GAVL_YUY2:
        case GAVL_UYVY:
        case GAVL_YUVJ_444_P:
        case GAVL_YUVJ_422_P:
        case GAVL_YUVJ_420_P:
        case GAVL_YUV_422_P_16:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_YUV_444_P_16:
          return GAVL_YUV_422_P_16; break;
        }
      break;
    case GAVL_YUVJ_444_P:
      switch(out_csp)
        {
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGB_15:
        case GAVL_BGR_15:
        case GAVL_RGB_16:
        case GAVL_BGR_16:
        case GAVL_RGB_24:
        case GAVL_BGR_24:
        case GAVL_RGB_32:
        case GAVL_BGR_32:
        case GAVL_RGB_48:
        case GAVL_RGBA_64:
        case GAVL_RGB_FLOAT:
        case GAVL_RGBA_FLOAT:
        case GAVL_RGBA_32:
        case GAVL_YUVA_32:
        case GAVL_YUV_444_P:
        case GAVL_YUV_444_P_16:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_YUV_411_P:
        case GAVL_YUV_410_P:
        case GAVL_YUV_420_P:
          return GAVL_YUV_444_P; break;
        case GAVL_YUV_422_P:
        case GAVL_YUY2:
        case GAVL_UYVY:
        case GAVL_YUVJ_444_P:
        case GAVL_YUVJ_422_P:
        case GAVL_YUVJ_420_P:
          return GAVL_PIXELFORMAT_NONE; break;

        case GAVL_YUV_422_P_16:
          return GAVL_YUVJ_422_P; break;
          return GAVL_YUV_422_P_16; break;
        }
      break;
    case GAVL_YUV_444_P_16:
      switch(out_csp)
        {
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGB_15:
        case GAVL_BGR_15:
        case GAVL_RGB_16:
        case GAVL_BGR_16:
        case GAVL_RGB_24:
        case GAVL_BGR_24:
        case GAVL_RGB_32:
        case GAVL_BGR_32:
        case GAVL_RGB_48:
        case GAVL_RGBA_64:
        case GAVL_RGB_FLOAT:
        case GAVL_RGBA_FLOAT:
        case GAVL_RGBA_32:
        case GAVL_YUVA_32:
        case GAVL_YUV_444_P:
        case GAVL_YUV_444_P_16:
        case GAVL_YUVJ_444_P:
        case GAVL_YUV_422_P_16:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_YUV_422_P:
        case GAVL_YUY2:
        case GAVL_UYVY:
        case GAVL_YUV_411_P:
        case GAVL_YUV_410_P:
        case GAVL_YUV_420_P:
          return GAVL_YUV_444_P; break;
        case GAVL_YUVJ_422_P:
        case GAVL_YUVJ_420_P:
          return GAVL_YUVJ_444_P; break;
        }
      break;
    case GAVL_YUV_422_P_16:
      switch(out_csp)
        {
        case GAVL_PIXELFORMAT_NONE:
        case GAVL_RGB_15:
        case GAVL_BGR_15:
        case GAVL_RGB_16:
        case GAVL_BGR_16:
        case GAVL_RGB_24:
        case GAVL_BGR_24:
        case GAVL_RGB_32:
        case GAVL_BGR_32:
        case GAVL_RGB_48:
        case GAVL_RGBA_64:
        case GAVL_RGB_FLOAT:
        case GAVL_RGBA_FLOAT:
        case GAVL_RGBA_32:
        case GAVL_YUVA_32:
        case GAVL_YUV_444_P:
        case GAVL_YUVJ_444_P:
          return GAVL_YUV_444_P_16; break;
        case GAVL_YUV_444_P_16:
        case GAVL_YUV_422_P_16:
        case GAVL_YUV_422_P:
        case GAVL_YUY2:
        case GAVL_UYVY:
        case GAVL_YUVJ_422_P:
          return GAVL_PIXELFORMAT_NONE; break;
        case GAVL_YUV_411_P:
        case GAVL_YUV_410_P:
        case GAVL_YUV_420_P:
          return GAVL_YUV_422_P; break;
        case GAVL_YUVJ_420_P:
          return GAVL_YUVJ_422_P; break;
        }

      break;
    }
  return GAVL_PIXELFORMAT_NONE;
  }
