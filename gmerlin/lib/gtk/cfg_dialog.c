/*****************************************************************
 
  cfg_dialog.c
 
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

#include <stdio.h>
#include <string.h>
#include "gtk_dialog.h"

typedef struct
  {
  bg_set_parameter_func set_param;

  void * callback_data;

  bg_gtk_widget_t * widgets;
  int num_widgets;

  bg_parameter_info_t * infos;
  bg_cfg_section_t * cfg_section;

  } dialog_section_t;

struct bg_dialog_s
  {
  GtkWidget * ok_button;
  GtkWidget * apply_button;
  GtkWidget * close_button;
  GtkWidget * window;
  GtkWidget * notebook;
  GtkWidget * mainbox;
  dialog_section_t * sections;
  int num_sections;
  };

static void apply_section(dialog_section_t * s)
  {
  int i, parameter_index;
  bg_parameter_value_t val;
  char * pos;
    
  /*  fprintf(stderr, "Apply section\n"); */
  parameter_index = 0;

  for(i = 0; i < s->num_widgets; i++)
    {
    while(s->infos[parameter_index].flags & BG_PARAMETER_HIDE_DIALOG)
      parameter_index++;
    
    s->widgets[i].funcs->set_value(&(s->widgets[i]));
    if(s->cfg_section)
      {
      bg_cfg_section_set_parameter(s->cfg_section,
                                   &(s->infos[parameter_index]),
                                   &(s->widgets[i].value));
      }

    if(s->set_param)
      {
      if((s->infos[parameter_index].type == BG_PARAMETER_DEVICE) &&
         (s->widgets[i].value.val_str) &&
         strchr(s->widgets[i].value.val_str, ':'))
        {
        val.val_str = malloc(strlen(s->widgets[i].value.val_str)+1);
        strcpy(val.val_str, s->widgets[i].value.val_str);
        pos = strchr(val.val_str, ':');
        *pos = '\0';
        s->set_param(s->callback_data, s->infos[parameter_index].name,
                     &val);
        free(val.val_str);
        }
      else
        s->set_param(s->callback_data, s->infos[parameter_index].name,
                     &(s->widgets[i].value));
      }
    parameter_index++;
    }

  s->set_param(s->callback_data, NULL, NULL);
  
  }

static void apply_values(bg_dialog_t * d)
  {
  int i;
  /*  fprintf(stderr, "Apply Section %d\n", d->num_sections); */
  for(i = 0; i < d->num_sections; i++)
    apply_section(&(d->sections[i]));
  }

