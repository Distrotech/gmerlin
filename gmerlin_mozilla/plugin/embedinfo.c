#include <string.h>

#include <gmerlin_mozilla.h>
#include <gmerlin/utils.h>

void bg_mozilla_embed_info_set_parameter(bg_mozilla_embed_info_t * e,
                                         const char * name,
                                         const char * value)
  {
  if(!strcmp(name, "src"))
    e->src = bg_strdup(e->src, value);
  }

int bg_mozilla_embed_info_check(bg_mozilla_embed_info_t * e)
  {
  if((e->mode = MODE_REAL) && !e->src)
    return 0;
  return 1;
  }

void bg_mozilla_embed_info_free(bg_mozilla_embed_info_t * e)
  {
  if(e->src) free(e->src);
  }
