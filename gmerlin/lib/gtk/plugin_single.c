/*****************************************************************
 
  plugin_single.c
 
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>

#include <pluginregistry.h>
#include <utils.h>
#include <cfg_dialog.h>

#include <gui_gtk/plugin.h>

#if GTK_MINOR_VERSION >= 4
#define GTK_2_4
#endif


struct bg_gtk_plugin_widget_single_s
  {
  GtkWidget * label;
  GtkWidget * combo;
  GtkWidget * config_button;
  GtkWidget * info_button;
  GtkWidget * audio_button;
  GtkWidget * video_button;
    
  bg_plugin_registry_t * reg;
  const bg_plugin_info_t * info;
  bg_plugin_handle_t * handle;

  bg_cfg_section_t * section;
  bg_cfg_section_t * audio_section;
  bg_cfg_section_t * video_section;

  int32_t type_mask;
  int32_t flag_mask;
  void (*set_plugin)(bg_plugin_handle_t *, void*);
  void * set_plugin_data;
  };

static void set_parameter(void * data, char * name,
                          bg_parameter_value_t * v)
  {
  bg_gtk_plugin_widget_single_t * widget;
  widget = (bg_gtk_plugin_widget_single_t *)data;

  if(widget->handle->plugin->set_parameter)
    {
    bg_plugin_lock(widget->handle);
    widget->handle->plugin->set_parameter(widget->handle->priv, name, v);
    bg_plugin_unlock(widget->handle);
    }
  }


static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_encoder_plugin_t * encoder;

  bg_gtk_plugin_widget_single_t * widget;
  bg_parameter_info_t * parameters;
  
  bg_dialog_t * dialog;
  
  widget = (bg_gtk_plugin_widget_single_t *)data;
  
  if(w == widget->info_button)
    {
    bg_gtk_plugin_info_show(widget->info);
    }
  else if(w == widget->config_button)
    {
    bg_plugin_lock(widget->handle);
    parameters = widget->handle->plugin->get_parameters(widget->handle->priv);
    bg_plugin_unlock(widget->handle);
    
    dialog = bg_dialog_create(widget->section,
                              set_parameter,
                              (void*)widget,
                              parameters,
                              widget->handle->info->long_name);
    bg_dialog_show(dialog);
    bg_dialog_destroy(dialog);
    }

  else if(w == widget->audio_button)
    {
    encoder = (bg_encoder_plugin_t*)(widget->handle->plugin);

    bg_plugin_lock(widget->handle);
    parameters = encoder->get_audio_parameters(widget->handle->priv);
    bg_plugin_unlock(widget->handle);
    
    dialog = bg_dialog_create(widget->audio_section,
                              NULL, NULL,
                              parameters,
                              widget->handle->info->long_name);
    bg_dialog_show(dialog);
    bg_dialog_destroy(dialog);
    }

  else if(w == widget->video_button)
    {
    encoder = (bg_encoder_plugin_t*)(widget->handle->plugin);

    bg_plugin_lock(widget->handle);
    parameters = encoder->get_video_parameters(widget->handle->priv);
    bg_plugin_unlock(widget->handle);
    
    dialog = bg_dialog_create(widget->video_section,
                              NULL, NULL,
                              parameters,
                              widget->handle->info->long_name);
    bg_dialog_show(dialog);
    bg_dialog_destroy(dialog);
    }
  
  }


static GtkWidget * create_pixmap_button(bg_gtk_plugin_widget_single_t * w,
                                        const char * filename,
                                        GtkTooltips * tooltips,
                                        const char * tooltip,
                                        const char * tooltip_private)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", filename);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();

  gtk_widget_show(image);
  button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);

  gtk_tooltips_set_tip(tooltips, button, tooltip, tooltip_private);

  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(button_callback), (gpointer)w);
  
  gtk_widget_show(button);
  return button;
  }

static void update_sensitive(bg_gtk_plugin_widget_single_t * widget)
  {
  bg_encoder_plugin_t * encoder;

  if(widget->handle->plugin->get_parameters)
    gtk_widget_set_sensitive(widget->config_button, 1);
  else
    gtk_widget_set_sensitive(widget->config_button, 0);

  if(widget->info->type & (BG_PLUGIN_ENCODER_AUDIO|BG_PLUGIN_ENCODER_VIDEO|BG_PLUGIN_ENCODER))
    {
    encoder = (bg_encoder_plugin_t*)(widget->handle->plugin);

    if(widget->audio_button)
      {
      if(encoder->get_audio_parameters)
        gtk_widget_set_sensitive(widget->audio_button, 1);
      else
        gtk_widget_set_sensitive(widget->audio_button, 0);
      }
    
    if(widget->video_button)
      {
      if(encoder->get_video_parameters)
        gtk_widget_set_sensitive(widget->video_button, 1);
      else
        gtk_widget_set_sensitive(widget->video_button, 0);
      }
    }

  }

static void change_callback(GtkWidget * w, gpointer data)
  {
  bg_encoder_plugin_t * encoder;

#ifndef GTK_2_4
  const char * long_name;
#endif
  
  bg_gtk_plugin_widget_single_t * widget;
  widget = (bg_gtk_plugin_widget_single_t *)data;

#ifdef GTK_2_4
  widget->info = bg_plugin_find_by_index(widget->reg,
                                         gtk_combo_box_get_active(GTK_COMBO_BOX(widget->combo)),
                                         widget->type_mask, widget->flag_mask);
#else
  long_name = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget->combo)->entry));
  //  fprintf(stderr, "Change callback %s\n", long_name);
  if(*long_name == '\0')
    return;

  widget->info = bg_plugin_find_by_long_name(widget->reg, long_name);
  
#endif

  if(widget->info)
    bg_plugin_registry_set_default(widget->reg, widget->type_mask, widget->info->name);
  
  if(widget->handle)
    bg_plugin_unref(widget->handle);
  
  widget->handle = bg_plugin_load(widget->reg, widget->info);
    
  update_sensitive(widget);
  
  widget->section = bg_plugin_registry_get_section(widget->reg,
                                                   widget->info->name);

  if(widget->info->type & (BG_PLUGIN_ENCODER_AUDIO|BG_PLUGIN_ENCODER_VIDEO|BG_PLUGIN_ENCODER))
    {
    encoder = (bg_encoder_plugin_t*)(widget->handle->plugin);
    if(encoder->get_audio_parameters)
      widget->audio_section = bg_cfg_section_find_subsection(widget->section, "$audio");
    else
      widget->audio_section = (bg_cfg_section_t*)0;

    if(encoder->get_video_parameters)
      widget->video_section = bg_cfg_section_find_subsection(widget->section, "$video");
    else
      widget->video_section = (bg_cfg_section_t*)0;
    
    }
  
  
  if(widget->set_plugin)
    widget->set_plugin(widget->handle, widget->set_plugin_data);
  
  }

bg_gtk_plugin_widget_single_t *
bg_gtk_plugin_widget_single_create(char * label,
                                   bg_plugin_registry_t * reg,
                                   uint32_t type_mask,
                                   uint32_t flag_mask,
                                   void (*set_plugin)(bg_plugin_handle_t * plugin,
                                                     void * data),
                                   void * set_plugin_data,
                                   GtkTooltips * tooltips)
  {
#ifdef GTK_2_4
  int default_index;
#else
  GList * strings;
  char * tmp_string;
#endif
  
  int num_plugins, i;
  bg_gtk_plugin_widget_single_t * ret;
  const bg_plugin_info_t * info;
  const bg_plugin_info_t * default_info;
  
  ret = calloc(1, sizeof(*ret));

  ret->set_plugin = set_plugin;
  ret->set_plugin_data = set_plugin_data;
  
  ret->reg = reg;
  ret->type_mask = type_mask;
  ret->flag_mask = flag_mask;

  /* Make label */

  ret->label = gtk_label_new(label);
  gtk_widget_show(ret->label);
    
  /* Make buttons */

  /* Config */
  
  ret->config_button = create_pixmap_button(ret, "config_16.png", tooltips,
                                            "Plugin options", "Plugin options");
  
  /* Info */
    
  ret->info_button = create_pixmap_button(ret, "info_16.png", tooltips,
                                          "Plugin info", "Plugin info");
  
  /* Audio */

  if(type_mask & (BG_PLUGIN_ENCODER_AUDIO | BG_PLUGIN_ENCODER))
    {
    ret->audio_button = create_pixmap_button(ret, "audio_16.png", tooltips,
                                            "Audio options", "Audio options");
    }

  /* Video */
    
  if(type_mask & (BG_PLUGIN_ENCODER_VIDEO | BG_PLUGIN_ENCODER))
    {
    ret->video_button = create_pixmap_button(ret, "video_16.png", tooltips,
                                            "Video options", "Video options");
    }
    
  /* Create combo */
    
  num_plugins = bg_plugin_registry_get_num_plugins(reg,
                                                   type_mask,
                                                   flag_mask);

  default_info = bg_plugin_registry_get_default(reg, type_mask);

  /* Make combo */
