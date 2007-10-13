
// #include <GL/glx.h>
#include <inttypes.h>
#include <pthread.h>

#include <gavl/gavl.h>

#define LEMURIA_TIME_SAMPLES 512
#define LEMURIA_FREQ_SAMPLES 256

typedef struct lemuria_engine_s lemuria_engine_t;

lemuria_engine_t *
lemuria_create(const char * display_string, int width, int height);

void lemuria_draw_frame(lemuria_engine_t *);

/* Update display */

void lemuria_update(lemuria_engine_t *);


void lemuria_wait(lemuria_engine_t *);


/* Add audio samples (can be called from another thread) */

void lemuria_update_audio(lemuria_engine_t * e, gavl_audio_frame_t * f);

void lemuria_adjust_format(lemuria_engine_t * e,
                           gavl_audio_format_t * format);

/* Start thread */

void lemuria_start(lemuria_engine_t *);

/* Stop thread */

void lemuria_stop(lemuria_engine_t *);

/* Destroy engine */

void lemuria_destroy(lemuria_engine_t *);


void lemuria_check_events(lemuria_engine_t * e);

void lemuria_flash_frame(lemuria_engine_t * e);

