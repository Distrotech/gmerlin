#define RECLIP_8(color) (uint8_t)((color>0xFF)?0xff:((color<0)?0:color))
#define RECLIP_16(color) (uint16_t)((color>0xFFFF)?0xFFFF:((color<0)?0:color))
#define RECLIP_FLOAT(color) ((color>1.0)?1.0:((color<0.0)?0.0:color))

#define RECLIP(color, min, max) ((color>max)?max:((color<min)?min:color))

/* Masks for BGR16 and RGB16 formats */

#define RGB16_LOWER_MASK  0x001f
#define RGB16_MIDDLE_MASK 0x07e0
#define RGB16_UPPER_MASK  0xf800

/* Extract 8 bit RGB values from 16 bit pixels */

#define RGB16_TO_R_8(pixel) gavl_rgb_5_to_8[(pixel & RGB16_UPPER_MASK)>>11]
#define RGB16_TO_G_8(pixel) gavl_rgb_6_to_8[(pixel & RGB16_MIDDLE_MASK)>>5]
#define RGB16_TO_B_8(pixel) gavl_rgb_5_to_8[(pixel & RGB16_LOWER_MASK)]

#define BGR16_TO_B_8(pixel) RGB16_TO_R_8(pixel)
#define BGR16_TO_G_8(pixel) RGB16_TO_G_8(pixel)
#define BGR16_TO_R_8(pixel) RGB16_TO_B_8(pixel)

/* Extract 16 bit RGB values from 16 bit pixels */

#define RGB16_TO_R_16(pixel) gavl_rgb_5_to_16[(pixel & RGB16_UPPER_MASK)>>11]
#define RGB16_TO_G_16(pixel) gavl_rgb_6_to_16[(pixel & RGB16_MIDDLE_MASK)>>5]
#define RGB16_TO_B_16(pixel) gavl_rgb_5_to_16[(pixel & RGB16_LOWER_MASK)]

#define BGR16_TO_B_16(pixel) RGB16_TO_R_16(pixel)
#define BGR16_TO_G_16(pixel) RGB16_TO_G_16(pixel)
#define BGR16_TO_R_16(pixel) RGB16_TO_B_16(pixel)

/* Extract float RGB values from 16 bit pixels */

#define RGB16_TO_R_FLOAT(pixel) gavl_rgb_5_to_float[(pixel & RGB16_UPPER_MASK)>>11]
#define RGB16_TO_G_FLOAT(pixel) gavl_rgb_6_to_float[(pixel & RGB16_MIDDLE_MASK)>>5]
#define RGB16_TO_B_FLOAT(pixel) gavl_rgb_5_to_float[(pixel & RGB16_LOWER_MASK)]

#define BGR16_TO_B_FLOAT(pixel) RGB16_TO_R_FLOAT(pixel)
#define BGR16_TO_G_FLOAT(pixel) RGB16_TO_G_FLOAT(pixel)
#define BGR16_TO_R_FLOAT(pixel) RGB16_TO_B_FLOAT(pixel)

/* Masks for BGR16 and RGB16 formats */

#define RGB15_LOWER_MASK  0x001f
#define RGB15_MIDDLE_MASK 0x03e0
#define RGB15_UPPER_MASK  0x7C00

/* Extract 8 bit RGB values from 16 bit pixels */

#define RGB15_TO_R_8(pixel) gavl_rgb_5_to_8[(pixel & RGB15_UPPER_MASK)>>10]
#define RGB15_TO_G_8(pixel) gavl_rgb_5_to_8[(pixel & RGB15_MIDDLE_MASK)>>5] 
#define RGB15_TO_B_8(pixel) gavl_rgb_5_to_8[(pixel & RGB15_LOWER_MASK)]

#define BGR15_TO_B_8(pixel) RGB15_TO_R_8(pixel)
#define BGR15_TO_G_8(pixel) RGB15_TO_G_8(pixel) 
#define BGR15_TO_R_8(pixel) RGB15_TO_B_8(pixel)

#define RGB15_TO_R_16(pixel) gavl_rgb_5_to_16[(pixel & RGB15_UPPER_MASK)>>10]
#define RGB15_TO_G_16(pixel) gavl_rgb_5_to_16[(pixel & RGB15_MIDDLE_MASK)>>5] 
#define RGB15_TO_B_16(pixel) gavl_rgb_5_to_16[(pixel & RGB15_LOWER_MASK)]

