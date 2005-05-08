/**************************************
* Function table for colorspaces
 **************************************/

typedef struct
  {
  /* Conversions among RGB formats */
  
  gavl_video_func_t swap_rgb_24;
  gavl_video_func_t swap_rgb_32;
  gavl_video_func_t swap_rgb_16;
  gavl_video_func_t swap_rgb_15;

  gavl_video_func_t rgb_15_to_16;
  gavl_video_func_t rgb_15_to_24;
  gavl_video_func_t rgb_15_to_32;

  gavl_video_func_t rgb_16_to_15;
  gavl_video_func_t rgb_16_to_24;
  gavl_video_func_t rgb_16_to_32;
  
  gavl_video_func_t rgb_24_to_15;
  gavl_video_func_t rgb_24_to_16;
  gavl_video_func_t rgb_24_to_32;
  
  gavl_video_func_t rgb_32_to_15;
  gavl_video_func_t rgb_32_to_16;
  gavl_video_func_t rgb_32_to_24;

  gavl_video_func_t rgb_15_to_16_swap;
  gavl_video_func_t rgb_15_to_24_swap;
  gavl_video_func_t rgb_15_to_32_swap;

  gavl_video_func_t rgb_16_to_15_swap;
  gavl_video_func_t rgb_16_to_24_swap;
  gavl_video_func_t rgb_16_to_32_swap;
  
  gavl_video_func_t rgb_24_to_15_swap;
  gavl_video_func_t rgb_24_to_16_swap;
  gavl_video_func_t rgb_24_to_32_swap;
  
  gavl_video_func_t rgb_32_to_15_swap;
  gavl_video_func_t rgb_32_to_16_swap;
  gavl_video_func_t rgb_32_to_24_swap;

  /* Conversion from RGBA to RGB formats */

  gavl_video_func_t rgba_32_to_rgb_15;
  gavl_video_func_t rgba_32_to_bgr_15;
  gavl_video_func_t rgba_32_to_rgb_16;
  gavl_video_func_t rgba_32_to_bgr_16;
  gavl_video_func_t rgba_32_to_rgb_24;
  gavl_video_func_t rgba_32_to_bgr_24;
  gavl_video_func_t rgba_32_to_rgb_32;
  gavl_video_func_t rgba_32_to_bgr_32;

  /* Conversion from RGB formats to RGBA */

  gavl_video_func_t rgb_15_to_rgba_32;
  gavl_video_func_t bgr_15_to_rgba_32;
  gavl_video_func_t rgb_16_to_rgba_32;
  gavl_video_func_t bgr_16_to_rgba_32;
  gavl_video_func_t rgb_24_to_rgba_32;
  gavl_video_func_t bgr_24_to_rgba_32;
  gavl_video_func_t rgb_32_to_rgba_32;
  gavl_video_func_t bgr_32_to_rgba_32;
    
  /* RGB -> YUV */

  gavl_video_func_t rgb_15_to_yuy2;
  gavl_video_func_t rgb_15_to_uyvy;
  gavl_video_func_t rgb_15_to_yuv_420_p;
  gavl_video_func_t rgb_15_to_yuv_410_p;
  gavl_video_func_t rgb_15_to_yuv_422_p;
  gavl_video_func_t rgb_15_to_yuv_411_p;
  gavl_video_func_t rgb_15_to_yuv_444_p;
  gavl_video_func_t rgb_15_to_yuvj_420_p;
  gavl_video_func_t rgb_15_to_yuvj_422_p;
  gavl_video_func_t rgb_15_to_yuvj_444_p;

  gavl_video_func_t bgr_15_to_yuy2;
  gavl_video_func_t bgr_15_to_uyvy;
  gavl_video_func_t bgr_15_to_yuv_420_p;
  gavl_video_func_t bgr_15_to_yuv_410_p;
  gavl_video_func_t bgr_15_to_yuv_422_p;
  gavl_video_func_t bgr_15_to_yuv_411_p;
  gavl_video_func_t bgr_15_to_yuv_444_p;
  gavl_video_func_t bgr_15_to_yuvj_420_p;
  gavl_video_func_t bgr_15_to_yuvj_422_p;
  gavl_video_func_t bgr_15_to_yuvj_411_p;
  gavl_video_func_t bgr_15_to_yuvj_444_p;

  gavl_video_func_t rgb_16_to_yuy2;
  gavl_video_func_t rgb_16_to_uyvy;
  gavl_video_func_t rgb_16_to_yuv_420_p;
  gavl_video_func_t rgb_16_to_yuv_410_p;
  gavl_video_func_t rgb_16_to_yuv_422_p;
  gavl_video_func_t rgb_16_to_yuv_411_p;
  gavl_video_func_t rgb_16_to_yuv_444_p;
  gavl_video_func_t rgb_16_to_yuvj_420_p;
  gavl_video_func_t rgb_16_to_yuvj_422_p;
  gavl_video_func_t rgb_16_to_yuvj_444_p;

  gavl_video_func_t bgr_16_to_yuy2;
  gavl_video_func_t bgr_16_to_uyvy;
  gavl_video_func_t bgr_16_to_yuv_420_p;
  gavl_video_func_t bgr_16_to_yuv_410_p;
  gavl_video_func_t bgr_16_to_yuv_422_p;
  gavl_video_func_t bgr_16_to_yuv_411_p;
  gavl_video_func_t bgr_16_to_yuv_444_p;
  gavl_video_func_t bgr_16_to_yuvj_420_p;
  gavl_video_func_t bgr_16_to_yuvj_422_p;
  gavl_video_func_t bgr_16_to_yuvj_444_p;

  gavl_video_func_t rgb_24_to_yuy2;
  gavl_video_func_t rgb_24_to_uyvy;
  gavl_video_func_t rgb_24_to_yuv_420_p;
  gavl_video_func_t rgb_24_to_yuv_410_p;
  gavl_video_func_t rgb_24_to_yuv_422_p;
  gavl_video_func_t rgb_24_to_yuv_411_p;
  gavl_video_func_t rgb_24_to_yuv_444_p;
  gavl_video_func_t rgb_24_to_yuvj_420_p;
  gavl_video_func_t rgb_24_to_yuvj_422_p;
  gavl_video_func_t rgb_24_to_yuvj_444_p;

  gavl_video_func_t bgr_24_to_yuy2;
  gavl_video_func_t bgr_24_to_uyvy;
  gavl_video_func_t bgr_24_to_yuv_420_p;
  gavl_video_func_t bgr_24_to_yuv_410_p;
  gavl_video_func_t bgr_24_to_yuv_422_p;
  gavl_video_func_t bgr_24_to_yuv_411_p;
  gavl_video_func_t bgr_24_to_yuv_444_p;
  gavl_video_func_t bgr_24_to_yuvj_420_p;
  gavl_video_func_t bgr_24_to_yuvj_422_p;
  gavl_video_func_t bgr_24_to_yuvj_444_p;

  gavl_video_func_t rgb_32_to_yuy2;
  gavl_video_func_t rgb_32_to_uyvy;
  gavl_video_func_t rgb_32_to_yuv_420_p;
  gavl_video_func_t rgb_32_to_yuv_410_p;
  gavl_video_func_t rgb_32_to_yuv_422_p;
  gavl_video_func_t rgb_32_to_yuv_411_p;
  gavl_video_func_t rgb_32_to_yuv_444_p;
  gavl_video_func_t rgb_32_to_yuvj_420_p;
  gavl_video_func_t rgb_32_to_yuvj_422_p;
  gavl_video_func_t rgb_32_to_yuvj_444_p;

  gavl_video_func_t bgr_32_to_yuy2;
  gavl_video_func_t bgr_32_to_uyvy;
  gavl_video_func_t bgr_32_to_yuv_420_p;
  gavl_video_func_t bgr_32_to_yuv_410_p;
  gavl_video_func_t bgr_32_to_yuv_422_p;
  gavl_video_func_t bgr_32_to_yuv_411_p;
  gavl_video_func_t bgr_32_to_yuv_444_p;
  gavl_video_func_t bgr_32_to_yuvj_420_p;
  gavl_video_func_t bgr_32_to_yuvj_422_p;
  gavl_video_func_t bgr_32_to_yuvj_444_p;

  gavl_video_func_t rgba_32_to_yuy2;
  gavl_video_func_t rgba_32_to_uyvy;
  gavl_video_func_t rgba_32_to_yuv_420_p;
  gavl_video_func_t rgba_32_to_yuv_410_p;
  gavl_video_func_t rgba_32_to_yuv_422_p;
  gavl_video_func_t rgba_32_to_yuv_411_p;
  gavl_video_func_t rgba_32_to_yuv_444_p;
  gavl_video_func_t rgba_32_to_yuvj_420_p;
  gavl_video_func_t rgba_32_to_yuvj_422_p;
  gavl_video_func_t rgba_32_to_yuvj_444_p;

  /* YUV -> RGB */

  gavl_video_func_t yuy2_to_rgb_15;
  gavl_video_func_t yuy2_to_bgr_15;
  gavl_video_func_t yuy2_to_rgb_16;
  gavl_video_func_t yuy2_to_bgr_16;
  gavl_video_func_t yuy2_to_rgb_24;
  gavl_video_func_t yuy2_to_bgr_24;
  gavl_video_func_t yuy2_to_rgb_32;
  gavl_video_func_t yuy2_to_bgr_32;
  gavl_video_func_t yuy2_to_rgba_32;

  gavl_video_func_t uyvy_to_rgb_15;
  gavl_video_func_t uyvy_to_bgr_15;
  gavl_video_func_t uyvy_to_rgb_16;
  gavl_video_func_t uyvy_to_bgr_16;
  gavl_video_func_t uyvy_to_rgb_24;
  gavl_video_func_t uyvy_to_bgr_24;
  gavl_video_func_t uyvy_to_rgb_32;
  gavl_video_func_t uyvy_to_bgr_32;
  gavl_video_func_t uyvy_to_rgba_32;

  gavl_video_func_t yuv_420_p_to_rgb_15;
  gavl_video_func_t yuv_420_p_to_bgr_15;
  gavl_video_func_t yuv_420_p_to_rgb_16;
  gavl_video_func_t yuv_420_p_to_bgr_16;
  gavl_video_func_t yuv_420_p_to_rgb_24;
  gavl_video_func_t yuv_420_p_to_bgr_24;
  gavl_video_func_t yuv_420_p_to_rgb_32;
  gavl_video_func_t yuv_420_p_to_bgr_32;
  gavl_video_func_t yuv_420_p_to_rgba_32;

  gavl_video_func_t yuv_410_p_to_rgb_15;
  gavl_video_func_t yuv_410_p_to_bgr_15;
  gavl_video_func_t yuv_410_p_to_rgb_16;
  gavl_video_func_t yuv_410_p_to_bgr_16;
  gavl_video_func_t yuv_410_p_to_rgb_24;
  gavl_video_func_t yuv_410_p_to_bgr_24;
  gavl_video_func_t yuv_410_p_to_rgb_32;
  gavl_video_func_t yuv_410_p_to_bgr_32;
  gavl_video_func_t yuv_410_p_to_rgba_32;
  
  gavl_video_func_t yuvj_420_p_to_rgb_15;
  gavl_video_func_t yuvj_420_p_to_bgr_15;
  gavl_video_func_t yuvj_420_p_to_rgb_16;
  gavl_video_func_t yuvj_420_p_to_bgr_16;
  gavl_video_func_t yuvj_420_p_to_rgb_24;
  gavl_video_func_t yuvj_420_p_to_bgr_24;
  gavl_video_func_t yuvj_420_p_to_rgb_32;
  gavl_video_func_t yuvj_420_p_to_bgr_32;
  gavl_video_func_t yuvj_420_p_to_rgba_32;

  gavl_video_func_t yuv_422_p_to_rgb_15;
  gavl_video_func_t yuv_422_p_to_bgr_15;
  gavl_video_func_t yuv_422_p_to_rgb_16;
  gavl_video_func_t yuv_422_p_to_bgr_16;
  gavl_video_func_t yuv_422_p_to_rgb_24;
  gavl_video_func_t yuv_422_p_to_bgr_24;
  gavl_video_func_t yuv_422_p_to_rgb_32;
  gavl_video_func_t yuv_422_p_to_bgr_32;
  gavl_video_func_t yuv_422_p_to_rgba_32;

  gavl_video_func_t yuv_411_p_to_rgb_15;
  gavl_video_func_t yuv_411_p_to_bgr_15;
  gavl_video_func_t yuv_411_p_to_rgb_16;
  gavl_video_func_t yuv_411_p_to_bgr_16;
  gavl_video_func_t yuv_411_p_to_rgb_24;
  gavl_video_func_t yuv_411_p_to_bgr_24;
  gavl_video_func_t yuv_411_p_to_rgb_32;
  gavl_video_func_t yuv_411_p_to_bgr_32;
  gavl_video_func_t yuv_411_p_to_rgba_32;

  gavl_video_func_t yuvj_422_p_to_rgb_15;
  gavl_video_func_t yuvj_422_p_to_bgr_15;
  gavl_video_func_t yuvj_422_p_to_rgb_16;
  gavl_video_func_t yuvj_422_p_to_bgr_16;
  gavl_video_func_t yuvj_422_p_to_rgb_24;
  gavl_video_func_t yuvj_422_p_to_bgr_24;
  gavl_video_func_t yuvj_422_p_to_rgb_32;
  gavl_video_func_t yuvj_422_p_to_bgr_32;
  gavl_video_func_t yuvj_422_p_to_rgba_32;

  gavl_video_func_t yuv_444_p_to_rgb_15;
  gavl_video_func_t yuv_444_p_to_bgr_15;
  gavl_video_func_t yuv_444_p_to_rgb_16;
  gavl_video_func_t yuv_444_p_to_bgr_16;
  gavl_video_func_t yuv_444_p_to_rgb_24;
  gavl_video_func_t yuv_444_p_to_bgr_24;
  gavl_video_func_t yuv_444_p_to_rgb_32;
  gavl_video_func_t yuv_444_p_to_bgr_32;
  gavl_video_func_t yuv_444_p_to_rgba_32;

  gavl_video_func_t yuvj_444_p_to_rgb_15;
  gavl_video_func_t yuvj_444_p_to_bgr_15;
  gavl_video_func_t yuvj_444_p_to_rgb_16;
  gavl_video_func_t yuvj_444_p_to_bgr_16;
  gavl_video_func_t yuvj_444_p_to_rgb_24;
  gavl_video_func_t yuvj_444_p_to_bgr_24;
  gavl_video_func_t yuvj_444_p_to_rgb_32;
  gavl_video_func_t yuvj_444_p_to_bgr_32;
  gavl_video_func_t yuvj_444_p_to_rgba_32;
  
  /* Conversions among YUV formats */

  gavl_video_func_t yuy2_to_yuv_420_p;
  gavl_video_func_t yuy2_to_yuv_410_p;
  gavl_video_func_t yuy2_to_yuv_422_p;
  gavl_video_func_t yuy2_to_yuv_411_p;
  gavl_video_func_t yuy2_to_yuv_444_p;
  gavl_video_func_t yuy2_to_yuvj_420_p;
  gavl_video_func_t yuy2_to_yuvj_422_p;
  gavl_video_func_t yuy2_to_yuvj_444_p;

  gavl_video_func_t uyvy_to_yuy2;
  gavl_video_func_t uyvy_to_yuv_420_p;
  gavl_video_func_t uyvy_to_yuv_410_p;
  gavl_video_func_t uyvy_to_yuv_422_p;
  gavl_video_func_t uyvy_to_yuv_411_p;
  gavl_video_func_t uyvy_to_yuv_444_p;
  gavl_video_func_t uyvy_to_yuvj_420_p;
  gavl_video_func_t uyvy_to_yuvj_422_p;
  gavl_video_func_t uyvy_to_yuvj_444_p;
  
  gavl_video_func_t yuv_420_p_to_yuv_422_p;
  gavl_video_func_t yuv_420_p_to_yuv_411_p;
  gavl_video_func_t yuv_420_p_to_yuv_444_p;
  gavl_video_func_t yuv_420_p_to_yuv_410_p;
  gavl_video_func_t yuv_420_p_to_yuvj_420_p;
  gavl_video_func_t yuv_420_p_to_yuvj_422_p;
  gavl_video_func_t yuv_420_p_to_yuvj_444_p;
  gavl_video_func_t yuv_420_p_to_yuy2;
  gavl_video_func_t yuv_420_p_to_uyvy;

  gavl_video_func_t yuv_410_p_to_yuv_420_p;
  gavl_video_func_t yuv_410_p_to_yuv_422_p;
  gavl_video_func_t yuv_410_p_to_yuv_411_p;
  gavl_video_func_t yuv_410_p_to_yuv_444_p;
  gavl_video_func_t yuv_410_p_to_yuvj_420_p;
  gavl_video_func_t yuv_410_p_to_yuvj_422_p;
  gavl_video_func_t yuv_410_p_to_yuvj_444_p;
  gavl_video_func_t yuv_410_p_to_yuy2;
  gavl_video_func_t yuv_410_p_to_uyvy;

  gavl_video_func_t yuv_422_p_to_yuv_420_p;
  gavl_video_func_t yuv_422_p_to_yuv_410_p;
  gavl_video_func_t yuv_422_p_to_yuv_411_p;
  gavl_video_func_t yuv_422_p_to_yuv_444_p;
  gavl_video_func_t yuv_422_p_to_yuvj_420_p;
  gavl_video_func_t yuv_422_p_to_yuvj_422_p;
  gavl_video_func_t yuv_422_p_to_yuvj_444_p;
  gavl_video_func_t yuv_422_p_to_yuy2;
  gavl_video_func_t yuv_422_p_to_uyvy;

  gavl_video_func_t yuv_411_p_to_yuv_420_p;
  gavl_video_func_t yuv_411_p_to_yuv_410_p;
  gavl_video_func_t yuv_411_p_to_yuv_444_p;
  gavl_video_func_t yuv_411_p_to_yuv_422_p;
  gavl_video_func_t yuv_411_p_to_yuvj_420_p;
  gavl_video_func_t yuv_411_p_to_yuvj_422_p;
  gavl_video_func_t yuv_411_p_to_yuvj_444_p;
  gavl_video_func_t yuv_411_p_to_yuy2;
  gavl_video_func_t yuv_411_p_to_uyvy;
  
  gavl_video_func_t yuv_444_p_to_yuv_420_p;
  gavl_video_func_t yuv_444_p_to_yuv_410_p;
  gavl_video_func_t yuv_444_p_to_yuv_422_p;
  gavl_video_func_t yuv_444_p_to_yuv_411_p;
  gavl_video_func_t yuv_444_p_to_yuvj_420_p;
  gavl_video_func_t yuv_444_p_to_yuvj_422_p;
  gavl_video_func_t yuv_444_p_to_yuvj_444_p;
  gavl_video_func_t yuv_444_p_to_yuy2;
  gavl_video_func_t yuv_444_p_to_uyvy;

  gavl_video_func_t yuvj_420_p_to_yuv_420_p;
  gavl_video_func_t yuvj_420_p_to_yuv_410_p;
  gavl_video_func_t yuvj_420_p_to_yuv_422_p;
  gavl_video_func_t yuvj_420_p_to_yuv_411_p;
  gavl_video_func_t yuvj_420_p_to_yuv_444_p;
  gavl_video_func_t yuvj_420_p_to_yuy2;
  gavl_video_func_t yuvj_420_p_to_uyvy;

  gavl_video_func_t yuvj_422_p_to_yuv_420_p;
  gavl_video_func_t yuvj_422_p_to_yuv_410_p;
  gavl_video_func_t yuvj_422_p_to_yuv_422_p;
  gavl_video_func_t yuvj_422_p_to_yuv_411_p;
  gavl_video_func_t yuvj_422_p_to_yuv_444_p;
  gavl_video_func_t yuvj_422_p_to_yuy2;
  gavl_video_func_t yuvj_422_p_to_uyvy;

  gavl_video_func_t yuvj_444_p_to_yuv_420_p;
  gavl_video_func_t yuvj_444_p_to_yuv_410_p;
  gavl_video_func_t yuvj_444_p_to_yuv_422_p;
  gavl_video_func_t yuvj_444_p_to_yuv_411_p;
  gavl_video_func_t yuvj_444_p_to_yuv_444_p;
  gavl_video_func_t yuvj_444_p_to_yuy2;
  gavl_video_func_t yuvj_444_p_to_uyvy;

  } gavl_colorspace_function_table_t;

