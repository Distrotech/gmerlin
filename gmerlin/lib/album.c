/*****************************************************************
 
  album.c
 
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

#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tree.h>
#include <treeprivate.h>

#include <utils.h>

bg_album_t * bg_album_create(bg_media_tree_t * tree, bg_album_t * parent)
  {
  bg_album_t * ret = calloc(1, sizeof(*ret));
  ret->tree = tree;
  ret->parent = parent;
  pthread_mutex_init(&(ret->mutex),(pthread_mutexattr_t *)0);
  return ret;
  }

char * bg_album_get_name(bg_album_t * a)
  {
  return a->name;
  }

char * bg_album_get_location(bg_album_t * a)
  {
  return a->location;
  }

void bg_album_set_name(bg_album_t * a, const char * name)
  {
  a->name = bg_strdup(a->name, name);
  }

int bg_album_get_num_entries(bg_album_t * a)
  {
  int ret = 0;
  bg_album_entry_t * entry;
  entry = a->entries;

  while(entry)
    {
    ret++;
    entry = entry->next;
    }
  return ret;
  }

bg_album_entry_t * bg_album_get_entry(bg_album_t * a, int i)
  {
  bg_album_entry_t * ret;
  ret = a->entries;

  while(i--)
    ret = ret->next;
  return ret;
  }

void bg_album_lock(bg_album_t * a)
  {
  pthread_mutex_lock(&(a->mutex));
  }

void bg_album_unlock(bg_album_t * a)
  {
  pthread_mutex_unlock(&(a->mutex));
  }

/* Add items */

static bg_album_entry_t * load_urls(bg_album_t * a,
                                    char ** locations,
                                    const char * plugin)
  {
  int i;
  bg_album_entry_t * new_entry;
  bg_album_entry_t * ret;
  bg_album_entry_t * end_entry;
  
  i = 0;

  ret       = (bg_album_entry_t*)0;
  end_entry = (bg_album_entry_t*)0;
  new_entry = (bg_album_entry_t*)0;
    
  fprintf(stderr, "Load URL\n");
  while(locations[i])
    {
    fprintf(stderr, "Load URL: %s\n", locations[i]);
    new_entry = bg_media_tree_load_url(a->tree,
                                       locations[i],
                                       plugin);
    if(!new_entry)
      {
      i++;
      continue;
      }
    
    if(!end_entry)
      {
      end_entry = new_entry;
      ret       = new_entry;
      }
    else
      {
      end_entry->next = new_entry;
      while(end_entry->next)
        end_entry = end_entry->next;
      }
    i++;
    }
  return ret;
  }

void bg_album_insert_entries_after(bg_album_t * album,
                                   bg_album_entry_t * new_entries,
                                   bg_album_entry_t * before)
  {
  bg_album_entry_t * last_new_entry;

  if(!new_entries)
    return;
  
  last_new_entry = new_entries;
  while(last_new_entry->next)
    last_new_entry = last_new_entry->next;
  
  if(!before)
    {
    last_new_entry->next = album->entries;
    album->entries = new_entries;
    }
  else
    {
    last_new_entry->next = before->next;
    before->next = new_entries;
    }

  if(!(album->flags & BG_ALBUM_REMOVABLE) &&
     !album->location &&
     album->tree)
    {
    album->location = bg_media_tree_new_album_filename(album->tree);
    }
  }

void bg_album_insert_entries_before(bg_album_t * album,
                                    bg_album_entry_t * new_entries,
                                    bg_album_entry_t * after)
  {
  bg_album_entry_t * before;
  bg_album_entry_t * last_new_entry;

  if(!new_entries)
    return;
  
  last_new_entry = new_entries;
  while(last_new_entry->next)
    last_new_entry = last_new_entry->next;

  /* Fill empty album */

  if(!album->entries)
    {
    album->entries = new_entries;
    }
  
  /* Append as first item */
  
  else if(after == album->entries)
    {
    last_new_entry->next = album->entries;
    album->entries = new_entries;
    }
  else
    {
    before = album->entries;
    while(before->next != after)
      before = before->next;
    before->next = new_entries;
    last_new_entry->next = after;
    }

  if(!(album->flags & BG_ALBUM_REMOVABLE) &&
     !album->location &&
     album->tree)
    {
    album->location = bg_media_tree_new_album_filename(album->tree);
    }
  }

