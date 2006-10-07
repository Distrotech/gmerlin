/*****************************************************************
 
  pluginregistry.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <cfg_registry.h>
#include <pluginregistry.h>
#include <pluginreg_priv.h>
#include <config.h>
#include <utils.h>
#include <singlepic.h>
#include <log.h>

#define LOG_DOMAIN "pluginregistry"

struct bg_plugin_registry_s
  {
  bg_plugin_info_t * entries;
  bg_cfg_section_t * config_section;

  bg_plugin_info_t * singlepic_input;
  bg_plugin_info_t * singlepic_stills_input;
  bg_plugin_info_t * singlepic_encoder;

  int encode_audio_to_video;
  int encode_subtitle_text_to_video;
  int encode_subtitle_overlay_to_video;
  
  int encode_pp;
  };

static void free_info(bg_plugin_info_t * info)
  {
  if(info->name)
    free(info->name);
  if(info->long_name)
    free(info->long_name);
  if(info->mimetypes)
    free(info->mimetypes);
  if(info->extensions)
    free(info->extensions);
  if(info->protocols)
    free(info->protocols);
  if(info->module_filename)
    free(info->module_filename);
  if(info->devices)
    bg_device_info_destroy(info->devices);

  if(info->parameters)
    bg_parameter_info_destroy_array(info->parameters);
  if(info->audio_parameters)
    bg_parameter_info_destroy_array(info->audio_parameters);
  if(info->video_parameters)
    bg_parameter_info_destroy_array(info->video_parameters);
  if(info->subtitle_text_parameters)
    bg_parameter_info_destroy_array(info->subtitle_text_parameters);
  if(info->subtitle_overlay_parameters)
    bg_parameter_info_destroy_array(info->subtitle_overlay_parameters);
  
  free(info);
  }

static void free_info_list(bg_plugin_info_t * entries)
  {
  bg_plugin_info_t * info;
  
  info = entries;

  while(info)
    {
    entries = info->next;
    free_info(info);
    info = entries;
    }
  }


static bg_plugin_info_t *
find_by_dll(bg_plugin_info_t * info, const char * filename)
  {
  while(info)
    {
    if(info->module_filename && !strcmp(info->module_filename, filename))
      return info;
    info = info->next;
    }
  return (bg_plugin_info_t*)0;
  }

static bg_plugin_info_t *
find_by_name(bg_plugin_info_t * info, const char * name)
  {
  //  fprintf(stderr, "Find by name %s\n", name);
  while(info)
    {
    //    fprintf(stderr, "Trying %s\n", info->name);
    if(!strcmp(info->name, name))
      return info;
    info = info->next;
    }
  //  fprintf(stderr, "%s not found\n", name);
  return (bg_plugin_info_t*)0;
  }

const bg_plugin_info_t * bg_plugin_find_by_name(bg_plugin_registry_t * reg,
                                                const char * name)
  {
  return find_by_name(reg->entries, name);
  }

static bg_plugin_info_t * find_by_long_name(bg_plugin_info_t * info,
                                                  const char * name)
  {
  while(info)
    {
    if(!strcmp(info->long_name, name))
      return info;
    info = info->next;
    }
  return (bg_plugin_info_t*)0;
  }

const bg_plugin_info_t * bg_plugin_find_by_protocol(bg_plugin_registry_t * reg,
                                                    const char * protocol)
  {
  const bg_plugin_info_t * info = reg->entries;
  while(info)
    {
    if(bg_string_match(protocol, info->protocols))
      return info;
    info = info->next;
    }
  return (bg_plugin_info_t*)0;
  }

const bg_plugin_info_t * bg_plugin_find_by_long_name(bg_plugin_registry_t *
                                                     reg, const char * name)
  {
  return find_by_long_name(reg->entries, name);
  }


const bg_plugin_info_t * bg_plugin_find_by_filename(bg_plugin_registry_t * reg,
                                                    const char * filename,
                                                    int typemask)
  {
  char * extension;
  bg_plugin_info_t * info, *ret = (bg_plugin_info_t*)0;
  int max_priority = BG_PLUGIN_PRIORITY_MIN - 1;

  if(!filename)
    return (const bg_plugin_info_t*)0;
  
  //  fprintf(stderr, "bg_plugin_find_by_filename %08x %s\n", typemask, filename);
  
  info = reg->entries;
  extension = strrchr(filename, '.');

  if(!extension)
    {
    return (const bg_plugin_info_t *)0;
    }
  extension++;
  
  //  fprintf(stderr, "info %p\n", info);
  
  while(info)
    {
    //    fprintf(stderr, "Trying: %08x %s %s\n", info->type, info->long_name, info->extensions);
    if(!(info->type & typemask) ||
       !(info->flags & BG_PLUGIN_FILE) ||
       !info->extensions)
      {
      //      fprintf(stderr, "skipping plugin: %s\n",
      //              info->long_name);
      info = info->next;
      continue;
      }
    if(bg_string_match(extension, info->extensions))
      {
      //      fprintf(stderr, "%s looks good %d %d\n", info->name, max_priority, info->priority);
      if(max_priority < info->priority)
        {
        max_priority = info->priority;
        ret = info;
        }
      // return info;
      }
    info = info->next;
    }
  return ret;
  }

static bg_plugin_info_t * remove_from_list(bg_plugin_info_t * list,
                                    bg_plugin_info_t * info)
  {
  bg_plugin_info_t * before;
  if(info == list)
    {
    list = list->next;
    return list;
    }

  before = list;

  while(before->next != info)
    before = before->next;
    
  before->next = info->next;
  info->next = (bg_plugin_info_t*)0;
  return list;
  }

static int check_plugin_version(void * handle)
  {
  int (*get_plugin_api_version)();

  get_plugin_api_version = dlsym(handle, "get_plugin_api_version");
  if(!get_plugin_api_version)
    return 0;

  if(get_plugin_api_version() != BG_PLUGIN_API_VERSION)
    return 0;
  return 1;
  }

static bg_plugin_info_t *
scan_directory(const char * directory, bg_plugin_info_t ** _file_info,
               int * changed,
               bg_cfg_section_t * cfg_section)
  {
  bg_plugin_info_t * ret;
  bg_plugin_info_t * end = (bg_plugin_info_t *)0;
  DIR * dir;
  struct dirent * entry;
  char filename[PATH_MAX];
  struct stat st;
  char * pos;
  void * test_module;
  bg_plugin_common_t * plugin;
  
  bg_plugin_info_t * file_info;
  bg_plugin_info_t *  new_info;
  bg_encoder_plugin_t * encoder;
  bg_input_plugin_t  * input;
  
  bg_cfg_section_t * plugin_section;
  bg_cfg_section_t * stream_section;
  bg_parameter_info_t * parameter_info;
  void * plugin_priv;

  if(_file_info)
    file_info = *_file_info;
  else
    file_info = (bg_plugin_info_t *)0;
  
  ret = (bg_plugin_info_t *)0;
    
  dir = opendir(directory);
  
  if(!dir)
    return (bg_plugin_info_t*)0;

  while((entry = readdir(dir)))
    {
    /* Check for the filename */
    
    pos = strrchr(entry->d_name, '.');
    if(!pos)
      continue;
    
    if(strcmp(pos, ".so"))
      continue;
    
    sprintf(filename, "%s/%s", directory, entry->d_name);
    if(stat(filename, &st))
      continue;

    /* Check if the plugin is already in the registry */

    new_info = find_by_dll(file_info, filename);
    if(new_info)
      {
      if((st.st_mtime == new_info->module_time) &&
         (bg_cfg_section_has_subsection(cfg_section,
                                        new_info->name)))
        {
        file_info = remove_from_list(file_info, new_info);
        //        fprintf(stderr, "Found cached plugin %s\n", new_info->name);
        if(!ret)
          {
          ret = new_info;
          end = ret;
          }
        else
          {
          end->next = new_info;
          end = end->next;
          }
        end->next = NULL;
        continue;
        }
      }

    if(!(*changed))
      {
      *changed = 1;
      closedir(dir);
      if(_file_info)
        *_file_info = file_info;
      return ret;
      }
    
    /* Open the DLL and see what's inside */

    test_module = dlopen(filename, RTLD_NOW);
    if(!test_module)
      {
      fprintf(stderr, "Cannot dlopen %s: %s\n", filename, dlerror());
      continue;
      }
    if(!check_plugin_version(test_module))
      {
      fprintf(stderr, "Plugin %s has no or wrong version\n", filename);
      dlclose(test_module);
      continue;
      }
    plugin = (bg_plugin_common_t*)(dlsym(test_module, "the_plugin"));
    if(!plugin)
      {
      fprintf(stderr, "No symbol the_plugin in %s\n", filename);
      dlclose(test_module);
      continue;
      }
    if(!plugin->priority)
      fprintf(stderr, "Warning: Plugin %s has zero priority\n",
              plugin->name);
    new_info = calloc(1, sizeof(*new_info));
    new_info->name = bg_strdup(new_info->name, plugin->name);

    new_info->long_name =  bg_strdup(new_info->long_name,
                                     plugin->long_name);
    new_info->mimetypes =  bg_strdup(new_info->mimetypes,
                                     plugin->mimetypes);
    new_info->extensions = bg_strdup(new_info->extensions,
                                     plugin->extensions);
    new_info->module_filename = bg_strdup(new_info->module_filename,
                                          filename);
    new_info->module_time = st.st_mtime;
    new_info->type        = plugin->type;
    new_info->flags       = plugin->flags;
    new_info->priority    = plugin->priority;
    
    /* Create parameter entries in the registry */

    plugin_section =
      bg_cfg_section_find_subsection(cfg_section, plugin->name);

    plugin_priv = plugin->create();
    
    if(plugin->get_parameters)
      {
          
      parameter_info = plugin->get_parameters(plugin_priv);
      
      bg_cfg_section_create_items(plugin_section,
                                  parameter_info);

      new_info->parameters = bg_parameter_info_copy_array(parameter_info);
      }
    
    if(plugin->type & (BG_PLUGIN_ENCODER_AUDIO|
                       BG_PLUGIN_ENCODER_VIDEO|
                       BG_PLUGIN_ENCODER_SUBTITLE_TEXT |
                       BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY |
                       BG_PLUGIN_ENCODER ))
      {
      encoder = (bg_encoder_plugin_t*)plugin;
      new_info->max_audio_streams = encoder->max_audio_streams;
      new_info->max_video_streams = encoder->max_video_streams;
      new_info->max_subtitle_text_streams = encoder->max_subtitle_text_streams;
      new_info->max_subtitle_overlay_streams = encoder->max_subtitle_overlay_streams;
      
      if(encoder->get_audio_parameters)
        {
        parameter_info = encoder->get_audio_parameters(plugin_priv);
        stream_section = bg_cfg_section_find_subsection(plugin_section,
                                                        "$audio");
        
        bg_cfg_section_create_items(stream_section,
                                    parameter_info);
        new_info->audio_parameters = bg_parameter_info_copy_array(parameter_info);
        }

      if(encoder->get_video_parameters)
        {
        parameter_info = encoder->get_video_parameters(plugin_priv);
        stream_section = bg_cfg_section_find_subsection(plugin_section,
                                                        "$video");
        
        bg_cfg_section_create_items(stream_section,
                                    parameter_info);
        new_info->video_parameters = bg_parameter_info_copy_array(parameter_info);
        }
      if(encoder->get_subtitle_text_parameters)
        {
        parameter_info = encoder->get_subtitle_text_parameters(plugin_priv);
        stream_section = bg_cfg_section_find_subsection(plugin_section,
                                                        "$subtitle_text");
        
        bg_cfg_section_create_items(stream_section,
                                    parameter_info);
        new_info->subtitle_text_parameters = bg_parameter_info_copy_array(parameter_info);
        }
      if(encoder->get_subtitle_overlay_parameters)
        {
        parameter_info = encoder->get_subtitle_overlay_parameters(plugin_priv);
        stream_section = bg_cfg_section_find_subsection(plugin_section,
                                                        "$subtitle_overlay");
        
        bg_cfg_section_create_items(stream_section,
                                    parameter_info);
        new_info->subtitle_overlay_parameters = bg_parameter_info_copy_array(parameter_info);
        }
      }
    if(plugin->type & (BG_PLUGIN_INPUT))
      {
      input = (bg_input_plugin_t*)plugin;
      if(input->protocols)
        new_info->protocols = bg_strdup(new_info->protocols,
                                        input->protocols);
      }
    
    if(plugin->find_devices)
      new_info->devices = plugin->find_devices();
    
    plugin->destroy(plugin_priv);
    dlclose(test_module);
    if(ret)
      {
      end->next = new_info;
      end = end->next;
      }
    else
      {
      ret = new_info;
      end = ret;
      }

    //    if(new_info)
    //      fprintf(stderr, "Loaded plugin %s\n", new_info->name);
    }
  
  closedir(dir);
  if(_file_info)
    *_file_info = file_info;
  
  return ret;
  }

