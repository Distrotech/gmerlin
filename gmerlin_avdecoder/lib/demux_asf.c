/*****************************************************************
 
  demux_asf.c
 
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>
#include <nanosoft.h>

#define AUDIO_ERROR_CONCEAL_NONE       0
#define AUDIO_ERROR_CONCEAL_INTERLEAVE 1

#define ASF_TIME_SCALE 1000

/* Copied from avifile CVS */

/* base ASF objects */
static const bgav_GUID_t guid_header = 
  { 0x75b22630, 0x668e, 0x11cf,
    { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c } };

static const bgav_GUID_t guid_data = 
  { 0x75b22636, 0x668e, 0x11cf,
    { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c } };

static const bgav_GUID_t guid_simple_index = 
  { 0x33000890, 0xe5b1, 0x11cf,
    { 0x89, 0xf4, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xcb } };


/* header ASF objects */
static const bgav_GUID_t guid_file_properties = 
  { 0x8cabdca1, 0xa947, 0x11cf,
    { 0x8e, 0xe4, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 } };

static const bgav_GUID_t guid_stream_header = 
  { 0xb7dc0791, 0xa9b7, 0x11cf,
    { 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 } };

static const bgav_GUID_t guid_stream_bitrate_properties =  /* (http://get.to/sdp) */
  { 0x7bf875ce, 0x468d, 0x11d1,
      { 0x8d, 0x82, 0x00, 0x60, 0x97, 0xc9, 0xa2, 0xb2 } };

static const bgav_GUID_t guid_content_description = 
  { 0x75b22633, 0x668e, 0x11cf,
    { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c } };

static const bgav_GUID_t guid_extended_content_encryption = 
  { 0x298ae614, 0x2622, 0x4c17,
    { 0xb9, 0x35, 0xda, 0xe0, 0x7e, 0xe9, 0x28, 0x9c } };

static const bgav_GUID_t guid_script_command = 
  { 0x1efb1a30, 0x0b62, 0x11d0,
    { 0xa3, 0x9b, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6 } };

static const bgav_GUID_t guid_marker = 
  { 0xf487cd01, 0xa951, 0x11cf,
    { 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 } };

static const bgav_GUID_t guid_header_extension = 
  { 0x5fbf03b5, 0xa92e, 0x11cf,
    { 0x8e, 0xe3, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 } };

static const bgav_GUID_t guid_bitrate_mutual_exclusion = 
  { 0xd6e229dc, 0x35da, 0x11d1,
    { 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe } };

static const bgav_GUID_t guid_codec_list = 
  { 0x86d15240, 0x311d, 0x11d0,
    { 0xa3, 0xa4, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6 } };

static const bgav_GUID_t guid_extended_content_description = 
  { 0xd2d0a440, 0xe307, 0x11d2,
    { 0x97, 0xf0, 0x00, 0xa0, 0xc9, 0x5e, 0xa8, 0x50 } };

static const bgav_GUID_t guid_error_correction = 
  { 0x75b22635, 0x668e, 0x11cf,
    { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c } };

static const bgav_GUID_t guid_padding = 
  { 0x1806d474, 0xcadf, 0x4509,
    { 0xa4, 0xba, 0x9a, 0xab, 0xcb, 0x96, 0xaa, 0xe8 } };

static const bgav_GUID_t guid_comment_header =
  { 0x75b22633, 0x668e, 0x11cf,
    { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c } };

/* stream properties object stream type */
static const bgav_GUID_t guid_audio_media = 
  { 0xf8699e40, 0x5b4d, 0x11cf,
    { 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b } };

static const bgav_GUID_t guid_video_media = 
  { 0xbc19efc0, 0x5b4d, 0x11cf,
    { 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b } };

static const bgav_GUID_t guid_command_media = 
  { 0x59dacfc0, 0x59e6, 0x11d0,
    { 0xa3, 0xac, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6 } };

/* GUID indicating that audio error concealment is absent */
static const bgav_GUID_t guid_audio_conceal_none=
  { 0x49f1a440, 0x4ece, 0x11d0,
    { 0xa3, 0xac, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6 } };

/* GUID indicating that interleaved audio error concealment is present */
static const bgav_GUID_t guid_audio_conceal_interleave =
  { 0xbfc3cd50, 0x618f, 0x11cf,
    { 0x8b, 0xb2, 0x00, 0xaa, 0x00, 0xb4, 0xe2, 0x20 } };

/* stream properties object error correction */
static const bgav_GUID_t guid_no_error_correction = 
  { 0x20fb5700, 0x5b55, 0x11cf,
    { 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b } };

static const bgav_GUID_t guid_audio_spread = 
  { 0xbfc3cd50, 0x618f, 0x11cf,
    { 0x8b, 0xb2, 0x00, 0xaa, 0x00, 0xb4, 0xe2, 0x20 } };


/* mutual exclusion object exlusion type */
static const bgav_GUID_t guid_mutex_bitrate = 
    { 0xd6e22a01, 0x35da, 0x11d1,
      { 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe } };

static const bgav_GUID_t guid_mutex_unknown =  
  { 0xd6e22a02, 0x35da, 0x11d1,
    { 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe } };


/* header extension */
static const bgav_GUID_t guid_reserved_1 = 
  { 0xabd3d211, 0xa9ba, 0x11cf,
    { 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 } };

/* script command */
static const bgav_GUID_t guid_reserved_script_command = 
  { 0x4B1ACBE3, 0x100B, 0x11D0,
    { 0xA3, 0x9B, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6 } };

