#include <config.h>
#include "EffecTV.h"
#include <gavl/gavl.h>
#include <gmerlin/plugin.h>
#include <gmerlin/translation.h>

#define BG_EFFECTV_REUSE_OUTPUT (1<<0)

typedef struct
  {
  effect * e;
  gavl_video_frame_t * in_frame;  /* Not padded */
  gavl_video_frame_t * out_frame; /* Not padded */
  gavl_video_format_t format;

  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;

  int started;

  int flags;
  
  } bg_effectv_plugin_t;

void bg_effectv_destroy(void * priv);

void bg_effectv_connect_input_port(void * priv,
                                   bg_read_video_func_t func,
                                   void * data, int stream, int port);

void bg_effectv_set_input_format(void * priv, gavl_video_format_t * format, int port);

void * bg_effectv_create(effectRegisterFunc * f, int flags);

void bg_effectv_destroy(void*priv);

void bg_effectv_get_output_format(void * priv, gavl_video_format_t * format);

int bg_effectv_read_video(void * priv, gavl_video_frame_t * frame, int stream);

#define EFFECTV_SET_PARAM_INT(n) \
  if(!strcmp(#n, name)) \
    { \
    if(priv->n != val->val_i) \
      { \
      priv->n = val->val_i; \
      changed = 1; \
      } \
    }
     
#define EFFECTV_SET_PARAM_FLOAT(n) \
  if(!strcmp(#n, name)) \
    { \
    if(priv->n != val->val_f) \
      { \
      priv->n = val->val_f; \
      changed = 1; \
      } \
    }

#define EFFECTV_SET_PARAM_COLOR(n) \
  if(!strcmp(#n, name)) \
    { \
      int r, g, b; \
      r = (int)(val->val_color[0]*255.0+0.5); \
      if(r & ~0xFF) r = (-r) >> 31; \
      g = (int)(val->val_color[1]*255.0+0.5); \
      if(g & ~0xFF) g = (-g) >> 31; \
      b = (int)(val->val_color[2]*255.0+0.5); \
      if(b & ~0xFF) b = (-b) >> 31; \
      priv->n = ((0<<24) + (r<<16) + (g <<8) + (b)); \
    }

