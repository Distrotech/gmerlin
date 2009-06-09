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

#include <config.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>
#include <gmerlin/cmdline.h>
#include <gmerlin/translation.h>

static int thumb_size = 128;

static gavl_time_t seek_time = GAVL_TIME_UNDEFINED;
static float seek_percentage = 10.0;

static void opt_size(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -s requires an argument\n");
    exit(-1);
    }
  thumb_size = atoi((*_argv)[arg]);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_time(void * data, int * argc, char *** _argv, int arg)
  {
  char * str;
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -t requires an argument\n");
    exit(-1);
    }
  str = (*_argv)[arg];
  if(str[strlen(str)-1] == '%')
    seek_percentage = strtod(str, NULL);
  else
    seek_time       = gavl_seconds_to_time(strtod(str, NULL));
  
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-s",
      .help_arg =    "<size>",
      .help_string = "Maximum width or height",
      .callback =    opt_size,
    },
    {
      .arg =         "-t",
      .help_arg =    "<time>",
      .help_string = "Time in seconds or percentage of duration (terminated with '%')",
      .callback =    opt_time,
    },
    { /* End */ }
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin-video-thumbnailer",
    .synopsis = TRS("[options] video_file thumbnail_file...\n"),
    .help_before = TRS("Video thumbnailer\n"),
    .args = (bg_cmdline_arg_array_t[]) { { TRS("Options"), global_options },
                                       {  } },
    .files = (bg_cmdline_ext_doc_t[])
    { { "~/.gmerlin/plugins.xml",
        TRS("Cache of the plugin registry (shared by all applicatons)") },
      { "~/.gmerlin/generic/config.xml",
        TRS("Default plugin parameters are read from there. Use gmerlin_plugincfg to change them.") },
      { /* End */ }
    },
  };