/* marker object */
static const bgav_GUID_t guid_reserved_marker = 
  { 0x4CFEDB20, 0x75F6, 0x11CF,
    { 0x9C, 0x0F, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB } };

/* various */
static const bgav_GUID_t guid_head2 = 
  { 0xabd3d211, 0xa9ba, 0x11cf,
    { 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 } };

static const bgav_GUID_t guid_codec_comment1_header = 
  { 0x86d15241, 0x311d, 0x11d0,
    { 0xa3, 0xa4, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6 } };

static const bgav_GUID_t guid_asf_2_0_header = 
  { 0xd6e229d1, 0x35da, 0x11d1,
    { 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe } };

/* ASF specific stream flags */

typedef struct
  {
  int error_concealment;
  uint8_t  descramble_h;
  uint16_t descramble_w;
  uint16_t descramble_b;
  uint16_t block_align_2;

  uint8_t * scramble_buffer;
  int scramble_buffer_size;
  
  } asf_audio_stream_t;
#if 0
static void dump_audio_stream(asf_audio_stream_t * as)
  {
  fprintf(stderr, "descramble_h: %d\n", as->descramble_h);
  fprintf(stderr, "descramble_w: %d\n", as->descramble_w);
  fprintf(stderr, "descramble_b: %d\n", as->descramble_b);
  }
#endif
static void asf_descramble(asf_audio_stream_t * as,
                    uint8_t * data,
                    int data_size)
  {
  uint8_t * dst;
  unsigned char *s2=data;
  int i=0,x,y;
  if(as->scramble_buffer_size < data_size)
    {
    as->scramble_buffer_size = data_size;
    as->scramble_buffer = realloc(as->scramble_buffer,
                                  as->scramble_buffer_size);
    }
  dst = as->scramble_buffer;
  
  
  while(data_size-i>=as->descramble_h*as->descramble_w*as->descramble_b)
    {
    for(x=0;x<as->descramble_w;x++)
      for(y=0;y<as->descramble_h;y++){
      memcpy(dst+i,s2+(y*as->descramble_w+x)*
             as->descramble_b,as->descramble_b);
      i+=as->descramble_b;
      }
    s2+=as->descramble_h*as->descramble_w*as->descramble_b;
    }
  //if(i<len) memcpy(dst+i,src+i,len-i);
  memcpy(data,dst,i);
  }

#if 0
typedef struct
  {
  int dummy;
  } asf_video_stream_t;
#endif
/* Private data */

typedef struct
  {
  bgav_GUID_t guid;		/* generated by client computer */
  uint64_t file_size;	/* in bytes */
                        /* invalid if broadcasting */
  uint64_t create_time; /* time of creation, in 100-nanosecond units */
                        /* since 1.1.1601 invalid if broadcasting */
  
  uint64_t packets_count; /* how many packets are there in the file */
                          /* invalid if broadcasting                */

  uint64_t play_time; /* play time, in 100-nanosecond units */
                      /* invalid if broadcasting            */

  uint64_t send_time; /* time to send file, in 100-nanosecond units */
                      /* invalid if broadcasting (could be ignored) */

  uint32_t preroll;  /* timestamp of the first packet, in milliseconds */
                     /* if nonzero - substract from time */

  uint32_t ignore;   /* preroll is 64bit - but let's just ignore it */

  uint32_t flags;    /* 0x01 - broadcast */
                     /* 0x02 - seekable  */
                     /* rest is reserved should be 0 */

  uint32_t min_pktsize;	/* size of a data packet   */
                        /* invalid if broadcasting */
  
  uint32_t max_pktsize;	/* shall be the same as for min_pktsize */
                        /* invalid if broadcasting              */

  uint32_t max_bitrate;	/* bandwith of stream in bps            */
                        /* should be the sum of bitrates of the */
                        /* individual media streams             */
  } asf_main_header_t;

typedef struct
  {
  uint64_t packets_read;
  asf_main_header_t hdr;
  int packet_size;
  uint8_t * packet_buffer;
  int64_t data_start;
  int64_t data_size;
  int do_sync;

  uint32_t first_timestamp;
  int need_first_timestamp;
  
  struct
    {
    int stream_id;
    uint32_t bitrate;
    } * stream_bitrates;
  int num_stream_bitrates;
  } asf_t;

static int probe_asf(bgav_input_context_t * input)
  {
  bgav_GUID_t guid;
  if(!bgav_GUID_get(&guid, input))
    return 0;

  if(bgav_GUID_equal(&guid, &guid_header))
    {
    //    fprintf(stderr, "Detected ASF format\n");
    return 1;
    }
  return 0;
  }

static int read_bitrate_properties(bgav_demuxer_context_t * ctx)
  {
  uint16_t i_tmp;
  int i;
  asf_t * asf;
  asf = (asf_t*)(ctx->priv);

  if(!bgav_input_read_16_le(ctx->input, &i_tmp))
    return 0;

  asf->num_stream_bitrates = i_tmp;

  if(!asf->num_stream_bitrates)
    {
    //    fprintf(stderr, "No bitrates specified\n");
    return 1;
    }
  asf->stream_bitrates     = calloc(asf->num_stream_bitrates,
                                    sizeof(*(asf->stream_bitrates)));

  for(i = 0; i < asf->num_stream_bitrates; i++)
    {
    if(!bgav_input_read_16_le(ctx->input, &i_tmp) ||
       !bgav_input_read_32_le(ctx->input, &(asf->stream_bitrates[i].bitrate)))
      return 0;
    asf->stream_bitrates[i].stream_id = i_tmp;
    //    fprintf(stderr, "Stream: %d, Bitrate %d\n", asf->stream_bitrates[i].stream_id,
    //        asf->stream_bitrates[i].bitrate);
    
    }
  return 1;
  }

