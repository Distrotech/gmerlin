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

#include <math.h>
#include <dlfcn.h>
#include <string.h>

#include <ladspa.h>

#include <config.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/translation.h>

#include <bgladspa.h>


#include <gmerlin/log.h>

#include <gmerlin/utils.h>


#define LOG_DOMAIN "ladspa"

static bg_parameter_info_t * 
create_parameters(const LADSPA_Descriptor * desc)
  {
  int num_parameters = 0;
  int index;
  int i;
  bg_parameter_info_t * ret;
  int is_int;
  float val_default = 0.0;
  
  for(i = 0; i < desc->PortCount; i++)
    {
    if(LADSPA_IS_PORT_INPUT(desc->PortDescriptors[i]) &&
       LADSPA_IS_PORT_CONTROL(desc->PortDescriptors[i]))
      num_parameters++;
    }
  if(desc->set_run_adding_gain)
    num_parameters++;
  if(desc->run_adding)
    num_parameters++;
  
  if(!num_parameters)
    return NULL;

  
  index = 0;
  ret = calloc(num_parameters+1, sizeof(*ret));
  
  for(i = 0; i < desc->PortCount; i++)
    {
    if(LADSPA_IS_PORT_INPUT(desc->PortDescriptors[i]) &&
       LADSPA_IS_PORT_CONTROL(desc->PortDescriptors[i]))
      {
      is_int = 0;
      
      ret[index].name =      bg_sprintf("%s", desc->PortNames[i]);
      ret[index].long_name = bg_sprintf("%s", desc->PortNames[i]);

      if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_TOGGLED)
        {
        is_int = 1;
        ret[index].type = BG_PARAMETER_CHECKBUTTON;

        if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) ==
           LADSPA_HINT_DEFAULT_1)
          ret[index].val_default.val_i = 1;
        }
      else if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_INTEGER)
        {
        is_int = 1;
        if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_BOUNDED_BELOW) &&
           (desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_BOUNDED_ABOVE))
          {
          ret[index].type = BG_PARAMETER_SLIDER_INT;
          ret[index].val_min.val_i = (int)(desc->PortRangeHints[i].LowerBound);
          ret[index].val_max.val_i = (int)(desc->PortRangeHints[i].UpperBound);
          }
        else
          ret[index].type = BG_PARAMETER_INT;
        }
      else if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_BOUNDED_BELOW) &&
              (desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_BOUNDED_ABOVE))
        {
        ret[index].type = BG_PARAMETER_SLIDER_FLOAT;
        ret[index].val_min.val_f = desc->PortRangeHints[i].LowerBound;
        ret[index].val_max.val_f = desc->PortRangeHints[i].UpperBound;
        ret[index].num_digits = 3;
        }
      else
        {
        ret[index].type = BG_PARAMETER_FLOAT;
        ret[index].num_digits = 3;
        }
      
      if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) ==
         LADSPA_HINT_DEFAULT_MINIMUM)
        val_default = desc->PortRangeHints[i].LowerBound;
      else if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) ==
              LADSPA_HINT_DEFAULT_MAXIMUM)
        val_default = desc->PortRangeHints[i].UpperBound;
      else if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) ==
              LADSPA_HINT_DEFAULT_LOW)
        {
        if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_LOGARITHMIC)
          {
          val_default =
            exp(log(desc->PortRangeHints[i].LowerBound) * 0.75 +
                log(desc->PortRangeHints[i].UpperBound) * 0.25);
          }
        else
          {
          val_default = 
            desc->PortRangeHints[i].LowerBound * 0.75 +
            desc->PortRangeHints[i].UpperBound * 0.25;
          }
        }
      else if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) ==
              LADSPA_HINT_DEFAULT_MIDDLE)
        {
        if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_LOGARITHMIC)
          {
          val_default =
            sqrt(desc->PortRangeHints[i].LowerBound *
                 desc->PortRangeHints[i].UpperBound);
          }
        else
          {
          val_default = 
            0.5 * (desc->PortRangeHints[i].LowerBound +
            desc->PortRangeHints[i].UpperBound);
          }
        }
      else if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) ==
              LADSPA_HINT_DEFAULT_HIGH)
        {
        if(desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_LOGARITHMIC)
          {
          val_default =
            exp(log(desc->PortRangeHints[i].LowerBound) * 0.25 +
                log(desc->PortRangeHints[i].UpperBound) * 0.75);
          }
        else
          {
          val_default = 
            desc->PortRangeHints[i].LowerBound * 0.25 +
            desc->PortRangeHints[i].UpperBound * 0.75;
          }
        }
      else if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) ==
              LADSPA_HINT_DEFAULT_100)
        {
        val_default = 100.0;
        }
      else if((desc->PortRangeHints[i].HintDescriptor & LADSPA_HINT_DEFAULT_MASK) ==
              LADSPA_HINT_DEFAULT_440)
        {
        val_default = 440.0;
        }
      if(is_int)
        ret[index].val_default.val_i = (int)(val_default);
      else
        ret[index].val_default.val_f = val_default;

      ret[index].flags |= BG_PARAMETER_SYNC;
      index++;
      }
    }

  if(desc->run_adding)
    {
    ret[index].name = bg_strdup(ret[index].name, "$run_adding");
    ret[index].long_name = bg_strdup(ret[index].long_name,
                                     TR("Add effect to input data"));
    ret[index].type = BG_PARAMETER_CHECKBUTTON;
    ret[index].val_default.val_i = 0;
    ret[index].flags = BG_PARAMETER_SYNC;
    index++;
    }
  if(desc->set_run_adding_gain)
    {
    ret[index].name = bg_strdup(ret[index].name, "$run_adding_gain");
    ret[index].long_name = bg_strdup(ret[index].long_name,
                                     TR("Add gain (dB)"));
    ret[index].type = BG_PARAMETER_SLIDER_FLOAT;
    ret[index].num_digits = 2;
    ret[index].val_min.val_f = -70.0;
    ret[index].val_max.val_f =  0.0;
    ret[index].val_default.val_f = 0.0;
    ret[index].help_string =
      bg_strdup(ret[index].help_string,
                TR("Overall gain for this filter. This is only valid if you add the effect to the input data"));
    ret[index].flags = BG_PARAMETER_SYNC;
    index++;
    }
  
  return ret;
  }

