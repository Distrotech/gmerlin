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

struct bg_plugin_registry_s
  {
  bg_plugin_info_t * entries;
  bg_cfg_section_t * config_section;
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
  free(info);
  }

static bg_plugin_info_t *
find_by_dll(bg_plugin_info_t * info, const char * filename)
  {
  while(info)
    {
    if(!strcmp(info->module_filename, filename))
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

const bg_plugin_info_t * bg_plugin_find_by_filename(bg_plugin_registry_t * reg,
                                                    const char * filename)
  {
  bg_plugin_info_t * info;
  char * pos;
  char * end;

  char * extension = (char*)0;

  //  fprintf(stderr, "bg_plugin_find_by_filename %p\n", reg);
  
  info = reg->entries;
  pos = strrchr(filename, '.');

  if(!pos)
    {
    return (const bg_plugin_info_t *)0;
    }

  pos++;
  extension = bg_strdup(extension, pos);

  /* Convert to lower case */

  pos = extension;
  while(*pos != '\0')
    {
    *pos = tolower(*pos);
    pos++;
    }

  //  fprintf(stderr, "info %p\n", info);
  
  while(info)
    {
    if(((info->type != BG_PLUGIN_INPUT) &&
        (info->type != BG_PLUGIN_REDIRECTOR)) ||
       !(info->flags & BG_PLUGIN_FILE) ||
       !info->extensions)
      {
      info = info->next;
      //      fprintf(stderr, "skipping plugin: %s\n",
      //              info->long_name);
      continue;
      }
    pos = info->extensions;
    //    fprintf(stderr, "plugin: %s, extension: %s\n",
    //            info->long_name, extension);
    while(1)
      {
      end = pos;
      while(!isspace(*end) && (*end != '\0'))
        end++;
      if(end == pos)
        break;
      if((strlen(extension) == (int)(end-pos)) &&
         !strncmp(pos, extension, (int)(end-pos)))
        {
        free(extension);
        return info;
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
    info = info->next;
    }
  free(extension);
  return (const bg_plugin_info_t *)0;
  }

const bg_plugin_info_t * bg_plugin_find_by_mimetype(bg_plugin_registry_t * reg,
                                                    const char * mimetype)
  {
  bg_plugin_info_t * info;
  char * pos;
  char * end;

  char * tmp_string = (char*)0;

  //  fprintf(stderr, "bg_plugin_find_by_filename %p\n", reg);
  
  tmp_string = bg_strdup((char*)0, mimetype);
      
  /* Convert to lower case */

  pos = tmp_string;
  while(*pos != '\0')
    {
    *pos = tolower(*pos);
    pos++;
    }
  
  //  fprintf(stderr, "info %p\n", info);


  info = reg->entries;
  
  while(info)
    {
    if(((info->type != BG_PLUGIN_INPUT) &&
        (info->type != BG_PLUGIN_REDIRECTOR)) ||
       !(info->flags & BG_PLUGIN_URL) ||
       !info->mimetypes)
      {
      fprintf(stderr, "skipping plugin: %s\n",
              info->long_name);
      info = info->next;
      continue;
      }
    pos = info->mimetypes;
    while(1)
      {
      end = pos;
      while(!isspace(*end) && (*end != '\0'))
        end++;

      if(end == pos)
        break;
      if((strlen(tmp_string) == (int)(end-pos)) &&
         !strncmp(pos, tmp_string, (int)(end-pos)))
        {
        free(tmp_string);
        return info;
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
    info = info->next;
    }
  free(tmp_string);
  return (const bg_plugin_info_t *)0;
  }

bg_plugin_info_t * remove_from_list(bg_plugin_info_t * list,
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

bg_plugin_info_t *
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

  bg_cfg_section_t * plugin_section;
  int parameter_index;
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
    if(plugin->get_parameters)
      {
      parameter_index = 0;    
      plugin_priv = plugin->create();
      parameter_info = plugin->get_parameters(plugin_priv);
      while(parameter_info[parameter_index].name)
        {
        bg_cfg_section_get_parameter(plugin_section,
                                     &(parameter_info[parameter_index]), 
                                     (bg_parameter_value_t*)0);
        parameter_index++;
        }
      plugin->destroy(plugin_priv);
      }
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

    if(new_info)
      fprintf(stderr, "Loaded plugin %s\n", new_info->name);
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
    file_info = bg_load_plugin_file(filename);
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
    {
    filename = bg_search_file_write("", "plugins.xml");
    if(filename)
      {
      bg_save_plugin_file(ret->entries, filename);
      free(filename);
      }
    }
  sort_string = (const char*)0;
  
  bg_cfg_section_get_parameter_string(ret->config_section, "Order", &sort_string);
  if(sort_string)
    bg_plugin_registry_sort(ret, sort_string);
  
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
    info = info->next;
    }
  return ret;
  }

void bg_plugin_registry_set_extensions(bg_plugin_registry_t * reg,
                                       const char * plugin_name,
                                       const char * extensions)
  {
  bg_plugin_info_t * info;
  char * filename;
  info = find_by_name(reg->entries, plugin_name);
  if(!info)
    return;
  if(!(info->flags & BG_PLUGIN_FILE))
    return;
  info->extensions = bg_strdup(info->extensions, extensions);

  filename = bg_search_file_write("", "plugins.xml");
  if(filename)
    {
    bg_save_plugin_file(reg->entries, filename);
    free(filename);
    }


  }

void bg_plugin_registry_set_mimetypes(bg_plugin_registry_t * reg,
                                      const char * plugin_name,
                                      const char * mimetypes)
  {
  bg_plugin_info_t * info;
  char * filename;
  info = find_by_name(reg->entries, plugin_name);
  if(!info)
    return;
  if(!(info->flags & BG_PLUGIN_URL))
    return;
  info->mimetypes = bg_strdup(info->mimetypes, mimetypes);

  filename = bg_search_file_write("", "plugins.xml");
  if(filename)
    {
    bg_save_plugin_file(reg->entries, filename);
    free(filename);
    }

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
    { BG_PLUGIN_OUTPUT_AUDIO,   "default_audio_output"   },
    { BG_PLUGIN_OUTPUT_VIDEO,   "default_video_output"   },
    { BG_PLUGIN_RECORDER_AUDIO, "default_audio_recorder" },
    { BG_PLUGIN_RECORDER_VIDEO, "default_video_recorder" },
    { BG_PLUGIN_NONE,           (char*)NULL              },
    
  };

const char * get_default_key(bg_plugin_type_t type)
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

#if 1
  fprintf(stderr, "bg_plugin_ref %p %s %d\n", h, h->info->name, h->refcount);
#endif

  bg_plugin_unlock(h);
  
  }

void bg_plugin_unref(bg_plugin_handle_t * h)
  {
  int refcount;

  bg_plugin_lock(h);
  h->refcount--;
#if 1
  fprintf(stderr, "bg_plugin_unref %p %s %d\n", h, h->info->name, h->refcount);
#endif
  refcount = h->refcount;
  bg_plugin_unlock(h);

  if(!refcount)
    {
    if(h->priv && h->plugin->destroy)
      h->plugin->destroy(h->priv);
    dlclose(h->dll_handle);
    free(h);
    }
  }

