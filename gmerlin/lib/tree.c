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


bg_media_tree_t * bg_media_tree_create(const char * filename,
                                       bg_plugin_registry_t * plugin_reg)
  {
  bg_media_tree_t * ret;
  const bg_plugin_info_t * info;
  int num_removable, i, j;
  const char * pos1;
  
  bg_album_t * plugin_album;
  bg_album_t * device_album;
  
  ret = calloc(1, sizeof(*ret));
  
  ret->com.plugin_reg = plugin_reg;
  ret->com.set_current_callback = bg_media_tree_set_current;
  ret->com.set_current_callback_data = ret;
  
  //  fprintf(stderr, "ret->plugin_reg: %p\n", ret->plugin_reg);
  //  fprintf(stderr, "ret: %p\n", ret);

  
  ret->filename = bg_strdup(ret->filename, filename);
  pos1 = strrchr(ret->filename, '/');
  
  ret->com.directory = bg_strndup(ret->com.directory, ret->filename, pos1);
  
  /* Load the entire tree */
  
  bg_media_tree_load(ret);
  
  /* Check for removable devices */

  num_removable =
    bg_plugin_registry_get_num_plugins(ret->com.plugin_reg,
                                       BG_PLUGIN_INPUT, BG_PLUGIN_REMOVABLE);

  for(i = 0; i < num_removable; i++)
    {
    info = bg_plugin_find_by_index(ret->com.plugin_reg, i,
                                   BG_PLUGIN_INPUT, BG_PLUGIN_REMOVABLE);
    
    /* Add album for the plugin */

    plugin_album = bg_album_create(&(ret->com), BG_ALBUM_TYPE_PLUGIN, NULL);
    plugin_album->name  = bg_strdup(plugin_album->name, info->long_name);
    plugin_album->plugin_info = info;

    
    
    ret->children = append_album(ret->children, plugin_album);

    /* Now, get the number of devices */
    
    if(info->devices && info->devices->device)
      {
      j = 0;
      
      while(info->devices[j].device)
        {
        device_album = bg_album_create(&(ret->com), BG_ALBUM_TYPE_REMOVABLE, plugin_album);
        device_album->location = bg_strdup(device_album->location,
                                           info->devices[j].device);
        if(info->devices[j].name)
          {
          device_album->name = bg_strdup(device_album->name,
                                         info->devices[j].name);
          }
        else
          {
          device_album->name = bg_strdup(device_album->name,
                                         info->devices[j].device);
          }
        device_album->plugin_info = info;
        plugin_album->children = append_album(plugin_album->children,
                                              device_album);
        j++;
        } /* Device loop */
      }
    } /* Plugin loop */
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
  if(t->com.directory)
    free(t->com.directory);
  if(t->com.load_handle)
    bg_plugin_unref(t->com.load_handle);
  free(t);
  }