#ifdef GTK_2_4
  default_index = -1;
  ret->combo = gtk_combo_box_new_text();
  g_signal_connect(G_OBJECT(ret->combo),
                   "changed", G_CALLBACK(change_callback),
                   (gpointer)ret);
  for(i = 0; i < num_plugins; i++)
    {
    info = bg_plugin_find_by_index(reg, i, type_mask, flag_mask);
    gtk_combo_box_append_text(GTK_COMBO_BOX(ret->combo), info->long_name);

    if(info == default_info)
      default_index = i;
    }
  if(default_index >= 0)
    gtk_combo_box_set_active(GTK_COMBO_BOX(ret->combo), default_index);
#else
  
  ret->combo = gtk_combo_new();
  gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(ret->combo)->entry),
                            FALSE);
  g_signal_connect(G_OBJECT(GTK_EDITABLE(GTK_COMBO(ret->combo)->entry)),
                   "changed", G_CALLBACK(change_callback),
                   (gpointer)ret);
  
  strings = (GList*)0;
  
  
  for(i = 0; i < num_plugins; i++)
    {
    info = bg_plugin_find_by_index(reg, i, type_mask, flag_mask);
    tmp_string = g_strdup(info->long_name);
    strings = g_list_append(strings, tmp_string);
    }

  gtk_combo_set_popdown_strings(GTK_COMBO(ret->combo), strings);
  /* Get default plugin */

  if(default_info)
    {
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ret->combo)->entry),
                       default_info->long_name);
    }