static void update_stream_bitrates(bgav_demuxer_context_t * ctx)
  {
  int i;
  bgav_stream_t * stream;
    
  asf_t * asf;
  asf = (asf_t*)(ctx->priv);

  //  fprintf(stderr, "**** update_stream_bitrates\n"); 
  for(i = 0; i < asf->num_stream_bitrates; i++)
    {
    stream = bgav_track_find_stream(ctx->tt->current_track, asf->stream_bitrates[i].stream_id);
    if(stream)
      stream->container_bitrate = asf->stream_bitrates[i].bitrate;
    else
      {
      //      fprintf(stderr, "NO STREAM FOUND %d\n",
      //              asf->stream_bitrates[i].stream_id);
      }
    }
  }


static int read_metadata(bgav_demuxer_context_t * ctx)
  {
  char * str = (char*)0;
  uint16_t len1, len2, len3, len4, len5;
  int str_len;
  bgav_charset_converter_t * cnv =
    (bgav_charset_converter_t*)0;
  
  if(!bgav_input_read_16_le(ctx->input, &len1) ||
     !bgav_input_read_16_le(ctx->input, &len2) ||
     !bgav_input_read_16_le(ctx->input, &len3) ||
     !bgav_input_read_16_le(ctx->input, &len4) ||
     !bgav_input_read_16_le(ctx->input, &len5))
    goto fail;

  cnv = bgav_charset_converter_create("UTF-16LE", "UTF-8");
  
  str_len = len1;
  if(str_len < len2)
    str_len = len2;
  if(str_len < len3)
    str_len = len3;
  if(str_len < len4)
    str_len = len4;
  if(str_len < len5)
    str_len = len5;

  str = malloc(str_len);
    
  /* Title */
  
  if(len1)
    {
    if(bgav_input_read_data(ctx->input, (uint8_t*)str, len1) < len1)
      goto fail;
    ctx->tt->current_track->metadata.title = bgav_convert_string(cnv, str, len1,
                                              NULL);
    //    fprintf(stderr, "Title: %s\n", ctx->current_track->metadata.title);
    }

  /* Author */
  
  if(len2)
    {
    if(bgav_input_read_data(ctx->input, (uint8_t*)str, len2) < len2)
      goto fail;
    ctx->tt->current_track->metadata.author =
      bgav_convert_string(cnv, str, len2, NULL);
    //    fprintf(stderr, "Author: %s\n", ctx->current_track->metadata.author);
    }

  /* Copyright */
  
  if(len3)
    {
    if(bgav_input_read_data(ctx->input, (uint8_t*)str, len3) < len3)
      goto fail;
    ctx->tt->current_track->metadata.copyright =
      bgav_convert_string(cnv, str, len3, NULL);
    //    fprintf(stderr, "Copyright: %s\n", ctx->current_track->metadata.copyright);
    }

  /* Comment */
  
  if(len4)
    {
    if(bgav_input_read_data(ctx->input, (uint8_t*)str, len4) < len4)
      goto fail;
    ctx->tt->current_track->metadata.comment =
      bgav_convert_string(cnv, str, len4, NULL);
    //    fprintf(stderr, "Comment: %s\n", ctx->current_track->metadata.comment);
    }

  /* Unknown stuff */

  if(len5)
    bgav_input_skip(ctx->input, len5);
  
  free(str);
  bgav_charset_converter_destroy(cnv);
  return 1;

  fail:
  if(cnv)
    bgav_charset_converter_destroy(cnv);
  if(str)
    free(str);
  return 0;
  }


