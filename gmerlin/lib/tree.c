/*****************************************************************
 
  tree.c
 
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <tree.h>
#include <http.h>
#include <treeprivate.h>
#include <utils.h>

static int nth_in_list(bg_album_t * children, bg_album_t * child)
  {
  int ret;
  bg_album_t * tmp_album;

  ret = 0;
  tmp_album = children;

  while(tmp_album)
    {
    if(tmp_album == child)
      return ret;
    ret++;
    tmp_album = tmp_album->next;
    } 
  return -1;
  }

int * bg_media_tree_get_path(bg_media_tree_t * tree, bg_album_t * album)
  {
  bg_album_t * parent;
  bg_album_t * child;
  int depth;
  int * ret;
  int i;
  
  /* Get the depth of the album */

  depth = 1;
  parent = album->parent;

  while(parent)
    {
    depth++;
    parent = parent->parent;
    }

  /* Fill indices */
  
  ret = malloc((depth+1)* sizeof(int));
  
  ret[depth] = -1;

  child = album;
  parent = child->parent;
  
  for(i = depth-1; i >= 1; i--)
    {
    ret[i] = nth_in_list(parent->children, child);
    parent = parent->parent;
    child = child->parent;
    }
  ret[0] = nth_in_list(tree->children, child);
  return ret;
  }

static bg_album_t * append_album(bg_album_t * list, bg_album_t * new_album)
  {
  bg_album_t * a;
  if(!list)
    return new_album;
  a = list;
  
  while(a->next)
    a = a->next;
  a->next = new_album;
  return list;
  }

static bg_album_t *
remove_from_list(bg_album_t * list, bg_album_t * album)
  {
  bg_album_t * sibling_before;

  if(album == list)
    return album->next;
  else
    {
    sibling_before = list;
    while(sibling_before->next != album)
      sibling_before = sibling_before->next;
    sibling_before->next = album->next;
    return list;
    }
  }

/* Insert new album into list */

static bg_album_t * insert_album_after(bg_album_t * list,
                                       bg_album_t * new_album,
                                       bg_album_t * sibling)
  {
  if(!list)
    {
    new_album->next = (bg_album_t*)0;
    return new_album;
    }
  if(!sibling)
    {
    new_album->next = list;
    return new_album;
    }
  
  new_album->next = sibling->next;
  sibling->next = new_album;
  return list;
  }

static bg_album_t * insert_album_before(bg_album_t * list,
                                        bg_album_t * new_album,
                                        bg_album_t * sibling)
  {
  bg_album_t * before;
  
  if(!list)
    {
    new_album->next = (bg_album_t*)0;
    return new_album;
    }
  
  before = list;

  if(list == sibling)
    {
    new_album->next = list;
    return new_album;
    }
  
  while(before->next != sibling)
    before = before->next;

  return insert_album_after(list, new_album, before);
  
  }

/* Functions used both by tree and album */

static int get_num_albums(bg_album_t * list)
  {
  int ret = 0;

  if(list)
    {
    ret++;

    while(list->next)
      {
      list = list->next;
      ret++;
      }
    }
  return ret;
  }

static bg_album_t * get_album(bg_album_t * list, int index)
  {
  int i;
  for(i = 0; i < index; i++)
    list = list->next;
  return list;
  }

/* For album */

int bg_album_get_num_children(bg_album_t * a)
  {
  return get_num_albums(a->children);
  }

bg_album_t * bg_album_get_child(bg_album_t * a, int i)
  {
  return get_album(a->children, i);
  }

/* For tree */

int bg_media_tree_get_num_albums(bg_media_tree_t * t)
  {
  return get_num_albums(t->children);
  }

bg_album_t * bg_media_tree_get_album(bg_media_tree_t * t, int i)
  {
  return get_album(t->children, i);
  }

static char * copy_device(const char * start, const char * end)
  {
  int i;
  
  char * ret = malloc(end - start + 1);
  
  for(i = 0; i < end - start; i++)
    {
    ret[i] = start[i];
    }
  ret[i] = '\0';
  return ret;
  }

