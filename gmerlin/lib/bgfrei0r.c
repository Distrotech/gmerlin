/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

/* Make gmerlin video filters from frei0r plugins.
 * See http://www.piksel.org/frei0r
 */

#include <config.h>

#include <dlfcn.h>
#include <string.h>


#include <gmerlin/utils.h>
#include <gmerlin/pluginregistry.h>
#include <bgfrei0r.h>
#include <frei0r.h>

#include <gmerlin/translation.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "frei0r"

static bg_parameter_info_t *
create_parameters(void * dll_handle, f0r_plugin_info_t * plugin_info)
  {
  int i;

  f0r_instance_t (*construct) (unsigned int width, unsigned int height);
  f0r_instance_t (*destruct) (f0r_instance_t);
  f0r_instance_t instance;

  int (*init)();
  void (*deinit)();
  
  void (*get_param_info) (f0r_param_info_t *info, int param_index);
  void (*get_param_value) (f0r_instance_t instance,
                           f0r_param_t param, int param_index);
  
  bg_parameter_info_t * ret = NULL;
  f0r_param_info_t  param_info;
  
  if(!plugin_info->num_params)
    return ret;

  get_param_info = dlsym(dll_handle, "f0r_get_param_info");
  if(!get_param_info)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cannot load frei0r plugin: %s", dlerror());
    return NULL;
    }
  
  construct = dlsym(dll_handle, "f0r_construct");
  if(!construct)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cannot load frei0r plugin: %s", dlerror());
    return NULL;
    }
  destruct = dlsym(dll_handle, "f0r_destruct");
  if(!destruct)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cannot load frei0r plugin: %s", dlerror());
    return NULL;
    }
  get_param_value = dlsym(dll_handle, "f0r_get_param_value");
  if(!get_param_value)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cannot load frei0r plugin: %s", dlerror());
    return NULL;
    }
  
  init = dlsym(dll_handle, "f0r_init");
  deinit = dlsym(dll_handle, "f0r_deinit");

  if(init)
    init();

  /* Need to create an instance so we get the default values */
  instance = construct(32, 32);
  
  ret = calloc(plugin_info->num_params+1, sizeof(*ret));
  
  for(i = 0; i < plugin_info->num_params; i++)
    {
    memset(&param_info, 0, sizeof(param_info));
    get_param_info(&param_info, i);

    ret[i].name = bg_strdup(NULL, param_info.name);
    ret[i].long_name = bg_strdup(NULL, param_info.name);
    ret[i].flags = BG_PARAMETER_SYNC;
    ret[i].help_string = bg_strdup(NULL,
                                       param_info.explanation);
    switch(param_info.type)
      {
      case F0R_PARAM_BOOL:
        {
        double val;
        get_param_value(instance, &val, i);
        if(val > 0.5)
          ret[i].val_default.val_i = 1;
        ret[i].type = BG_PARAMETER_CHECKBUTTON;
        break;
        }
      case F0R_PARAM_DOUBLE:
        {
        double val;
        get_param_value(instance, &val, i);
        ret[i].val_default.val_f = val;
        ret[i].type = BG_PARAMETER_SLIDER_FLOAT;
        ret[i].num_digits = 4;
        ret[i].val_min.val_f = 0.0;
        ret[i].val_max.val_f = 1.0;
        break;
        }
      case F0R_PARAM_COLOR:
        {
        f0r_param_color_t val;
        get_param_value(instance, &val, i);
        ret[i].val_default.val_color[0] = val.r;
        ret[i].val_default.val_color[1] = val.g;
        ret[i].val_default.val_color[2] = val.b;
        ret[i].type = BG_PARAMETER_COLOR_RGB;
        break;
        }
      case F0R_PARAM_POSITION:
        {
        f0r_param_position_t val;
        get_param_value(instance, &val, i);
        ret[i].val_default.val_pos[0] = val.x;
        ret[i].val_default.val_pos[1] = val.y;
        ret[i].type = BG_PARAMETER_POSITION;
        ret[i].num_digits = 4;
        break;
        }
      case F0R_PARAM_STRING:
        ret[i].type = BG_PARAMETER_STRING;
        break;
      }
    }
  destruct(instance);

  if(deinit)
    deinit();

  return ret;
  }

