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
#include <config.h>
#include <utils.h>
#include <singlepic.h>

struct bg_plugin_registry_s
  {
  bg_plugin_info_t * entries;
  bg_cfg_section_t * config_section;

  bg_plugin_info_t * singlepic_input;
  bg_plugin_info_t * singlepic_encoder;

  int encode_audio_to_video;
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
  if(info->module_filename)
    free(info->module_filename);
  if(info->devices)
    bg_device_info_destroy(info->devices);
  free(info);
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

const bg_plugin_info_t *
bg_plugin_find_by_dll(bg_plugin_registry_t * reg, const char * filename)
  {
  return find_by_dll(reg->entries, filename);
  }

static bg_plugin_info_t *
find_by_name(bg_plugin_info_t * info, const char * name)
  {
  while(info)
    {
    if(!strcmp(info->name, name))
      return info;
    info = info->next;
    }
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

const bg_plugin_info_t * bg_plugin_find_by_long_name(bg_plugin_registry_t *
                                                     reg, const char * name)
  {
  return find_by_long_name(reg->entries, name);
  }

static int string_match(const char * key,
                        const char * key_end,
                        const char * key_list)
  {
  const char * pos;
  const char * end;

  pos = key_list;
      
  //  fprintf(stderr, "string_match: %s %d %s\n", key, (int)(key_end - key), key_list);

  if(!key_list)
    return 0;
  
  while(1)
    {
    end = pos;
    while(!isspace(*end) && (*end != '\0'))
      end++;
    if(end == pos)
      break;

    //    fprintf(stderr,
    //            "String match Key: %s, keylist: %s, ley_len: %d, key_list_len: %d\n",
    //            key, pos, (int)(key_end - key), (int)(end-pos));
    if(((int)(key_end - key) == (int)(end-pos)) &&
       !strncasecmp(pos, key, (int)(end-pos)))
      {
      //      fprintf(stderr, "BINGOOOOOO\n");
      return 1;
      }
    pos = end;
    if(pos == '\0')
      break;
    else
      {
      while(isspace(*pos) && (pos != '\0'))
        pos++;
      }
    }
  return 0;
  }

const bg_plugin_info_t * bg_plugin_find_by_filename(bg_plugin_registry_t * reg,
                                                    const char * filename,
                                                    int typemask)
  {
  bg_plugin_info_t * info;
  
  char * extension;
  char * extension_end;
  
  // fprintf(stderr, "bg_plugin_find_by_filename %p\n", reg);
  
  info = reg->entries;
  extension = strrchr(filename, '.');

  if(!extension)
    {
    return (const bg_plugin_info_t *)0;
    }
  extension++;
  extension_end = &(extension[strlen(extension)]);
  
  //  fprintf(stderr, "info %p\n", info);
  
  while(info)
    {
    //    fprintf(stderr, "Trying: %s %s\n", info->long_name, info->extensions);
    if(!(info->type & typemask) ||
       !(info->flags & BG_PLUGIN_FILE) ||
       !info->extensions)
      {
      //      fprintf(stderr, "skipping plugin: %s\n",
      //              info->long_name);
      info = info->next;
      continue;
      }
    if(string_match(extension, extension_end, info->extensions))
      return info;
    info = info->next;
    }
  return (const bg_plugin_info_t *)0;
  }

const bg_plugin_info_t *
bg_plugin_find_by_mimetype(bg_plugin_registry_t * reg,
                           const char * mimetype,
                           const char * url)
  {
  bg_plugin_info_t * info;
  const char * mimetype_end;
  
  mimetype_end = &mimetype[strlen(mimetype)];
  
  //  fprintf(stderr, "bg_plugin_find_by_mimetype %p\n", reg);
    
  //  fprintf(stderr, "info %p\n", info);
  
  info = reg->entries;
  
  while(info)
    {
    if((info->type != BG_PLUGIN_INPUT) ||
       !(info->flags & BG_PLUGIN_URL) ||
       !info->mimetypes)
      {
      //      fprintf(stderr, "skipping plugin: %s\n",
      //              info->long_name);
      info = info->next;
      continue;
      }

    if(string_match(mimetype, mimetype_end, info->mimetypes))
      {
      return info;
      }

    info = info->next;
    }
  return (const bg_plugin_info_t *)0;
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
  
  bg_cfg_section_t * plugin_section;
  bg_cfg_section_t * stream_section;
  bg_parameter_info_t * parameter_info;
  void * plugin_priv;
  
  file_info = *_file_info;

  ret = (bg_plugin_info_t *)0;

  *changed = 0;
  
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
          end->next = NULL;
          }
        continue;
        }
      }

    /* Open the DLL and see what's inside */

    test_module = dlopen(filename, RTLD_NOW);
    if(!test_module)
      {
      fprintf(stderr, "Cannot dlopen %s: %s\n", filename, dlerror());
      continue;
      }
    plugin = (bg_plugin_common_t*)(dlsym(test_module, "the_plugin"));
    if(!plugin)
      {
      fprintf(stderr, "No symbol the_plugin in %s\n", filename);
      dlclose(test_module);
      continue;
      }
    *changed = 1;
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
    new_info->type = plugin->type;
    new_info->flags = plugin->flags;

    /* Create parameter entries in the registry */

    plugin_section =
      bg_cfg_section_find_subsection(cfg_section, plugin->name);

    plugin_priv = plugin->create();
    
    if(plugin->get_parameters)
      {
          
      parameter_info = plugin->get_parameters(plugin_priv);

      bg_cfg_section_create_items(plugin_section,
                                  parameter_info);
      }
    
    if(plugin->type & (BG_PLUGIN_ENCODER_AUDIO|
                       BG_PLUGIN_ENCODER_VIDEO|
                       BG_PLUGIN_ENCODER))
      {
      encoder = (bg_encoder_plugin_t*)plugin;

      if(encoder->get_audio_parameters)
        {
        parameter_info = encoder->get_audio_parameters(plugin_priv);
        stream_section = bg_cfg_section_find_subsection(plugin_section,
                                                        "$audio");
        
        bg_cfg_section_create_items(stream_section,
                                    parameter_info);
        }

      if(encoder->get_video_parameters)
        {
        parameter_info = encoder->get_video_parameters(plugin_priv);
        stream_section = bg_cfg_section_find_subsection(plugin_section,
                                                        "$video");
        
        bg_cfg_section_create_items(stream_section,
                                    parameter_info);
        }
      
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
  const char * sort_string;
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

    while(file_info)
      {
      tmp_info = file_info->next;
      free_info(file_info);
      file_info = tmp_info;
      }
    }
  
  if(changed)
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
  
  ret->singlepic_encoder = bg_singlepic_encoder_info(ret);

  if(ret->singlepic_encoder)
    {
    //    fprintf(stderr, "Found Singlepicture encoder\n");
    tmp_info->next = ret->singlepic_encoder;
    tmp_info = tmp_info->next;
    }
  
  
  /* Lets sort them */
  
  sort_string = (const char*)0;
  
  bg_cfg_section_get_parameter_string(ret->config_section,
                                      "Order", &sort_string);
  if(sort_string)
    bg_plugin_registry_sort(ret, sort_string);

  /* Get flags */

  bg_cfg_section_get_parameter_int(ret->config_section, "encode_audio_to_video",
                                   &(ret->encode_audio_to_video));
    
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
       (test_info->flags & flag_mask))
      {
      if(i == index)
        return test_info;
      i++;
      }
    test_info = test_info->next;
    }
  return (bg_plugin_info_t*)0;
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
       (info->flags & flag_mask))
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

  bg_plugin_registry_save(reg->entries);
  
  }

