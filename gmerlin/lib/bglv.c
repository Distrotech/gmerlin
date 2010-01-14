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

#include <config.h>
#include <libvisual/libvisual.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/translation.h>
#include <gmerlin/keycodes.h>

#include <bglv.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

/* Fixme: for now, we assume, that X11 is always present... */
#include <x11/x11.h>

static int lv_initialized = 0;
pthread_mutex_t lv_initialized_mutex = PTHREAD_MUTEX_INITIALIZER;

static const int bg_attributes[VISUAL_GL_ATTRIBUTE_LAST] =
  {
    [VISUAL_GL_ATTRIBUTE_NONE]             = -1, /**< No attribute. */
    [VISUAL_GL_ATTRIBUTE_BUFFER_SIZE]      = BG_GL_ATTRIBUTE_BUFFER_SIZE,
    [VISUAL_GL_ATTRIBUTE_LEVEL]            = BG_GL_ATTRIBUTE_LEVEL,
    [VISUAL_GL_ATTRIBUTE_RGBA]             = BG_GL_ATTRIBUTE_RGBA,
    [VISUAL_GL_ATTRIBUTE_DOUBLEBUFFER]     = BG_GL_ATTRIBUTE_DOUBLEBUFFER,
    [VISUAL_GL_ATTRIBUTE_STEREO]           = BG_GL_ATTRIBUTE_STEREO,
    [VISUAL_GL_ATTRIBUTE_AUX_BUFFERS]      = BG_GL_ATTRIBUTE_AUX_BUFFERS,
    [VISUAL_GL_ATTRIBUTE_RED_SIZE]         = BG_GL_ATTRIBUTE_RED_SIZE,
    [VISUAL_GL_ATTRIBUTE_GREEN_SIZE]       = BG_GL_ATTRIBUTE_GREEN_SIZE,
    [VISUAL_GL_ATTRIBUTE_BLUE_SIZE]        = BG_GL_ATTRIBUTE_BLUE_SIZE,
    [VISUAL_GL_ATTRIBUTE_ALPHA_SIZE]       = BG_GL_ATTRIBUTE_ALPHA_SIZE,
    [VISUAL_GL_ATTRIBUTE_DEPTH_SIZE]       = BG_GL_ATTRIBUTE_DEPTH_SIZE,
    [VISUAL_GL_ATTRIBUTE_STENCIL_SIZE]     = BG_GL_ATTRIBUTE_STENCIL_SIZE,
    [VISUAL_GL_ATTRIBUTE_ACCUM_RED_SIZE]   = BG_GL_ATTRIBUTE_ACCUM_RED_SIZE,
    [VISUAL_GL_ATTRIBUTE_ACCUM_GREEN_SIZE] = BG_GL_ATTRIBUTE_ACCUM_GREEN_SIZE,
    [VISUAL_GL_ATTRIBUTE_ACCUM_BLUE_SIZE]  = BG_GL_ATTRIBUTE_ACCUM_BLUE_SIZE,
    [VISUAL_GL_ATTRIBUTE_ACCUM_ALPHA_SIZE] = BG_GL_ATTRIBUTE_ACCUM_ALPHA_SIZE,
  };

#define SAMPLES_PER_FRAME 512

static int key_callback(void * data, int key, int mask);
static int key_release_callback(void * data, int key, int mask);
static int motion_callback(void * data, int x, int y, int mask);
static int button_callback(void * data, int x, int y, int button, int mask);
static int button_release_callback(void * data, int x, int y, int button, int mask);

static void log_error(const char *message,
                      const char *funcname, void *priv)
  {
  char * domain;
  domain = bg_sprintf("lv.%s", funcname);
  bg_log_notranslate(BG_LOG_ERROR, domain, "%s", message);
  free(domain);
  }

static void log_info(const char *message,
                      const char *funcname, void *priv)
  {
  char * domain;
  domain = bg_sprintf("lv.%s", funcname);
  bg_log_notranslate(BG_LOG_INFO, domain, "%s", message);
  free(domain);
  }

static void log_warning(const char *message,
                        const char *funcname, void *priv)
  {
  char * domain;
  domain = bg_sprintf("lv.%s", funcname);
  bg_log_notranslate(BG_LOG_WARNING, domain, "%s", message);
  free(domain);
  }

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
  
  /* Set the log callbacks */
  visual_log_set_info_handler(log_info, (void*)0);
  visual_log_set_warning_handler(log_warning, (void*)0);
  visual_log_set_critical_handler(log_warning, (void*)0);
  visual_log_set_error_handler(log_error, (void*)0);
  
  lv_initialized = 1;
  pthread_mutex_unlock(&lv_initialized_mutex);
  }

