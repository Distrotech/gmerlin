/*****************************************************************
  
  transcoder_track.h
  
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
  
  http://gmerlin.sourceforge.net
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
  
*****************************************************************/

#ifndef __BG_TRANSCODER_TRACK_H_
#define __BG_TRANSCODER_TRACK_H_

#include <libxml/tree.h>
#include <libxml/parser.h>


/* This defines a track with all information
   necessary for transcoding */

typedef struct
  {
  bg_parameter_info_t * encoder_parameters;
  bg_cfg_section_t * encoder_section;
  bg_cfg_section_t * general_section;
  } bg_transcoder_track_audio_t;

bg_parameter_info_t *
bg_transcoder_track_audio_get_general_parameters();

typedef struct
  {
  bg_parameter_info_t * encoder_parameters;
  bg_cfg_section_t * encoder_section;
  bg_cfg_section_t * general_section;
  } bg_transcoder_track_video_t;

bg_parameter_info_t *
bg_transcoder_track_video_get_general_parameters();

typedef struct bg_transcoder_track_s
  {
  bg_parameter_info_t * metadata_parameters;
  bg_cfg_section_t    * metadata_section;

  bg_parameter_info_t * general_parameters;
  bg_cfg_section_t    * general_section;

  bg_parameter_info_t * audio_encoder_parameters;
  bg_cfg_section_t    * audio_encoder_section;
  
  bg_parameter_info_t * video_encoder_parameters;
  bg_cfg_section_t    * video_encoder_section;
  
  int num_audio_streams;
  int num_video_streams;

  bg_transcoder_track_audio_t * audio_streams;
  bg_transcoder_track_video_t * video_streams;
      
  /* For chaining */
  struct bg_transcoder_track_s * next;

  /* Is the track selected by the GUI? */

  int selected;

  } bg_transcoder_track_t;

bg_transcoder_track_t *
bg_transcoder_track_create(const char * url,
                           const bg_plugin_info_t * plugin,
                           int track, bg_plugin_registry_t * plugin_reg,
                           bg_cfg_section_t * section);

bg_transcoder_track_t *
bg_transcoder_track_create_from_urilist(const char * list,
                                        int len,
                                        bg_plugin_registry_t * plugin_reg,
                                        bg_cfg_section_t * section);

bg_transcoder_track_t *
bg_transcoder_track_create_from_albumentries(const char * xml_string,
                                             int len,
                                             bg_plugin_registry_t * plugin_reg,
                                             bg_cfg_section_t * section);

void bg_transcoder_track_destroy(bg_transcoder_track_t * t);

/* For putting informations into the track list */

/* strings must be freed after */

char * bg_transcoder_track_get_name(bg_transcoder_track_t * t);
char * bg_transcoder_track_get_audio_encoder(bg_transcoder_track_t * t);
char * bg_transcoder_track_get_video_encoder(bg_transcoder_track_t * t);

gavl_time_t  bg_transcoder_track_get_duration(bg_transcoder_track_t * t);


/*
 *  The following function is for internal use ONLY
 *  (not worth making a private header file for one routine)
 */

void
bg_transcoder_track_create_parameters(bg_transcoder_track_t * track,
                                      bg_plugin_handle_t * audio_encoder,
                                      bg_plugin_handle_t * video_encoder);
  
/* transcoder_track_xml.c */

void bg_transcoder_tracks_save(bg_transcoder_track_t * t, const char * filename);

bg_transcoder_track_t *
bg_transcoder_tracks_load(const char * filename,
                          bg_plugin_registry_t * plugin_reg);


#endif
