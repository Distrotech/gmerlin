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

#include <config.h>
#include <translation.h>

#include <pluginregistry.h>

#include <gui_gtk/plugin.h>
#include <gui_gtk/gtkutils.h>


static char * auto_string = TRS("Auto Select");

struct bg_gtk_plugin_menu_s
  {
  int auto_supported;
  GtkWidget * box;
  GtkWidget * combo;
  GtkWidget * label;
  char ** plugins;  
  char ** plugin_names;
  };

bg_gtk_plugin_menu_t *
bg_gtk_plugin_menu_create(char ** plugins,
                          int auto_supported,
                          bg_plugin_registry_t * plugin_reg)
  {
  int index;
  GtkWidget * label;
  bg_gtk_plugin_menu_t * ret;

  const bg_plugin_info_t * plugin_info;
  
  ret = calloc(1, sizeof(*ret));
  ret->auto_supported = auto_supported;
  ret->plugin_names = plugins;

  ret->plugins = plugins;

  ret->combo = gtk_combo_box_new_text();

  if(auto_supported)
    gtk_combo_box_append_text(GTK_COMBO_BOX(ret->combo), TR(auto_string));

  index = 0;

  if(plugins)
    {
    while(plugins[index])
      {
      plugin_info = bg_plugin_find_by_name(plugin_reg, plugins[index]);
      
      bg_bindtextdomain(plugin_info->gettext_domain,
                     plugin_info->gettext_directory);
      
      gtk_combo_box_append_text(GTK_COMBO_BOX(ret->combo),
                                TRD(plugin_info->long_name, plugin_info->gettext_domain));
      index++;
      }
    }
  /* We always take the 0th option */
  gtk_combo_box_set_active(GTK_COMBO_BOX(ret->combo), 0);
    
  gtk_widget_show(ret->combo);
    
  ret->box = gtk_hbox_new(0, 5);
  label = gtk_label_new(TR("Plugin: "));
  gtk_widget_show(label);
  gtk_widget_show(ret->combo);

  
  gtk_box_pack_start(GTK_BOX(ret->box), label, FALSE, FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(ret->box), ret->combo);

  gtk_widget_show(ret->box);
  
  return ret;
  }

const char * bg_gtk_plugin_menu_get_plugin(bg_gtk_plugin_menu_t * m)
  {
  int selected;
  
  selected = gtk_combo_box_get_active(GTK_COMBO_BOX(m->combo));
  
  if(m->auto_supported)
    {
    if(!selected)
      return (const char*)0;
    else
      return m->plugin_names[selected];
    }
  else
    return m->plugin_names[selected];
  }

GtkWidget * bg_gtk_plugin_menu_get_widget(bg_gtk_plugin_menu_t * m)
  {
  return m->box;
  }

void bg_gtk_plugin_menu_destroy(bg_gtk_plugin_menu_t * m)
  {
  }