static VisUIWidget * check_widget(VisUIWidget * w, const char * name,
                                  bg_parameter_info_t * info)
  {
  VisUIWidget * ret = (VisUIWidget*)0;
  int i;
  int num_items;
  VisListEntry * list_entry;
  switch(w->type)
    {
    case VISUAL_WIDGET_TYPE_RANGE:       /**< Range base widget: \a VisUIRange. */
    case VISUAL_WIDGET_TYPE_ENTRY:       /**< Entry box widget: \a VisUIEntry. */
    case VISUAL_WIDGET_TYPE_MUTATOR:     /**< Mutator base widget: \a VisUIMutator. */
    case VISUAL_WIDGET_TYPE_NULL:    /**< NULL widget */
    case VISUAL_WIDGET_TYPE_WIDGET:      /**< Base widget: \a VisUIWidget. */
    case VISUAL_WIDGET_TYPE_LABEL:       /**< Label widget: \a VisUILabel. */
    case VISUAL_WIDGET_TYPE_IMAGE:       /**< Image widget: \a VisUIImage. */
    case VISUAL_WIDGET_TYPE_SEPARATOR:   /**< Separator widget: \a VisUISeparator. */
    case VISUAL_WIDGET_TYPE_COLORPALETTE:/**< Color palette widget: \a VisUIColorPalette. */
    case VISUAL_WIDGET_TYPE_CHOICE:      /**< Choice base widget: \a VisUIChoice. */
      break;
    case VISUAL_WIDGET_TYPE_CONTAINER:   /**< Container widget: \a VisUIContainer. */
    case VISUAL_WIDGET_TYPE_FRAME:       /**< Frame widget: \a VisUIFrame. */
      /* Get child */
      return check_widget(VISUAL_UI_CONTAINER(w)->child, name, info);
      break;
    case VISUAL_WIDGET_TYPE_BOX:         /**< Box widget: \a VisUIBox. */
      /* Get children */
      list_entry = (VisListEntry*)0;
      while(visual_list_next(&(VISUAL_UI_BOX(w)->childs), &list_entry))
        {
        if((ret = check_widget(list_entry->data, name, info)))
          return ret;
        }
      break;
    case VISUAL_WIDGET_TYPE_TABLE:       /**< Table widget: \a VisUITable. */
      /* Get children */
      list_entry = (VisListEntry*)0;
      while(visual_list_next(&(VISUAL_UI_TABLE(w)->childs), &list_entry))
        {
        if((ret = check_widget(((VisUITableEntry*)list_entry->data)->widget, name, info)))
          {
          return ret;
          }
        }
      break;
    case VISUAL_WIDGET_TYPE_NOTEBOOK:    /**< Notebook widget: \a VisUINotebook. */
      /* Get children */
      /* Get children */
      list_entry = (VisListEntry*)0;
      while(visual_list_next(&(VISUAL_UI_NOTEBOOK(w)->childs), &list_entry))
        {
        if((ret = check_widget(list_entry->data, name, info)))
          return ret;
        }
      break;
    case VISUAL_WIDGET_TYPE_SLIDER:      /**< Slider widget: \a VisUISlider. */
      if(strcmp(name, VISUAL_UI_MUTATOR(w)->param->name))
        return 0;
      if((VISUAL_UI_MUTATOR(w)->param->type == VISUAL_PARAM_ENTRY_TYPE_FLOAT) ||
         (VISUAL_UI_MUTATOR(w)->param->type == VISUAL_PARAM_ENTRY_TYPE_DOUBLE))
        {
        info->type = BG_PARAMETER_SLIDER_FLOAT;
        info->val_min.val_f = VISUAL_UI_RANGE(w)->min;
        info->val_max.val_f = VISUAL_UI_RANGE(w)->max;
        info->num_digits = VISUAL_UI_RANGE(w)->precision;
        if((VISUAL_UI_MUTATOR(w)->param->type == VISUAL_PARAM_ENTRY_TYPE_FLOAT))
          info->val_default.val_f = VISUAL_UI_MUTATOR(w)->param->numeric.floating;
        else
          info->val_default.val_f = VISUAL_UI_MUTATOR(w)->param->numeric.doubleflt;
        }
      else
        {
        info->type = BG_PARAMETER_SLIDER_INT;
        info->val_min.val_i = (int)VISUAL_UI_RANGE(w)->min;
        info->val_max.val_i = (int)VISUAL_UI_RANGE(w)->max;
        info->val_default.val_i = VISUAL_UI_MUTATOR(w)->param->numeric.integer;
        }
      info->flags |= BG_PARAMETER_SYNC;
      ret = w;
      break;
    case VISUAL_WIDGET_TYPE_NUMERIC:     /**< Numeric widget: \a VisUINumeric. */
      if(strcmp(name, VISUAL_UI_MUTATOR(w)->param->name))
        return 0;
      if((VISUAL_UI_MUTATOR(w)->param->type == VISUAL_PARAM_ENTRY_TYPE_FLOAT) ||
         (VISUAL_UI_MUTATOR(w)->param->type == VISUAL_PARAM_ENTRY_TYPE_DOUBLE))
        {
        info->type = BG_PARAMETER_FLOAT;
        info->val_min.val_f = VISUAL_UI_RANGE(w)->min;
        info->val_max.val_f = VISUAL_UI_RANGE(w)->max;
        info->num_digits = VISUAL_UI_RANGE(w)->precision;
        if((VISUAL_UI_MUTATOR(w)->param->type == VISUAL_PARAM_ENTRY_TYPE_FLOAT))
          info->val_default.val_f = VISUAL_UI_MUTATOR(w)->param->numeric.floating;
        else
          info->val_default.val_f = VISUAL_UI_MUTATOR(w)->param->numeric.doubleflt;
        }
      else
        {
        info->type = BG_PARAMETER_INT;
        info->val_min.val_i = (int)VISUAL_UI_RANGE(w)->min;
        info->val_max.val_i = (int)VISUAL_UI_RANGE(w)->max;
        info->val_default.val_i = VISUAL_UI_MUTATOR(w)->param->numeric.integer;
        }
      info->flags |= BG_PARAMETER_SYNC;
      ret = w;
      break;
    case VISUAL_WIDGET_TYPE_COLOR:       /**< Color widget: \a VisUIColor. */
    case VISUAL_WIDGET_TYPE_COLORBUTTON: /**< Color button widget: \a VisUIColorButton. */
      if(strcmp(name, VISUAL_UI_MUTATOR(w)->param->name))
        return 0;
      info->type = BG_PARAMETER_COLOR_RGB;
      info->flags |= BG_PARAMETER_SYNC;
      info->val_default.val_color[0] = (float)VISUAL_UI_MUTATOR(w)->param->color.r / 255.0;
      info->val_default.val_color[1] = (float)VISUAL_UI_MUTATOR(w)->param->color.g / 255.0;
      info->val_default.val_color[2] = (float)VISUAL_UI_MUTATOR(w)->param->color.b / 255.0;
      ret = w;
      break;
    case VISUAL_WIDGET_TYPE_POPUP:       /**< Popup widget: \a VisUIPopup. */
    case VISUAL_WIDGET_TYPE_LIST:        /**< List widget: \a VisUIList. */
    case VISUAL_WIDGET_TYPE_RADIO:       /**< Radio widget: \a VisUIRadio. */
      if(strcmp(name, VISUAL_UI_MUTATOR(w)->param->name))
        return 0;
      info->type = BG_PARAMETER_STRINGLIST;
      info->flags |= BG_PARAMETER_SYNC;
      num_items = 0;
      list_entry = (VisListEntry*)0;
      while(visual_list_next(&(VISUAL_UI_CHOICE(w)->choices.choices), &list_entry))
        num_items++;
      info->multi_names_nc = calloc(num_items+1, sizeof(info->multi_names_nc));
      list_entry = (VisListEntry*)0;
      for(i = 0; i < num_items; i++)
        {
        visual_list_next(&(VISUAL_UI_CHOICE(w)->choices.choices), &list_entry);
        info->multi_names_nc[i] = bg_strdup((char*)0, ((VisUIChoiceEntry*)(list_entry->data))->name);

        /* Check if this is the current value */
        //        visual_param_entry_compare(((VisUIChoiceEntry*)(list_entry->data))->value,
        //                                       VISUAL_UI_MUTATOR(w)->param)
        if(!i)
          {
          info->val_default.val_str = bg_strdup((char*)0, info->multi_names_nc[i]);
          }
        }
      ret = w;
      break;
    case VISUAL_WIDGET_TYPE_CHECKBOX:     /**< Checkbox widget: \a VisUICheckbox. */
      if(strcmp(name, VISUAL_UI_MUTATOR(w)->param->name))
        return 0;
      info->type = BG_PARAMETER_CHECKBUTTON;
      info->flags |= BG_PARAMETER_SYNC;
      ret = w;
      break;
    }
  if(ret)
    info->help_string = bg_strdup(info->help_string, w->tooltip);
  bg_parameter_info_set_const_ptrs(info);
  return ret;
  }

