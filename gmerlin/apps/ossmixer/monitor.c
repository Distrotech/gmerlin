#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <utils.h>
#include <pluginregistry.h>
#include <gui_gtk/audio.h>

#include "monitor.h"

// Multithreaded operation

//#define RECORD_THREAD 

struct monitor_s
  {
  /* Monitor stuff */
  
  bg_plugin_registry_t   * plugin_registry;
  bg_plugin_handle_t     * plugin_handle;

  bg_gtk_vumeter_t       * vumeter;

  gavl_audio_frame_t     * frame;
  gavl_audio_converter_t * converter;
  GtkWidget * widget;

  GtkWidget * config_button;
  GtkWidget * mute_button;
  int do_monitor;
  gavl_audio_format_t format;
  int timeout_running;
  
#ifdef RECORD_THREAD
  pthread_t record_thread;
#endif
  
  };


/* Utility functions */

static GtkWidget * create_pixmap_button(const char * filename)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", filename);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();

  gtk_widget_show(image);
  button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);
  return button;
  }

static GtkWidget * create_pixmap_toggle_button(const char * filename)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", filename);

  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();
    
  gtk_widget_show(image);
  button = gtk_toggle_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);
  return button;
  }

/* Recording theread */
#ifdef RECORD_THREAD

static void * thread_func(void * data)
  {
  bg_ra_plugin_t * plugin;
  monitor_t * m =(monitor_t*)data;
  plugin = (bg_ra_plugin_t *)m->plugin_handle->plugin;

  fprintf(stderr, "Starting record thread\n");
  
  while(1)
    {
    plugin->read_samples(m->plugin_handle->priv,
                         m->frame);
    bg_gtk_vumeter_update(m->vumeter, m->frame);
    }
  return (void*)0;
  }
#endif

static gboolean idle_func(gpointer data)
  {
  monitor_t * m;
#ifndef RECORD_THREAD
  bg_ra_plugin_t * plugin;
#endif
  m = (monitor_t*)data;
#ifndef RECORD_THREAD
  if(m->plugin_handle)
    {
    plugin = (bg_ra_plugin_t *)m->plugin_handle->plugin;
    plugin->read_frame(m->plugin_handle->priv,
                       m->frame);
    }
  else
    {
    gavl_audio_frame_mute(m->frame, &m->format);
    }
  
  bg_gtk_vumeter_update(m->vumeter, m->frame);
#endif
  
  bg_gtk_vumeter_draw(m->vumeter);
  
  //  if(!m->do_monitor)
  //    return FALSE;
  //  else
  return TRUE;
  }

static void start_monitor(monitor_t * m)
  {
  const bg_plugin_info_t * info;
  guint interval;
  
  bg_ra_plugin_t * plugin;
  
  info = bg_plugin_find_by_name(m->plugin_registry, "i_oss");
  if(!info)
    {
    fprintf(stderr, "No OSS recorder found\n");
    return;
    }
  m->plugin_handle =
    bg_plugin_load(m->plugin_registry, info);
            
  if(!m->plugin_handle)
    {
    fprintf(stderr, "Cannot load OSS recorder plugin\n");
    return;
    }

  /* Open the plugin and get the audio format */

  plugin = (bg_ra_plugin_t *)m->plugin_handle->plugin;

  if(!plugin->open(m->plugin_handle->priv, &(m->format)))
    {
    fprintf(stderr, "Cannot open OSS recorder plugin\n");
    return;
    }
  m->frame = gavl_audio_frame_create(&(m->format));

  /* Choose timeout interval */

  bg_gtk_vumeter_set_format(m->vumeter, &m->format);

  if(!m->timeout_running)
    {
    interval =
      (guint)((float)m->format.samples_per_frame * 800.0 /
              (float)m->format.samplerate);
    //  interval = 20;
    
    fprintf(stderr, "timeout: %d\n", interval);

    g_timeout_add(interval, idle_func, (gpointer)m);
    m->timeout_running = 1;
    }
  
#ifdef RECORD_THREAD
  pthread_create(&(m->record_thread), (pthread_attr_t*)0,
                 &thread_func, (void*)m);
#endif
  }

static void stop_monitor(monitor_t * m)
  {
  bg_ra_plugin_t * plugin;

#ifdef RECORD_THREAD
  pthread_cancel(m->record_thread);
  pthread_join(m->record_thread, (void**)0);
#endif
  if(m->plugin_handle)
    {
    plugin = (bg_ra_plugin_t*)(m->plugin_handle->plugin);

    plugin->close(m->plugin_handle->priv);
    bg_plugin_unref(m->plugin_handle);
    m->plugin_handle = (bg_plugin_handle_t*)0;
    }
  bg_gtk_vumeter_reset_overflow(m->vumeter);
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  monitor_t * m =(monitor_t*)data;
  
  
  if(w == m->config_button)
    {
    
    }
  else if(w == m->mute_button)
    {
    m->do_monitor =
      !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->mute_button));

    if(m->do_monitor)
      {
      start_monitor(m);
      }
    else
      {
      stop_monitor(m);
      }
    }
  
  }

monitor_t * monitor_create(bg_cfg_section_t * section)
  {
  GtkWidget * table;
  GtkWidget * label;
  bg_cfg_section_t * plugin_section;

  monitor_t * ret = calloc(1, sizeof(*ret));

  plugin_section = bg_cfg_section_find_subsection(section, "plugins");
    
  ret->plugin_registry = bg_plugin_registry_create(plugin_section);
  
  ret->vumeter = bg_gtk_vumeter_create(2);

  ret->config_button = create_pixmap_button("config_16.png");
  ret->mute_button = create_pixmap_toggle_button("mute_16.png");

  g_signal_connect(G_OBJECT(ret->config_button),
                   "clicked", G_CALLBACK(button_callback),
                   ret);
  g_signal_connect(G_OBJECT(ret->mute_button),
                   "toggled", G_CALLBACK(button_callback),
                   ret);
  
  gtk_widget_show(ret->config_button);
  gtk_widget_show(ret->mute_button);
  
  table = gtk_table_new(3, 2, 0);
  
  label = gtk_label_new("Monitor");
  gtk_widget_show(label);
    
  gtk_table_attach(GTK_TABLE(table),
                   label, 0, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach_defaults(GTK_TABLE(table),
                            bg_gtk_vumeter_get_widget(ret->vumeter),
                            0, 2, 1, 2);

  gtk_table_attach(GTK_TABLE(table),
                   ret->config_button, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   ret->mute_button,   1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_widget_show(table);
 
  ret->widget = gtk_handle_box_new();
  gtk_handle_box_set_handle_position(GTK_HANDLE_BOX(ret->widget),
                                     GTK_POS_TOP);
  
  gtk_container_add(GTK_CONTAINER(ret->widget), table);
  gtk_widget_show(ret->widget);
  return ret;
  }

GtkWidget * monitor_get_widget(monitor_t * m)
  {
  return m->widget;
  }

