
typedef struct track_dialog_s track_dialog_t;

track_dialog_t * track_dialog_create(bg_transcoder_track_t * t,
                                     void (*set_name)(void * priv, const char * name),
                                     void * priv);

void track_dialog_run(track_dialog_t *);
void track_dialog_destroy(track_dialog_t *);
