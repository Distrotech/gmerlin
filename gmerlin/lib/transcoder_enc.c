/*****************************************************************
  
  transcoder_enc.c
  
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
  
  http://gmerlin.sourceforge.net
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
  
*****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <pluginregistry.h>
#include <tree.h>
#include <transcoder_track.h>
#include <utils.h>

void create_parameters(bg_plugin_registry_t * plugin_reg,
                       bg_transcoder_encoder_info_t * encoder_info)
  {
  /* Video */
  if(encoder_info->video_info)
    {
    encoder_info->video_encoder_parameters = encoder_info->video_info->parameters;
    encoder_info->video_stream_parameters  = encoder_info->video_info->video_parameters;
    }

  /* Audio */
  if(encoder_info->audio_info)
    {
    encoder_info->audio_encoder_parameters = encoder_info->audio_info->parameters;
    encoder_info->audio_stream_parameters  = encoder_info->audio_info->audio_parameters;
    }
  else if(encoder_info->video_info)
    {
    encoder_info->audio_stream_parameters  = encoder_info->video_info->audio_parameters;
    }

  /* Subtitle text */
  if(encoder_info->subtitle_text_info)
    {
    encoder_info->subtitle_text_encoder_parameters = encoder_info->subtitle_text_info->parameters;
    encoder_info->subtitle_text_stream_parameters  =
      encoder_info->subtitle_text_info->subtitle_text_parameters;
    }
  else if(encoder_info->video_info)
    {
    encoder_info->subtitle_text_stream_parameters  = encoder_info->video_info->subtitle_text_parameters;
    }

  /* Subtitle overlay */
  if(encoder_info->subtitle_overlay_info)
    {
    encoder_info->subtitle_overlay_encoder_parameters = encoder_info->subtitle_overlay_info->parameters;
    encoder_info->subtitle_overlay_stream_parameters  =
      encoder_info->subtitle_text_info->subtitle_overlay_parameters;
    }
  else if(encoder_info->video_info)
    {
    encoder_info->subtitle_overlay_stream_parameters  =
      encoder_info->video_info->subtitle_overlay_parameters;
    }
  }

int bg_transcoder_encoder_info_get_from_registry(bg_plugin_registry_t * plugin_reg,
                                                  bg_transcoder_encoder_info_t * encoder_info)
  {
  memset(encoder_info, 0, sizeof(*encoder_info));
  
  /* Find video encoder */
  
  encoder_info->video_info = bg_plugin_registry_get_default(plugin_reg,
                                                           BG_PLUGIN_ENCODER |
                                                           BG_PLUGIN_ENCODER_VIDEO);
  
  if(!encoder_info->video_info)
    {
    fprintf(stderr, "No video encoder found, check installation\n");
    return 0;
    }

  /* Find audio encoder */
  
  if((encoder_info->video_info->type == BG_PLUGIN_ENCODER_VIDEO) ||
     !bg_plugin_registry_get_encode_audio_to_video(plugin_reg))
    {
    encoder_info->audio_info = bg_plugin_registry_get_default(plugin_reg,
                                                                     BG_PLUGIN_ENCODER_AUDIO);
    if(!encoder_info->audio_info)
      {
      fprintf(stderr, "No audio encoder found, check installation\n");
      return 0;
      }
    }

  /* Text subtitles */
  
  if((encoder_info->video_info->type == BG_PLUGIN_ENCODER_VIDEO) ||
     !bg_plugin_registry_get_encode_subtitle_text_to_video(plugin_reg))
    {
    encoder_info->subtitle_text_info = bg_plugin_registry_get_default(plugin_reg,
                                                                      BG_PLUGIN_ENCODER_SUBTITLE_TEXT);
    if(!encoder_info->subtitle_text_info)
      {
      fprintf(stderr, "No encoder for text subtitles found, check installation\n");
      return 0;
      }
    }

  /* Overlay subtitles */

  if((encoder_info->video_info->type == BG_PLUGIN_ENCODER_VIDEO) ||
     !bg_plugin_registry_get_encode_subtitle_overlay_to_video(plugin_reg))
    {
    encoder_info->subtitle_overlay_info =
      bg_plugin_registry_get_default(plugin_reg,
                                     BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY);
    if(!encoder_info->subtitle_overlay_info)
      {
      fprintf(stderr, "No encoder for overlay subtitles found, check installation\n");
      return 0;
      }
    }
  
  /* Create sections */

  /* Video sections */
  encoder_info->video_encoder_section = bg_plugin_registry_get_section(plugin_reg,
                                                                       encoder_info->video_info->name);

  if(bg_cfg_section_has_subsection(encoder_info->video_encoder_section,
                                   "$video"))
    encoder_info->video_stream_section =
      bg_cfg_section_find_subsection(encoder_info->video_encoder_section, "$video");
  
  /* Audio sections */
  
  if(encoder_info->audio_info)
    {
    encoder_info->audio_encoder_section =
      bg_plugin_registry_get_section(plugin_reg,
                                     encoder_info->audio_info->name);

    if(bg_cfg_section_has_subsection(encoder_info->audio_encoder_section,
                                     "$audio"))
      encoder_info->audio_stream_section =
        bg_cfg_section_find_subsection(encoder_info->audio_encoder_section, "$audio");
    
    }
  else
    {
    if(bg_cfg_section_has_subsection(encoder_info->video_encoder_section,
                                     "$audio"))
      encoder_info->audio_stream_section =
        bg_cfg_section_find_subsection(encoder_info->video_encoder_section, "$audio");
    }

  /* Text subtitle sections */
  
  if(encoder_info->subtitle_text_info)
    {
    encoder_info->subtitle_text_encoder_section =
      bg_plugin_registry_get_section(plugin_reg,
                                     encoder_info->subtitle_text_info->name);

    if(bg_cfg_section_has_subsection(encoder_info->subtitle_text_encoder_section,
                                     "$subtitle_text"))
      encoder_info->subtitle_text_stream_section =
        bg_cfg_section_find_subsection(encoder_info->subtitle_text_encoder_section,
                                       "$subtitle_text");
    }
  else
    {
    if(bg_cfg_section_has_subsection(encoder_info->video_encoder_section,
                                     "$subtitle_text"))
      encoder_info->subtitle_text_stream_section =
        bg_cfg_section_find_subsection(encoder_info->video_encoder_section,
                                       "$subtitle_text");
    
    }

  /* Overlay subtitle sections */

  if(encoder_info->subtitle_overlay_info)
    {
    encoder_info->subtitle_overlay_encoder_section =
      bg_plugin_registry_get_section(plugin_reg,
                                     encoder_info->subtitle_overlay_info->name);

    if(bg_cfg_section_has_subsection(encoder_info->subtitle_overlay_encoder_section,
                                     "$subtitle_overlay"))
      encoder_info->subtitle_overlay_stream_section =
        bg_cfg_section_find_subsection(encoder_info->subtitle_overlay_encoder_section,
                                       "$subtitle_overlay");
    }
  else
    {
    if(bg_cfg_section_has_subsection(encoder_info->video_encoder_section,
                                     "$subtitle_overlay"))
      encoder_info->subtitle_overlay_stream_section =
        bg_cfg_section_find_subsection(encoder_info->video_encoder_section,
                                       "$subtitle_overlay");
    
    }

  create_parameters(plugin_reg, encoder_info);
  
  return 1;
  }

