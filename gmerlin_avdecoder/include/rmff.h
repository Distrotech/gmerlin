/*****************************************************************
 
  rmff.h
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <sdp.h>

/*
 *  This is shared between the real rtsp code and
 *  the rm demultiplexer
 */

/* Chunk header */

typedef struct
  {
  uint32_t id;
  uint32_t size;
  uint16_t version;
  } bgav_rmff_chunk_t;

int bgav_rmff_chunk_header_read(bgav_rmff_chunk_t * c,
                                bgav_input_context_t * input);

/* File header */

typedef struct
  {
  /* Version 0 */
  uint32_t file_version;
  uint32_t num_headers;
  } bgav_rmff_file_header_t;


int bgav_rmff_file_header_read(bgav_rmff_chunk_t * c,
                               bgav_input_context_t * input,
                               bgav_rmff_file_header_t * ret);



/* Properties header */

typedef struct
  {
  /* Version 0 */
  uint32_t   max_bit_rate;
  uint32_t   avg_bit_rate;
  uint32_t   max_packet_size;
  uint32_t   avg_packet_size;
  uint32_t   num_packets;
  uint32_t   duration;
  uint32_t   preroll;
  uint32_t   index_offset;
  uint32_t   data_offset;
  uint16_t   num_streams;
  uint16_t   flags;
  } bgav_rmff_prop_t;

int bgav_rmff_prop_read(bgav_rmff_chunk_t * c,
                        bgav_input_context_t * input,
                        bgav_rmff_prop_t * ret);

void bgav_rmff_prop_dump(bgav_rmff_prop_t * p);



/* Media properties */

typedef struct
  {
  /* Version 0 */
  uint16_t  stream_number;
  uint32_t  max_bit_rate;
  uint32_t  avg_bit_rate;
  uint32_t  max_packet_size;
  uint32_t  avg_packet_size;
  uint32_t  start_time;
  uint32_t  preroll;
  uint32_t  duration;
  uint8_t   stream_name_size;
  uint8_t * stream_name;
  uint8_t   mime_type_size;
  uint8_t * mime_type;
  uint32_t  type_specific_len;
  uint8_t * type_specific_data;
  } bgav_rmff_mdpr_t;

void bgav_rmff_mdpr_dump(bgav_rmff_mdpr_t * m);

int bgav_rmff_mdpr_read(bgav_rmff_chunk_t * c,
                        bgav_input_context_t * input,
                        bgav_rmff_mdpr_t * ret);


/* Content description */

typedef struct
  {
  /* Version 0 */
  uint16_t  title_len;
  uint8_t * title;
  uint16_t  author_len;
  uint8_t * author;
  uint16_t  copyright_len;
  uint8_t * copyright;
  uint16_t  comment_len;
  uint8_t * comment;
  } bgav_rmff_cont_t;

void bgav_rmff_cont_dump(bgav_rmff_cont_t * m);

int bgav_rmff_cont_read(bgav_rmff_chunk_t * c,
                        bgav_input_context_t * input,
                        bgav_rmff_cont_t * ret);
void bgav_rmff_cont_free(bgav_rmff_cont_t * m);

/* Index */

typedef struct
  {
  uint16_t version;
  uint32_t timestamp;
  uint32_t offset;
  uint32_t packet_count_for_this_packet;
  } bgav_rmff_index_record_t;

int bgav_rmff_index_record_read(bgav_input_context_t * input,
                                bgav_rmff_index_record_t * ret);

typedef struct
  {
  uint32_t                   num_indices;
  uint16_t                   stream_number;
  uint32_t                   next_index_header;
  bgav_rmff_index_record_t * records;
  } bgav_rmff_indx_t;

int bgav_rmff_indx_read(bgav_input_context_t * input,
                        bgav_rmff_indx_t * ret);
void bgav_rmff_indx_free(bgav_rmff_indx_t * indx);


/* Now, the overall file header */

typedef struct
  {
  bgav_rmff_mdpr_t mdpr;
  bgav_rmff_indx_t indx;
  int has_indx;
  } bgav_rmff_stream_t;

typedef struct
  {
  /* Version 0 */
  uint32_t num_packets;
  uint32_t next_data_header;
  } bgav_rmff_data_header_t;

int bgav_rmff_data_header_read(bgav_input_context_t * ctx,
                               bgav_rmff_data_header_t * ret);
  
                               
typedef struct
  {
  bgav_rmff_file_header_t file_header;
  bgav_rmff_prop_t prop;
  bgav_rmff_cont_t cont;
  bgav_rmff_data_header_t data_header;
  
  int num_streams;

  bgav_rmff_stream_t * streams;
  
  /* Filled in during parsing */

  int64_t data_start;
  int64_t data_size;
    
  } bgav_rmff_header_t;

/* Used by the demuxer for reading files */

bgav_rmff_header_t * bgav_rmff_header_read(bgav_input_context_t * ctx);

/* Used by the rtsp plugin */

bgav_rmff_header_t * bgav_rmff_header_create(int num_streams);

/* Used by both */

void bgav_rmff_header_destroy(bgav_rmff_header_t * h);

/* Open a rm demuxer with header as argument */

int bgav_demux_rm_open_with_header(bgav_demuxer_context_t * ctx,
                                   bgav_rmff_header_t * h);
  
/* Create an rmdff header from an sdp structure */

bgav_rmff_header_t * bgav_rmff_header_create_from_sdp(bgav_sdp_t * sdp,
                                                      int network_bandwidth,
                                                      char ** stream_rules);

void bgav_rmff_header_dump(bgav_rmff_header_t * header);