bg_plugin_info_t *
bg_frei0r_get_info(void * dll_handle, const char * filename)
  {
  bg_plugin_info_t * ret = NULL;

  f0r_plugin_info_t plugin_info;
  
  void (*get_plugin_info)(f0r_plugin_info_t *info);

  get_plugin_info = dlsym(dll_handle, "f0r_get_plugin_info");
 
  if(!get_plugin_info)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot load frei0r plugin: %s", dlerror());
    return ret;
    }
  memset(&plugin_info, 0, sizeof(plugin_info));
  get_plugin_info(&plugin_info);

  ret = calloc(1, sizeof(*ret));

  ret->name      = bg_sprintf("bg_f0r_%s", plugin_info.name);
  ret->long_name = bg_sprintf("frei0r %s", plugin_info.name);
  ret->type        = BG_PLUGIN_FILTER_VIDEO;
  ret->api         = BG_PLUGIN_API_FREI0R;
  ret->flags       = BG_PLUGIN_FILTER_1;
  ret->module_filename = bg_strdup(NULL, filename);

  // fprintf(stderr, "Loading %s\n", ret->name);
  
  /* Check if we can use this at all */
  if(plugin_info.plugin_type != F0R_PLUGIN_TYPE_FILTER)
    {
    ret->flags |= BG_PLUGIN_UNSUPPORTED;
    return ret;
    }

  ret->description = bg_sprintf(TRS("Author: %s\n%s"),
                                plugin_info.author, plugin_info.explanation);

  ret->parameters = create_parameters(dll_handle, &plugin_info);
  
  return ret;
  }

/* Plugin function wrappers */

typedef struct
  {
  /* Function pointers */
  
  f0r_instance_t (*construct) (unsigned int width, unsigned int height);
  void (*destruct) (f0r_instance_t instance);
  void (*set_param_value) (f0r_instance_t instance,
                           f0r_param_t param, int param_index);
  void (*update) (f0r_instance_t instance, double time,
                  const uint32_t *inframe, uint32_t *outframe);
  
  f0r_instance_t instance;
  f0r_plugin_info_t plugin_info;
  /* Parameter values are cached here */
  bg_cfg_section_t * section;

  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;

  gavl_video_frame_t * in_frame;
  gavl_video_frame_t * out_frame;
  
  gavl_video_format_t format;
  
  const bg_parameter_info_t * parameters;
  int do_swap;
  } frei0r_t;

static void set_parameter_instance(void * data,
                                   const char * name,
                                   const bg_parameter_value_t * v)
  {
  int index;
  frei0r_t * vp;
  const bg_parameter_info_t * info;

  if(!name)
    return;
  
  vp = (frei0r_t *)data;
  info = bg_parameter_find(vp->parameters, name);
  if(info)
    {
    index = info - vp->parameters;

    switch(info->type)
      {
      case BG_PARAMETER_CHECKBUTTON:
        {
        double val = v->val_i ? 1.0 : 0.0;
        vp->set_param_value(vp->instance, &val, index);
        break;
        }
      case BG_PARAMETER_SLIDER_FLOAT:
        {
        double val = v->val_f;
        vp->set_param_value(vp->instance, &val, index);
        break;
        }
      case BG_PARAMETER_COLOR_RGB:
        {
        f0r_param_color_t val;
        val.r = v->val_color[0];
        val.g = v->val_color[1];
        val.b = v->val_color[2];
        vp->set_param_value(vp->instance, &val, index);
        break;
        }
      case BG_PARAMETER_POSITION:
        {
        f0r_param_position_t val;
        val.x = v->val_pos[0];
        val.y = v->val_pos[1];
        vp->set_param_value(vp->instance, &val, index);
        break;
        }
      default:
        break;
      }
    
    }
  }

static void set_parameter_frei0r(void * data,
                                 const char * name,
                                 const bg_parameter_value_t * val)
  {
  frei0r_t * vp;
  const bg_parameter_info_t * info;
  vp = (frei0r_t *)data;
  
  if(!name)
    return;
  
  if(vp->instance)
    set_parameter_instance(data, name, val);
  
  /*
   *  Save the value into the section, so we can pass this to the next
   *  instance (gmerlin filters survive format changes, f0r filters don't)
   */
  
  if(!vp->section)
    vp->section = bg_cfg_section_create_from_parameters("bla",
                                                        vp->parameters);
  
  info = bg_parameter_find(vp->parameters, name);
  if(info)
    bg_cfg_section_set_parameter(vp->section, info, val);
  }

static void connect_input_port_frei0r(void *priv,
                                      bg_read_video_func_t func, void *data, int stream, int port)
  {
  frei0r_t * vp;
  vp = (frei0r_t *)priv;
  
  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  }

static void get_output_format_frei0r(void * priv,
                                     gavl_video_format_t * format)
  {
  frei0r_t * vp;
  vp = (frei0r_t *)priv;
  gavl_video_format_copy(format, &vp->format);
  }

static const gavl_pixelformat_t packed32_formats[] =
  {
    GAVL_RGB_32,
    GAVL_BGR_32,
    GAVL_RGBA_32,
    GAVL_YUVA_32,
    GAVL_PIXELFORMAT_NONE,
  };

