
/* Interpolate C = A * F + B * (1 - F) = F * (A - B) + B
 *  
 * before:
 * mm0: 00 A3 00 A2 00 A1 00 A0 (src)
 * mm1: 00 B3 00 B2 00 B1 00 B0 (dst)
 * mm4: 00 F3 00 F2 00 F1 00 F0 0..7f
 * mm5: 00 00 00 00 00 00 00 00 (Must be cleared before)
 *
 * after:
 * mm7: 00 00 00 00 C3 C3 C1 C0
 * mm0: destroyed
 */

static mmx_t interpolate_rgb16_upper_mask =   { 0x000000000000f800LL };
static mmx_t interpolate_rgb16_middle_mask =  { 0x00000000000007e0LL };
static mmx_t interpolate_rgb16_lower_mask =   { 0x000000000000001fLL };

static mmx_t interpolate_rgb15_upper_mask =   { 0x0000000000007C00LL };
static mmx_t interpolate_rgb15_middle_mask =  { 0x00000000000003e0LL };
static mmx_t interpolate_rgb15_lower_mask =   { 0x000000000000001fLL };

#define INTERPOLATE_INIT_TEMP \
  mmx_t tmp;\
  pxor_r2r(mm5,mm5);

#define INTERPOLATE_1D \
  movq_r2r(mm0, mm7);    /* mm7: A */                        \
  psubsw_r2r(mm1, mm7);  /* mm7: A-B */                      \
  pmullw_r2r(mm4, mm7);  /* mm7: F * (A_B) */                \
  psraw_i2r(7, mm7);     /* fit into 8 bits  */              \
  paddsw_r2r(mm1, mm7);  /* mm7: F * (A_B)+B */

#define INTERPOLATE_LOAD_SRC_RGBA32 \
  movd_m2r(*src, mm0);\
  movq_r2r(mm0, mm4);\
  punpcklbw_r2r(mm5, mm0);\
  psrlq_i2r(25, mm4);\
  movq_r2r(mm4, mm6);\
  psllq_i2r(16, mm6);\
  por_r2r(mm6, mm4);\
  movq_r2r(mm4, mm6);\
  psllq_i2r(32, mm6);\
  por_r2r(mm6, mm4);
  

#define INTERPOLATE_WRITE_RGBA32 \
  packuswb_r2r(mm5, mm7);/* Pack result */\
  movd_r2m(mm7, *dst);

#define INTERPOLATE_WRITE_RGB32 \
  packuswb_r2r(mm5, mm7);/* Pack result */\
  movd_r2m(mm7, *dst); \
  dst[3] = 0xff;

#define INTERPOLATE_WRITE_RGB24 \
  MOVQ_R2M(mm7, tmp); \
  dst[0] = tmp.ub[0]; \
  dst[1] = tmp.ub[2]; \
  dst[2] = tmp.ub[4];

#define INTERPOLATE_WRITE_BGR24 \
  MOVQ_R2M(mm7, tmp); \
  dst[0] = tmp.ub[4]; \
  dst[1] = tmp.ub[2]; \
  dst[2] = tmp.ub[0];

#define INTERPOLATE_WRITE_15 \
  psrlw_i2r(3, mm7);\
  movq_r2r(mm7,mm6);\
  pand_m2r(interpolate_rgb15_lower_mask, mm6);\
  movq_r2r(mm6,mm0);\
  movq_r2r(mm7,mm6);\
  psrlq_i2r(11, mm6);\
  pand_m2r(interpolate_rgb15_middle_mask, mm6);\
  por_r2r(mm6,mm0);\
  psrlq_i2r(22, mm7);\
  pand_m2r(interpolate_rgb15_upper_mask, mm7);\
  por_r2r(mm7,mm0);\
  MOVQ_R2M(mm0, tmp);\
  *dst = tmp.uw[0];

#define INTERPOLATE_WRITE_15_SWAP \
  psrlw_i2r(3, mm7);\
  movq_r2r(mm7,mm6);\
  pand_m2r(interpolate_rgb15_lower_mask, mm6);\
  movq_r2r(mm6,mm0);\
  psllw_i2r(10, mm0);\
  movq_r2r(mm7,mm6);\
  psrlq_i2r(11, mm6);\
  pand_m2r(interpolate_rgb15_middle_mask, mm6);\
  por_r2r(mm6,mm0);\
  psrlq_i2r(32, mm7);\
  por_r2r(mm7,mm0);\
  MOVQ_R2M(mm0, tmp);\
  *dst = tmp.uw[0];

#define INTERPOLATE_WRITE_16 \
  psrlw_i2r(2, mm7);\
  movq_r2r(mm7,mm6);\
  psrlw_i2r(1, mm6);\
  pand_m2r(interpolate_rgb16_lower_mask, mm6);\
  movq_r2r(mm6,mm0);\
  movq_r2r(mm7,mm6);                              \
  psrlq_i2r(11, mm6);\
  pand_m2r(interpolate_rgb16_middle_mask, mm6);\
  por_r2r(mm6,mm0);\
  psrlq_i2r(22, mm7);\
  pand_m2r(interpolate_rgb16_upper_mask, mm7);\
  por_r2r(mm7,mm0);\
  MOVQ_R2M(mm0, tmp);\
  *dst = tmp.uw[0];

#define INTERPOLATE_WRITE_16_SWAP \
  psrlw_i2r(2, mm7);\
  movq_r2r(mm7,mm6);\
  psrlw_i2r(1, mm6);\
  pand_m2r(interpolate_rgb16_lower_mask, mm6);\
  movq_r2r(mm6,mm0);\
  psllw_i2r(11, mm0);\
  movq_r2r(mm7,mm6);\
  psrlq_i2r(11, mm6);\
  pand_m2r(interpolate_rgb16_middle_mask, mm6);\
  por_r2r(mm6,mm0);\
  psrlq_i2r(33, mm7);\
  pand_m2r(interpolate_rgb16_lower_mask, mm7);\
  por_r2r(mm7,mm0);\
  MOVQ_R2M(mm0, tmp);\
  *dst = tmp.uw[0];
  