static bg_parameter_info_t *
create_parameters(VisActor * actor, VisUIWidget *** widgets,
                  VisParamEntry *** params_ret)
  {
  int num_parameters, i, index, supported;
  bg_parameter_info_t * ret;
  VisParamContainer * params;
  //  VisHashmapChainEntry *entry;
  VisParamEntry *param_entry;
  VisListEntry * list_entry;
  VisUIWidget * widget;
  VisUIWidget * param_widget;
  
  params = visual_plugin_get_params(visual_actor_get_plugin(actor));
  
  /* Count parameters */
  num_parameters = 0;
  
  list_entry = (VisListEntry*)0;

  while(visual_list_next(&params->entries,
                         &list_entry))
    num_parameters++;

  if(!num_parameters)
    return (bg_parameter_info_t*)0;
  /* Create parameters */
  ret = calloc(num_parameters+1, sizeof(*ret));

  if(widgets)
    *widgets = calloc(num_parameters, sizeof(**widgets));

  if(params_ret)
    *params_ret = calloc(num_parameters, sizeof(**params_ret));
  
  list_entry = (VisListEntry*)0;
  index = 0;

  widget = visual_plugin_get_userinterface(visual_actor_get_plugin(actor));
  
  for(i = 0; i < num_parameters; i++)
    {
    visual_list_next(&params->entries, &list_entry);
    param_entry = list_entry->data;
    //    param_entry = VISUAL_PARAMENTRY(entry->data);
    
    if(params_ret)
      (*params_ret)[index] = param_entry;
    
    supported = 1;
    
    if(widget)
      param_widget = check_widget(widget, param_entry->name, &ret[index]);
    else
      param_widget = (VisUIWidget*)0;
    
    if(!param_widget)
      {
      switch(param_entry->type)
        {
        case VISUAL_PARAM_ENTRY_TYPE_NULL:     /**< No parameter. */
          supported = 0;
          break;
        case VISUAL_PARAM_ENTRY_TYPE_STRING:   /**< String parameter. */
          ret[index].type = BG_PARAMETER_STRING;
          ret[index].val_default.val_str =
            bg_strdup(ret[index].val_default.val_str,
                      param_entry->string);
          break;
        case VISUAL_PARAM_ENTRY_TYPE_INTEGER:  /**< Integer parameter. */
          ret[index].type = BG_PARAMETER_INT;
          ret[index].flags |= BG_PARAMETER_SYNC;
          ret[index].val_default.val_i = param_entry->numeric.integer;
          break;
        case VISUAL_PARAM_ENTRY_TYPE_FLOAT:    /**< Floating point parameter. */
          ret[index].type = BG_PARAMETER_FLOAT;
          ret[index].flags |= BG_PARAMETER_SYNC;
          ret[index].val_default.val_f = param_entry->numeric.floating;
          break;
        case VISUAL_PARAM_ENTRY_TYPE_DOUBLE:   /**< Double floating point parameter. */
          ret[index].type = BG_PARAMETER_FLOAT;
          ret[index].flags |= BG_PARAMETER_SYNC;
          ret[index].val_default.val_f = param_entry->numeric.doubleflt;
          break;
        case VISUAL_PARAM_ENTRY_TYPE_COLOR:    /**< VisColor parameter. */
          ret[index].type = BG_PARAMETER_COLOR_RGB;
          ret[index].flags |= BG_PARAMETER_SYNC;
          ret[index].val_default.val_color[0] = (float)param_entry->color.r / 255.0;
          ret[index].val_default.val_color[1] = (float)param_entry->color.g / 255.0;
          ret[index].val_default.val_color[2] = (float)param_entry->color.b / 255.0;
          break;
        case VISUAL_PARAM_ENTRY_TYPE_PALETTE:  /**< VisPalette parameter. */
        case VISUAL_PARAM_ENTRY_TYPE_OBJECT:   /**< VisObject parameter. */
        case VISUAL_PARAM_ENTRY_TYPE_END:      /**< List end, and used as terminator for VisParamEntry lists. */
          supported = 0;
          break;
        }
      }
    
    if(widgets)
      (*widgets)[index] = param_widget;
    
    if(!supported)
      continue;
    
    ret[index].name = bg_strdup((char*)0, param_entry->name);
    ret[index].long_name = bg_strdup((char*)0, param_entry->name);
    index++;
    }
  return ret;
  }

