#include <string.h>

#include <pluginregistry.h>
#include <utils.h>

int main(int argc, char ** argv)
  {
  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;
  bg_plugin_registry_t * plugin_reg;
  const bg_plugin_info_t * plugin_info;
  /* Plugin handles */
  bg_image_writer_plugin_t * output_plugin;
  
  bg_plugin_handle_t * output_handle = NULL;

  gavl_video_converter_t * cnv;

  gavl_video_frame_t * in_frame;
  gavl_video_frame_t * out_frame;

  gavl_video_format_t in_format;
  gavl_video_format_t out_format;
  char * tmp_path;

  memset(&in_format, 0, sizeof(in_format));
  memset(&out_format, 0, sizeof(out_format));
  
  if(argc == 1)
    {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return -1;
    }

  /* Create registries */

  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  /* Load input image */

  in_frame = bg_plugin_registry_load_image(plugin_reg,
                                           argv[1],
                                           &in_format);

  /* Initialize output plugin */

  plugin_info = bg_plugin_find_by_name(plugin_reg, "iw_jpeg");

  output_handle = bg_plugin_load(plugin_reg, plugin_info);
  output_plugin = (bg_image_writer_plugin_t*)(output_handle->plugin);
  
  gavl_video_format_copy(&out_format, &in_format);
  
  output_plugin->write_header(output_handle->priv, "out.jpg", 
                              &out_format);

  fprintf(stderr, "Input format:\n");
  gavl_video_format_dump(&in_format);
    
  cnv = gavl_video_converter_create();

  if(gavl_video_converter_init(cnv, &in_format, &out_format))
    {
    fprintf(stderr, "Output format:\n");
    gavl_video_format_dump(&out_format);
    
    out_frame = gavl_video_frame_create(&out_format);
    gavl_video_convert(cnv, in_frame, out_frame);
    output_plugin->write_image(output_handle->priv, out_frame);
    gavl_video_frame_destroy(out_frame);
    }
  else
    output_plugin->write_image(output_handle->priv, in_frame);

  fprintf(stderr, "Write out.png\n");
  
  gavl_video_frame_destroy(in_frame);
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  return 0;
  }
