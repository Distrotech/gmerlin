
typedef struct
  {
  bg_gtk_plugin_widget_single_t * video_encoder;
  bg_gtk_plugin_widget_single_t * audio_encoder;

  bg_gtk_plugin_widget_single_t * subtitle_text_encoder;
  bg_gtk_plugin_widget_single_t * subtitle_overlay_encoder;
  
  GtkWidget * audio_to_video;
  GtkWidget * subtitle_text_to_video;

  GtkWidget * subtitle_overlay_to_video;
  
  GtkWidget * widget;

  GtkTooltips * tooltips;
  bg_plugin_registry_t * plugin_reg;

  } encoder_widget_t;

void encoder_widget_init(encoder_widget_t * ret, bg_plugin_registry_t * plugin_reg);

void encoder_widget_destroy(encoder_widget_t * wid);

void encoder_widget_update_sensitive(encoder_widget_t * win);

/* Encoder window */

typedef struct encoder_window_s encoder_window_t;

encoder_window_t * encoder_window_create(bg_plugin_registry_t * plugin_reg);

void encoder_window_run(encoder_window_t * win, bg_transcoder_track_t * tracks);
void encoder_window_destroy(encoder_window_t * win);