bg_plugin_registry_t *
bg_plugin_registry_create(bg_cfg_section_t * section)
  {
  bg_plugin_registry_t * ret;
  bg_plugin_info_t * file_info;
  bg_plugin_info_t * tmp_info;
  char * filename;
  int changed = 0;
  ret = calloc(1, sizeof(*ret));
  ret->config_section = section;

  /* Load registry file */

  file_info = (bg_plugin_info_t*)0; 
  
  filename = bg_search_file_read("", "plugins.xml");
  if(filename)
    {
    file_info = bg_plugin_registry_load(filename);
    free(filename);
    }
  
  tmp_info = scan_directory(GMERLIN_PLUGIN_DIR,
                            &file_info, &changed,
                            section);
  
  ret->entries = tmp_info;
  
  if(file_info)
    {
    changed = 1;
    free_info_list(file_info);
    }
  
  if(changed)
    {
    /* Reload the entire registry to prevent mysterious crashes */
    free_info_list(ret->entries);
    ret->entries = scan_directory(GMERLIN_PLUGIN_DIR,
                                  (bg_plugin_info_t**)0, &changed,
                                  section);
    //    bg_plugin_registry_save(ret->entries);
    }
  bg_plugin_registry_save(ret->entries);
  
  /* Now we have all external plugins, time to create the meta plugins */

  tmp_info = ret->entries;
  while(tmp_info->next)
    tmp_info = tmp_info->next;

  
  ret->singlepic_input = bg_singlepic_input_info(ret);

  if(ret->singlepic_input)
    {
    //    fprintf(stderr, "Found Singlepicture input\n");
    tmp_info->next = ret->singlepic_input;
    tmp_info = tmp_info->next;
    }

  ret->singlepic_stills_input = bg_singlepic_stills_input_info(ret);

  if(ret->singlepic_stills_input)
    {
    //    fprintf(stderr, "Found Singlepicture stills input\n");
    tmp_info->next = ret->singlepic_stills_input;
    tmp_info = tmp_info->next;
    }
  
  ret->singlepic_encoder = bg_singlepic_encoder_info(ret);

  if(ret->singlepic_encoder)
    {
    //    fprintf(stderr, "Found Singlepicture encoder\n");
    tmp_info->next = ret->singlepic_encoder;
    tmp_info = tmp_info->next;
    }
  
  /* Get flags */

  bg_cfg_section_get_parameter_int(ret->config_section, "encode_audio_to_video",
                                   &(ret->encode_audio_to_video));
  bg_cfg_section_get_parameter_int(ret->config_section, "encode_subtitle_text_to_video",
                                   &(ret->encode_subtitle_text_to_video));
  bg_cfg_section_get_parameter_int(ret->config_section, "encode_subtitle_overlay_to_video",
                                   &(ret->encode_subtitle_overlay_to_video));
  
  bg_cfg_section_get_parameter_int(ret->config_section, "encode_pp",
                                   &(ret->encode_pp));
    
  return ret;
  }