#define BGR15_TO_B_16(pixel) RGB15_TO_R_16(pixel)
#define BGR15_TO_G_16(pixel) RGB15_TO_G_16(pixel) 
#define BGR15_TO_R_16(pixel) RGB15_TO_B_16(pixel)

#define RGB15_TO_R_FLOAT(pixel) gavl_rgb_5_to_float[(pixel & RGB15_UPPER_MASK)>>10]
#define RGB15_TO_G_FLOAT(pixel) gavl_rgb_5_to_float[(pixel & RGB15_MIDDLE_MASK)>>5] 
#define RGB15_TO_B_FLOAT(pixel) gavl_rgb_5_to_float[(pixel & RGB15_LOWER_MASK)]

#define BGR15_TO_B_FLOAT(pixel) RGB15_TO_R_FLOAT(pixel)
#define BGR15_TO_G_FLOAT(pixel) RGB15_TO_G_FLOAT(pixel) 
#define BGR15_TO_R_FLOAT(pixel) RGB15_TO_B_FLOAT(pixel)

/* Conversion from 8 bit */

#define RGB_8_TO_16(val) (((uint32_t)(val))<<8)|(val)
#define RGB_8_TO_FLOAT(val) ((float)(val)/255.0)

/* Conversion from float */

#define RGB_FLOAT_TO_8(val) (uint8_t)((val)*255.0+0.5)
#define RGB_FLOAT_TO_16(val) (uint16_t)((val)*65535.0+0.5)

/* Conversion from 16 bit */

#define RGB_16_TO_8(val) ((val)>>8)
#define RGB_16_TO_FLOAT(val) ((float)(val)/65535.0)

/* Conversion from YUV float */

#define Y_FLOAT_TO_8(val) (int)(val * 219.0) + 16;
#define UV_FLOAT_TO_8(val) (int)(val * 224.0) + 128;

#define Y_FLOAT_TO_16(val) (int)(val * 219.0 * (float)0x100) + 0x1000;
#define UV_FLOAT_TO_16(val) (int)(val * 224.0 * (float)0x100) + 0x8000;

#define YJ_FLOAT_TO_8(val) (int)(val * 255.0);
#define UVJ_FLOAT_TO_8(val) (int)(val * 255.0) + 128;

/* Conversion from YUV 16 */

#define Y_16_TO_Y_8(val) ((val)>>8)
#define UV_16_TO_UV_8(val) ((val)>>8)

#define Y_16_TO_YJ_8(val)   ((((RECLIP(val, 0x1000, 0xEB00)-0x1000)*255)/219)>>8)
#define UV_16_TO_UVJ_8(val) ((((RECLIP(val, 0x1000, 0xF000)-0x1000)*255)/224)>>8)

/* Conversion from YUV 8 */

#define Y_8_TO_16(val) ((val)<<8)
#define UV_8_TO_16(val) ((val)<<8)

#define Y_8_TO_YJ_8(val)   gavl_y_8_to_yj_8[val]
#define UV_8_TO_UVJ_8(val) gavl_uv_8_to_uvj_8[val]

#define YJ_8_TO_Y_8(val)   gavl_yj_8_to_y_8[val]
#define UVJ_8_TO_UV_8(val) gavl_uvj_8_to_uv_8[val]

#define YJ_8_TO_Y_16(val)   gavl_yj_8_to_y_16[val]
#define UVJ_8_TO_UV_16(val) gavl_uvj_8_to_uv_16[val]


/* RGB -> YUV conversion */

/* 24 -> 8 */

#define RGB_24_TO_Y_8(r,g,b,y) y=(gavl_r_to_y[r]+gavl_g_to_y[g]+gavl_b_to_y[b])>>16;

#define RGB_24_TO_YUV_8(r,g,b,y,u,v)                                    \
  RGB_24_TO_Y_8(r,g,b,y);                                               \
  u=(gavl_r_to_u[r]+gavl_g_to_u[g]+gavl_b_to_u[b])>>16;                                \
  v=(gavl_r_to_v[r]+gavl_g_to_v[g]+gavl_b_to_v[b])>>16;

#define RGB_24_TO_YJ_8(r,g,b,y) y=(gavl_r_to_yj[r]+gavl_g_to_yj[g]+gavl_b_to_yj[b])>>16;

