/*****************************************************************
 
  plugininfo.c
 
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
#include <string.h>

#include <pluginregistry.h>
#include <gui_gtk/plugin.h>

struct bg_gtk_plugin_info_s
  {
  GtkWidget * window;
  GtkWidget * close_button;
  };

static struct
  {
  char * name;
  bg_plugin_type_t type;
  }
type_names[] =
  {
    { "Input",          BG_PLUGIN_INPUT },
    { "Audio output",   BG_PLUGIN_OUTPUT_AUDIO },
    { "Video output",   BG_PLUGIN_OUTPUT_VIDEO },
    { "Audio recorder", BG_PLUGIN_RECORDER_AUDIO },
    { "Video recorder", BG_PLUGIN_RECORDER_VIDEO },
    { "Redirector",     BG_PLUGIN_REDIRECTOR    },
    { (char*)0,         BG_PLUGIN_NONE }
  };

static struct
  {
  char * name;
  uint32_t flag;
  }
flag_names[] =
  {
    { "Removable Device", BG_PLUGIN_REMOVABLE }, /* Removable media (CD, DVD etc.) */
    { "Recorder",  BG_PLUGIN_RECORDER  }, /* Plugin can record              */
    { "File",      BG_PLUGIN_FILE      }, /* Plugin reads/writes files      */
    { "URL",       BG_PLUGIN_URL       }, /* Plugin reads URLs or streams   */
    { "Playback",  BG_PLUGIN_PLAYBACK  }, /* Output plugins for playback    */
    { (char*)0,    0                   },
  };

static char * get_flag_string(uint32_t flags)
  {
  char * ret;
  int i, j, index, num_flags;
  uint32_t flag;

  ret = malloc(1024);
  *ret = '\0';
  
  /* Count the flags */
    
  num_flags = 0;
  for(i = 0; i < 32; i++)
    {
    flag = (1<<i);
    if(flags & flag)
      num_flags++;
    }

  /* Create the string */
  
  index = 0;
  
  for(i = 0; i < 32; i++)
    {
    flag = (1<<i);
    if(flags & flag)
      {
      j = 0;
      while(flag_names[j].name)
        {
        if(flag_names[j].flag == flag)
          {
          strcat(ret, flag_names[j].name);
          if(index < num_flags - 1)
            strcat(ret, ", ");
          index++;
          break;
          }
        j++;
        }
      }
    }
  return ret;
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_plugin_info_t * win;
  win = (bg_gtk_plugin_info_t*)data;

  gtk_widget_hide(win->window);
  gtk_widget_destroy(win->window);
  free(win);
  } 

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  } 

static bg_gtk_plugin_info_t *
bg_gtk_plugin_info_create(const bg_plugin_info_t * info)
  {
  GtkWidget * label;
  GtkWidget * table;
  int index;
  char * tmp_string;
  
  bg_gtk_plugin_info_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);

  gtk_window_set_title(GTK_WINDOW(ret->window), info->long_name);

  /* Create close button */

  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);

  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                   G_CALLBACK(button_callback), (gpointer)ret);
  gtk_widget_show(ret->close_button);
    
  table = gtk_table_new(6, 2, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  
  /* Create labels */

  label = gtk_label_new("Name:");
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

  label = gtk_label_new(info->name);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 0, 1);
  
  label = gtk_label_new("Long Name:");
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

  label = gtk_label_new(info->long_name);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 1, 2);
  
  label = gtk_label_new("Type:");
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);

  index = 0;
  while(type_names[index].name)
    {
    if(type_names[index].type == info->type)
      {
      label = gtk_label_new(type_names[index].name);
      gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
      gtk_widget_show(label);
      gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 2, 3);
      }
    index++;
    }
  
  label = gtk_label_new("Flags:");
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);

  tmp_string = get_flag_string(info->flags);
  
  label = gtk_label_new(tmp_string);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 3, 4);
  free(tmp_string);
  
  
  label = gtk_label_new("DLL Filename:");
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 4, 5);
  
  label = gtk_label_new(info->module_filename);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 4, 5);

  gtk_table_attach(GTK_TABLE(table), ret->close_button, 0, 2, 5, 6,
                   GTK_SHRINK, GTK_SHRINK, 0, 0);
  
  
  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(ret->window), table);
    
  return ret;
  }


void bg_gtk_plugin_info_show(const bg_plugin_info_t * i)
  {
  bg_gtk_plugin_info_t * info;
  info = bg_gtk_plugin_info_create(i);
  
  gtk_widget_show(info->window);
  }


