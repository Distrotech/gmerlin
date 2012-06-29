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
 * *****************************************************************/

#include <string.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>

static int load_file(bg_plugin_registry_t * plugin_reg,
                     bg_plugin_handle_t ** input_handle,
                     bg_input_plugin_t ** input_plugin,
                     const char * file,
                     gavl_video_format_t * format)
  {
  bg_track_info_t * ti;
  *input_handle = NULL;
  if(!bg_input_plugin_load(plugin_reg,
                           file,
                           NULL,
                           input_handle,
                           NULL, 0))
    {
    fprintf(stderr, "Cannot open %s\n", file);
    return 0;
    }
  *input_plugin = (bg_input_plugin_t*)((*input_handle)->plugin);

  ti = (*input_plugin)->get_track_info((*input_handle)->priv, 0);
  if((*input_plugin)->set_track)
    (*input_plugin)->set_track((*input_handle)->priv, 0);
  
  if(!ti->num_video_streams)
    {
    fprintf(stderr, "File %s has no video\n", file);
    return 0;
    }

  /* Select first stream */
  (*input_plugin)->set_video_stream((*input_handle)->priv, 0,
                                    BG_STREAM_ACTION_DECODE);
  
  /* Start playback */
  if((*input_plugin)->start)
    (*input_plugin)->start((*input_handle)->priv);
  
  /* Get video format */

  gavl_video_format_copy(format,
                         &ti->video_streams[0].format);
  
  return 1;
  }

int main(int argc, char ** argv)
  {
  double psnr[4];
    
  gavl_video_format_t format_1;
  gavl_video_format_t format_2;

  gavl_video_frame_t * frame_1;
  gavl_video_frame_t * frame_2;

  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;
  bg_plugin_registry_t * plugin_reg;
  char * tmp_path;
  int index;

  bg_plugin_handle_t * input_handle_1;
  bg_plugin_handle_t * input_handle_2;
  
  bg_input_plugin_t * input_plugin_1;
  bg_input_plugin_t * input_plugin_2;

  int frame = 0;
  
  memset(&format_1, 0, sizeof(format_1));
  memset(&format_2, 0, sizeof(format_2));
  
  if(argc < 3)
    {
    fprintf(stderr, "Usage: %s <video1> <video2>\n", argv[0]);
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

  /* Load inputs */

  if(!load_file(plugin_reg,
                &input_handle_1,
                &input_plugin_1,
                argv[1],
                &format_1))
    {
    fprintf(stderr, "Cannot open %s\n", argv[1]);
    return -1;
    }

  if(!load_file(plugin_reg,
                &input_handle_2,
                &input_plugin_2,
                argv[2],
                &format_2))
    {
    fprintf(stderr, "Cannot open %s\n", argv[2]);
    return -1;
    }

  if((format_1.image_width != format_2.image_width) ||
     (format_1.image_height != format_2.image_height) ||
     (format_1.pixelformat != format_2.pixelformat))
    {
    fprintf(stderr, "Format mismatch\n");
    return -1;
    }
  
  frame_1 = gavl_video_frame_create(&format_1);
  frame_2 = gavl_video_frame_create(&format_2);

  while(1)
    {
    if(!input_plugin_1->read_video(input_handle_1->priv,
                                  frame_1, 0))
      break;
    if(!input_plugin_2->read_video(input_handle_2->priv,
                                   frame_2, 0))
      break;
    gavl_video_frame_psnr(psnr, frame_1, frame_2, &format_1);

    printf("%d ", frame++);
    
    index = 0;
    if(gavl_pixelformat_is_gray(format_1.pixelformat))
      {
      printf("%.2f ", psnr[index]);
      index++;
      }
    else
      {
      printf("%.2f ", psnr[index]);
      printf("%.2f ", psnr[index+1]);
      printf("%.2f", psnr[index+2]);
      index+=3;
      }
    
    if(gavl_pixelformat_has_alpha(format_1.pixelformat))
      {
      printf(" %.2f\n", psnr[index]);
      }
    else
      printf("\n");

    
    }
  
  return 0;
  }