int main(int argc, char ** argv)
  {
  char ** files = (char **)0;
  
  char * in_file = NULL;
  char * out_file = NULL;
  
  bg_input_plugin_t        * input_plugin;
  bg_image_writer_plugin_t * output_plugin;

  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;
  bg_plugin_registry_t * plugin_reg;
  bg_track_info_t * info;
  char * tmp_path;

  const bg_plugin_info_t * plugin_info;
  
  /* Plugin handles */
  
  bg_plugin_handle_t * input_handle = NULL;
  bg_plugin_handle_t * output_handle = NULL;

  /* Formats */

  gavl_video_format_t input_format;
  gavl_video_format_t output_format;
  
  /* Frames */
  
  gavl_video_frame_t * input_frame = (gavl_video_frame_t *)0;
  gavl_video_frame_t * output_frame = (gavl_video_frame_t *)0;

  /* Converter */
  
  gavl_video_converter_t * cnv;
  int do_convert, have_frame;
  
  /* Get commandline options */
  bg_cmdline_init(&app_data);
  
  bg_cmdline_parse(global_options, &argc, &argv, NULL);
  files = bg_cmdline_get_locations_from_args(&argc, &argv);

  if(!files || !(files[0]) || !(files[1]) || files[2])
    {
    fprintf(stderr, "No input files given\n");
    return -1;
    }

  in_file = files[0];
  out_file = files[1];
  
  /* Create registries */

  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);
  
  /* Load input plugin */
  input_handle = (bg_plugin_handle_t*)0;
  if(!bg_input_plugin_load(plugin_reg,
                           in_file,
                           (const bg_plugin_info_t*)0,
                           &input_handle,
                           (bg_input_callbacks_t*)0, 0))
    {
    fprintf(stderr, "Cannot open %s\n", in_file);
    return -1;
    }
  input_plugin = (bg_input_plugin_t*)(input_handle->plugin);

  if(input_plugin->get_num_tracks &&
     !input_plugin->get_num_tracks(input_handle->priv))
    {
    fprintf(stderr, "%s has no tracks\n", in_file);
    }
  
  info = input_plugin->get_track_info(input_handle->priv, 0);
  
  /* Select track */
  if(input_plugin->set_track)
    input_plugin->set_track(input_handle->priv, 0);

  if(!info->num_video_streams)
    {
    fprintf(stderr, "File %s has no video\n", in_file);
    return -1;
    }

  /* Select first stream */
  input_plugin->set_video_stream(input_handle->priv, 0,
                                 BG_STREAM_ACTION_DECODE);
  
  /* Start playback */
  if(input_plugin->start)
    input_plugin->start(input_handle->priv);

  /* Get video format */
  memcpy(&input_format, &(info->video_streams[0].format),
         sizeof(input_format));

  /* Get the output format */
  
  gavl_video_format_copy(&output_format, &input_format);
  
  /* Scale the image to square pixels */
  
  output_format.image_width *= output_format.pixel_width;
  output_format.image_height *= output_format.pixel_height;
  
  if(output_format.image_width > input_format.image_height)
    {
    output_format.image_height = (thumb_size * output_format.image_height) /
      output_format.image_width;
    output_format.image_width = thumb_size;
    }
  else
    {
    output_format.image_width      = (thumb_size * output_format.image_width) /
      output_format.image_height;
    output_format.image_height = thumb_size;
    }
  
  output_format.pixel_width = 1;
  output_format.pixel_height = 1;
  output_format.interlace_mode = GAVL_INTERLACE_NONE;
  output_format.pixelformat = GAVL_RGBA_32;
  output_format.frame_width = output_format.image_width;
  output_format.frame_height = output_format.image_height;
  
  /* Load output plugin */
  
  //  plugin_info =
  //    bg_plugin_find_by_filename(plugin_reg, out_file, BG_PLUGIN_IMAGE_WRITER);
  plugin_info =
    bg_plugin_find_by_name(plugin_reg, "iw_png");
  
  if(!plugin_info)
    {
    fprintf(stderr, "No plugin for %s\n", out_file);
    return -1;
    }
  
  output_handle = bg_plugin_load(plugin_reg, plugin_info);

  if(!output_handle)
    {
    fprintf(stderr, "Loading %s failed\n", plugin_info->long_name);
    return -1;
    }

  output_plugin = (bg_image_writer_plugin_t*)output_handle->plugin;
  
  /* Create input frame */

  input_frame = gavl_video_frame_create(&input_format);
  
  /* Seek to the time */

  if(seek_time == GAVL_TIME_UNDEFINED)
    {
    if(info->duration == GAVL_TIME_UNDEFINED)
      {
      seek_time = 10 * GAVL_TIME_SCALE;
      }
    else
      {
      seek_time =
        (gavl_time_t)((seek_percentage / 100.0) * (double)(info->duration)+0.5);
      }
    }
  
  if(info->flags & BG_TRACK_SEEKABLE)
    {
    input_plugin->seek(input_handle->priv, &seek_time, GAVL_TIME_SCALE);
    have_frame = input_plugin->read_video(input_handle->priv, input_frame, 0);
    }
  else
    {
    while((have_frame = input_plugin->read_video(input_handle->priv, input_frame, 0)))
      {
      if(gavl_time_unscale(input_format.timescale,
                           input_frame->timestamp) >= seek_time)
        break;
      }
    }

  if(!have_frame)
    return -1;
  
  /* Initialize image writer */
  
  if(!output_plugin->write_header(output_handle->priv,
                                  out_file, &output_format))
    {
    fprintf(stderr, "Writing image header failed\n");
    return -1;
    }

  //  gavl_video_format_dump(&input_format);
  //  gavl_video_format_dump(&output_format);
  
  /* Initialize video converter */
  cnv = gavl_video_converter_create();
  do_convert = gavl_video_converter_init(cnv, &input_format, &output_format);
  
  if(do_convert)
    {
    output_frame = gavl_video_frame_create(&output_format);
    gavl_video_convert(cnv, input_frame, output_frame);
    if(!output_plugin->write_image(output_handle->priv,
                                   output_frame))
      {
      fprintf(stderr, "Writing image failed\n");
      return -1;
      }
    gavl_video_frame_destroy(output_frame);
    }
  else
    {
    if(!output_plugin->write_image(output_handle->priv,
                                   input_frame))
      {
      fprintf(stderr, "Writing image failed\n");
      return -1;
      }
    }

  bg_plugin_unref(input_handle);
  bg_plugin_unref(output_handle);

  gavl_video_frame_destroy(input_frame);

  gavl_video_converter_destroy(cnv);
  
  return 0;
  }