void bg_plugin_registry_set_mimetypes(bg_plugin_registry_t * reg,
                                      const char * plugin_name,
                                      const char * mimetypes)
  {
  bg_plugin_info_t * info;
  info = find_by_name(reg->entries, plugin_name);
  if(!info)
    return;
  if(!(info->flags & BG_PLUGIN_URL))
    return;
  info->mimetypes = bg_strdup(info->mimetypes, mimetypes);
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
    { BG_PLUGIN_IMAGE_WRITER,                    "default_image_writer"   },
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
  
  key = get_default_key(type);
  if(key)  
    bg_cfg_section_get_parameter_string(r->config_section, key, &name);

  if(!name)
    {
    return find_by_index(r->entries,
                         0, type, BG_PLUGIN_ALL);
    }
  else
    {
    return bg_plugin_find_by_name(r, name);
    }
  }

void bg_plugin_registry_sort(bg_plugin_registry_t * r, const char * sort_string)
  {
  char ** names;
  int index;
  bg_plugin_info_t * info;
  bg_plugin_info_t * new_entries    = (bg_plugin_info_t *)0;
  bg_plugin_info_t * last_new_entry = (bg_plugin_info_t *)0;
  
  names = bg_strbreak(sort_string, ',');

  index = 0;
  while(names[index])
    {
    info = find_by_name(r->entries, names[index]);

    if(!info)
      {
      index++;
      continue;
      }

    r->entries = remove_from_list(r->entries, info);
    
    if(!new_entries)
      {
      new_entries = info;
      last_new_entry = new_entries;
      }
    else
      {
      last_new_entry->next = info;
      last_new_entry = last_new_entry->next;
      }
    index++;
    }
  last_new_entry->next = r->entries;
  r->entries = new_entries;
  bg_strbreak_free(names);
  bg_cfg_section_set_parameter_string(r->config_section, "Order", sort_string);
  }


void bg_plugin_ref(bg_plugin_handle_t * h)
  {
  bg_plugin_lock(h);
  h->refcount++;

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
    if(h->dll_handle)
      dlclose(h->dll_handle);
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
    gavl_video_frame_destroy(ret);
    return (gavl_video_frame_t*)0;
  
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
                         char ** error_msg)
  {
  const char * msg;
  int num_plugins, i;
  uint32_t flags;
  
  bg_input_plugin_t * plugin;

  int try_and_error = 1;
  if(!info) /* No plugin given, seek one */
    {
    info = bg_plugin_find_by_filename(reg, location,
                                      (BG_PLUGIN_INPUT));
    }
  else
    try_and_error = 0; /* We never try other plugins than the given one */
  
  if(info)
    {
    /* Try to load this */

    load_plugin(reg, info, ret);

    if(!(*ret))
      return 0;
    
    plugin = (bg_input_plugin_t*)((*ret)->plugin);

    if(!plugin->open((*ret)->priv, location))
      {
      fprintf(stderr, "Opening %s with %s failed\n", location,
              info->long_name);

      msg = (const char *)0;
      if((*ret)->plugin->get_error)
        msg = (*ret)->plugin->get_error((*ret)->priv);
      if(msg)
        *error_msg = bg_sprintf(msg);
      else
        *error_msg = bg_sprintf("Unknown error");
      return 0;
      }
    else
      return 1;
    }

  if(!try_and_error)
    return 0;

  flags = bg_string_is_url(location) ? BG_PLUGIN_URL : BG_PLUGIN_FILE;
  
  num_plugins = bg_plugin_registry_get_num_plugins(reg,
                                                   BG_PLUGIN_INPUT, flags);
  for(i = 0; i < num_plugins; i++)
    {
    info = bg_plugin_find_by_index(reg, i, BG_PLUGIN_INPUT, flags);

    load_plugin(reg, info, ret);
    
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
