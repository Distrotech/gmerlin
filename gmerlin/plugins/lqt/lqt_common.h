#include <quicktime/lqt.h>
#include <quicktime/lqt_codecinfo.h>
#include <parameter.h>

void bg_lqt_create_codec_info(bg_parameter_info_t * parameter_info,
                              int audio, int video, int encode, int decode);

int bg_lqt_set_parameter(const char * name, bg_parameter_value_t * val,
                         bg_parameter_info_t * info);