bg_media_tree_t * bg_media_tree_create(const char * filename,
                                       bg_plugin_registry_t * plugin_reg)
  {
  const char * device_string = NULL;
  bg_media_tree_t * ret;
  const bg_plugin_info_t * info;
  int num_removable, i;
  bg_cfg_section_t * plugin_section;
  const char * pos1;
  const char * pos2;
  int num_devices;
  
  bg_album_t * plugin_album;
  bg_album_t * device_album;
  
  ret = calloc(1, sizeof(*ret));
  
  /* Create plugin registry */

  ret->plugin_reg = plugin_reg;

  //  fprintf(stderr, "ret->plugin_reg: %p\n", ret->plugin_reg);
  //  fprintf(stderr, "ret: %p\n", ret);

  
  ret->filename = bg_strdup(ret->filename, filename);
  pos1 = strrchr(ret->filename, '/');
  
  ret->directory = bg_strndup(ret->directory, ret->filename, pos1);
  
  /* Load the entire tree */
  
  bg_media_tree_load(ret);
  
  /* Check for removable devices */

  num_removable =
    bg_plugin_registry_get_num_plugins(ret->plugin_reg,
                                       BG_PLUGIN_INPUT, BG_PLUGIN_REMOVABLE);

  for(i = 0; i < num_removable; i++)
    {
    info = bg_plugin_find_by_index(ret->plugin_reg, i,
                                   BG_PLUGIN_INPUT, BG_PLUGIN_REMOVABLE);
    
    /* Check, if this plugin has an item "device" */
        
    plugin_section = bg_plugin_registry_get_section(ret->plugin_reg,
                                                    info->name);
    
    bg_cfg_section_get_parameter_string(plugin_section,
                                        "device", &device_string);
    if(!device_string || (*device_string == '\0'))
      continue;

    //    fprintf(stderr, "Found %s, devices: %s\n",
    //            info->long_name, device_string);
    
    /* Add album for the plugin */

    plugin_album = bg_album_create(ret, NULL);
    plugin_album->name  = bg_strdup(plugin_album->name, info->long_name);
    plugin_album->flags = BG_ALBUM_REMOVABLE;

    ret->children = append_album(ret->children, plugin_album);

    /* Now, get the number of devices */

    num_devices = 1;
    pos1 = device_string;

    do{
      pos2 = strchr(pos1, ':');
      if(!pos2)
        pos2 = &(pos1[strlen(pos1)]);

      device_album = bg_album_create(ret, plugin_album);
      
      device_album->location = copy_device(pos1, pos2);

      //      fprintf(stderr, "Location: %s\n", device_album->location);

      device_album->plugin_name = bg_strdup(device_album->plugin_name,
                                            info->long_name);

      device_album->name = bg_sprintf("%s @ %s", device_album->plugin_name,
                                      device_album->location);
            
      //      fprintf(stderr, "Creating album for %s\n", device_album->name);
      device_album->flags = BG_ALBUM_REMOVABLE;
      plugin_album->children = append_album(plugin_album->children,
                                            device_album);
      pos1 = pos2;
      pos1++;
      } while(*pos2 != '\0');
    
    }  
  return ret;
  }

void bg_media_tree_destroy(bg_media_tree_t * t)
  {
  bg_album_t * next_album;
  bg_media_tree_save(t);

  while(t->children)
    {
    next_album = t->children->next;
        
    bg_album_destroy(t->children);
    t->children = next_album;
    }
  if(t->directory)
    free(t->directory);
  if(t->load_handle)
    bg_plugin_unref(t->load_handle);
  free(t);
  }

static void load_plugin(bg_media_tree_t * tree,
                        const bg_plugin_info_t * info)
  {
  if(!tree->load_handle || strcmp(tree->load_handle->info->name, info->name))
    {
    if(tree->load_handle)
      bg_plugin_unref(tree->load_handle);
    tree->load_handle = bg_plugin_load(tree->plugin_reg, info);
    //    tree->plugin = (bg_input_plugin_t*)(tree->load_handle->plugin);
    }
  }

static int load_plugin_by_filename(bg_media_tree_t * tree,
                                   const char * filename)
  {
  const bg_plugin_info_t * info;
  info = bg_plugin_find_by_filename(tree->plugin_reg, filename,
                                    (BG_PLUGIN_INPUT | BG_PLUGIN_REDIRECTOR));
  if(!info)
    return 0;
  load_plugin(tree, info);
  return 1;
  }