void bg_plugin_registry_destroy(bg_plugin_registry_t * reg)
  {
  bg_plugin_info_t * info;

  info = reg->entries;

  while(info)
    {
    reg->entries = info->next;
    free_info(info);
    info = reg->entries;
    }
  free(reg);
  }

static bg_plugin_info_t * find_by_index(bg_plugin_info_t * info,
                                        int index, uint32_t type_mask,
                                        uint32_t flag_mask)
  {
  int i;

  i = 0;

  bg_plugin_info_t * test_info;
  
  test_info = info;

  while(test_info)
    {
    if((test_info->type & type_mask) &&
       ((flag_mask == BG_PLUGIN_ALL) ||
        (!test_info->flags && !flag_mask) || (test_info->flags & flag_mask)))
      {
      if(i == index)
        return test_info;
      i++;
      }
    test_info = test_info->next;
    }
  return (bg_plugin_info_t*)0;
  }

static bg_plugin_info_t * find_by_priority(bg_plugin_info_t * info,
                                           uint32_t type_mask,
                                           uint32_t flag_mask)
  {
  bg_plugin_info_t * test_info, *ret = (bg_plugin_info_t*)0;
  int priority_max = BG_PLUGIN_PRIORITY_MIN - 1;
  
  test_info = info;

  while(test_info)
    {
    if((test_info->type & type_mask) &&
       ((flag_mask == BG_PLUGIN_ALL) ||
        (test_info->flags & flag_mask) ||
        (!test_info->flags && !flag_mask)))
      {
      if(priority_max < test_info->priority)
        {
        priority_max = test_info->priority;
        ret = test_info;
        }
      }
    test_info = test_info->next;
    }
  return ret;
  }

