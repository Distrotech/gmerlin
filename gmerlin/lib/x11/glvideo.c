/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
  gavl_rectangle_i_t src_rect;
  int dst_x, dst_y;
  } texture_t;

typedef struct
  {
  int extensions;
  
  //  texture_t video;
  //  texture_t * overlays;
  } gl_priv_t;

static int has_extension(const char * extensions,
                         const char * name)
  {
  int len;
  const char * pos;

  if(!extensions)
    return 0;
  
  len = strlen(name);
  while(1)
    {
    if(!(pos = strstr(extensions, name)))
      return 0;
    if((pos[len] == ' ') || (pos[len] == '\0'))
      return 1;
    extensions = pos + len;
    }
  
  return 0;
  }

static int check_gl(bg_x11_window_t * win,
                    gavl_pixelformat_t * formats_ret,
                    gl_priv_t * priv)
  {
  int format_index = 0;
    
  if(!win->gl_fbconfigs)
    {
    formats_ret[0] = GAVL_PIXELFORMAT_NONE;
    return 0;
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
  formats_ret[format_index] = GAVL_PIXELFORMAT_NONE;
  
  return 1;
  }

static int init_gl(driver_data_t * d)
  {
  gl_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  d->priv = priv;
  
  d->pixelformats = malloc(13*sizeof(*d->pixelformats));
  
  return check_gl(d->win, d->pixelformats, priv);
  }

static gavl_video_frame_t *
create_texture(gl_priv_t * glp,
               int width, int height,
               gavl_pixelformat_t pixelformat)
  {
  gavl_video_frame_t * ret;
  texture_t * priv;
  gavl_video_format_t texture_format;
  gavl_video_format_t image_format;
  
  priv = calloc(1, sizeof(*priv));
  
  switch(pixelformat)
    {
    case GAVL_GRAY_8:
      priv->type   = GL_LUMINANCE;
      priv->format = GL_UNSIGNED_BYTE;
      break;
    case GAVL_GRAYA_16:
      priv->type   = GL_LUMINANCE_ALPHA;
      priv->format = GL_UNSIGNED_BYTE;
      break;
    case GAVL_GRAY_16:
      priv->type   = GL_LUMINANCE;
      priv->format = GL_UNSIGNED_SHORT;
      break;
    case GAVL_GRAYA_32:
      priv->type   = GL_LUMINANCE_ALPHA;
      priv->format = GL_UNSIGNED_SHORT;
      break;
    case GAVL_GRAY_FLOAT:
      priv->type   = GL_LUMINANCE;
      priv->format = GL_FLOAT;
      break;
    case GAVL_GRAYA_FLOAT:
      priv->type   = GL_LUMINANCE_ALPHA;
      priv->format = GL_FLOAT;
      break;
    case GAVL_RGB_24:
      priv->type   = GL_RGB;
      priv->format = GL_UNSIGNED_BYTE;
      break;
    case GAVL_RGBA_32:
      priv->type   = GL_RGBA;
      priv->format = GL_UNSIGNED_BYTE;
      break;
    case GAVL_RGB_48:
      priv->type   = GL_RGB;
      priv->format = GL_UNSIGNED_SHORT;
      break;
    case GAVL_RGBA_64:
      priv->type   = GL_RGBA;
      priv->format = GL_UNSIGNED_SHORT;
      break;
    case GAVL_RGB_FLOAT:
      priv->type   = GL_RGB;
      priv->format = GL_FLOAT;
      break;
    case GAVL_RGBA_FLOAT:
      priv->type   = GL_RGBA;
      priv->format = GL_FLOAT;
      break;
    default:
      break;
    }
  memset(&image_format, 0, sizeof(image_format));
  memset(&texture_format, 0, sizeof(texture_format));
  
  image_format.image_width = width;
  image_format.image_height = height;
  image_format.pixelformat = pixelformat;
  image_format.pixel_width = 1;
  image_format.pixel_height = 1;

  gavl_video_format_set_frame_size(&image_format, 0, 0);
  
  gavl_video_format_copy(&texture_format, &image_format);
  
  if(!(glp->extensions & texture_non_power_of_two))
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

//  fprintf(stderr, "Create texture:\n");
//  gavl_video_format_dump(&image_format);
//  gavl_video_format_dump(&texture_format);
  
  priv->width  = texture_format.frame_width;
  priv->height = texture_format.frame_height;
  
  ret = gavl_video_frame_create_nopad(&texture_format);
  ret->user_data = priv;

  glGenTextures(1,&priv->texture);
  glBindTexture(GL_TEXTURE_2D,priv->texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glTexImage2D(GL_TEXTURE_2D, 0,
               GL_RGBA,
               // priv->type,
               texture_format.frame_width,
               texture_format.frame_height,
               0,
               priv->type,
               priv->format,
               ret->planes[0]);
  gavl_video_frame_destroy(ret);

  /* Create real frame */
  ret = gavl_video_frame_create(&image_format);
  ret->user_data = priv;
  
  return ret;
  }

static void destroy_frame_gl(driver_data_t * d, gavl_video_frame_t * f)
  {
  texture_t * t = f->user_data;

  bg_x11_window_set_gl(d->win);
  glDeleteTextures(1, &t->texture);
  gavl_video_frame_destroy(f);
  bg_x11_window_unset_gl(d->win);
  free(t);
  }

static int open_gl(driver_data_t * d)
  {
  bg_x11_window_t * w;
  gl_priv_t * priv;
  const char * extensions;
  
  priv = d->priv;
  w = d->win;
  
  bg_x11_window_start_gl(w);
  /* Get the format */
  bg_x11_window_set_gl(w);
  
  extensions = (const char *) glGetString(GL_EXTENSIONS);

  if(has_extension(extensions, "GL_ARB_texture_non_power_of_two"))
    priv->extensions |= texture_non_power_of_two;
  
  w->frame = create_texture(priv,
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
  
  ret = create_texture(d->priv,
                       w->video_format.image_width,
                       w->video_format.image_height,
                       w->video_format.pixelformat);
  
  return ret;
  }

static void put_frame_gl(driver_data_t * d, gavl_video_frame_t * f)
  {
  int i;
  gl_priv_t * priv;
  bg_x11_window_t * w;
  float tex_x1, tex_x2, tex_y1, tex_y2;
  float v_x1, v_x2, v_y1, v_y2;
  texture_t * t = f->user_data;
  
  priv = d->priv;
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
  
  glBindTexture(GL_TEXTURE_2D, t->texture);
  
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                  w->video_format.image_width,
                  w->video_format.image_height,
                  t->type,
                  t->format,
                  f->planes[0]);
  
  /* Draw this */
  
  tex_x1 = (w->src_rect.x+2.0) / t->width;
  tex_y1 = (w->src_rect.y+2.0) / t->height;
  
  tex_x2 = (w->src_rect.x + w->src_rect.w) / t->width;
  tex_y2 = (w->src_rect.y + w->src_rect.h) / t->height;
  
  glColor4f(w->background_color[0], 
            w->background_color[1], 
            w->background_color[2],
            1.0);
  
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glBegin(GL_QUADS);
  glTexCoord2f(tex_x1,tex_y2); glVertex3f(0,             0,             0);
  glTexCoord2f(tex_x1,tex_y1); glVertex3f(0,             w->dst_rect.h, 0);
  glTexCoord2f(tex_x2,tex_y1); glVertex3f(w->dst_rect.w, w->dst_rect.h, 0);
  glTexCoord2f(tex_x2,tex_y2); glVertex3f(w->dst_rect.w, 0,             0);
  glEnd();

  /* Draw overlays */
  if(w->num_overlay_streams)
    {
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for(i = 0; i < w->num_overlay_streams; i++)
      {
      if(!w->overlay_streams[i].active)
        continue;

      t = w->overlay_streams[i].ovl->user_data;
      
      glBindTexture(GL_TEXTURE_2D,t->texture);
      
      tex_x1 = (float)(t->src_rect.x) /
        t->width;
      
      tex_y1 = (float)(t->src_rect.y) /
        t->height;
    
      tex_x2 = (float)(t->src_rect.x + t->src_rect.w) /
        t->width;

      tex_y2 = (float)(t->src_rect.y + t->src_rect.h) /
        t->height;
      
      v_x1 = t->dst_x - w->src_rect.x;
      v_y1 = t->dst_y - w->src_rect.y;

      v_x2 = v_x1 + t->src_rect.w;
      v_y2 = v_y1 + t->src_rect.h;
      
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
  bg_x11_window_swap_gl(w);
  glDisable(GL_TEXTURE_2D);
  bg_x11_window_unset_gl(w);
  }

/* Overlay support */

static gavl_video_frame_t *
create_overlay_gl(driver_data_t* d, overlay_stream_t * str)
  {
  gavl_video_frame_t * ret;
  bg_x11_window_set_gl(d->win);

  ret = create_texture(d->priv,
                       str->format.image_width,
                       str->format.image_height,
                       str->format.pixelformat);

  bg_x11_window_unset_gl(d->win);
  return ret;
  }

static void
destroy_overlay_gl(driver_data_t* d, overlay_stream_t * str,
                  gavl_video_frame_t * frame)
  {
  destroy_frame_gl(d, frame);
  }


static void init_overlay_stream_gl(driver_data_t* d, overlay_stream_t * str)
  {
  gl_priv_t * priv;
  bg_x11_window_t * w;
  
  priv = d->priv;
  w = d->win;
  
  if(!gavl_pixelformat_is_rgb(w->overlay_streams[w->num_overlay_streams].format.pixelformat))
    w->overlay_streams[w->num_overlay_streams].format.pixelformat = GAVL_RGBA_32;
  

  w->num_overlay_streams++;
  }

static void set_overlay_gl(driver_data_t* d, overlay_stream_t * str)
  {
  gl_priv_t * priv;
  bg_x11_window_t * w;
  
  priv = d->priv;
  w = d->win;

  if(str->active)
    {
    texture_t * t = str->ovl->user_data;
    
    bg_x11_window_set_gl(w);
    glBindTexture(GL_TEXTURE_2D, t->texture);
    
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    str->format.image_width,
                    str->format.image_height,
                    t->type,
                    t->format,
                    str->ovl->planes[0]);
    bg_x11_window_unset_gl(w);
    }
  }

static void close_gl(driver_data_t * d)
  {
  gl_priv_t * priv;
  bg_x11_window_t * w;
  
  priv = d->priv;
  w = d->win;
  
  bg_x11_window_stop_gl(w);
  }

static void cleanup_gl(driver_data_t * d)
  {
  gl_priv_t * priv;
  priv = d->priv;
  free(priv);
  }

const video_driver_t gl_driver =
  {
    .name               = "OpenGL",
    .can_scale          = 1,
    .init               = init_gl,
    .open               = open_gl,
    .create_frame       = create_frame_gl,
    .put_frame          = put_frame_gl,
    .destroy_frame      = destroy_frame_gl,
    .close              = close_gl,
    .cleanup            = cleanup_gl,
    .init_overlay_stream = init_overlay_stream_gl,
    .set_overlay        = set_overlay_gl,
    .create_overlay     = create_overlay_gl,
    .destroy_overlay    = destroy_overlay_gl,
  };