static int load_plugin_by_mimetype(bg_media_tree_t * tree,
                                    const char * mimetype)
  {
  const bg_plugin_info_t * info;
  info = bg_plugin_find_by_mimetype(tree->plugin_reg, mimetype);
  if(!info)
    return 0;
  load_plugin(tree, info);
  return 1;
  }

static int load_plugin_by_name(bg_media_tree_t * tree,
                               const char * name)
  {
  const bg_plugin_info_t * info;
  info = bg_plugin_find_by_name(tree->plugin_reg, name);
  if(!info)
    return 0;
  load_plugin(tree, info);
  return 1;
  }

static int load_plugin_by_long_name(bg_media_tree_t * tree,
                               const char * name)
  {
  const bg_plugin_info_t * info;
  info = bg_plugin_find_by_long_name(tree->plugin_reg, name);
  if(!info)
    return 0;
  load_plugin(tree, info);
  return 1;
  }

static void update_entry(bg_media_tree_t * tree,
                         bg_album_entry_t * entry,
                         bg_track_info_t  * track_info)
  {
  char * start_pos;
  char * end_pos;
  int name_set = 0;
  fprintf(stderr, "Update entry!");
  entry->num_audio_streams = track_info->num_audio_streams;
  entry->num_video_streams = track_info->num_video_streams;

  entry->num_subpicture_streams = track_info->num_subpicture_streams;
  entry->num_programs           = track_info->num_programs;
  
  /* Track info has a name */

  if(tree->use_metadata && tree->metadata_format)
    {
    entry->name = bg_strdup(entry->name,
                            bg_create_track_name(track_info,
                                                 tree->metadata_format));
    if(entry->name)
      name_set = 1;
    }

  if(!name_set)
    {
    if(track_info->name)
      {
      entry->name = bg_strdup(entry->name, track_info->name);
      }
    /* Take filename minus extension */
    else
      {
      start_pos = strrchr(entry->location, '/');
      if(start_pos)
        start_pos++;
      else
        start_pos = entry->location;
      end_pos = strrchr(start_pos, '.');
      if(!end_pos)
        end_pos = &(start_pos[strlen(start_pos)]);
      entry->name = bg_strndup(entry->name, start_pos, end_pos);
      }
    }
  entry->duration = track_info->duration;
  entry->flags &= ~BG_ALBUM_ENTRY_ERROR;
  }

static int refresh_entry(bg_media_tree_t * tree,
                         bg_album_entry_t * entry)
  {
  char * system_location;

  bg_input_plugin_t * plugin;
  bg_track_info_t * track_info;
    
  /* Open the track */
  
  system_location = bg_utf8_to_system(entry->location,
                                      strlen(entry->location));

  plugin = (bg_input_plugin_t*)(tree->load_handle->plugin);
  
  if(!plugin->open(tree->load_handle->priv, system_location))
    {
    fprintf(stderr, "Opening %s with %s failed\n", system_location,
            tree->load_handle->info->name);
    //    bg_hexdump(system_location, strlen(system_location));
        
    free(system_location);
    entry->flags |= BG_ALBUM_ENTRY_ERROR;
    return 0;
    }

  free(system_location);
  fprintf(stderr, "Refresh entry: %d\n", entry->index);
  track_info = plugin->get_track_info(tree->load_handle->priv,
                                      entry->index);

  update_entry(tree, entry, track_info);
  plugin->close(tree->load_handle->priv);
  return 1;
  }

int bg_media_tree_refresh_entry(bg_media_tree_t * tree,
                                bg_album_entry_t * entry)
  {
  int result = 0;
  
  //  fprintf(stderr, "Opening %s %p\n", (char*)(entry->location), tree);
  
  /* Check, which plugin to use */

  if(entry->plugin)
    result = load_plugin_by_name(tree, entry->plugin);
  else if(entry->location)
    result = load_plugin_by_filename(tree,
                                     (char*)(entry->location));

  if(!result)
    {
    fprintf(stderr, "No plugin found for %s\n",
            (char*)(entry->location));
    entry->flags |= BG_ALBUM_ENTRY_ERROR;
    return 0;
    }
  return refresh_entry(tree, entry);
  }

