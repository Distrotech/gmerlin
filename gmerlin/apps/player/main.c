/* System includes */
#include <stdlib.h>

#include <gtk/gtk.h>

/* Gmerlin includes */

#include <player.h>

#include "gmerlin.h"

#include <utils.h>
#include <gui_gtk/gtkutils.h>

int main(int argc, char ** argv)
  {
  gmerlin_t * gmerlin;
  bg_cfg_registry_t * cfg_reg;
  char * tmp_path;

  cfg_reg = bg_cfg_registry_create();
  
  tmp_path = bg_search_file_read("player", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);


  /* Fire up the actual player */

  bg_gtk_init(&argc, &argv);
  
  gmerlin = gmerlin_create(cfg_reg);

  gmerlin_run(gmerlin);
  
  gmerlin_destroy(gmerlin);

  tmp_path =  bg_search_file_write("player", "config.xml");

  bg_cfg_registry_save(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  bg_cfg_registry_destroy(cfg_reg);
  return 0;
  }

