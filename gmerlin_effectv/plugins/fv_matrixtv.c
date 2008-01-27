/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * matrixTV - A Matrix Like effect.
 * This plugin for EffectTV is under GNU General Public License
 * See the "COPYING" that should be shiped with this source code
 * Copyright (C) 2001-2003 Monniez Christophe
 * d-fence@swing.be
 *
 * 2003/12/24 Kentaro Fukuchi
 * - Completely rewrote but based on Monniez's idea.
 * - Uses edge detection, not only G value of each pixel.
 * - Added 4x4 font includes number, alphabet and Japanese Katakana characters.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#include "matrixFont.xpm"
#define CHARNUM 80
#define FONT_W 4
#define FONT_H 4
#define FONT_DEPTH 4

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);


typedef struct {
	int mode;
	int y;
	int timer;
	int speed;
} Blip;

#define MODE_NONE 0
#define MODE_FALL 1
#define MODE_STOP 2
#define MODE_SLID 3

typedef struct
  {
  int stat;
  int mode;
  unsigned char font[CHARNUM * FONT_W * FONT_H];
  unsigned char *cmap;
  unsigned char *vmap;
  unsigned char *img;
  int mapW, mapH;
  RGB32 palette[256 * FONT_DEPTH];
  int pause;
  Blip *blips;
  } matrix_t;


static RGB32 green(unsigned int v);
static void setPalette(effect * e);
static void setPattern(effect * e);
static void drawChar(effect * e, RGB32 *dest, unsigned char c, unsigned char v);
static void createImg(effect * e, RGB32 *src);
static void updateCharMap(effect * e);

static effect *matrixRegister(void)
  {
  effect *entry;
  matrix_t * priv;

  entry = (effect *)calloc(1, sizeof(effect));
  if(entry == NULL)
    {
    return NULL;
    }

  priv = calloc(1, sizeof( *priv));
  entry->priv = priv;
  
  entry->start = start;
  entry->stop = stop;
  entry->draw = draw;
  
  return entry;
  }

static int start(effect * e)
  {
  matrix_t * priv = (matrix_t*)e->priv;
  priv->mapW = e->video_width / FONT_W;
  priv->mapH = e->video_height / FONT_H;
  priv->cmap = malloc(priv->mapW * priv->mapH);
  priv->vmap = malloc(priv->mapW * priv->mapH);
  priv->img = malloc(priv->mapW * priv->mapH);
  if(priv->cmap == NULL || priv->vmap == NULL || priv->img == NULL)
    {
    return -1;
    }
  
  priv->blips = (Blip *)malloc(priv->mapW * sizeof(Blip));
  if(priv->blips == NULL)
    {
    return -1;
    }
  
  setPattern(e);
  setPalette(e);
  
  memset(priv->cmap, CHARNUM - 1, priv->mapW * priv->mapH * sizeof(unsigned char));
  memset(priv->vmap, 0, priv->mapW * priv->mapH * sizeof(unsigned char));
  memset(priv->blips, 0, priv->mapW * sizeof(Blip));
  priv->pause = 0;
  
  priv->stat = 1;
  return 0;
  }