bg_album_entry_t * bg_media_tree_load_url(bg_media_tree_t * tree,
                                          char * url,
                                          const char * plugin_long_name)
  {
  int i, num_entries;
  char * tmp_file_name;
  const char * ptr;
  
  bg_redirector_plugin_t * redir;

  bg_album_entry_t * new_entry;
  bg_album_entry_t * end_entry = (bg_album_entry_t*)0;
  bg_album_entry_t * ret       = (bg_album_entry_t*)0;
    
  bg_http_connection_t * con = (bg_http_connection_t*)0;

  char * system_location;

  bg_input_plugin_t * plugin;
  bg_track_info_t * track_info;

  
  /* Load the appropriate plugin */

  /* 1st case: We know the plugin in advance */
  if(plugin_long_name)
    {
    if(!load_plugin_by_long_name(tree, plugin_long_name))
      return (bg_album_entry_t*)0;
    }
  /* 2st case: URL is a http url so we can fetch the mimetype */
  else if(!strncmp(url, "http://", 7))
    {
    con = bg_http_connection_create();
    
    if(!bg_http_connection_connect(con, url, NULL))
      {
      fprintf(stderr, "Could not connect to %s\n", url);
      bg_http_connection_destroy(con);
      return (bg_album_entry_t*)0;
      }

    ptr = bg_http_connection_get_mimetype(con);
    
    if(!load_plugin_by_mimetype(tree, ptr))
      {
      fprintf(stderr, "No plugin found for mimetype %s\n",
              ptr);
      bg_http_connection_close(con);
      bg_http_connection_destroy(con);
      return (bg_album_entry_t*)0;
      }
    }
  /* 3rd case: Track has a filename so we can look for the extension */
  else
    {
    if(!load_plugin_by_filename(tree, url))
      {
      fprintf(stderr, "No plugin found for %s\n", url);
      return (bg_album_entry_t*)0;
      }
    }

  if(tree->load_handle->info->type == BG_PLUGIN_REDIRECTOR)
    {
    /* Open redirector */
    redir = (bg_redirector_plugin_t*)(tree->load_handle->plugin);

    fprintf(stderr, "Using redirector plugin %s\n",
            tree->load_handle->info->name);
    
    if(con) /* Get redirector via http */
      {
      tmp_file_name = bg_create_unique_filename("/tmp/gmerlin_tmp%08x");
      
      bg_http_connection_save(con, tmp_file_name);
      bg_http_connection_close(con);
      bg_http_connection_destroy(con);

      redir->parse(tree->load_handle->priv, tmp_file_name);
      remove(tmp_file_name);
      free(tmp_file_name);
      }
    else
      redir->parse(tree->load_handle->priv, url);
    
    num_entries = redir->get_num_streams(tree->load_handle->priv);
      
    for(i = 0; i < num_entries; i++)
      {
      new_entry = calloc(1, sizeof(*new_entry));
      
      ptr = redir->get_stream_url(tree->load_handle->priv, i);
      new_entry->location = bg_strdup((char*)0, ptr);
      
      ptr = redir->get_stream_name(tree->load_handle->priv, i);
      
      if(ptr)
        new_entry->name = bg_strdup((char*)0, ptr);
      else
        new_entry->name = bg_strdup((char*)0, new_entry->location);
      
      ptr = redir->get_stream_plugin(tree->load_handle->priv, i);
      if(ptr)
        new_entry->plugin = bg_strdup((char*)0, ptr);
      
      if(ret)
        {
        end_entry->next = new_entry;
        end_entry = end_entry->next;
        }
      else
        {
        ret = new_entry;
        end_entry = ret;
        }
      }
    }
  /* Use track info from the plugin */
  else
    {
    fprintf(stderr, "Using input plugin %s\n",
            tree->load_handle->info->name);
    
    if(con)
      {
      bg_http_connection_close(con);
      bg_http_connection_destroy(con);
      }

    /* Open the track */
  
    plugin = (bg_input_plugin_t*)(tree->load_handle->plugin);
        
    if(!plugin->open(tree->load_handle->priv, url))
      {
      fprintf(stderr, "Opening %s with %s failed\n", system_location,
              tree->load_handle->info->name);
      //    bg_hexdump(system_location, strlen(system_location));
      bg_album_entry_destroy(ret);
      free(system_location);
      return (bg_album_entry_t*)0;
      }

    if(!plugin->get_num_tracks)
      num_entries = 1;
    else
      num_entries = plugin->get_num_tracks(tree->load_handle->priv);
    
    for(i = 0; i < num_entries; i++)
      {
      new_entry = bg_album_entry_create(tree);
      new_entry->location = bg_system_to_utf8(url, strlen(url));
      new_entry->index = i;
      new_entry->total_tracks = num_entries;
      
      track_info = plugin->get_track_info(tree->load_handle->priv,
                                          i);
      update_entry(tree, new_entry, track_info);

      if(ret)
        {
        end_entry->next = new_entry;
        end_entry = end_entry->next;
        }
      else
        {
        ret = new_entry;
        end_entry = ret;
        }
      }
    plugin->close(tree->load_handle->priv);
    }
  
  return ret;
  }

