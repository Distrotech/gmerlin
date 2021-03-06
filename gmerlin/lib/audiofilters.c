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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/converters.h>


#include <gmerlin/utils.h>

#include <gmerlin/filters.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "audiofilters"

/* Audio */

typedef struct
  {
  bg_plugin_handle_t * handle;
  bg_fa_plugin_t     * plugin;
  gavl_audio_source_t * out_src;
  } audio_filter_t;

struct bg_audio_filter_chain_s
  {
  gavl_audio_source_t * out_src; // Last filter element
  gavl_audio_source_t * in_src; // Legacy!!

  /* Legacy !! */
  int in_samples;
  int out_samples;
  gavl_audio_frame_t * out_frame;
  

  
  int num_filters;
  audio_filter_t * filters;
  const bg_gavl_audio_options_t * opt;
  bg_plugin_registry_t * plugin_reg;
  
  bg_parameter_info_t * parameters;
  
  char * filter_string;
  int need_rebuild;
  int need_restart;

  pthread_mutex_t mutex;

  /* Legacy!! */
  bg_read_audio_func_t in_func;
  void * in_data;
  int in_stream;
  
  };

int bg_audio_filter_chain_need_restart(bg_audio_filter_chain_t * ch)
  {
  return ch->need_restart || ch->need_rebuild;
  }


static int audio_filter_create(audio_filter_t * f,
                               bg_audio_filter_chain_t * ch,
                                const char * name)
  {
  const bg_plugin_info_t * info;
  info = bg_plugin_find_by_name(ch->plugin_reg, name);
  if(!info)
    return 0;
  f->handle = bg_plugin_load(ch->plugin_reg, info);
  if(!f->handle)
    return 0;

  f->plugin = (bg_fa_plugin_t*)(f->handle->plugin);
  return 1;
  }

void bg_audio_filter_chain_lock(void * priv)
  {
  bg_audio_filter_chain_t * cnv = priv;
  pthread_mutex_lock(&cnv->mutex);
  }

void bg_audio_filter_chain_unlock(void * priv)
  {
  bg_audio_filter_chain_t * cnv = priv;
  pthread_mutex_unlock(&cnv->mutex);
  }


static void audio_filter_destroy(audio_filter_t * f)
  {
  if(f->handle)
    bg_plugin_unref_nolock(f->handle);
  }

static void destroy_audio_chain(bg_audio_filter_chain_t * ch)
  {
  int i;
  
  /* Destroy previous filters */
  for(i = 0; i < ch->num_filters; i++)
    audio_filter_destroy(&ch->filters[i]);
  if(ch->filters)
    {
    free(ch->filters);
    ch->filters = NULL;
    }
  ch->num_filters = 0;
  }

static void bg_audio_filter_chain_rebuild(bg_audio_filter_chain_t * ch)
  {
  int i;
  char ** filter_names;

  ch->need_rebuild = 0;
  filter_names = bg_strbreak(ch->filter_string, ',');
  
  destroy_audio_chain(ch);
  
  if(!filter_names)
    return;
  
  while(filter_names[ch->num_filters])
    ch->num_filters++;

  ch->filters = calloc(ch->num_filters, sizeof(*ch->filters));
  for(i = 0; i < ch->num_filters; i++)
    {
    audio_filter_create(&ch->filters[i], ch,
                        filter_names[i]);
    }
  bg_strbreak_free(filter_names);
  }

bg_audio_filter_chain_t *
bg_audio_filter_chain_create(const bg_gavl_audio_options_t * opt,
                             bg_plugin_registry_t * plugin_reg)
  {
  bg_audio_filter_chain_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->opt = opt;
  ret->plugin_reg = plugin_reg;
  
  pthread_mutex_init(&ret->mutex, NULL);
  return ret;
  }

static const bg_parameter_info_t params[] =
  {
    {
      .name      = "audio_filters",
      .opt       = "f",
      .long_name = TRS("Audio Filters"),
      .preset_path = "audiofilters",
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .type = BG_PARAMETER_MULTI_CHAIN,
      .flags = BG_PARAMETER_SYNC,
    },
    { /* End */ }
  };

static void create_audio_parameters(bg_audio_filter_chain_t * ch)
  {
  ch->parameters = bg_parameter_info_copy_array(params);
  bg_plugin_registry_set_parameter_info(ch->plugin_reg,
                                        BG_PLUGIN_FILTER_AUDIO,
                                        BG_PLUGIN_FILTER_1,
                                        ch->parameters);
  }

const bg_parameter_info_t *
bg_audio_filter_chain_get_parameters(bg_audio_filter_chain_t * ch)
  {
  if(!ch->parameters)
    create_audio_parameters(ch);
  return ch->parameters;
  }