static int open_asf(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  int64_t chunk_start_pos;
  
  bgav_BITMAPINFOHEADER_t bh;
  bgav_WAVEFORMAT_t     wf;
  uint8_t * buf = (uint8_t*)0;
  uint8_t * pos;
  uint16_t stream_number;
  int buf_size = 0;
  bgav_GUID_t guid, guid1;
  uint64_t size;
  uint32_t type_specific_size;
  uint32_t stream_specific_size;
  int error_concealment;
  asf_t * asf;
  asf_audio_stream_t  * asf_as;
  //  asf_video_stream_t  * asf_vs;
  bgav_stream_t * bgav_as;
  bgav_stream_t * bgav_vs;
  //  fprintf(stderr, "Open asf\n");

  /* Create track */
  ctx->tt = bgav_track_table_create(1);
  
  if(!bgav_GUID_read(&guid, ctx->input))
    return 0;
  
  /* Read header */
  
  if(!bgav_GUID_equal(&guid, &guid_header))
    return 0;

  bgav_input_skip(ctx->input, 8); /*  Chunk length UINT64          */
  bgav_input_skip(ctx->input, 4); /*  Number of subchunks UINT32   */
  bgav_input_skip(ctx->input, 2); /*  Unknown                      */

  /* Now the file header should come */
  asf = calloc(1, sizeof(*asf));

  asf->need_first_timestamp = 1;
  
  ctx->priv = asf;
  while(1) /* Read all GUIDs until the data comes */
    {
    chunk_start_pos = ctx->input->position;
    if(!bgav_GUID_read(&guid, ctx->input) ||
       !bgav_input_read_64_le(ctx->input, &size))
      goto fail;
    //    fprintf(stderr, "GUID: ");
    //    dump_guid(&guid);
    //    fprintf(stderr, "Size: %lld\n", size);
    
    /* Overall file properties */
    
    if(bgav_GUID_equal(&guid, &guid_file_properties))
      {
      if(!bgav_GUID_read(&asf->hdr.guid, ctx->input) ||
         !bgav_input_read_64_le(ctx->input, &(asf->hdr.file_size)) ||
         !bgav_input_read_64_le(ctx->input, &(asf->hdr.create_time)) ||
         !bgav_input_read_64_le(ctx->input, &(asf->hdr.packets_count)) ||
         !bgav_input_read_64_le(ctx->input, &(asf->hdr.play_time)) ||
         !bgav_input_read_64_le(ctx->input, &(asf->hdr.send_time)) ||
         !bgav_input_read_32_le(ctx->input, &(asf->hdr.preroll)) ||
         !bgav_input_read_32_le(ctx->input, &(asf->hdr.ignore)) ||
         !bgav_input_read_32_le(ctx->input, &(asf->hdr.flags)) ||
         !bgav_input_read_32_le(ctx->input, &(asf->hdr.min_pktsize)) ||
         !bgav_input_read_32_le(ctx->input, &(asf->hdr.max_pktsize)) ||
         !bgav_input_read_32_le(ctx->input, &(asf->hdr.max_bitrate)))
        goto fail;
      //      fprintf(stderr, "*** Preroll: %lld\n", asf->hdr.preroll);
      //      asf->packet_size = asf->hdr.max_pktsize;
      //      asf->nb_packets = asf->hdr.packets_count;

      // fprintf(stderr, "Send time: 0x%llx\n", asf->hdr.send_time);
      if(asf->hdr.send_time)
        ctx->tt->current_track->duration = (asf->hdr.send_time - asf->hdr.preroll) 
          / (10000000/GAVL_TIME_SCALE);
      //      fprintf(stderr, "**** Duration: %lld\n", ctx->duration);
      }
    /* Stream properties */
    else if(bgav_GUID_equal(&guid, &guid_stream_header))
      {
      /* Stream type */
      if(!bgav_GUID_read(&guid, ctx->input))
        goto fail;

      /* Error concealment type */
      if(!bgav_GUID_read(&guid1, ctx->input))
        goto fail;

      error_concealment = AUDIO_ERROR_CONCEAL_NONE;
      
      //      if(bgav_GUID_equal(&guid1, &guid_audio_conceal_none))
      //        error_concealment = AUDIO_ERROR_CONCEAL_NONE;
      if(bgav_GUID_equal(&guid1, &guid_audio_conceal_interleave))
        error_concealment = AUDIO_ERROR_CONCEAL_INTERLEAVE;
      
      bgav_input_skip(ctx->input, 8);  /* Unknown */
        
      if(!bgav_input_read_32_le(ctx->input, &type_specific_size) ||
         !bgav_input_read_32_le(ctx->input, &stream_specific_size) ||
         !bgav_input_read_16_le(ctx->input, &stream_number))
        goto fail;

      bgav_input_skip(ctx->input, 4);  /* Unknown */
      
      /* Found audio stream */
      if(bgav_GUID_equal(&guid, &guid_audio_media))
        {
        //        fprintf(stderr, "Found audio stream\n");
        
        bgav_as = bgav_track_add_audio_stream(ctx->tt->current_track);
        bgav_as->stream_id = stream_number;

        asf_as  = calloc(1, sizeof(*asf_as));
       
        bgav_as->priv = asf_as;
        asf_as->error_concealment = error_concealment;

        /* Type specific data */
        
        if(buf_size < type_specific_size)
          {
          buf_size = type_specific_size;
          buf = realloc(buf, buf_size);
          }
        if(bgav_input_read_data(ctx->input, buf, type_specific_size) <
           type_specific_size)
          goto fail;
        bgav_WAVEFORMAT_read(&wf, buf, type_specific_size);
        //        bgav_WAVEFORMAT_dump(&wf);
        bgav_WAVEFORMAT_get_format(&wf, bgav_as);
        bgav_WAVEFORMAT_free(&wf);
        
        /* Stream specific data */

        if(buf_size < stream_specific_size)
          {
          buf_size = stream_specific_size;
          buf = realloc(buf, buf_size);
          }
        if(bgav_input_read_data(ctx->input, buf, stream_specific_size) <
           stream_specific_size)
          goto fail;
        if(asf_as->error_concealment == AUDIO_ERROR_CONCEAL_INTERLEAVE)
          {
          pos = buf;
          
          asf_as->descramble_h = *pos;
          pos++;
          asf_as->descramble_w = BGAV_PTR_2_16LE(pos);
          pos+=2;
          asf_as->descramble_b = BGAV_PTR_2_16LE(pos);
          pos+=2;
          asf_as->descramble_w /= asf_as->descramble_b;
          
          asf_as->block_align_2 = BGAV_PTR_2_16LE(pos);
          pos+=2;
          }
        else
          {
          asf_as->descramble_b = 1;
          asf_as->descramble_h = 1;
          asf_as->descramble_w = 1;
          }
        bgav_as->timescale = 1000;
        //        dump_audio_stream(asf_as);
        }
      
      /* Found video stream */
      else if(bgav_GUID_equal(&guid, &guid_video_media))
        {
        //        fprintf(stderr, "Found video stream\n");
        bgav_vs = bgav_track_add_video_stream(ctx->tt->current_track);
        bgav_vs->stream_id = stream_number;
        //        asf_vs  = calloc(1, sizeof(*asf_vs));
        
        //        bgav_vs->priv = asf_vs;

        if(buf_size < type_specific_size)
          {
          buf_size = type_specific_size;
          buf = realloc(buf, buf_size);
          }
        if(bgav_input_read_data(ctx->input, buf, type_specific_size) <
           type_specific_size)
          goto fail;
        pos = buf;

        pos += 4;  /* Picture width UINT32 4 */
        pos += 4;  /* Picture height UINT32 4 */
        pos ++;    /* Unknown	UINT8	1 */
        pos += 2; /* Size of Bitmapinfoheader */
        
        //        bgav_input_read_32_le(ctx->input, &size1);

        bgav_BITMAPINFOHEADER_read(&bh, &pos);
        //        bgav_BITMAPINFOHEADER_dump(&bh);
        bgav_BITMAPINFOHEADER_get_format(&bh, bgav_vs);
        /* Fill in the remeaining values */
        
        bgav_vs->data.video.format.timescale = 1000;
        bgav_vs->data.video.format.frame_duration = 40; /* 25.0 fps, but we don't care about this */
        bgav_vs->data.video.format.free_framerate = 1;
        
        //        gavl_video_format_dump(&(bgav_vs->format));
        if(pos - buf < type_specific_size)
          {
          bgav_vs->ext_size = type_specific_size - (pos - buf);
          bgav_vs->ext_data = malloc(bgav_vs->ext_size);
          memcpy(bgav_vs->ext_data, pos, bgav_vs->ext_size);
          }

        /* Stream specific (not known for now) */

        if(stream_specific_size)
          {
          bgav_input_skip(ctx->input, stream_specific_size);
          fprintf(stderr,
                  "Warning: Video stream contains stream specific data\n");
          }
        
        }
      }
    /* Metadata */
    else if(bgav_GUID_equal(&guid_comment_header, &guid))
      {
      //      fprintf(stderr, "Comment header\n");
      if(!read_metadata(ctx))
        return 0;
      }
    else if(bgav_GUID_equal(&guid_stream_bitrate_properties, &guid))
      {
      if(!read_bitrate_properties(ctx))
        return 0;
      }
    else if(bgav_GUID_equal(&guid_data, &guid))
      {
      asf->data_size = size;
      break;
      }

    /* Skip unused junk */
    if(ctx->input->position - chunk_start_pos < size)
      {
      //      fprintf(stderr, "Skipping %lld unused/unknown bytes...",
      //              size - (ctx->input->position - chunk_start_pos));
      bgav_input_skip(ctx->input,
                      size - (ctx->input->position - chunk_start_pos));
      //      fprintf(stderr, "Pos is now: %lld\n",
      //              ctx->input->position);
      
      }
    //    if(size & 1)
    //      {
    //      bgav_input_skip(ctx->input, 1);
    //      fprintf(stderr, "Padding Size: %lld\n", size);
    //      }
    //    else
    //      fprintf(stderr, "Not Padding Size: %lld\n", size);
    
    }

  /* Skip 26 bytes */
  
  bgav_input_skip(ctx->input, 16); /* Unknown GUID */
  bgav_input_skip(ctx->input, 8);  /* Number of packets	UINT64 */
  bgav_input_skip(ctx->input, 1);  /* Unknown UINT8  */
  bgav_input_skip(ctx->input, 1);  /* Unknown UINT8 */

  /* Set the packet size */
  
  asf->packet_size = asf->hdr.max_pktsize;
  asf->packet_buffer = malloc(asf->packet_size);
  
  asf->data_start = ctx->input->position;
  
  //  fprintf(stderr, "Reached Data section, %lld %lld\n",
  //          asf->data_start,
  //          asf->data_size);

  /* Update stream bitrates */

  if(asf->num_stream_bitrates)
    {
    update_stream_bitrates(ctx);
    }
      
  if(buf)
    free(buf);
  
  if(ctx->input->input->seek_byte && asf->hdr.packets_count)
    {
    //    fprintf(stderr, "Can SEEK %p\n", ctx->input->input->seek_byte);
    ctx->can_seek = 1;
    }
  ctx->stream_description = bgav_sprintf("Windows media format (ASF)");
  
  return 1;
  fail:
  if(buf)
    free(buf);
  return 0;
  }

