/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <config.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>
#include <gmerlin/ocr.h>

#include <stdio.h>
#include <string.h>

int main(int argc, char ** argv)
  {
  char * tmp_path;
  bg_cfg_registry_t * cfg_reg = NULL;
  bg_cfg_section_t * cfg_section;
  bg_plugin_registry_t * plugin_reg = NULL;

  gavl_video_format_t format;
  gavl_video_frame_t * frame = NULL;
  bg_ocr_t * ocr = NULL;
  char * ret = NULL;
  bg_parameter_value_t val;
  
  /* Create config registry */
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  /* Get the config section for the plugins */
  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");

  /* Create plugin registry */
  plugin_reg = bg_plugin_registry_create(cfg_section);

  /* Create ocr */
  ocr = bg_ocr_create(plugin_reg);
  val.val_str = ".";
  bg_ocr_set_parameter(ocr, "tmpdir", &val);
  if(!ocr)
    goto fail;
  
  /* Load image */
  memset(&format, 0, sizeof(format));
  frame = bg_plugin_registry_load_image(plugin_reg, argv[1],
                                        &format, NULL);

  if(!bg_ocr_init(ocr, &format, "deu"))
    return -1;

  if(!bg_ocr_run(ocr, &format, frame, &ret))
    return -1;

  fprintf(stderr, "Got string: %s\n", ret);

  fail:

  if(ocr)
    bg_ocr_destroy(ocr);
  if(frame)
    gavl_video_frame_destroy(frame);

  if(plugin_reg)
    bg_plugin_registry_destroy(plugin_reg);
  if(cfg_reg)
    bg_cfg_registry_destroy(cfg_reg);

  if(ret)
    free(ret);
  
  return 0;
  }
