/*****************************************************************
 
  loader.c
 
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
#include <stdlib.h>

#include <dlfcn.h>

#include <pluginregistry.h>

bg_plugin_handle_t * bg_plugin_load(bg_plugin_registry_t * reg,
                                    const bg_plugin_info_t * info)
  {
  bg_plugin_handle_t * ret;
  bg_parameter_info_t * parameters;
  bg_cfg_section_t * section;
  
  ret = calloc(1, sizeof(*ret));
  pthread_mutex_init(&(ret->mutex),(pthread_mutexattr_t *)0);

  /* We need all symbols global because some plugins might reference them */
  
  ret->dll_handle = dlopen(info->module_filename, RTLD_NOW | RTLD_GLOBAL);
  if(!ret->dll_handle)
    {
    fprintf(stderr, "Cannot dlopen plugin %s: %s\n", info->module_filename,
            dlerror());
    goto fail;
    }
  
  ret->plugin = dlsym(ret->dll_handle, "the_plugin");
  if(!ret->plugin)
    goto fail;

  ret->priv = ret->plugin->create();
  ret->info = info;

  /* Apply saved parameters */

  if(ret->plugin->get_parameters)
    {
    parameters = ret->plugin->get_parameters(ret->priv);
    
    section = bg_plugin_registry_get_section(reg, info->name);
    
    bg_cfg_section_apply(section, parameters, ret->plugin->set_parameter,
                         ret->priv);
    }
  bg_plugin_ref(ret);
  return ret;

fail:
  pthread_mutex_destroy(&(ret->mutex));
  if(ret->dll_handle)
    dlclose(ret->dll_handle);
  free(ret);
  return (bg_plugin_handle_t*)0;
  }

bg_plugin_handle_t * bg_plugin_copy(bg_plugin_registry_t * plugin_reg,
                                    bg_plugin_handle_t * h)
  {
  bg_parameter_info_t * parameters;
  bg_cfg_section_t * section;
  bg_plugin_handle_t * ret;

  ret = calloc(1, sizeof(*ret));
  
  memcpy(ret, h, sizeof(*ret));

  ret->priv = ret->plugin->create();

  /* Apply saved parameters */

  if(ret->plugin->get_parameters)
    {
    parameters = ret->plugin->get_parameters(ret->priv);
    
    section = bg_plugin_registry_get_section(plugin_reg, ret->info->name);
    
    bg_cfg_section_apply(section, parameters, ret->plugin->set_parameter,
                         ret->priv);
    }
  ret->refcount = 0;
  bg_plugin_ref(ret);
  return ret;
  }


void bg_plugin_lock(bg_plugin_handle_t * h)
  {
  pthread_mutex_lock(&(h->mutex));
  }

void bg_plugin_unlock(bg_plugin_handle_t * h)
  {
  pthread_mutex_unlock(&(h->mutex));
  }