void bg_album_insert_urls_before(bg_album_t * a,
                                 char ** locations,
                                 const char * plugin,
                                 bg_album_entry_t * after)
  {
  bg_album_entry_t * new_entries;
  new_entries = load_urls(a, locations, plugin);

  bg_album_insert_entries_before(a, new_entries, after);
  
  }

void bg_album_insert_urls_after(bg_album_t * a,
                                char ** locations,
                                const char * plugin,
                                bg_album_entry_t * before)
  {
  bg_album_entry_t * new_entries;
  new_entries = load_urls(a, locations, plugin);
  
  bg_album_insert_entries_after(a, new_entries, before);
  }

/* Inserts a string of the type text/uri-list into the album */

void bg_album_insert_urilist_after(bg_album_t * a, const char * str,
                                   int len, bg_album_entry_t * before)
  {
  char ** uri_list;
  
  uri_list = bg_urilist_decode(str, len);

  if(!uri_list)
    return;

  bg_album_insert_urls_after(a, uri_list, (const char*)0, before);

  bg_urilist_free(uri_list);
  }

void bg_album_insert_urilist_before(bg_album_t * a, const char * str,
                                    int len, bg_album_entry_t * after)
  {
  char ** uri_list;
  
  uri_list = bg_urilist_decode(str, len);

  if(!uri_list)
    return;

  bg_album_insert_urls_before(a, uri_list, (const char*)0, after);
  
  bg_urilist_free(uri_list);
  }

/* Open / close */

static void open_removable(bg_album_t * a)
  {
  bg_parameter_value_t val;
  bg_track_info_t * track_info;
  int i;
  int num_tracks;
  const bg_plugin_info_t * info;
  bg_input_plugin_t * plugin;
  bg_album_entry_t * new_entry;
    
  info =
    bg_plugin_find_by_long_name(a->tree->plugin_reg,
                                a->plugin_name);

  a->handle = bg_plugin_load(a->tree->plugin_reg, info);
  
  bg_plugin_lock(a->handle);

  plugin = (bg_input_plugin_t*)a->handle->plugin;
  
  /* Select device */
  val.val_str = a->location;
  plugin->common.set_parameter(a->handle->priv, "device", &val);
  plugin->common.set_parameter(a->handle->priv, NULL, NULL);

  /* Open the plugin */

  plugin->open(a->handle->priv, (void*)0);

  /* Get number of tracks */

  num_tracks = plugin->get_num_tracks(a->handle->priv);

  for(i = 0; i < num_tracks; i++)
    {
    track_info = plugin->get_track_info(a->handle->priv, i);
    
    new_entry = calloc(1, sizeof(*new_entry));

    new_entry->index = i;
    new_entry->name = bg_strdup((char*)0, track_info->name);

    new_entry->num_video_streams = track_info->num_video_streams;
    new_entry->num_audio_streams = track_info->num_audio_streams;
    new_entry->num_subpicture_streams = track_info->num_subpicture_streams;
    new_entry->num_programs = track_info->num_programs;
    new_entry->duration = track_info->duration;

    bg_album_insert_entries_before(a, new_entry, (bg_album_entry_t*)0);
    }
  bg_plugin_unlock(a->handle);
  }

void bg_album_open(bg_album_t * a)
  {
  char * tmp_filename;

  a->open_count++;

  if(a->open_count > 1)
    return;

  if(bg_album_is_removable(a))
    {
    /* Get infos from the plugin */
    open_removable(a);
    }
  else if(a->location)
    {
    tmp_filename = bg_sprintf("%s/%s", a->tree->directory, a->location);
    bg_album_load(a, tmp_filename);
    if(!a->entries)
      {
      free(a->location);
      a->location = (char*)0;
      }
    free(tmp_filename);
    }
  }

void bg_album_entry_destroy(bg_album_entry_t * entry)
  {
  if(entry->name)
    free(entry->name);
  if(entry->location)
    free(entry->location);
  if(entry->plugin)
    free(entry->plugin);
  free(entry);
  }

bg_album_entry_t * bg_album_entry_create(bg_media_tree_t * tree)
  {
  bg_album_entry_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->id = bg_media_tree_get_unique_id(tree);
  return ret;
  }


