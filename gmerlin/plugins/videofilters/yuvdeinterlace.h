#include <plugin.h>

typedef struct yuvdeinterlacer_s yuvdeinterlacer_t;

yuvdeinterlacer_t * yuvdeinterlacer_create();

void yuvdeinterlacer_destroy(yuvdeinterlacer_t * d);

void yuvdeinterlacer_set_parameter(void * data, char * name, bg_parameter_value_t * val);

void yuvdeinterlacer_init(yuvdeinterlacer_t * di,
                          gavl_video_format_t * format);

void yuvdeinterlacer_get_output_format(yuvdeinterlacer_t * di,
                                       gavl_video_format_t * format);

void yuvdeinterlacer_connect_input(void * priv,
                                   bg_read_video_func_t func,
                                   void * data, int stream);

int yuvdeinterlacer_read(void * priv, gavl_video_frame_t * frame, int stream);


void yuvdeinterlacer_reset(yuvdeinterlacer_t * di);

int yuvdeinterlacer_need_restart(yuvdeinterlacer_t * di);

extern bg_parameter_info_t yuvdeinterlacer_parameters[];