static int stop(effect * e)
  {
  matrix_t * priv = (matrix_t*)e->priv;
  priv->stat = 0;
  free(priv->cmap);
  free(priv->vmap);
  free(priv->img);
  free(priv->blips);
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
  {
  int x, y;
  RGB32 *p, *q;
  unsigned char *c, *v, *i;
  unsigned int val;
  RGB32 a, b;
  matrix_t * priv = (matrix_t*)e->priv;
  
  if(priv->pause == 0)
    {
    updateCharMap(e);
    createImg(e, src);
    }
  
  c = priv->cmap;
  v = priv->vmap;
  i = priv->img;

  p = dest;
  for(y=0; y<priv->mapH; y++)
    {
    q = p;
    for(x=0; x<priv->mapW; x++)
      {
      val = *i | *v;
      //			if(val > 255) val = 255;
      drawChar(e, q, *c, val);
      i++;
      v++;
      c++;
      q += FONT_W;
      }
    p += e->video_width * FONT_H;
    }

  if(priv->mode == 1)
    {
    for(x=0; x<e->video_area; x++)
      {
      a = *dest;
      b = *src++;
      b = (b & 0xfefeff) >> 1;
      *dest++ = a | b;
      }
    }

  return 0;
  }

/* Create edge-enhanced image data from the input */
static void createImg(effect * e, RGB32 *src)
  {
  int x, y;
  RGB32 *p;
  unsigned char *q;
  unsigned int val;
  RGB32 pc, pr, pb; //center, right, below
  int r, g, b;
  matrix_t * priv = (matrix_t*)e->priv;

  q = priv->img;

  for(y=0; y<priv->mapH; y++) {
  p = src;
  for(x=0; x<priv->mapW; x++) {
  pc = *p;
  pr = *(p + FONT_W - 1);
  pb = *(p + e->video_width * (FONT_H - 1));

  r = (int)(pc & 0xff0000) >> 15;
  g = (int)(pc & 0x00ff00) >> 7;
  b = (int)(pc & 0x0000ff) * 2;

  val = (r + 2*g + b) >> 5; // val < 64

  r -= (int)(pr & 0xff0000)>>16;
  g -= (int)(pr & 0x00ff00)>>8;
  b -= (int)(pr & 0x0000ff);
  r -= (int)(pb & 0xff0000)>>16;
  g -= (int)(pb & 0x00ff00)>>8;
  b -= (int)(pb & 0x0000ff);

  val += (r * r + g * g + b * b)>>5;

  if(val > 160) val = 160; // want not to make blip from the edge.
  *q = (unsigned char)val;

  p += FONT_W;
  q++;
  }
  src += e->video_width * FONT_H;
  }
  }

#define WHITE 0.45
static RGB32 green(unsigned int v)
  {
  unsigned int w;

  if(v < 256) {
  return ((int)(v*WHITE)<<16)|(v<<8)|(int)(v*WHITE);
  }

  w = v - (int)(256*WHITE);
  if(w > 255) w = 255;
  return (w << 16) + 0xff00 + w;
  }

static void setPalette(effect * e)
  {
  int i;
  matrix_t * priv = (matrix_t*)e->priv;

  for(i=0; i<256; i++)
    {
    priv->palette[i*FONT_DEPTH  ] = 0;
    priv->palette[i*FONT_DEPTH+1] = green(0x44 * i / 170);
    priv->palette[i*FONT_DEPTH+2] = green(0x99 * i / 170);
    priv->palette[i*FONT_DEPTH+3] = green(0xff * i / 170);
    }
  }

static void setPattern(effect * e)
  {
  int c, l, x, y, cx, cy;
  char *p;
  unsigned char v;
  matrix_t * priv = (matrix_t*)e->priv;

  /* FIXME: This code is highly depends on the structure of bundled */
  /*        matrixFont.xpm. */
  for(l = 0; l < 32; l++) {
  p = matrixFont[5 + l];
  cy = l /4;
  y = l % 4;
  for(c = 0; c < 40; c++) {
  cx = c / 4;
  x = c % 4;
  switch(*p) {
  case ' ':
    v = 0;
    break;
  case '.':
    v = 1;
    break;
  case 'o':
    v = 2; 
    break;
  case 'O':
  default:
    v = 3;
    break;
  }
  priv->font[(cy * 10 + cx) * FONT_W * FONT_H + y * FONT_W + x] = v;
  p++;
  }
  }
  }

static void drawChar(effect * e, RGB32 *dest, unsigned char c, unsigned char v)
  {
  matrix_t * priv = (matrix_t*)e->priv;
  int x, y, i;
  unsigned int *p;
  unsigned char *f;

  i = 0;
  if(v == 255) { // sticky characters
  v = 160;
  }

  p = &priv->palette[(int)v * FONT_DEPTH];
  f = &priv->font[(int)c * FONT_W * FONT_H];
  for(y=0; y<FONT_H; y++) {
  for(x=0; x<FONT_W; x++) {
  *dest++ = p[*f];
  f++;
  }
  dest += e->video_width - FONT_W;
  }
  }

static void darkenColumn(effect * e, int);
static void blipNone(effect * e, int);
static void blipFall(effect * e, int);
static void blipStop(effect * e, int);
static void blipSlide(effect * e, int);

static void updateCharMap(effect * e)
  {
  int x;
  matrix_t * priv = (matrix_t*)e->priv;

  for(x=0; x<priv->mapW; x++) {
  darkenColumn(e, x);
  switch(priv->blips[x].mode) {
  default:
  case MODE_NONE:
    blipNone(e, x);
    break;
  case MODE_FALL:
    blipFall(e, x);
    break;
  case MODE_STOP:
    blipStop(e, x);
    break;
  case MODE_SLID:
    blipSlide(e, x);
    break;
  }
  }
  }

static void darkenColumn(effect * e, int x)
  {
  int y;
  unsigned char *p;
  int v;
  matrix_t * priv = (matrix_t*)e->priv;

  p = priv->vmap + x;
  for(y=0; y<priv->mapH; y++) {
  v = *p;
  if(v < 255) {
  v *= 0.9;
  *p = v;
  }
  p += priv->mapW;
  }
  }

static void blipNone(effect * e, int x)
  {
  matrix_t * priv = (matrix_t*)e->priv;
  unsigned int r;

  // This is a test code to reuse a randome number for multi purpose. :-P
  // Of course it isn't good code because fastrand() doesn't generate ideal
  // randome numbers.
  r = inline_fastrand(e);

  if((r & 0xf0) == 0xf0) {
  priv->blips[x].mode = MODE_FALL;
  priv->blips[x].y = 0;
  priv->blips[x].speed = (r >> 30) + 1;
  priv->blips[x].timer = 0;
  } else if((r & 0x0f000) ==  0x0f000) {
  priv->blips[x].mode = MODE_SLID;
  priv->blips[x].timer = (r >> 28) + 15;
  priv->blips[x].speed = ((r >> 24) & 3) + 2;
  }
  }

static void blipFall(effect * e, int x)
  {
  int i, y;
  unsigned char *p, *c;
  unsigned int r;
  matrix_t * priv = (matrix_t*)e->priv;

  y = priv->blips[x].y;
  p = priv->vmap + x + y * priv->mapW;
  c = priv->cmap + x + y * priv->mapW;

  for(i=priv->blips[x].speed; i>0; i--) {
  if(priv->blips[x].timer > 0) {
  *p = 255;
  } else {
  *p = 254 - i * 10;
  }
  *c = inline_fastrand(e) % CHARNUM;
  p += priv->mapW;
  c += priv->mapW;
  y++;
  if(y >= priv->mapH) break;
  }
  if(priv->blips[x].timer > 0) {
  priv->blips[x].timer--;
  }

  if(y >= priv->mapH) {
  priv->blips[x].mode = MODE_NONE;
  }

  priv->blips[x].y = y;

  if(priv->blips[x].timer == 0) {
  r = inline_fastrand(e);
  if((r & 0x3f00) == 0x3f00) {
  priv->blips[x].timer = (r >> 28) + 8;
  } else if(priv->blips[x].speed > 1 && (r & 0x7f) == 0x7f) {
  priv->blips[x].mode = MODE_STOP;
  priv->blips[x].timer = (r >> 26) + 30;
  }
  }
  }

static void blipStop(effect * e, int x)
  {
  int y;
  matrix_t * priv = (matrix_t*)e->priv;

  y = priv->blips[x].y;
  priv->vmap[x + y * priv->mapW] = 254;
  priv->cmap[x + y * priv->mapW] = inline_fastrand(e) % CHARNUM;

  priv->blips[x].timer--;

  if(priv->blips[x].timer < 0) {
  priv->blips[x].mode = MODE_FALL;
  }
  }

static void blipSlide(effect * e, int x)
  {
  int y, dy;
  unsigned char *p;
  matrix_t * priv = (matrix_t*)e->priv;

  priv->blips[x].timer--;
  if(priv->blips[x].timer < 0) {
  priv->blips[x].mode = MODE_NONE;
  }

  p = priv->cmap + x + priv->mapW * (priv->mapH - 1);
  dy = priv->mapW * priv->blips[x].speed;

  for(y=priv->mapH - priv->blips[x].speed; y>0; y--) {
  *p = *(p - dy);
  p -= priv->mapW;
  }
  for(y=priv->blips[x].speed; y>0; y--) {
  *p = inline_fastrand(e) % CHARNUM;
  p -= priv->mapW;
  }
  }


static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "mode",
      .long_name = TRS("Blend"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_SYNC,
    },
    { /* */ },
  };

static const bg_parameter_info_t * get_parameters(void * data)
  {
  return parameters;
  }

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t *val)
  {
  int changed;
  bg_effectv_plugin_t * vp = (bg_effectv_plugin_t *)data;
  matrix_t * priv = (matrix_t*)vp->e->priv;
  
  if(!name)
    return;
  EFFECTV_SET_PARAM_INT(mode);
  }

static void * create_matrixtv()
  {
  return bg_effectv_create(matrixRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_matrixtv",
      .long_name = TRS("MatrixTV"),
      .description = TRS("The Matrix's visual effect has been metamorphosed to the realtime video effect. Edge-enhanced input image is reflected to the brightness of falling letters. Blending with the input image is also available. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_matrixtv,
      .destroy =   bg_effectv_destroy,
      .get_parameters =   get_parameters,
      .set_parameter =    set_parameter,
      .priority =         1,
    },
    
    .connect_input_port = bg_effectv_connect_input_port,
    
    .set_input_format = bg_effectv_set_input_format,
    .get_output_format = bg_effectv_get_output_format,

    .read_video = bg_effectv_read_video,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
