#ifndef MEDIALIST_H
#define MEDIALIST_H

#include <gmerlin/pluginregistry.h>

typedef struct
  {
  int num_audio_streams;
  int num_video_streams;
  char * filename;
  char * name;
  gavl_time_t duration;
  int refcount;    // How many tracks reference this file
  int track;       // Track within the file
  
  int id;          /* Internal ID */
  } bg_nle_file_t;

typedef struct
  {
  bg_nle_file_t ** files;
  int num_files;
  int files_alloc;
  
  bg_plugin_registry_t * plugin_reg;
  char * open_path;
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

void bg_nle_media_list_destroy(bg_nle_media_list_t * list);

/* medialist_xml.c */

bg_nle_media_list_t *
bg_nle_media_list_load(bg_plugin_registry_t * plugin_reg,
                       xmlDocPtr xml_doc, xmlNodePtr node);

void
bg_nle_media_list_save(bg_nle_media_list_t * t, xmlNodePtr node);

#endif
