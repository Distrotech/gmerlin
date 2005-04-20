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

#include <stdlib.h>
#include <string.h>

typedef struct
  {
  gavl_colorspace_t colorspace;
  char * name;
  } colorspace_tab_t;

colorspace_tab_t colorspace_tab[] =
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
    { GAVL_YUY2, "YUV 422 (YUY2)" },
    { GAVL_UYVY, "YUV 422 (UYVY)" },
    { GAVL_YUV_420_P, "YUV 420 Planar" },
    { GAVL_YUV_410_P, "YUV 410 Planar" },
    { GAVL_YUV_411_P, "YUV 411 Planar" },
    { GAVL_YUV_422_P, "YUV 422 Planar" },
    { GAVL_YUV_444_P, "YUV 444 Planar" },
    { GAVL_YUVJ_420_P, "YUVJ 420 Planar" },
    { GAVL_YUVJ_422_P, "YUVJ 422 Planar" },
    { GAVL_YUVJ_444_P, "YUVJ 444 Planar" },
    { GAVL_COLORSPACE_NONE, "Undefined" }
  };

static int num_colorspaces = sizeof(colorspace_tab)/sizeof(colorspace_tab_t);

int gavl_colorspace_num_planes(gavl_colorspace_t csp)
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
      return 1;
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      return 3;
      break;
    case GAVL_COLORSPACE_NONE:
      return 0;
      break;
    }
  return 0;
  }

void gavl_colorspace_chroma_sub(gavl_colorspace_t csp, int * sub_h,
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
    case GAVL_YUVJ_444_P:
      *sub_h = 1;
      *sub_v = 1;
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUVJ_420_P:
      *sub_h = 2;
      *sub_v = 2;
      break;
    case GAVL_YUV_422_P:
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
    case GAVL_COLORSPACE_NONE:
      *sub_h = 0;
      *sub_v = 0;
      break;
    }
  }

int gavl_num_colorspaces()
  {
  return num_colorspaces - 1;
  }

gavl_colorspace_t gavl_get_colorspace(int index)
  {
  return colorspace_tab[index].colorspace;
  }

int gavl_colorspace_is_rgb(gavl_colorspace_t colorspace)
  {
  switch(colorspace)
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
      return 1;
    case GAVL_YUY2:
    case GAVL_UYVY:
    case GAVL_YUV_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
    case GAVL_COLORSPACE_NONE:
      return 0;
    }
  return 0;
  }

int gavl_colorspace_is_yuv(gavl_colorspace_t colorspace)
  {
  switch(colorspace)
    {
    case GAVL_YUY2:
    case GAVL_UYVY:
    case GAVL_YUV_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      return 1;
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
    case GAVL_RGB_24:
    case GAVL_BGR_24:
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGBA_32:
    case GAVL_COLORSPACE_NONE:
      return 0;
    }
  return 0;
  }

int gavl_colorspace_is_planar(gavl_colorspace_t colorspace)
  {
  switch(colorspace)
    {
    case GAVL_YUV_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      return 1;
    default:
      return 0;
    }
  return 0;
  }

int gavl_colorspace_has_alpha(gavl_colorspace_t colorspace)
  {
  switch(colorspace)
    {
    case GAVL_RGBA_32:
      return 1;
    default:
      return 0;
    }
  return 0;
  
  }

const char * gavl_colorspace_to_string(gavl_colorspace_t colorspace)
  {
  int i;
  for(i = 0; i < num_colorspaces; i++)
    {
    if(colorspace_tab[i].colorspace == colorspace)
      return colorspace_tab[i].name;
    }
  return (const char*)0;
  }

gavl_colorspace_t gavl_string_to_colorspace(const char * name)
  {
  int i;
  for(i = 0; i < num_colorspaces; i++)
    {
    if(!strcmp(colorspace_tab[i].name, name))
      return colorspace_tab[i].colorspace;
    }
  return GAVL_COLORSPACE_NONE;
  }