void bg_album_close(bg_album_t *a )
  {

  bg_album_entry_t * tmp_entry;
  char * tmp_filename;
  
  a->open_count--;
  
  if(!a->open_count)
    {
    /* Tell the tree, if we are the current album */

    if(a->tree && (a == a->tree->current_album))
      {
      bg_media_tree_set_current(a->tree,
                                (bg_album_t*)0,
                                (const bg_album_entry_t*)0);
      }
    
    if(bg_album_is_removable(a) && !a->children)
      {
      bg_plugin_unref(a->handle);
      }
    else
      {
      if(!a->entries) /* If album is empty, delete the file */
        {
        if(a->tree && a->location)
          {
          tmp_filename = bg_sprintf("%s/%s", a->tree->directory, a->location);
          remove(tmp_filename);
          free(tmp_filename);
          free(a->location);
          a->location = (char*)0;
          }
        }
      else
        bg_album_save(a, NULL);
      }
    
    /* Delete entries */
    
    while(a->entries)
      {
      tmp_entry = a->entries->next;
      bg_album_entry_destroy(a->entries);
      a->entries = tmp_entry;
      }
    }
  }


int bg_album_is_removable(bg_album_t * a)
  {
  return (a->flags & BG_ALBUM_REMOVABLE) ? 1 : 0;
  }

void bg_album_set_expanded(bg_album_t * a, int expanded)
  {
  if(expanded)
    a->flags |= BG_ALBUM_EXPANDED;
  else
    a->flags &= ~BG_ALBUM_EXPANDED;
  }

int bg_album_get_expanded(bg_album_t * a)
  {
  if(a->flags & BG_ALBUM_EXPANDED)
    return 1;
  return 0;
  }

int bg_album_is_open(bg_album_t * a)
  {
  return (a->open_count) ? 1 : 0;
  }

static void free_entry(bg_album_entry_t * e)
  {
  if(e->name)
    free(e->name);
  
  if(e->location)
    free(e->location);
  
  if(e->plugin)
    free(e->plugin);
  free(e);

  }

void bg_album_destroy(bg_album_t * a)
  {
  bg_album_t       * tmp_album;
  bg_album_entry_t * tmp_entry;

  if(a->open_count && a->entries)
    bg_album_save(a, (const char*)0);
   
  if(a->name)
    free(a->name);
  if(a->location)
    free(a->location);
  if(a->open_path)
    free(a->open_path);

  pthread_mutex_destroy(&(a->mutex));
  
  /* Free entries */

  while(a->entries)
    {
    tmp_entry = a->entries->next;

    free_entry(a->entries);
    
    a->entries = tmp_entry;
    }
    
  /* Free Children */

  while(a->children)
    {
    tmp_album = a->children->next;
    bg_album_destroy(a->children);
    a->children = tmp_album;
    }

  /* free rest */

  free(a);
  }

void bg_album_delete_selected(bg_album_t * album)
  {
  bg_album_entry_t * cur;
  bg_album_entry_t * cur_next;
  bg_album_entry_t * new_entries_end = (bg_album_entry_t *)0;
  bg_album_entry_t * new_entries;

  if(!album->entries)
    return;

  cur = album->entries;
  new_entries = (bg_album_entry_t*)0;

  while(cur)
    {
    cur_next = cur->next;

    if(cur->flags & BG_ALBUM_ENTRY_SELECTED)
      free_entry(cur);
    else
      {
      if(!new_entries)
        {
        new_entries = cur;
        new_entries_end = cur;
        }
      else
        {
        new_entries_end->next = cur;
        new_entries_end = new_entries_end->next;
        }
      }
    cur = cur_next;
    }
  if(new_entries)
    new_entries_end->next = (bg_album_entry_t*)0;
  album->entries = new_entries;
  }

void bg_album_select_error_tracks(bg_album_t * album)
  {
  bg_album_entry_t * cur;
  cur = album->entries;
  while(cur)
    {
    if(cur->flags & BG_ALBUM_ENTRY_ERROR)
      cur->flags |= BG_ALBUM_ENTRY_SELECTED;
    else
      cur->flags &= ~BG_ALBUM_ENTRY_SELECTED;
    cur = cur->next;
    }
  }

void bg_album_refresh_selected(bg_album_t * album)
  {
  bg_album_entry_t * cur;
  cur = album->entries;
  while(cur)
    {
    if(cur->flags & BG_ALBUM_ENTRY_SELECTED)
      bg_media_tree_refresh_entry(album->tree, cur);
    cur = cur->next;
    }
  }


