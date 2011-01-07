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

#include <stdlib.h>
#include <string.h>

#include <gmerlin/parameter.h>
#include <gmerlin/textrenderer.h>
#include <gmerlin/utils.h>
#include <gmerlin/pluginregistry.h>

int main(int argc, char ** argv)
  {
  gavl_video_format_t frame_format;
  gavl_video_format_t ovl_format;
  char * tmp_path;
  bg_cfg_registry_t    * cfg_reg;
  bg_cfg_section_t     * cfg_section;
  bg_plugin_registry_t * plugin_reg;
  
  gavl_overlay_t ovl;
  
  bg_parameter_value_t val;
  bg_text_renderer_t * r;

  if(argc != 3)
    {
    fprintf(stderr, "usage: %s <font-name> <string>\n", argv[0]);
    return 1;
    }

  /* Create */
  r = bg_text_renderer_create();

  /* Set parameters */
  val.val_color[0] = 1.0;
  val.val_color[1] = 0.0;
  val.val_color[2] = 0.0;
  val.val_color[3] = 0.5;
  bg_text_renderer_set_parameter(r, "color", &val);

  val.val_color[0] = 0.0;
  val.val_color[1] = 0.0;
  val.val_color[2] = 1.0;
  val.val_color[3] = 1.0;
  bg_text_renderer_set_parameter(r, "border_color", &val);

  /* Border width */
  val.val_f = 5.0;
  bg_text_renderer_set_parameter(r, "border_width", &val);
  
  
  /* Font */
  val.val_str = argv[1];
  bg_text_renderer_set_parameter(r, "font", &val);

  /* Cache size */
  
  val.val_i = 255;
  bg_text_renderer_set_parameter(r, "cache_size", &val);
  /* Initialize */

  memset(&frame_format, 0, sizeof(frame_format));
  frame_format.image_width  = 640;
  frame_format.image_height = 480;
  frame_format.frame_width  = 640;
  frame_format.frame_height = 480;
  frame_format.pixel_width  = 1;
  frame_format.pixel_height = 1;
  frame_format.pixelformat =  GAVL_RGB_FLOAT;
  //  frame_format.pixelformat =  GAVL_YUV_444_P;

  
  bg_text_renderer_init(r, &frame_format, &ovl_format);

  ovl.frame = gavl_video_frame_create(&ovl_format);
  
  /* Render */

  bg_text_renderer_render(r, argv[2], &ovl);
  
  /* Save png */

  cfg_reg = bg_cfg_registry_create();
    
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  bg_plugin_registry_save_image(plugin_reg, "text.png", ovl.frame,
                                &ovl_format, NULL);
  
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);

  gavl_video_frame_destroy(ovl.frame);
  
  /* Cleanup */
    
  bg_text_renderer_destroy(r);

  return 0;
  }