static gavl_colorspace_function_table_t *
create_colorspace_function_table(const gavl_video_options_t * opt,
                                 int width, int height)
  {
  gavl_colorspace_function_table_t * csp_tab;
  int real_accel_flags = opt->accel_flags ?
    gavl_real_accel_flags(opt->accel_flags) : 0;
    
  csp_tab =
    calloc(1,sizeof(gavl_colorspace_function_table_t));
  
  if(opt->conversion_flags & GAVL_SCANLINE)
    {
    if(!real_accel_flags || (real_accel_flags & GAVL_ACCEL_C))
      {
      gavl_init_rgb_rgb_scanline_funcs_c(csp_tab);
      gavl_init_rgb_yuv_scanline_funcs_c(csp_tab);
      gavl_init_yuv_rgb_scanline_funcs_c(csp_tab);
      gavl_init_yuv_yuv_scanline_funcs_c(csp_tab);
      }
#ifdef ARCH_X86
    if(!real_accel_flags || (real_accel_flags & GAVL_ACCEL_MMX))
      {
      gavl_init_rgb_rgb_scanline_funcs_mmx(csp_tab, width, opt->quality);
      gavl_init_rgb_yuv_scanline_funcs_mmx(csp_tab, width, opt->quality);
      gavl_init_yuv_yuv_scanline_funcs_mmx(csp_tab, width, opt->quality);
      gavl_init_yuv_rgb_scanline_funcs_mmx(csp_tab, width, opt->quality);
      }
    if(!real_accel_flags || (real_accel_flags & GAVL_ACCEL_MMXEXT))
      {
      gavl_init_rgb_rgb_scanline_funcs_mmxext(csp_tab, width, opt->quality);
      gavl_init_rgb_yuv_scanline_funcs_mmxext(csp_tab, width, opt->quality);
      gavl_init_yuv_yuv_scanline_funcs_mmxext(csp_tab, width, opt->quality);
      gavl_init_yuv_rgb_scanline_funcs_mmxext(csp_tab, width, opt->quality);
      }
#endif
    }
  else
    {
    if(!real_accel_flags || (real_accel_flags & GAVL_ACCEL_C))
      {
      gavl_init_rgb_rgb_funcs_c(csp_tab);
      gavl_init_rgb_yuv_funcs_c(csp_tab);
      gavl_init_yuv_rgb_funcs_c(csp_tab);
      gavl_init_yuv_yuv_funcs_c(csp_tab);
      }
#ifdef ARCH_X86
    if(!real_accel_flags || (real_accel_flags & GAVL_ACCEL_MMX))
      {
      gavl_init_rgb_rgb_funcs_mmx(csp_tab, width, opt->quality);
      gavl_init_rgb_yuv_funcs_mmx(csp_tab, width, opt->quality);
      gavl_init_yuv_yuv_funcs_mmx(csp_tab, width, opt->quality);
      gavl_init_yuv_rgb_funcs_mmx(csp_tab, width, opt->quality);
      }
    if(!real_accel_flags || (real_accel_flags & GAVL_ACCEL_MMXEXT))
      {
      gavl_init_rgb_rgb_funcs_mmxext(csp_tab, width, opt->quality);
      gavl_init_rgb_yuv_funcs_mmxext(csp_tab, width, opt->quality);
      gavl_init_yuv_yuv_funcs_mmxext(csp_tab, width, opt->quality);
      gavl_init_yuv_rgb_funcs_mmxext(csp_tab, width, opt->quality);
      }
#endif
    }
  return csp_tab;
  }

