/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <gtk/gtk.h> 
#include <stdlib.h> 
#include <string.h> 


#include <config.h>
#include <gmerlin/cfg_registry.h>
#include <gmerlin/preset.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gmerlin/cfg_dialog.h>

#include <gui_gtk/presetmenu.h>

struct bg_gtk_preset_menu_s
  {
  GtkWidget * menubar;
  GtkWidget * menu;
  
  GtkWidget ** preset_items;

  GtkWidget * save_item;
  
  int num_preset_items;
  int preset_items_alloc;

  bg_preset_t * presets;
  char * path;

  void (*load_cb)(void*);
  void (*save_cb)(void*);
  void * cb_data;

  char * preset_name;

  bg_cfg_section_t * section;
  
  };

static void menu_callback(GtkWidget * w, gpointer data);

static void set_name_parameter(void * data, const char * name,
                               const bg_parameter_value_t * val)
  {
  bg_gtk_preset_menu_t * menu = data;

  if(!name)
    return;

  if(!strcmp(name, "name"))
    menu->preset_name = bg_strdup(menu->preset_name, val->val_str);
  }

static void update_menu(bg_gtk_preset_menu_t * menu)
  {
  bg_preset_t * tmp;
  int i;
  // Count presets
  
  menu->num_preset_items = 0;
  
  tmp = menu->presets;
  while(tmp)
    {
    menu->num_preset_items++;
    tmp = tmp->next;
    }

  if(menu->num_preset_items > menu->preset_items_alloc)
    {
    // Create preset items
    int old_alloc = menu->preset_items_alloc;

    menu->preset_items_alloc = menu->num_preset_items + 4;
    menu->preset_items = realloc(menu->preset_items,
                                 menu->preset_items_alloc *
                                 sizeof(*menu->preset_items));

    for(i = old_alloc; i < menu->preset_items_alloc; i++)
      {
      menu->preset_items[i] = gtk_menu_item_new_with_label("");
      g_signal_connect(G_OBJECT(menu->preset_items[i]),
                       "activate", G_CALLBACK(menu_callback),
                       (gpointer)menu);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu),
                            menu->preset_items[i]);
      }
    }

  tmp = menu->presets;
  for(i = 0; i < menu->num_preset_items; i++)
    {
    gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(menu->preset_items[i]))), tmp->name);
    gtk_widget_show(menu->preset_items[i]);
    tmp = tmp->next;
    }
  for(i = menu->num_preset_items; i < menu->preset_items_alloc; i++)
    {
    gtk_widget_hide(menu->preset_items[i]);
    }
  }


static void menu_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_preset_menu_t * menu = data;
  
  if(w == menu->save_item)
    {
    bg_parameter_info_t pi[2];
    bg_dialog_t * dlg;
    
    if(menu->save_cb)
      menu->save_cb(menu->cb_data);
    
    memset(pi, 0, sizeof(pi));
    pi[0].name = "name";
    pi[0].long_name = TR("Name");
    pi[0].type = BG_PARAMETER_STRING;
    pi[0].val_default.val_str = TR("New preset");

    
    dlg = bg_dialog_create(NULL,
                           set_name_parameter,
                           NULL,
                           menu,
                           pi,
                           TR("Save preset"));
    if(bg_dialog_show(dlg, menu->menubar))
      {
      fprintf(stderr, "Saving preset %s\n", menu->preset_name);

      menu->presets = bg_preset_add(menu->presets,
                                    menu->path,
                                    menu->preset_name,
                                    menu->section);
      update_menu(menu);
      }
    else
      {
      fprintf(stderr, "Not saving preset %s\n", menu->preset_name);
      }
    }
  else
    {
    int index = -1, i;
    bg_preset_t * p;
    bg_cfg_section_t * preset_section;
    
    for(i = 0; i < menu->num_preset_items; i++)
      {
      if(w == menu->preset_items[i])
        {
        index = i;
        break;
        }
      }

    if(index < 0)
      return;
    
    p = menu->presets;
    
    for(i = 0; i < index; i++)
      p = p->next;

    fprintf(stderr, "Loading preset %s\n", p->name);

    preset_section = bg_preset_load(p);
    
    bg_cfg_section_transfer(preset_section, menu->section);
    if(menu->load_cb)
      menu->load_cb(menu->cb_data);

    bg_cfg_section_destroy(preset_section);
    }
  }

static GtkWidget *
create_item(bg_gtk_preset_menu_t * w, GtkWidget * parent,
            const char * label, const char * pixmap)
  {
  GtkWidget * ret, *image;
  char * path;
    
  if(pixmap)
    {
    path = bg_search_file_read("icons", pixmap);
    if(path)
      {
      image = gtk_image_new_from_file(path);
      free(path);
      }
    else
      image = gtk_image_new();
    gtk_widget_show(image);
    ret = gtk_image_menu_item_new_with_label(label);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(ret), image);
    }
  else
    {
    ret = gtk_menu_item_new_with_label(label);
    }
  
  g_signal_connect(G_OBJECT(ret), "activate", G_CALLBACK(menu_callback),
                   (gpointer)w);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), ret);
  return ret;
  }

bg_gtk_preset_menu_t *
bg_gtk_preset_menu_create(const char * preset_path,
                          bg_cfg_section_t * s,
                          void (*load_cb)(void *),
                          void (*save_cb)(void *), 
                          void * cb_data)
  {
  GtkWidget * item;
  
  bg_gtk_preset_menu_t * ret = calloc(1, sizeof(*ret));

  ret->load_cb = load_cb;
  ret->save_cb = save_cb;
  ret->cb_data = cb_data;
  
  ret->path = bg_strdup(ret->path, preset_path);
  ret->presets = bg_presets_load(ret->path);
  ret->section = s;
  ret->menu = gtk_menu_new();

  ret->save_item = create_item(ret, ret->menu, TR("Save..."),
                               "save_16.png");

  item = gtk_separator_menu_item_new();
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menu), item);
  
  
  ret->menubar = gtk_menu_bar_new();

  item = gtk_menu_item_new_with_label(TR("Preset"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), ret->menu);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menubar), item);
  gtk_widget_show(ret->menubar);

  update_menu(ret);
  
  return ret;  
  }

void bg_gtk_preset_menu_destroy(bg_gtk_preset_menu_t * m)
  {
  if(m->preset_items)
    free(m->preset_items);
  if(m->preset_name)
    free(m->preset_name);
  if(m->path)
    free(m->path);
  free(m);
  }

GtkWidget * bg_gtk_preset_menu_get_widget(bg_gtk_preset_menu_t * m)
  {
  return m->menubar;
  }
