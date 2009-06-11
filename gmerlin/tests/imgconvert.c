/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <string.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>
#include <gmerlin/bggavl.h>
#include <gmerlin/cmdline.h>

#include <config.h>
#include <gmerlin/translation.h>

bg_parameter_info_t conversion_parameters[] =
  {
    BG_GAVL_PARAM_CONVERSION_QUALITY,
    BG_GAVL_PARAM_SCALE_MODE,
    BG_GAVL_PARAM_DEINTERLACE,
    { /* End of parameters */ }
  };

bg_cfg_section_t * conversion_section = (bg_cfg_section_t *)0;
bg_gavl_video_options_t vopt;

static void set_video_parameter(void * data, const char * name,
                                const bg_parameter_value_t * v)
  {
  if(!name)
    return;
  bg_gavl_video_set_parameter(data, name, v);
  }


static void opt_video_options(void * data, int * argc, char *** argv, int arg)
  {
  //  fprintf(stderr, "opt_video_options\n");

  if(arg >= *argc)
    {
    fprintf(stderr, "Option -co requires an argument\n");
    exit(-1);
    }

  /* Parse the option string */
  if(!bg_cmdline_apply_options(conversion_section,
                               set_video_parameter,
                               &vopt,
                               conversion_parameters,
                               (*argv)[arg]))
    {
    fprintf(stderr, "Error parsing option string %s\n", (*argv)[arg]);
    exit(-1);
    }
     
  bg_cmdline_remove_arg(argc, argv, arg);
  }

//static void opt_help(void * data, int * argc, char *** argv, int arg);

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-co",
      .help_arg =    "<options>",
      .help_string = "Conversion options",
      .callback =    opt_video_options,
      .parameters =  conversion_parameters,
    },
    { /* End of options */ }
  };

bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin_imgconvert",
    .synopsis = TRS("[-co options] input_image output_image\n"),
    .help_before = TRS("Image converter\n"),
    .args = (bg_cmdline_arg_array_t[]) { { TRS("Options"), global_options },
                                       {  } },
  };

int main(int argc, char ** argv)
  {
  gavl_video_options_t * opt;
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
  
  bg_metadata_t metadata;
  
  conversion_section =
    bg_cfg_section_create_from_parameters("conversion",
                                          conversion_parameters);
  
  memset(&in_format, 0, sizeof(in_format));
  memset(&out_format, 0, sizeof(out_format));
  memset(&metadata, 0, sizeof(metadata));
  
  cnv = gavl_video_converter_create();
  /* Handle options */
  bg_gavl_video_options_init(&vopt);

  /* Parse options */
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);
  
  files = bg_cmdline_get_locations_from_args(&argc, &argv);

  if(!files || !files[0] || !files[1] || files[2])
    bg_cmdline_print_help(argv[0], 0);
  
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
                                           &in_format, &metadata);
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

  bg_gavl_video_options_set_format(&vopt, &in_format, &out_format);
  
  output_plugin->write_header(output_handle->priv, files[1], &out_format, &metadata);

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

  bg_metadata_dump(&metadata);
  
  opt = gavl_video_converter_get_options(cnv);
  gavl_video_options_copy(opt, vopt.opt);
  
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
  bg_plugin_unref(output_handle);

  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);

  bg_gavl_video_options_free(&vopt);

  gavl_video_converter_destroy(cnv);

  bg_metadata_free(&metadata);
  
  return 0;
  }