gavl_video_func_t
gavl_find_colorspace_converter(const gavl_video_options_t * opt,
                               gavl_colorspace_t input_colorspace,
                               gavl_colorspace_t output_colorspace,
                               int width,
                               int height)
  {
  gavl_video_func_t ret = NULL;
  gavl_colorspace_function_table_t * tab =
    create_colorspace_function_table(opt, width, height);


  switch(input_colorspace)
    {
    case GAVL_RGB_15:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->rgb_15_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgb_15_to_uyvy;
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
        case GAVL_YUV_411_P:
          ret = tab->rgb_15_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgb_15_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_RGB_15:
          break;
        }
      break;
    case GAVL_BGR_15:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->bgr_15_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->bgr_15_to_uyvy;
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
        case GAVL_YUV_411_P:
          ret = tab->bgr_15_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->bgr_15_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_BGR_15:
          break;
        }
      break;
    case GAVL_RGB_16:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->rgb_16_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgb_16_to_uyvy;
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
        case GAVL_YUV_411_P:
          ret = tab->rgb_16_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgb_16_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_RGB_16:
          break;
        }
      break;
    case GAVL_BGR_16:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->bgr_16_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->bgr_16_to_uyvy;
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
        case GAVL_YUV_411_P:
          ret = tab->bgr_16_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->bgr_16_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_BGR_16:
          break;
        }
      break;
    case GAVL_RGB_24:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->rgb_24_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgb_24_to_uyvy;
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
        case GAVL_YUV_411_P:
          ret = tab->rgb_24_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgb_24_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_RGB_24:
          break;
        }
      break;
    case GAVL_BGR_24:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->bgr_24_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->bgr_24_to_uyvy;
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
        case GAVL_YUV_411_P:
          ret = tab->bgr_24_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->bgr_24_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_BGR_24:
          break;
        }
      break;
    case GAVL_RGB_32:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->rgb_32_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgb_32_to_uyvy;
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
        case GAVL_YUV_411_P:
          ret = tab->rgb_32_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgb_32_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_RGB_32:
          break;
        }
      break;
    case GAVL_BGR_32:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->bgr_32_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->bgr_32_to_uyvy;
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
        case GAVL_YUV_411_P:
          ret = tab->bgr_32_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->bgr_32_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_BGR_32:
          break;
        }
      break;
    case GAVL_RGBA_32:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->rgba_32_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->rgba_32_to_uyvy;
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
        case GAVL_YUV_411_P:
          ret = tab->rgba_32_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->rgba_32_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_RGBA_32:
          break;
        }
      break;
    case GAVL_YUY2:
      switch(output_colorspace)
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
        case GAVL_YUV_420_P:
          ret = tab->yuy2_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuy2_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuy2_to_yuv_422_p;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuy2_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuy2_to_yuv_444_p;
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
        /* Keep GCC happy */
        case GAVL_COLORSPACE_NONE:
        case GAVL_YUY2:
          break;
        }
      break;
    case GAVL_UYVY:
      switch(output_colorspace)
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
        case GAVL_YUV_420_P:
          ret = tab->uyvy_to_yuv_420_p;
          break;
        case GAVL_YUV_410_P:
          ret = tab->uyvy_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->uyvy_to_yuv_422_p;
          break;
        case GAVL_YUV_411_P:
          ret = tab->uyvy_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->uyvy_to_yuv_444_p;
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
        /* Keep GCC happy */
        case GAVL_COLORSPACE_NONE:
        case GAVL_UYVY:
          break;
        }
      break;
    case GAVL_YUV_420_P:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->yuv_420_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_420_p_to_uyvy;
          break;
        case GAVL_YUV_410_P:
          ret = tab->yuv_420_p_to_yuv_410_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuv_420_p_to_yuv_422_p;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuv_420_p_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuv_420_p_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_YUV_420_P:
          break;
        }
      break;
    case GAVL_YUV_410_P:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->yuv_410_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_410_p_to_uyvy;
          break;
        case GAVL_YUV_420_P:
          ret = tab->yuv_410_p_to_yuv_420_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuv_410_p_to_yuv_422_p;
          break;
        case GAVL_YUV_411_P:
          ret = tab->yuv_410_p_to_yuv_411_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuv_410_p_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_YUV_410_P:
          break;
        }
      break;
    case GAVL_YUV_422_P:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->yuv_422_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_422_p_to_uyvy;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_YUV_422_P:
          break;
        }
      break;
    case GAVL_YUV_411_P:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->yuv_411_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_411_p_to_uyvy;
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
        case GAVL_YUV_444_P:
          ret = tab->yuv_411_p_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_YUV_411_P:
          break;
        }
      break;
    case GAVL_YUV_444_P:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->yuv_444_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuv_444_p_to_uyvy;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_YUV_444_P:
          break;
        }
      break;
    case GAVL_YUVJ_420_P:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->yuvj_420_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuvj_420_p_to_uyvy;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuvj_420_p_to_yuv_422_p;
          break;
        case GAVL_YUV_444_P:
          ret = tab->yuvj_420_p_to_yuv_444_p;
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
        case GAVL_COLORSPACE_NONE:
        case GAVL_YUVJ_420_P:
          break;
        }
      break;
    case GAVL_YUVJ_422_P:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->yuvj_422_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuvj_422_p_to_uyvy;
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
        case GAVL_YUVJ_420_P:
          ret = tab->yuv_422_p_to_yuv_420_p;
          break;
        case GAVL_YUV_422_P:
          ret = tab->yuvj_422_p_to_yuv_422_p;
          break;
        case GAVL_YUVJ_444_P:
          ret = tab->yuv_422_p_to_yuv_444_p;
          break;
        /* Keep GCC happy */
        case GAVL_COLORSPACE_NONE:
        case GAVL_YUVJ_422_P:
          break;
        }
      break;
    case GAVL_YUVJ_444_P:
      switch(output_colorspace)
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
        case GAVL_YUY2:
          ret = tab->yuvj_444_p_to_yuy2;
          break;
        case GAVL_UYVY:
          ret = tab->yuvj_444_p_to_uyvy;
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
        /* Keep GCC happy */
        case GAVL_COLORSPACE_NONE:
        case GAVL_YUVJ_444_P:
          break;
        }
      break;

      
    case GAVL_COLORSPACE_NONE:
      break;
    }
  free(tab);  
  return ret;
  }

/* bytes_per_component is only valid for planar formats */
  
int gavl_colorspace_bytes_per_component(gavl_colorspace_t csp)
  {
  switch(csp)
    {
    case GAVL_COLORSPACE_NONE:
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
    }
  return 0;
  }

/* bytes_per_pixel is only valid for packed formats */

int gavl_colorspace_bytes_per_pixel(gavl_colorspace_t csp)
  {
  switch(csp)
    {
    case GAVL_COLORSPACE_NONE:
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
      return 4;
      break;
    case GAVL_YUY2:
    case GAVL_UYVY:
      return 2;
    case GAVL_YUV_420_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_444_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_410_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      return 0;
    }
  return 0;
  }
