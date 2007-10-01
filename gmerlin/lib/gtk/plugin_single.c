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

#include <config.h>

#include <pluginregistry.h>
#include <utils.h>
#include <cfg_dialog.h>

#include <gui_gtk/plugin.h>
#include <gui_gtk/gtkutils.h>



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
  bg_cfg_section_t * subtitle_text_section;
  bg_cfg_section_t * subtitle_overlay_section;

  int32_t type_mask;
  int32_t flag_mask;
  void (*set_plugin)(const bg_plugin_info_t *, void*);
  void * set_plugin_data;

  };

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * v)
  {
  bg_gtk_plugin_widget_single_t * widget;
  widget = (bg_gtk_plugin_widget_single_t *)data;

  if(widget->handle && widget->handle->plugin->set_parameter)
    {
    bg_plugin_lock(widget->handle);
    widget->handle->plugin->set_parameter(widget->handle->priv, name, v);
    bg_plugin_unlock(widget->handle);
    }
  }


static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_parameter_info_t * parameters;
  bg_gtk_plugin_widget_single_t * widget;
  
  bg_dialog_t * dialog;
  
  widget = (bg_gtk_plugin_widget_single_t *)data;
  
  if(w == widget->info_button)
    {
    bg_gtk_plugin_info_show(widget->info);
    }
  else if(w == widget->config_button)
    {
    if(widget->handle)
      parameters = widget->handle->plugin->get_parameters(widget->handle->priv);
    else
      parameters = widget->info->parameters;
    dialog = bg_dialog_create(widget->section,
                              set_parameter,
                              (void*)widget,
                              parameters,
                              TRD(widget->info->long_name, widget->info->gettext_domain));
    bg_dialog_show(dialog);
    bg_dialog_destroy(dialog);
    }

  else if(w == widget->audio_button)
    {
    dialog = bg_dialog_create(widget->audio_section,
                              NULL, NULL,
                              widget->info->audio_parameters,
                              TRD(widget->info->long_name, widget->info->gettext_domain));
    bg_dialog_show(dialog);
    bg_dialog_destroy(dialog);
    }

  else if(w == widget->video_button)
    {
    dialog = bg_dialog_create(widget->video_section,
                              NULL, NULL,
                              widget->info->video_parameters,
                              TRD(widget->info->long_name, widget->info->gettext_domain));
    bg_dialog_show(dialog);
    bg_dialog_destroy(dialog);
    }
  
  }


static GtkWidget * create_pixmap_button(bg_gtk_plugin_widget_single_t * w,
                                        const char * filename,
                                        const char * tooltip)
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

  bg_gtk_tooltips_set_tip(button, tooltip, PACKAGE);

  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(button_callback), (gpointer)w);
  
  gtk_widget_show(button);
  return button;
  }

static void update_sensitive(bg_gtk_plugin_widget_single_t * widget)
  {
  if(!widget->info)
    return;
  
  if(widget->info->parameters)
    gtk_widget_set_sensitive(widget->config_button, 1);
  else
    gtk_widget_set_sensitive(widget->config_button, 0);

  if(widget->info->type & (BG_PLUGIN_ENCODER_AUDIO|
                           BG_PLUGIN_ENCODER_VIDEO|
                           BG_PLUGIN_ENCODER))
    {
    if(widget->audio_button)
      {
      if(widget->info->audio_parameters)
        gtk_widget_set_sensitive(widget->audio_button, 1);
      else
        gtk_widget_set_sensitive(widget->audio_button, 0);
      }
    
    if(widget->video_button)
      {
      if(widget->info->video_parameters)
        gtk_widget_set_sensitive(widget->video_button, 1);
      else
        gtk_widget_set_sensitive(widget->video_button, 0);
      }
    }

  }

static void change_callback(GtkWidget * w, gpointer data)
  {
  
  bg_gtk_plugin_widget_single_t * widget;

  widget = (bg_gtk_plugin_widget_single_t *)data;
    
  widget->info =
    bg_plugin_find_by_index(widget->reg,
                            gtk_combo_box_get_active(GTK_COMBO_BOX(widget->combo)),
                            widget->type_mask, widget->flag_mask);

  if(widget->handle)
    {
    bg_plugin_unref(widget->handle);
    widget->handle = (bg_plugin_handle_t*)0;
    }
  update_sensitive(widget);
  
  widget->section = bg_plugin_registry_get_section(widget->reg,
                                                   widget->info->name);

  if(widget->info->type & (BG_PLUGIN_ENCODER_AUDIO|
                           BG_PLUGIN_ENCODER_VIDEO|
                           BG_PLUGIN_ENCODER_SUBTITLE_TEXT|
                           BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY|
                           BG_PLUGIN_ENCODER))
    {
    if(widget->info->audio_parameters)
      widget->audio_section =
        bg_cfg_section_find_subsection(widget->section, "$audio");
    else
      widget->audio_section = (bg_cfg_section_t*)0;

    if(widget->info->video_parameters)
      widget->video_section =
        bg_cfg_section_find_subsection(widget->section, "$video");
    else
      widget->video_section = (bg_cfg_section_t*)0;

    if(widget->info->subtitle_text_parameters)
      widget->subtitle_text_section =
        bg_cfg_section_find_subsection(widget->section, "$subtitle_text");
    else
      widget->subtitle_text_section = (bg_cfg_section_t*)0;
    
    if(widget->info->subtitle_overlay_parameters)
      widget->subtitle_overlay_section =
        bg_cfg_section_find_subsection(widget->section, "$subtitle_overlay");
    else
      widget->subtitle_overlay_section = (bg_cfg_section_t*)0;

    }
  
  if(widget->set_plugin)
    widget->set_plugin(widget->info, widget->set_plugin_data);
  
  }

