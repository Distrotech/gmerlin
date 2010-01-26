/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

int main(int argc, char ** argv)
  {
  gavl_video_format_t format_1;
  gavl_video_format_t format_2;
  gavl_video_format_t format;
  
  gavl_video_frame_t * frame_in_1;
  gavl_video_frame_t * frame_in_2;
  
  gavl_video_frame_t * frame_1;
  gavl_video_frame_t * frame_2;
  gavl_video_frame_t * frame_out;
  
  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;
  bg_plugin_registry_t * plugin_reg;
  char * tmp_path;
  
  int do_convert;
  
  gavl_video_converter_t * cnv;
  
  memset(&format_1, 0, sizeof(format_1));
  memset(&format_2, 0, sizeof(format_2));
  
  if((argc < 3) || (argc > 4))
    {
    fprintf(stderr, "Usage: %s <image1> <image2> [<output_image>]\n", argv[0]);
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

  frame_in_1 =
    bg_plugin_registry_load_image(plugin_reg, argv[1], &format_1, NULL);  
  if(!frame_in_1)
    {
    fprintf(stderr, "Cannot open %s\n", argv[1]);
    return -1;
    }
  
  frame_in_2 =
    bg_plugin_registry_load_image(plugin_reg, argv[2], &format_2, NULL);
  if(!frame_in_2)
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

  cnv = gavl_video_converter_create();

  gavl_video_format_copy(&format, &format_1);
  format.pixelformat = GAVL_GRAY_FLOAT;

  
  do_convert = gavl_video_converter_init(cnv, &format_1, &format);

  if(do_convert)
    {
    frame_1 = gavl_video_frame_create(&format);
    frame_2 = gavl_video_frame_create(&format);

    gavl_video_convert(cnv, frame_in_1, frame_1);
    gavl_video_convert(cnv, frame_in_2, frame_2);
    }
  else
    {
    frame_1 = frame_in_1;
    frame_2 = frame_in_2;
    }

  frame_out = gavl_video_frame_create(&format);

  if(!gavl_video_frame_ssim(frame_1,
                            frame_2,
                            frame_out,
                            &format))
    {
    fprintf(stderr, "Calculating SSIM failed\n");
    return -1;
    }

  if(argc == 4)
    {
    bg_plugin_registry_save_image(plugin_reg, argv[3], frame_out, &format, NULL);
    }
  else
    {
    int i, j;
    double ret = 0.0;
    float * ptr;
    for(i = 0; i < format.image_height; i++)
      {
      ptr = (float*)(frame_out->planes[0] + frame_out->strides[0] * i);
      for(j = 0; j < format.image_height; j++)
        {
        ret += *ptr;
        ptr++;
        }
      }
    ret /= (double)(format.image_height * format.image_width);
    fprintf(stderr, "Mean SSIM: %.16f\n", ret);
    }
  
  //  fprintf(stderr, "Format:\n\n");
  //  gavl_video_format_dump(&format_1);
    
  return 0;
  }
