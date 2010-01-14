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

  memset(&format_1, 0, sizeof(format_1));
  memset(&format_2, 0, sizeof(format_2));
  
  if(argc < 3)
    {
    fprintf(stderr, "Usage: %s <image1> <image2>\n", argv[0]);
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

  frame_1 =
    bg_plugin_registry_load_image(plugin_reg, argv[1], &format_1, NULL);  
  if(!frame_1)
    {
    fprintf(stderr, "Cannot open %s\n", argv[1]);
    return -1;
    }
  
  frame_2 =
    bg_plugin_registry_load_image(plugin_reg, argv[2], &format_2, NULL);
  if(!frame_2)
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
  
  //  fprintf(stderr, "Format:\n\n");
  //  gavl_video_format_dump(&format_1);
    
  gavl_video_frame_psnr(psnr, frame_1, frame_2, &format_1);
  index = 0;
  fprintf(stderr, "PSNR: ");
  
  if(gavl_pixelformat_is_gray(format_1.pixelformat))
    {
    fprintf(stderr, "Gray: %.2f dB", psnr[index]);
    index++;
    }
  else if(gavl_pixelformat_is_yuv(format_1.pixelformat))
    {
    fprintf(stderr, "Y': %.2f dB, ", psnr[index]);
    fprintf(stderr, "Cb: %.2f dB, ", psnr[index+1]);
    fprintf(stderr, "Cr: %.2f dB", psnr[index+2]);
    index+=3;
    }
  else if(gavl_pixelformat_is_rgb(format_1.pixelformat))
    {
    fprintf(stderr, "R: %.2f dB, ", psnr[index]);
    fprintf(stderr, "G: %.2f dB, ", psnr[index+1]);
    fprintf(stderr, "B: %.2f dB", psnr[index+2]);
    index+=3;
    }
  if(gavl_pixelformat_has_alpha(format_1.pixelformat))
    {
    fprintf(stderr, ", A: %.2f dB\n", psnr[index]);
    }
  else
    fprintf(stderr, "\n");
  
  return 0;
  }
