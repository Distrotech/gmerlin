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

static const struct
  {
  gavl_color_channel_t ch;
  const char * name;
  }
channels[] =
  { 
    { GAVL_CCH_RED,   "R" }, //!< Red
    { GAVL_CCH_GREEN, "G" }, //!< Green
    { GAVL_CCH_BLUE,  "B" }, //!< Blue
    { GAVL_CCH_Y,     "Y" }, //!< Luminance (also gracscale)
    { GAVL_CCH_CB,    "U" }, //!< Chrominance blue (aka U)
    { GAVL_CCH_CR,    "V" }, //!< Chrominance red (aka V)
    { GAVL_CCH_ALPHA, "A" }, //!< Transparency (or, to be more precise opacity)
  };

int num_channels = sizeof(channels)/sizeof(channels[0]);

int main(int argc, char ** argv)
  {
  int i, j;
  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;
  bg_plugin_registry_t * plugin_reg;

  char * tmp_string;
  
  gavl_video_frame_t * in_frame;
  gavl_video_frame_t * out_frame;
  
  gavl_video_format_t in_format;
  gavl_video_format_t out_format;

  int num_formats;

  gavl_pixelformat_t fmt;
  
  /* Create registries */
  
  cfg_reg = bg_cfg_registry_create();
  tmp_string =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_string);
  if(tmp_string)
    free(tmp_string);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);
  
  /* Create converter */

  num_formats = gavl_num_pixelformats();

  for(i = 0; i < num_formats; i++)
    {
    fmt = gavl_get_pixelformat(i);

    memset(&out_format, 0, sizeof(out_format));
    
    for(j = 0; j < num_channels; j++)
      {
      tmp_string =
        bg_sprintf("%s_%s.gavi",
                   gavl_pixelformat_to_string(fmt),
                   channels[j].name);

      in_frame = bg_plugin_registry_load_image(plugin_reg,
                                               tmp_string,
                                               &in_format, NULL);
      free(tmp_string);
      if(!in_frame)
        continue;

      if(!out_format.image_width)
        {
        gavl_video_format_copy(&out_format, &in_format);
        out_format.pixelformat = fmt;
        out_frame = gavl_video_frame_create(&out_format);
        gavl_video_frame_clear(out_frame, &out_format);
        }
      
      if(!gavl_video_frame_insert_channel(&out_format,
                                          channels[j].ch,
                                          in_frame,
                                          out_frame))
        {
        fprintf(stderr, "Huh? Inserting %s to %s failed\n",
                channels[j].name,
                gavl_pixelformat_to_string(fmt));
        return -1;
        }

      gavl_video_frame_destroy(in_frame);
      
      }

    tmp_string =
      bg_sprintf("insert_%s.gavi",
                 gavl_pixelformat_to_string(fmt));
    
    bg_plugin_registry_save_image(plugin_reg,
                                  tmp_string,
                                  out_frame, &out_format,
                                  NULL);
    
    fprintf(stderr, "Wrote %s\n", tmp_string);
    
    free(tmp_string);
    
    
    }
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  
  return 0;
  
  }
