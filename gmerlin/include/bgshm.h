#include <limits.h>
#include <inttypes.h>

typedef struct bg_shm_s bg_shm_t;

bg_shm_t * bg_shm_alloc_write(int size);
bg_shm_t * bg_shm_alloc_read(int id, int size);

uint8_t * bg_shm_get_buffer(bg_shm_t * s, int * size);
int bg_shm_get_id(bg_shm_t * s);

void bg_shm_ref(bg_shm_t * s);
void bg_shm_unref(bg_shm_t * s);
int bg_shm_refcount(bg_shm_t * s);

void bg_shm_free(bg_shm_t*);

/* Shared memory pool */

typedef struct bg_shm_pool_s bg_shm_pool_t;

bg_shm_pool_t * bg_shm_pool_create(int seg_size, int wr);
void bg_shm_pool_destroy(bg_shm_pool_t *);


/* Get a shared memory segment for reading. */
bg_shm_t * bg_shm_pool_get_read(bg_shm_pool_t *, int id);
bg_shm_t * bg_shm_pool_get_write(bg_shm_pool_t *);

