
typedef struct gmerlin_webcam_window_s gmerlin_webcam_window_t;

gmerlin_webcam_window_t *
gmerlin_webcam_window_create(gmerlin_webcam_t * w,
                             bg_plugin_registry_t * reg,
                             bg_cfg_section_t * section);
void
gmerlin_webcam_window_destroy(gmerlin_webcam_window_t * w);

void
gmerlin_webcam_window_show(gmerlin_webcam_window_t * w);


bg_parameter_info_t *
gmerlin_webcam_window_get_parameters(gmerlin_webcam_window_t * w);

int
gmerlin_webcam_window_get_parameter(void * priv, char * name,
                                    bg_parameter_value_t * val);

void
gmerlin_webcam_window_set_parameter(void * priv, char * name,
                                    bg_parameter_value_t * val);