/* Packet info */

typedef struct
  {
  uint8_t flags;
  uint8_t segtype;
  int plen;      // packet size
  int sequence;  // sequence
  int padding;   // padding size (padding)
  uint32_t time;
  int16_t duration;
  
  int segsizetype;
  int segs;
  } asf_packet_header_t;
#if 0
static void dump_packet_header(asf_packet_header_t * h)
  {
  fprintf(stderr, "asf packet header\n");
  fprintf(stderr, "  packet_flags:    0x%02x\n", h->flags);
  fprintf(stderr, "  packet_property: 0x%02x\n", h->segtype);
  fprintf(stderr, "  packet_length:   %d\n",     h->plen);
  fprintf(stderr, "  sequence:        %d\n",     h->sequence);
  fprintf(stderr, "  padsize:         %d\n",     h->padding);
  fprintf(stderr, "  timestamp:       %d\n",     h->time);
  fprintf(stderr, "  duration:        %d\n",     h->duration);
  fprintf(stderr, "  segsizetype:     0x%02x\n", h->segsizetype);
  fprintf(stderr, "  segments:        %d\n",     h->segs);
  }
#endif
/* Returns the number of bytes used or -1 */

static int read_packet_header(asf_t * asf,
                              asf_packet_header_t * ret,
                              uint8_t * data)
  {
  uint8_t * data_ptr;

  ret->time        = 0;
  ret->duration    = 0;
  ret->segs        = 1;
  ret->segsizetype = 0x80;
  
  data_ptr = data + (1 + (*data & 15));
  
  ret->flags = *(data_ptr++);
  ret->segtype      = *(data_ptr++);

  // Read packet size (plen):
  switch((ret->flags>>5)&3)
    {
    case 3: ret->plen=BGAV_PTR_2_32LE(data_ptr);data_ptr+=4;break; // dword
    case 2: ret->plen=BGAV_PTR_2_16LE(data_ptr);data_ptr+=2;break; // word
    case 1: ret->plen=data_ptr[0];data_ptr++;break;                // byte
    default: ret->plen=0;
      //plen==0 is handled later
      //mp_msg(MSGT_DEMUX,MSGL_V,"Invalid plen type! assuming plen=0\n");
    }
  
  // Read sequence:
  switch((ret->flags>>1)&3)
    {
    case 3: ret->sequence=BGAV_PTR_2_32LE(data_ptr);data_ptr+=4;break;// dword
    case 2: ret->sequence=BGAV_PTR_2_16LE(data_ptr);data_ptr+=2;break;// word
    case 1: ret->sequence=data_ptr[0];data_ptr++;break;            // byte
    default: ret->sequence=0;
    }
  
  // Read padding size (padding):
  switch((ret->flags>>3)&3)
    {
    case 3: ret->padding=BGAV_PTR_2_32LE(data_ptr);data_ptr+=4;break;// dword
    case 2: ret->padding=BGAV_PTR_2_16LE(data_ptr);data_ptr+=2;break;// word
    case 1: ret->padding=data_ptr[0];data_ptr++;break;             // byte
    default: ret->padding=0;
    }
  
  if(((ret->flags>>5)&3)!=0)
    {
    // Explicit (absoulte) packet size
    //    fprintf(stderr,"Explicit packet size specified: %d  \n",ret->plen);
    if(ret->plen>asf->packet_size)
      fprintf(stderr,
             "Warning! plen>packetsize! (%d>%d)  \n",
              ret->plen,asf->packet_size);
    }
  else
    {
    // Padding (relative) size
    ret->plen=asf->packet_size-ret->padding;
    }
  
  // Read time & duration:
  ret->time = BGAV_PTR_2_32LE(data_ptr);     data_ptr+=4;
  ret->duration = BGAV_PTR_2_16LE(data_ptr); data_ptr+=2;
  
  // Read payload flags:
  if(ret->flags&1)
    {
    // multiple sub-packets
    ret->segsizetype=data_ptr[0]>>6;
    ret->segs=data_ptr[0] & 0x3F;
    ++data_ptr;
    }
  //  dump_packet_header(ret);
  return data_ptr - data;
  }
  