char * bg_media_tree_new_album_filename(bg_media_tree_t * tree)
  {
  /*
   *  Album filenames are constructed like "aXXXXXXXX.xml",
   *  where XXXXXXXX is a hexadecimal unique identifier
   */
  char * template = (char*)0;
  char * path = (char*)0;
  char * ret = (char*)0;
  char * pos;
  
  template = bg_sprintf("%s/a%%08x.xml", tree->directory);

  path = bg_create_unique_filename(template);

  if(!path)
    goto fail;

  pos = strrchr(path, '/');

  pos++;
  ret = bg_strdup((char*)0, pos);
  free(path);
    
  fail:
  if(template)
    free(template);
  
  return ret;
  }


bg_album_t * bg_media_tree_append_album(bg_media_tree_t * tree,
                                        bg_album_t * parent)
  {
  bg_album_t * album_before;
  bg_album_t * album_after;
  
  bg_album_t * new_album =
    bg_album_create(tree, parent);

  if(parent)
    {
    if(parent->children)
      {
      album_before = parent->children;
      while(album_before->next)
        album_before = album_before->next;
      album_before->next = new_album;
      }
    else
      parent->children = new_album;
    }

  /* Top level album, take care of the system albums */

  else
    {
    if(tree->children)
      {
      album_before = tree->children;

      /* Tree has only device albums */
            
      if(bg_album_is_removable(album_before))
        {
        new_album->next = tree->children; 
        tree->children = new_album;
        }

      else
        {
        
        while(album_before->next && !bg_album_is_removable(album_before->next))
          album_before = album_before->next;
        album_after = album_before->next;
        album_before->next = new_album;
        new_album->next = album_after;
        }
      }
    else
      tree->children = new_album;
    }
  return new_album;
  }

void bg_media_tree_remove_album(bg_media_tree_t * tree,
                                bg_album_t * album)
  {
  char * tmp_path = (char *)0;

  if(album->parent)
    album->parent->children =
      remove_from_list(album->parent->children, album);
  else
    tree->children = remove_from_list(tree->children, album);
  
  /* Check if album file if present */
  
  if(album->location)
    tmp_path = bg_sprintf("%s/%s", tree->directory, album->location);
  
  /* Schredder everything */

  bg_album_destroy(album);

  if(tmp_path)
    {
    remove(tmp_path);
    free(tmp_path);     
    }
  }

/* Check if we can move an album */

static int check_move_common(bg_media_tree_t * t,
                             bg_album_t * album,
                      bg_album_t * sibling)
  {
  bg_album_t * parent;

  if(album == sibling)
    return 0;

  parent = sibling->parent;
  while(parent)
    {
    if(album == parent)
      return 0;
    parent = parent->parent;
    }
  return 1;
  }

int bg_media_tree_check_move_album_before(bg_media_tree_t * t,
                                          bg_album_t * album,
                                          bg_album_t * after)
  {
  if(!check_move_common(t, album, after))
    {
    return 0;
    }

  /* Never drop after removable albums */

  if(bg_album_is_removable(after))
    return 0;
  return 1;
  }

