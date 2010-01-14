/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <string.h>

#include <x11/x11.h>
#include <x11/x11_window_private.h>

#include <GL/gl.h>

#define texture_non_power_of_two (1<<0)

typedef struct
  {
  GLuint texture;
  GLenum format;
  GLenum type;
  int width, height;

  /* For overlays only */
  int active;
  gavl_rectangle_i_t src_rect;
  int dst_x, dst_y;
  } texture_t;

typedef struct
  {
  int extensions;
  
  texture_t video;
  texture_t * overlays;
  } gl_priv_t;

static int has_extension(const char * extensions,
                         const char * name)
  {
  char end;
  char * pos;

  if(!(pos = strstr(extensions, name)))
    return 0;
  
  end = pos[strlen(name)];
  
  if((end == ' ') || (end == '\0'))
    return 1;
  return 0;
  }

static void check_gl(bg_x11_window_t * win,
                     gavl_pixelformat_t * formats_ret,
                     gl_priv_t * priv)
  {
  int format_index = 0;
  
  const char * extensions;

  if(!win->gl_vi)
    {
    formats_ret[0] = GAVL_PIXELFORMAT_NONE;
    return;
    }
  formats_ret[format_index++] = GAVL_RGB_24;
  formats_ret[format_index++] = GAVL_RGBA_32;
  formats_ret[format_index++] = GAVL_RGB_48;
  formats_ret[format_index++] = GAVL_RGBA_64;
  formats_ret[format_index++] = GAVL_RGB_FLOAT;
  formats_ret[format_index++] = GAVL_RGBA_FLOAT;

  formats_ret[format_index++] = GAVL_GRAY_8;
  formats_ret[format_index++] = GAVL_GRAYA_16;
  formats_ret[format_index++] = GAVL_GRAY_16;
  formats_ret[format_index++] = GAVL_GRAYA_32;
  formats_ret[format_index++] = GAVL_GRAY_FLOAT;
  formats_ret[format_index++] = GAVL_GRAYA_FLOAT;
  
  bg_x11_window_set_gl(win);
  
  extensions = (const char *) glGetString(GL_EXTENSIONS);

  if(has_extension(extensions, "GL_ARB_texture_non_power_of_two"))
    priv->extensions |= texture_non_power_of_two;
  
  bg_x11_window_unset_gl(win);
  
  formats_ret[format_index] = GAVL_PIXELFORMAT_NONE;
  return;
  }

static int init_gl(driver_data_t * d)
  {
  gl_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  d->priv = priv;
  
  d->pixelformats = malloc(13*sizeof(*d->pixelformats));
  check_gl(d->win, d->pixelformats, priv);
  
  return 1;
  }