typedef struct
  {
  unsigned char streamno;
  unsigned int seq;
  unsigned int x;   // offset or timestamp
  unsigned int rlen;
  //
  int len;
  unsigned int time2;
  int keyframe;
  } asf_segment_header_t;
#if 0
static void dump_segment_header(asf_segment_header_t*h)
  {
  fprintf(stderr, "Segment header:\n");
  fprintf(stderr, "  Stream number: %d\n", h->streamno);
  fprintf(stderr, "  Seq:           %d\n", h->seq);
  fprintf(stderr, "  x:             %d\n", h->x);   // offset or timestamp
  fprintf(stderr, "  rlen:          %d\n", h->rlen);
  fprintf(stderr, "  len:           %d\n", h->len);
  fprintf(stderr, "  time2:         %d\n", h->time2);
  fprintf(stderr, "  keyframe:      %d\n", h->keyframe);
  }
#endif
static int read_segment_header(asf_t * asf,
                               asf_packet_header_t * pkt_hdr,
                               asf_segment_header_t * ret,
                               uint8_t * data)
  {
  uint8_t * data_ptr;

  ret->time2=0;
  ret->keyframe=0;

  data_ptr = data;
  
  ret->streamno = (*data_ptr) & 0x7f;
  ret->keyframe = !!((*data_ptr) & 0x80);

  data_ptr++;

  // Read media object number (seq):
  switch((pkt_hdr->segtype>>4)&3)
    {
    case 3: ret->seq=BGAV_PTR_2_32LE(data_ptr);data_ptr+=4;break; // dword
    case 2: ret->seq=BGAV_PTR_2_16LE(data_ptr);data_ptr+=2;break; // word
    case 1: ret->seq=data_ptr[0];data_ptr++;break;         // byte
    default: ret->seq=0;
    }
  
  // Read offset or timestamp:
  switch((pkt_hdr->segtype>>2)&3)
    {
    case 3: ret->x=BGAV_PTR_2_32LE(data_ptr);data_ptr+=4;break; // dword
    case 2: ret->x=BGAV_PTR_2_16LE(data_ptr);data_ptr+=2;break; // word
    case 1: ret->x=data_ptr[0];data_ptr++;break;         // byte
    default: ret->x=0;
    }
  
  // Read replic.data len:
  switch((pkt_hdr->segtype)&3)
    {
    case 3: ret->rlen=BGAV_PTR_2_32LE(data_ptr);data_ptr+=4;break; // dword
    case 2: ret->rlen=BGAV_PTR_2_16LE(data_ptr);data_ptr+=2;break; // word
    case 1: ret->rlen=data_ptr[0];data_ptr++;break;              // byte
    default: ret->rlen=0;
    }

  switch(ret->rlen)
    {
    case 0x01: // 1 = special, means grouping
      //printf("grouping: %02X  \n",p[0]);
      ++data_ptr; // skip PTS delta
      break;
    default:
      if(ret->rlen>=8)
        {
        data_ptr+=4;       // skip object size
        ret->time2=BGAV_PTR_2_32LE(data_ptr); // read PTS
        data_ptr+=ret->rlen-4;
        }
      else
        {
        fprintf(stderr,
               "unknown segment type (rlen): 0x%02X  \n",ret->rlen);
        ret->time2=0; // unknown
        data_ptr+=ret->rlen;
        return -1;
        }
    }
  if(pkt_hdr->flags&1)
    {
    // multiple segments
    switch(pkt_hdr->segsizetype)
      {
      case 3: ret->len=BGAV_PTR_2_32LE(data_ptr);data_ptr+=4;break;  // dword
      case 2: ret->len=BGAV_PTR_2_16LE(data_ptr);data_ptr+=2;break;  // word
      case 1: ret->len=data_ptr[0];data_ptr++;break;           // byte
      default: ret->len=pkt_hdr->plen-(data_ptr-asf->packet_buffer); // ???
      }
    }
  else
    {
    // single segment
    ret->len=pkt_hdr->plen-(data_ptr-asf->packet_buffer);
    }
#if 0
  if(ret->len<0 || (data_ptr+ret->len)>p_end)
    {
    fprintf(stderr,
           "ASF_parser: warning! segment len=%d\n",len);
    }
#endif
  //  fprintf(stderr,
  //          "  seg #%d: streamno=%d  seq=%d  type=%02X  len=%d\n",
  //          seg,streamno,seq,rlen,len);
  //  dump_segment_header(ret);
  return (data_ptr - data);
  }