int bg_media_tree_check_move_album_after(bg_media_tree_t * t,
                                         bg_album_t * album,
                                         bg_album_t * before)
  {
  if(!check_move_common(t, album, before))
    {
    return 0;
    }

  /* Never drop after removable albums */

  if(bg_album_is_removable(before))
    return 0;

  return 1;
  }

/* Move an album inside the tree */

void bg_media_tree_move_album_before(bg_media_tree_t * t,
                                     bg_album_t * album,
                                     bg_album_t * after)
  {
  if(!bg_media_tree_check_move_album_before(t, album, after))
    return;

  /* Remove the album from the parent's children list */

  if(album->parent)
    {
    album->parent->children =
      remove_from_list(album->parent->children, album);
    }
  else
    {
    t->children = remove_from_list(t->children, album);
    }

  /* Insert at new location */

  if(after->parent)
    {
    after->parent->children =
      insert_album_before(after->parent->children, album, after);
    album->parent = after->parent;
    }
  else
    {
    t->children = insert_album_before(t->children, album, after);
    album->parent = (bg_album_t*)0;
    }
  }

void bg_media_tree_move_album_after(bg_media_tree_t * t,
                                    bg_album_t * album,
                                    bg_album_t * before)
  {
  if(!bg_media_tree_check_move_album_after(t, album, before))
    return;

  /* Remove the album from the parent's children list */

  if(album->parent)
    {
    album->parent->children =
      remove_from_list(album->parent->children, album);
    }
  else
    {
    t->children = remove_from_list(t->children, album);
    }

  /* Insert at new location */

  if(before->parent)
    {
    before->parent->children =
      insert_album_after(before->parent->children, album, before);
    album->parent = before->parent;
    }
  else
    {
    t->children = insert_album_after(t->children, album, before);
    album->parent = (bg_album_t*)0;
    }

  
  }

int bg_media_tree_check_move_album(bg_media_tree_t * t,
                                   bg_album_t * album,
                                   bg_album_t * parent)
  {
  bg_album_t * test_album;
  
  /* Never move album into it's subalbum */

  test_album = parent;

  while(test_album)
    {
    if(album == test_album)
      return 0;
    test_album = test_album->parent;
    }

  /* Never drop into removable albums */

  if(bg_album_is_removable(parent))
    return 0;
  
  return 1;
  }

void bg_media_tree_move_album(bg_media_tree_t * t,
                              bg_album_t * album,
                              bg_album_t * parent)
  {
  if(!bg_media_tree_check_move_album(t, album, parent))
    return;

  /* Remove the album from the parent's children list */

  if(album->parent)
    {
    album->parent->children =
      remove_from_list(album->parent->children, album);
    }
  else
    {
    t->children = remove_from_list(t->children, album);
    }

  if(!parent)
    {
    t->children = insert_album_before(t->children, album, (bg_album_t*)0);
    album->parent = (bg_album_t*)0;
    }
  else
    {
    parent->children = insert_album_before(parent->children,
                                           album, (bg_album_t*)0);
    album->parent = parent;
    }
  }


/* Set and get entries for playback */

void bg_media_tree_set_current(bg_media_tree_t * t,
                               bg_album_t * album,
                               const bg_album_entry_t * entry)
  {
  if(t->current_album && (t->current_album != album))
    {
    t->current_album->current_entry = (bg_album_entry_t*)0;
    bg_album_changed(t->current_album);
    }
  t->current_album = album;
  if(t->current_handle)
    {
    bg_plugin_unref(t->current_handle);
    t->current_handle = (bg_plugin_handle_t*)0;
    }

  if(t->current_album)
    {
    t->current_entry = t->current_album->entries;
    
    while(t->current_entry != entry)
      t->current_entry = t->current_entry->next;
    }
  
  if(t->change_callback)
    t->change_callback(t, t->change_callback_data);
  }

/* Set the next and previous track */

int bg_media_tree_next(bg_media_tree_t * tree, int wrap)
  {
  if(tree->current_album)
    return bg_album_next(tree->current_album, wrap);
  return 0;
  }

int bg_media_tree_previous(bg_media_tree_t * tree, int wrap)
  {
  if(tree->current_album)
    return bg_album_previous(tree->current_album, wrap);
  return 0;
  }