#define RGB_24_TO_YUVJ_8(r,g,b,y,u,v) \
  RGB_24_TO_YJ_8(r,g,b,y);                                              \
  u=(gavl_r_to_uj[r]+gavl_g_to_uj[g]+gavl_b_to_uj[b])>>16;                             \
  v=(gavl_r_to_vj[r]+gavl_g_to_vj[g]+gavl_b_to_vj[b])>>16;

/* 24 -> 16 */

#define RGB_24_TO_Y_16(r,g,b,y) y=(gavl_r_to_y[r]+gavl_g_to_y[g]+gavl_b_to_y[b])>>8;

#define RGB_24_TO_YUV_16(r,g,b,y,u,v)                                    \
  RGB_24_TO_Y_16(r,g,b,y);                                               \
  u=(gavl_r_to_u[r]+gavl_g_to_u[g]+gavl_b_to_u[b])>>8;                                \
  v=(gavl_r_to_v[r]+gavl_g_to_v[g]+gavl_b_to_v[b])>>8;

/* 48 -> 8 */


#define r_16_to_y ((int64_t)((0.29900*219.0/255.0)*0x10000))
#define g_16_to_y ((int64_t)((0.58700*219.0/255.0)*0x10000))
#define b_16_to_y ((int64_t)((0.11400*219.0/255.0)*0x10000))

#define r_16_to_u ((int64_t)(-(0.16874*224.0/255.0)*0x10000))
#define g_16_to_u ((int64_t)(-(0.33126*224.0/255.0)*0x10000))
#define b_16_to_u ((int64_t)( (0.50000*224.0/255.0)*0x10000))

#define r_16_to_v ((int64_t)( (0.50000*224.0/255.0)*0x10000))
#define g_16_to_v ((int64_t)(-(0.41869*224.0/255.0)*0x10000))
#define b_16_to_v ((int64_t)(-(0.08131*224.0/255.0)*0x10000))

#define r_16_to_yj ((int64_t)((0.29900)*0x10000))
#define g_16_to_yj ((int64_t)((0.58700)*0x10000))
#define b_16_to_yj ((int64_t)((0.11400)*0x10000))

#define r_16_to_uj ((int64_t)(-(0.16874)*0x10000))
#define g_16_to_uj ((int64_t)(-(0.33126)*0x10000))
#define b_16_to_uj ((int64_t)( (0.50000)*0x10000))

#define r_16_to_vj ((int64_t)( (0.50000)*0x10000))
#define g_16_to_vj ((int64_t)(-(0.41869)*0x10000))
#define b_16_to_vj ((int64_t)(-(0.08131)*0x10000))

#define RGB_48_TO_Y_8(r,g,b,y) \
  y=(r_16_to_y * r + g_16_to_y * g + b_16_to_y * b + 16 * 0x1000000LL)>>24;

#define RGB_48_TO_YUV_8(r,g,b,y,u,v)                                    \
  RGB_48_TO_Y_8(r,g,b,y);                                               \
  u=(r_16_to_u * r + g_16_to_u * g + b_16_to_u * b + 0x80000000LL)>>24;  \
  v=(r_16_to_v * r + g_16_to_v * g + b_16_to_v * b + 0x80000000LL)>>24;

#define RGB_48_TO_YJ_8(r,g,b,y) \
  y=(r_16_to_yj * r + g_16_to_yj * g + b_16_to_yj * b)>>24;

#define RGB_48_TO_YUVJ_8(r,g,b,y,u,v)                                    \
  RGB_48_TO_YJ_8(r,g,b,y);                                               \
  u=(r_16_to_uj * r + g_16_to_uj * g + b_16_to_uj * b + 0x80000000LL)>>24;  \
  v=(r_16_to_vj * r + g_16_to_vj * g + b_16_to_vj * b + 0x80000000LL)>>24;

/* 48 -> 16 */

#define RGB_48_TO_Y_16(r,g,b,y) \
  y=(r_16_to_y * r + g_16_to_y * g + b_16_to_y * b + 16 * 0x1000000LL)>>16;

#define RGB_48_TO_YUV_16(r,g,b,y,u,v)                                    \
  RGB_48_TO_Y_16(r,g,b,y);                                               \
  u=(r_16_to_u * r + g_16_to_u * g + b_16_to_u * b + 0x80000000LL)>>16;  \
  v=(r_16_to_v * r + g_16_to_v * g + b_16_to_v * b + 0x80000000LL)>>16;

