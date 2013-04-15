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
#define LOG_DOMAIN "videofilters"


/* Video */

typedef struct
  {
  bg_plugin_handle_t * handle;
  bg_fv_plugin_t     * plugin;
  gavl_video_source_t * out_src;
  } video_filter_t;

struct bg_video_filter_chain_s
  {
  gavl_video_source_t * out_src; // Last Filter element
  gavl_video_source_t * in_src; // Legacy!!
  
  int num_filters;
  video_filter_t * filters;
  const bg_gavl_video_options_t * opt;
  bg_plugin_registry_t * plugin_reg;
  
  bg_parameter_info_t * parameters;

  char * filter_string;
  
  int need_rebuild;
  int need_restart;
  
  bg_read_video_func_t in_func;
  void * in_data;
  int in_stream;

  
  pthread_mutex_t mutex;
  };

int bg_video_filter_chain_need_restart(bg_video_filter_chain_t * ch)
  {
  return ch->need_restart || ch->need_rebuild;
  }

static int
video_filter_create(video_filter_t * f,
                    bg_video_filter_chain_t * ch,
                    const char * name)
  {
  const bg_plugin_info_t * info;
  info = bg_plugin_find_by_name(ch->plugin_reg, name);
  if(!info)
    return 0;
  f->handle = bg_plugin_load(ch->plugin_reg, info);
  if(!f->handle)
    return 0;

  f->plugin = (bg_fv_plugin_t*)(f->handle->plugin);
  return 1;
  }

static void video_filter_destroy(video_filter_t * f)
  {
  if(f->handle)
    bg_plugin_unref_nolock(f->handle);
  }

static void destroy_video_chain(bg_video_filter_chain_t * ch)
  {
  int i;
  
  /* Destroy previous filters */
  for(i = 0; i < ch->num_filters; i++)
    video_filter_destroy(&ch->filters[i]);
  if(ch->filters)
    {
    free(ch->filters);
    ch->filters = NULL;
    }
  ch->num_filters = 0;
  }

static void bg_video_filter_chain_rebuild(bg_video_filter_chain_t * ch)
  {
  int i;
  char ** filter_names;
  filter_names = bg_strbreak(ch->filter_string, ',');
  
  destroy_video_chain(ch);
  
  if(!filter_names)
    return;
  
  while(filter_names[ch->num_filters])
    ch->num_filters++;

  ch->filters = calloc(ch->num_filters, sizeof(*ch->filters));
  for(i = 0; i < ch->num_filters; i++)
    {
    video_filter_create(&ch->filters[i], ch,
                        filter_names[i]);
    }
  bg_strbreak_free(filter_names);
  ch->need_rebuild = 0;
  }

bg_video_filter_chain_t *
bg_video_filter_chain_create(const bg_gavl_video_options_t * opt,
                             bg_plugin_registry_t * plugin_reg)
  {
  bg_video_filter_chain_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->opt = opt;
  ret->plugin_reg = plugin_reg;
  
  pthread_mutex_init(&ret->mutex, NULL);
  return ret;
  }

static const bg_parameter_info_t params[] =
  {
    {
      .name      = "video_filters",
      .opt       = "f",
      .long_name = TRS("Video Filters"),
      .preset_path = "videofilters",
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .type = BG_PARAMETER_MULTI_CHAIN,
      .flags = BG_PARAMETER_SYNC,
    },
    { /* End */ }
  };

static void create_video_parameters(bg_video_filter_chain_t * ch)
  {
  ch->parameters = bg_parameter_info_copy_array(params);
  bg_plugin_registry_set_parameter_info(ch->plugin_reg,
                                        BG_PLUGIN_FILTER_VIDEO,
                                        BG_PLUGIN_FILTER_1,
                                        ch->parameters);
  }

const bg_parameter_info_t *
bg_video_filter_chain_get_parameters(bg_video_filter_chain_t * ch)
  {
  if(!ch->parameters)
    create_video_parameters(ch);
  return ch->parameters;
  }

