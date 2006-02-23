#include <stdlib.h>

#include <parameter.h>
#include <textrenderer.h>
#include <utils.h>
#include <pluginregistry.h>

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

  if(argc != 2)
    {
    fprintf(stderr, "usage: %s <font-name>\n");
    return 1;
    }

  /* Create */
  r = bg_text_renderer_create();

  val.val_color = malloc(4 * sizeof(float));
  /* Set parameters */
  val.val_color[0] = 1.0;
  val.val_color[1] = 1.0;
  val.val_color[2] = 1.0;
  val.val_color[3] = 1.0;
  bg_text_renderer_set_parameter(r, "color", &val);
  free(val.val_color);
  //  val.val_str = "Sans 15";
  val.val_str = argv[1];
  bg_text_renderer_set_parameter(r, "font", &val);
  
  /* Initialize */

  frame_format.image_width  = 640;
  frame_format.image_height = 480;
  frame_format.frame_width  = 640;
  frame_format.frame_height = 480;
  frame_format.pixel_width  = 1;
  frame_format.pixel_height = 1;
  frame_format.pixelformat =  GAVL_YUV_420_P;

  bg_text_renderer_init(r, &frame_format, &ovl_format);

  ovl.frame = gavl_video_frame_create(&ovl_format);
  
  /* Render */

  bg_text_renderer_render(r, "Ein Pferd isst keinen Gurkensalat", &ovl);
  
  /* Save png */

  cfg_reg = bg_cfg_registry_create();
    
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  bg_plugin_registry_save_image(plugin_reg, "text.png", ovl.frame,
                                &ovl_format);
  
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  
  /* Cleanup */
    
  bg_text_renderer_destroy(r);

  return 0;
  }