/* RGB Float -> YUV */

#define r_float_to_y  0.29900
#define g_float_to_y  0.58700
#define b_float_to_y  0.11400

#define r_float_to_u  (-0.16874)
#define g_float_to_u  (-0.33126)
#define b_float_to_u   0.50000

#define r_float_to_v   0.50000
#define g_float_to_v  (-0.41869)
#define b_float_to_v  (-0.08131)

#define INIT_RGB_FLOAT_TO_YUV \
  float y_tmp, u_tmp, v_tmp;

/* Float -> 8 */

#define RGB_FLOAT_TO_Y_8(r, g, b, y)                              \
  y_tmp = r_float_to_y * r + g_float_to_y * g + b_float_to_y * b; \
  y = Y_FLOAT_TO_8(y_tmp);

#define RGB_FLOAT_TO_YUV_8(r, g, b, y, u, v)                      \
  RGB_FLOAT_TO_Y_8(r, g, b, y)                                    \
  u_tmp = r_float_to_u * r + g_float_to_u * g + b_float_to_u * b; \
  v_tmp = r_float_to_v * r + g_float_to_v * g + b_float_to_v * b; \
  u = UV_FLOAT_TO_8(u_tmp);                                        \
  v = UV_FLOAT_TO_8(v_tmp);

#define RGB_FLOAT_TO_YJ_8(r, g, b, y)                              \
  y_tmp = r_float_to_y * r + g_float_to_y * g + b_float_to_y * b; \
  y = YJ_FLOAT_TO_8(y_tmp);

#define RGB_FLOAT_TO_YUVJ_8(r, g, b, y, u, v)                      \
  RGB_FLOAT_TO_YJ_8(r, g, b, y)                                    \
  u_tmp = r_float_to_u * r + g_float_to_u * g + b_float_to_u * b; \
  v_tmp = r_float_to_v * r + g_float_to_v * g + b_float_to_v * b; \
  u = UVJ_FLOAT_TO_8(u_tmp);                                        \
  v = UVJ_FLOAT_TO_8(v_tmp);

/* Float -> 16 */

#define RGB_FLOAT_TO_Y_16(r, g, b, y)                              \
  y_tmp = r_float_to_y * r + g_float_to_y * g + b_float_to_y * b; \
  y = Y_FLOAT_TO_16(y_tmp);

#define RGB_FLOAT_TO_YUV_16(r, g, b, y, u, v)                      \
  RGB_FLOAT_TO_Y_16(r, g, b, y)                                    \
  u_tmp = r_float_to_u * r + g_float_to_u * g + b_float_to_u * b; \
  v_tmp = r_float_to_v * r + g_float_to_v * g + b_float_to_v * b; \
  u = UV_FLOAT_TO_16(u_tmp);                                        \
  v = UV_FLOAT_TO_16(v_tmp);


/* YUV (8bit) -> */


#define YUV_8_TO_RGB_24(y,u,v,r,g,b) i_tmp=(gavl_y_to_rgb[y]+gavl_v_to_r[v])>>16;\
                               r=RECLIP_8(i_tmp);\
                               i_tmp=(gavl_y_to_rgb[y]+gavl_u_to_g[u]+gavl_v_to_g[v])>>16;\
                               g=RECLIP_8(i_tmp);\
                               i_tmp=(gavl_y_to_rgb[y]+gavl_u_to_b[u])>>16;\
                               b=RECLIP_8(i_tmp);

#define YUVJ_8_TO_RGB_24(y,u,v,r,g,b) i_tmp=(gavl_yj_to_rgb[y]+gavl_vj_to_r[v])>>16;\
                                r=RECLIP_8(i_tmp);\
                                i_tmp=(gavl_yj_to_rgb[y]+gavl_uj_to_g[u]+gavl_vj_to_g[v])>>16;\
                                g=RECLIP_8(i_tmp);\
                                i_tmp=(gavl_yj_to_rgb[y]+gavl_uj_to_b[u])>>16;\
                                b=RECLIP_8(i_tmp);

#define YUV_8_TO_RGB_48(y,u,v,r,g,b) i_tmp=(gavl_y_to_rgb[y]+gavl_v_to_r[v])>>8;\
                               r=RECLIP_16(i_tmp);\
                               i_tmp=(gavl_y_to_rgb[y]+gavl_u_to_g[u]+gavl_v_to_g[v])>>8;\
                               g=RECLIP_16(i_tmp);\
                               i_tmp=(gavl_y_to_rgb[y]+gavl_u_to_b[u])>>8;\
                               b=RECLIP_16(i_tmp);

