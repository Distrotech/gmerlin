
#include <gavl/gavf.h>
#include <gavl/connectors.h>

#include <gmerlin/parameter.h>

/* Reader */

typedef struct bg_plug_s bg_plug_t;

bg_plug_t * bg_plug_create_reader(void);

bg_plug_t * bg_plug_create_writer(void);

void bg_plug_destroy(bg_plug_t *);

const bg_parameter_info_t *
bg_plug_get_input_parameters(bg_plug_t * p);

const bg_parameter_info_t *
bg_plug_get_output_parameters(bg_plug_t * p);

void bg_plug_set_parameter(void * data, const char * name,
                           const bg_parameter_value_t * val);

int bg_plug_open(bg_plug_t *, const char * location);

gavf_t * bg_plug_reader_get_gavf(bg_plug_t *);

int bg_plug_start_write(bg_plug_t * p);

/* Return the header of the stream the next packet belongs to */
const gavf_stream_header_t * bg_plug_next_packet_header(bg_plug_t * p);

int bg_plug_get_stream_source(bg_plug_t * p, const gavf_stream_header_t *,
                              gavl_audio_source_t ** as,
                              gavl_video_source_t ** vs,
                              gavl_packet_source_t ** ps);


int bg_plug_get_stream_sink(bg_plug_t * p, const gavf_stream_header_t *,
                            gavl_audio_sink_t ** as,
                            gavl_video_sink_t ** vs,
                            gavl_packet_sink_t ** ps);