void gavl_init_rgb_rgb_funcs_c(gavl_colorspace_function_table_t *);
void gavl_init_rgb_yuv_funcs_c(gavl_colorspace_function_table_t *);
void gavl_init_yuv_yuv_funcs_c(gavl_colorspace_function_table_t *);
void gavl_init_yuv_rgb_funcs_c(gavl_colorspace_function_table_t *);

#if 0
void gavl_init_rgb_yuv_scanline_funcs_c(gavl_colorspace_function_table_t *);
void gavl_init_rgb_rgb_scanline_funcs_c(gavl_colorspace_function_table_t *);
void gavl_init_yuv_yuv_scanline_funcs_c(gavl_colorspace_function_table_t *);
void gavl_init_yuv_rgb_scanline_funcs_c(gavl_colorspace_function_table_t *);
#endif

#ifdef ARCH_X86
void gavl_init_rgb_rgb_funcs_mmx(gavl_colorspace_function_table_t *,
                                 int width, int quality);

void
gavl_init_rgb_yuv_funcs_mmx(gavl_colorspace_function_table_t *,
                            int width, int quality);


void
gavl_init_yuv_yuv_funcs_mmx(gavl_colorspace_function_table_t *,
                            int width, int quality);


void
gavl_init_yuv_rgb_funcs_mmx(gavl_colorspace_function_table_t *,
                            int width, int quality);

