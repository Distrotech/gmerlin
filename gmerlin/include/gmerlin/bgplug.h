/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#ifndef __BGPLUG_H_
#define __BGPLUG_H_

#include <gavl/gavf.h>
#include <gavl/connectors.h>

#include <gmerlin/parameter.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/mediaconnector.h>

/* Reader */

typedef struct bg_plug_s bg_plug_t;

bg_plug_t * bg_plug_create_reader(bg_plugin_registry_t * plugin_reg);
bg_plug_t * bg_plug_create_writer(bg_plugin_registry_t * plugin_reg);

void bg_plug_destroy(bg_plug_t *);

const bg_parameter_info_t *
bg_plug_get_input_parameters();

const bg_parameter_info_t *
bg_plug_get_output_parameters();

void bg_plug_set_parameter(void * data, const char * name,
                           const bg_parameter_value_t * val);

int bg_plug_open(bg_plug_t *, gavf_io_t * io,
                 const gavl_metadata_t * m,
                 const gavl_chapter_list_t * cl, int io_flags);

int bg_plug_set_from_ph(bg_plug_t *,
                        const gavf_program_header_t * ph);

int bg_plug_open_location(bg_plug_t * p, const char * location,
                          const gavl_metadata_t * m,
                          const gavl_chapter_list_t * cl);


gavf_t * bg_plug_get_gavf(bg_plug_t*);

const gavl_metadata_t * bg_plug_get_metadata(bg_plug_t*);
const gavl_chapter_list_t * bg_plug_get_chapter_list(bg_plug_t*);

/* Initialization function for readers and writers */

int bg_plug_add_mediaconnector_stream(bg_plug_t * p,
                                      bg_mediaconnector_stream_t * s);

int bg_plug_connect_mediaconnector_stream(bg_plug_t * p,
                                          bg_mediaconnector_stream_t * s,
                                          const gavf_stream_header_t * h);
/* Set up a writer from a media connector */
int bg_plug_setup_writer(bg_plug_t*, bg_mediaconnector_t * conn);

/* Set up a media connector from a reader */
int bg_plug_setup_reader(bg_plug_t*, bg_mediaconnector_t * conn);


/* Needs to be called before any I/O is done */
int bg_plug_start(bg_plug_t * p);

/* Return the header of the stream the next packet belongs to */
const gavf_stream_header_t * bg_plug_next_packet_header(bg_plug_t * p);

/* Add streams for encoding, on the fly compression might get applied */

int bg_plug_add_audio_stream(bg_plug_t * p,
                             const gavl_compression_info_t * ci,
                             const gavl_audio_format_t * format,
                             const gavl_metadata_t * m,
                             bg_cfg_section_t * encode_section);

int bg_plug_add_video_stream(bg_plug_t * p,
                             const gavl_compression_info_t * ci,
                             const gavl_video_format_t * format,
                             const gavl_metadata_t * m,
                             bg_cfg_section_t * encode_section);

int bg_plug_add_overlay_stream(bg_plug_t * p,
                               const gavl_compression_info_t * ci,
                               const gavl_video_format_t * format,
                               const gavl_metadata_t * m,
                               bg_cfg_section_t * encode_section);

/* Proxy stuff */

void bg_plug_set_audio_proxy(bg_plug_t * p, int stream,
                             gavl_audio_source_t * asrc,
                             gavl_packet_source_t * psrc);

void bg_plug_set_video_proxy(bg_plug_t * p, int stream,
                             gavl_video_source_t * vsrc,
                             gavl_packet_source_t * psrc);

void bg_plug_set_overlay_proxy(bg_plug_t * p, int stream,
                               gavl_video_source_t * vsrc,
                               gavl_packet_source_t * psrc);

void bg_plug_set_text_proxy(bg_plug_t * p, int stream,
                            gavl_video_source_t * vsrc,
                            gavl_packet_source_t * psrc);
                

int bg_plug_add_text_stream(bg_plug_t * p,
                            uint32_t timescale,
                            const gavl_metadata_t * m);

int bg_plug_get_stream_source(bg_plug_t * p,
                              const gavf_stream_header_t * h,
                              gavl_audio_source_t ** as,
                              gavl_video_source_t ** vs,
                              gavl_packet_source_t ** ps);


int bg_plug_get_stream_sink(bg_plug_t * p,
                            const gavf_stream_header_t * h,
                            gavl_audio_sink_t ** as,
                            gavl_video_sink_t ** vs,
                            gavl_packet_sink_t ** ps);

void bg_plug_set_stream_action(bg_plug_t * p,
                               const gavf_stream_header_t * h,
                               bg_stream_action_t action);

gavl_sink_status_t bg_plug_put_packet(bg_plug_t * p,
                                      gavl_packet_t * pkt);


int bg_plug_got_error(bg_plug_t * p);

const gavf_stream_header_t *
bg_plug_header_from_index(bg_plug_t * p, int index);

const gavf_stream_header_t *
bg_plug_header_from_id(bg_plug_t * p, uint32_t id);

/* Set parameters */

void
bg_plug_set_compressor_config(bg_plug_t * p,
                              const bg_parameter_info_t * ac_params,
                              const bg_parameter_info_t * vc_params,
                              const bg_parameter_info_t * oc_params);

/* I/O Stuff */

#define BG_PLUG_IO_IS_LOCAL   (1<<0)
#define BG_PLUG_IO_IS_REGULAR (1<<1)

#define BG_PLUG_IO_METHOD_READ  0
#define BG_PLUG_IO_METHOD_WRITE 1

#define BG_PLUG_IO_STATUS_200 200 // OK
#define BG_PLUG_IO_STATUS_400 400 // Bad Request
#define BG_PLUG_IO_STATUS_404 404 // Not found
#define BG_PLUG_IO_STATUS_405 405 // Method Not Allowed
#define BG_PLUG_IO_STATUS_423 423 // Locked
#define BG_PLUG_IO_STATUS_505 505 // Protocol Version Not Supported
#define BG_PLUG_IO_STATUS_503 503 // Service Unavailable

/* Called by bg_plug_open_location */


gavf_io_t * bg_plug_io_open_location(const char * location,
                                     int method, int * flags);

gavf_io_t * bg_plug_io_open_socket(int fd,
                                   int method, int * flags);

/* bgplug network protocol primitives */

int bg_plug_request_read(int fd, gavl_metadata_t * req);
int bg_plug_response_write(int fd, gavl_metadata_t * res);

void
bg_plug_request_set_method(gavl_metadata_t * req, int metod);

int 
bg_plug_request_get_method(gavl_metadata_t * req, int * metod);

void
bg_plug_response_set_status(gavl_metadata_t * res, int status);


const char * bg_plug_request_get_location(gavl_metadata_t * req);

#endif // __BGPLUG_H_