static void create_texture(gl_priv_t * priv,
                           texture_t * ret,
                           int width, int height, gavl_pixelformat_t pixelformat)
  {
  gavl_video_frame_t * dummy;
  
  gavl_video_format_t texture_format;
  switch(pixelformat)
    {
    case GAVL_GRAY_8:
      ret->type   = GL_LUMINANCE;
      ret->format = GL_UNSIGNED_BYTE;
      break;
    case GAVL_GRAYA_16:
      ret->type   = GL_LUMINANCE_ALPHA;
      ret->format = GL_UNSIGNED_BYTE;
      break;
    case GAVL_GRAY_16:
      ret->type   = GL_LUMINANCE;
      ret->format = GL_UNSIGNED_SHORT;
      break;
    case GAVL_GRAYA_32:
      ret->type   = GL_LUMINANCE_ALPHA;
      ret->format = GL_UNSIGNED_SHORT;
      break;
    case GAVL_GRAY_FLOAT:
      ret->type   = GL_LUMINANCE;
      ret->format = GL_FLOAT;
      break;
    case GAVL_GRAYA_FLOAT:
      ret->type   = GL_LUMINANCE_ALPHA;
      ret->format = GL_FLOAT;
      break;
    case GAVL_RGB_24:
      ret->type   = GL_RGB;
      ret->format = GL_UNSIGNED_BYTE;
      break;
    case GAVL_RGBA_32:
      ret->type   = GL_RGBA;
      ret->format = GL_UNSIGNED_BYTE;
      break;
    case GAVL_RGB_48:
      ret->type   = GL_RGB;
      ret->format = GL_UNSIGNED_SHORT;
      break;
    case GAVL_RGBA_64:
      ret->type   = GL_RGBA;
      ret->format = GL_UNSIGNED_SHORT;
      break;
    case GAVL_RGB_FLOAT:
      ret->type   = GL_RGB;
      ret->format = GL_FLOAT;
      break;
    case GAVL_RGBA_FLOAT:
      ret->type   = GL_RGBA;
      ret->format = GL_FLOAT;
      break;
    default:
      break;
    }
  memset(&texture_format, 0, sizeof(texture_format));
  
  texture_format.image_width = width;
  texture_format.image_height = height;
  texture_format.pixelformat = pixelformat;
  texture_format.pixel_width = 1;
  texture_format.pixel_height = 1;
  
  if(!(priv->extensions & texture_non_power_of_two))
    {
    texture_format.frame_width = 1;
    texture_format.frame_height = 1;
    
    while(texture_format.frame_width < width)
      texture_format.frame_width <<= 1;
    while(texture_format.frame_height < height)
      texture_format.frame_height <<= 1;
    }
  else
    {
    texture_format.frame_width = texture_format.image_width;
    texture_format.frame_height = texture_format.image_height;
    }

  ret->width  = texture_format.frame_width;
  ret->height = texture_format.frame_height;
  
  
  dummy = gavl_video_frame_create_nopad(&texture_format);
  glGenTextures(1,&ret->texture);
  glBindTexture(GL_TEXTURE_2D,ret->texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glTexImage2D(GL_TEXTURE_2D, 0,
               ret->type,
               texture_format.frame_width,
               texture_format.frame_height,
               0,
               ret->type,
               ret->format,
               dummy->planes[0]);
  gavl_video_frame_destroy(dummy);
  }

static void delete_texture(texture_t * ret)
  {
  glDeleteTextures(1, &ret->texture);
  }

static int open_gl(driver_data_t * d)
  {
  bg_x11_window_t * w;
  gl_priv_t * priv;
  
  priv = (gl_priv_t *)(d->priv);
  w = d->win;
  /* Get the format */
  bg_x11_window_set_gl(w);
  
  create_texture(priv,
                 &priv->video,
                 w->video_format.image_width,
                 w->video_format.image_height,
                 d->pixelformat);
  
  glDisable(GL_DEPTH_TEST);
  
  glClearColor (0.0, 0.0, 0.0, 0.0);
  glShadeModel(GL_FLAT);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  
  /* Video format will have frame_size == image_size */
  
  w->video_format.frame_width  = w->video_format.image_width;
  w->video_format.frame_height = w->video_format.image_height;
  
  bg_x11_window_unset_gl(w);
  return 1;
  }

static gavl_video_frame_t * create_frame_gl(driver_data_t * d)
  {
  gavl_video_frame_t * ret;
  bg_x11_window_t * w;
  w = d->win;
  ret = gavl_video_frame_create_nopad(&w->video_format);
  return ret;
  }

static void put_frame_gl(driver_data_t * d, gavl_video_frame_t * f)
  {
  int i;
  gl_priv_t * priv;
  bg_x11_window_t * w;
  float tex_x1, tex_x2, tex_y1, tex_y2;
  float v_x1, v_x2, v_y1, v_y2;
  priv = (gl_priv_t *)(d->priv);
  w = d->win;

  bg_x11_window_set_gl(w);

  glClear(GL_COLOR_BUFFER_BIT);
  
  /* Setup Viewport */
  glViewport(w->dst_rect.x, w->dst_rect.y, w->dst_rect.w, w->dst_rect.h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, // Left
          w->dst_rect.w,// Right
          0,// Bottom
          w->dst_rect.h,// Top
          -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /* Put image into texture */
  glEnable(GL_TEXTURE_2D);
  
  glBindTexture(GL_TEXTURE_2D,priv->video.texture);
  
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                  w->video_format.image_width,
                  w->video_format.image_height,
                  priv->video.type,
                  priv->video.format,
                  f->planes[0]);
  
  /* Draw this */
  
  tex_x1 = (w->src_rect.x+2.0) / priv->video.width;
  tex_y1 = (w->src_rect.y+2.0) / priv->video.height;
  
  tex_x2 = (w->src_rect.x + w->src_rect.w) / priv->video.width;
  tex_y2 = (w->src_rect.y + w->src_rect.h) / priv->video.height;
  
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glBegin(GL_QUADS);
  glTexCoord2f(tex_x1,tex_y2); glVertex3f(0,             0,             0);
  glTexCoord2f(tex_x1,tex_y1); glVertex3f(0,             w->dst_rect.h, 0);
  glTexCoord2f(tex_x2,tex_y1); glVertex3f(w->dst_rect.w, w->dst_rect.h, 0);
  glTexCoord2f(tex_x2,tex_y2); glVertex3f(w->dst_rect.w, 0,             0);
  glEnd();

  /* Draw overlays */
  if(w->num_overlay_streams)
    {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for(i = 0; i < w->num_overlay_streams; i++)
      {
      if(!priv->overlays[i].active)
        continue;
      
      glBindTexture(GL_TEXTURE_2D,priv->overlays[i].texture);
      
      tex_x1 = (float)(priv->overlays[i].src_rect.x) /
        priv->overlays[i].width;
      
      tex_y1 = (float)(priv->overlays[i].src_rect.y) /
        priv->overlays[i].height;
    
      tex_x2 = (float)(priv->overlays[i].src_rect.x + priv->overlays[i].src_rect.w) /
        priv->overlays[i].width;

      tex_y2 = (float)(priv->overlays[i].src_rect.y + priv->overlays[i].src_rect.h) /
        priv->overlays[i].height;
      
      v_x1 = priv->overlays[i].dst_x - w->src_rect.x;
      v_y1 = priv->overlays[i].dst_y - w->src_rect.y;

      v_x2 = v_x1 + priv->overlays[i].src_rect.w;
      v_y2 = v_y1 + priv->overlays[i].src_rect.h;
      
      v_x1 = (v_x1 * w->dst_rect.w) / w->src_rect.w;
      v_x2 = (v_x2 * w->dst_rect.w) / w->src_rect.w;

      v_y1 = w->dst_rect.h - (v_y1 * w->dst_rect.h) / w->src_rect.h;
      v_y2 = w->dst_rect.h - (v_y2 * w->dst_rect.h) / w->src_rect.h;
      
      glBegin(GL_QUADS);
      glTexCoord2f(tex_x1,tex_y2); glVertex3f(v_x1, v_y2, 0.1);
      glTexCoord2f(tex_x1,tex_y1); glVertex3f(v_x1, v_y1, 0.1);
      glTexCoord2f(tex_x2,tex_y1); glVertex3f(v_x2, v_y1, 0.1);
      glTexCoord2f(tex_x2,tex_y2); glVertex3f(v_x2, v_y2, 0.1);
      glEnd();
      }
    glDisable(GL_BLEND);
    }
  glDisable(GL_TEXTURE_2D);
  
  bg_x11_window_unset_gl(w);
  bg_x11_window_swap_gl(w);
  }

static void destroy_frame_gl(driver_data_t * d, gavl_video_frame_t * f)
  {
  gavl_video_frame_destroy(f);
  }

/* Overlay support */

static void add_overlay_stream_gl(driver_data_t* d)
  {
  gl_priv_t * priv;
  bg_x11_window_t * w;
  
  priv = (gl_priv_t *)(d->priv);
  w = d->win;

  priv->overlays =
    realloc(priv->overlays,
            (w->num_overlay_streams+1) * sizeof(*(priv->overlays)));
  memset(&priv->overlays[w->num_overlay_streams], 0,
         sizeof(priv->overlays[w->num_overlay_streams]));
  
  if(!gavl_pixelformat_is_rgb(w->overlay_streams[w->num_overlay_streams].format.pixelformat))
    w->overlay_streams[w->num_overlay_streams].format.pixelformat = GAVL_RGBA_32;
  
  bg_x11_window_set_gl(w);

  create_texture(priv,
                 &priv->overlays[w->num_overlay_streams],
                 w->overlay_streams[w->num_overlay_streams].format.image_width,
                 w->overlay_streams[w->num_overlay_streams].format.image_height,
                 w->overlay_streams[w->num_overlay_streams].format.pixelformat);
  
  bg_x11_window_unset_gl(w);
  
  }

static void set_overlay_gl(driver_data_t* d, int stream, gavl_overlay_t * ovl)
  {
  gl_priv_t * priv;
  bg_x11_window_t * w;
  
  priv = (gl_priv_t *)(d->priv);
  w = d->win;

  if(ovl)
    {
    bg_x11_window_set_gl(w);
    glBindTexture(GL_TEXTURE_2D,priv->overlays[stream].texture);
    
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    w->overlay_streams[stream].format.image_width,
                    w->overlay_streams[stream].format.image_height,
                    priv->overlays[stream].type,
                    priv->overlays[stream].format,
                    ovl->frame->planes[0]);
    priv->overlays[stream].active = 1;
    bg_x11_window_unset_gl(w);

    gavl_rectangle_i_copy(&priv->overlays[stream].src_rect, &ovl->ovl_rect);
    priv->overlays[stream].dst_x = ovl->dst_x;
    priv->overlays[stream].dst_y = ovl->dst_y;
    }
  else
    priv->overlays[stream].active = 0;
  
  }


