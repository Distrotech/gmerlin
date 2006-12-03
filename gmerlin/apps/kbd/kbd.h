
/* Common definitions */

typedef struct
  {
  uint32_t scancode;   /* X11 specific */
  uint32_t modifiers_i;   /* X11 specific */
  char     * modifiers;
  char     * command;
  } kbd_table_t;

/* Load/Save (xml) */

kbd_table_t * kbd_table_load(const char * filename, int * len);
void kbd_table_save(const char * filename, kbd_table_t *, int len);

/* Destroy */

void kbd_table_destroy(kbd_table_t *, int len);

void kbd_table_entry_free(kbd_table_t * kbd);
