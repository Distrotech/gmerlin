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

static struct
  {
  gavl_color_channel_t ch;
  char c;
  }
channel_names[] =
  {
    { GAVL_CCH_RED,   'r' },
    { GAVL_CCH_GREEN, 'g' },
    { GAVL_CCH_BLUE,  'b' },
    { GAVL_CCH_Y,     'y' },
    { GAVL_CCH_CB,    'u' },
    { GAVL_CCH_CR,    'v' },
    { GAVL_CCH_ALPHA, 'a' },
  };

static char get_channel_name(gavl_color_channel_t ch)
  {
  int i;
  for(i = 0; i < sizeof(channel_names)/sizeof(channel_names[0]); i++)
    {
    if(ch == channel_names[i].ch)
      return channel_names[i].c;
    }
  return 0x00;
  }

int main(int argc, char ** argv)
  {
  int num_channels;
  gavl_color_channel_t ch;
  int i;
  
  gavl_video_format_t format_1;
  gavl_video_format_t format_2;

  gavl_video_frame_t * frame_1;
  gavl_video_frame_t * frame_2;
  gavl_metadata_t m;

  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;
  bg_plugin_registry_t * plugin_reg;
  char * tmp_path;
  char * filename;
  
  memset(&format_1, 0, sizeof(format_1));
  memset(&format_2, 0, sizeof(format_2));
  memset(&m, 0, sizeof(m));

  if(argc < 4)
    {
    fprintf(stderr, "Usage: %s <image1> <output_prefix> <output_extension>\n", argv[0]);
    return -1;
    }

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

  num_channels = gavl_pixelformat_num_channels(format_1.pixelformat);
  
  for(i = 0; i < num_channels; i++)
    {
    ch = gavl_pixelformat_get_channel(format_1.pixelformat, i);

    if(!gavl_get_color_channel_format(&format_1, &format_2, ch))
      {
      fprintf(stderr, "Invalid channel\n");
      return -1;
      }

    frame_2 = gavl_video_frame_create(&format_2);
    
    if(!gavl_video_frame_extract_channel(&format_1, ch, frame_1, frame_2))
      {
      fprintf(stderr, "Cannot extract channel\n");
      return -1;
      }

    filename = bg_sprintf("%s_%c.%s", argv[2], get_channel_name(ch), argv[3]);
    
    bg_plugin_registry_save_image(plugin_reg,
                                  filename,
                                  frame_2,
                                  &format_2, &m);
    
    fprintf(stderr, "Saved %s\n", filename);
    
    gavl_video_frame_destroy(frame_2);
    free(filename);
    }
  return 0;
  }
