typedef struct bg_lcdproc_s bg_lcdproc_t;

/* Create / destroy */

bg_lcdproc_t * bg_lcdproc_create(bg_player_t * player);
void bg_lcdproc_destroy(bg_lcdproc_t*);

/*
 *  Config stuff. The function set_parameter automatically
 *  starts and stops the thread
 */

bg_parameter_info_t * bg_lcdproc_get_parameters(bg_lcdproc_t *);
void bg_lcdproc_set_parameter(void * data, char * name,
                              bg_parameter_value_t * val);