static void
count_ports(const LADSPA_Descriptor * desc, int * in_ports, 
            int * out_ports, int * in_control_ports, 
            int * out_control_ports)
  {
  int i;
  *in_ports = 0;
  *out_ports = 0;
  *in_control_ports = 0;
  *out_control_ports = 0;
  
  for(i = 0; i < desc->PortCount; i++)
    {
    if(LADSPA_IS_PORT_AUDIO(desc->PortDescriptors[i]))
      {
      if(LADSPA_IS_PORT_INPUT(desc->PortDescriptors[i]))
        (*in_ports)++;
      else if(LADSPA_IS_PORT_OUTPUT(desc->PortDescriptors[i]))
        (*out_ports)++;
      }
    else if(LADSPA_IS_PORT_CONTROL(desc->PortDescriptors[i]))
      {
      if(LADSPA_IS_PORT_INPUT(desc->PortDescriptors[i]))
        (*in_control_ports)++;
      else if(LADSPA_IS_PORT_OUTPUT(desc->PortDescriptors[i]))
        (*out_control_ports)++;
      }
    }
  }

static bg_plugin_info_t * get_info(const LADSPA_Descriptor * desc)
  {
  bg_plugin_info_t * ret;
  int in_ports, out_ports, in_control_ports, out_control_ports;
  
  ret = calloc(1, sizeof(*ret));

  ret->name        = bg_sprintf("fa_ladspa_%s", desc->Label);
  ret->long_name   = bg_strdup(NULL, desc->Name);
  ret->type        = BG_PLUGIN_FILTER_AUDIO;
  ret->api         = BG_PLUGIN_API_LADSPA;
  ret->flags       = BG_PLUGIN_FILTER_1;
  ret->description = bg_sprintf(TR("ladspa plugin\nAuthor:\t%s\nCopyright:\t%s"),
                                desc->Maker, desc->Copyright);
  
  /* Check for ports */
  count_ports(desc, &in_ports, &out_ports, &in_control_ports,
              &out_control_ports);

  if((in_ports != out_ports) ||
     (in_ports < 1) || ((in_ports > 2)))
    {
    /* Unsupported */
    ret->flags       |= BG_PLUGIN_UNSUPPORTED;
    }
  
  ret->parameters = create_parameters(desc);
  return ret;
  }

