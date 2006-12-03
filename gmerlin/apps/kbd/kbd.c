
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "kbd.h"

void kbd_table_entry_free(kbd_table_t * kbd)
  {
  if(kbd->command) free(kbd->command);
  if(kbd->modifiers) free(kbd->modifiers);
  memset(kbd, 0, sizeof(*kbd));
  }

void kbd_table_destroy(kbd_table_t * kbd, int len)
  {
  int i;
  for(i = 0; i < len; i++)
    {
    kbd_table_entry_free(kbd + i);
    }
  if(kbd) free(kbd);
  }