#define YUVJ_8_TO_RGB_48(y,u,v,r,g,b) i_tmp=(gavl_yj_to_rgb[y]+gavl_vj_to_r[v])>>8;\
                                r=RECLIP_16(i_tmp);\
                                i_tmp=(gavl_yj_to_rgb[y]+gavl_uj_to_g[u]+gavl_vj_to_g[v])>>8;\
                                g=RECLIP_16(i_tmp);\
                                i_tmp=(gavl_yj_to_rgb[y]+gavl_uj_to_b[u])>>8;\
                                b=RECLIP_16(i_tmp);

#define YUV_8_TO_RGB_FLOAT(y,u,v,r,g,b)                                 \
  i_tmp=(gavl_y_to_rgb_float[y]+gavl_v_to_r_float[v]);                         \
  r=RECLIP_FLOAT(i_tmp);                                                   \
  i_tmp=(gavl_y_to_rgb_float[y]+gavl_u_to_g_float[u]+gavl_v_to_g_float[v]);         \
  g=RECLIP_FLOAT(i_tmp);                                                   \
  i_tmp=(gavl_y_to_rgb_float[y]+gavl_u_to_b_float[u]);                         \
  b=RECLIP_FLOAT(i_tmp);

#define YUVJ_8_TO_RGB_FLOAT(y,u,v,r,g,b)                                \
  i_tmp=(gavl_yj_to_rgb_float[y]+gavl_vj_to_r_float[v]);                       \
  r=RECLIP_FLOAT(i_tmp);                                                   \
  i_tmp=(gavl_yj_to_rgb_float[y]+gavl_uj_to_g_float[u]+gavl_vj_to_g_float[v]);      \
  g=RECLIP_FLOAT(i_tmp);                                                   \
  i_tmp=(gavl_yj_to_rgb_float[y]+gavl_uj_to_b_float[u]);                       \
  b=RECLIP_FLOAT(i_tmp);

/* YUV (16 bit) -> 8 */

#define y_16_to_rgb  ((int64_t)(255.0/219.0 * 0x10000))
#define v_16_to_r    ((int64_t)( 1.40200*255.0/224.0 * 0x10000))
#define u_16_to_g    ((int64_t)(-0.34414*255.0/224.0 * 0x10000))
#define v_16_to_g    ((int64_t)(-0.71414*255.0/224.0 * 0x10000))
#define u_16_to_b    ((int64_t)( 1.77200*255.0/224.0 * 0x10000))

#define YUV_16_TO_RGB_24(y,u,v,r,g,b) \
  i_tmp=(y_16_to_rgb * (y-0x1000) + v_16_to_r * (v-0x8000))>>24;        \
  r = RECLIP_8(i_tmp);\
  i_tmp=(y_16_to_rgb * (y-0x1000) + u_16_to_g * (u-0x8000)+ v_16_to_g * (v-0x8000))>>24; \
  g = RECLIP_8(i_tmp);\
  i_tmp=(y_16_to_rgb * (y-0x1000) + u_16_to_b * (u-0x8000))>>24; \
  b = RECLIP_8(i_tmp);


/* YUV (16 bit) -> 16 */

#define YUV_16_TO_RGB_48(y,u,v,r,g,b) \
  i_tmp=(y_16_to_rgb * (y-0x1000) + v_16_to_r * (v-0x8000))>>16;        \
  r = RECLIP_16(i_tmp);\
  i_tmp=(y_16_to_rgb * (y-0x1000) + u_16_to_g * (u-0x8000)+ v_16_to_g * (v-0x8000))>>16; \
  g = RECLIP_16(i_tmp);\
  i_tmp=(y_16_to_rgb * (y-0x1000) + u_16_to_b * (u-0x8000))>>16; \
  b = RECLIP_16(i_tmp);

/* YUV (16 bit) -> float */

#define y_16_to_rgb_float (255.0/219.0/65535.0)
#define v_16_to_r_float   (1.40200*255.0/224.0/65535.0)
#define u_16_to_g_float   (-0.34414*255.0/224.0/65535.0)
#define v_16_to_g_float   (-0.71414*255.0/224.0/65535.0)
#define u_16_to_b_float   (1.77200*255.0/224.0/65535.0)