static bg_album_entry_t * extract_selected(bg_album_t * album)
  {
  bg_album_entry_t * selected_end = (bg_album_entry_t *)0;
  bg_album_entry_t * other_end = (bg_album_entry_t *)0;
  bg_album_entry_t * tmp_entry;
  
  bg_album_entry_t * other    = (bg_album_entry_t*)0;
  bg_album_entry_t * selected = (bg_album_entry_t*)0;
  
  while(album->entries)
    {
    tmp_entry = album->entries->next;

    if(album->entries->flags & BG_ALBUM_ENTRY_SELECTED)
      {
      if(!selected)
        {
        selected = album->entries;
        selected_end = selected;
        }
      else
        {
        selected_end->next = album->entries;
        selected_end = selected_end->next;
        }
      selected_end->next = (bg_album_entry_t*)0;
      }
    else
      {
      if(!other)
        {
        other = album->entries;
        other_end = other;
        }
      else
        {
        other_end->next = album->entries;
        other_end = other_end->next;
        }
      other_end->next = (bg_album_entry_t*)0;
      }
    album->entries = tmp_entry;
    }
  album->entries = other;
  return selected;
  }

void bg_album_move_selected_up(bg_album_t * album)
  {
  bg_album_entry_t * selected;
  selected = extract_selected(album);

  bg_album_insert_entries_after(album,
                                selected,
                                (bg_album_entry_t*)0);
  }

void bg_album_move_selected_down(bg_album_t * album)
  {
  bg_album_entry_t * selected;
  selected = extract_selected(album);

  bg_album_insert_entries_before(album,
                                 selected,
                                 (bg_album_entry_t*)0);
  }

typedef struct
  {
  bg_album_entry_t * entry;
  char * sort_string;
  } sort_struct;

void bg_album_sort(bg_album_t * album)
  {
  int num_entries;
  int i, j;
  char * tmp_string;
  int sort_string_len;
  sort_struct * s_tmp;
  int keep_going;
  
  sort_struct ** s;
  bg_album_entry_t * tmp_entry;
  
  /* 1. Count the entries */
  
  num_entries = 0;

  tmp_entry = album->entries;

  while(tmp_entry)
    {
    tmp_entry = tmp_entry->next;
    num_entries++;
    }

  if(!num_entries)
    return;
  
  /* Set up the album array */

  s = malloc(num_entries * sizeof(sort_struct*));

  tmp_entry = album->entries;
  
  for(i = 0; i < num_entries; i++)
    {
    s[i] = calloc(1, sizeof(sort_struct));
    s[i]->entry = tmp_entry;

    /* Set up the sort string */

    tmp_string = bg_utf8_to_system(tmp_entry->name,
                                   strlen(tmp_entry->name));

    sort_string_len = strxfrm((char*)0, tmp_string, 0);
    s[i]->sort_string = malloc(sort_string_len+1);
    strxfrm(s[i]->sort_string, tmp_string, sort_string_len+1);

    free(tmp_string);
    
    /* Advance */
    
    tmp_entry = tmp_entry->next;
    }

  /* Now, do a braindead bubblesort algorithm */

  for(i = 0; i < num_entries - 1; i++)
    {
    keep_going = 0;
    for(j = num_entries-1; j > i; j--)
      {
      if(strcmp(s[j]->sort_string, s[j-1]->sort_string) < 0)
        {
        s_tmp  = s[j];
        s[j]   = s[j-1];
        s[j-1] = s_tmp;
        keep_going = 1;
        }
      }
    if(!keep_going)
      break;
    }
  
  /* Rechain entries */

  album->entries = s[0]->entry;

  for(i = 0; i < num_entries-1; i++)
    {
    s[i]->entry->next = s[i+1]->entry;
    }
  
  s[num_entries-1]->entry->next =
    (bg_album_entry_t*)0;
  
  /* Free everything */

  for(i = 0; i < num_entries; i++)
    {
    free(s[i]->sort_string);
    free(s[i]);
    }
  free(s);
  }

void bg_album_rename_track(bg_album_t * album,
                           const bg_album_entry_t * entry_c,
                           const char * name)
  {
  bg_album_entry_t * entry;

  entry = album->entries;

  while(entry)
    {
    if(entry == entry_c)
      break;
    entry = entry->next;
    }
  entry->name = bg_strdup(entry->name, name);
  }

bg_album_entry_t * bg_album_get_current_entry(bg_album_t * a)
  {
  return a->current_entry;
  }

