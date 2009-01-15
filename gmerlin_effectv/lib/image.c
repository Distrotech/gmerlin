/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * image.c: utilities for image processing.
 *
 */

#include <string.h>
#include <stdlib.h>
#include "EffecTV.h"
#include "utils.h"

/* Initializer is called from utils_init(). */
int image_init(effect * e)
  {
  e->stretching_buffer = (RGB32 *)calloc(e->video_area, sizeof(RGB32));
  e->background = (RGB32 *)calloc(e->video_area, sizeof(RGB32));
  e->diff = (unsigned char *)calloc(e->video_area, sizeof(unsigned char));
  e->diff2 = (unsigned char *)calloc(e->video_area, sizeof(unsigned char));
  if(e->stretching_buffer == NULL ||
     e->background == NULL ||
     e->diff == NULL ||
     e->diff2 == NULL)
    {
    return -1;
    }
  //  memset(e->diff2, 0, e->video_area * sizeof(unsigned char));

  return 0;
  }

void image_end(effect * e)
  {
  free(e->stretching_buffer);
  free(e->background);
  free(e->diff);
  free(e->diff2);
  }

void image_stretching_buffer_clear(effect * e, RGB32 color)
  {
  int i;
  RGB32 *p;

  p = e->stretching_buffer;
  for(i=0; i<e->video_area; i++) {
  *p++ = color;
  }
  }

void image_stretch(RGB32 *src, int src_width, int src_height,
                   RGB32 *dest, int dest_width, int dest_height)
  {
  int x, y;
  int sx, sy;
  int tx, ty;
  RGB32 *p;

  tx = src_width * 65536 / dest_width;
  ty = src_height * 65536 / dest_height;
  sy = 0;
  for(y=0; y<dest_height; y++) {
  p = src + (sy>>16) * src_width;
  sx = 0;
  for(x=0; x<dest_width; x++) {
  *dest++ = p[(sx>>16)];
  sx += tx;
  }
  sy += ty;
  }
  }

#if 0
static void image_stretch_to_screen_double(void)
  {
  int x, y;
  RGB32 *src, *dest1, *dest2;
  int width = video_width;
  int height = video_height;
  int swidth = screen_width;

  src = stretching_buffer;
  dest1 = (RGB32 *)screen_getaddress();
  dest2 = dest1 + swidth;

  for(y=0; y<height; y++) {
  for(x=0; x<width; x++) {
  dest1[x*2] = *src;
  dest1[x*2+1] = *src;
  dest2[x*2] = *src;
  dest2[x*2+1] = *src;
  src++;
  }
  dest1 += swidth*2;
  dest2 += swidth*2;
  }
  }

void image_stretch_to_screen(void)
  {
  if(screen_scale == 2) {
  image_stretch_to_screen_double();
  } else {
  image_stretch(stretching_buffer, video_width, video_height,
                (RGB32 *)screen_getaddress(), screen_width, screen_height);
  }
  }

#endif

/*
 * Collection of background subtraction functions
 */

/* checks only fake-Y value */
/* In these function Y value is treated as R*2+G*4+B. */

void image_set_threshold_y(effect * e, int threshold)
  {
  e->y_threshold = threshold * 7; /* fake-Y value is timed by 7 */
  }

void image_bgset_y(effect * e, RGB32 *src)
  {
  int i;
  int R, G, B;
  RGB32 *p;
  short *q;

  p = src;
  q = (short *)e->background;
  for(i=0; i<e->video_area; i++) {
  R = ((*p)&0xff0000)>>(16-1);
  G = ((*p)&0xff00)>>(8-2);
  B = (*p)&0xff;
  *q = (short)(R + G + B);
  p++;
  q++;
  }
  }

unsigned char *image_bgsubtract_y(effect * e, RGB32 *src)
  {
  int i;
  int R, G, B;
  RGB32 *p;
  short *q;
  unsigned char *r;
  int v;

  p = src;
  q = (short *)e->background;
  r = e->diff;
  for(i=0; i<e->video_area; i++) {
  R = ((*p)&0xff0000)>>(16-1);
  G = ((*p)&0xff00)>>(8-2);
  B = (*p)&0xff;
  v = (R + G + B) - (int)(*q);
  *r = ((v + e->y_threshold)>>24) | ((e->y_threshold - v)>>24);

  p++;
  q++;
  r++;
  }

  return e->diff;
  /* The origin of subtraction function is;
   * diff(src, dest) = (abs(src - dest) > threshold) ? 0xff : 0;
   *
   * This functions is transformed to;
   * (threshold > (src - dest) > -threshold) ? 0 : 0xff;
   *
   * (v + threshold)>>24 is 0xff when v is less than -threshold.
   * (v - threshold)>>24 is 0xff when v is less than threshold.
   * So, ((v + threshold)>>24) | ((threshold - v)>>24) will become 0xff when
   * abs(src - dest) > threshold.
   */
  }

/* Background image is refreshed every frame */
unsigned char *image_bgsubtract_update_y(effect * e, RGB32 *src)
  {
  int i;
  int R, G, B;
  RGB32 *p;
  short *q;
  unsigned char *r;
  int v;

  p = src;
  q = (short *)e->background;
  r = e->diff;
  for(i=0; i<e->video_area; i++) {
  R = ((*p)&0xff0000)>>(16-1);
  G = ((*p)&0xff00)>>(8-2);
  B = (*p)&0xff;
  v = (R + G + B) - (int)(*q);
  *q = (short)(R + G + B);
  *r = ((v + e->y_threshold)>>24) | ((e->y_threshold - v)>>24);

  p++;
  q++;
  r++;
  }

  return e->diff;
  }


