/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 * 
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gmerlin/utils.h>
#include <gmerlin/pluginregistry.h>

int main(int argc, char ** argv)
  {
  bg_cfg_registry_t    * cfg_reg;
  bg_plugin_registry_t * plugin_reg;

  char * tmp_path;
  bg_cfg_section_t * cfg_section;

  char * th_filename;
  gavl_video_format_t th_format;
  gavl_video_frame_t * th_frame;

  memset(&th_format, 0, sizeof(th_format));
  
  /* Create registries */
  cfg_reg = bg_cfg_registry_create();

  tmp_path = bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  /* Get thumbnail */
  
  if(bg_get_thumbnail(argv[1],
                      plugin_reg,
                      &th_filename,
                      &th_frame,
                      &th_format))
    {
    fprintf(stderr, "Thumbnail file: %s\n", th_filename);
    fprintf(stderr, "Thumbnail format:\n");
    gavl_video_format_dump(&th_format);
    fprintf(stderr, "Thumbnail frame: %p\n", th_frame);
    gavl_video_frame_destroy(th_frame);
    free(th_filename);
    }

  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  
  return 0;
  }


