#ifndef TIMELINE_H
#define TIMELINE_H

typedef struct bg_nle_timeline_s bg_nle_timeline_t;

bg_nle_timeline_t * bg_nle_timeline_create(bg_nle_project_t*);
void bg_nle_timeline_destroy(bg_nle_timeline_t *);

GtkWidget * bg_nle_timeline_get_widget(bg_nle_timeline_t *);

void bg_nle_timeline_add_track(bg_nle_timeline_t *,
                               bg_nle_track_t * track);

#endif // TIMELINE_H