bg_plugin_info_t * bg_lv_get_info(const char * filename)
  {
  int i;
  VisVideoAttributeOptions *vidoptions;
  bg_x11_window_t * win;
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
    
  
  ret->name        = bg_sprintf("vis_lv_%s", actor_name);
  ret->long_name   = bg_strdup((char*)0, info->name);
  ret->type        = BG_PLUGIN_VISUALIZATION;
  ret->api         = BG_PLUGIN_API_LV;
  ret->description = bg_sprintf(TR("libvisual plugin"));
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
    {
    ret->flags |=  BG_PLUGIN_VISUALIZE_GL;
    
    win = bg_x11_window_create(NULL);
    
    /* Create an OpenGL context. For this, we need the OpenGL attributes */
    vidoptions = visual_actor_get_video_attribute_options(actor);
    for(i = 0; i < VISUAL_GL_ATTRIBUTE_LAST; i++)
      {
      if((vidoptions->gl_attributes[i].mutated) && (bg_attributes[i] >= 0))
        {
        bg_x11_window_set_gl_attribute(win, bg_attributes[i],
                                       vidoptions->gl_attributes[i].value);
        }
      }
    /* Set bogus dimensions, will be corrected by the size_callback */
    bg_x11_window_set_size(win, 640, 480);
    
    bg_x11_window_realize(win);
    bg_x11_window_set_gl(win);
    }
  else
    {
    ret->flags |=  BG_PLUGIN_VISUALIZE_FRAME;
    win = (bg_x11_window_t*)0;
    }
  ret->priority = 1;

  /* Must realize the actor to get the parameters */
  visual_actor_realize(actor);
  
  ret->parameters = create_parameters(actor, (VisUIWidget***)0, (VisParamEntry***)0);
  
  visual_object_unref(VISUAL_OBJECT(actor));
  
  if(win)
    {
    bg_x11_window_unset_gl(win);
    bg_x11_window_destroy(win);
    }
  
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
  bg_x11_window_callbacks_t window_callbacks;
  
  bg_parameter_info_t * parameters;
  VisUIWidget ** widgets;
  VisParamEntry ** params;
  
  bg_ov_callbacks_t * ov_callbacks;
  int have_audio;
  } lv_priv_t;

static void set_callbacks_lv(void * data, bg_ov_callbacks_t * cb)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  priv->ov_callbacks = cb;
  }

static void draw_frame_gl_lv(void * data, gavl_video_frame_t * frame)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  
  bg_x11_window_set_gl(priv->win);
  visual_actor_run(priv->actor, priv->audio);
  bg_x11_window_unset_gl(priv->win);
  priv->have_audio = 0;
  }

static void draw_frame_ov_lv(void * data, gavl_video_frame_t * frame)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  
  visual_video_set_buffer(priv->video, frame->planes[0]);
  visual_video_set_pitch(priv->video, frame->strides[0]);
  visual_actor_run(priv->actor, priv->audio);
  priv->have_audio = 0;
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
  visual_audio_analyze(priv->audio);
  }