const bg_plugin_info_t *
bg_plugin_find_by_index(bg_plugin_registry_t * reg, int index,
                        uint32_t type_mask, uint32_t flag_mask)
  {
  return find_by_index(reg->entries, index,
                       type_mask, flag_mask);
  }

int bg_plugin_registry_get_num_plugins(bg_plugin_registry_t * reg,
                                       uint32_t type_mask, uint32_t flag_mask)
  {
  bg_plugin_info_t * info;
  int ret = 0;
  
  info = reg->entries;

  while(info)
    {
    if((info->type & type_mask) &&
       ((!info->flags && !flag_mask) || (info->flags & flag_mask)))
      ret++;

    //    fprintf(stderr, "Tried %s %d\n", info->name, ret);
    info = info->next;
    }
  return ret;
  }

void bg_plugin_registry_set_extensions(bg_plugin_registry_t * reg,
                                       const char * plugin_name,
                                       const char * extensions)
  {
  bg_plugin_info_t * info;
  info = find_by_name(reg->entries, plugin_name);
  if(!info)
    return;
  if(!(info->flags & BG_PLUGIN_FILE))
    return;
  info->extensions = bg_strdup(info->extensions, extensions);

  fprintf(stderr, "Set extensions: %s\n", extensions);
  
  bg_plugin_registry_save(reg->entries);
  
  }

void bg_plugin_registry_set_protocols(bg_plugin_registry_t * reg,
                                      const char * plugin_name,
                                      const char * protocols)
  {
  bg_plugin_info_t * info;
  info = find_by_name(reg->entries, plugin_name);
  if(!info)
    return;
  if(!(info->flags & BG_PLUGIN_URL))
    return;
  info->protocols = bg_strdup(info->protocols, protocols);
  bg_plugin_registry_save(reg->entries);

  }

void bg_plugin_registry_set_priority(bg_plugin_registry_t * reg,
                                     const char * plugin_name,
                                     int priority)
  {
  bg_plugin_info_t * info;
  info = find_by_name(reg->entries, plugin_name);
  if(!info)
    return;
  info->priority = priority;
  bg_plugin_registry_save(reg->entries);
  }

