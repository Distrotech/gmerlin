#include <stdlib.h>
#include <string.h>

#include <pluginregistry.h>
#include <utils.h>

#include <cfg_dialog.h>
#include <transcoder_track.h>
#include "trackdialog.h"

struct track_dialog_s
  {
  /* Metadata */

  bg_parameter_info_t * param_metadata;

  /* General info */
  
  bg_parameter_info_t * param_general;

  /* Config dialog */

  bg_dialog_t * cfg_dialog;
  
  };

track_dialog_t * track_dialog_create(bg_transcoder_track_t * t)
  {
  int i;
  char * label;
  track_dialog_t * ret;

  ret = calloc(1, sizeof(*ret));
  ret->cfg_dialog = bg_dialog_create_multi("Transcode options");

  /* General */

  ret->param_general  = bg_transcoder_track_get_parameters_general(t);

  bg_dialog_add(ret->cfg_dialog,
                "General",
                (bg_cfg_section_t*)0,
                bg_transcoder_track_set_parameter_general,
                t,
                ret->param_general);

  
  /* Metadata */

  ret->param_metadata = bg_metadata_get_parameters(&(t->metadata));

  bg_dialog_add(ret->cfg_dialog,
                "Metadata",
                (bg_cfg_section_t*)0,
                bg_metadata_set_parameter,
                &(t->metadata),
                ret->param_metadata);

  /* Audio streams */

  for(i = 0; i < t->num_audio_streams; i++)
    {
    label = bg_sprintf("Audio format %d", i+1);
    bg_dialog_add(ret->cfg_dialog,
                  label,
                  (bg_cfg_section_t*)0,
                  bg_transcoder_audio_stream_set_format_parameter,
                  &(t->audio_streams[i]),
                  bg_transcoder_audio_stream_get_parameters(&(t->audio_streams[i])));
    free(label);

    label = bg_sprintf("Audio encoder %d", i+1);
    bg_dialog_add(ret->cfg_dialog,
                  label,
                  (bg_cfg_section_t*)0,
                  bg_transcoder_audio_stream_set_encoder_parameter,
                  &(t->audio_streams[i]),
                  t->audio_streams[i].encoder_parameters);
    free(label);
    }

  /* Video streams */

  for(i = 0; i < t->num_video_streams; i++)
    {
    label = bg_sprintf("Video format %d", i+1);
    bg_dialog_add(ret->cfg_dialog,
                  label,
                  (bg_cfg_section_t*)0,
                  bg_transcoder_video_stream_set_format_parameter,
                  &(t->video_streams[i]),
                  bg_transcoder_video_stream_get_parameters(&(t->video_streams[i])));
    free(label);

    label = bg_sprintf("Video encoder %d", i+1);
    bg_dialog_add(ret->cfg_dialog,
                  label,
                  (bg_cfg_section_t*)0,
                  bg_transcoder_video_stream_set_encoder_parameter,
                  &(t->video_streams[i]),
                  t->video_streams[i].encoder_parameters);
    free(label);
    }
  
  
  return ret;
  
  }

void track_dialog_run(track_dialog_t * d)
  {
  bg_dialog_show(d->cfg_dialog);
  }

void track_dialog_destroy(track_dialog_t * d)
  {
  bg_dialog_destroy(d->cfg_dialog);
  }