bg_plugin_info_t * bg_ladspa_get_info(void * dll_handle, const char * filename)
  {
  bg_plugin_info_t * ret = NULL;
  bg_plugin_info_t * end = NULL;
  bg_plugin_info_t * new = NULL;
  int index;
  
  const LADSPA_Descriptor * desc;
  LADSPA_Descriptor_Function desc_func;

  desc_func = dlsym(dll_handle, "ladspa_descriptor");
  if(!desc_func)
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "No symbol \"ladspa_descriptor\" found: %s",
            dlerror());
    return NULL;
    }
  index = 0;
  while((desc = desc_func(index)))
    {
    new = get_info(desc);
    new->index = index;
    new->module_filename = bg_strdup(NULL, filename);
    if(ret)
      {
      end->next = new;
      end = end->next;
      }
    else
      {
      ret = new;
      end = ret;
      }
    index++;
    }
  return ret;
  }

typedef struct
  {
  LADSPA_Data * config_ports;
  gavl_audio_format_t format;
  //  gavl_audio_frame_t * frame;
  const LADSPA_Descriptor * desc;
  float run_adding_gain;
  int run_adding;

  gavl_audio_source_t * in_src;
  gavl_audio_source_t * out_src;
  
  bg_parameter_info_t * parameters;

  /* Port maps */
  int num_in_ports;
  int * in_ports;
  int num_out_ports;
  int * out_ports;
  int num_in_c_ports;
  int * in_c_ports;
  int num_out_c_ports;
  int * out_c_ports;
  
  int num_instances;

  int inplace_broken; 

  struct
    {
    LADSPA_Handle Instance;
    int input_port;
    int output_port;
    int run;
    } channels[GAVL_MAX_CHANNELS];
  } ladspa_priv_t;

static void cleanup_ladspa(ladspa_priv_t * lp)
  {
  int i;
  for(i = 0; i < lp->num_instances; i++)
    {
    if(lp->desc->deactivate)
      lp->desc->deactivate(lp->channels[i].Instance);
    if(lp->desc->cleanup)
      lp->desc->cleanup(lp->channels[i].Instance);
    }
  lp->num_instances = 0;
  }

/* Initialize instances, called after the input format is known */

static void init_ladspa(ladspa_priv_t * lp)
  {
  int i, j;
  cleanup_ladspa(lp);
  
  if(lp->num_out_ports == 1)
    {
    lp->num_instances = lp->format.num_channels;
    }
  else if(lp->format.num_channels != lp->num_out_ports)
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, 
           "Remixing to stereo for filter \"%s\"",
           lp->desc->Name);
    lp->format.num_channels = 2;
    lp->format.channel_locations[0] = GAVL_CHID_NONE;
    gavl_set_channel_setup(&lp->format);
    lp->num_instances = 1;
    }
  else /* Stereo -> Stereo */
    lp->num_instances = 1;
  
//  lp->format.samples_per_frame = 0;
  for(i = 0; i < lp->num_instances; i++)
    {
    lp->channels[i].Instance =
      lp->desc->instantiate(lp->desc, lp->format.samplerate);

    for(j = 0; j < lp->num_in_c_ports; j++)
      {
      lp->desc->connect_port(lp->channels[i].Instance,
                             lp->in_c_ports[j],
                             &lp->config_ports[lp->in_c_ports[j]]);
      }
    for(j = 0; j < lp->num_out_c_ports; j++)
      {
      lp->desc->connect_port(lp->channels[i].Instance,
                             lp->out_c_ports[j],
                             &lp->config_ports[lp->out_c_ports[j]]);
      }
    
    if(lp->desc->activate)
      lp->desc->activate(lp->channels[i].Instance);

    if(lp->desc->set_run_adding_gain)
      lp->desc->set_run_adding_gain(lp->channels[i].Instance,
                                    lp->run_adding_gain);
    
    lp->channels[i].input_port = lp->in_ports[0];
    lp->channels[i].output_port = lp->out_ports[0];
    }
  for(i = lp->num_instances; i < lp->format.num_channels; i++)
    {
    lp->channels[i].Instance = lp->channels[0].Instance;
    lp->channels[i].input_port = lp->in_ports[i];
    lp->channels[i].output_port = lp->out_ports[i];
    }
  }


static void connect_input(ladspa_priv_t * lp, gavl_audio_frame_t * f)
  {
  int i;
  for(i = 0; i < lp->format.num_channels; i++)
    {
    lp->desc->connect_port(lp->channels[i].Instance,
                           lp->channels[i].input_port,
                           f->channels.f[i]);
    }
  }

