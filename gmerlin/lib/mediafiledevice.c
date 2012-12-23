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

#include <config.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gavl/metatags.h>

#include <gmerlin/translation.h>
#include <gmerlin/converters.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/pluginregistry.h>

#include <gmerlin/mediafiledevice.h>
#include <gmerlin/utils.h>
#include <gmerlin/tree.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "mediafiledevice"

#define SAMPLES_PER_FRAME 1024

typedef struct
  {
  bg_plugin_registry_t * plugin_reg;

  gavl_audio_format_t in_format;
  gavl_audio_format_t out_format;

  bg_audio_converter_t * cnv;
  gavl_audio_options_t * opt;

  int num_files;
  int files_alloc;
  int current;
  int do_shuffle;
  char * album_file;
  
  struct
    {
    char * location;
    char * plugin;
    int track;
    } * files;

  /* Shuffle list */
  int * indices;
  
  bg_plugin_handle_t * h;
  bg_input_plugin_t * plugin;

  bg_read_audio_func_t in_func;
  void * in_data;
  int    in_stream;
  gavl_audio_frame_t * frame;

  int64_t sample_counter;

  gavl_timer_t * timer;
  bg_recorder_callbacks_t * callbacks;
  
  time_t mtime;
  } audiofile_t;

static int get_mtime(const char * file, time_t * ret)
  {
  struct stat st;
  if(stat(file, &st))
    return 0;
  *ret = st.st_mtime;
  return 1;
  }

static const bg_parameter_info_t parameters_audio[] =
  {
    {
      .name =        "channel_mode",
      .long_name =   TRS("Channel Mode"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "stereo" },
      .multi_names =   (char const *[]){ "mono", "stereo", NULL },
      .multi_labels =  (char const *[]){ TRS("Mono"), TRS("Stereo"), NULL },
    },
    {
      .name =        "samplerate",
      .long_name =   TRS("Samplerate [Hz]"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 44100 },
      .val_min =     { .val_i =  8000 },
      .val_max =     { .val_i = 96000 },
    },
    {
      .name =        "album",
      .long_name =   TRS("Album"),
      .type        =   BG_PARAMETER_FILE,
    },
    {
      .name =       "shuffle",
      .long_name =     TRS("Shuffle"),
      .type      =   BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of parameters */ }
  };


static const bg_parameter_info_t * get_parameters_audio(void * priv)
  {
  return parameters_audio;
  }


static void
set_parameter_audio(void * p, const char * name,
                    const bg_parameter_value_t * val)
  {
  audiofile_t * m = p;
  if(!name)
    return;

  if(!strcmp(name, "channel_mode"))
    {
    if(!strcmp(val->val_str, "mono"))
      m->out_format.num_channels = 1;
    else if(!strcmp(val->val_str, "stereo"))
      m->out_format.num_channels = 2;
    }
  else if(!strcmp(name, "samplerate"))
    m->out_format.samplerate = val->val_i;
  else if(!strcmp(name, "album"))
    m->album_file = bg_strdup(m->album_file, val->val_str);
  else if(!strcmp(name, "shuffle"))
    m->do_shuffle = val->val_i;
  
  }

static void destroy_audio(void * p)
  {
  audiofile_t * m = p;
  bg_audio_converter_destroy(m->cnv);
  gavl_audio_options_destroy(m->opt);
  gavl_timer_destroy(m->timer);
  }

static void free_track_list(audiofile_t * m)
  {
  int i;

  if(m->files)
    {
    for(i = 0; i < m->num_files; i++)
      {
      if(m->files[i].location)
        free(m->files[i].location);
      if(m->files[i].plugin)
        free(m->files[i].plugin);
      }
    free(m->files);
    m->files = NULL;
    }
  if(m->indices)
    {
    free(m->indices);
    m->indices = NULL;
    }
  m->num_files = 0;
  m->files_alloc = 0;
  m->current = 0;
  }

static void append_track(audiofile_t * m,
                         const char * location,
                         const char * plugin,
                         int track)
  {
  if(m->num_files + 1 > m->files_alloc)
    {
    m->files_alloc += 128;
    m->files = realloc(m->files, sizeof(*m->files) * m->files_alloc);
    memset(m->files + m->num_files, 0,
           sizeof(*m->files) * (m->files_alloc - m->num_files));
    }
  m->files[m->num_files].location =
    bg_strdup(m->files[m->num_files].location, location);
  m->files[m->num_files].plugin =
    bg_strdup(m->files[m->num_files].plugin, plugin);
  m->files[m->num_files].track = track;
  m->num_files++;
  }