int bg_album_next(bg_album_t * a, int wrap)
  {
  if(a->current_entry)
    {
    if(!a->current_entry->next)
      {
      if(wrap)
        {
        a->current_entry = a->entries;
        if(a->tree)
          bg_media_tree_set_current(a->tree, a, a->current_entry);
        bg_album_changed(a);
        return 1;
        }
      else
        return 0;
      }
    else
      {
      a->current_entry = a->current_entry->next;
      if(a->tree)
        bg_media_tree_set_current(a->tree, a, a->current_entry);
      bg_album_changed(a);
      return 1;
      }
    }
  else
    return 0;
  
  }

int bg_album_previous(bg_album_t * a, int wrap)
  {
  bg_album_entry_t * tmp_entry;
  
  if(!a->current_entry)
    return 0;
    
  if(a->current_entry == a->entries)
    {
    if(!wrap)
      return 0;
    tmp_entry = a->entries; 

    while(tmp_entry->next)
      tmp_entry = tmp_entry->next;
    a->current_entry = tmp_entry;
    if(a->tree)
      bg_media_tree_set_current(a->tree, a, a->current_entry);

    bg_album_changed(a);
    return 1;
    }
  else
    {
    tmp_entry = a->entries; 
    while(tmp_entry->next != a->current_entry)
      tmp_entry = tmp_entry->next;
    a->current_entry = tmp_entry;
    if(a->tree)
      bg_media_tree_set_current(a->tree, a, a->current_entry);

    bg_album_changed(a);
    return 1;    
    }

  }

void bg_album_set_change_callback(bg_album_t * a,
                                  void (*change_callback)(bg_album_t * a, void * data),
                                  void * change_callback_data)
  {
  a->change_callback      = change_callback;
  a->change_callback_data = change_callback_data;
  }

void bg_album_changed(bg_album_t * a)
  {
  if(a->change_callback)
    a->change_callback(a, a->change_callback_data);
  }

void bg_album_set_current(bg_album_t * a, const bg_album_entry_t * e)
  {
  bg_album_entry_t * tmp_entry;
  
  if(a->tree)
    bg_media_tree_set_current(a->tree, a, e);

  tmp_entry = a->entries;
  while(tmp_entry != e)
    tmp_entry = tmp_entry->next;
  
  a->current_entry = tmp_entry;
  bg_album_changed(a);
  }


void bg_album_play(bg_album_t * a)
  {
  if(a->tree && a->tree->play_callback)
    a->tree->play_callback(a->tree, a->tree->play_callback_data);
  }


void bg_album_set_coords(bg_album_t * a, int x, int y, int width, int height)
  {
  a->x      = x;
  a->y      = y;
  a->width  = width;
  a->height = height;
  }

void bg_album_get_coords(bg_album_t * a, int * x, int * y,
                         int * width, int * height)
  {
  *x      = a->x;
  *y      = a->y;
  *width  = a->width;
  *height = a->height;
  }
     
void bg_album_set_open_path(bg_album_t * a, const char * path)
  {
  a->open_path = bg_strdup(a->open_path, path);
  }

const char * bg_album_get_open_path(bg_album_t * a)
  {
  return a->open_path;
  }

int bg_album_is_current(bg_album_t * a)
  {
  if(!a->tree)
    return 1;
  return (a == a->tree->current_album) ? 1 : 0;
  }

int bg_album_entry_is_current(bg_album_t * a,
                              bg_album_entry_t * e)
  {
  if(!a->tree)
    return 0;
  return ((a == a->tree->current_album) &&
          (e == a->tree->current_entry)) ? 1 : 0; 
  }

char ** bg_album_get_plugins(bg_album_t * a,
                             uint32_t type_mask,
                             uint32_t flag_mask)
  {
  int num_plugins, i;
  char ** ret;
  bg_plugin_registry_t * reg;
  const bg_plugin_info_t * info;
  if(!a->tree)
    return (char**)0;
    
  reg = a->tree->plugin_reg;
  
  num_plugins = bg_plugin_registry_get_num_plugins(reg, type_mask, flag_mask);
  ret = calloc(num_plugins + 1, sizeof(char*));
  for(i = 0; i < num_plugins; i++)
    {
    info = bg_plugin_find_by_index(reg, i, type_mask, flag_mask);
    ret[i] = bg_strdup(NULL, info->long_name);
    }
  return ret;
  }

void bg_album_free_plugins(bg_album_t * a, char ** plugins)
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