static void connect_output(ladspa_priv_t * lp, gavl_audio_frame_t * f)
  {
  int i;
  for(i = 0; i < lp->format.num_channels; i++)
    {
    lp->desc->connect_port(lp->channels[i].Instance,
                           lp->channels[i].output_port,
                           f->channels.f[i]);
    }
  }

static gavl_source_status_t
read_func(void * priv,
          gavl_audio_frame_t ** frame)
  {
  
  
  int i;
  ladspa_priv_t * lp;
  int num_samples;
  gavl_source_status_t st;
  gavl_audio_frame_t * in_frame = NULL;
  lp = (ladspa_priv_t *)priv;
  
  if(lp->inplace_broken)
    {
    if((st = gavl_audio_source_read_frame(lp->in_src, &in_frame)) !=
       GAVL_SOURCE_OK)
      return st;
    
    if(lp->run_adding)
      gavl_audio_frame_copy(&lp->format, *frame, in_frame,
                            0, 0, in_frame->valid_samples,
                            in_frame->valid_samples);
    connect_input(lp, in_frame);
    num_samples = in_frame->valid_samples;

    (*frame)->valid_samples = num_samples;
    (*frame)->timestamp = in_frame->timestamp;
    }
  else
    {
    if((st = gavl_audio_source_read_frame(lp->in_src, frame)) !=
       GAVL_SOURCE_OK)
      return st;
    connect_input(lp, *frame);
    num_samples = (*frame)->valid_samples;
    }
  
  connect_output(lp, *frame);
  
  /* Run */
  for(i = 0; i < lp->num_instances; i++)
    {
    if(lp->run_adding)
      lp->desc->run_adding(lp->channels[i].Instance, num_samples);
    else
      lp->desc->run(lp->channels[i].Instance, num_samples);
    }
  return GAVL_SOURCE_OK;
  }

static void reset_ladspa(void * priv)
  {
  int i;
  ladspa_priv_t * lp;
  lp = (ladspa_priv_t *)priv;
  
  /* Reset */
  if(lp->desc->deactivate)
    {
    for(i = 0; i < lp->num_instances; i++)
      lp->desc->deactivate(lp->channels[i].Instance);
    }
  if(lp->desc->activate)
    {
    for(i = 0; i < lp->num_instances; i++)
      lp->desc->activate(lp->channels[i].Instance);
    }
  }

static const bg_parameter_info_t * get_parameters_ladspa(void * priv)
  {
  ladspa_priv_t * lp;
  lp = (ladspa_priv_t *)priv;
  return lp->parameters;
  }

static void set_parameter_ladspa(void * priv, const char * name,
                                 const bg_parameter_value_t * val)
  {
  int i;
  ladspa_priv_t * lp;
  float gain_lin;
  if(!name)
    return;

  lp = (ladspa_priv_t *)priv;


  if(!strcmp(name, "$run_adding_gain"))
    {
    gain_lin = pow(10, val->val_f / 20.0);
    
    for(i = 0; i < lp->num_instances; i++)
      {
      lp->desc->set_run_adding_gain(lp->channels[i].Instance,
                                    gain_lin);
      }
    lp->run_adding_gain = gain_lin;
    return;
    }
  if(!strcmp(name, "$run_adding"))
    {
    if(lp->desc->run_adding)
      lp->run_adding = val->val_i;
    else
      lp->run_adding = 0;
    return;
    }
  for(i = 0; i < lp->num_in_c_ports; i++)
    {
    if(!strcmp(name, lp->desc->PortNames[lp->in_c_ports[i]]))
      {
      if(lp->desc->PortRangeHints[lp->in_c_ports[i]].HintDescriptor &
         (LADSPA_HINT_INTEGER | LADSPA_HINT_TOGGLED))
        lp->config_ports[lp->in_c_ports[i]] = val->val_i;
      else
        lp->config_ports[lp->in_c_ports[i]] = val->val_f;
      break;
      }
    }
  }

static gavl_audio_source_t *
connect_ladspa(void * priv, gavl_audio_source_t * src,
               const gavl_audio_options_t * opt)
  {
  ladspa_priv_t * lp = priv;

  lp->in_src = src;

  if(lp->out_src)
    gavl_audio_source_destroy(lp->out_src);
  
  gavl_audio_format_copy(&lp->format,
                         gavl_audio_source_get_src_format(lp->in_src));
  lp->format.interleave_mode = GAVL_INTERLEAVE_NONE;
  lp->format.sample_format = GAVL_SAMPLE_FLOAT;
  init_ladspa(lp);
  
  gavl_audio_source_set_dst(lp->in_src, 0, &lp->format);

  lp->out_src = gavl_audio_source_create_source(read_func,
                                                lp, 0,
                                                lp->in_src);
  return lp->out_src;
  }

