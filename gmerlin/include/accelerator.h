#ifndef __ACCELERATOR_H_
#define __ACCELERATOR_H_

typedef struct
  {
  int key;   /* See keycodes.h */
  int mask;  /* See keycodes.h */
  int id;    /* Choosen by the application */
  } bg_accelerator_t;

typedef struct bg_accelerator_map_s bg_accelerator_map_t;

void
bg_accelerator_map_append(bg_accelerator_map_t * m,
                          int key, int mask, int id);

/* Array is terminated with BG_KEY_NONE */

void
bg_accelerator_map_append_array(bg_accelerator_map_t * m,
                                const bg_accelerator_t * tail);

void
bg_accelerator_map_destroy(bg_accelerator_map_t * m);

bg_accelerator_map_t *
bg_accelerator_map_create();

void
bg_accelerator_map_remove(bg_accelerator_map_t * m, int id);

void
bg_accelerator_map_clear(bg_accelerator_map_t * m);


int
bg_accelerator_map_has_accel(const bg_accelerator_map_t * m,
                             int key, int mask, int * id);

int
bg_accelerator_map_has_accel_with_id(const bg_accelerator_map_t * m,
                                     int id);


const bg_accelerator_t * 
bg_accelerator_map_get_accels(const bg_accelerator_map_t * m);

#endif // __ACCELERATOR_H_
