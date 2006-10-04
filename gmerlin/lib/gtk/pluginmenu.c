/*****************************************************************
 
  pluginmenu.c
 
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
#include <string.h>
#include <pluginregistry.h>
#include <gui_gtk/plugin.h>

#if GTK_MINOR_VERSION >= 4
#define GTK_2_4
#endif

static char * auto_string = "Auto Select";

struct bg_gtk_plugin_menu_s
  {
  int auto_supported;
  GtkWidget * box;
  GtkWidget * combo;
  GtkWidget * label;
#ifdef GTK_2_4
  char ** plugins;  
#else
  GList * plugins;
#endif
  };

bg_gtk_plugin_menu_t *
bg_gtk_plugin_menu_create(char ** plugins,
                          int auto_supported)
  {
  int index;
  GtkWidget * label;
  bg_gtk_plugin_menu_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->auto_supported = auto_supported;
#ifdef GTK_2_4
  ret->plugins = plugins;

  ret->combo = gtk_combo_box_new_text();

  if(auto_supported)
    gtk_combo_box_append_text(GTK_COMBO_BOX(ret->combo), auto_string);

  index = 0;

  if(plugins)
    {
    while(plugins[index])
      {
      gtk_combo_box_append_text(GTK_COMBO_BOX(ret->combo), plugins[index]);
      index++;
      }
    }
  /* We always take the 0th option */
  gtk_combo_box_set_active(GTK_COMBO_BOX(ret->combo), 0);
    
#else
  if(auto_supported)
    ret->plugins = g_list_append(ret->plugins, auto_string);
  
  index = 0;
  
  if(plugins)
    {
    while(plugins[index])
      {
      ret->plugins = g_list_append(ret->plugins, plugins[index]);
      index++;
      }
    }
  
  ret->combo = gtk_combo_new();
  gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(ret->combo)->entry), FALSE);

  gtk_combo_set_popdown_strings(GTK_COMBO(ret->combo), ret->plugins);
#endif
  gtk_widget_show(ret->combo);
    
  ret->box = gtk_hbox_new(0, 5);
  label = gtk_label_new("Plugin: ");
  gtk_widget_show(label);
  gtk_widget_show(ret->combo);

  
  gtk_box_pack_start(GTK_BOX(ret->box), label, FALSE, FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(ret->box), ret->combo);

  gtk_widget_show(ret->box);
  
  return ret;
  }

const char * bg_gtk_plugin_menu_get_plugin(bg_gtk_plugin_menu_t * m)
  {
  const char * ret;

#ifdef GTK_2_4
  int index;
  index = gtk_combo_box_get_active(GTK_COMBO_BOX(m->combo));
  if(m->auto_supported)
    {
    if(!index)
      ret = (const char*)0;
    else
      ret = m->plugins[index-1];
    }
  else
    ret = m->plugins[index];
  
#else
  ret = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(m->combo)->entry));

  if(!strcmp(ret, auto_string))
    ret = (const char*)0;
#endif
  return ret;
  }

GtkWidget * bg_gtk_plugin_menu_get_widget(bg_gtk_plugin_menu_t * m)
  {
  return m->box;
  }

void bg_gtk_plugin_menu_destroy(bg_gtk_plugin_menu_t * m)
  {
#ifdef GTK_2_4
  
#else
  if(m->plugins)
    {
    g_list_free(m->plugins);
    }
#endif
  }
