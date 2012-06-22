#include <limits.h>
#include <inttypes.h>

typedef struct
  {
  uint8_t * addr;
  int size;
  char name[NAME_MAX];
  } bg_shm_t;

bg_shm_t * bg_shm_alloc_write(int size);
bg_shm_t * bg_shm_alloc_read(const char * name, int size);

void bg_shm_free(bg_shm_t*);