int bg_transcoder_encoder_info_get_from_track(bg_plugin_registry_t * plugin_reg,
                                               bg_transcoder_track_t * track,
                                               bg_transcoder_encoder_info_t * encoder_info)
  {
  char * video_encoder;
  char * audio_encoder;
  char * subtitle_text_encoder;
  char * subtitle_overlay_encoder;
  int create_sections = 0;
  bg_transcoder_encoder_info_t default_info;
  memset(encoder_info, 0, sizeof(*encoder_info));
    
  audio_encoder = bg_transcoder_track_get_audio_encoder(track);
  video_encoder = bg_transcoder_track_get_video_encoder(track);
  subtitle_text_encoder =
    bg_transcoder_track_get_subtitle_text_encoder(track);
  subtitle_overlay_encoder =
    bg_transcoder_track_get_subtitle_overlay_encoder(track);

  encoder_info->video_info = bg_plugin_find_by_name(plugin_reg, video_encoder);
  
  if(strcmp(audio_encoder, video_encoder))
    encoder_info->audio_info =
      bg_plugin_find_by_name(plugin_reg, audio_encoder);

  
  if(subtitle_text_encoder)
    {
    if(strcmp(subtitle_text_encoder, video_encoder))
      encoder_info->subtitle_text_info =
        bg_plugin_find_by_name(plugin_reg, subtitle_text_encoder);
    }
  else
    {
    encoder_info->subtitle_text_info =
      bg_plugin_registry_get_default(plugin_reg,
                                     BG_PLUGIN_ENCODER_SUBTITLE_TEXT);    
    create_sections = 1;
    }
  if(subtitle_overlay_encoder)
    {
    if(strcmp(subtitle_overlay_encoder, video_encoder))
      encoder_info->subtitle_overlay_info =
        bg_plugin_find_by_name(plugin_reg, subtitle_overlay_encoder);
    }
  else
    {
    encoder_info->subtitle_overlay_info =
      bg_plugin_registry_get_default(plugin_reg,
                                     BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY);
    create_sections = 1;
    }
  create_parameters(plugin_reg, encoder_info);

  if(create_sections)
    {
    bg_transcoder_encoder_info_get_from_registry(plugin_reg, &default_info);
    bg_transcoder_track_create_encoder_sections(track, &default_info);
    }
  
  free(video_encoder);
  free(audio_encoder);
  if(subtitle_text_encoder)    free(subtitle_text_encoder);
  if(subtitle_overlay_encoder) free(subtitle_overlay_encoder);
  
  return 1;
  }

void
bg_transcoder_encoder_info_get_sections_from_track(bg_transcoder_track_t * t,
                                                   bg_transcoder_encoder_info_t * encoder_info)
  {
  encoder_info->audio_encoder_section = t->audio_encoder_section;
  encoder_info->video_encoder_section = t->video_encoder_section;
  encoder_info->subtitle_text_encoder_section = t->subtitle_text_encoder_section;
  encoder_info->subtitle_overlay_encoder_section =
    t->subtitle_overlay_encoder_section;

  /* We take sections from the first stream */
  if(t->num_audio_streams)
    {
    encoder_info->audio_stream_section = t->audio_streams[0].encoder_section;
    }

  if(t->num_video_streams)
    {
    encoder_info->video_stream_section = t->video_streams[0].encoder_section;
    }

  if(t->num_subtitle_text_streams)
    {
    encoder_info->subtitle_text_stream_section =
      t->subtitle_text_streams[0].encoder_section_text;

    encoder_info->subtitle_overlay_stream_section =
      t->subtitle_text_streams[0].encoder_section_overlay;
    }
  else if(t->num_subtitle_overlay_streams)
    {
    encoder_info->subtitle_overlay_stream_section =
      t->subtitle_overlay_streams[0].encoder_section;
    
    }

  }
