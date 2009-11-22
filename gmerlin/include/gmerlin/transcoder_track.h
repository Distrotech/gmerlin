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

#ifndef __BG_TRANSCODER_TRACK_H_
#define __BG_TRANSCODER_TRACK_H_

#include <libxml/tree.h>
#include <libxml/parser.h>
// #include <gmerlin/encoderinfo.h>
/* This defines a track with all information
   necessary for transcoding */

typedef struct bg_transcoder_track_s bg_transcoder_track_t;

typedef struct
  {
  char * label;

  bg_parameter_info_t * general_parameters;
  bg_parameter_info_t * filter_parameters;
  bg_cfg_section_t * encoder_section;
  bg_cfg_section_t * general_section;
  bg_cfg_section_t * filter_section;
  } bg_transcoder_track_audio_t;

typedef struct
  {
  char * label;

  bg_parameter_info_t * general_parameters;
  bg_parameter_info_t * filter_parameters;
  
  bg_cfg_section_t * encoder_section;

  bg_cfg_section_t * general_section;
  bg_cfg_section_t * filter_section;
  } bg_transcoder_track_video_t;

typedef struct
  {
  char * label;
  int in_index;
  
  bg_parameter_info_t * general_parameters;
  bg_cfg_section_t * general_section;
  bg_cfg_section_t * textrenderer_section;

  bg_cfg_section_t * encoder_section_text;
  bg_cfg_section_t * encoder_section_overlay;
  
  } bg_transcoder_track_subtitle_text_t;

typedef struct
  {
  char * label;
  int in_index;

  bg_parameter_info_t * general_parameters;
  bg_cfg_section_t * general_section;
  bg_cfg_section_t * encoder_section;
  
  } bg_transcoder_track_subtitle_overlay_t;


struct bg_transcoder_track_s
  {
  bg_cfg_section_t    * input_section;
  
  bg_parameter_info_t * metadata_parameters;
  bg_cfg_section_t    * metadata_section;

  bg_parameter_info_t * general_parameters;
  bg_cfg_section_t    * general_section;

  bg_cfg_section_t    * audio_encoder_section;
  bg_cfg_section_t    * video_encoder_section;
  bg_cfg_section_t    * subtitle_text_encoder_section;
  bg_cfg_section_t    * subtitle_overlay_encoder_section;
  
  int num_audio_streams;
  int num_video_streams;
  int num_subtitle_text_streams;
  int num_subtitle_overlay_streams;

  bg_transcoder_track_audio_t    * audio_streams;
  bg_transcoder_track_video_t    * video_streams;
  bg_transcoder_track_subtitle_text_t    * subtitle_text_streams;
  bg_transcoder_track_subtitle_overlay_t * subtitle_overlay_streams;
  
  /* For chaining */
  struct bg_transcoder_track_s * next;

  /* Is the track selected by the GUI? */

  int selected;

  /* This is non NULL if we have a redirector */
  char * url;

  /* Chapter list (can be NULL) */
  bg_chapter_list_t * chapter_list;
  };

const bg_parameter_info_t *
bg_transcoder_track_get_general_parameters();

const bg_parameter_info_t *
bg_transcoder_track_audio_get_general_parameters();

const bg_parameter_info_t *
bg_transcoder_track_video_get_general_parameters();

const bg_parameter_info_t *
bg_transcoder_track_subtitle_text_get_general_parameters();

const bg_parameter_info_t *
bg_transcoder_track_subtitle_overlay_get_general_parameters();


bg_transcoder_track_t *
bg_transcoder_track_create(const char * url,
                           const bg_plugin_info_t * plugin,
                           int track, bg_plugin_registry_t * plugin_reg,
                           bg_cfg_section_t * section,
                           bg_cfg_section_t * encoder_section,
                           char * name);

bg_transcoder_track_t *
bg_transcoder_track_create_from_urilist(const char * list,
                                        int len,
                                        bg_plugin_registry_t * plugin_reg,
                                        bg_cfg_section_t * section,
                                        bg_cfg_section_t * encoder_section);