static gavl_video_frame_t * create_overlay_gl(driver_data_t* d, int stream)
  {
  return gavl_video_frame_create_nopad(&d->win->overlay_streams[stream].format);
  }

static void destroy_overlay_gl(driver_data_t* d, int stream, gavl_video_frame_t* ovl)
  {
  gavl_video_frame_destroy(ovl);
  }


static void close_gl(driver_data_t * d)
  {
  int i;
  gl_priv_t * priv;
  bg_x11_window_t * w;
  
  priv = (gl_priv_t *)(d->priv);
  w = d->win;
  
  bg_x11_window_set_gl(w);
  delete_texture(&priv->video);
  
  for(i = 0; i < w->num_overlay_streams; i++)
    delete_texture(&priv->overlays[i]);
  if(priv->overlays)
    {
    free(priv->overlays);
    priv->overlays = NULL; 
    }
  bg_x11_window_unset_gl(w);
  }

static void cleanup_gl(driver_data_t * d)
  {
  gl_priv_t * priv;
  priv = (gl_priv_t *)(d->priv);
  free(priv);
  }

const video_driver_t gl_driver =
  {
    .can_scale          = 1,
    .init               = init_gl,
    .open               = open_gl,
    .create_frame       = create_frame_gl,
    .put_frame          = put_frame_gl,
    .destroy_frame      = destroy_frame_gl,
    .close              = close_gl,
    .cleanup            = cleanup_gl,
    .add_overlay_stream = add_overlay_stream_gl,
    .set_overlay        = set_overlay_gl,
    .create_overlay     = create_overlay_gl,
    .destroy_overlay    = destroy_overlay_gl,
  };
