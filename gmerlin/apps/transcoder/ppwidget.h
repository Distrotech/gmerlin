
typedef struct
  {
  bg_gtk_plugin_widget_single_t * plugins;
  GtkWidget * pp;

  GtkWidget * widget;

  GtkTooltips * tooltips;
  bg_plugin_registry_t * plugin_reg;
  } encoder_pp_widget_t;

void encoder_pp_widget_init(encoder_pp_widget_t * ret, bg_plugin_registry_t * plugin_reg);

void encoder_pp_widget_destroy(encoder_pp_widget_t * wid);

/* Encoder window */

typedef struct encoder_pp_window_s encoder_pp_window_t;

encoder_pp_window_t * encoder_pp_window_create(bg_plugin_registry_t * plugin_reg);
void encoder_pp_window_run(encoder_pp_window_t * win, bg_transcoder_track_global_t * global);
void encoder_pp_window_destroy(encoder_pp_window_t * win);
