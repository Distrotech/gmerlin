#include <gtk/gtk.h>
#include <gui_gtk/audio.h>
#include <gui_gtk/gtkutils.h>

#include <pluginregistry.h>
#include <utils.h>

typedef struct
  {
  bg_ra_plugin_t * ra_plugin;
  bg_plugin_handle_t * ra_handle;
  gavl_audio_frame_t * frame;
  bg_gtk_vumeter_t * meter;
  gavl_audio_format_t format;
  }
idle_data_t;

static gboolean idle_callback(gpointer data)
  {
  idle_data_t * id = (idle_data_t *)data;
  
  id->ra_plugin->read_frame(id->ra_handle->priv,
                            id->frame,
                            id->format.samples_per_frame);
  
  bg_gtk_vumeter_update(id->meter, id->frame);
  return TRUE;
  }

int main(int argc, char ** argv)
  {
  GtkWidget * window;
  bg_plugin_registry_t * plugin_reg;
  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;
  char * tmp_path;
  idle_data_t id;
  const bg_plugin_info_t * info;
  
  bg_gtk_init(&argc, &argv, (char*)0);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  
  id.meter = bg_gtk_vumeter_create(2, 1);
  
  gtk_container_add(GTK_CONTAINER(window),
                    bg_gtk_vumeter_get_widget(id.meter));

  /* Create plugin registry */
  
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  /* Load and open plugin */

  info = bg_plugin_registry_get_default(plugin_reg,
                                        BG_PLUGIN_RECORDER_AUDIO);
  
  id.ra_handle = bg_plugin_load(plugin_reg, info);
  id.ra_plugin = (bg_ra_plugin_t*)(id.ra_handle->plugin);
  
  /* The soundcard might be busy from last time,
     give the kernel some time to free the device */
  
  if(!id.ra_plugin->open(id.ra_handle->priv, &id.format))
    {
    fprintf(stderr, "Couldn't open audio device");
    return -1;
    }
  /* */
  id.frame = gavl_audio_frame_create(&id.format);

  bg_gtk_vumeter_set_format(id.meter, &id.format);
  
  g_idle_add(idle_callback, &id);
  
  gtk_widget_show(window);

  gtk_main();

  return 0;
  }
