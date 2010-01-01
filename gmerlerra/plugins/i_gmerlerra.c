
#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <renderer.h>

static void * create_plugin()
  {
  return bg_nle_plugin_create(NULL, NULL);
  }

const bg_input_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "i_gmerlerra",
      .long_name =      TRS("Gmerlerra plugin"),
      .description =    TRS("Renderer for gmerlerra projects"),
      .type =           BG_PLUGIN_INPUT,
      .flags =          BG_PLUGIN_FILE,
      .priority =       BG_PLUGIN_PRIORITY_MIN,
      .create =         create_plugin,
      .destroy =        bg_nle_plugin_destroy,
      .get_parameters = bg_nle_plugin_get_parameters,
      .set_parameter =  bg_nle_plugin_set_parameter,
    },
    .get_extensions = bg_nle_plugin_get_extensions,
    /* Open file/device */
    .open = bg_nle_plugin_open,
    .set_callbacks = bg_nle_plugin_set_callbacks,
  /* For file and network plugins, this can be NULL */
    .get_num_tracks = bg_nle_plugin_get_num_tracks,
    //    .get_edl  = bg_avdec_get_edl,
    /* Return track information */
    .get_track_info = bg_nle_plugin_get_track_info,

    /* Set track */
    //    .set_track =             bg_avdec_set_track,
    /* Set streams */
    .set_audio_stream =      bg_nle_plugin_set_audio_stream,
    .set_video_stream =      bg_nle_plugin_set_video_stream,
    //    .set_subtitle_stream =   bg_avdec_set_subtitle_stream,

    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    .start =                 bg_nle_plugin_start,

    //    .get_frame_table =       bg_nle_plugin_get_frame_table,

    /* Read one audio frame (returns FALSE on EOF) */
    .read_audio =    bg_nle_plugin_read_audio,
    /* Read one video frame (returns FALSE on EOF) */

    //    .has_still  =      bg_avdec_has_still,
    .read_video =      bg_nle_plugin_read_video,
    .skip_video =      bg_nle_plugin_skip_video,

    //    .has_subtitle =          bg_avdec_has_subtitle,

    //    .read_subtitle_text =    bg_avdec_read_subtitle_text,
    //    .read_subtitle_overlay = bg_avdec_read_subtitle_overlay,

    /*
     *  Do percentage seeking (can be NULL)
     *  Media streams are supposed to be seekable, if this
     *  function is non-NULL AND the duration field of the track info
     *  is > 0
     */
    .seek = bg_nle_plugin_seek,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
