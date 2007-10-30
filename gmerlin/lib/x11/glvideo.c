#include <string.h>

#include <x11/x11.h>
#include <x11/x11_window_private.h>

#include <GL/gl.h>

#define texture_non_power_of_two (1<<0)

typedef struct
  {
  GLenum texture_format;
  GLenum texture_type;
  GLuint texture;
  int extensions;
  int texture_width;
  int texture_height;
  
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
    {
    fprintf(stderr, "Got extension %s\n", name);
    return 1;
    }
  return 0;
  }

static void check_gl(bg_x11_window_t * win,
                     gavl_pixelformat_t * formats_ret,
                     gl_priv_t * priv)
  {
  int format_index = 0;
  
  const char * extensions;
  
  formats_ret[format_index++] = GAVL_RGB_24;
  formats_ret[format_index++] = GAVL_RGBA_32;
  
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

  fprintf(stderr, "init_gl\n");
  
  d->pixelformats = malloc(5*sizeof(*d->pixelformats));
  check_gl(d->win, d->pixelformats, priv);
  fprintf(stderr, "init_gl 1 %p\n", d->pixelformats);
  
  return 1;
  }

static int open_gl(driver_data_t * d)
  {
  bg_x11_window_t * w;
  gl_priv_t * priv;
  gavl_video_frame_t * dummy;
  gavl_video_format_t texture_format;
  fprintf(stderr, "open_gl 0\n");
  
  priv = (gl_priv_t *)(d->priv);
  w = d->win;
  /* Get the format */
  bg_x11_window_set_gl(w);

  fprintf(stderr, "open_gl 1\n");
  
  switch(d->pixelformat)
    {
    case GAVL_RGB_24:
      priv->texture_type   = GL_RGB;
      priv->texture_format = GL_UNSIGNED_BYTE;
      break;
    case GAVL_RGBA_32:
      priv->texture_type   = GL_RGBA;
      priv->texture_format = GL_UNSIGNED_BYTE;
      break;
    default:
      break;
    }

  gavl_video_format_copy(&texture_format, 
                         &w->video_format);

  if(!(priv->extensions & texture_non_power_of_two))
    {
    texture_format.frame_width = 1;
    texture_format.frame_height = 1;
    
    while(texture_format.frame_width < texture_format.image_width)
      texture_format.frame_width <<= 1;
    while(texture_format.frame_height < texture_format.image_height)
      texture_format.frame_height <<= 1;
    }
  else
    {
    texture_format.frame_width = texture_format.image_width;
    texture_format.frame_height = texture_format.image_height;
    }
  
  priv->texture_width = texture_format.frame_width;
  priv->texture_height = texture_format.frame_height;
  
  dummy = gavl_video_frame_create_nopad(&texture_format);

  glGenTextures(1,&priv->texture);
  glBindTexture(GL_TEXTURE_2D,priv->texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glDisable(GL_DEPTH_TEST);

  glTexImage2D(GL_TEXTURE_2D, 0,
               priv->texture_type,
               texture_format.frame_width,
               texture_format.frame_height,
               0,
               priv->texture_type,
               priv->texture_format,dummy->planes[0]);
  
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
  gl_priv_t * priv;
  bg_x11_window_t * w;
  float tex_x1, tex_x2, tex_y1, tex_y2;
  
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
  
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w->video_format.image_width,
                  w->video_format.image_height,
                  GL_RGB,GL_UNSIGNED_BYTE,f->planes[0]);

  /* Draw this */
  
  tex_x1 = w->src_rect.x / priv->texture_width;
  tex_y1 = w->src_rect.y / priv->texture_height;
  
  tex_x2 = (w->src_rect.x + w->src_rect.w) / priv->texture_width;
  tex_y2 = (w->src_rect.y + w->src_rect.h) / priv->texture_height;
  
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glBegin(GL_QUADS);
  glTexCoord2f(tex_x1,tex_y2); glVertex3f(0,                 0,                  0);
  glTexCoord2f(tex_x1,tex_y1); glVertex3f(0,                 w->dst_rect.h,0);
  glTexCoord2f(tex_x2,tex_y1); glVertex3f(w->dst_rect.w,w->dst_rect.h,0);
  glTexCoord2f(tex_x2,tex_y2); glVertex3f(w->dst_rect.w,0,                  0);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  bg_x11_window_unset_gl(w);
  bg_x11_window_swap_gl(w);
  }

static void destroy_frame_gl(driver_data_t * d, gavl_video_frame_t * f)
  {
  gavl_video_frame_destroy(f);
  }

static void close_gl(driver_data_t * d)
  {
  gl_priv_t * priv;
  bg_x11_window_t * w;
  
  priv = (gl_priv_t *)(d->priv);
  w = d->win;
  
  bg_x11_window_set_gl(w);
  glDeleteTextures(1, &priv->texture);
  bg_x11_window_unset_gl(w);
  }

static void cleanup_gl(driver_data_t * d)
  {
  gl_priv_t * priv;
  priv = (gl_priv_t *)(d->priv);
  free(priv);
  }

video_driver_t gl_driver =
  {
    .can_scale          = 1,
    .init               = init_gl,
    .open               = open_gl,
    .create_frame       = create_frame_gl,
    .put_frame          = put_frame_gl,
    .destroy_frame      = destroy_frame_gl,
    .close              = close_gl,
    .cleanup            = cleanup_gl,
  };
