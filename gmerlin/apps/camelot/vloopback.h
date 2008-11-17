

typedef struct bg_vloopback_s bg_vloopback_t;

bg_vloopback_t * bg_vloopback_create();
void bg_vloopback_destroy(bg_vloopback_t *);

int bg_vloopback_open(bg_vloopback_t *, const char * device);
void bg_vloopback_close(bg_vloopback_t *);

void bg_vloopback_set_format(bg_vloopback_t *, const gavl_video_format_t * format);
void bg_vloopback_put_frame(bg_vloopback_t *, gavl_video_frame_t * frame);

