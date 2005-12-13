#include <string.h>

#include <pluginregistry.h>
#include <utils.h>
#include <bggavl.h>
#include <cmdline.h>

bg_parameter_info_t conversion_parameters[] =
  {
    BG_GAVL_PARAM_CONVERSION_QUALITY,
    BG_GAVL_PARAM_CROP,
    BG_GAVL_PARAM_SCALE_MODE,
    BG_GAVL_PARAM_FRAME_SIZE,
    BG_GAVL_PARAM_DEINTERLACE,
    { /* End of parameters */ }
  };

bg_cfg_section_t * conversion_section = (bg_cfg_section_t *)0;
bg_gavl_video_options_t opt;

static void set_video_parameter(void * data, char * name,
                                bg_parameter_value_t * v)
  {
  if(!name)
    return;
  bg_gavl_video_set_parameter(data, name, v);
  }


static void opt_video_options(void * data, int * argc, char *** argv, int arg)
  {
  fprintf(stderr, "opt_video_options\n");

  if(arg >= *argc)
    {
    fprintf(stderr, "Option -co requires an argument\n");
    exit(-1);
    }

  /* Parse the option string */
  if(!bg_cmdline_apply_options(conversion_section,
                               set_video_parameter,
                               &opt,
                               conversion_parameters,
                               (*argv)[arg]))
    {
    fprintf(stderr, "Error parsing option string %s\n", (*argv)[arg]);
    exit(-1);
    }
     
  bg_cmdline_remove_arg(argc, argv, arg);
  }

static void opt_help(void * data, int * argc, char *** argv, int arg);

static bg_cmdline_arg_t global_options[] =
  {
    {
      arg:         "-co",
      help_arg:    "<options>",
      help_string: "Conversion options",
      callback:    opt_video_options,
      parameters:  conversion_parameters,
    },
    {
      arg:         "-help",
      help_string: "Print this help",
      callback:    opt_help,
    },
    { /* End of options */ }
  };

static void opt_help(void * data, int * argc, char *** argv, int arg)
  {
  fprintf(stderr, "Usage: %s [-co <options>] <input_image> <output_image>\n\n", (*argv)[0]);
  fprintf(stderr, "Options:\n\n");
  bg_cmdline_print_help(global_options);
  //  fprintf(stderr, "\ncommand is of the following:\n\n");
  //  bg_cmdline_print_help(commands);
  exit(0);
  }

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
  char ** files;
  int num_conversions;
  
  conversion_section =
    bg_cfg_section_create_from_parameters("conversion",
                                          conversion_parameters);
  
  memset(&in_format, 0, sizeof(in_format));
  memset(&out_format, 0, sizeof(out_format));

  cnv = gavl_video_converter_create();
  /* Handle options */
  opt.opt = gavl_video_converter_get_options(cnv);
  bg_gavl_video_options_init(&opt);

  /* Parse options */

  bg_cmdline_parse(global_options, &argc, &argv, NULL);
  
  files = bg_cmdline_get_locations_from_args(&argc, &argv);

  if(!files || !files[0] || !files[1] || files[2])
    opt_help(NULL, &argc, &argv, 0);
  
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
                                           files[0],
                                           &in_format);
  if(!in_frame)
    {
    fprintf(stderr, "Couldn't load %s\n", files[0]);
    return -1;
    }
  
  /* Initialize output plugin */
  fprintf(stderr, "%s -> %s\n", files[0], files[1]);
  plugin_info = bg_plugin_find_by_filename(plugin_reg, files[1],
                                           BG_PLUGIN_IMAGE_WRITER);
  if(!plugin_info)
    {
    fprintf(stderr, "Cannot detect plugin for %s\n", files[1]);
    return -1;
    }
  
  output_handle = bg_plugin_load(plugin_reg, plugin_info);
  output_plugin = (bg_image_writer_plugin_t*)(output_handle->plugin);
  
  gavl_video_format_copy(&out_format, &in_format);

  bg_gavl_video_options_set_framesize(&opt, &in_format, &out_format);
  bg_gavl_video_options_set_rectangles(&opt, &in_format, &out_format);
  
  output_plugin->write_header(output_handle->priv, files[1], &out_format);

  /* For testing gavl chroma sampling */
#if 0
  if(in_format.pixelformat == GAVL_YUVJ_420_P)
    {
    in_format.pixelformat = GAVL_YUVJ_420_P;
    in_format.chroma_placement = GAVL_CHROMA_PLACEMENT_MPEG2;
    //    in_format.interlace_mode = GAVL_INTERLACE_BOTTOM_FIRST;
    }
  if(out_format.pixelformat == GAVL_YUVJ_420_P)
    {
    out_format.pixelformat = GAVL_YUVJ_420_P;
    out_format.chroma_placement = GAVL_CHROMA_PLACEMENT_MPEG2;
    //    out_format.interlace_mode = GAVL_INTERLACE_BOTTOM_FIRST;
    }
#endif
  fprintf(stderr, "Input format:\n");
  gavl_video_format_dump(&in_format);

  fprintf(stderr, "Output format:\n");
  gavl_video_format_dump(&out_format);
    
  num_conversions = gavl_video_converter_init(cnv, &in_format, &out_format);
  fprintf(stderr, "num_conversions: %d\n",num_conversions);
  
  if(num_conversions)
    {
    
    out_frame = gavl_video_frame_create(&out_format);
    gavl_video_convert(cnv, in_frame, out_frame);
    output_plugin->write_image(output_handle->priv, out_frame);
    gavl_video_frame_destroy(out_frame);
    }
  else
    output_plugin->write_image(output_handle->priv, in_frame);

  fprintf(stderr, "Wrote %s\n", files[1]);
  
  gavl_video_frame_destroy(in_frame);
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  return 0;
  }
