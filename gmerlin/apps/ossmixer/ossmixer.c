#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <sys/soundcard.h>

#include <gui_gtk/audio.h>
#include <utils.h>

#include <pluginregistry.h>
#include <gui_gtk/gtkutils.h>

#include "monitor.h"

typedef struct oss_mixer_s
  {
  bg_gtk_mixer_control_t * controls[SOUND_MIXER_NRDEVICES];
  monitor_t * monitor;
  
  int mixer_fd;

  GtkWidget * widget;
      
  } oss_mixer_t;


static char * device_labels[] = SOUND_DEVICE_LABELS;
static char * device_names[]  = SOUND_DEVICE_NAMES;

void mixer_change(bg_gtk_mixer_control_t * ctrl, float * volume, void * data)
  {
  int i;
  unsigned int volume_i;
  
  oss_mixer_t * c = (oss_mixer_t *)data;
  /* fprintf(stderr, "mixer_change %f %f\n", volume[0], volume[1]); */
  for(i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    {
    if(ctrl == c->controls[i])
      {
      volume_i = (int)(100.0 * volume[0]) |
        (((int)(100.0 * volume[1])) << 8);
      if(ioctl(c->mixer_fd, MIXER_WRITE(i), &volume_i) == -1)
        fprintf(stderr, "ioctl failed\n");
      break;
      }
    }
  }

void mixer_set_record(bg_gtk_mixer_control_t * ctrl, int record, void * data)
  {
  int i;
  oss_mixer_t * c = (oss_mixer_t *)data;
  
  if(record)
    {
    /* Switch all other record sources off */
    for(i = 0; i < SOUND_MIXER_NRDEVICES; i++)
      {
      if(ctrl != c->controls[i])
        {
        if(c->controls[i])
          bg_gtk_mixer_control_set_record(c->controls[i], 0);
        }
      
      else
        ioctl(c->mixer_fd, SOUND_MIXER_WRITE_RECSRC, &i);
      }
    }
  else
    {
    for(i = 0; i < SOUND_MIXER_NRDEVICES; i++)
      {
      if(ctrl == c->controls[i])
        {
        break;
        }
      }
    }
  
  }

static bg_gtk_mixer_callbacks_t callbacks =
  {
    change:     mixer_change,
    set_record: mixer_set_record
  };

oss_mixer_t * create_mixer(const char * device,
                           bg_cfg_section_t * section)
  {
  int flags, i;
  int stereo_flags, record_flags;
  int num_controls, xpos;
  int stereo, record;
  bg_cfg_section_t * channel_section;
  bg_cfg_section_t * monitor_section;
  
  oss_mixer_t * ret = calloc(1, sizeof(*ret));

  if((ret->mixer_fd = open(device, 0)) == -1)
    {
    fprintf(stderr, "Cannot open device %s\n", device);
    goto fail;
    }

  if(ioctl(ret->mixer_fd, SOUND_MIXER_READ_DEVMASK, &flags) == -1)
    goto fail;
  if(ioctl(ret->mixer_fd, SOUND_MIXER_READ_STEREODEVS, &stereo_flags) == -1)
    goto fail;
  if(ioctl(ret->mixer_fd, SOUND_MIXER_READ_RECMASK, &record_flags) == -1)
    goto fail;
  
  num_controls = 0;
  for(i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    {
    if(flags & (1<<i))
      {
      stereo = !!(stereo_flags & (1<<i));
      record =  !!(record_flags & (1<<i));

      channel_section = bg_cfg_section_find_subsection(section,
                                                       device_names[i]);
      /*      fprintf(stderr, "Creating mixer %s...", device_labels[i]); */
      ret->controls[i] = bg_gtk_mixer_control_create(device_labels[i],
                                                     stereo, record,
                                                     &callbacks,
                                                     (void*)ret);
      bg_gtk_mixer_control_configure(ret->controls[i], channel_section);
      /*      fprintf(stderr, "done\n"); */
      num_controls++;
      }
    }

  ret->widget = gtk_table_new(1, num_controls + 1, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(ret->widget), 3);
  xpos = 0;
  for(i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    {
    if(ret->controls[i])
      {
      gtk_table_attach_defaults(GTK_TABLE(ret->widget),
                                bg_gtk_mixer_control_get_widget(ret->controls[i]),
                                xpos, xpos+1, 0, 1);
      xpos++;
      }
    
    }

  monitor_section = bg_cfg_section_find_subsection(section, "monitor");
  ret->monitor = monitor_create(monitor_section);
  
  gtk_table_attach_defaults(GTK_TABLE(ret->widget),
                            monitor_get_widget(ret->monitor),
                            xpos, xpos+1, 0, 1);
    
  gtk_widget_show(ret->widget);
  return ret;
  
fail:

  if(ret->mixer_fd != -1)
    close(ret->mixer_fd);
  free(ret);
  return (oss_mixer_t*)0;
  }

void destroy_mixer(oss_mixer_t * m)
  {
  int i;
  for(i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    {
    if(m->controls[i])
      {
      bg_gtk_mixer_control_destroy(m->controls[i]);
      }
    }
  free(m);
  }



typedef struct
  {
  oss_mixer_t ** mixers;
  int num_mixers;
  GtkWidget * notebook;
  GtkWidget * window;
  } mixer_window;

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  mixer_window * m = (mixer_window*)data;

  if(w == m->window)
    {
    gtk_widget_hide(m->window);
    gtk_main_quit();
    }
  return TRUE;
  }

mixer_window * mixer_window_create(bg_cfg_registry_t * reg)
  {
  const char * device;
  GtkWidget * label;
  int i;
  bg_cfg_section_t * mixers_section;
  bg_cfg_section_t * mixer_section;
  
  mixer_window * ret = calloc(1, sizeof(*ret));

  mixers_section = bg_cfg_registry_find_section(reg, "mixers");
  
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);
 
  ret->num_mixers = 1;
  ret->mixers = calloc(ret->num_mixers, sizeof(oss_mixer_t*));

  ret->notebook = gtk_notebook_new();

  for(i = 0; i < ret->num_mixers; i++)
    {
    device = "/dev/mixer";
    mixer_section = bg_cfg_section_find_subsection(mixers_section, device);
    ret->mixers[i] = create_mixer(device, mixer_section);
    label = gtk_label_new(device);
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(ret->notebook),
                             ret->mixers[i]->widget, label);
    }
  gtk_widget_show(ret->notebook);
  gtk_container_add(GTK_CONTAINER(ret->window), ret->notebook);
  return ret;
  }

static void mixer_window_destroy(mixer_window * w)
  {
  int i;
  for(i = 0; i < w->num_mixers; i++)
    {
    destroy_mixer(w->mixers[i]);
    }
  free(w->mixers);
  gtk_widget_destroy(w->window);
  free(w);
  }

int main(int argc, char ** argv)
  {
  mixer_window * w;
  bg_cfg_registry_t * reg;
  bg_cfg_section_t * plugin_section;
  char * cfg_file;

  reg = bg_cfg_registry_create();
  
  cfg_file = bg_search_file_read("ossmixer", "config.xml");

  if(cfg_file)
    {
    fprintf(stderr, "Opening config file %s\n", cfg_file);
    bg_cfg_registry_load(reg, cfg_file);
    free(cfg_file);
    }

  plugin_section = bg_cfg_registry_find_section(reg, "plugins");
    
  bg_gtk_init(&argc, &argv);

  w = mixer_window_create(reg);

  gtk_widget_show(w->window);
  gtk_main();

  mixer_window_destroy(w);
    
  cfg_file = bg_search_file_write("ossmixer", "config.xml");

  if(cfg_file)
    {
    fprintf(stderr, "Saving %s\n", cfg_file);
    bg_cfg_registry_save(reg, cfg_file);
    free(cfg_file);
    }
  else
    fprintf(stderr, "cannot open config file\n");

  bg_cfg_registry_destroy(reg);

    
  return 0;
  }
