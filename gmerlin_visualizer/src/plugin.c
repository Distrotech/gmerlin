/*****************************************************************

  plugin.c

  Copyright (c) 2001 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

/* This file is part of gmerlin_vizualizer */

#include <gtk/gtk.h>
#include <xmms/plugin.h>
#include <dlfcn.h>
#include <mainwindow.h>
#include <stdio.h>

VisPlugin * load_vis_plugin(char * filename)
  {
  void * plugin_handle;
  void *(*gpi) (void);

  VisPlugin * ret;
  
  plugin_handle = dlopen(filename, RTLD_NOW);
  if(!plugin_handle)
    {
    fprintf(stderr, "%s\n", dlerror());
    return (VisPlugin*)0;
    }
  gpi = dlsym(plugin_handle, "get_vplugin_info");

  if(!gpi)
    {
    dlclose(plugin_handle);
    return (VisPlugin*)0;
    }
  ret = (VisPlugin*)gpi(); 

  ret->handle = plugin_handle;
  ret->filename = g_strdup(filename);
  
  return ret;
  }

void load_plugin(xesd_plugin_info * info)
  {
  if(info->plugin)
    return;
  info->plugin = load_vis_plugin(info->filename);
  }

void unload_plugin(xesd_plugin_info * info)
  {
  g_free(info->plugin->filename);
  dlclose(info->plugin->handle);
  info->plugin = NULL;
  }