bg_transcoder_track_t *
bg_transcoder_track_create_from_albumentries(const char * xml_string,
                                             bg_plugin_registry_t * plugin_reg,
                                             bg_cfg_section_t * section,
                                             bg_cfg_section_t * encoder_section);

void bg_transcoder_track_destroy(bg_transcoder_track_t * t);

/* For putting informations into the track list */

/* strings must be freed after */

char * bg_transcoder_track_get_name(bg_transcoder_track_t * t);

const char * bg_transcoder_track_get_audio_encoder(bg_transcoder_track_t * t);
const char * bg_transcoder_track_get_video_encoder(bg_transcoder_track_t * t);
const char * bg_transcoder_track_get_subtitle_text_encoder(bg_transcoder_track_t * t);
const char * bg_transcoder_track_get_subtitle_overlay_encoder(bg_transcoder_track_t * t);

void bg_transcoder_track_get_duration(bg_transcoder_track_t * t,
                                      gavl_time_t * ret, gavl_time_t * ret_total);

/*
 *  The following function is for internal use ONLY
 *  (not worth making a private header file for one routine)
 */

void
bg_transcoder_track_create_parameters(bg_transcoder_track_t * track,
                                      bg_plugin_registry_t * plugin_reg);

#if 0
void
bg_transcoder_track_set_encoders(bg_transcoder_track_t * track,
                                 bg_plugin_registry_t * plugin_reg,
                                 const bg_encoder_info_t * info);
#endif

void bg_transcoder_track_get_encoders(bg_transcoder_track_t * t,
                                      bg_plugin_registry_t * plugin_reg,
                                      bg_cfg_section_t * encoder_section);

void bg_transcoder_track_set_encoders(bg_transcoder_track_t * t,
                                      bg_plugin_registry_t * plugin_reg,
                                      bg_cfg_section_t * encoder_section);
  

#if 0
void bg_transcoder_track_create_encoder_sections(bg_transcoder_track_t * t,
                                                 const bg_encoder_info_t * info);
#endif

/*
 *  Global options
 */

typedef struct
  {
  char * pp_plugin; /* Postprocess */
  bg_cfg_section_t    * pp_section;
  } bg_transcoder_track_global_t;

void
bg_transcoder_track_global_to_reg(bg_transcoder_track_global_t * g,
                                  bg_plugin_registry_t * plugin_reg);

void
bg_transcoder_track_global_from_reg(bg_transcoder_track_global_t * g,
                                    bg_plugin_registry_t * plugin_reg);


void
bg_transcoder_track_global_free(bg_transcoder_track_global_t * g);

/* Functions, which operate on lists of transcoder tracks */

bg_transcoder_track_t *
bg_transcoder_tracks_delete_selected(bg_transcoder_track_t * t);

bg_transcoder_track_t *
bg_transcoder_tracks_move_selected_up(bg_transcoder_track_t * t);

bg_transcoder_track_t *
bg_transcoder_tracks_move_selected_down(bg_transcoder_track_t * t);

char *
bg_transcoder_tracks_selected_to_xml(bg_transcoder_track_t * t);

bg_transcoder_track_t * 
bg_transcoder_tracks_from_xml(char *, bg_plugin_registry_t * plugin_reg);


bg_transcoder_track_t *
bg_transcoder_tracks_append(bg_transcoder_track_t * t, bg_transcoder_track_t * tail);

bg_transcoder_track_t *
bg_transcoder_tracks_prepend(bg_transcoder_track_t * t, bg_transcoder_track_t * head);

bg_transcoder_track_t *
bg_transcoder_tracks_extract_selected(bg_transcoder_track_t ** t);


/* transcoder_track_xml.c */

void bg_transcoder_tracks_save(bg_transcoder_track_t * t,
                               bg_transcoder_track_global_t * g,
                               const char * filename);

bg_transcoder_track_t *
bg_transcoder_tracks_load(const char * filename,
                          bg_transcoder_track_global_t * g,
                          bg_plugin_registry_t * plugin_reg);

#endif
