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
#include <http.h>

static char * new_filename(bg_album_t * album)
  {
  /*
   *  Album filenames are constructed like "aXXXXXXXX.xml",
   *  where XXXXXXXX is a hexadecimal unique identifier
   */
  char * template = (char*)0;
  char * path = (char*)0;
  char * ret = (char*)0;
  char * pos;
  
  template = bg_sprintf("%s/a%%08x.xml", album->com->directory);

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

void bg_album_update_entry(bg_album_t * album,
                           bg_album_entry_t * entry,
                           bg_track_info_t  * track_info)
  {
  char * start_pos;
  char * end_pos;
  int name_set = 0;
  //  fprintf(stderr, "Update entry!\n");
  entry->num_audio_streams = track_info->num_audio_streams;
  entry->num_video_streams = track_info->num_video_streams;

  entry->num_subpicture_streams = track_info->num_subpicture_streams;
  entry->num_programs           = track_info->num_programs;

  if(!(entry->flags & BG_ALBUM_ENTRY_PRIVNAME))
    {
    if(entry->name)
      {
      free(entry->name);
      entry->name = (char*)0;
      }
    /* Track info has a name */
    
    if(album->com->use_metadata && album->com->metadata_format)
      {
      entry->name = bg_create_track_name(track_info,
                                         album->com->metadata_format);
      if(entry->name)
        name_set = 1;
      }
    
    if(!name_set)
      {
      if(track_info->name)
        {
        entry->name = bg_strdup(entry->name, track_info->name);
        //      fprintf(stderr, "entry->name: %s\n", entry->name);
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
    }
  entry->duration = track_info->duration;
  entry->flags &= ~BG_ALBUM_ENTRY_ERROR;
  
  if(track_info->url)
    {
    entry->location = bg_strdup(entry->location, track_info->url);
    entry->index = 0;
    entry->total_tracks = 1;
    }
  if(track_info->plugin)
    entry->plugin   = bg_strdup(entry->plugin, track_info->plugin);
  
  }

bg_album_t * bg_album_create(bg_album_common_t * com, bg_album_type_t type,
                             bg_album_t * parent)
  {
  bg_album_t * ret = calloc(1, sizeof(*ret));
  ret->com = com;
  ret->parent = parent;
  ret->type = type;
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
#if 0
static bg_album_entry_t * load_url(bg_album_t * a,
                                   char * location,
                                   const char * plugin)
  {
  bg_album_entry_t * new_entry;
  bg_album_entry_t * ret;
  bg_album_entry_t * end_entry;
  
  ret       = (bg_album_entry_t*)0;
  end_entry = (bg_album_entry_t*)0;
  new_entry = (bg_album_entry_t*)0;
    
    new_entry = bg_album_load_url(a,
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
#endif
static void insertion_done(bg_album_t * album)
  {
  switch(album->type)
    {
    case BG_ALBUM_TYPE_REGULAR:
      if(!album->location)
        album->location = new_filename(album);
      break;
    case BG_ALBUM_TYPE_INCOMING:
    case BG_ALBUM_TYPE_REMOVABLE:
    case BG_ALBUM_TYPE_PLUGIN:
      break;
    }
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

  insertion_done(album);
  
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

  insertion_done(album);

  }

void bg_album_insert_urls_before(bg_album_t * a,
                                 char ** locations,
                                 const char * plugin,
                                 bg_album_entry_t * after)
  {
  int i = 0;
  bg_album_entry_t * new_entries;

  while(locations[i])
    {
    new_entries = bg_album_load_url(a, locations[i], plugin);
    bg_album_insert_entries_before(a, new_entries, after);
    bg_album_changed(a);
    i++;
    }
  }

void bg_album_insert_urls_after(bg_album_t * a,
                                char ** locations,
                                const char * plugin,
                                bg_album_entry_t * before)
  {
  int i = 0;
  bg_album_entry_t * new_entries;

  while(locations[i])
    {
    new_entries = bg_album_load_url(a, locations[i], plugin);
    bg_album_insert_entries_after(a, new_entries, before);

    before = new_entries;
    while(before->next)
      before = before->next;
    bg_album_changed(a);
    i++;
    }
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

static int open_removable(bg_album_t * a)
  {
  //  bg_parameter_value_t val;
  bg_track_info_t * track_info;
  int i;
  int num_tracks;
  //  const bg_plugin_info_t * info;
  bg_input_plugin_t * plugin;
  bg_album_entry_t * new_entry;
  
  a->handle = bg_plugin_load(a->com->plugin_reg, a->plugin_info);

  bg_plugin_lock(a->handle);

  plugin = (bg_input_plugin_t*)a->handle->plugin;
  
  /* Open the plugin */

  if(!plugin->open(a->handle->priv, a->location))
    {
    bg_plugin_unlock(a->handle);
    return 0;
    }
  
  /* Get number of tracks */

  num_tracks = plugin->get_num_tracks(a->handle->priv);

  for(i = 0; i < num_tracks; i++)
    {
    track_info = plugin->get_track_info(a->handle->priv, i);
    
    new_entry = calloc(1, sizeof(*new_entry));

    new_entry->index = i;
    new_entry->total_tracks = num_tracks;
    new_entry->name   = bg_strdup((char*)0, track_info->name);
    new_entry->plugin = bg_strdup((char*)0, a->handle->info->name);
    
    new_entry->location = bg_strdup(new_entry->location,
                                      a->location);
    new_entry->num_video_streams = track_info->num_video_streams;
    new_entry->num_audio_streams = track_info->num_audio_streams;
    new_entry->num_subpicture_streams =
      track_info->num_subpicture_streams;
    new_entry->num_programs = track_info->num_programs;
    //    fprintf(stderr, "Album Duration: %lld\n", track_info->duration);
    new_entry->duration = track_info->duration;

    bg_album_insert_entries_before(a, new_entry, (bg_album_entry_t*)0);
    }
  bg_plugin_unlock(a->handle);
  return 1;
  }

bg_album_type_t bg_album_get_type(bg_album_t * a)
  {
  return a->type;
  }

int bg_album_open(bg_album_t * a)
  {
  char * tmp_filename;
  
  if(a->open_count)
    return 1;

  switch(a->type)
    {
    case BG_ALBUM_TYPE_REGULAR:
      if(a->location)
        {
        tmp_filename = bg_sprintf("%s/%s", a->com->directory, a->location);
        bg_album_load(a, tmp_filename);
        if(!a->entries)
          {
          free(a->location);
          a->location = (char*)0;
          }
        free(tmp_filename);
        }
      break;
    case BG_ALBUM_TYPE_REMOVABLE:
      /* Get infos from the plugin */
      if(!open_removable(a))
        return 0;
      break;
    case BG_ALBUM_TYPE_PLUGIN:
      return 0; /* Cannot be opened */
      break;
    case BG_ALBUM_TYPE_INCOMING:
      tmp_filename = bg_sprintf("%s/incoming.xml", a->com->directory);
      bg_album_load(a, tmp_filename);
      if(!a->entries)
        {
        free(a->location);
        a->location = (char*)0;
        }
      free(tmp_filename);
      break;
    }
  a->open_count++;
  return 1;
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

bg_album_entry_t * bg_album_entry_create(bg_album_t * album)
  {
  bg_album_entry_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->id = bg_album_get_unique_id(album);
  return ret;
  }


void bg_album_close(bg_album_t *a )
  {

  bg_album_entry_t * tmp_entry;
  char * tmp_filename;
  
  a->open_count--;

  if(a->open_count)
    return;
    
  /* Tell the tree, if we are the current album */
  
  if((a == a->com->current_album) && a->com->set_current_callback)
    {
    a->com->set_current_callback(a->com->set_current_callback_data,
                                 (bg_album_t*)0,
                                 (const bg_album_entry_t*)0);
    }
  switch(a->type)
    {
    case BG_ALBUM_TYPE_REMOVABLE:
      bg_plugin_unref(a->handle);
      break;
    case BG_ALBUM_TYPE_REGULAR:
      if(a->location)
        {
        if(!a->entries) /* If album is empty, delete the file */
          {
          tmp_filename = bg_sprintf("%s/%s", a->com->directory, a->location);
          remove(tmp_filename);
          free(tmp_filename);
          free(a->location);
          a->location = (char*)0;
          }
        else
          bg_album_save(a, NULL);
        }
      break;
    case BG_ALBUM_TYPE_INCOMING:
      bg_album_save(a, NULL);
      break;
    case BG_ALBUM_TYPE_PLUGIN:
      break;
    }
  
  /* Delete entries */
  
  while(a->entries)
    {
    tmp_entry = a->entries->next;
    bg_album_entry_destroy(a->entries);
    a->entries = tmp_entry;
    }
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

    bg_album_entry_destroy(a->entries);
    
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
      bg_album_entry_destroy(cur);
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
      bg_album_refresh_entry(album, cur);
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
  entry->flags |= BG_ALBUM_ENTRY_PRIVNAME;
  }

bg_album_entry_t * bg_album_get_current_entry(bg_album_t * a)
  {
  return a->com->current_entry;
  }

int bg_album_next(bg_album_t * a, int wrap)
  {
  if(a->com->current_entry)
    {
    if(!a->com->current_entry->next)
      {
      if(wrap)
        {
        if(a->com->set_current_callback)
          a->com->set_current_callback(a->com->set_current_callback_data,
                                       a, a->entries);
        bg_album_changed(a);
        return 1;
        }
      else
        return 0;
      }
    else
      {
      if(a->com->set_current_callback)
        a->com->set_current_callback(a->com->set_current_callback_data,
                                     a, a->com->current_entry->next);
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
  
  if(!a->com->current_entry)
    return 0;
    
  if(a->com->current_entry == a->entries)
    {
    if(!wrap)
      return 0;
    tmp_entry = a->entries; 

    while(tmp_entry->next)
      tmp_entry = tmp_entry->next;
    if(a->com->set_current_callback)
      a->com->set_current_callback(a->com->set_current_callback_data,
                                   a, tmp_entry);
    bg_album_changed(a);
    return 1;
    }
  else
    {
    tmp_entry = a->entries; 
    while(tmp_entry->next != a->com->current_entry)
      tmp_entry = tmp_entry->next;
    if(a->com->set_current_callback)
      a->com->set_current_callback(a->com->set_current_callback_data,
                                   a, tmp_entry);
    
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
    
  tmp_entry = a->entries;
  while(tmp_entry != e)
    tmp_entry = tmp_entry->next;
  
  if(a->com->set_current_callback)
    a->com->set_current_callback(a->com->set_current_callback_data,
                                 a, tmp_entry);
  
  bg_album_changed(a);
  }


void bg_album_play(bg_album_t * a)
  {
  if(a->com->play_callback)
    a->com->play_callback(a->com->play_callback_data);
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
  return (a == a->com->current_album) ? 1 : 0;
  }

int bg_album_entry_is_current(bg_album_t * a,
                              bg_album_entry_t * e)
  {
  return ((a == a->com->current_album) &&
          (e == a->com->current_entry)) ? 1 : 0; 
  }

bg_plugin_registry_t * bg_album_get_plugin_registry(bg_album_t * a)
  {
  return a->com->plugin_reg;
  }

void bg_album_get_times(bg_album_t * a,
                        gavl_time_t * duration_before,
                        gavl_time_t * duration_current,
                        gavl_time_t * duration_after)
  {
  bg_album_entry_t * e;

  if(a != a->com->current_album)
    {
    *duration_before = GAVL_TIME_UNDEFINED;
    *duration_current = GAVL_TIME_UNDEFINED;
    *duration_after = GAVL_TIME_UNDEFINED;
    return;
    }

  e = a->entries;
  *duration_before = 0;
  while(e != a->com->current_entry)
    {
    if(e->duration == GAVL_TIME_UNDEFINED)
      {
      *duration_before = GAVL_TIME_UNDEFINED;
      break;
      }
    *duration_before += e->duration;
    e = e->next;
    }

  *duration_current = a->com->current_entry->duration;
  
  *duration_after = 0;

  e = a->com->current_entry->next;
  while(e)
    {
    if(e->duration == GAVL_TIME_UNDEFINED)
      {
      *duration_after = GAVL_TIME_UNDEFINED;
      break;
      }
    *duration_after += e->duration;
    e = e->next;
    }
  }

void bg_album_set_error(bg_album_t * a, int err)
  {
  if(err)
    a->flags |= BG_ALBUM_ERROR;
  else
    a->flags &= ~BG_ALBUM_ERROR;
  }

int  bg_album_get_error(bg_album_t * a)
  {
  return !!(a->flags & BG_ALBUM_ERROR);
  }

void bg_album_append_child(bg_album_t * parent, bg_album_t * child)
  {
  bg_album_t * album_before;
  if(parent->children)
    {
    album_before = parent->children;
    while(album_before->next)
      album_before = album_before->next;
    album_before->next = child;
    }
  else
    parent->children = child;
  }

static void add_device(bg_album_t * album,
                       const char * device,
                       const char * name)
  {
  bg_album_t * device_album;
  device_album = bg_album_create(album->com, BG_ALBUM_TYPE_REMOVABLE, album);
  device_album->location = bg_strdup(device_album->location, device);
  if(name)
    {
    device_album->name = bg_strdup(device_album->name,
                                   name);
    }
  else
    {
    device_album->name = bg_strdup(device_album->name,
                                   device);
    }

  device_album->plugin_info = album->plugin_info;
  bg_album_append_child(album, device_album);
  }

void bg_album_add_device(bg_album_t * album,
                         const char * device,
                         const char * name)
  {
  add_device(album, device, name);
  bg_plugin_registry_add_device(album->com->plugin_reg,
                                album->plugin_info->name,
                                device, name);
  }

static bg_album_t *
remove_from_list(bg_album_t * list, bg_album_t * album, int * index)
  {
  bg_album_t * sibling_before;

  *index = 0;
  
  if(album == list)
    return album->next;
  else
    {
    sibling_before = list;
    (*index)++;
    while(sibling_before->next != album)
      {
      sibling_before = sibling_before->next;
      (*index)++;
      }
    sibling_before->next = album->next;
    return list;
    }
  }

void bg_album_remove_from_parent(bg_album_t * album)
  {
  int index;
  if(!album->parent)
    return;
  
  album->parent->children = remove_from_list(album->parent->children, album, &index);

  fprintf(stderr, "bg_album_remove_from_parent: %d\n", index);
  
  if(album->type == BG_ALBUM_TYPE_REMOVABLE)
    {
    bg_plugin_registry_remove_device(album->com->plugin_reg,
                                     album->plugin_info->name,
                                     album->plugin_info->devices[index].device,
                                     album->plugin_info->devices[index].name);
    }
  }

void bg_album_set_devices(bg_album_t * a)
  {
  bg_album_t * tmp_album;
  int j;

  /* Delete previous children */
  while(a->children)
    {
    tmp_album = a->children->next;
    bg_album_destroy(a->children);
    a->children = tmp_album;
    }
  
  if(a->plugin_info->devices && a->plugin_info->devices->device)
    {
    j = 0;
    
    while(a->plugin_info->devices[j].device)
      {
      add_device(a, a->plugin_info->devices[j].device,
                 a->plugin_info->devices[j].name);
      j++;
      } /* Device loop */
    }

  }

void bg_album_find_devices(bg_album_t * a)
  {
  bg_plugin_registry_find_devices(a->com->plugin_reg, a->plugin_info->name);
  bg_album_set_devices(a);
  }

bg_album_entry_t * bg_album_load_url(bg_album_t * album,
                                     char * url,
                                     const char * plugin_long_name)
  {
  int i, num_entries;
    
  //  bg_redirector_plugin_t * redir;

  bg_album_entry_t * new_entry;
  bg_album_entry_t * end_entry = (bg_album_entry_t*)0;
  bg_album_entry_t * ret       = (bg_album_entry_t*)0;
  
  //  char * system_location;

  bg_input_plugin_t * plugin;
  bg_track_info_t * track_info;
  const bg_plugin_info_t * info;
  //  const char * file_plugin_name;
  
  //  fprintf(stderr, "bg_media_tree_load_url %s %s\n", url,
  //          (plugin_long_name ? plugin_long_name : "NULL"));
  
  /* Load the appropriate plugin */

  if(plugin_long_name)
    {
    info = bg_plugin_find_by_long_name(album->com->plugin_reg,
                                       plugin_long_name);
    }
  else
    info = (bg_plugin_info_t*)0;

  if(!bg_input_plugin_load(album->com->plugin_reg,
                           url, info,
                           &(album->com->load_handle)))
    return (bg_album_entry_t*)0;
  
  plugin = (bg_input_plugin_t*)(album->com->load_handle->plugin);
  
  /* Open the track */
  
  if(!plugin->get_num_tracks)
    num_entries = 1;
  else
    num_entries = plugin->get_num_tracks(album->com->load_handle->priv);
  
  for(i = 0; i < num_entries; i++)
    {
    new_entry = bg_album_entry_create(album);
    new_entry->location = bg_system_to_utf8(url, strlen(url));
    new_entry->index = i;
    new_entry->total_tracks = num_entries;
    //    fprintf(stderr, "Loading [%d/%d]\n", new_entry->total_tracks,
    //            new_entry->index);
    
    track_info = plugin->get_track_info(album->com->load_handle->priv, i);
    bg_album_update_entry(album, new_entry, track_info);

    if(plugin_long_name)
      new_entry->plugin = bg_strdup(new_entry->plugin,
                                    album->com->load_handle->info->name);
    
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
  plugin->close(album->com->load_handle->priv);
  return ret;
  }

int bg_album_get_unique_id(bg_album_t * album)
  {
  album->com->highest_id++;
  return album->com->highest_id;
  }

static int refresh_entry(bg_album_t * album,
                         bg_album_entry_t * entry)
  {
  char * system_location;

  bg_input_plugin_t * plugin;
  bg_track_info_t * track_info;
    
  /* Open the track */
  
  system_location = bg_utf8_to_system(entry->location,
                                      strlen(entry->location));

  plugin = (bg_input_plugin_t*)(album->com->load_handle->plugin);
  
  if(!plugin->open(album->com->load_handle->priv, system_location))
    {
    fprintf(stderr, "Opening %s with %s failed\n", system_location,
            album->com->load_handle->info->name);
    //    bg_hexdump(system_location, strlen(system_location));
        
    free(system_location);
    entry->flags |= BG_ALBUM_ENTRY_ERROR;
    return 0;
    }

  free(system_location);
  //  fprintf(stderr, "Refresh entry: %d\n", entry->index);
  track_info = plugin->get_track_info(album->com->load_handle->priv,
                                      entry->index);

  bg_album_update_entry(album, entry, track_info);
  plugin->close(album->com->load_handle->priv);
  return 1;
  }

int bg_album_refresh_entry(bg_album_t * album,
                           bg_album_entry_t * entry)
  {
  const bg_plugin_info_t * info;
  
  /* Check, which plugin to use */

  if(entry->plugin)
    {
    info = bg_plugin_find_by_name(album->com->plugin_reg, entry->plugin);
    }
  else
    info = (bg_plugin_info_t*)0;
  
  if(!bg_input_plugin_load(album->com->plugin_reg,
                           entry->location,
                           info,
                           &(album->com->load_handle)))
    {
    fprintf(stderr, "No plugin found for %s\n",
            (char*)(entry->location));
    entry->flags |= BG_ALBUM_ENTRY_ERROR;
    return 0;
    }
  return refresh_entry(album, entry);
  }