static void button_callback(GtkWidget * w, gpointer * data)
  {
  bg_dialog_t * d = (bg_dialog_t *)data;
  if((w == d->close_button) ||
     (w == d->window))
    {
    gtk_widget_hide(d->window);
    gtk_main_quit();
    }
  else if(w == d->apply_button)
    {
    apply_values(d);
    }
  else if(w == d->ok_button)
    {
    gtk_widget_hide(d->window);
    gtk_main_quit();
    apply_values(d);
    }
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

bg_dialog_t * create_dialog(const char * title)
  {
  bg_dialog_t * ret;
  GtkWidget * buttonbox;
    
  ret = calloc(1, sizeof(*ret));

  ret->window       = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(ret->window), title);
    
  ret->apply_button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  ret->ok_button    = gtk_button_new_from_stock(GTK_STOCK_OK);

  gtk_window_set_modal(GTK_WINDOW(ret->window), TRUE);
  g_signal_connect(G_OBJECT(ret->ok_button), "clicked",
                   G_CALLBACK(button_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                     G_CALLBACK(button_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->apply_button), "clicked",
                     G_CALLBACK(button_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                     G_CALLBACK(delete_callback), (gpointer)ret);

  GTK_WIDGET_SET_FLAGS (ret->close_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (ret->apply_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (ret->ok_button, GTK_CAN_DEFAULT);
  
  gtk_widget_show(ret->apply_button);
  gtk_widget_show(ret->close_button);
  gtk_widget_show(ret->ok_button);
  
  /* Create the rest */
  
  ret->mainbox = gtk_vbox_new(0, 5);
  buttonbox = gtk_hbutton_box_new();
  gtk_box_set_spacing(GTK_BOX(buttonbox), 10);
  
  gtk_container_set_border_width(GTK_CONTAINER(buttonbox), 10);
  
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->ok_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->apply_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->close_button);
  gtk_widget_show(buttonbox);
  gtk_box_pack_end(GTK_BOX(ret->mainbox), buttonbox, FALSE, FALSE, 0);
  gtk_widget_show(ret->mainbox);
  gtk_container_add(GTK_CONTAINER(ret->window), ret->mainbox);  
  return ret;
  }

GtkWidget * create_section(dialog_section_t * section,
                           bg_parameter_info_t * info,
                           bg_cfg_section_t * cfg_section,
                           bg_set_parameter_func set_param,
                           void * data)
  {
  int i, count;
  int row, column, num_columns;
  GtkWidget * table;
    
  section->num_widgets = 0;
  i = 0;
  while(info[i].name &&
        (info[i].type != BG_PARAMETER_SECTION))
    {
    if(!(info[i].flags & BG_PARAMETER_HIDE_DIALOG))
      section->num_widgets++;
    i++;
    }
  section->infos = info;
  section->cfg_section = cfg_section;
  section->callback_data = data;
  section->set_param = set_param;
    
  /* Now, create and place all widgets */

  table = gtk_table_new(1, 1, 0);

  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    
  row = 0;
  column = 0;
  num_columns = 1;
    
  section->widgets = calloc(section->num_widgets, sizeof(bg_gtk_widget_t));

  i = 0;
  count = 0;

  while(count  < section->num_widgets)
    {
    if(info[i].flags & BG_PARAMETER_HIDE_DIALOG)
      {
      i++;
      continue;
      }
    
    if(info[i].flags & BG_PARAMETER_SYNC)
      {
      section->widgets[count].change_callback = set_param;
      section->widgets[count].change_callback_data = data;
      }
    section->widgets[count].info = &(info[i]);
    switch(info[i].type)
      {
      case BG_PARAMETER_CHECKBUTTON:
        bg_gtk_create_checkbutton(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_INT:
        bg_gtk_create_int(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_FLOAT:
        bg_gtk_create_float(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_SLIDER_INT:
        bg_gtk_create_slider_int(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_SLIDER_FLOAT:
        bg_gtk_create_slider_float(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_STRING:
        bg_gtk_create_string(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_STRINGLIST:
        bg_gtk_create_stringlist(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_COLOR_RGB:
        bg_gtk_create_color_rgb(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_COLOR_RGBA:
        bg_gtk_create_color_rgba(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_FILE:
        bg_gtk_create_file(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_DIRECTORY:
        bg_gtk_create_directory(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_FONT:
        bg_gtk_create_font(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_DEVICE:
        bg_gtk_create_device(&(section->widgets[count]), &(info[i]));
        break;
      case BG_PARAMETER_MULTI_MENU:
        bg_gtk_create_multi_menu(&(section->widgets[count]), &(info[i]),
                                 cfg_section, set_param, data);
        break;
      case BG_PARAMETER_MULTI_LIST:
        bg_gtk_create_multi_list(&(section->widgets[count]), &(info[i]),
                                 cfg_section, set_param, data);
        break;
      default:
        break;
      }
    section->widgets[count].funcs->attach(section->widgets[count].priv, table,
                                 &row, &num_columns);

    /* Get the value from the config data... */
    if(cfg_section)
      bg_cfg_section_get_parameter(cfg_section, &(info[i]),
                                   &(section->widgets[count].value));
    /* ... or from the parameter default */
    else
      {
      bg_parameter_value_copy(&(section->widgets[count].value),
                              &(info[i].val_default),
                              &(info[i]));
      }
    if(section->widgets[count].info->flags & BG_PARAMETER_SYNC)
      bg_gtk_change_callback_block(&section->widgets[count], 1);
    section->widgets[count].funcs->get_value(&(section->widgets[count]));    
    if(section->widgets[count].info->flags & BG_PARAMETER_SYNC)
      bg_gtk_change_callback_block(&section->widgets[count], 0);
    i++;
    count++;
    }
  gtk_widget_show(table);
  return table;
  }

static int count_sections(bg_parameter_info_t * info)
  {
  int ret = 0;
  int i = 0;

  if(!info[0].name)
    return 0;

  if(info[0].type == BG_PARAMETER_SECTION)
    {
    while(info[i].name)
      {
      if(info[i].type == BG_PARAMETER_SECTION)
        ret++;
      i++;
      }
    return ret;
    }
  return 0;
  }

bg_dialog_t * bg_dialog_create(bg_cfg_section_t * section,
                               bg_set_parameter_func set_param,
                               void * callback_data,
                               bg_parameter_info_t * info,
                               const char * title)
  {
  int i, index;
  int num_sections;
  GtkWidget * label;
  GtkWidget * table;
  
  bg_dialog_t * ret = create_dialog(title);

  num_sections = count_sections(info);
  
  if(num_sections)
    {
    ret->notebook = gtk_notebook_new();

    ret->num_sections = num_sections;
    ret->sections = calloc(ret->num_sections, sizeof(dialog_section_t));
    
    index = 0;

    for(i = 0; i < ret->num_sections; i++)
      {
      label = gtk_label_new(info[index].long_name);
      gtk_widget_show(label);
      
      while(info[index].type == BG_PARAMETER_SECTION)
        index++;
      table = create_section(&(ret->sections[i]), &(info[index]),
                             section, set_param, callback_data);
      gtk_notebook_append_page(GTK_NOTEBOOK(ret->notebook),
                               table, label);
      while(info[index].name &&
            (info[index].type != BG_PARAMETER_SECTION))
        index++;
      }
    gtk_widget_show(ret->notebook);
    gtk_box_pack_start(GTK_BOX(ret->mainbox), ret->notebook, TRUE, TRUE, 0);
    }
  else
    {
    ret->num_sections = 1;
    ret->sections = calloc(1,
                           ret->num_sections * sizeof(dialog_section_t));
    table = create_section(ret->sections, info, section, set_param, callback_data);
    gtk_box_pack_start(GTK_BOX(ret->mainbox), table, TRUE, TRUE, 0);
    }
  return ret;
  }

bg_dialog_t * bg_dialog_create_multi(const char * title)
  {
  bg_dialog_t * ret = create_dialog(title);
  ret->notebook = gtk_notebook_new();
  gtk_widget_show(ret->notebook);
  gtk_box_pack_start(GTK_BOX(ret->mainbox), ret->notebook, TRUE, TRUE, 0);
  return ret;
  }

void bg_dialog_add(bg_dialog_t *d,
                   const char * name,
                   bg_cfg_section_t * section,
                   bg_set_parameter_func set_param,
                   void * callback_data,
                   bg_parameter_info_t * info)
  {
  int num_items;
  int num_sections;
  GtkWidget * table;
  num_items = 0;
  num_sections = 0;
  GtkWidget * tab_label;
  int i, item_index, section_index;
  
  while(info[num_items+num_sections].name)
    {
    if(info[num_items+num_sections].type == BG_PARAMETER_SECTION)
      num_sections++;
    else
      num_items++;
    }
 
  if(!num_sections)
    {
    d->sections = realloc(d->sections,
                            (d->num_sections+1)*sizeof(dialog_section_t));
    table = create_section(&(d->sections[d->num_sections]),
                           info, section, set_param, callback_data);
    tab_label = gtk_label_new(name);
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(d->notebook),
                             table, tab_label);
    d->num_sections++;
    }
  else
    {
    d->sections = realloc(d->sections,
                            (d->num_sections+num_sections)*
                            sizeof(dialog_section_t));
    item_index = 0;
    section_index = d->num_sections;
    for(i = 0; i < num_sections; i++)
      {
      tab_label = gtk_label_new(info[item_index].long_name);
      gtk_widget_show(tab_label);

      item_index++;
      
      table = create_section(&(d->sections[section_index]),
                             &(info[item_index]),
                             section, set_param, callback_data);
      gtk_notebook_append_page(GTK_NOTEBOOK(d->notebook),
                               table, tab_label);

      while((info[item_index].name) &&
            (info[item_index].type != BG_PARAMETER_SECTION))
        item_index++;
      section_index++;
      }
    d->num_sections += num_sections;
    }

  }

void bg_dialog_show(bg_dialog_t * d)
  {
  gtk_widget_show(d->window);
  gtk_main();
  }

void bg_dialog_destroy(bg_dialog_t * d)
  {
  int i, j;

  for(i = 0; i < d->num_sections; i++)
    {
    for(j = 0; j < d->sections[i].num_widgets; j++)
      d->sections[i].widgets[j].funcs->destroy(&d->sections[i].widgets[j]);
    free(d->sections[i].widgets);
    }
  free(d->sections);
  gtk_widget_destroy(d->window);
  free(d);
  }

void bg_gtk_change_callback(GtkWidget * gw, gpointer data)
  {
  bg_gtk_widget_t * w = (bg_gtk_widget_t*)data;
  
  w->funcs->set_value(w);
  w->change_callback(w->change_callback_data,
                     w->info->name, &(w->value));
  }

void bg_gtk_change_callback_block(bg_gtk_widget_t * w, int block)
  {
  if(!w->callback_widget)
    {
    fprintf(stderr, "bg_gtk_change_callback_block: No callback installed\n");
    return;
    }
  if(block)
    g_signal_handler_block(w->callback_widget, w->callback_id);
  else
    g_signal_handler_unblock(w->callback_widget, w->callback_id);
  }
