#include <gtk/gtk.h>
#include "alsamixer.h"
#include <cfg_registry.h>

typedef struct card_widget_s card_widget_t;

/* Control widget */

typedef struct control_widget_s control_widget_t;

control_widget_t * control_widget_create(alsa_mixer_group_t * c,
                                         bg_cfg_section_t * section,
                                         card_widget_t * card);

void control_widget_destroy(control_widget_t *);

GtkWidget * control_widget_get_widget(control_widget_t *);

int control_widget_is_upper(control_widget_t *);

int control_widget_get_index(control_widget_t *);
void control_widget_set_index(control_widget_t *, int);

int control_widget_get_own_window(control_widget_t *);
void control_widget_set_own_window(control_widget_t *, int);


void control_widget_set_parameter(void * data, char * name,
                                  bg_parameter_value_t * v);

int control_widget_get_parameter(void * data, char * name,
                                 bg_parameter_value_t * v);

void control_widget_read_config(control_widget_t *);
void control_widget_write_config(control_widget_t *);

void control_widget_set_hidden(control_widget_t *, int hidden);
int control_widget_get_hidden(control_widget_t *);

const char * control_widget_get_label(control_widget_t *);

/* Coords are handled by the card widgets, but the control widgets store them */

void control_widget_get_coords(control_widget_t *, int * x, int * y, int * width, int * height);
void control_widget_set_coords(control_widget_t *, int x, int y, int width, int height);

/* Card widget */


card_widget_t * card_widget_create(alsa_card_t * c, bg_cfg_section_t * section);

void card_widget_destroy(card_widget_t *);

GtkWidget * card_widget_get_widget(card_widget_t *);

int card_widget_num_upper_controls(card_widget_t *);
int card_widget_num_lower_controls(card_widget_t *);

void card_widget_move_control_left(card_widget_t*, control_widget_t*);
void card_widget_move_control_right(card_widget_t*, control_widget_t*);
void card_widget_move_control_first(card_widget_t*, control_widget_t*);
void card_widget_move_control_last(card_widget_t*, control_widget_t*);

void card_widget_tearoff_control(card_widget_t*, control_widget_t*);
void card_widget_tearon_control(card_widget_t*, control_widget_t*);

void card_widget_get_window_coords(card_widget_t*);

void card_widget_configure(card_widget_t *);


/* Mixer window */

typedef struct mixer_window_s mixer_window_t;

mixer_window_t * mixer_window_create(alsa_mixer_t * mixer,
                                     bg_cfg_registry_t * cfg_reg);
void mixer_window_destroy(mixer_window_t *);
void mixer_window_run(mixer_window_t *);