static void close_lv(void * data)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  
  //  if(priv->win)
  //    bg_x11_window_show(priv->win, 0);
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
  
  /* Get the depth */
  depths = visual_actor_get_supported_depth(priv->actor);

  if(depths & VISUAL_VIDEO_DEPTH_32BIT)    /**< 32 bits surface flag. */
    {
    video_format->pixelformat = GAVL_BGR_32;
    depth = VISUAL_VIDEO_DEPTH_32BIT;
    }
  else if(depths & VISUAL_VIDEO_DEPTH_24BIT)    /**< 24 bits surface flag. */
    {
    video_format->pixelformat = GAVL_BGR_24;
    depth = VISUAL_VIDEO_DEPTH_24BIT;
    }
  else if(depths & VISUAL_VIDEO_DEPTH_16BIT)    /**< 16 bits 5-6-5 surface flag. */
    {
    video_format->pixelformat = GAVL_RGB_16;
    depth = VISUAL_VIDEO_DEPTH_16BIT;
    }
  else if(depths & VISUAL_VIDEO_DEPTH_8BIT)    /**< 8 bits indexed surface flag. */
    {
    video_format->pixelformat = GAVL_BGR_24;
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



static void size_changed(void * data, int width, int height)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  visual_video_set_dimension(priv->video, width, height);
  visual_actor_set_video(priv->actor, priv->video);
  visual_actor_video_negotiate(priv->actor, 0, FALSE, FALSE);
  }

static int
open_gl_lv(void * data, gavl_audio_format_t * audio_format,
           const char * window_id)
  {
  int width, height;
  
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;

  visual_video_set_depth(priv->video, VISUAL_VIDEO_DEPTH_GL);
  
  adjust_audio_format(audio_format);

    
  gavl_audio_format_copy(&priv->audio_format, audio_format);
  
  bg_x11_window_set_gl(priv->win);
  visual_actor_set_video(priv->actor, priv->video);
  bg_x11_window_unset_gl(priv->win);
  
  /* Set the size changed callback after initializing the libvisual stuff */
  bg_x11_window_set_callbacks(priv->win, &priv->window_callbacks);
  bg_x11_window_show(priv->win, 1);

  
  bg_x11_window_set_gl(priv->win);
  
  bg_x11_window_get_size(priv->win, &width, &height);
  
  /* We cannot use the size callback above since it's called before the
     actor is realized */
  visual_video_set_dimension(priv->video, width, height);
  visual_actor_video_negotiate(priv->actor, 0, FALSE, FALSE);
  
  bg_x11_window_unset_gl(priv->win);
  return 1;
  }

static void show_frame_lv(void * data)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  bg_x11_window_swap_gl(priv->win);
  
  bg_x11_window_set_gl(priv->win);
  bg_x11_window_handle_events(priv->win, 0);
  bg_x11_window_unset_gl(priv->win);
  }

static const bg_parameter_info_t * get_parameters_lv(void * data)
  {
  lv_priv_t * priv;
  priv = (lv_priv_t*)data;
  return priv->parameters;
  }

static void set_parameter_lv(void * data, const char * name,
                             const bg_parameter_value_t * val)
  {
  int supported;
  lv_priv_t * priv;
  int index;
  int i_tmp;
  uint8_t r, g, b;
  char * tmp_string;
  VisParamEntry * param;
  VisListEntry * list_entry;

  VisColor * color;
  const bg_parameter_info_t * info;
  
  if(!name)
    return;
  priv = (lv_priv_t*)data;

  info = bg_parameter_find(priv->parameters, name);
  if(!info)
    return;

  /* This would crash if multi_parameters were supported */
  index = info - priv->parameters;

  tmp_string = bg_strdup((char*)0, name);
  param = visual_param_entry_new(tmp_string);
  free(tmp_string);
  /* Menus have to be treated specially */
  if(info->type == BG_PARAMETER_STRINGLIST)
    {
    if(!priv->widgets[index])
      return;
    /* Get the selected index */
    supported = 0;
    list_entry = (VisListEntry*)0;
    while(visual_list_next(&(VISUAL_UI_CHOICE(priv->widgets[index])->choices.choices),
                           &list_entry))
      {
      if(!strcmp(((VisUIChoiceEntry*)(list_entry->data))->name, val->val_str))
        {
        visual_param_entry_set_from_param(param,
                                          ((VisUIChoiceEntry*)(list_entry->data))->value);
        supported = 1;
        break;
        }
      }
    }
  else
    {
    supported = 1;
    switch(priv->params[index]->type)
      {
      case VISUAL_PARAM_ENTRY_TYPE_NULL:     /**< No parameter. */
        supported = 0;
        break;
      case VISUAL_PARAM_ENTRY_TYPE_STRING:   /**< String parameter. */
        if(val->val_str)
          visual_param_entry_set_string(param, val->val_str);
        else
          supported = 0;
        break;
      case VISUAL_PARAM_ENTRY_TYPE_INTEGER:  /**< Integer parameter. */
        visual_param_entry_set_integer(param, val->val_i);
        break;
      case VISUAL_PARAM_ENTRY_TYPE_FLOAT:    /**< Floating point parameter. */
        visual_param_entry_set_float(param, val->val_f);
        break;
      case VISUAL_PARAM_ENTRY_TYPE_DOUBLE:   /**< Double floating point parameter. */
        visual_param_entry_set_double(param, val->val_f);
        break;
      case VISUAL_PARAM_ENTRY_TYPE_COLOR:    /**< VisColor parameter. */
        i_tmp = (int)(val->val_color[0] * 255.0 + 0.5);
        if(i_tmp < 0) i_tmp = 0;
        if(i_tmp > 255) i_tmp = 255;
        r = i_tmp;
        
        i_tmp = (int)(val->val_color[1] * 255.0 + 0.5);
        if(i_tmp < 0) i_tmp = 0;
        if(i_tmp > 255) i_tmp = 255;
        g = i_tmp;

        i_tmp = (int)(val->val_color[2] * 255.0 + 0.5);
        if(i_tmp < 0) i_tmp = 0;
        if(i_tmp > 255) i_tmp = 255;
        b = i_tmp;

        color = visual_color_new();
        visual_color_set(color, r, g, b);
        visual_param_entry_set_color_by_color(param, color);
        visual_object_unref(VISUAL_OBJECT(color));
        break;
      case VISUAL_PARAM_ENTRY_TYPE_PALETTE:  /**< VisPalette parameter. */
      case VISUAL_PARAM_ENTRY_TYPE_OBJECT:   /**< VisObject parameter. */
      case VISUAL_PARAM_ENTRY_TYPE_END:      /**< List end, and used as terminator for VisParamEntry lists. */
        supported = 0;
        break;
      }
    }
  if(supported)
    {
    visual_event_queue_add_param(visual_plugin_get_eventqueue(visual_actor_get_plugin(priv->actor)),
                              param);
    }
  else
    visual_object_unref(VISUAL_OBJECT(param));
  }