/* The range of r, g, b are [0..7] */
void image_set_threshold_RGB(effect * e, int r, int g, int b)
  {
  unsigned char R, G, B;

  R = G = B = 0xff;
  R = R<<r;
  G = G<<g;
  B = B<<b;
  e->rgb_threshold = (RGB32)(R<<16 | G<<8 | B);
  }

void image_bgset_RGB(effect * e, RGB32 *src)
  {
  int i;
  RGB32 *p;

  p = e->background;
  for(i=0; i<e->video_area; i++) {
  *p++ = (*src++) & 0xfefefe;
  }
  }

unsigned char *image_bgsubtract_RGB(effect * e, RGB32 *src)
  {
  int i;
  RGB32 *p, *q;
  unsigned a, b;
  unsigned char *r;

  p = src;
  q = e->background;
  r = e->diff;
  for(i=0; i<e->video_area; i++) {
  a = (*p++)|0x1010100;
  b = *q++;
  a = a - b;
  b = a & 0x1010100;
  b = b - (b>>8);
  b = b ^ 0xffffff;
  a = a ^ b;
  a = a & e->rgb_threshold;
  *r++ = (0 - a)>>24;
  }
  return e->diff;
  }

unsigned char *image_bgsubtract_update_RGB(effect * e, RGB32 *src)
  {
  int i;
  RGB32 *p, *q;
  unsigned a, b;
  unsigned char *r;

  p = src;
  q = e->background;
  r = e->diff;
  for(i=0; i<e->video_area; i++) {
  a = *p|0x1010100;
  b = *q&0xfefefe;
  *q++ = *p++;
  a = a - b;
  b = a & 0x1010100;
  b = b - (b>>8);
  b = b ^ 0xffffff;
  a = a ^ b;
  a = a & e->rgb_threshold;
  *r++ = (0 - a)>>24;
  }
  return e->diff;
  }

/* noise filter for subtracted image. */
unsigned char *image_diff_filter(effect * e, unsigned char *diff)
  {
  int x, y;
  unsigned char *src, *dest;
  unsigned int count;
  unsigned int sum1, sum2, sum3;
  const int width = e->video_width;

  src = diff;
  dest = e->diff2 + width +1;
  for(y=1; y<e->video_height-1; y++) {
  sum1 = src[0] + src[width] + src[width*2];
  sum2 = src[1] + src[width+1] + src[width*2+1];
  src += 2;
  for(x=1; x<width-1; x++) {
  sum3 = src[0] + src[width] + src[width*2];
  count = sum1 + sum2 + sum3;
  sum1 = sum2;
  sum2 = sum3;
  *dest++ = (0xff*3 - count)>>24;
  src++;
  }
  dest += 2;
  }
	
  return e->diff2;
  }

/* Y value filters */
unsigned char *image_y_over(effect * e, RGB32 *src)
  {
  int i;
  int R, G, B, v;
  unsigned char *p = e->diff;

  for(i = e->video_area; i>0; i--) {
  R = ((*src)&0xff0000)>>(16-1);
  G = ((*src)&0xff00)>>(8-2);
  B = (*src)&0xff;
  v = e->y_threshold - (R + G + B);
  *p = (unsigned char)(v>>24);
  src++;
  p++;
  }

  return e->diff;
  }

unsigned char *image_y_under(effect * e, RGB32 *src)
  {
  int i;
  int R, G, B, v;
  unsigned char *p = e->diff;

  for(i = e->video_area; i>0; i--) {
  R = ((*src)&0xff0000)>>(16-1);
  G = ((*src)&0xff00)>>(8-2);
  B = (*src)&0xff;
  v = (R + G + B) - e->y_threshold;
  *p = (unsigned char)(v>>24);
  src++;
  p++;
  }

  return e->diff;
  }

/* tiny edge detection */
unsigned char *image_edge(effect * e, RGB32 *src)
  {
  int x, y;
  unsigned char *p, *q;
  int r, g, b;
  int ar, ag, ab;
  int w;

  p = (unsigned char *)src;
  q = e->diff2;
  w = e->video_width * sizeof(RGB32);

  for(y=0; y<e->video_height - 1; y++) {
  for(x=0; x<e->video_width - 1; x++) {
  b = p[0];
  g = p[1];
  r = p[2];
  ab = abs(b - p[4]);
  ag = abs(g - p[5]);
  ar = abs(r - p[6]);
  ab += abs(b - p[w]);
  ag += abs(g - p[w+1]);
  ar += abs(r - p[w+2]);
  b = ab+ag+ar;
  if(b > e->y_threshold) {
  *q = 255;
  } else {
  *q = 0;
  }
  q++;
  p += 4;
  }
  p += 4;
  *q++ = 0;
  }
  memset(q, 0, e->video_width);

  return e->diff2;
  }

/* horizontal flipping */
void image_hflip(effect * e, RGB32 *src, RGB32 *dest, int width, int height)
  {
  int x, y;

  src += width - 1;
  for(y=0; y<height; y++) {
  for(x=0; x<width; x++) {
  *dest++ = *src--;
  }
  src += width * 2;
  }
  }