static int build_track_list(audiofile_t * m)
  {
  bg_album_entry_t * entries;
  bg_album_entry_t * e;
  char * album_xml;

  free_track_list(m);
  
  if(!m->album_file)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No album file given");
    return 0;
    }
  album_xml = bg_read_file(m->album_file, NULL);

  if(!album_xml)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s could not be opened",
           m->album_file);
    return 0;
    }
  entries = bg_album_entries_new_from_xml(album_xml);
  free(album_xml);
  
  if(!entries)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s contains no entries",
           m->album_file);
    return 0;
    }
  e = entries;

  while(e)
    {
    if(!e->num_audio_streams)
      {
      e = e->next;
      continue;
      }
    append_track(m, e->location, e->plugin, e->index);
    e = e->next;
    }
  if(entries)
    bg_album_entries_destroy(entries);
  
  if(!m->num_files)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s contains no usable entries",
           m->album_file);
    return 0;
    }

  if(m->do_shuffle)
    {
    int i, index;
    int tmp;
    m->indices = malloc(sizeof(*m->indices) * m->num_files);

    for(i = 0; i < m->num_files; i++)
      m->indices[i] = i;
    
    for(i = 0; i < m->num_files; i++)
      {
      index = rand() % m->num_files;
      tmp = m->indices[i];
      m->indices[i] = m->indices[index];
      m->indices[index] = tmp;
      }
    }
  
  return m->num_files;
  }

static int open_file(audiofile_t * m)
  {
  bg_track_info_t * ti;
  int num_tracks;
  const bg_plugin_info_t * info;
  int idx;
  time_t new_mtime;

  if(get_mtime(m->album_file, &new_mtime) && (new_mtime != m->mtime))
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Reloading album (file changed)");
    m->mtime = new_mtime;
    if(!build_track_list(m))
      return 0;
    }
  
  idx = m->indices ? m->indices[m->current] : m->current;
  if(m->h)
    {
    bg_plugin_unref(m->h); 
    m->h = NULL;
    }
  
  if(m->files[idx].plugin)
    info = bg_plugin_find_by_name(m->plugin_reg, m->files[idx].plugin);
  else
    info = NULL;
  
  if(!bg_input_plugin_load(m->plugin_reg,
                           m->files[idx].location,
                           info, &m->h, NULL, 0))
    return 0;

  m->plugin = (bg_input_plugin_t *)m->h->plugin;

  /* Select track */
  if(m->plugin->get_num_tracks)
    num_tracks = m->plugin->get_num_tracks(m->h->priv);
  else
    num_tracks = 1;
  
  if(num_tracks <= m->files[idx].track)
    return 0;

  ti = m->plugin->get_track_info(m->h->priv, m->files[idx].track);
  
  if(m->plugin->set_track)
    m->plugin->set_track(m->h->priv, m->files[idx].track);

  if(!ti->num_audio_streams)
    return 0;

  m->plugin->set_audio_stream(m->h->priv, 0, BG_STREAM_ACTION_DECODE);

  if(m->callbacks && m->callbacks->metadata_changed)
    {
    if(!gavl_metadata_get(&ti->metadata, GAVL_META_LABEL))
      {
      gavl_metadata_set_nocpy(&ti->metadata, GAVL_META_LABEL,
                              bg_get_track_name_default(m->files[idx].location,
                                                        m->files[idx].track,
                                                        num_tracks));
      }
    m->callbacks->metadata_changed(m->callbacks->data, &ti->metadata);
    }
  
  if(!m->plugin->start(m->h->priv))
    return 0;

  gavl_audio_format_copy(&m->in_format, &ti->audio_streams[0].format);

  /* Input channel */

  m->in_func = m->plugin->read_audio;
  m->in_data = m->h->priv;
  m->in_stream = 0;
  
  /* Set up format converter */
  
  if(bg_audio_converter_init(m->cnv,
                             &m->in_format,
                             &m->out_format))
    {
    bg_audio_converter_connect_input(m->cnv, m->in_func, m->in_data, m->in_stream);
    m->in_func = bg_audio_converter_read;
    m->in_data = m->cnv;
    m->in_stream = 0;
    }
  return 1;
  }

static int open_audio(void * p,
                      gavl_audio_format_t * format,
                      gavl_video_format_t * video_format, gavl_metadata_t * metadata)
  {
  audiofile_t * m = p;
  /* Finalize audio format */

  m->sample_counter = 0;
  
  m->out_format.channel_locations[0] = GAVL_CHID_NONE;
  gavl_set_channel_setup(&m->out_format);

  gavl_audio_format_copy(format, &m->out_format);
  m->frame = gavl_audio_frame_create(&m->out_format);

  if(!get_mtime(m->album_file, &m->mtime))
    return 0;
  
  /* Create track list */
  if(!build_track_list(m))
    return 0;

  m->current = 0;
  
  /* Open first file */
  if(!open_file(m))
    return 0;
  gavl_timer_start(m->timer);
  return 1;
  }

