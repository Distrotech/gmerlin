

typedef struct bg_vloopback_s bg_vloopback_t;

bg_vloopback_t * bg_vloopback_create();
void bg_vloopback_destroy(bg_vloopback_t *);

int bg_vloopback_open(bg_vloopback_t *);
void bg_vloopback_close(bg_vloopback_t *);

void bg_vloopback_set_format(bg_vloopback_t *, const gavl_video_format_t * format);
void bg_vloopback_put_frame(bg_vloopback_t *, gavl_video_frame_t * frame);

const bg_parameter_info_t * bg_vloopback_get_parameters(bg_vloopback_t *);
void bg_vloopback_set_parameter(void * data, const char * name,
                                const bg_parameter_value_t * val);

int bg_vloopback_changed(bg_vloopback_t *);