void bg_media_tree_set_change_callback(bg_media_tree_t * tree,
                                       void (*change_callback)(bg_media_tree_t*, void*),
                                       void* change_callback_data)
  {
  tree->change_callback = change_callback;
  tree->change_callback_data = change_callback_data;
  }

void bg_media_tree_set_play_callback(bg_media_tree_t * tree,
                                     void (*play_callback)(bg_media_tree_t*, void*),
                                     void* play_callback_data)
  {
  tree->play_callback      = play_callback;
  tree->play_callback_data = play_callback_data;
  }

bg_plugin_handle_t *
bg_media_tree_get_current_track(bg_media_tree_t * t, int * index)
  {
  bg_track_info_t * track_info;
  const bg_plugin_info_t * info;
  bg_input_plugin_t * input_plugin;
  
  if(!t->current_entry || !t->current_album)
    return (bg_plugin_handle_t *)0;
  
  if(bg_album_is_removable(t->current_album))
    {
    t->current_handle = t->current_album->handle;
    bg_plugin_ref(t->current_handle);
    input_plugin = (bg_input_plugin_t*)(t->current_handle->plugin);
    }
  else
    {
    if(t->current_handle)
      bg_plugin_unref(t->current_handle);
    
    if(t->current_entry->plugin)
      info = bg_plugin_find_by_name(t->plugin_reg, t->current_entry->plugin);
    else
      info = bg_plugin_find_by_filename(t->plugin_reg,
                                        t->current_entry->location,
                                        (BG_PLUGIN_INPUT | BG_PLUGIN_REDIRECTOR));
    
    t->current_handle = bg_plugin_load(t->plugin_reg, info);
    input_plugin = (bg_input_plugin_t*)(t->current_handle->plugin);
    input_plugin->open(t->current_handle->priv, t->current_entry->location);
    }

  track_info = input_plugin->get_track_info(t->current_handle->priv,
                                            t->current_entry->index);
  update_entry(t, t->current_entry, track_info);
  bg_album_changed(t->current_album);
  
  if(index)
    *index = t->current_entry->index;
  return t->current_handle;
  }

bg_album_t * bg_media_tree_get_current_album(bg_media_tree_t * t)
  {
  return t->current_album;
  }

const char * bg_media_tree_get_current_track_name(bg_media_tree_t * t)
  {
  if(!t->current_album || !t->current_entry)
    return (const char *)0;
  return t->current_entry->name;
  }

void bg_media_tree_set_coords(bg_media_tree_t * t, int x, int y, int width, int height)
  {
  t->x = x;
  t->y = y;
  t->width = width;
  t->height = height;
  }

void bg_media_tree_get_coords(bg_media_tree_t * t, int * x, int * y,
                              int * width, int * height)
  {
  *x      = t->x;
  *y      = t->y;
  *width  = t->width;
  *height = t->height;
  }

int bg_media_tree_get_unique_id(bg_media_tree_t * tree)
  {
  tree->highest_id++;
  return tree->highest_id;
  }

/* Parameter stuff */

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "use_metadata",
      long_name:   "Use metadata for track names",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    {
      name:        "metadata_format",
      long_name:   "Format for track names",
      type:        BG_PARAMETER_STRING,
      val_default: { val_str: "%p - %t" }
    },
    { /* End of parameters */ }
  };

bg_parameter_info_t * bg_media_tree_get_parameters(bg_media_tree_t * tree)
  {
  return parameters;
  }

void bg_media_tree_set_parameter(void * priv, char * name, bg_parameter_value_t * val)
  {
  bg_media_tree_t * tree;
  tree = (bg_media_tree_t*)priv;
  if(!name)
    return;

  if(!strcmp(name, "use_metadata"))
    {
    tree->use_metadata = val->val_i;
    }
  else if(!strcmp(name, "metadata_format"))
    {
    tree->metadata_format = val->val_str;
    }
  }

void bg_media_tree_mark_error(bg_media_tree_t * t, int err)
  {
  if(t->current_entry)
    {
    if(err)
      t->current_entry->flags |= BG_ALBUM_ENTRY_ERROR;
    else
      t->current_entry->flags &= ~BG_ALBUM_ENTRY_ERROR;
    }
  if(t->current_album)
    bg_album_changed(t->current_album);
  }