/* High level load/unload */

int bg_lv_load(bg_plugin_handle_t * ret,
               const char * name, int plugin_flags, const char * window_id)
  {
  lv_priv_t * priv;
  int i;
  bg_visualization_plugin_t * p;
  VisVideoAttributeOptions *vidoptions;
  
  check_init();
  
  /* Set up callbacks */
  p = calloc(1, sizeof(*p));
  ret->plugin_nc = (bg_plugin_common_t*)p;
  ret->plugin = ret->plugin_nc;
  
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
  p->set_callbacks = set_callbacks_lv;
  p->common.get_parameters = get_parameters_lv;
  p->common.set_parameter  = set_parameter_lv;
    
  /* Set up private data */
  priv = calloc(1, sizeof(*priv));
  ret->priv = priv;
  priv->audio = visual_audio_new();
#if 0
  priv->ov_callbacks.data = priv;
  priv->ov_callbacks.key_callback = key_callback;
  priv->ov_callbacks.key_release_callback = key_release_callback;
  priv->ov_callbacks.button_callback = button_callback;
  priv->ov_callbacks.button_release_callback = button_release_callback;
  priv->ov_callbacks.motion_callback = motion_callback;
#endif
  /* Remove gmerlin added prefix from the plugin name */
  priv->actor = visual_actor_new(name + 7);

  if(plugin_flags & BG_PLUGIN_VISUALIZE_GL)
    {
    priv->win = bg_x11_window_create(window_id);
    
    priv->window_callbacks.data = priv;
    priv->window_callbacks.size_changed = size_changed;
    priv->window_callbacks.key_callback = key_callback;
    priv->window_callbacks.key_release_callback = key_release_callback;
    priv->window_callbacks.button_callback = button_callback;
    priv->window_callbacks.button_release_callback = button_release_callback;
    priv->window_callbacks.motion_callback = motion_callback;
    
    /* Create an OpenGL context. For this, we need the OpenGL attributes */
    vidoptions = visual_actor_get_video_attribute_options(priv->actor);
    
    for(i = 0; i < VISUAL_GL_ATTRIBUTE_LAST; i++)
      {
      if((vidoptions->gl_attributes[i].mutated) && (bg_attributes[i] >= 0))
        {
        bg_x11_window_set_gl_attribute(priv->win, bg_attributes[i],
                                       vidoptions->gl_attributes[i].value);
        }
      }
    
    /* Set bogus dimensions, will be corrected by the size_callback */
    bg_x11_window_set_size(priv->win, 640, 480);
    
    bg_x11_window_realize(priv->win);
    bg_x11_window_set_gl(priv->win);
    }
  visual_actor_realize(priv->actor);

  if(plugin_flags & BG_PLUGIN_VISUALIZE_GL)
    bg_x11_window_unset_gl(priv->win);
  
  priv->parameters = create_parameters(priv->actor, &priv->widgets, &priv->params);
  
  priv->video = visual_video_new();
  
  return 1;
  }

void bg_lv_unload(bg_plugin_handle_t * h)
  {
  lv_priv_t * priv;
  check_init();
  priv = (lv_priv_t *)(h->priv);
  
  if(priv->win)
    bg_x11_window_set_gl(priv->win);

  visual_object_unref(VISUAL_OBJECT(priv->audio));
  visual_object_unref(VISUAL_OBJECT(priv->video));
  visual_object_unref(VISUAL_OBJECT(priv->actor));

  if(priv->win)
    {
    bg_x11_window_unset_gl(priv->win);
    bg_x11_window_destroy(priv->win);
    }
  
  free(h->plugin_nc);
  free(priv);
  }

/* Callbacks */

static const struct
  {
  int bg_code;
  VisKey lv_code;
  VisKeyState state;
  }
