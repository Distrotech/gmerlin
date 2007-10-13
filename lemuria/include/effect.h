/* typedef struct technosophy_engine_s technosophy_engine_t */

struct effect_plugin_s
  {
  void * (*init)();
  void (*draw)(lemuria_engine_t *, void * data);
  void (*cleanup)(void*);
  };

void lemuria_set_background(lemuria_engine_t * engine);

void lemuria_set_texture(lemuria_engine_t * engine);

void lemuria_set_foreground(lemuria_engine_t * engine);

void lemuria_next_background(lemuria_engine_t * engine);

void lemuria_next_texture(lemuria_engine_t * engine);

void lemuria_next_foreground(lemuria_engine_t * engine);

/* Manage effects (effect.c) */

void lemuria_manage_effects(lemuria_engine_t * engine);

