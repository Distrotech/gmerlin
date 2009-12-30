#ifndef PROJECTWINDOW_H
#define PROJECTWINDOW_H
#include <gmerlin/pluginregistry.h>

typedef struct bg_nle_project_window_s bg_nle_project_window_t;

void
bg_nle_project_window_init_global(bg_cfg_registry_t * cfg_reg,
                                  bg_plugin_registry_t * plugin_reg);

bg_nle_project_window_t *
bg_nle_project_window_create(const char * project, bg_plugin_registry_t * plugin_reg);

void bg_nle_project_window_show(bg_nle_project_window_t *);
void bg_nle_project_window_destroy(bg_nle_project_window_t *);

extern int bg_nle_snap_to_frames;
extern int bg_nle_play_every_frame;


#endif