bg_cfg_section_t *
bg_plugin_registry_get_section(bg_plugin_registry_t * reg,
                               const char * plugin_name)
  {
  return bg_cfg_section_find_subsection(reg->config_section, plugin_name);
  }

static struct
  {
  bg_plugin_type_t type;
  char * key;
  } default_keys[] =
  {
    { BG_PLUGIN_OUTPUT_AUDIO,                    "default_audio_output"   },
    { BG_PLUGIN_OUTPUT_VIDEO,                    "default_video_output"   },
    { BG_PLUGIN_RECORDER_AUDIO,                  "default_audio_recorder" },
    { BG_PLUGIN_RECORDER_VIDEO,                  "default_video_recorder" },
    { BG_PLUGIN_ENCODER_AUDIO,                   "default_audio_encoder"  },
    { BG_PLUGIN_ENCODER_VIDEO|BG_PLUGIN_ENCODER, "default_video_encoder" },
    { BG_PLUGIN_ENCODER_SUBTITLE_TEXT,           "default_subtitle_text_encoder" },
    { BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY,        "default_subtitle_overlay_encoder" },
    { BG_PLUGIN_IMAGE_WRITER,                    "default_image_writer"   },
    { BG_PLUGIN_ENCODER_PP,                      "default_encoder_pp"  },
    { BG_PLUGIN_NONE,                            (char*)NULL              },
  };

static const char * get_default_key(bg_plugin_type_t type)
  {
  int i = 0;
  while(default_keys[i].key)
    {
    if(type == default_keys[i].type)
      return default_keys[i].key;
    i++;
    }
  return (const char*)0;
  }

void bg_plugin_registry_set_default(bg_plugin_registry_t * r,
                                    bg_plugin_type_t type,
                                    const char * name)
  {
  const char * key;

  key = get_default_key(type);
  if(key)
    bg_cfg_section_set_parameter_string(r->config_section, key, name);
  }

const bg_plugin_info_t * bg_plugin_registry_get_default(bg_plugin_registry_t * r,
                                                        bg_plugin_type_t type)
  {
  const char * key;
  const char * name = (const char*)0;
  const bg_plugin_info_t * ret;
  
  key = get_default_key(type);
  if(key)  
    bg_cfg_section_get_parameter_string(r->config_section, key, &name);

  if(!name)
    {
    return find_by_priority(r->entries,
                            type, BG_PLUGIN_ALL);
    }
  else
    {
    ret = bg_plugin_find_by_name(r, name);
    if(!ret)
      ret = find_by_priority(r->entries,
                            type, BG_PLUGIN_ALL);
    //    fprintf(stderr, "bg_plugin_find_by_name %s\n", name);
    return ret;
    }
  }



void bg_plugin_ref(bg_plugin_handle_t * h)
  {
  bg_plugin_lock(h);
  h->refcount++;

  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "bg_plugin_ref %s: %d", h->info->name, h->refcount);
  
#if 0
  fprintf(stderr, "bg_plugin_ref %p %s %d\n", h, h->info->name, h->refcount);
#endif

  bg_plugin_unlock(h);
  
  }

void bg_plugin_unref(bg_plugin_handle_t * h)
  {
  int refcount;
  bg_cfg_section_t * section;
  bg_plugin_lock(h);
  
  h->refcount--;
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "bg_plugin_unref %s: %d", h->info->name, h->refcount);

#if 0
  fprintf(stderr, "bg_plugin_unref %p %s %d\n", h, h->info->name, h->refcount);
#endif
  refcount = h->refcount;
  bg_plugin_unlock(h);

  if(!refcount)
    {
    if(h->plugin->get_parameter)
      {
      bg_plugin_lock(h);
      section = bg_plugin_registry_get_section(h->plugin_reg, h->info->name);
      bg_cfg_section_get(section,
                         h->plugin->get_parameters(h->priv),
                         h->plugin->get_parameter,
                         h->priv);
      bg_plugin_unlock(h);
      }
    if(h->priv && h->plugin->destroy)
      h->plugin->destroy(h->priv);
    //    if(h->dll_handle)
    //      dlclose(h->dll_handle);
    free(h);
    }
  
  }

gavl_video_frame_t *
bg_plugin_registry_load_image(bg_plugin_registry_t * r,
                              const char * filename,
                              gavl_video_format_t * format)
  {
  const bg_plugin_info_t * info;
  
  bg_image_reader_plugin_t * ir;
  bg_plugin_handle_t * handle = (bg_plugin_handle_t *)0;
  gavl_video_frame_t * ret = (gavl_video_frame_t*)0;
  
  info = bg_plugin_find_by_filename(r, filename, BG_PLUGIN_IMAGE_READER);

  if(!info)
    {
    fprintf(stderr, "No plugin found for image %s\n", filename);
    goto fail;
    }
  
  handle = bg_plugin_load(r, info);
  if(!handle)
    goto fail;
  
  ir = (bg_image_reader_plugin_t*)(handle->plugin);

  if(!ir->read_header(handle->priv, filename, format))
    goto fail;
  
  ret = gavl_video_frame_create(format);
  if(!ir->read_image(handle->priv, ret))
    goto fail;
  
  bg_plugin_unref(handle);
  return ret;

  fail:
  if(ret)
    gavl_video_frame_destroy(ret);
  return (gavl_video_frame_t*)0;
  }