static void set_input_format_frei0r(void *priv, gavl_video_format_t *format, int port)
  {
  /* Set input format */
  frei0r_t * vp;
  vp = (frei0r_t *)priv;
  switch(vp->plugin_info.color_model)
    {
    case F0R_COLOR_MODEL_BGRA8888:
      vp->do_swap = 1;
      /* Fall through */
    case F0R_COLOR_MODEL_RGBA8888:
      format->pixelformat = GAVL_RGBA_32;
      break;
    case F0R_COLOR_MODEL_PACKED32:
      format->pixelformat = gavl_pixelformat_get_best(format->pixelformat,
                                                      packed32_formats,
                                                      NULL);
      break;
    }
  /* Frei0r demands image sizes to be a multiple of 8. We fake this by making a larger
     frame */

  format->frame_width  = ((format->image_width+7)/8)*8;
  format->frame_height = ((format->image_height+7)/8)*8;

  gavl_video_format_copy(&vp->format, format);

  /* Fire up the plugin */
  
  vp->instance = vp->construct(vp->format.frame_width,
                                 vp->format.frame_height);

  /* Now, we can set parameters */

  if(vp->section)
    {
    bg_cfg_section_apply(vp->section,
                         vp->parameters,
                         set_parameter_instance, vp);

    }
  
  
  vp->in_frame = gavl_video_frame_create_nopad(&vp->format);
  if(vp->out_frame)
    {
    gavl_video_frame_destroy(vp->out_frame);
    vp->out_frame = NULL;
    }
  }

static int read_video_frei0r(void *priv, gavl_video_frame_t *frame, int stream)
  {
  frei0r_t * vp;
  double time;
  vp = (frei0r_t *)priv;

  if(!vp->read_func(vp->read_data, vp->in_frame, vp->read_stream))
    return 0;

  time = gavl_time_to_seconds(gavl_time_unscale(vp->format.timescale,
                                                vp->in_frame->timestamp));
  
  if(frame->strides[0] == vp->format.frame_width * 4)
    vp->update(vp->instance, time,
               (const uint32_t*)vp->in_frame->planes[0],
               (uint32_t*)frame->planes[0]);
  else
    {
    if(!vp->out_frame)
      vp->out_frame = gavl_video_frame_create_nopad(&vp->format);
    vp->update(vp->instance, time,
               (const uint32_t*)vp->in_frame->planes[0],
               (uint32_t*)vp->out_frame->planes[0]);
    gavl_video_frame_copy(&vp->format, frame, vp->out_frame);
    
    }
  frame->timestamp = vp->in_frame->timestamp;
  frame->duration = vp->in_frame->duration;
  
  return 1;
  }

static const bg_parameter_info_t * get_parameters_frei0r(void * priv)
  {
  frei0r_t * vp;
  vp = (frei0r_t *)priv;
  return vp->parameters;
  }

int bg_frei0r_load(bg_plugin_handle_t * ret,
                   const bg_plugin_info_t * info)
  {
  bg_fv_plugin_t * vf;
  frei0r_t * priv;
  void (*get_plugin_info)(f0r_plugin_info_t *info);
  
  vf = calloc(1, sizeof(*vf));
  ret->plugin_nc = (bg_plugin_common_t*)vf;
  ret->plugin = ret->plugin_nc;

  vf->set_input_format = set_input_format_frei0r;
  vf->connect_input_port = connect_input_port_frei0r;
  vf->get_output_format = get_output_format_frei0r;
  vf->read_video = read_video_frei0r;

  if(info->parameters)
    {
    ret->plugin_nc->get_parameters = get_parameters_frei0r;
    ret->plugin_nc->set_parameter  = set_parameter_frei0r;
    }
  get_plugin_info = dlsym(ret->dll_handle, "f0r_get_plugin_info");
  if(!get_plugin_info)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cannot load frei0r plugin: %s", dlerror());
    return 0;
    }

  priv = calloc(1,  sizeof(*priv));
  ret->priv = priv;
  
  get_plugin_info(&priv->plugin_info);
  
  priv->parameters = info->parameters;
  
  /* Get function pointers */
  priv->construct = dlsym(ret->dll_handle, "f0r_construct");
  if(!priv->construct)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot load frei0r plugin: %s", dlerror());
    return 0;
    }
  priv->destruct  = dlsym(ret->dll_handle, "f0r_destruct");
  if(!priv->destruct)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot load frei0r plugin: %s", dlerror());
    return 0;
    }
  priv->set_param_value  = dlsym(ret->dll_handle, "f0r_set_param_value");
  if(!priv->set_param_value)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot load frei0r plugin: %s", dlerror());
    return 0;
    }
  priv->update  = dlsym(ret->dll_handle, "f0r_update");
  if(!priv->update)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot load frei0r plugin: %s", dlerror());
    return 0;
    }
  
  return 1;
  }

void bg_frei0r_unload(bg_plugin_handle_t * h)
  {
  frei0r_t * vp;
  vp = (frei0r_t *)h->priv;
  if(vp->instance)  vp->destruct(vp->instance);
  if(vp->in_frame)  gavl_video_frame_destroy(vp->in_frame);
  if(vp->out_frame) gavl_video_frame_destroy(vp->out_frame);
  free(vp);
  }


