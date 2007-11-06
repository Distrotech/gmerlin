#include <config.h>
#include <stdlib.h>

#include <lemuria.h>
#include <libvisual/libvisual.h>

/* Private context sensitive data goes here, */
typedef struct
  {
  lemuria_engine_t * e;
  } lemuria_t;

/* This function is called before we really start rendering, it's the init function */
static int lv_lemuria_init (VisPluginData *plugin)
  {
  lemuria_t *priv;
  /* Allocate the lemuria private data structure, and register it as a private */
  priv = calloc(1, sizeof(*priv));
  priv->e = lemuria_create();
  
  //priv = visual_mem_new0 (lemuria_t, 1);
  visual_object_set_private (VISUAL_OBJECT (plugin), priv);
  return 0;
  }

static int lv_lemuria_cleanup (VisPluginData *plugin)
  {
  lemuria_t *priv = (lemuria_t*)visual_object_get_private (VISUAL_OBJECT (plugin));
  
  lemuria_destroy(priv->e);
  free(priv);
  return 0;
  }

/* This is used to ask a plugin if it can handle a certain size, and if not, to
 * set the size it wants by putting a value in width, height that represents the
 * required size */
static int lv_lemuria_requisition (VisPluginData *plugin, int *width, int *height)
  {
  int reqw, reqh;
  
  /* Size negotiate with the window */
  reqw = *width;
  reqh = *height;
  
  if (reqw < 256)
    reqw = 256;
  
  if (reqh < 256)
    reqh = 256;
  
  *width = reqw;
  *height = reqh;
  
  return 0;
  }

static int lv_lemuria_dimension (VisPluginData *plugin, VisVideo *video, int width, int height)
  {
  lemuria_t *priv = (lemuria_t*)visual_object_get_private (VISUAL_OBJECT (plugin));
  
  lemuria_set_size(priv->e, width, height);
  return 0;
  }

/* This is the main event loop, where all kind of events can be handled, more information
 * regarding these can be found at:
 * http://libvisual.sourceforge.net/newdocs/docs/html/union__VisEvent.html 
 */

static int lv_lemuria_events (VisPluginData *plugin, VisEventQueue *events)
  {
  lemuria_t *priv = (lemuria_t*)visual_object_get_private (VISUAL_OBJECT (plugin));
  VisEvent ev;
  // VisParamEntry *param;
  
  while (visual_event_queue_poll (events, &ev)) 
    {
    switch (ev.type) 
      {
      case VISUAL_EVENT_KEYDOWN:
        switch(ev.event.keyboard.keysym.sym)
          {
          case VKEY_a:
            if(ev.event.keyboard.keysym.mod & VKMOD_CTRL)
              lemuria_set_foreground(priv->e);
            else
              lemuria_next_foreground(priv->e);
            break;
          case VKEY_w:
            if(ev.event.keyboard.keysym.mod & VKMOD_CTRL)
              lemuria_set_background(priv->e);
            else
              lemuria_next_background(priv->e);
            break;
          case VKEY_t:
            if(ev.event.keyboard.keysym.mod & VKMOD_CTRL)
              lemuria_set_texture(priv->e);
            else
              lemuria_next_texture(priv->e);
            break;
          case VKEY_F1:
            lemuria_print_help(priv->e);
            break;
          default:
            break;
          }
        break;
      case VISUAL_EVENT_RESIZE:
        lv_lemuria_dimension(plugin, ev.event.resize.video,
                             ev.event.resize.width, ev.event.resize.height);
        break;
        
      default: /* to avoid warnings */
        break;
      }
    }
  
  return 0;
  }

/* Using this function we can update the palette when we're in 8bits mode, which
 * we aren't with lemuria, so just ignore :) */
static VisPalette *lv_lemuria_palette (VisPluginData *plugin)
{
        return NULL;
}

/* This is where the real rendering happens! This function is what we call, many times
 * a second to get our graphical frames. */