void
bg_plugin_registry_save_image(bg_plugin_registry_t * r,
                              const char * filename,
                              gavl_video_frame_t * frame,
                              const gavl_video_format_t * format)
  {
  const bg_plugin_info_t * info;
  gavl_video_format_t tmp_format;
  gavl_video_converter_t * cnv;
  bg_image_writer_plugin_t * iw;
  bg_plugin_handle_t * handle = (bg_plugin_handle_t *)0;
  gavl_video_frame_t * tmp_frame = (gavl_video_frame_t*)0;
  
  info = bg_plugin_find_by_filename(r, filename, BG_PLUGIN_IMAGE_WRITER);

  cnv = gavl_video_converter_create();
  
  if(!info)
    {
    fprintf(stderr, "No plugin found for image %s\n", filename);
    goto fail;
    }
  
  handle = bg_plugin_load(r, info);
  if(!handle)
    goto fail;
  
  iw = (bg_image_writer_plugin_t*)(handle->plugin);

  gavl_video_format_copy(&tmp_format, format);
  
  if(!iw->write_header(handle->priv, filename, &tmp_format))
    goto fail;

  if(gavl_video_converter_init(cnv, format, &tmp_format))
    {
    tmp_frame = gavl_video_frame_create(&tmp_format);
    gavl_video_convert(cnv, frame, tmp_frame);
    if(!iw->write_image(handle->priv, tmp_frame))
      goto fail;
    }
  else
    {
    if(!iw->write_image(handle->priv, frame))
      goto fail;
    }
  bg_plugin_unref(handle);
  fail:
  if(tmp_frame)
    gavl_video_frame_destroy(tmp_frame);
  gavl_video_converter_destroy(cnv);
  }


bg_plugin_handle_t * bg_plugin_load(bg_plugin_registry_t * reg,
                                    const bg_plugin_info_t * info)
  {
  bg_plugin_handle_t * ret;
  bg_parameter_info_t * parameters;
  bg_cfg_section_t * section;

  if(!info)
    return (bg_plugin_handle_t*)0;
  
  ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = reg;
  
  pthread_mutex_init(&(ret->mutex),(pthread_mutexattr_t *)0);

  if(info->module_filename)
    {
    /* We need all symbols global because some plugins might reference them */
    ret->dll_handle = dlopen(info->module_filename, RTLD_NOW | RTLD_GLOBAL);
    if(!ret->dll_handle)
      {
      fprintf(stderr, "Cannot dlopen plugin %s: %s\n", info->module_filename,
              dlerror());
      goto fail;
      }
    if(!check_plugin_version(ret->dll_handle))
      {
      fprintf(stderr, "Plugin %s has no or wrong version\n",
              info->module_filename);
      goto fail;
      }


    
    ret->plugin = dlsym(ret->dll_handle, "the_plugin");
    if(!ret->plugin)
      goto fail;
    
    ret->priv = ret->plugin->create();
    }
  else if(reg->singlepic_input &&
          !strcmp(reg->singlepic_input->name, info->name))
    {
    ret->plugin = bg_singlepic_input_get();
    ret->priv = bg_singlepic_input_create(reg);
    }
  else if(reg->singlepic_stills_input &&
          !strcmp(reg->singlepic_stills_input->name, info->name))
    {
    ret->plugin = bg_singlepic_stills_input_get();
    ret->priv = bg_singlepic_stills_input_create(reg);
    }
  else if(reg->singlepic_encoder &&
          !strcmp(reg->singlepic_encoder->name, info->name))
    {
    ret->plugin = bg_singlepic_encoder_get();
    ret->priv = bg_singlepic_encoder_create(reg);
    }
  ret->info = info;

  /* Apply saved parameters */

  if(ret->plugin->get_parameters)
    {
    parameters = ret->plugin->get_parameters(ret->priv);
    
    section = bg_plugin_registry_get_section(reg, info->name);
    
    bg_cfg_section_apply(section, parameters, ret->plugin->set_parameter,
                         ret->priv);
    }
  bg_plugin_ref(ret);
  return ret;

fail:
  pthread_mutex_destroy(&(ret->mutex));
  if(ret->dll_handle)
    dlclose(ret->dll_handle);
  free(ret);
  return (bg_plugin_handle_t*)0;
  }

void bg_plugin_lock(bg_plugin_handle_t * h)
  {
  pthread_mutex_lock(&(h->mutex));
  }

void bg_plugin_unlock(bg_plugin_handle_t * h)
  {
  pthread_mutex_unlock(&(h->mutex));
  }

void bg_plugin_registry_add_device(bg_plugin_registry_t * reg,
                                   const char * plugin_name,
                                   const char * device,
                                   const char * name)
  {
  bg_plugin_info_t * info;

  info = find_by_name(reg->entries, plugin_name);
  if(!info)
    return;

  info->devices = bg_device_info_append(info->devices,
                                        device, name);

  //  fprintf(stderr, "bg_plugin_registry_save...");
  bg_plugin_registry_save(reg->entries);
  //  fprintf(stderr, "done\n");
  }

