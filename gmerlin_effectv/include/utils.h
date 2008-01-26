/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * utils.h: header file for utils
 *
 */

#ifndef __UTILS_H__
#define __UTILS_H__


/* DEFINE's by nullset@dookie.net */
#define RED(n)  ((n>>16) & 0x000000FF)
#define GREEN(n) ((n>>8) & 0x000000FF)
#define BLUE(n)  ((n>>0) & 0x000000FF)
#define RGB(r,g,b) ((0<<24) + ((r)<<16) + ((g) <<8) + (b))
#define INTENSITY(n)	( ( (RED(n)+GREEN(n)+BLUE(n))/3))

/*
 * utils.c
 */

int utils_init(effect*e);
void utils_end(effect*e);

void HSItoRGB(double H, double S, double I, int *r, int *g, int *b);

unsigned int fastrand(effect*e);
void fastsrand(effect*e, unsigned int);
#define inline_fastrand(e) (e->fastrand_val=e->fastrand_val*1103515245+12345)


/*
 * buffer.c
 */

// int sharedbuffer_init(void);
// void sharedbuffer_end(void);

/* The effects uses shared buffer must call this function at first in
 * each effect registrar.
 */
// void sharedbuffer_reset(void);

/* Allocates size bytes memory in shared buffer and returns a pointer to the
 * memory. NULL is returned when the rest memory is not enough for the request.
 */
// unsigned char *sharedbuffer_alloc(int);


/*
 * image.c
 */

int image_init(effect*e);
void image_end(effect*e);
void image_stretching_buffer_clear(effect*e,RGB32 color);
void image_stretch(RGB32 *, int, int, RGB32 *, int, int);

// void image_stretch_to_screen(void);

void image_set_threshold_y(effect * e, int threshold);
void image_bgset_y(effect * e, unsigned int *src);
unsigned char *image_bgsubtract_y(effect * e, unsigned int *src);
unsigned char *image_bgsubtract_update_y(effect * e, unsigned int *src);

void image_set_threshold_RGB(effect * e, int r, int g, int b);
void image_bgset_RGB(effect * e, unsigned int *src);
unsigned char *image_bgsubtract_RGB(effect * e, unsigned int *src);
unsigned char *image_bgsubtract_update_RGB(effect * e, unsigned int *src);

unsigned char *image_diff_filter(effect * e, unsigned char *diff);
unsigned char *image_y_over(effect * e, RGB32 *src);
unsigned char *image_y_under(effect * e, RGB32 *src);
unsigned char *image_edge(effect * e, RGB32 *src);

void image_hflip(effect * e, RGB32 *src, RGB32 *dest, int width, int height);

/*
 * yuv.c
 */


rgb2yuv_t * rgb2yuv_init(void);
yuv2rgb_t * yuv2rgb_init(void);

// unsigned char yuv_RGBtoY(int);

#endif /* __UTILS_H__ */
