#include <medialist.h>

typedef struct bg_nle_media_browser_s bg_nle_media_browser_t;

bg_nle_media_browser_t *
bg_nle_media_browser_create(bg_nle_project_t*);

void bg_nle_media_browser_destroy(bg_nle_media_browser_t*);

void bg_nle_media_browser_load_files(bg_nle_media_browser_t*);

GtkWidget *
bg_nle_media_browser_get_widget(bg_nle_media_browser_t*);


void bg_nle_media_browser_add_file(bg_nle_media_browser_t * b, int index);
void bg_nle_media_browser_delete_file(bg_nle_media_browser_t * b, int index);