void bg_plugin_registry_set_device_name(bg_plugin_registry_t * reg,
                                        const char * plugin_name,
                                        const char * device,
                                        const char * name)
  {
  int i;
  bg_plugin_info_t * info;

  info = find_by_name(reg->entries, plugin_name);
  if(!info || !info->devices)
    return;
  
  i = 0;
  while(info->devices[i].device)
    {
    if(!strcmp(info->devices[i].device, device))
      {
      info->devices[i].name = bg_strdup(info->devices[i].name, name);
      bg_plugin_registry_save(reg->entries);
      return;
      }
    i++;
    }
  
  }

static int my_strcmp(const char * str1, const char * str2)
  {
  if(!str1 && !str2)
    return 0;
  else if(str1 && str2)
    return strcmp(str1, str2); 
  return 1;
  }

void bg_plugin_registry_remove_device(bg_plugin_registry_t * reg,
                                      const char * plugin_name,
                                      const char * device,
                                      const char * name)
  {
  bg_plugin_info_t * info;
  int index;
  int num_devices;
  info = find_by_name(reg->entries, plugin_name);
  if(!info)
    return;
    
  index = -1;
  num_devices = 0;
  while(info->devices[num_devices].device)
    {
    if(!my_strcmp(info->devices[num_devices].name, name) &&
       !strcmp(info->devices[num_devices].device, device))
      {
      index = num_devices;
      }
    num_devices++;
    }

  //  fprintf(stderr, "bg_plugin_registry_remove_device %s %s %d %d %d\n",
  //          device, name, index, num_devices, sizeof(*(info->devices)));

  if(index != -1)
    memmove(&(info->devices[index]), &(info->devices[index+1]),
            sizeof(*(info->devices)) * (num_devices - index));
    
  bg_plugin_registry_save(reg->entries);
  }

void bg_plugin_registry_find_devices(bg_plugin_registry_t * reg,
                                     const char * plugin_name)
  {
  bg_plugin_info_t * info;
  bg_plugin_handle_t * handle;
  
  info = find_by_name(reg->entries, plugin_name);
  if(!info)
    return;

  handle = bg_plugin_load(reg, info);
    
  bg_device_info_destroy(info->devices);
  info->devices = (bg_device_info_t*)0;
  
  if(!handle || !handle->plugin->find_devices)
    return;

  info->devices = handle->plugin->find_devices();
  bg_plugin_registry_save(reg->entries);
  }

char ** bg_plugin_registry_get_plugins(bg_plugin_registry_t*reg,
                                       uint32_t type_mask,
                                       uint32_t flag_mask)
  {
  int num_plugins, i;
  char ** ret;
  const bg_plugin_info_t * info;
  
  num_plugins = bg_plugin_registry_get_num_plugins(reg, type_mask, flag_mask);
  ret = calloc(num_plugins + 1, sizeof(char*));
  for(i = 0; i < num_plugins; i++)
    {
    info = bg_plugin_find_by_index(reg, i, type_mask, flag_mask);
    ret[i] = bg_strdup(NULL, info->long_name);
    }
  return ret;
  
  }

void bg_plugin_registry_free_plugins(char ** plugins)
  {
  int index = 0;
  if(!plugins)
    return;
  while(plugins[index])
    {
    free(plugins[index]);
    index++;
    }
  free(plugins);
  
  }

static void load_plugin(bg_plugin_registry_t * reg,
                        const bg_plugin_info_t * info,
                        bg_plugin_handle_t ** ret)
  {
  if(!(*ret) || strcmp((*ret)->info->name, info->name))
    {
    if(*ret)
      {
      bg_plugin_unref(*ret);
      *ret = (bg_plugin_handle_t*)0;
      }
    *ret = bg_plugin_load(reg, info);
    }
  }