int bg_ladspa_load(bg_plugin_handle_t * ret,
                    const bg_plugin_info_t * info)
  {
  int i;
  LADSPA_Descriptor_Function desc_func;
  int in_port_index = 0, out_port_index = 0;
  int in_c_port_index = 0, out_c_port_index = 0;
  
  ladspa_priv_t * priv;
  bg_fa_plugin_t * af;
  af = calloc(1, sizeof(*af));
  ret->plugin_nc = (bg_plugin_common_t*)af;
  ret->plugin = ret->plugin_nc;
#if 0
  af->set_input_format = set_input_format_ladspa;
  af->connect_input_port = connect_input_port_ladspa;
  af->get_output_format = get_output_format_ladspa;
  af->read_audio = read_audio_ladspa;
#endif
  af->reset      = reset_ladspa;

  af->connect      = connect_ladspa;

  
  if(info->parameters)
    {
    ret->plugin_nc->get_parameters = get_parameters_ladspa;
    ret->plugin_nc->set_parameter  = set_parameter_ladspa;
    }
  
  priv = calloc(1, sizeof(*priv));
  ret->priv = priv;
  priv->parameters = info->parameters;
 
  desc_func = dlsym(ret->dll_handle, "ladspa_descriptor");
  if(!desc_func)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "No symbol \"ladspa_descriptor\" found: %s",
            dlerror());
    return 0;
    }
  priv->desc = desc_func(info->index);

  if(priv->desc->Properties & LADSPA_PROPERTY_INPLACE_BROKEN) 
    priv->inplace_broken = 1;

  priv->config_ports = calloc(priv->desc->PortCount,
                              sizeof(*priv->config_ports));
  
  //  lp->Instance = 
  
  count_ports(priv->desc, &priv->num_in_ports, &priv->num_out_ports,
              &priv->num_in_c_ports, &priv->num_out_c_ports);

  if(priv->num_in_ports)  
    priv->in_ports = malloc(priv->num_in_ports* 
                            sizeof(*priv->in_ports));
  if(priv->num_out_ports)  
    priv->out_ports = malloc(priv->num_out_ports* 
                            sizeof(*priv->out_ports));

  if(priv->num_in_c_ports)  
    priv->in_c_ports = malloc(priv->num_in_c_ports* 
                            sizeof(*priv->in_c_ports));
  if(priv->num_out_c_ports)  
    priv->out_c_ports = malloc(priv->num_out_c_ports*
                            sizeof(*priv->out_c_ports));

  for(i = 0; i < priv->desc->PortCount; i++)
    {
    if(LADSPA_IS_PORT_AUDIO(priv->desc->PortDescriptors[i]))
      {
      if(LADSPA_IS_PORT_INPUT(priv->desc->PortDescriptors[i]))
        priv->in_ports[in_port_index++] = i;
      else if(LADSPA_IS_PORT_OUTPUT(priv->desc->PortDescriptors[i]))
        priv->out_ports[out_port_index++] = i;
      }
    else if(LADSPA_IS_PORT_CONTROL(priv->desc->PortDescriptors[i]))
      {
      if(LADSPA_IS_PORT_INPUT(priv->desc->PortDescriptors[i]))
        priv->in_c_ports[in_c_port_index++] = i;
      else if(LADSPA_IS_PORT_OUTPUT(priv->desc->PortDescriptors[i]))
        priv->out_c_ports[out_c_port_index++] = i;
      }
    }
  return 1;
  }

#define FREE(p) if(p) free(p)

void bg_ladspa_unload(bg_plugin_handle_t * h)
  {
  ladspa_priv_t * lp;
  lp = (ladspa_priv_t *)h->priv;
  FREE(lp->config_ports);
  FREE(lp->in_ports);
  FREE(lp->out_ports);
  FREE(lp->in_c_ports);
  FREE(lp->out_c_ports);

  if(lp->out_src)
    gavl_audio_source_destroy(lp->out_src);
  
  cleanup_ladspa(lp);
  free(h->plugin_nc);
  free(lp);
  }

#undef FREE