keycodes[] =
  {
    { BG_KEY_0, VKEY_0 }, //!< 0
    { BG_KEY_1, VKEY_1 }, //!< 1
    { BG_KEY_2, VKEY_2 }, //!< 2
    { BG_KEY_3, VKEY_3 }, //!< 3
    { BG_KEY_4, VKEY_4 }, //!< 4
    { BG_KEY_5, VKEY_5 }, //!< 5
    { BG_KEY_6, VKEY_6 }, //!< 6
    { BG_KEY_7, VKEY_7 }, //!< 7
    { BG_KEY_8, VKEY_8 }, //!< 8
    { BG_KEY_9, VKEY_9 }, //!< 9

    { BG_KEY_SPACE,       VKEY_SPACE }, //!< Space
    { BG_KEY_RETURN,      VKEY_RETURN }, //!< Return (Enter)
    { BG_KEY_LEFT,        VKEY_LEFT }, //!< Left
    { BG_KEY_RIGHT,       VKEY_RIGHT }, //!< Right
    { BG_KEY_UP,          VKEY_UP }, //!< Up
    { BG_KEY_DOWN,        VKEY_DOWN }, //!< Down
    { BG_KEY_PAGE_UP,     VKEY_PAGEUP }, //!< Page Up
    { BG_KEY_PAGE_DOWN,   VKEY_PAGEDOWN }, //!< Page Down
    { BG_KEY_HOME,        VKEY_HOME }, //!< Page Down
    { BG_KEY_PLUS,        VKEY_PLUS }, //!< Plus
    { BG_KEY_MINUS,       VKEY_MINUS }, //!< Minus
    { BG_KEY_TAB,         VKEY_TAB }, //!< Tab
    { BG_KEY_ESCAPE,      VKEY_ESCAPE }, //!< Esc
    { BG_KEY_MENU,        VKEY_MENU }, //!< Menu key

    { BG_KEY_QUESTION,    VKEY_QUESTION }, //!< ?
    { BG_KEY_EXCLAM,      VKEY_EXCLAIM }, //!< !
    { BG_KEY_QUOTEDBL,    VKEY_QUOTEDBL }, //!< "
    { BG_KEY_DOLLAR,      VKEY_DOLLAR   },//!< $
    //    { BG_KEY_PERCENT,     }, //!< %
    { BG_KEY_APMERSAND,   VKEY_AMPERSAND }, //!< &
    { BG_KEY_SLASH,       VKEY_SLASH     },//!< /
    { BG_KEY_LEFTPAREN,   VKEY_LEFTPAREN },//!< (
    { BG_KEY_RIGHTPAREN,  VKEY_RIGHTPAREN },  //!< )
    { BG_KEY_EQUAL,       VKEY_EQUALS }, //!< =
    { BG_KEY_BACKSLASH,   VKEY_BACKSLASH },//!< :-)

    
    { BG_KEY_A,  VKEY_a, VKMOD_SHIFT },//!< A
    { BG_KEY_B,  VKEY_b, VKMOD_SHIFT },//!< B
    { BG_KEY_C,  VKEY_c, VKMOD_SHIFT },//!< C
    { BG_KEY_D,  VKEY_d, VKMOD_SHIFT },//!< D
    { BG_KEY_E,  VKEY_e, VKMOD_SHIFT },//!< E
    { BG_KEY_F,  VKEY_f, VKMOD_SHIFT },//!< F
    { BG_KEY_G,  VKEY_g, VKMOD_SHIFT },//!< G
    { BG_KEY_H,  VKEY_h, VKMOD_SHIFT },//!< H
    { BG_KEY_I,  VKEY_i, VKMOD_SHIFT },//!< I
    { BG_KEY_J,  VKEY_j, VKMOD_SHIFT },//!< J
    { BG_KEY_K,  VKEY_k, VKMOD_SHIFT },//!< K
    { BG_KEY_L,  VKEY_l, VKMOD_SHIFT },//!< L
    { BG_KEY_M,  VKEY_m, VKMOD_SHIFT },//!< M
    { BG_KEY_N,  VKEY_n, VKMOD_SHIFT },//!< N
    { BG_KEY_O,  VKEY_o, VKMOD_SHIFT },//!< O
    { BG_KEY_P,  VKEY_p, VKMOD_SHIFT },//!< P
    { BG_KEY_Q,  VKEY_q, VKMOD_SHIFT },//!< Q
    { BG_KEY_R,  VKEY_r, VKMOD_SHIFT },//!< R
    { BG_KEY_S,  VKEY_s, VKMOD_SHIFT },//!< S
    { BG_KEY_T,  VKEY_t, VKMOD_SHIFT },//!< T
    { BG_KEY_U,  VKEY_u, VKMOD_SHIFT },//!< U
    { BG_KEY_V,  VKEY_v, VKMOD_SHIFT },//!< V
    { BG_KEY_W,  VKEY_w, VKMOD_SHIFT },//!< W
    { BG_KEY_X,  VKEY_x, VKMOD_SHIFT },//!< X
    { BG_KEY_Y,  VKEY_y, VKMOD_SHIFT },//!< Y
    { BG_KEY_Z,  VKEY_z, VKMOD_SHIFT },//!< Z

    { BG_KEY_a,  VKEY_a },//!< a
    { BG_KEY_b,  VKEY_b },//!< b
    { BG_KEY_c,  VKEY_c },//!< c
    { BG_KEY_d,  VKEY_d },//!< d
    { BG_KEY_e,  VKEY_e },//!< e
    { BG_KEY_f,  VKEY_f },//!< f
    { BG_KEY_g,  VKEY_g },//!< g
    { BG_KEY_h,  VKEY_h },//!< h
    { BG_KEY_i,  VKEY_i },//!< i
    { BG_KEY_j,  VKEY_j },//!< j
    { BG_KEY_k,  VKEY_k },//!< k
    { BG_KEY_l,  VKEY_l },//!< l
    { BG_KEY_m,  VKEY_m },//!< m
    { BG_KEY_n,  VKEY_n },//!< n
    { BG_KEY_o,  VKEY_o },//!< o
    { BG_KEY_p,  VKEY_p },//!< p
    { BG_KEY_q,  VKEY_q },//!< q
    { BG_KEY_r,  VKEY_r },//!< r
    { BG_KEY_s,  VKEY_s },//!< s
    { BG_KEY_t,  VKEY_t },//!< t
    { BG_KEY_u,  VKEY_u },//!< u
    { BG_KEY_v,  VKEY_v },//!< v
    { BG_KEY_w,  VKEY_w },//!< w
    { BG_KEY_x,  VKEY_x },//!< x
    { BG_KEY_y,  VKEY_y },//!< y
    { BG_KEY_z,  VKEY_z },//!< z


    { BG_KEY_F1,  VKEY_F1 },//!< F1
    { BG_KEY_F2,  VKEY_F2 },//!< F2
    { BG_KEY_F3,  VKEY_F3 },//!< F3
    { BG_KEY_F4,  VKEY_F3 },//!< F4
    { BG_KEY_F5,  VKEY_F5 },//!< F5
    { BG_KEY_F6,  VKEY_F6 },//!< F6
    { BG_KEY_F7,  VKEY_F7 },//!< F7
    { BG_KEY_F8,  VKEY_F8 },//!< F8
    { BG_KEY_F9,  VKEY_F9 },//!< F9
    { BG_KEY_F10, VKEY_F10 },//!< F10
    { BG_KEY_F11, VKEY_F11 },//!< F11
    { BG_KEY_F12, VKEY_F12 },//!< F12
    
  };