#define YUV_16_TO_RGB_FLOAT(y,u,v,r,g,b) \
  i_tmp=(y_16_to_rgb_float * (y-0x1000) + v_16_to_r_float * (v-0x8000));        \
  r = RECLIP_FLOAT(i_tmp);\
  i_tmp=(y_16_to_rgb_float * (y-0x1000) + u_16_to_g_float * (u-0x8000)+ v_16_to_g_float * (v-0x8000)); \
  g = RECLIP_FLOAT(i_tmp);\
  i_tmp=(y_16_to_rgb_float * (y-0x1000) + u_16_to_b_float * (u-0x8000)); \
  b = RECLIP_FLOAT(i_tmp);


/* Combine r, g and b values to a 16 bit rgb pixel (taken from avifile) */

#define PACK_8_TO_RGB16(r,g,b,pixel) pixel=((((((r<<5)&0xff00)|g)<<6)&0xfff00)|b)>>3;
#define PACK_8_TO_BGR16(r,g,b,pixel) pixel=((((((b<<5)&0xff00)|g)<<6)&0xfff00)|r)>>3;

#define PACK_8_TO_RGB15(r,g,b,pixel) pixel=((((((r<<5)&0xff00)|g)<<5)&0xfff00)|b)>>3;
#define PACK_8_TO_BGR15(r,g,b,pixel) pixel=((((((b<<5)&0xff00)|g)<<5)&0xfff00)|r)>>3;

#define PACK_16_TO_RGB16(r,g,b,pixel) pixel=((((((r>>3)&0xff00)|(g>>8))<<6)&0xfff00)|(b>>8))>>3;
#define PACK_16_TO_BGR16(r,g,b,pixel) pixel=((((((b>>3)&0xff00)|(g>>8))<<6)&0xfff00)|(r>>8))>>3;

#define PACK_16_TO_RGB15(r,g,b,pixel) pixel=((((((r>>3)&0xff00)|(g>>8))<<5)&0xfff00)|(b>>8))>>3;
#define PACK_16_TO_BGR15(r,g,b,pixel) pixel=((((((b>>3)&0xff00)|(g>>8))<<5)&0xfff00)|(r>>8))>>3;


/* RGBA -> RGB */

#define INIT_RGBA_32 uint32_t anti_alpha;\
  int background_r=ctx->options->background_16[0] >> 8;    \
  int background_g=ctx->options->background_16[1] >> 8;    \
  int background_b=ctx->options->background_16[2] >> 8;

#define INIT_RGBA_64  uint32_t anti_alpha;\
  uint32_t background_r=ctx->options->background_16[0];  \
  uint32_t background_g=ctx->options->background_16[1];  \
  uint32_t background_b=ctx->options->background_16[2];

#define INIT_RGBA_FLOAT  float anti_alpha;                              \
  float background_r=ctx->options->background_float[0];                 \
  float background_g=ctx->options->background_float[1];                 \
  float background_b=ctx->options->background_float[2];

#define RGBA_32_TO_RGB_24(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b) \
anti_alpha = 0xFF - src_a;\
dst_r=((src_a*src_r)+(anti_alpha*background_r))>>8;\
dst_g=((src_a*src_g)+(anti_alpha*background_g))>>8;\
dst_b=((src_a*src_b)+(anti_alpha*background_b))>>8;

#define RGBA_32_TO_RGB_48(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b) \
anti_alpha = 0xFF - src_a;\
dst_r=((src_a*src_r)+(anti_alpha*background_r));\
dst_g=((src_a*src_g)+(anti_alpha*background_g));\
dst_b=((src_a*src_b)+(anti_alpha*background_b));

#define RGBA_64_TO_RGB_24(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b) \
  anti_alpha = 0xFFFF - src_a;                                          \
  dst_r=((src_a*src_r)+(anti_alpha*background_r))>>24; \
  dst_g=((src_a*src_g)+(anti_alpha*background_g))>>24; \
  dst_b=((src_a*src_b)+(anti_alpha*background_b))>>24;

#define RGBA_64_TO_RGB_48(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b) \
  anti_alpha = 0xFFFF - src_a;                                          \
  dst_r=((src_a*src_r)+(anti_alpha*background_r))>>16; \
  dst_g=((src_a*src_g)+(anti_alpha*background_g))>>16; \
  dst_b=((src_a*src_b)+(anti_alpha*background_b))>>16;