#endif

  /* Show */
  
  gtk_widget_show(ret->combo);
  


  return ret;
  }

void bg_gtk_plugin_widget_single_destroy(bg_gtk_plugin_widget_single_t * w)
  {
  if(w->handle)
    bg_plugin_unref(w->handle);
  //  fprintf(stderr, "bg_gtk_plugin_widget_single_destroy\n");
  free(w);
  }

void bg_gtk_plugin_widget_single_attach(bg_gtk_plugin_widget_single_t * w,
                                        GtkWidget * table,
                                        int * row, int * num_columns)
  {
  int columns_needed = 4;
  int col;
    
  if(w->audio_button)
    columns_needed++;
  if(w->video_button)
    columns_needed++;
  if(*num_columns < columns_needed)
    *num_columns = columns_needed;

  gtk_table_resize(GTK_TABLE(table), *row+1, *num_columns);
  
  gtk_table_attach(GTK_TABLE(table), w->label,
                   0, 1, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table),
                   w->combo,
                   1, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table),
                   w->info_button,
                   2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table),
                   w->config_button,
                   3, 4, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);

  col = 4;
  if(w->audio_button)
    {
    gtk_table_attach(GTK_TABLE(table),
                     w->audio_button,
                     col, col+1, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
    col++;
    }
  if(w->video_button)
    {
    gtk_table_attach(GTK_TABLE(table),
                     w->video_button,
                     col, col+1, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
    }
  
  (*row)++;
  }

void bg_gtk_plugin_widget_single_set_sensitive(bg_gtk_plugin_widget_single_t * w,
                                               int sensitive)
  {
  gtk_widget_set_sensitive(w->combo, sensitive);
  gtk_widget_set_sensitive(w->info_button, sensitive);
  gtk_widget_set_sensitive(w->config_button, sensitive);
  if(w->audio_button)
    gtk_widget_set_sensitive(w->audio_button, sensitive);
  if(w->video_button)
    gtk_widget_set_sensitive(w->video_button, sensitive);

  if(sensitive)
    update_sensitive(w);
  
  }

#if 0
GtkWidget *
bg_gtk_plugin_widget_single_get_widget(bg_gtk_plugin_widget_single_t * w)
  {
  return w->table;
  }
#endif

bg_plugin_handle_t *
bg_gtk_plugin_widget_single_get_plugin(bg_gtk_plugin_widget_single_t * w)
  {
  return w->handle;
  }
