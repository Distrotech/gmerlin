#include <stdio.h>
#include <stdlib.h>
#include <utils.h>
#include <plugin.h>
#include <pluginregistry.h>

int keep_going = 1;



int main(int argc, char ** argv)
  {
  char * tmp_path;
  bg_cfg_section_t     * section;
  bg_cfg_registry_t    * cfg_reg;
  bg_plugin_registry_t * plugin_reg;
  gavl_video_converter_t * cnv;
  gavl_video_options_t opt;
  
  int do_convert;

  gavl_video_format_t input_format;
  gavl_video_format_t output_format;
  
  bg_plugin_handle_t * input_handle;
  bg_plugin_handle_t * output_handle;

  bg_rv_plugin_t     * input;
  bg_ov_plugin_t     * output;
  
  const bg_plugin_info_t * info;
  gavl_video_frame_t * frame = (gavl_video_frame_t*)0;
  gavl_video_frame_t * input_frame = (gavl_video_frame_t*)0;

  /* Create config registry */
  
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  /* Create plugin registry */

  section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(section);

  /* Load v4l plugin */

  info = bg_plugin_find_by_name(plugin_reg, "i_v4l");

  if(!info)
    {
    fprintf(stderr, "Video4linux plugin not installed\n");
    return -1;
    }

  input_handle = bg_plugin_load(plugin_reg, info);

  if(!input_handle)
    {
    fprintf(stderr, "Video4linux plugin could not be loaded\n");
    return -1;
    }

  input = (bg_rv_plugin_t*)input_handle->plugin;

  /* Load output plugin */
  
  info = bg_plugin_registry_get_default(plugin_reg,
                                        BG_PLUGIN_OUTPUT_VIDEO);
  
  if(!info)
    {
    fprintf(stderr, "No video output plugin found\n");
    return -1;
    }
  
  output_handle = bg_plugin_load(plugin_reg, info);

  if(!output_handle)
    {
    fprintf(stderr, "Video output plugin could not be loaded\n");
    return -1;
    }

  output = (bg_ov_plugin_t*)output_handle->plugin;

  /* Open input */

  if(!input->open(input_handle->priv, &input_format))
    {
    fprintf(stderr, "Opening video device fauled\n");
    return -1;
    }

  fprintf(stderr, "Opened input device, format:\n");
  gavl_video_format_dump(&input_format);
  
  gavl_video_format_copy(&output_format, &input_format);
    
  /* Open output */

  if(!output->open(output_handle->priv, &output_format, "Webcam"))
    {
    fprintf(stderr, "Opening video device fauled\n");
    return -1;
    }
  if(output->show_window)
    output->show_window(output_handle->priv, 1);
  
  fprintf(stderr, "Opened output plugin, format:\n");
  gavl_video_format_dump(&output_format);
  
  /* Fire up video converter */

  cnv = gavl_video_converter_create();
  gavl_video_default_options(&opt);

  do_convert = gavl_video_converter_init(cnv, &opt, &input_format, &output_format);

  /* Allocate video image */

  if(output->alloc_frame)
    frame = output->alloc_frame(output_handle->priv);
    
  if(do_convert)
    {
    input_frame = gavl_video_frame_create(&input_format);
    }

  while(keep_going)
    {
    if(do_convert)
      {
      if(!input->read_frame(input_handle->priv, input_frame))
        {
        fprintf(stderr, "read_frame failed\n");
        return -1;
        }
      gavl_video_convert(cnv, input_frame, frame);
      }
    else
      {
      //      fprintf(stderr, "Get frame\n");
      input->read_frame(input_handle->priv, frame);
      }
    //    fprintf(stderr, "done\nPut frame...");
    output->put_video(output_handle->priv, frame);
    //    fprintf(stderr, "done\n");
    }
  return 0;
  }