static int get_key_mask(int bg_mask)
  {
  int ret = 0;
  if(bg_mask & BG_KEY_SHIFT_MASK)
    ret |= VKMOD_SHIFT;
  if(bg_mask & BG_KEY_CONTROL_MASK)
    ret |= VKMOD_CTRL;
  if(bg_mask & BG_KEY_ALT_MASK)
    ret |= VKMOD_ALT;
  return ret;
  }

static int get_key_code(int bg_code, int bg_mask,
                        int * lv_code, int * lv_mask)
  {
  int i;
  *lv_mask = get_key_mask(bg_mask);
  for(i = 0; i < sizeof(keycodes) / sizeof(keycodes[0]); i++)
    {
    if(bg_code == keycodes[i].bg_code)
      {
      *lv_mask |= keycodes[i].state;
      *lv_code = keycodes[i].lv_code;
      return 1;
      }
    }
  return 0;
  }

static int key_callback(void * data, int key, int mask)
  {
  VisEventQueue *eventqueue;
  int lv_key, lv_mask;
  lv_priv_t * priv = (lv_priv_t*)data;
  
  if(!get_key_code(key, mask, &lv_key, &lv_mask))
    return 0;

  eventqueue = visual_plugin_get_eventqueue(visual_actor_get_plugin(priv->actor));
  
  visual_event_queue_add_keyboard(eventqueue,
                                  lv_key, lv_mask, VISUAL_KEY_DOWN);
  return 1;
  }

static int key_release_callback(void * data, int key, int mask)
  {
  VisEventQueue *eventqueue;
  int lv_key, lv_mask;
  lv_priv_t * priv = (lv_priv_t*)data;
  
  if(!get_key_code(key, mask, &lv_key, &lv_mask))
    return 0;

  eventqueue = visual_plugin_get_eventqueue(visual_actor_get_plugin(priv->actor));
  
  visual_event_queue_add_keyboard(eventqueue,
                                  lv_key, lv_mask, VISUAL_KEY_UP);
  return 1;
  }

static int motion_callback(void * data, int x, int y, int mask)
  {
  VisEventQueue *eventqueue;
  lv_priv_t * priv = (lv_priv_t*)data;
  eventqueue = visual_plugin_get_eventqueue(visual_actor_get_plugin(priv->actor));
  
  visual_event_queue_add_mousemotion(eventqueue, x, y);

  if(priv->ov_callbacks && priv->ov_callbacks->motion_callback)
    {
    priv->ov_callbacks->motion_callback(priv->ov_callbacks->data,
                                        x, y, mask);
    }

  return 1;
  }

static int button_callback(void * data, int x, int y, int button, int mask)
  {
  VisEventQueue *eventqueue;
  lv_priv_t * priv = (lv_priv_t*)data;
  eventqueue = visual_plugin_get_eventqueue(visual_actor_get_plugin(priv->actor));
  visual_event_queue_add_mousebutton(eventqueue, button, VISUAL_MOUSE_DOWN, x, y);
  if(priv->ov_callbacks && priv->ov_callbacks->button_callback)
    {
    priv->ov_callbacks->button_callback(priv->ov_callbacks->data,
                                        x, y, button, mask);
    }
  return 1;
  }

static int button_release_callback(void * data, int x, int y, int button, int mask)
  {
  VisEventQueue *eventqueue;
  lv_priv_t * priv = (lv_priv_t*)data;
  eventqueue = visual_plugin_get_eventqueue(visual_actor_get_plugin(priv->actor));
  visual_event_queue_add_mousebutton(eventqueue, button, VISUAL_MOUSE_UP, x, y);

  if(priv->ov_callbacks && priv->ov_callbacks->button_release_callback)
    {
    priv->ov_callbacks->button_release_callback(priv->ov_callbacks->data,
                                                x, y, button, mask);
    }

  return 1;
  }
