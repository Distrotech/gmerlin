#include <config.h>
#include <libvisual/libvisual.h>

#include <pluginregistry.h>
#include <translation.h>

#include <bglv.h>
#include <utils.h>

/* Fixme: for now, we can assume, that X11 is always present .... */
#include <x11/x11.h>

static int lv_initialized = 0;
pthread_mutex_t lv_initialized_mutex = PTHREAD_MUTEX_INITIALIZER;

#define SAMPLES_PER_FRAME 512


static void check_init()
  {
  char * argv = { "libgmerlin" };
  char ** argvp = &argv;
  int argc = 1;
  pthread_mutex_lock(&lv_initialized_mutex);
  if(lv_initialized)
    {
    pthread_mutex_unlock(&lv_initialized_mutex);
    return;
    }
  
  /* Initialize the library */
  visual_init(&argc, &argvp);
  lv_initialized = 1;
  pthread_mutex_unlock(&lv_initialized_mutex);
  }

bg_plugin_info_t * bg_lv_get_info(const char * filename)
  {
  bg_plugin_info_t * ret;
  VisPluginRef * ref;
  VisList * list;
  VisActor * actor;
  VisPluginInfo * info;
  char * tmp_string;
  const char * actor_name = (const char*)0;
  check_init();
  
  list = visual_plugin_get_registry();
  /* Find out if there is a plugin matching the filename */
  while((actor_name = visual_actor_get_next_by_name(actor_name)))
    {
    ref = visual_plugin_find(list, actor_name);
    if(ref && !strcmp(ref->file, filename))
      break;
    }
  if(!actor_name)
    return (bg_plugin_info_t *)0;
  
  actor = visual_actor_new(actor_name);
  
  if(!actor)
    return (bg_plugin_info_t *)0;

  ret = calloc(1, sizeof(*ret));

  info = visual_plugin_get_info(visual_actor_get_plugin(actor));
    
  fprintf(stderr, "Loaded %s\n", actor_name);
  
  ret->name        = bg_sprintf("vis_lv_%s", actor_name);
  ret->long_name   = bg_strdup((char*)0, info->name);
  ret->type        = BG_PLUGIN_VISUALIZATION;
  ret->api         = BG_PLUGIN_API_LV;
  ret->description = bg_sprintf(TR("libvisual plugin plugin"));
  ret->module_filename = bg_strdup((char*)0, filename);
  /* Optional info */
  if(info->author && *info->author)
    {
    tmp_string = bg_sprintf(TR("\nAuthor: %s"),
                            info->author);
    ret->description = bg_strcat(ret->description, tmp_string);
    free(tmp_string);
    }
  if(info->version && *info->version)
    {
    tmp_string = bg_sprintf(TR("\nVersion: %s"),
                            info->version);
    ret->description = bg_strcat(ret->description, tmp_string);
    free(tmp_string);
    }
  if(info->about && *info->about)
    {
    tmp_string = bg_sprintf(TR("\nAbout: %s"),
                            info->about);
    ret->description = bg_strcat(ret->description, tmp_string);
    free(tmp_string);
    }
  if(info->help && *info->help)
    {
    tmp_string = bg_sprintf(TR("\nHelp: %s"),
                            info->help);
    ret->description = bg_strcat(ret->description, tmp_string);
    free(tmp_string);
    }
  if(info->license && *info->license)
    {
    tmp_string = bg_sprintf(TR("\nLicense: %s"),
                            info->license);
    ret->description = bg_strcat(ret->description, tmp_string);
    free(tmp_string);
    }
  
  
  /* Check out if it's an OpenGL plugin */
  if(visual_actor_get_supported_depth(actor) &
     VISUAL_VIDEO_DEPTH_GL)
    ret->flags |=  BG_PLUGIN_VISUALIZE_GL;
  else
    ret->flags |=  BG_PLUGIN_VISUALIZE_FRAME;
  ret->priority = 1;
  visual_object_unref(VISUAL_OBJECT(actor));
  return ret;
  }

/* Private structure */

typedef struct
  {
  int gl;
  VisActor * actor;
  VisVideo * video;
  VisAudio * audio;
  gavl_audio_format_t audio_format;
  
  /* OpenGL */
  bg_x11_window_t * win;
  } lv_priv_t;

static void draw_frame_gl_lv(void * data, gavl_video_frame_t * frame)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  
  }

static void draw_frame_ov_lv(void * data, gavl_video_frame_t * frame)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  
  visual_video_set_buffer(priv->video, frame->planes[0]);
  visual_video_set_pitch(priv->video, frame->strides[0]);
  visual_actor_run(priv->actor, priv->audio);
  }

