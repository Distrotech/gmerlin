#include <stdlib.h>

#include <utils.h>
#include "gui.h"
#include <gui_gtk/gtkutils.h>
#include <gui_gtk/message.h>

static void nocards()
  {
  bg_gtk_message("Could not find any Alsa mixers.\nCheck your setup!\n", BG_GTK_MESSAGE_ERROR);
  exit(-1);
  }

int main(int argc, char ** argv)
  {
  char * tmp_path;
  alsa_mixer_t * mixer;
  mixer_window_t * mixer_window;
  bg_cfg_registry_t * cfg_reg;

  cfg_reg = bg_cfg_registry_create();
  
  tmp_path = bg_search_file_read("alsamixer", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
    
  //  alsa_mixer_dump(mixer);

  bg_gtk_init(&argc, &argv, "mixer_icon.png");

  mixer = alsa_mixer_create();

  if(!mixer)
    {
    nocards();
    }
  
  mixer_window = mixer_window_create(mixer, cfg_reg);
  
  mixer_window_run(mixer_window);

  mixer_window_destroy(mixer_window);
  alsa_mixer_destroy(mixer);
  
  /* Cleanup */

  tmp_path = bg_search_file_write("alsamixer", "config.xml");
  bg_cfg_registry_save(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  bg_cfg_registry_destroy(cfg_reg);
  
  return 0;
  }
