#include "gui.h"

struct mixer_window_s
  {
  GtkWidget * main_win;
  //  GtkWidget * aux_win;

  int num_cards;
  card_widget_t ** cards;
  bg_cfg_registry_t * cfg_reg;
  };

static gboolean delete_callback(GtkWidget * w, GdkEventAny * evt,
                                gpointer data)
  {
  gtk_main_quit();
  return TRUE;
  }

mixer_window_t * mixer_window_create(alsa_mixer_t * mixer, bg_cfg_registry_t * cfg_reg)
  {
  int i;
  GtkWidget * notebook;
  GtkWidget * label;
  bg_cfg_section_t * section;
  mixer_window_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->cfg_reg = cfg_reg;
  ret->main_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_signal_connect(G_OBJECT(ret->main_win),
                   "delete-event", G_CALLBACK(delete_callback),
                   ret);
    
  ret->cards = calloc(mixer->num_cards, sizeof(*(ret->cards)));
  
  notebook = gtk_notebook_new();
  ret->num_cards = mixer->num_cards;
  for(i = 0; i < mixer->num_cards; i++)
    {
    section = bg_cfg_registry_find_section(ret->cfg_reg, mixer->cards[i]->name);
    label = gtk_label_new(mixer->cards[i]->name);
    gtk_widget_show(label);
    ret->cards[i] = card_widget_create(mixer->cards[i], section);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                             card_widget_get_widget(ret->cards[i]),
                             label);
    }
  
  gtk_widget_show(notebook);
  gtk_container_add(GTK_CONTAINER(ret->main_win), notebook);
  
  return ret;
  }

void mixer_window_destroy(mixer_window_t * w)
  {
  int i;
  for(i = 0; i < w->num_cards; i++)
    card_widget_destroy(w->cards[i]);
  
  free(w);
  }

void mixer_window_run(mixer_window_t * w)
  {
  gtk_widget_show(w->main_win);
  gtk_main();
  }