int bg_input_plugin_load(bg_plugin_registry_t * reg,
                         const char * location,
                         const bg_plugin_info_t * info,
                         bg_plugin_handle_t ** ret,
                         char ** error_msg,
                         bg_input_callbacks_t * callbacks)
  {
  const char * real_location;
  char * protocol = (char*)0, * path = (char*)0;
  
  const char * msg;
  int num_plugins, i;
  uint32_t flags;
  bg_input_plugin_t * plugin;
  int try_and_error = 1;
  const bg_plugin_info_t * first_plugin = (const bg_plugin_info_t*)0;
  
  
  if(!location)
    return 0;
  
  real_location = location;
  
  if(!info) /* No plugin given, seek one */
    {
    if(bg_string_is_url(location))
      {
      if(bg_url_split(location,
                      &protocol,
                      (char **)0, // user,
                      (char **)0, // password,
                      (char **)0, // hostname,
                      (int *)0,   //  port,
                      &path))
        {
        info = bg_plugin_find_by_protocol(reg, protocol);
        if(info)
          {
          if(info->flags & BG_PLUGIN_REMOVABLE)
            real_location = path;
          }
        }
      }
    else
      {
      info = bg_plugin_find_by_filename(reg, real_location,
                                        (BG_PLUGIN_INPUT));
      }
    first_plugin = info;
    }
  else
    try_and_error = 0; /* We never try other plugins than the given one */
  
  if(info)
    {
    /* Try to load this */

    load_plugin(reg, info, ret);

    if(!(*ret))
      {
      if(error_msg)
        *error_msg = bg_sprintf("Loading plugin failed");
      return 0;
      }
    
    plugin = (bg_input_plugin_t*)((*ret)->plugin);

    if(plugin->set_callbacks)
      plugin->set_callbacks((*ret)->priv, callbacks);
    
    if(!plugin->open((*ret)->priv, real_location))
      {
      //      fprintf(stderr, "Opening %s with %s failed\n", location,
      //              info->long_name);

      if(error_msg)
        {
        msg = (const char *)0;
        if((*ret)->plugin->get_error)
          msg = (*ret)->plugin->get_error((*ret)->priv);
        if(msg)
          *error_msg = bg_sprintf(msg);
        else
          *error_msg = bg_sprintf("Unknown error");

        //        fprintf(stderr, "Error message: %s\n", *error_msg);
        
        return 0;
        }
      }
    else
      return 1;
    }

  if(protocol) free(protocol);
  if(path)     free(path);
  
  if(!try_and_error)
    return 0;

  
  flags = bg_string_is_url(location) ? BG_PLUGIN_URL : BG_PLUGIN_FILE;
  
  num_plugins = bg_plugin_registry_get_num_plugins(reg,
                                                   BG_PLUGIN_INPUT, flags);
  for(i = 0; i < num_plugins; i++)
    {
    info = bg_plugin_find_by_index(reg, i, BG_PLUGIN_INPUT, flags);

    if(info == first_plugin)
      continue;
        
    load_plugin(reg, info, ret);

    if(!*ret)
      continue;
    
    plugin = (bg_input_plugin_t*)((*ret)->plugin);
    //    fprintf(stderr, "Trying to load %s with %s...", location,
    //            info->long_name);
    if(!plugin->open((*ret)->priv, location))
      {
      //        plugin->close(album->com->load_handle->priv);
      //      fprintf(stderr, "Failed\n");

      if(*error_msg)
        free(*error_msg);

      msg = (const char *)0;
      if((*ret)->plugin->get_error)
        msg = (*ret)->plugin->get_error((*ret)->priv);
      if(msg)
        *error_msg = bg_sprintf(msg);
      else
        *error_msg = bg_sprintf("Unknown error");

      }
    else
      {
      //      fprintf(stderr, "Success\n");
      return 1;
      }
    }
  //  fprintf(stderr, "Cannot load %s: giving up\n", location);
  return 0;
  }

void bg_plugin_registry_set_encode_audio_to_video(bg_plugin_registry_t * reg,
                                                  int audio_to_video)
  {
  reg->encode_audio_to_video = audio_to_video;
  bg_cfg_section_set_parameter_int(reg->config_section, "encode_audio_to_video", audio_to_video);
  }

int bg_plugin_registry_get_encode_audio_to_video(bg_plugin_registry_t * reg)
  {
  return reg->encode_audio_to_video;
  }


void bg_plugin_registry_set_encode_subtitle_text_to_video(bg_plugin_registry_t * reg,
                                                  int subtitle_text_to_video)
  {
  reg->encode_subtitle_text_to_video = subtitle_text_to_video;
  bg_cfg_section_set_parameter_int(reg->config_section, "encode_subtitle_text_to_video",
                                   subtitle_text_to_video);
  }

int bg_plugin_registry_get_encode_subtitle_text_to_video(bg_plugin_registry_t * reg)
  {
  return reg->encode_subtitle_text_to_video;
  }

void bg_plugin_registry_set_encode_subtitle_overlay_to_video(bg_plugin_registry_t * reg,
                                                  int subtitle_overlay_to_video)
  {
  reg->encode_subtitle_overlay_to_video = subtitle_overlay_to_video;
  bg_cfg_section_set_parameter_int(reg->config_section, "encode_subtitle_overlay_to_video",
                                   subtitle_overlay_to_video);
  }

int bg_plugin_registry_get_encode_subtitle_overlay_to_video(bg_plugin_registry_t * reg)
  {
  return reg->encode_subtitle_overlay_to_video;
  }



void bg_plugin_registry_set_encode_pp(bg_plugin_registry_t * reg,
                                      int use_pp)
  {
  reg->encode_pp = use_pp;
  bg_cfg_section_set_parameter_int(reg->config_section, "encode_pp", use_pp);
  }

int bg_plugin_registry_get_encode_pp(bg_plugin_registry_t * reg)
  {
  return reg->encode_pp;
  }


int bg_plugin_equal(bg_plugin_handle_t * h1,
                     bg_plugin_handle_t * h2)
  {
  return h1 == h2;
  }