static void add_packet(bgav_demuxer_context_t * ctx,
                       uint8_t * data,
                       int len,
                       int id,
                       int seq,
                       unsigned long time,
                       unsigned short dur,
                       int offs,
                       int keyframe)
  {
  bgav_stream_t * stream;
  asf_audio_stream_t * as;
  asf_t * asf = (asf_t *)(ctx->priv);

  //  if(asf->do_sync)
  //    fprintf(stderr, "Add packet: %d %d %d\n", id, keyframe, offs);
  stream = bgav_track_find_stream(ctx->tt->current_track, id);
  
  if(!stream)
    return;
  
  if(asf->do_sync)
    {
    if((stream->type == BGAV_STREAM_VIDEO) &&
       (stream->time_scaled < 0) && (!keyframe || (offs > 0)))
      return;
    else if((stream->type == BGAV_STREAM_AUDIO) &&
            (stream->time_scaled < 0) && (offs > 0))
      return;
    }
  
  if(stream->packet)
    {
    if(stream->packet_seq != seq)
      {
      if(stream->type == BGAV_STREAM_AUDIO)
        {
        as = (asf_audio_stream_t*)(stream->priv);
        if((as->descramble_w > 1) && (as->descramble_h > 1))
          asf_descramble(as,
                         stream->packet->data,
                         stream->packet->data_size);
        }
      // fprintf(stderr, "* * * Sending packet: %d\n", stream->packet->data_size);
      bgav_packet_done_write(stream->packet);
      stream->packet = (bgav_packet_t*)0;
      }
    else
      {
      // append data to it!
      if((stream->packet->data_size != offs) &&
         (offs != -1))
        fprintf(stderr, "Warning: data_size %d, Offset: %d\n",
                stream->packet->data_size, offs);
      bgav_packet_alloc(stream->packet,
                        stream->packet->data_size + len);
      memcpy(&(stream->packet->data[stream->packet->data_size]), data, len);
      stream->packet->data_size += len;
      // fprintf(stderr, "* * * Appended to packet: %d\n", stream->packet->data_size);
      return;
      }
    }
  
  stream->packet = bgav_packet_buffer_get_packet_write(stream->packet_buffer, stream);
  bgav_packet_alloc(stream->packet, len);
  
  if(asf->need_first_timestamp)
    {
    asf->first_timestamp = time;
    asf->need_first_timestamp = 0;
    //    fprintf(stderr, "First timestamp: %d\n", asf->first_timestamp);
    }
  if(time > asf->first_timestamp)
    time -= asf->first_timestamp;
  else
    time = 0;
  
  stream->packet->timestamp_scaled = time;

  //  fprintf(stderr, "timestamp: %lld, timestamp_scaled: %llx\n", 
  //          stream->packet->timestamp, stream->packet->timestamp_scaled);
  
  // stream->packet->timestamp -= ((gavl_time_t)(asf->hdr.preroll) * GAVL_TIME_SCALE) / 1000;
  
  if(asf->do_sync && (stream->time_scaled < 0))
    {
    //    fprintf(stderr, "Sync %d %f\n", id, gavl_time_to_seconds(stream->packet->timestamp));
    stream->time_scaled = time;
    }
  //  if(id == 2)
  //    fprintf(stderr, "Timestamps: %d %lld\n", time, stream->packet->timestamp);
  stream->packet->keyframe = keyframe;
  stream->packet_seq = seq;
  memcpy(stream->packet->data,data,len);
  stream->packet->data_size = len;
  //  fprintf(stderr, "* * * New packet: %d\n", stream->packet->data_size);
  }