static void close_audio(void * p)
  {
  audiofile_t * m = p;
  gavl_timer_stop(m->timer);
  }

static int read_frame_audio(void * p, gavl_audio_frame_t * f,
                            int stream,
                            int num_samples)
  {
  int samples_read;
  int samples_to_read;
  gavl_time_t current_time;
  gavl_time_t diff_time;
  
  audiofile_t * m = p;

  f->valid_samples = 0;
  
  while(f->valid_samples < num_samples)
    {
    samples_to_read = SAMPLES_PER_FRAME;

    if(samples_to_read + f->valid_samples > num_samples)
      samples_to_read = num_samples - f->valid_samples;
    
    samples_read = m->in_func(m->in_data, m->frame,
                              m->in_stream, samples_to_read);

    if(samples_read > 0)
      {
      gavl_audio_frame_copy(&m->out_format,
                            f, m->frame,
                            f->valid_samples /* dst_pos */,
                            0                /* src_pos */,
                            samples_read,
                            samples_read);
      f->valid_samples += samples_read;
      }
    else /* Open next file */
      {
      while(1)
        {
        m->current++;
        if(m->current >= m->num_files)
          m->current = 0;
        if(open_file(m))
          break;
        }
      
      }
    }

  /* Wait until we can output the data */
  
  current_time = gavl_timer_get(m->timer);
  diff_time =
    gavl_time_unscale(m->out_format.samplerate, m->sample_counter) -
    current_time;

  if(diff_time > 0)
    gavl_time_delay(&diff_time);
  
  m->sample_counter += f->valid_samples;
  return f->valid_samples;
  }

static void set_callbacks_audio(void * p, bg_recorder_callbacks_t * callbacks)
  {
  audiofile_t * m = p;
  m->callbacks = callbacks;
  }

static const bg_recorder_plugin_t audiofile_input =
  {
    .common =
    {
      BG_LOCALE,
      .name =          bg_audiofiledevice_name,
      .long_name =     TRS("Audiofile recorder"),
      .description =   TRS("Take a bunch of audio file and make them available as a recording device"),
      .type =          BG_PLUGIN_RECORDER_AUDIO,
      .flags =         BG_PLUGIN_RECORDER,
      .priority =      1,
      .destroy =       destroy_audio,

      .get_parameters = get_parameters_audio,
      .set_parameter =  set_parameter_audio,
    },
    .set_callbacks = set_callbacks_audio,
    .open =          open_audio,
    .read_audio =    read_frame_audio,
    .close =         close_audio,

  };

bg_plugin_info_t * bg_audiofiledevice_info(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * ret;

  const bg_recorder_plugin_t * plugin = &audiofile_input;
  
  if(!bg_plugin_registry_get_num_plugins(reg, BG_PLUGIN_INPUT, BG_PLUGIN_FILE))
    return NULL;
  
  ret = calloc(1, sizeof(*ret));

  ret->gettext_domain      = bg_strdup(ret->gettext_domain, plugin->common.gettext_domain);
  
  ret->gettext_directory   = bg_strdup(ret->gettext_directory,
                                       plugin->common.gettext_directory);
  
  
  ret->name      = bg_strdup(ret->name, plugin->common.name);
  ret->long_name = bg_strdup(ret->long_name, plugin->common.long_name);
  ret->description = bg_strdup(ret->description, plugin->common.description);
  
  ret->priority  =  plugin->common.priority;
  ret->type  =  plugin->common.type;
  ret->flags =  plugin->common.flags;
  ret->parameters = bg_parameter_info_copy_array(parameters_audio);
  return ret;
  }

const bg_plugin_common_t * bg_audiofiledevice_get()
  {
  return (const bg_plugin_common_t*)(&audiofile_input);
  }

void * bg_audiofiledevice_create(bg_plugin_registry_t * reg)
  {
  audiofile_t * ret = calloc(1, sizeof(*ret));
  ret->plugin_reg = reg;

  /* Set common audio format parameters */
  ret->out_format.sample_format = GAVL_SAMPLE_FLOAT;
  ret->out_format.interleave_mode = GAVL_INTERLEAVE_NONE;
  ret->out_format.samples_per_frame = SAMPLES_PER_FRAME;
  
  ret->opt = gavl_audio_options_create();
  ret->cnv = bg_audio_converter_create(ret->opt);
  ret->timer = gavl_timer_create();
  
  return ret;
  } 
