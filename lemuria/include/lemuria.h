
#include <inttypes.h>

#define LEMURIA_TIME_SAMPLES 512
#define LEMURIA_FREQ_SAMPLES 256

typedef struct lemuria_engine_s lemuria_engine_t;

lemuria_engine_t * lemuria_create(void);

void lemuria_set_size(lemuria_engine_t *, int width, int height);

void lemuria_draw_frame(lemuria_engine_t *);

/* Update display */

void lemuria_wait(lemuria_engine_t *);


/* Add audio samples (can be called from another thread) */

void lemuria_update_audio(lemuria_engine_t * e, int16_t * channels[2]);

/* Destroy engine */

void lemuria_destroy(lemuria_engine_t *);

void lemuria_set_background(lemuria_engine_t * engine);

void lemuria_set_texture(lemuria_engine_t * engine);

void lemuria_set_foreground(lemuria_engine_t * engine);

void lemuria_next_background(lemuria_engine_t * engine);

void lemuria_next_texture(lemuria_engine_t * engine);

void lemuria_next_foreground(lemuria_engine_t * engine);

#define LEMURIA_ANTIALIAS_NONE 0
#define LEMURIA_ANTIALIAS_BEST 2

void lemuria_set_antialiasing(lemuria_engine_t * engine, int antialiasing);

void lemuria_print_help(lemuria_engine_t * e);