static void update_lv(void * data, gavl_audio_frame_t * frame)
  {
  lv_priv_t * priv;
  VisBuffer buffer;
  
  priv = (lv_priv_t*)data;
  
  visual_buffer_init(&buffer, frame->samples.s_16,
                     frame->valid_samples * 2, NULL);

  visual_audio_samplepool_input(priv->audio->samplepool,
                                &buffer,
                                VISUAL_AUDIO_SAMPLE_RATE_44100,
                                VISUAL_AUDIO_SAMPLE_FORMAT_S16,
                                VISUAL_AUDIO_SAMPLE_CHANNEL_STEREO);
#if 0
  gavl_audio_frame_copy(&priv->audio_format, priv->audio_frame, frame,
                        0, 0, SAMPLES_PER_FRAME, SAMPLES_PER_FRAME);
  visual_audio_analyze(priv->audio);
#endif
  }

static void close_lv(void * data)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  
  }

static void adjust_audio_format(gavl_audio_format_t * f)
  {
  f->num_channels = 2;
  f->channel_locations[0] = GAVL_CHID_NONE;
  gavl_set_channel_setup(f);
  f->interleave_mode = GAVL_INTERLEAVE_ALL;
  f->sample_format = GAVL_SAMPLE_S16;
  f->samples_per_frame = SAMPLES_PER_FRAME;
  }

static int
open_ov_lv(void * data, gavl_audio_format_t * audio_format,
           gavl_video_format_t * video_format)
  {
  int depths, depth;
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  adjust_audio_format(audio_format);
  
  visual_actor_realize(priv->actor);
  
  priv->video = visual_video_new();
  
  fprintf(stderr, "open_ov_lv\n");
  
  /* Get the depth */
  depths = visual_actor_get_supported_depth(priv->actor);

  if(depths & VISUAL_VIDEO_DEPTH_32BIT)    /**< 32 bits surface flag. */
    {
    video_format->pixelformat = GAVL_RGB_32;
    depth = VISUAL_VIDEO_DEPTH_32BIT;
    }
  else if(depths & VISUAL_VIDEO_DEPTH_24BIT)    /**< 24 bits surface flag. */
    {
    video_format->pixelformat = GAVL_RGB_24;
    depth = VISUAL_VIDEO_DEPTH_24BIT;
    }
  else if(depths & VISUAL_VIDEO_DEPTH_16BIT)    /**< 16 bits 5-6-5 surface flag. */
    {
    video_format->pixelformat = GAVL_RGB_16;
    depth = VISUAL_VIDEO_DEPTH_16BIT;
    }
  else if(depths & VISUAL_VIDEO_DEPTH_8BIT)    /**< 8 bits indexed surface flag. */
    {
    video_format->pixelformat = GAVL_RGB_24;
    depth = VISUAL_VIDEO_DEPTH_24BIT;
    }
  else
    return 0;
  
  visual_video_set_depth(priv->video, depth);
  visual_video_set_dimension(priv->video,
                             video_format->image_width,
                             video_format->image_height);
  
  visual_actor_set_video(priv->actor, priv->video);
  visual_actor_video_negotiate(priv->actor,
                               0, FALSE, FALSE);
  return 1;
  }

static int
open_gl_lv(void * data, gavl_audio_format_t * audio_format,
           const char * window_id)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  priv->win = bg_x11_window_create(window_id);
  adjust_audio_format(audio_format);
  
  gavl_audio_format_copy(&priv->audio_format, audio_format);
  
  /* Create an OpenGL context */
  
  return 1;
  }

static void show_frame_lv(void * data)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  }

/* High level load/unload */

int bg_lv_load(bg_plugin_handle_t * ret,
               const char * name, int plugin_flags)
  {
  lv_priv_t * priv;
  bg_visualization_plugin_t * p;
  
  check_init();
  
  /* Set up callbacks */
  p = calloc(1, sizeof(*p));
  ret->plugin = (bg_plugin_common_t*)p;
  
  if(plugin_flags & BG_PLUGIN_VISUALIZE_GL)
    {
    p->open_win = open_gl_lv;
    p->draw_frame = draw_frame_gl_lv;
    p->show_frame = show_frame_lv;
    }
  else
    {
    p->open_ov = open_ov_lv;
    p->draw_frame = draw_frame_ov_lv;
    }
  p->update = update_lv;
  p->close  = close_lv;
     
  /* Set up private data */
  priv = calloc(1, sizeof(*priv));
  ret->priv = priv;
  priv->audio = visual_audio_new();
  
  /* Remove gmerlin added prefix from the plugin name */
  priv->actor = visual_actor_new(name + 7);
  return 1;
  }

void bg_lv_unload(bg_plugin_handle_t * h)
  {
  check_init();
  
  }
