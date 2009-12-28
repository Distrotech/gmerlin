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

void bg_nle_media_browser_set_oa_parameter(void * data, const char * name,
                                           const bg_parameter_value_t * val);
void bg_nle_media_browser_set_ov_parameter(void * data, const char * name,
                                           const bg_parameter_value_t * val);

void bg_nle_media_browser_set_display_parameter(void * data, const char * name,
                                                const bg_parameter_value_t * val);
