typedef struct monitor_s monitor_t;

monitor_t * monitor_create(bg_cfg_section_t * section);

GtkWidget * monitor_get_widget(monitor_t *);

void monitor_destroy(monitor_t *);

