#include <string.h>

#include <gmerlin_mozilla.h>
#include <gmerlin/utils.h>

void bg_mozilla_embed_info_set_parameter(bg_mozilla_embed_info_t * e,
                                         const char * name,
                                         const char * value)
  {
  if(!strcmp(name, "src"))
    e->src = bg_strdup(e->src, value);
  else if(!strcmp(name, "target"))
    e->target = bg_strdup(e->target, value);
  else if(!strcmp(name, "type"))
    e->type = bg_strdup(e->type, value);
  else if(!strcmp(name, "id"))
    e->id = bg_strdup(e->id, value);
  else if(!strcmp(name, "controls"))
    e->controls = bg_strdup(e->controls, value);
  }

int bg_mozilla_embed_info_check(bg_mozilla_embed_info_t * e)
  {
  fprintf(stderr, "Mode: %d, target: %s\n", 
          e->mode, e->target);
  if((e->mode == MODE_REAL) && e->controls &&
     strcasecmp(e->controls, "imagewindow"))
    return 0;
  //  if((e->mode == MODE_VLC) && !e->target)
  //    return 1;
  return 1;
  }

void bg_mozilla_embed_info_free(bg_mozilla_embed_info_t * e)
  {
  if(e->src)    free(e->src);
  if(e->target) free(e->target);
  if(e->type)   free(e->type);
  if(e->id)   free(e->id);
  if(e->controls)   free(e->controls);
  }