void
bg_video_filter_chain_set_parameter(void * data, const char * name,
                                    const bg_parameter_value_t * val)
  {
  int i;
  char * pos;
  video_filter_t * f;
  
  bg_video_filter_chain_t * ch;
  ch = (bg_video_filter_chain_t *)data;
    
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

  
  if(!strcmp(name, "video_filters"))
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
  else if(!strncmp(name, "video_filters.", 14))
    {
    if(ch->need_rebuild)
      bg_video_filter_chain_rebuild(ch);
    
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
      if(f->plugin->need_restart &&
         f->plugin->need_restart(f->handle->priv))
        ch->need_restart = 1;
      }
    }
  the_end:
  return;
  }

/* Legacy */
static gavl_source_status_t
read_func_in(void * priv, gavl_video_frame_t ** frame)
  {
  bg_video_filter_chain_t * ch = priv;
  if(!ch->in_func(ch->in_data, *frame, ch->in_stream))
    return GAVL_SOURCE_EOF;
  return GAVL_SOURCE_OK;
  }


int bg_video_filter_chain_init(bg_video_filter_chain_t * ch,
                               const gavl_video_format_t * in_format,
                               gavl_video_format_t * out_format)
  {
  ch->need_restart = 0;
  

  if(ch->in_src)
    gavl_video_source_destroy(ch->in_src);

  ch->in_src = gavl_video_source_create(read_func_in, ch, 0, in_format);

  bg_video_filter_chain_connect(ch, ch->in_src);
  gavl_video_format_copy(out_format, gavl_video_source_get_src_format(ch->out_src));
  return ch->num_filters;
  }

void bg_video_filter_chain_connect_input(bg_video_filter_chain_t * ch,
                                         bg_read_video_func_t func, void * priv,
                                         int stream)
  {
  ch->in_func = func;
  ch->in_data = priv;
  ch->in_stream = stream;
  }

int bg_video_filter_chain_read(void * priv, gavl_video_frame_t* frame,
                               int stream)
  {
  bg_video_filter_chain_t * ch = priv;
  return (gavl_video_source_read_frame(ch->out_src, &frame) == GAVL_SOURCE_OK);
  }

int bg_video_filter_chain_set_out_format(bg_video_filter_chain_t * ch,
                                         const gavl_video_format_t * out_format)
  {
  gavl_video_source_set_dst(ch->out_src, 0, out_format);
  return 1; // Unused anyway?
  }


void bg_video_filter_chain_destroy(bg_video_filter_chain_t * ch)
  {
  if(ch->parameters)
    bg_parameter_info_destroy_array(ch->parameters);
  if(ch->filter_string)
    free(ch->filter_string);
  destroy_video_chain(ch);

  if(ch->in_src)
    gavl_video_source_destroy(ch->in_src);

  pthread_mutex_destroy(&ch->mutex);
  
  free(ch);
  }

void bg_video_filter_chain_lock(void * priv)
  {
  bg_video_filter_chain_t * cnv = priv;
  pthread_mutex_lock(&cnv->mutex);
  }

void bg_video_filter_chain_unlock(void * priv)
  {
  bg_video_filter_chain_t * cnv = priv;
  pthread_mutex_unlock(&cnv->mutex);
  }

void bg_video_filter_chain_reset(bg_video_filter_chain_t * ch)
  {
  int i;
  for(i = 0; i < ch->num_filters; i++)
    {
    if(ch->filters[i].plugin->reset)
      ch->filters[i].plugin->reset(ch->filters[i].handle->priv);
    gavl_video_source_reset(ch->filters[i].out_src);
    }
  }

gavl_video_source_t *
bg_video_filter_chain_connect(bg_video_filter_chain_t * ch,
                              gavl_video_source_t * src)
  {
  int i;

  if(ch->need_rebuild)
    bg_video_filter_chain_rebuild(ch);
  
  for(i = 0; i < ch->num_filters; i++)
    {
    gavl_video_options_copy(gavl_video_source_get_options(src),
                            ch->opt->opt);
    
    ch->filters[i].out_src =
      ch->filters[i].plugin->connect(ch->filters[i].handle->priv,
                                     src, ch->opt->opt);
    src = ch->filters[i].out_src;
    }
  
  ch->out_src = src;
  gavl_video_source_set_lock_funcs(ch->out_src,
                                   bg_video_filter_chain_lock,
                                   bg_video_filter_chain_unlock,
                                   ch);
  
  return ch->out_src;
  }
