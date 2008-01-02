
typedef struct bg_colormatrix_s bg_colormatrix_t;

bg_colormatrix_t * bg_colormatrix_create();

void bg_colormatrix_destroy(bg_colormatrix_t *);

void bg_colormatrix_set_rgba(bg_colormatrix_t *, float coeffs[4][5]);
void bg_colormatrix_set_yuva(bg_colormatrix_t *, float coeffs[4][5]);

void bg_colormatrix_set_rgb(bg_colormatrix_t *, float coeffs[3][4]);
void bg_colormatrix_set_yuv(bg_colormatrix_t *, float coeffs[3][4]);

/* Constants for flags */
#define BG_COLORMATRIX_FORCE_ALPHA (1<<0)

void bg_colormatrix_init(bg_colormatrix_t *,
                         gavl_video_format_t * format, int flags);

void bg_colormatrix_process(bg_colormatrix_t *,
                            gavl_video_frame_t * frame);

