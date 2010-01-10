#ifndef MEDIALIST_H
#define MEDIALIST_H

#include <gmerlin/pluginregistry.h>

#include <types.h>

typedef struct
  {
  int timescale;
  int64_t start_time;
  int64_t duration;
  } bg_nle_audio_stream_t;

typedef struct
  {
  int timescale;
  gavl_frame_table_t * frametable;
  
  gavl_timecode_format_t tc_format;

  } bg_nle_video_stream_t;

typedef struct
  {
  int num_audio_streams;
  int num_video_streams;

  bg_nle_audio_stream_t * audio_streams;
  bg_nle_video_stream_t * video_streams;
  
  char * filename;

  char filename_hash[33];
  
  char * name;
  /* For loading the track */
  char * plugin;
  bg_cfg_section_t * section;
  
  gavl_time_t duration;
  int refcount;    // How many tracks reference this file
  int track;       // Track within the file

  bg_nle_id_t id;
  
  char * cache_dir;
  } bg_nle_file_t;

void bg_nle_file_destroy(bg_nle_file_t * file);
bg_nle_file_t * bg_nle_file_copy(const bg_nle_file_t * file);

typedef struct
  {
  bg_nle_file_t ** files;
  int num_files;
  int files_alloc;
  
  bg_plugin_registry_t * plugin_reg;
  char * open_path;

  char * cache_dir; // Is saved in the project
  } bg_nle_media_list_t;

bg_nle_media_list_t *
bg_nle_media_list_create(bg_plugin_registry_t * plugin_reg);

bg_nle_file_t *
bg_nle_media_list_load_file(bg_nle_media_list_t * list,
                            const char * file,
                            const char * plugin);

bg_nle_file_t *
bg_nle_media_list_find_file(bg_nle_media_list_t * list,
                            const char * filename, int track);

bg_nle_file_t *
bg_nle_media_list_find_file_by_id(bg_nle_media_list_t * list, bg_nle_id_t id);


void bg_nle_media_list_destroy(bg_nle_media_list_t * list);

void bg_nle_media_list_insert(bg_nle_media_list_t * list,
                              bg_nle_file_t * file, int index);

int bg_nle_media_list_merge(bg_nle_media_list_t * list,
                            bg_nle_file_t * file);

void bg_nle_media_list_delete(bg_nle_media_list_t * list,
                              int index);

char * bg_nle_media_list_get_frame_table_filename(bg_nle_file_t * file,
                                                  int stream);

/* This is used whenever a file is opened */
bg_plugin_handle_t * bg_nle_media_list_open_file(bg_nle_media_list_t * list,
                                                 bg_nle_file_t * file);



/* medialist_xml.c */

bg_nle_file_t * bg_nle_file_load(xmlDocPtr xml_doc, xmlNodePtr node);
void bg_nle_file_save(xmlNodePtr node, bg_nle_file_t * file);

bg_nle_media_list_t *
bg_nle_media_list_load(bg_plugin_registry_t * plugin_reg,
                       xmlDocPtr xml_doc, xmlNodePtr node);

void
bg_nle_media_list_save(bg_nle_media_list_t * t, xmlNodePtr node);



#endif
