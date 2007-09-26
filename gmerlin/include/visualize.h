
typedef struct bg_visualizer_s bg_visualizer_t;

bg_visualizer_t * bg_visualizer_create(bg_plugin_registry_t * plugin_reg);

void bg_visualizer_destroy(bg_visualizer_t *);

bg_parameter_info_t * bg_visualizer_get_parameters(bg_visualizer_t*);

void bg_visualizer_set_parameter(void * priv,
                                 const char * name,
                                 const bg_parameter_value_t * v);

void bg_visualizer_open(bg_visualizer_t * v,
                        const gavl_audio_format_t * format,
                        bg_plugin_handle_t * ov_handle);

void bg_visualizer_update(bg_visualizer_t * v, gavl_audio_frame_t *);

void bg_visualizer_close(bg_visualizer_t * v);
