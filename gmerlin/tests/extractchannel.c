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
  gavl_video_frame_t * tmp_frame = NULL;
  gavl_video_frame_t * out_frame;
  gavl_video_frame_t * f;
  
  gavl_video_format_t in_format;
  gavl_video_format_t tmp_format;
  gavl_video_format_t out_format;
  int do_convert;

  int num_formats;
    
  gavl_video_converter_t * cnv;
  
  if(argc != 2)
    {
    fprintf(stderr, "Usage: %s <image>\n", argv[0]);
    return -1;
    }
  
  /* Create registries */
  
  cfg_reg = bg_cfg_registry_create();
  tmp_string =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_string);
  if(tmp_string)
    free(tmp_string);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);
  
  /* Load input image */
  in_frame = bg_plugin_registry_load_image(plugin_reg,
                                           argv[1],
                                           &in_format, NULL);
  
  if(!in_frame)
    {
    fprintf(stderr, "Couldn't load %s\n", argv[1]);
    return -1;
    }

  gavl_video_format_copy(&tmp_format, &in_format);
  
  /* Create converter */
  cnv = gavl_video_converter_create();

  num_formats = gavl_num_pixelformats();

  for(i = 0; i < num_formats; i++)
    {
    tmp_format.pixelformat = gavl_get_pixelformat(i);

    do_convert = gavl_video_converter_init(cnv, &in_format, &tmp_format);

    if(do_convert)
      {
      tmp_frame = gavl_video_frame_create(&tmp_format);
      gavl_video_convert(cnv, in_frame, tmp_frame);
      f = tmp_frame;
      }
    else
      f = in_frame;
    
    for(j = 0; j < num_channels; j++)
      {
      /* Check if channel is available */
      if(!gavl_get_color_channel_format(&tmp_format,
                                        &out_format,
                                        channels[j].ch))
        continue;
      
      out_frame = gavl_video_frame_create(&out_format);

      if(!gavl_video_frame_extract_channel(&tmp_format,
                                           channels[j].ch,
                                           f,
                                           out_frame))
        {
        fprintf(stderr, "Huh? Extracting %s from %s failed\n",
                channels[j].name,
                gavl_pixelformat_to_string(tmp_format.pixelformat));
        return -1;
        }
      tmp_string =
        bg_sprintf("%s_%s.gavi",
                   gavl_pixelformat_to_string(tmp_format.pixelformat),
                   channels[j].name);

      bg_plugin_registry_save_image(plugin_reg,
                                    tmp_string,
                                    out_frame, &out_format,
                                    NULL);

      fprintf(stderr, "Wrote %s\n", tmp_string);
      
      free(tmp_string);
      gavl_video_frame_destroy(out_frame);
      
      }
    if(tmp_frame)
      {
      gavl_video_frame_destroy(tmp_frame);
      tmp_frame = NULL;
      }
    
    }

  gavl_video_frame_destroy(in_frame);

  gavl_video_converter_destroy(cnv);
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  
  
  return 0;
  
  }