bg_album_t * bg_media_tree_append_album(bg_media_tree_t * tree,
                                        bg_album_t * parent)
  {
  bg_album_t * album_before;
  bg_album_t * album_after;
  
  bg_album_t * new_album =
    bg_album_create(&(tree->com), BG_ALBUM_TYPE_REGULAR, parent);

  if(parent)
    {
    bg_album_append_child(parent, new_album);
    }

  /* Top level album, take care of the system albums */

  else
    {
    if(tree->children)
      {
      album_before = tree->children;

      /* Tree has only device albums */
            
      if(album_before->type == BG_ALBUM_TYPE_PLUGIN)
        {
        new_album->next = tree->children; 
        tree->children = new_album;
        }
      else
        {
        while(album_before->next && (album_before->next->type != BG_ALBUM_TYPE_PLUGIN))
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
    bg_album_remove_from_parent(album);
  else
    tree->children = remove_from_list(tree->children, album);
  
  /* Check if album file if present */
  
  if(album->location)
    tmp_path = bg_sprintf("%s/%s", tree->com.directory, album->location);
  
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

  switch(after->type)
    {
    case BG_ALBUM_TYPE_REMOVABLE:
    case BG_ALBUM_TYPE_PLUGIN:
      return 0;
    case BG_ALBUM_TYPE_INCOMING:
    case BG_ALBUM_TYPE_REGULAR:
      return 1;
    }
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

  switch(before->type)
    {
    case BG_ALBUM_TYPE_REMOVABLE:
    case BG_ALBUM_TYPE_PLUGIN:
      return 0;
    case BG_ALBUM_TYPE_INCOMING:
    case BG_ALBUM_TYPE_REGULAR:
      return 1;
    }
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

  switch(parent->type)
    {
    case BG_ALBUM_TYPE_REMOVABLE:
    case BG_ALBUM_TYPE_PLUGIN:
    case BG_ALBUM_TYPE_INCOMING:
      return 0;
    case BG_ALBUM_TYPE_REGULAR:
      return 1;
    }
  
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

void bg_media_tree_set_current(void * data,
                               bg_album_t * album,
                               const bg_album_entry_t * entry)
  {
  bg_album_t * last_current_album;
  
  bg_media_tree_t * t = (bg_media_tree_t*)data;

  last_current_album = t->com.current_album;
  
  if(last_current_album && (last_current_album != album))
    {
    t->com.current_entry = (bg_album_entry_t*)0;
    bg_album_changed(last_current_album);
    }
  t->com.current_album = album;

  if(t->com.current_album)
    {
    t->com.current_entry = t->com.current_album->entries;
    
    while(t->com.current_entry != entry)
      t->com.current_entry = t->com.current_entry->next;
    }
  if(t->change_callback)
    t->change_callback(t, t->change_callback_data);
  }

/* Set the next and previous track */

int bg_media_tree_next(bg_media_tree_t * tree, int wrap)
  {
  if(tree->com.current_album)
    return bg_album_next(tree->com.current_album, wrap);
  return 0;
  }

int bg_media_tree_previous(bg_media_tree_t * tree, int wrap)
  {
  if(tree->com.current_album)
    return bg_album_previous(tree->com.current_album, wrap);
  return 0;
  }

void bg_media_tree_set_change_callback(bg_media_tree_t * tree,
                                       void (*change_callback)(bg_media_tree_t*, void*),
                                       void* change_callback_data)
  {
  tree->change_callback = change_callback;
  tree->change_callback_data = change_callback_data;
  }

void bg_media_tree_set_error_callback(bg_media_tree_t * tree,
                                      void (*error_callback)(bg_media_tree_t*, void*,const char*),
                                      void* error_callback_data)
  {
  tree->error_callback      = error_callback;
  tree->error_callback_data = error_callback_data;
  }

void bg_media_tree_set_play_callback(bg_media_tree_t * tree,
                                     void (*play_callback)(void*),
                                     void* play_callback_data)
  {
  tree->com.play_callback      = play_callback;
  tree->com.play_callback_data = play_callback_data;
  }

bg_plugin_handle_t *
bg_media_tree_get_current_track(bg_media_tree_t * t, int * index)
  {
  char * error_message;
  bg_track_info_t * track_info;
  const bg_plugin_info_t * info;
  bg_input_plugin_t * input_plugin;
  bg_plugin_handle_t * ret;
  char * system_location = (char*)0;
  
  //  fprintf(stderr, "bg_media_tree_get_current_track %p %p\n",
  //          t->com.current_entry, t->com.current_album);
  
  if(!t->com.current_entry || !t->com.current_album)
    {
    error_message = bg_sprintf("Select a track first");
    goto fail;
    }
  if(t->com.current_album->type == BG_ALBUM_TYPE_REMOVABLE)
    {
    ret = t->com.current_album->handle;
    bg_plugin_ref(ret);
    input_plugin = (bg_input_plugin_t*)(ret->plugin);
    }
  else
    {
    system_location = bg_utf8_to_system(t->com.current_entry->location,
                                        strlen(t->com.current_entry->location));

    if(t->com.current_entry->plugin)
      info = bg_plugin_find_by_name(t->com.plugin_reg, t->com.current_entry->plugin);
    else
      info = bg_plugin_find_by_filename(t->com.plugin_reg,
                                        system_location,
                                        (BG_PLUGIN_INPUT));
    
    if(!info)
      {
      error_message = bg_sprintf("Cannot open %s: no plugin found",
                                 system_location);
      goto fail;
      }
    ret = bg_plugin_load(t->com.plugin_reg, info);
    input_plugin = (bg_input_plugin_t*)(ret->plugin);

    if(!input_plugin->open(ret->priv,
                           system_location))
      {
      if(input_plugin->common.get_error)
        error_message = bg_sprintf("Cannot open %s: %s",
                                   t->com.current_entry->location,
                                   input_plugin->common.get_error(ret->priv));
      else
        error_message = bg_sprintf("Cannot open %s", t->com.current_entry->location);
      goto fail;
      }
    }
  track_info = input_plugin->get_track_info(ret->priv,
                                            t->com.current_entry->index);
  bg_album_update_entry(t->com.current_album, t->com.current_entry, track_info);
  bg_album_changed(t->com.current_album);

  if(system_location)
    free(system_location);
    
  if(index)
    *index = t->com.current_entry->index;
  return ret;
  
  fail:
  if(system_location)
    free(system_location);
  if(t->error_callback)
    t->error_callback(t, t->error_callback_data, error_message);
  free(error_message);
  bg_media_tree_mark_error(t, 1);
  return (bg_plugin_handle_t *)0;
  }

bg_album_t * bg_media_tree_get_current_album(bg_media_tree_t * t)
  {
  return t->com.current_album;
  }

const char * bg_media_tree_get_current_track_name(bg_media_tree_t * t)
  {
  if(!t->com.current_album || !t->com.current_entry)
    return (const char *)0;
  return t->com.current_entry->name;
  }

void bg_media_tree_set_coords(bg_media_tree_t * t, int x, int y,
                              int width, int height)
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

void bg_media_tree_set_parameter(void * priv, char * name,
                                 bg_parameter_value_t * val)
  {
  bg_media_tree_t * tree;
  tree = (bg_media_tree_t*)priv;
  if(!name)
    return;

  if(!strcmp(name, "use_metadata"))
    {
    tree->com.use_metadata = val->val_i;
    }
  else if(!strcmp(name, "metadata_format"))
    {
    tree->com.metadata_format = val->val_str;
    }
  }

void bg_media_tree_mark_error(bg_media_tree_t * t, int err)
  {
  if(t->com.current_entry)
    {
    if(err)
      t->com.current_entry->flags |= BG_ALBUM_ENTRY_ERROR;
    else
      t->com.current_entry->flags &= ~BG_ALBUM_ENTRY_ERROR;
    }
  if(t->com.current_album)
    bg_album_changed(t->com.current_album);
  }


void bg_media_tree_rename_album(bg_media_tree_t * t,
                                bg_album_t * a, const char * name)
  {
  a->name = bg_strdup(a->name, name);

  if((a->type == BG_ALBUM_TYPE_REMOVABLE) &&
      a->plugin_info)
    {
    bg_plugin_registry_set_device_name(t->com.plugin_reg, a->plugin_info->name, a->location,
                                       name);
    }
  }