void
bg_gtk_plugin_widget_single_set_change_callback(bg_gtk_plugin_widget_single_t * w,
                                                void (*set_plugin)(const bg_plugin_info_t * plugin,
                                                                   void * data),
                                                void * set_plugin_data)
  {
  w->set_plugin = set_plugin;
  w->set_plugin_data = set_plugin_data;
  
  }
                                                

bg_gtk_plugin_widget_single_t *
bg_gtk_plugin_widget_single_create(char * label,
                                   bg_plugin_registry_t * reg,
                                   uint32_t type_mask,
                                   uint32_t flag_mask)
  {
  int default_index;
  
  int num_plugins, i;
  bg_gtk_plugin_widget_single_t * ret;
  const bg_plugin_info_t * info;
  const bg_plugin_info_t * default_info;
  
  ret = calloc(1, sizeof(*ret));

  
  ret->reg = reg;
  ret->type_mask = type_mask;
  ret->flag_mask = flag_mask;

  /* Make label */

  ret->label = gtk_label_new(label);
  gtk_misc_set_alignment(GTK_MISC(ret->label), 0.0, 0.5);
  gtk_widget_show(ret->label);
    
  /* Make buttons */

  /* Config */
  
  ret->config_button = create_pixmap_button(ret, "config_16.png",
                                            TRS("Plugin options"));
  
  /* Info */
    
  ret->info_button = create_pixmap_button(ret, "info_16.png", 
                                          TRS("Plugin info"));
  
  /* Audio */

  if(type_mask & (BG_PLUGIN_ENCODER_AUDIO | BG_PLUGIN_ENCODER))
    {
    ret->audio_button = create_pixmap_button(ret, "audio_16.png", 
                                            TRS("Audio options"));
    }
  
  /* Video */
    
  if(type_mask & (BG_PLUGIN_ENCODER_VIDEO | BG_PLUGIN_ENCODER))
    {
    ret->video_button = create_pixmap_button(ret, "video_16.png", 
                                            TRS("Video options"));
    }
  
  /* Create combo */
    
  num_plugins = bg_plugin_registry_get_num_plugins(reg,
                                                   type_mask,
                                                   flag_mask);

  default_info = bg_plugin_registry_get_default(reg, type_mask);

  
  /* Make combo */
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

  /* Show */
  
  gtk_widget_show(ret->combo);
  


  return ret;
  }

void bg_gtk_plugin_widget_single_destroy(bg_gtk_plugin_widget_single_t * w)
  {
  if(w->handle)
    bg_plugin_unref(w->handle);
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

const bg_plugin_info_t *
bg_gtk_plugin_widget_single_get_plugin(bg_gtk_plugin_widget_single_t * w)
  {
  return w->info;
  }

void bg_gtk_plugin_widget_single_set_plugin(bg_gtk_plugin_widget_single_t * w,
                                            const bg_plugin_info_t * info)
  {
  int index;
  int num_plugins;
  int i;
  const bg_plugin_info_t * test_info;
  index = -1;

  num_plugins = bg_plugin_registry_get_num_plugins(w->reg,
                                                   w->type_mask,
                                                   w->flag_mask);
  
  for(i = 0; i < num_plugins; i++)
    {
    test_info = bg_plugin_find_by_index(w->reg, i, w->type_mask, w->flag_mask);
    
    if(info == test_info)
      {
      index = i;
      break;
      }
    }
  if(index >= 0)
    gtk_combo_box_set_active(GTK_COMBO_BOX(w->combo), index);
  }

bg_plugin_handle_t *
bg_gtk_plugin_widget_single_load_plugin(bg_gtk_plugin_widget_single_t * w)
  {
  if(w->handle)
    bg_plugin_unref(w->handle);
  w->handle = bg_plugin_load(w->reg, w->info);
  return w->handle;
  }

bg_cfg_section_t *
bg_gtk_plugin_widget_single_get_section(bg_gtk_plugin_widget_single_t * w)
  {
  return w->section;
  }

bg_cfg_section_t *
bg_gtk_plugin_widget_single_get_audio_section(bg_gtk_plugin_widget_single_t * w)
  {
  return w->audio_section;
  }

bg_cfg_section_t *
bg_gtk_plugin_widget_single_get_video_section(bg_gtk_plugin_widget_single_t * w)
  {
  return w->video_section;

  }

bg_cfg_section_t *
bg_gtk_plugin_widget_single_get_subtitle_text_section(bg_gtk_plugin_widget_single_t * w)
  {
  return w->subtitle_text_section;

  }

bg_cfg_section_t *
bg_gtk_plugin_widget_single_get_subtitle_overlay_section(bg_gtk_plugin_widget_single_t * w)
  {
  return w->subtitle_overlay_section;

  }


void
bg_gtk_plugin_widget_single_set_section(bg_gtk_plugin_widget_single_t * w,
                                        bg_cfg_section_t * s)
  {
  w->section = s;
  }

void
bg_gtk_plugin_widget_single_set_audio_section(bg_gtk_plugin_widget_single_t * w,
                                              bg_cfg_section_t * s)
  {
  w->audio_section = s;
  }

void
bg_gtk_plugin_widget_single_set_video_section(bg_gtk_plugin_widget_single_t * w,
                                              bg_cfg_section_t * s)
  {
  w->video_section = s;

  }

void
bg_gtk_plugin_widget_single_set_subtitle_text_section(bg_gtk_plugin_widget_single_t * w,
                                                      bg_cfg_section_t * s)
  {
  w->subtitle_text_section = s;

  }

void
bg_gtk_plugin_widget_single_set_subtitle_overlay_section(bg_gtk_plugin_widget_single_t * w,
                                                         bg_cfg_section_t * s)
  {
  w->subtitle_overlay_section = s;
  
  }