static int lv_lemuria_render (VisPluginData *plugin, VisVideo *video, VisAudio *audio)
  {
  lemuria_t *priv = (lemuria_t*)visual_object_get_private (VISUAL_OBJECT (plugin));
  VisBuffer pcmb;
  float pcm[2][512];
  short pcms_0[512];
  short pcms_1[512];
  short * pcms[2];
  int i;
  
  visual_buffer_set_data_pair (&pcmb, pcm[0], sizeof (pcm[0]));
  visual_audio_get_sample (audio, &pcmb, VISUAL_AUDIO_CHANNEL_LEFT);
  
  visual_buffer_set_data_pair (&pcmb, pcm[1], sizeof (pcm[1]));
  visual_audio_get_sample (audio, &pcmb, VISUAL_AUDIO_CHANNEL_RIGHT);

  /* Ugly, but there really should be a way to get int16_t data from the VisAudio */
  for (i = 0; i < 512; i++)
    {
    pcms_0[i] = pcm[0][i] * 32768.0;
    pcms_1[i] = pcm[1][i] * 32768.0;
    }
  pcms[0] = pcms_0;
  pcms[1] = pcms_1;
  
  lemuria_update_audio(priv->e, pcms);
  lemuria_draw_frame(priv->e);
  return 0;
  }

VISUAL_PLUGIN_API_VERSION_VALIDATOR

extern const VisPluginInfo *get_plugin_info (int *count);


/* Main plugin stuff */
/* The get_plugin_info function provides the libvisual plugin registry, and plugin loader
 * with the very basic plugin information */
extern const VisPluginInfo *get_plugin_info (int *count)
  {
  /* Initialize the plugin specific data structure
   * with pointers to the functions that represent
   * the plugin interface it's implementation, more info:
   * http://libvisual.sourceforge.net/newdocs/docs/html/struct__VisActorPlugin.html */
  static VisActorPlugin actor[1];
  static VisPluginInfo info[1];
  
  actor[0].requisition = lv_lemuria_requisition;
  actor[0].palette = lv_lemuria_palette;
  actor[0].render = lv_lemuria_render;
  actor[0].vidoptions.depth = VISUAL_VIDEO_DEPTH_GL; /* We want GL clearly */
    
    
  info[0].type = VISUAL_PLUGIN_TYPE_ACTOR;
    
  info[0].plugname = "lemuria";
  info[0].name = "libvisual Lemuria";
  info[0].author = "Burkhard Plaum";
  info[0].version = VERSION;
  info[0].about = "OpenGL animations";
  info[0].help =  "If you wants this too long, you'll need some.";
  
  info[0].init = lv_lemuria_init;
  info[0].cleanup = lv_lemuria_cleanup;
  info[0].events = lv_lemuria_events;
  
  info[0].plugin = VISUAL_OBJECT (&actor[0]);
  *count = sizeof (info) / sizeof (*info);
  
  VISUAL_VIDEO_ATTRIBUTE_OPTIONS_GL_ENTRY(actor[0].vidoptions, VISUAL_GL_ATTRIBUTE_ALPHA_SIZE, 8);
  VISUAL_VIDEO_ATTRIBUTE_OPTIONS_GL_ENTRY(actor[0].vidoptions, VISUAL_GL_ATTRIBUTE_DEPTH_SIZE, 16);
  VISUAL_VIDEO_ATTRIBUTE_OPTIONS_GL_ENTRY(actor[0].vidoptions, VISUAL_GL_ATTRIBUTE_DOUBLEBUFFER, 1);
  VISUAL_VIDEO_ATTRIBUTE_OPTIONS_GL_ENTRY(actor[0].vidoptions, VISUAL_GL_ATTRIBUTE_RED_SIZE, 8);
  VISUAL_VIDEO_ATTRIBUTE_OPTIONS_GL_ENTRY(actor[0].vidoptions, VISUAL_GL_ATTRIBUTE_GREEN_SIZE, 8);
  VISUAL_VIDEO_ATTRIBUTE_OPTIONS_GL_ENTRY(actor[0].vidoptions, VISUAL_GL_ATTRIBUTE_BLUE_SIZE, 8);
  return info;
  }