void
bg_audio_filter_chain_set_parameter(void * data, const char * name,
                                    const bg_parameter_value_t * val)
  {
  bg_audio_filter_chain_t * ch;
  int i;
  char * pos;
  audio_filter_t * f;
  
  ch = (bg_audio_filter_chain_t *)data;
  
  if(!name)
    {
    for(i = 0; i < ch->num_filters; i++)
      {
      if(ch->filters[i].plugin->common.set_parameter)
        ch->filters[i].plugin->common.set_parameter(ch->filters[i].handle->priv,
                                             NULL, NULL);
      }
    return;
    }

  if(!strcmp(name, "audio_filters"))
    {
    if(!ch->filter_string && !val->val_str)
      goto the_end;
    
    if(ch->filter_string && val->val_str &&
       !strcmp(ch->filter_string, val->val_str))
      {
      goto the_end;
      }
    /* Rebuild chain */
    ch->filter_string = gavl_strrep(ch->filter_string, val->val_str);
    ch->need_rebuild = 1;
    }
  else if(!strncmp(name, "audio_filters.", 14))
    {
    if(ch->need_rebuild)
      bg_audio_filter_chain_rebuild(ch);
    
    pos = strchr(name, '.');
    pos++;
    i = atoi(pos);
    f = ch->filters + i;

    pos = strchr(pos, '.');
    if(!pos)
      goto the_end;
    pos++;
    if(f->plugin->common.set_parameter)
      {
      f->plugin->common.set_parameter(f->handle->priv, pos, val);
      if(f->plugin->need_restart && f->plugin->need_restart(f->handle->priv))
        ch->need_restart = 1;
      }
    }
  the_end:
  return;
  }

/* Legacy */
static gavl_source_status_t
read_func_in(void * priv, gavl_audio_frame_t ** frame)
  {
  int num_samples;
  bg_audio_filter_chain_t * ch = priv;

  num_samples =
    (*frame == ch->out_frame) ? ch->out_samples : ch->in_samples;
  
  if(!ch->in_func(ch->in_data, *frame, ch->in_stream, num_samples))
    return GAVL_SOURCE_EOF;
  return GAVL_SOURCE_OK;
  }

int bg_audio_filter_chain_init(bg_audio_filter_chain_t * ch,
                               const gavl_audio_format_t * in_format,
                               gavl_audio_format_t * out_format)
  {
  ch->need_restart = 0;
  

  if(ch->in_src)
    gavl_audio_source_destroy(ch->in_src);
  
  ch->in_src = gavl_audio_source_create(read_func_in, ch, 0, in_format);
  ch->in_samples = in_format->samples_per_frame;
  
  bg_audio_filter_chain_connect(ch, ch->in_src);
  gavl_audio_format_copy(out_format,
                         gavl_audio_source_get_src_format(ch->out_src));
  
  //  if(ch->in_samples > out_format->samples_per_frame)
  //    ch->in_samples = out_format->samples_per_frame;
  return ch->num_filters;
  }

/* LEGACY */
int bg_audio_filter_chain_set_out_format(bg_audio_filter_chain_t * ch,
                                         const gavl_audio_format_t * out_format)
  {
  gavl_audio_source_set_dst(ch->out_src, 0, out_format);
  return 1;
  }

/* LEGACY */
void bg_audio_filter_chain_connect_input(bg_audio_filter_chain_t * ch,
                                         bg_read_audio_func_t func,
                                         void * priv, int stream)
  {
  ch->in_func = func;
  ch->in_data = priv;
  ch->in_stream = stream;
  }

int bg_audio_filter_chain_read(void * priv, gavl_audio_frame_t* frame,
                               int stream, int num_samples)
  {
  bg_audio_filter_chain_t * ch = priv;

  ch->out_frame = frame;
  ch->out_samples = num_samples;
  
  return gavl_audio_source_read_samples(ch->out_src,
                                        frame, num_samples);
  }

void bg_audio_filter_chain_destroy(bg_audio_filter_chain_t * ch)
  {
  if(ch->parameters)
    bg_parameter_info_destroy_array(ch->parameters);

  if(ch->filter_string)
    free(ch->filter_string);
  
  destroy_audio_chain(ch);
  pthread_mutex_destroy(&ch->mutex);
  free(ch);
  }

void bg_audio_filter_chain_reset(bg_audio_filter_chain_t * ch)
  {
  int i;
  for(i = 0; i < ch->num_filters; i++)
    {
    if(ch->filters[i].plugin->reset)
      ch->filters[i].plugin->reset(ch->filters[i].handle->priv);
    gavl_audio_source_reset(ch->filters[i].out_src);
    }
  }

gavl_audio_source_t *
bg_audio_filter_chain_connect(bg_audio_filter_chain_t * ch,
                              gavl_audio_source_t * src)
  {
  int i;

  if(ch->need_rebuild)
    bg_audio_filter_chain_rebuild(ch);
  
  for(i = 0; i < ch->num_filters; i++)
    {
    gavl_audio_options_copy(gavl_audio_source_get_options(src),
                            ch->opt->opt);
    
    ch->filters[i].out_src =
      ch->filters[i].plugin->connect(ch->filters[i].handle->priv,
                                     src, ch->opt->opt);
    src = ch->filters[i].out_src;
    }
  
  ch->out_src = src;
  gavl_audio_source_set_lock_funcs(ch->out_src,
                                   bg_audio_filter_chain_lock,
                                   bg_audio_filter_chain_unlock,
                                   ch);
  return ch->out_src;
  }