#define RGBA_64_TO_RGB_FLOAT(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b) \
  anti_alpha = 0xFFFF - src_a;                                          \
  dst_r=RGB_16_TO_FLOAT(((src_a*src_r)+(anti_alpha*background_r))>>16); \
  dst_g=RGB_16_TO_FLOAT(((src_a*src_g)+(anti_alpha*background_g))>>16); \
  dst_b=RGB_16_TO_FLOAT(((src_a*src_b)+(anti_alpha*background_b))>>16);


#define RGBA_FLOAT_TO_RGB_24(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b) \
  anti_alpha = 1.0 - src_a;                                             \
  dst_r=RGB_FLOAT_TO_8(src_a*src_r+anti_alpha*background_r);            \
  dst_g=RGB_FLOAT_TO_8(src_a*src_g+anti_alpha*background_g);            \
  dst_b=RGB_FLOAT_TO_8(src_a*src_b+anti_alpha*background_b);

#define RGBA_FLOAT_TO_RGB_48(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b) \
  anti_alpha = 1.0 - src_a;                                             \
  dst_r=RGB_FLOAT_TO_16(src_a*src_r+anti_alpha*background_r);            \
  dst_g=RGB_FLOAT_TO_16(src_a*src_g+anti_alpha*background_g);            \
  dst_b=RGB_FLOAT_TO_16(src_a*src_b+anti_alpha*background_b);

#define RGBA_FLOAT_TO_RGB_FLOAT(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b) \
  anti_alpha = 1.0 - src_a;                                             \
  dst_r=src_a*src_r+anti_alpha*background_r;            \
  dst_g=src_a*src_g+anti_alpha*background_g;            \
  dst_b=src_a*src_b+anti_alpha*background_b;

/* YUVA 32 -> YUV */

#define INIT_YUVA_32 uint32_t anti_alpha;\
  int background_y;    \
  int background_u;    \
  int background_v;    \
  RGB_48_TO_YUV_8(ctx->options->background_16[0], \
                  ctx->options->background_16[1], \
                  ctx->options->background_16[2],\
                  background_y, background_u, background_v);

#define YUVA_32_TO_YUV_8(src_y, src_u, src_v, src_a, dst_y, dst_u, dst_v) \
  anti_alpha = 0xFF - src_a;                                            \
  dst_y=((src_a*src_y)+(anti_alpha*background_y))>>8;                   \
  dst_u=((src_a*src_u)+(anti_alpha*background_u))>>8;                   \
  dst_v=((src_a*src_v)+(anti_alpha*background_v))>>8;
  
#define YUVA_32_TO_Y_8(src_y, src_a, dst_y)                \
  anti_alpha = 0xFF - src_a;                                            \
  dst_y=((src_a*src_y)+(anti_alpha*background_y))>>8;

#define YUVA_32_TO_YUVJ_8(src_y, src_u, src_v, src_a, dst_y, dst_u, dst_v) \
  anti_alpha = 0xFF - src_a;                                            \
  dst_y=Y_16_TO_YJ_8(((src_a*src_y)+(anti_alpha*background_y)));        \
  dst_u=UV_16_TO_UVJ_8(((src_a*src_u)+(anti_alpha*background_u)));      \
  dst_v=UV_16_TO_UVJ_8(((src_a*src_v)+(anti_alpha*background_v)));
  
#define YUVA_32_TO_YJ_8(src_y, src_a, dst_y)                \
  anti_alpha = 0xFF - src_a;                                            \
  dst_y=Y_16_TO_YJ_8(((src_a*src_y)+(anti_alpha*background_y)));


#define YUVA_32_TO_YUV_16(src_y, src_u, src_v, src_a, dst_y, dst_u, dst_v) \
  anti_alpha = 0xFF - src_a;                                            \
  dst_y=((src_a*src_y)+(anti_alpha*background_y));                   \
  dst_u=((src_a*src_u)+(anti_alpha*background_u));                   \
  dst_v=((src_a*src_v)+(anti_alpha*background_v));
  
#define YUVA_32_TO_Y_16(src_y, src_a, dst_y)               \
  anti_alpha = 0xFF - src_a;                                            \
  dst_y=((src_a*src_y)+(anti_alpha*background_y));