static int next_packet_asf(bgav_demuxer_context_t * ctx)
  {
  int i, len2, result;
  asf_t * asf;
  asf = (asf_t*)(ctx->priv);
  uint8_t * data_ptr;
  asf_packet_header_t pkt_hdr;
  asf_segment_header_t seg_hdr;

  //  fprintf(stderr, "Next packet %lld\n", asf->data_size);
  //  if(ctx->input->position >= asf->data_start + asf->data_size)
  //    return 0;

  if(asf->hdr.packets_count && (asf->packets_read >= asf->hdr.packets_count))
    return 0;
  
  if(bgav_input_read_data(ctx->input, asf->packet_buffer,
                          asf->packet_size) < asf->packet_size)
    return 0;
  
  data_ptr = asf->packet_buffer +
    read_packet_header(asf, &pkt_hdr, asf->packet_buffer);

  //  fprintf(stderr, "* * * Packet header:\n");
  //  dump_packet_header(&pkt_hdr);
  
  for(i = 0; i < pkt_hdr.segs; i++)
    {
    result = read_segment_header(asf, &pkt_hdr, &seg_hdr, data_ptr);
    if(result < 0)
      {
      fprintf(stderr, "Read segment header failed\n");
      return 0;
      }
    data_ptr += result;
        
    //    fprintf(stderr, "* * * Segment header:\n");
    //    dump_segment_header(&seg_hdr);
    switch(seg_hdr.rlen)
      {
      case 0x01:
        while(seg_hdr.len > 0)
          {
          len2 = *data_ptr;
          data_ptr++;
          //          if(seg_hdr.streamno == 1)
          //          fprintf(stderr, "Add packet 1 Time: %d Stream: %d\n", seg_hdr.x, seg_hdr.streamno);
          add_packet(ctx,
                     data_ptr,          /* Data     */
                     len2,              /* Len      */
                     seg_hdr.streamno,  /* ID       */
                     seg_hdr.seq,       /* Sequence */
                     seg_hdr.x,         /* Time     */
                     pkt_hdr.duration,  /* Duration */ 
                     -1,                /* Offset   */
                     seg_hdr.keyframe); /* Keyframe */
          data_ptr += len2;
          seg_hdr.len-=len2+1;
          seg_hdr.seq++;
          }
        break;
      default:
        {
        //        if(seg_hdr.streamno == 1)
        //        fprintf(stderr, "Add packet 2 Time: %d Stream: %d\n", seg_hdr.time2, seg_hdr.streamno);
        add_packet(ctx,
                   data_ptr,          /* Data     */
                   seg_hdr.len,       /* Len      */
                   seg_hdr.streamno,  /* ID       */
                   seg_hdr.seq,       /* Sequence */
                   seg_hdr.time2,     /* Time     */
                   pkt_hdr.duration,  /* Duration */
                   seg_hdr.x,         /* Offset   */
                   seg_hdr.keyframe); /* Keyframe */
        data_ptr += seg_hdr.len;
        }
      }
        
    }
  return 1;
  }

static void seek_asf(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  int64_t filepos;
  asf_t * asf = (asf_t*)(ctx->priv);
  /* Reset all the streams */
  
#if 0
  fprintf(stderr, "Start: %lld, count: %lld size: %d, Perc: %f, Time: %f/%f\n",
          asf->data_start, asf->hdr.packets_count, asf->packet_size,
          gavl_time_to_seconds(time)/gavl_time_to_seconds(ctx->duration),
          gavl_time_to_seconds(time), gavl_time_to_seconds(ctx->duration));
#endif
  asf->packets_read =
    (int64_t)((double)asf->hdr.packets_count *
              (gavl_time_to_seconds(time)/
               gavl_time_to_seconds(ctx->tt->current_track->duration)));
  
  filepos = asf->data_start +
    asf->packet_size * asf->packets_read;
  
  //  while(1)
  //    {
    //    fprintf(stderr, "Filepos: %lld\n", filepos);

  bgav_input_seek(ctx->input, filepos, SEEK_SET);
  //    fprintf(stderr, "Resync...");

    asf->do_sync = 1;
    while(!bgav_track_has_sync(ctx->tt->current_track))
      {
      next_packet_asf(ctx);
      }
    asf->do_sync = 0;
    
  }


static void close_asf(bgav_demuxer_context_t * ctx)
  {
  int i;
  asf_audio_stream_t * as;

  asf_t * asf = (asf_t *)(ctx->priv);

  if(asf->packet_buffer)
    free(asf->packet_buffer);

  if(asf->stream_bitrates)
    free(asf->stream_bitrates);
  
  for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
    {
    as = (asf_audio_stream_t*)ctx->tt->current_track->audio_streams[i].priv;

    if(ctx->tt->current_track->audio_streams[i].ext_data)
      free(ctx->tt->current_track->audio_streams[i].ext_data);

    if(as->scramble_buffer)
      free(as->scramble_buffer);
    if(ctx->tt->current_track->audio_streams[i].priv)
      free(ctx->tt->current_track->audio_streams[i].priv);
    }
  for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
    {
    if(ctx->tt->current_track->video_streams[i].ext_data)
      free(ctx->tt->current_track->video_streams[i].ext_data);
    
    if(ctx->tt->current_track->video_streams[i].priv)
      free(ctx->tt->current_track->video_streams[i].priv);
    }
  free(ctx->priv);
  }


bgav_demuxer_t bgav_demuxer_asf =
  {
    seek_iterative: 1,
    probe:          probe_asf,
    open:           open_asf,
    next_packet:    next_packet_asf,
    seek:           seek_asf,
    close:          close_asf
  };
