#include <stdlib.h>
#include <string.h>

#include <pluginregistry.h>
#include <utils.h>

#include <cfg_dialog.h>
#include <transcoder_track.h>
#include "trackdialog.h"

struct track_dialog_s
  {
  /* Config dialog */

  bg_dialog_t * cfg_dialog;

  void (*update_callback)(void * priv);
  void * update_priv;
  };

static void set_parameter_general(void * priv, char * name, bg_parameter_value_t * val)
  {
  track_dialog_t * d;
  d = (track_dialog_t *)priv;

  if(!name)
    {
    if(d->update_callback)
      d->update_callback(d->update_priv);
    }
  }

track_dialog_t * track_dialog_create(bg_transcoder_track_t * t,
                                     void (*update_callback)(void * priv),
                                     void * update_priv, int show_tooltips)
  {
  int i;
  char * label;
  track_dialog_t * ret;
  void * parent;
    
  ret = calloc(1, sizeof(*ret));

  ret->update_callback = update_callback;
  ret->update_priv     = update_priv;
  
  ret->cfg_dialog = bg_dialog_create_multi("Track options");
  
  /* General */
  
  bg_dialog_add(ret->cfg_dialog,
                "General",
                t->general_section,
                set_parameter_general, ret,
                t->general_parameters);

  
  /* Metadata */
  
  bg_dialog_add(ret->cfg_dialog,
                "Metadata",
                t->metadata_section,
                NULL,
                NULL,
                t->metadata_parameters);

  /* Audio encoder */

  if(t->audio_encoder_parameters)
    {
    bg_dialog_add(ret->cfg_dialog,
                  bg_cfg_section_get_name(t->audio_encoder_section),
                  t->audio_encoder_section,
                  NULL,
                  NULL,
                  t->audio_encoder_parameters);
    }

  /* Video encoder */

  if(t->video_encoder_parameters)
    {
    bg_dialog_add(ret->cfg_dialog,
                  bg_cfg_section_get_name(t->video_encoder_section),
                  t->video_encoder_section,
                  NULL,
                  NULL,
                  t->video_encoder_parameters);
    }
  
  
  /* Audio streams */

  for(i = 0; i < t->num_audio_streams; i++)
    {
    label = bg_sprintf("Audio stream %d", i+1);
    parent = bg_dialog_add_parent(ret->cfg_dialog, NULL,
                                  label);
    free(label);
    
    bg_dialog_add_child(ret->cfg_dialog, parent,
                        "General",
                        t->audio_streams[i].general_section,
                        NULL,
                        NULL,
                        bg_transcoder_track_audio_get_general_parameters());

    if(t->audio_streams[i].encoder_parameters)
      bg_dialog_add_child(ret->cfg_dialog, parent,
                          bg_cfg_section_get_name(t->audio_streams[i].encoder_section),
                          t->audio_streams[i].encoder_section,
                          NULL,
                          NULL,
                          t->audio_streams[i].encoder_parameters);
    }

  /* Video streams */

  for(i = 0; i < t->num_video_streams; i++)
    {
    label = bg_sprintf("Video stream %d", i+1);

    parent = bg_dialog_add_parent(ret->cfg_dialog, NULL,
                                  label);
    free(label);
    
    bg_dialog_add_child(ret->cfg_dialog, parent,
                        "General",
                        t->video_streams[i].general_section,
                        NULL,
                        NULL,
                        bg_transcoder_track_video_get_general_parameters());

    if(t->video_streams[i].encoder_parameters)
      bg_dialog_add_child(ret->cfg_dialog, parent,
                          bg_cfg_section_get_name(t->video_streams[i].encoder_section),
                          t->video_streams[i].encoder_section,
                          NULL,
                          NULL,
                          t->video_streams[i].encoder_parameters);
    }

  bg_dialog_set_tooltips(ret->cfg_dialog, show_tooltips);

  fprintf(stderr, "Show tooltips: %d\n", show_tooltips);
  
  return ret;
  
  }

void track_dialog_run(track_dialog_t * d)
  {
  bg_dialog_show(d->cfg_dialog);
  }

void track_dialog_destroy(track_dialog_t * d)
  {
  bg_dialog_destroy(d->cfg_dialog);
  free(d);
  }