void
gavl_init_rgb_rgb_funcs_mmxext(gavl_colorspace_function_table_t *,
                               int width, int quality);


void gavl_init_rgb_yuv_funcs_mmxext(gavl_colorspace_function_table_t *,
                                    int width, int quality);


void gavl_init_yuv_yuv_funcs_mmxext(gavl_colorspace_function_table_t *,
                                    int width, int quality);

void
gavl_init_yuv_rgb_funcs_mmxext(gavl_colorspace_function_table_t *,
                               int width, int quality);


#if 0
void
gavl_init_rgb_rgb_scanline_funcs_mmx(gavl_colorspace_function_table_t *,
                                     int width, int quality);
void
gavl_init_rgb_yuv_scanline_funcs_mmx(gavl_colorspace_function_table_t *,
                                     int width, int quality);
void
gavl_init_yuv_rgb_scanline_funcs_mmx(gavl_colorspace_function_table_t *,
                                     int width, int quality);
void
gavl_init_yuv_yuv_scanline_funcs_mmx(gavl_colorspace_function_table_t *,
                                     int width, int quality);
void
gavl_init_rgb_rgb_scanline_funcs_mmxext(gavl_colorspace_function_table_t *,
                                        int width, int quality);
void
gavl_init_rgb_yuv_scanline_funcs_mmxext(gavl_colorspace_function_table_t *,
                                        int width, int quality);
void
gavl_init_yuv_yuv_scanline_funcs_mmxext(gavl_colorspace_function_table_t *,
                                        int width, int quality);

void
gavl_init_yuv_rgb_scanline_funcs_mmxext(gavl_colorspace_function_table_t *,
                                        int width, int quality);
#endif

#endif
