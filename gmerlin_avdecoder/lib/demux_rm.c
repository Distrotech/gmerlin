/*****************************************************************
 
  demux_rm.c
 
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <avdec_private.h>

#define PN_SAVE_ENABLED 0x0001
#define PN_PERFECT_PLAY_ENABLED 0x0002
#define PN_LIVE_BROADCAST 0x0004

#define PN_KEYFRAME_FLAG 0x0002

#define FILE_ID BGAV_MK_FOURCC('.', 'R', 'M', 'F')
#define PROP_ID BGAV_MK_FOURCC('P', 'R', 'O', 'P')
#define MDPR_ID BGAV_MK_FOURCC('M', 'D', 'P', 'R')
#define CONT_ID BGAV_MK_FOURCC('C', 'O', 'N', 'T')
#define DATA_ID BGAV_MK_FOURCC('D', 'A', 'T', 'A')
#define INDX_ID BGAV_MK_FOURCC('I', 'N', 'D', 'X')

static void dump_string(const char * str, int len)
  {
  int  i;
  for(i = 0; i < len; i++)
    fputc(str[i], stderr);
  }

static char * read_data(bgav_input_context_t * input, int len)
  {
  char * ret;
  ret = malloc(len+1);
  if(bgav_input_read_data(input, ret, len) < len)
    {
    free(ret);
    return (char*)0;
    }
  ret[len] = '\0';
  return ret;
  }

/* Chunk header */

typedef struct
  {
  uint32_t id;
  uint32_t size;
  uint16_t version;
  } rm_chunk_t;

static int read_chunk_header(rm_chunk_t * c, bgav_input_context_t * input)
  {
  return bgav_input_read_fourcc(input, &(c->id)) &&
    bgav_input_read_32_be(input, &(c->size))&&
    bgav_input_read_16_be(input, &(c->version));
  }

/* File header */

typedef struct
  {
  /* Version 0 */
  uint32_t file_version;
  uint32_t num_headers;
  } rm_file_header_t;

static int
read_file_header(rm_chunk_t * c,
                 bgav_input_context_t * input,
                 rm_file_header_t * ret)
  {
  return bgav_input_read_32_be(input, &(ret->file_version)) &&
    bgav_input_read_32_be(input, &(ret->num_headers));
  }

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
  } rm_prop_t;

static void dump_prop(rm_prop_t * p)
  {
  fprintf(stderr, "PROP:");
  
  fprintf(stderr, "max_bit_rate:    %d\n", p->max_bit_rate);
  fprintf(stderr, "avg_bit_rate:    %d\n", p->avg_bit_rate);
  fprintf(stderr, "max_packet_size: %d\n", p->max_packet_size);
  fprintf(stderr, "avg_packet_size: %d\n", p->avg_packet_size);
  fprintf(stderr, "num_packets:     %d\n", p->num_packets);
  fprintf(stderr, "duration:        %d\n", p->duration);
  fprintf(stderr, "preroll:         %d\n", p->preroll);
  fprintf(stderr, "index_offset:    %d\n", p->index_offset);
  fprintf(stderr, "data_offset:     %d\n", p->data_offset);
  fprintf(stderr, "num_streams:     %d\n", p->num_streams);
  fprintf(stderr, "flags:           %d\n", p->flags);
  
  }


static int
read_prop(rm_chunk_t * c,
          bgav_input_context_t * input,
          rm_prop_t * ret)
  {
  int result;
  result =
    bgav_input_read_32_be(input, &(ret->max_bit_rate)) &&
    bgav_input_read_32_be(input, &(ret->avg_bit_rate)) &&
    bgav_input_read_32_be(input, &(ret->max_packet_size)) &&
    bgav_input_read_32_be(input, &(ret->avg_packet_size)) &&
    bgav_input_read_32_be(input, &(ret->num_packets)) &&
    bgav_input_read_32_be(input, &(ret->duration)) &&
    bgav_input_read_32_be(input, &(ret->preroll)) &&
    bgav_input_read_32_be(input, &(ret->index_offset)) &&
    bgav_input_read_32_be(input, &(ret->data_offset)) &&
    bgav_input_read_16_be(input, &(ret->num_streams)) &&
    bgav_input_read_16_be(input, &(ret->flags));
  //  dump_prop(ret);
  return result;
  }

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
  } rm_mdpr_t;

static void dump_mdpr(rm_mdpr_t * m)
  {
  fprintf(stderr, "MDPR:\n");
  fprintf(stderr, "stream_number:    %d\n", m->stream_number);
  fprintf(stderr, "max_bit_rate:     %d\n", m->max_bit_rate);
  fprintf(stderr, "avg_bit_rate:     %d\n", m->avg_bit_rate);
  fprintf(stderr, "max_packet_size:  %d\n", m->max_packet_size);
  fprintf(stderr, "avg_packet_size:  %d\n", m->avg_packet_size);
  fprintf(stderr, "start_time:       %d\n", m->start_time);
  fprintf(stderr, "preroll:          %d\n", m->preroll);
  fprintf(stderr, "duration:         %d\n", m->duration);
  fprintf(stderr, "stream_name:      ");
  dump_string(m->stream_name, m->stream_name_size);
  fprintf(stderr, "\nmime_type:        ");
  dump_string(m->mime_type, m->mime_type_size);
  fprintf(stderr, "\ntype_specific_len:  %d\n", m->type_specific_len);
  }

static int
read_mdpr(rm_chunk_t * c,
          bgav_input_context_t * input,
          rm_mdpr_t * ret)
  {
  if(!bgav_input_read_16_be(input, &(ret->stream_number)) ||
     !bgav_input_read_32_be(input, &(ret->max_bit_rate)) ||
     !bgav_input_read_32_be(input, &(ret->avg_bit_rate)) ||
     !bgav_input_read_32_be(input, &(ret->max_packet_size)) ||
     !bgav_input_read_32_be(input, &(ret->avg_packet_size)) ||
     !bgav_input_read_32_be(input, &(ret->start_time)) ||
     !bgav_input_read_32_be(input, &(ret->preroll)) ||
     !bgav_input_read_32_be(input, &(ret->duration)) ||
     !bgav_input_read_8(input, &(ret->stream_name_size)))
    return 0;

  ret->stream_name = read_data(input, ret->stream_name_size);
  if(!ret->stream_name)
    return 0;

  if(!bgav_input_read_8(input, &(ret->mime_type_size)))
    return 0;

  ret->mime_type = read_data(input, ret->mime_type_size);
  if(!ret->mime_type)
    return 0;

  if(!bgav_input_read_32_be(input, &(ret->type_specific_len)))
    return 0;

  if(!ret->type_specific_len)
    return 1;
  
  ret->type_specific_data = read_data(input, ret->type_specific_len);
  if(!ret->type_specific_data)
    return 0;

  //  dump_mdpr(ret);

  
  return 1;
  }

static void destroy_mdpr(rm_mdpr_t * m)
  {
  if(m->stream_name)
    free(m->stream_name);
  if(m->mime_type)
    free(m->mime_type);
  if(m->type_specific_data)
    free(m->type_specific_data);
  }

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
  } rm_cont_t;

/* Data chunk header */

typedef struct
  {
  /* Version 0 */
  uint32_t num_packets;
  uint32_t next_data_header;
  } rm_data_t;

static int read_data_chunk_header(bgav_input_context_t * input, rm_data_t * data)
  {
  return bgav_input_read_32_be(input, &(data->num_packets)) &&
    bgav_input_read_32_be(input, &(data->next_data_header));
  }

/* Index stuff */

typedef struct
  {
  uint16_t version;
  uint32_t timestamp;
  uint32_t offset;
  uint32_t packet_count_for_this_packet;
  } rm_index_record_t;

static int read_index_record(bgav_input_context_t * input,
                             rm_index_record_t * ret)
  {
  return
    bgav_input_read_16_be(input, &(ret->version)) &&
    bgav_input_read_32_be(input, &(ret->timestamp)) &&
    bgav_input_read_32_be(input, &(ret->offset)) &&
    bgav_input_read_32_be(input, &(ret->packet_count_for_this_packet));
  }

typedef struct
  {
  uint32_t            num_indices;
  uint16_t            stream_number;
  uint32_t            next_index_header;
  rm_index_record_t * records;
  } rm_indx_t;

static int read_indx(bgav_input_context_t * input,
                     rm_indx_t * ret)
  {
  int i;
  if(!bgav_input_read_32_be(input, &(ret->num_indices)) ||
     !bgav_input_read_16_be(input, &(ret->stream_number)) ||
     !bgav_input_read_32_be(input, &(ret->next_index_header)))
    return 0;
  if(ret->num_indices)
    {
    ret->records = malloc(ret->num_indices * sizeof(*(ret->records)));
    for(i = 0; i < ret->num_indices; i++)
      {
      if(!read_index_record(input, &(ret->records[i])))
        return 0;
      }
    }
  return 1;
  }

static void free_indx(rm_indx_t * ret)
  {
  if(ret->records)
    free(ret->records);
  memset(ret, 0, sizeof(*ret));
  }
     
static void dump_indx(rm_indx_t * indx)
  {
  int i;
  fprintf(stderr, "num_indices:       %d",   indx->num_indices);
  fprintf(stderr, "stream_number:     %d\n", indx->stream_number);
  fprintf(stderr, "next_index_header: %d\n", indx->next_index_header);
  /*
    uint32_t timestamp;
    uint32_t offset;
    uint32_t packet_count_for_this_packet;
  */

  for(i = 0; i < indx->num_indices; i++)
    {
    fprintf(stderr, "Time: %d, offset: %d, count: %d\n",
            indx->records[i].timestamp,
            indx->records[i].offset,
            indx->records[i].packet_count_for_this_packet);
    }
  }

static uint32_t seek_indx(rm_indx_t * indx, uint32_t millisecs,
                          int32_t * position, int32_t * start_packet,
                          int32_t * end_packet)
  {
  uint32_t ret;
  ret = indx->num_indices - 1;
  fprintf(stderr, "Seek Indx %d\n", millisecs);
  //  dump_indx(indx);
  while(ret)
    {
    if(indx->records[ret].timestamp < millisecs)
      break;
    ret--;
    }
  if(*position > indx->records[ret].offset)
    *position = indx->records[ret].offset;
  if(*start_packet > indx->records[ret].packet_count_for_this_packet)
    *start_packet = indx->records[ret].packet_count_for_this_packet;
  if(*end_packet < indx->records[ret].packet_count_for_this_packet)
    *end_packet = indx->records[ret].packet_count_for_this_packet;
    
  fprintf(stderr, "Ret: %d, Time: %d, offset: %d, count: %d\n",
          ret,
          indx->records[ret].timestamp,
          indx->records[ret].offset,
          indx->records[ret].packet_count_for_this_packet);
  
  return ret;
  }


/* Data packet header */

typedef struct
  {
  uint16_t    object_version;

  /* if (object_version == 0) */

  uint16_t   length;
  uint16_t   stream_number;
  uint32_t   timestamp;
  uint8_t   reserved; 
  uint8_t   flags; 
  } rm_packet_header_t;

int packet_header_read(bgav_input_context_t * input,
                       rm_packet_header_t * ret)
  {
  return
    bgav_input_read_16_be(input, &(ret->object_version)) &&
    bgav_input_read_16_be(input, &(ret->length)) &&
    bgav_input_read_16_be(input, &(ret->stream_number)) &&
    bgav_input_read_32_be(input, &(ret->timestamp)) &&
    bgav_input_read_8(input, &(ret->reserved)) &&
    bgav_input_read_8(input, &(ret->flags));
  }

static void packet_header_dump(rm_packet_header_t * h)
  {
  fprintf(stderr, "Packet L: %d, S: %d, T: %d, F: %x\n",
          h->length, h->stream_number, h->timestamp, h->flags);
  }

/* Audio and video stream specific stuff */

typedef struct
  {
  rm_mdpr_t mdpr;
  rm_indx_t indx;

  uint16_t version;
  uint16_t flavor;
  uint32_t coded_frame_size;
  uint16_t sub_packet_h;
  uint16_t frame_size;
  uint16_t sub_packet_size;
  
  //  uint8_t * extradata;
  int bytes_to_read;
  int has_index;
  uint32_t index_record;
  } rm_audio_stream_t;

static void dump_audio(rm_audio_stream_t*s)
  {
  fprintf(stderr, "Audio stream:\n");
  fprintf(stderr, "version %d\n", s->version);
  fprintf(stderr, "flavor: %d\n", s->flavor);
  fprintf(stderr, "coded_frame_size: %d\n", s->coded_frame_size);
  fprintf(stderr, "sub_packet_h %d\n", s->sub_packet_h);
  fprintf(stderr, "frame_size %d\n", s->frame_size);
  fprintf(stderr, "sub_packet_size %d\n", s->sub_packet_size);
  fprintf(stderr, "Bytes to read %d\n", s->bytes_to_read);
  
  //  uint8_t * extradata;

  }

static void init_audio_stream(bgav_demuxer_context_t * ctx,
                              rm_mdpr_t * mdpr,
                              uint8_t * data)
  {
  uint16_t codecdata_length;
  uint16_t samplesize;
  bgav_stream_t * bg_as;
  rm_audio_stream_t * rm_as;
  uint8_t desc_len;
  bg_as = bgav_demuxer_add_audio_stream(ctx);
  rm_as = calloc(1, sizeof(*rm_as));

  bg_as->priv = rm_as;

  /* Skip initial header */
  data += 4;
  
  rm_as->version = BGAV_PTR_2_16BE(data);data+=2;
  //  fprintf(stderr,"version: %d\n", rm_as->version);
  //		    stream_skip(demuxer->stream, 2); /* version (4 or 5) */
  data += 2; // 00 00
  data += 4; /* .ra4 or .ra5 */
  data += 4; // ???
  data += 2; /* version (4 or 5) */
  data += 4; // header size == 0x4E
  rm_as->flavor = BGAV_PTR_2_16BE(data);data+=2;/* codec flavor id */
  rm_as->coded_frame_size = BGAV_PTR_2_32BE(data);data+=4;/* needed by codec */
  data += 4; // big number
  data += 4; // bigger number
  data += 4; // 2 || -''-
  // data += 2; // 0x10
  rm_as->sub_packet_h = BGAV_PTR_2_16BE(data);data+=2;
  //		    data += 2; /* coded frame size */
  rm_as->frame_size = BGAV_PTR_2_16BE(data);data+=2;
  // fprintf(stderr,"frame_size: %d\n", rm_as->frame_size);
  rm_as->sub_packet_size = BGAV_PTR_2_16BE(data);data+=2;
  //  fprintf(stderr,"sub_packet_size: %d\n", rm_as->sub_packet_size);
  data += 2; // 0
      
  if (rm_as->version == 5)
    data += 6; //0,srate,0
  
  bg_as->data.audio.format.samplerate = BGAV_PTR_2_16BE(data);data+=2;
  data += 2;  // 0
  samplesize = BGAV_PTR_2_16BE(data);data += 2;
  samplesize /= 8;
  bg_as->data.audio.bits_per_sample = samplesize * 8;
  bg_as->data.audio.format.num_channels = BGAV_PTR_2_16BE(data);data+=2;
  //  fprintf(stderr,"samplerate: %d, channels: %d\n",
  //          bg_as->format.samplerate, bg_as->format.num_channels);
  if (rm_as->version == 5)
    {
    data += 4;  // "genr"
    bg_as->fourcc = BGAV_PTR_2_FOURCC(data);data += 4;
    }
  else
    {		
    /* Desc #1 */
    desc_len = *data;
    data += desc_len+1;
    /* Desc #2 */
    desc_len = *data;
    data++;
    bg_as->fourcc = BGAV_PTR_2_FOURCC(data);
    data += desc_len;
    }

  /* Get extradata for codec */

  data += 3;
  if(rm_as->version == 5)
    data++;

  data += 2; /* Better for version 5 and cook codec */
    
  codecdata_length = BGAV_PTR_2_16BE(data);data+=2;
  
  fprintf(stderr, "Codecdata length: %d\n", codecdata_length);
  
  bg_as->ext_size = 10+codecdata_length;
  bg_as->ext_data = malloc(bg_as->ext_size);

  bg_as->data.audio.block_align = rm_as->frame_size;
  
  ((short*)(bg_as->ext_data))[0]=rm_as->sub_packet_size;
  ((short*)(bg_as->ext_data))[1]=rm_as->sub_packet_h;
  ((short*)(bg_as->ext_data))[2]=rm_as->flavor;
  ((short*)(bg_as->ext_data))[3]=rm_as->coded_frame_size;
  ((short*)(bg_as->ext_data))[4]=codecdata_length;

  memcpy(&(((short*)(bg_as->ext_data))[5]), data, codecdata_length);
  
  //  gavl_audio_format_dump(&(bg_as->data.audio.format));
  bg_as->stream_id = mdpr->stream_number;
  
  if(bg_as->fourcc == BGAV_MK_FOURCC('1','4','_','4'))
    {
    rm_as->bytes_to_read = bg_as->data.audio.block_align;
    }
  else
    {
    rm_as->bytes_to_read = bg_as->data.audio.block_align * rm_as->sub_packet_h;
    }
  //  dump_audio(rm_as);
  }

typedef struct
  {
  rm_mdpr_t mdpr;
  rm_indx_t indx;
  int has_index;

  uint32_t index_record;
  
  uint32_t kf_pts;
  uint32_t kf_base;
  
  } rm_video_stream_t;

static void init_video_stream(bgav_demuxer_context_t * ctx,
                              rm_mdpr_t * mdpr,
                              uint8_t * data)
  {
  uint32_t tmp;
  bgav_stream_t * bg_vs;
  rm_video_stream_t * rm_vs;
  uint32_t format_le;
  
  bg_vs = bgav_demuxer_add_video_stream(ctx);
  rm_vs = calloc(1, sizeof(*rm_vs));

  bg_vs->ext_size = 3 * sizeof(uint32_t);
  bg_vs->ext_data = calloc(bg_vs->ext_size, 1);
  bg_vs->priv = rm_vs;

  data += 4;

  bg_vs->fourcc = BGAV_PTR_2_FOURCC(data);data+=4;

  // fprintf(stderr,"video fourcc: %.4s (%x)\n", (char *)&bg_vs->fourcc, bg_vs->fourcc);
  
  /* emulate BITMAPINFOHEADER */

  bg_vs->data.video.format.frame_width = BGAV_PTR_2_16BE(data);data+=2;
  bg_vs->data.video.format.frame_height = BGAV_PTR_2_16BE(data);data+=2;

  bg_vs->data.video.format.image_width = bg_vs->data.video.format.frame_width;
  bg_vs->data.video.format.image_height = bg_vs->data.video.format.frame_height;

  bg_vs->data.video.format.pixel_width = 1;
  bg_vs->data.video.format.pixel_height = 1;
    
  bg_vs->data.video.format.framerate_num = BGAV_PTR_2_16BE(data);data+=2;
  bg_vs->data.video.format.framerate_den = 1;
  // we probably won't even care about fps
  if (bg_vs->data.video.format.framerate_num<=0) bg_vs->data.video.format.framerate_num=24; 
  
#if 1
  data += 4;
#else
  printf("unknown1: 0x%X  \n",BGAV_PTR_2_32BE(data));data+=4;
  printf("unknown2: 0x%X  \n",stream_read_word(demuxer->stream));
  printf("unknown3: 0x%X  \n",stream_read_word(demuxer->stream));
#endif
  //		    if(sh->format==0x30335652 || sh->format==0x30325652 )
  if(1)
    {
    tmp=BGAV_PTR_2_16BE(data);data+=2;
    if(tmp>0){
    bg_vs->data.video.format.framerate_num = tmp;
    }
    } else {
    int fps=BGAV_PTR_2_16BE(data);data+=2;
    printf("realvid: ignoring FPS = %d\n",fps);
    }
  data += 2;
  
  // read codec sub-format (to make difference between low and high rate codec)
  //  ((unsigned int*)(sh->bih+1))[0]=BGAV_PTR_2_32BE(data);data+=4;
  ((uint32_t*)(bg_vs->ext_data))[0] = BGAV_PTR_2_32BE(data);data+=4;
  
  /* h263 hack */
  tmp = BGAV_PTR_2_32BE(data);data+=4;
  ((uint32_t*)(bg_vs->ext_data))[1] = tmp;
  //  fprintf(stderr,"H.263 ID: %x\n", tmp);
  switch (tmp)
    {
    case 0x10000000:
      /* sub id: 0 */
      /* codec id: rv10 */
      break;
    case 0x10003000:
    case 0x10003001:
      /* sub id: 3 */
      /* codec id: rv10 */
      bg_vs->fourcc = BGAV_MK_FOURCC('R', 'V', '1', '3');
      break;
    case 0x20001000:
    case 0x20100001:
    case 0x20200002:
      /* codec id: rv20 */
      break;
    case 0x30202002:
      /* codec id: rv30 */
      break;
    case 0x40000000:
    case 0x40002000:
      /* codec id: rv40 */
      break;
    default:
      /* codec id: none */
      fprintf(stderr,"unknown id: %x\n", tmp);
    }

  format_le =
    ((bg_vs->fourcc & 0x000000FF) << 24) ||
    ((bg_vs->fourcc & 0x0000FF00) << 8) ||
    ((bg_vs->fourcc & 0x00FF0000) >> 8) ||
    ((bg_vs->fourcc & 0xFF000000) >> 24);
  
  if((format_le<=0x30335652) && (tmp>=0x20200002)){
  // read secondary WxH for the cmsg24[] (see vd_realvid.c)
  ((unsigned short*)(bg_vs->ext_data))[4]=4*(unsigned short)(*data); //widht
  data++;
  ((unsigned short*)(bg_vs->ext_data))[5]=4*(unsigned short)(*data); //height
  data++;
  } 
  
  bg_vs->data.video.format.free_framerate = 1;
  bg_vs->stream_id = mdpr->stream_number;
  //  gavl_video_format_dump(&(bg_vs->format));
  }

typedef struct
  {
  rm_data_t data_chunk_header;
  uint32_t data_start;
  uint32_t data_size;
  int do_seek;
  uint32_t next_packet;

  uint32_t first_timestamp;
  int need_first_timestamp;
  } rm_private_t;

static bgav_stream_t * find_stream(bgav_demuxer_context_t * ctx,
                                   int id)
  {
  int i;
  for(i = 0; i < ctx->num_audio_streams; i++)
    {
    if(ctx->audio_streams[i].stream_id == id)
      return &(ctx->audio_streams[i]);
    }
  for(i = 0; i < ctx->num_video_streams; i++)
    {
    if(ctx->video_streams[i].stream_id == id)
      return &(ctx->video_streams[i]);
    }
  return (bgav_stream_t*)0;
  }

/*
 *  The metadata are ASCII format, so we can read them
 *  here as UTF-8
 */

#define READ_STRING(dst) \
  if(!bgav_input_read_16_be(ctx->input, &len)) \
    return 0;\
  if(len) \
    { \
    buf = malloc(len); \
    if(bgav_input_read_data(ctx->input, buf, len) < len) \
      { \
      free(buf); \
      return 0; \
      } \
    ctx->metadata.dst = bgav_convert_string(buf, len, "ISO-8859-1", "UTF-8"); \
    free(buf); \
    }

static int read_cont(bgav_demuxer_context_t * ctx)
  {
  uint16_t len;
  char * buf;
  fprintf(stderr, "Read Cont\n");
  
  /* Title */
  
  READ_STRING(title);
  
  /* Author */

  READ_STRING(author);

  /* Copyright */

  READ_STRING(copyright);
  
  /* Comment */

  READ_STRING(comment);
    
  return 1;
  }

int open_rmff(bgav_demuxer_context_t * ctx)
  {
  int i;
  int keep_going = 1;
  bgav_stream_t * index_stream;
  int64_t file_position;
  rm_audio_stream_t * as;
  rm_video_stream_t * vs;
  
  rm_chunk_t chunk;
  rm_prop_t prop;
  rm_mdpr_t mdpr;
  rm_file_header_t file_header;
  rm_private_t * priv;
  uint8_t * pos;
  /* File header */
  uint32_t header = 0;
  uint32_t next_index_header;
  fprintf(stderr, "Open rmff\n");
  rm_indx_t indx;
  if(!read_chunk_header(&chunk, ctx->input))
    return 0;
  
  if(chunk.id != FILE_ID)
    return 0;

  if(!read_file_header(&chunk,ctx->input, &file_header))
    return 0;

  fprintf(stderr, "Header size: %d\n", chunk.size);

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  while(keep_going)
    {
    if(!read_chunk_header(&chunk, ctx->input))
      return 0;
    switch(chunk.id)
      {
      case PROP_ID:
        //        fprintf(stderr, "Properties\n");
        if(!read_prop(&chunk,ctx->input, &prop))
          goto fail;
        break;
      case MDPR_ID:
        fprintf(stderr, "Media Properties\n");
        if(!read_mdpr(&chunk,ctx->input, &mdpr))
          return 0;
        
        /* Check, what stream this is (From here on, it gets ugly) */
        i = mdpr.type_specific_len - 4;
        pos = mdpr.type_specific_data;
        while(i)
          {
          header = BGAV_PTR_2_FOURCC(pos);
          if((header == BGAV_MK_FOURCC('.', 'r', 'a', 0xfd)) ||
             (header == BGAV_MK_FOURCC('V', 'I', 'D', 'O')))
            break;
          pos++;
          i--;
          }
        if(header == BGAV_MK_FOURCC('.', 'r', 'a', 0xfd))
          {
          //          fprintf(stderr, "Found audio stream\n");
          init_audio_stream(ctx, &mdpr, pos);
          }
        else if(header == BGAV_MK_FOURCC('V', 'I', 'D', 'O'))
          {
          //          fprintf(stderr, "Found video stream\n");
          init_video_stream(ctx, &mdpr, pos);
          }
        else
          {
          fprintf(stderr, "Unknown stream type: %s\n", mdpr.mime_type);
          }
        destroy_mdpr(&mdpr);
        break;
      case CONT_ID:
        //        fprintf(stderr, "Content description\n");
        if(!read_cont(ctx))
          goto fail;
        break;
      case DATA_ID:
        //        fprintf(stderr, "Data\n");
        
        /* Read data chunk header */
        if(!read_data_chunk_header(ctx->input, &(priv->data_chunk_header)))
          goto fail;
        
        /* We reached the data section. Now check if the file is seekabkle
           and read the indices */
        priv->data_start = file_position = ctx->input->position;
        if(chunk.size > 10)
          priv->data_size = chunk.size - 10;
        else
          priv->data_size = 0;
        if(ctx->input->input->seek_byte && prop.index_offset)
          {
          bgav_input_seek(ctx->input, prop.index_offset, SEEK_SET);
          
          /* TODO: Read stream indices */
          
          while(1)
            {
            read_chunk_header(&chunk, ctx->input);
            //            fprintf(stderr, "Index fourcc: ");
            if(chunk.id != INDX_ID)
              {
              fprintf(stderr, "No index found, where I expected one\n");
              break;
              }
            read_indx(ctx->input, &indx);
            next_index_header = indx.next_index_header;
            
            //            dump_indx(&indx);
            index_stream = find_stream(ctx, indx.stream_number);
            if(index_stream)
              {
              if(index_stream->type == BGAV_STREAM_AUDIO)
                {
                as = (rm_audio_stream_t*)(index_stream->priv);
                as->has_index = 1;
                memcpy(&(as->indx), &indx, sizeof(indx));
                memset(&indx, 0, sizeof(indx));
                }
              else if(index_stream->type == BGAV_STREAM_VIDEO)
                {
                vs = (rm_video_stream_t*)(index_stream->priv);
                vs->has_index = 1;
                memcpy(&(vs->indx), &indx, sizeof(indx));
                memset(&indx, 0, sizeof(indx));
                }
              else
                free_indx(&indx);
              }
            else
              {
              free_indx(&indx);
              }
            
            if(next_index_header)
              {
              bgav_input_skip(ctx->input,
                              next_index_header - ctx->input->position);
              fprintf(stderr, "Skipping %lld bytes",
                      next_index_header - ctx->input->position);
              }
            else
              break;
            }
          bgav_input_seek(ctx->input, priv->data_start, SEEK_SET);
          }
        keep_going = 0;
        //        bgav_input_skip(ctx->input, chunk.size - 10);
        break;
      case INDX_ID:
        fprintf(stderr, "Index\n");
        bgav_input_skip(ctx->input, chunk.size - 10);
        break;
      }
    }

  /* Update global fields */
  
  ctx->duration = prop.duration * (GAVL_TIME_SCALE / 1000); 

  //  if((prop.flags & PN_LIVE_BROADCAST) || !(ctx->input->
  //    {
    //    fprintf(stderr, "Playing live broadcast\n");
    priv->need_first_timestamp = 1;
    //    }
  
  if(ctx->input->input->seek_byte)
    {
    ctx->can_seek = 1;
    for(i = 0; i < ctx->num_audio_streams; i++)
      {
      if(!((rm_audio_stream_t*)(ctx->audio_streams[i].priv))->has_index)
        ctx->can_seek = 0;
      //      else
      //        dump_indx(&(((rm_audio_stream_t*)(ctx->audio_streams[i].priv))->indx));
      }
    for(i = 0; i < ctx->num_video_streams; i++)
      {
      if(!((rm_video_stream_t*)(ctx->video_streams[i].priv))->has_index)
        ctx->can_seek = 0;
      //      else
      //        dump_indx(&(((rm_video_stream_t*)(ctx->video_streams[i].priv))->indx));
      }
    }

  //  fprintf(stderr, "Can seek: %d\n", ctx->can_seek);

  ctx->stream_description = bgav_sprintf("Real Media file format");
  
  return 1;
  fail:
  
  return 0;
  }

int probe_rmff(bgav_input_context_t * input)
  {
  uint32_t header;

  if(!bgav_input_get_fourcc(input, &header))
    return 0;
  
  if(header == FILE_ID)
    {
    fprintf(stderr, "Detected Real video format\n");
    return 1;
    }
  return 0;
  }

#define READ_8(b) if(!bgav_input_read_8(ctx->input,&b))return 0;len--
#define READ_16(b) if(!bgav_input_read_16_be(ctx->input,&b))return 0;len-=2

typedef struct dp_hdr_s {
    uint32_t chunks;    // number of chunks
    uint32_t timestamp; // timestamp from packet header
    uint32_t len;       // length of actual data
    uint32_t chunktab;  // offset to chunk offset array
} dp_hdr_t;

#define SKIP_BITS(n) buffer<<=n
#define SHOW_BITS(n) ((buffer)>>(32-(n)))

int64_t fix_timestamp(bgav_stream_t * stream, uint8_t * s, uint32_t timestamp)
  {
  uint32_t buffer= (s[0]<<24) + (s[1]<<16) + (s[2]<<8) + s[3];
  int kf=timestamp;
  int pict_type;
  int orig_kf;
  rm_video_stream_t * priv = (rm_video_stream_t*)stream->priv;
#if 1
  if(stream->fourcc==BGAV_MK_FOURCC('R','V','3','0') ||
     stream->fourcc==BGAV_MK_FOURCC('R','V','4','0'))
    {
    if(stream->fourcc==BGAV_MK_FOURCC('R','V','3','0'))
      {
      SKIP_BITS(3);
      pict_type= SHOW_BITS(2);
      SKIP_BITS(2 + 7);
      }
    else
      {
      SKIP_BITS(1);
      pict_type= SHOW_BITS(2);
      SKIP_BITS(2 + 7 + 3);
      }
    orig_kf=kf= SHOW_BITS(13);  //    kf= 2*SHOW_BITS(12);
    //    if(pict_type==0)
    if(pict_type<=1)
      {
      // I frame, sync timestamps:
      priv->kf_base=timestamp-kf;
      //      fprintf(stderr, "\nTS: base=%d\n",priv->kf_base);
      kf=timestamp;
      }
    else
      {
      // P/B frame, merge timestamps:
      int tmp=timestamp-priv->kf_base;
      kf|=tmp&(~0x1fff);        // combine with packet timestamp
      if(kf<tmp-4096) kf+=8192; else // workaround wrap-around problems
      if(kf>tmp+4096) kf-=8192;
      kf+=priv->kf_base;
      }
    if(pict_type != 3)
      { // P || I  frame -> swap timestamps
      int tmp=kf;
      kf=priv->kf_pts;
      priv->kf_pts=tmp;
      //      if(kf<=tmp) kf=0;
      }
    //    if(verbose>1) printf("\nTS: %08X -> %08X (%04X) %d %02X %02X %02X %02X %5d\n",timestamp,kf,orig_kf,pict_type,s[0],s[1],s[2],s[3],kf-(int)(1000.0*priv->v_pts));
    }
#endif
  //  fprintf(stderr, "Real fix timestamp: %d\n", (kf * GAVL_TIME_SCALE) / 1000);
  return ((int64_t)kf * GAVL_TIME_SCALE) / 1000;
  }

static int process_video_chunk(bgav_demuxer_context_t * ctx,
                               rm_packet_header_t * h,
                               bgav_stream_t * stream)
  {
  /* Video sub multiplexing */
  uint8_t tmp_8;
  uint16_t tmp_16;
  bgav_packet_t * p;
  int vpkg_header, vpkg_length, vpkg_offset;
  int vpkg_seqnum=-1;
  int vpkg_subseq=0;
  dp_hdr_t* dp_hdr;
  unsigned char* dp_data;
  uint32_t* extra;
  int len = h->length - 12;

  while(len > 2)
    {
    vpkg_length = 0;

    // read packet header
    // bit 7: 1=last block in block chain
    // bit 6: 1=short header (only one block?)

    READ_8(tmp_8);
    vpkg_header=tmp_8;
    
    if (0x40==(vpkg_header&0xc0))
      {
      // seems to be a very short header
      // 2 bytes, purpose of the second byte yet unknown
      bgav_input_skip(ctx->input, 1);
      len--;
      vpkg_offset=0;
      vpkg_length=len;
      //      fprintf(stderr, "Short header\n");
      }
    else
      {
      if (0==(vpkg_header&0x40))
        {
        // sub-seqnum (bits 0-6: number of fragment. bit 7: ???)
        READ_8(tmp_8);
        vpkg_subseq=tmp_8;
        //        fprintf(stderr,  "subseq: %0.2X ",vpkg_subseq);
        vpkg_subseq&=0x7f;
        }
      
      // size of the complete packet
      // bit 14 is always one (same applies to the offset)

      READ_16(tmp_16);
      vpkg_length=tmp_16;

      //      fprintf(stderr, "l: %0.2X %0.2X ",vpkg_length>>8,vpkg_length&0xff);
      if (!(vpkg_length&0xC000))
        {
        vpkg_length<<=16;
        READ_16(tmp_16);
        vpkg_length|=tmp_16;
        //        fprintf(stderr, "l+: %0.2X %0.2X ",(vpkg_length>>8)&0xff,vpkg_length&0xff);
        }
      else
        vpkg_length&=0x3fff;

      // offset of the following data inside the complete packet
      // Note: if (hdr&0xC0)==0x80 then offset is relative to the
      // _end_ of the packet, so it's equal to fragment size!!!

      READ_16(tmp_16);
      vpkg_offset = tmp_16;

      // fprintf(stderr, "o: %0.2X %0.2X ",vpkg_offset>>8,vpkg_offset&0xff);
      if (!(vpkg_offset&0xC000))
        {
        vpkg_offset<<=16;
        READ_16(tmp_16);
        vpkg_offset|=tmp_16;
        //        fprintf(stderr, "o+: %0.2X %0.2X ",(vpkg_offset>>8)&0xff,vpkg_offset&0xff);
        }
      else
        vpkg_offset&=0x3fff;
      READ_8(tmp_8);
      vpkg_seqnum=tmp_8;
      //      vpkg_seqnum=stream_read_char(demuxer->stream); --len;
      //      fprintf(stderr, "seq: %0.2X ",vpkg_seqnum);
      }
    //    fprintf(stderr, "vpkg_header: %08x vpkg_length: %d, vpkg_offset %d vpkg_seqnum: %d len: %d\n",
    //            vpkg_header, vpkg_length, vpkg_offset, vpkg_seqnum, len);
    //    fprintf(stderr, "\n");
    //    fprintf(stderr, "blklen=%d\n", len);
    //    mp_msg(MSGT_DEMUX,MSGL_DBG2, "block: hdr=0x%0x, len=%d, offset=%d, seqnum=%d\n",
    //           vpkg_header, vpkg_length, vpkg_offset, vpkg_seqnum);
    if(stream->packet)
      {
      p=stream->packet;
      dp_hdr=(dp_hdr_t*)p->data;
      dp_data=p->data+sizeof(dp_hdr_t);
      extra=(uint32_t*)(p->data+dp_hdr->chunktab);
      //      fprintf(stderr,
      //              "we have an incomplete packet (oldseq=%d new=%d)\n",vs->seqnum,vpkg_seqnum);
      // we have an incomplete packet:
      if(stream->packet_seq!=vpkg_seqnum)
        {
        // this fragment is for new packet, close the old one
        //        fprintf(stderr,
        //                "closing probably incomplete packet\n");

        /* TODO: Timestamps */
        p->timestamp=(dp_hdr->len<3)?0:
          fix_timestamp(stream,dp_data,dp_hdr->timestamp);
        bgav_packet_done_write(p);
        // ds_add_packet(ds,dp);
        stream->packet = (bgav_packet_t*)0;
        // ds->asf_packet=NULL;
        }
      else
        {
        // append data to it!
        ++dp_hdr->chunks;
        //        mp_msg(MSGT_DEMUX,MSGL_DBG2,"[chunks=%d  subseq=%d]\n",dp_hdr->chunks,vpkg_subseq);
        if(dp_hdr->chunktab+8*(1+dp_hdr->chunks)>p->data_alloc)
          {
          // increase buffer size, this should not happen!
          //          mp_msg(MSGT_DEMUX,MSGL_WARN, "chunktab buffer too small!!!!!\n");
          bgav_packet_alloc(p, dp_hdr->chunktab+8*(4+dp_hdr->chunks));
          p->data_size = dp_hdr->chunktab+8*(4+dp_hdr->chunks);
          //          dp->len=dp_hdr->chunktab+8*(4+dp_hdr->chunks);
          //          dp->buffer=realloc(dp->buffer,dp->len);
          // re-calc pointers:
          dp_hdr=(dp_hdr_t*)p->data;
          dp_data=p->data+sizeof(dp_hdr_t);
          extra=(uint32_t*)(p->data+dp_hdr->chunktab);
          }
        extra[2*dp_hdr->chunks+0]=1;
        extra[2*dp_hdr->chunks+1]=dp_hdr->len;
        if(0x80==(vpkg_header&0xc0))
          {
          // last fragment!
          //          if(dp_hdr->len!=vpkg_length-vpkg_offset)
          //            mp_msg(MSGT_DEMUX,MSGL_V,
          //                   "warning! assembled.len=%d  frag.len=%d  total.len=%d  \n",
          //                   dp->len,vpkg_offset,vpkg_length-vpkg_offset);
          if(bgav_input_read_data(ctx->input, dp_data+dp_hdr->len, vpkg_offset) < vpkg_offset)
            return 0;
          len-=vpkg_offset;
          //          stream_read(demuxer->stream, dp_data+dp_hdr->len, vpkg_offset);
          if(dp_data[dp_hdr->len]&0x20)
            --dp_hdr->chunks;
          else
            dp_hdr->len+=vpkg_offset;
          //          mp_dbg(MSGT_DEMUX,MSGL_DBG2,
          //                 "fragment (%d bytes) appended, %d bytes left\n",
          //                 vpkg_offset,len);
          // we know that this is the last fragment -> we can close the packet!
          /* TODO: Timestamp */
#if 1
          p->timestamp=(dp_hdr->len<3)?0:
            fix_timestamp(stream,dp_data,dp_hdr->timestamp);
#endif
          bgav_packet_done_write(p);
          stream->packet = (bgav_packet_t*)0;
          // ds_add_packet(ds,dp);
          // ds->asf_packet=NULL;
          // continue parsing
          continue;
          }
        // non-last fragment:
        // if(dp_hdr->len!=vpkg_offset)
        //  mp_msg(MSGT_DEMUX,MSGL_V,
        //         "warning! assembled.len=%d  offset=%d  frag.len=%d  total.len=%d  \n",
        //         dp->len,vpkg_offset,len,vpkg_length);
        if(bgav_input_read_data(ctx->input, dp_data+dp_hdr->len, len) < len)
          return 0;
        //        stream_read(demuxer->stream, dp_data+dp_hdr->len, len);
        if(dp_data[dp_hdr->len]&0x20)
          --dp_hdr->chunks;
        else
          dp_hdr->len+=len;
        len=0;
        break; // no more fragments in this chunk!
        }
      }
    // create new packet!

    p = bgav_packet_buffer_get_packet_write(stream->packet_buffer);
    bgav_packet_alloc(p, sizeof(dp_hdr_t)+vpkg_length+8*(1+2*(vpkg_header&0x3F)));
    p->data_size = sizeof(dp_hdr_t)+vpkg_length+8*(1+2*(vpkg_header&0x3F));
        
    //    dp = new_demux_packet(sizeof(dp_hdr_t)+vpkg_length+8*(1+2*(vpkg_header&0x3F)));
    // the timestamp seems to be in milliseconds
    //    dp->pts = 0; // timestamp/1000.0f; //timestamp=0;
    //    dp->pos = demuxer->filepos;
    //    dp->flags = (flags & 0x2) ? 0x10 : 0;
    stream->packet_seq = vpkg_seqnum;
    //    ds->asf_seq = vpkg_seqnum;
    dp_hdr=(dp_hdr_t*)p->data;
    dp_hdr->chunks=0;
    dp_hdr->timestamp=h->timestamp;
    //    fprintf(stderr, "Packet timestamp: %d\n", h->timestamp);
    dp_hdr->chunktab=sizeof(dp_hdr_t)+vpkg_length;
    dp_data=p->data+sizeof(dp_hdr_t);
    extra=(uint32_t*)(p->data+dp_hdr->chunktab);
    extra[0]=1; extra[1]=0; // offset of the first chunk
    if(0x00==(vpkg_header&0xc0))
      {
      // first fragment:
      dp_hdr->len=len;
      if(bgav_input_read_data(ctx->input, dp_data, len) < len)
        return 0;
      //      stream_read(demuxer->stream, dp_data, len);
      stream->packet=p;
      len=0;
      break;
      }
    // whole packet (not fragmented):
    dp_hdr->len=vpkg_length; len-=vpkg_length;

    if(bgav_input_read_data(ctx->input, dp_data, vpkg_length) < vpkg_length)
      return 0;

    //    stream_read(demuxer->stream, dp_data, vpkg_length);
    /* TODO: Timestanp */
    p->timestamp=(dp_hdr->len<3)?0:
      fix_timestamp(stream,dp_data,dp_hdr->timestamp);
    bgav_packet_done_write(p);
    
    // ds_add_packet(ds,dp);
    
    } // while(len>0)
  
  if(len)
    {
    fprintf(stderr, "\n******** !!!!!!!! BUG!! len=%d !!!!!!!!!!! ********\n",len);
    if(len>0)
      bgav_input_skip(ctx->input, len);
      // stream_skip(demuxer->stream, len);
    }
  return 1;
  }

static int process_audio_chunk(bgav_demuxer_context_t * ctx,
                               rm_packet_header_t * h,
                               bgav_stream_t * stream)
  {
  int bytes_to_read;
  int packet_size;
  rm_audio_stream_t * as;

  as = (rm_audio_stream_t*)stream->priv;

  packet_size = h->length - 12;
  
  if(!stream->packet) /* Get a new packet */
    {
    stream->packet = bgav_packet_buffer_get_packet_write(stream->packet_buffer);
    bgav_packet_alloc(stream->packet, as->bytes_to_read);
    stream->packet->data_size = 0;
    }
  
  bytes_to_read = (stream->packet->data_alloc - stream->packet->data_size < packet_size) ?
    stream->packet->data_alloc - stream->packet->data_size : packet_size;
    
  if(bgav_input_read_data(ctx->input, &(stream->packet->data[stream->packet->data_size]),
                          bytes_to_read) < bytes_to_read)
    return 0;

  if(bytes_to_read < packet_size)
    {
    fprintf(stderr, "Warning: Packet doesn't fit into buffer\n");
    bgav_input_skip(ctx->input, packet_size - bytes_to_read);
    }
  
  stream->packet->data_size += bytes_to_read;

  if(stream->packet->data_size >= as->bytes_to_read)
    {
    bgav_packet_done_write(stream->packet);
    stream->packet = (bgav_packet_t*)0;
    }
  
  return 1;
  }

static int next_packet_rmff(bgav_demuxer_context_t * ctx)
  {
  //  bgav_packet_t * p;
  bgav_stream_t * stream;
  rm_private_t * rm;
  rm_packet_header_t h;
  rm_audio_stream_t * as;
  rm_video_stream_t * vs;
  int result = 0;
  rm = (rm_private_t*)(ctx->priv);
  //  fprintf(stderr, "Data size: %d\n", rm->data_size);
    
  if(rm->data_size && (ctx->input->position + 10 >= rm->data_start + rm->data_size))
    return 0;
  if(!packet_header_read(ctx->input, &h))
    return 0;

  if(rm->need_first_timestamp)
    {
    rm->first_timestamp = h.timestamp;
    rm->need_first_timestamp = 0;
    //    fprintf(stderr, "First timestamp: %d\n", rm->first_timestamp);
    }
  h.timestamp -= rm->first_timestamp;
  
  //  if(rm->do_seek)
  //  packet_header_dump(&h);
  stream = bgav_demuxer_find_stream(ctx, h.stream_number);

  if(!stream) /* Skip unknown stuff */
    {
    bgav_input_skip(ctx->input, h.length - 12);
    return 1;
    }
  if(stream->type == BGAV_STREAM_VIDEO)
    {
    if(rm->do_seek)
      {
      vs = (rm_video_stream_t*)(stream->priv);
      //      dump_indx(
      if(vs->indx.records[vs->index_record].packet_count_for_this_packet > rm->next_packet)
        {
        //        fprintf(stderr, "Skipping video packet\n");
        bgav_input_skip(ctx->input, h.length - 12);
        result = 1;
        }
      else
        result = process_video_chunk(ctx, &h, stream);
      }
    else
      result = process_video_chunk(ctx, &h, stream);
    }
  else if(stream->type == BGAV_STREAM_AUDIO)
    {
    if(rm->do_seek)
      {
      as = (rm_audio_stream_t*)(stream->priv);
      if(as->indx.records[as->index_record].packet_count_for_this_packet > rm->next_packet)
        {
        //        fprintf(stderr, "Skipping audio packet\n");
        bgav_input_skip(ctx->input, h.length - 12);
        result = 1;
        }
      else
        result = process_audio_chunk(ctx, &h, stream);
      }
    else
      result = process_audio_chunk(ctx, &h, stream);
    }
  rm->next_packet++;
  return result;
  }

void seek_rmff(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  uint32_t i;
  bgav_stream_t * stream;
  rm_audio_stream_t * as;
  rm_video_stream_t * vs;
  rm_private_t * rm;
  
  uint32_t real_time; /* We'll never be more accurate than milliseconds */
  uint32_t position = ~0x0;

  uint32_t start_packet = ~0x00;
  uint32_t end_packet =    0x00;
  
  //  fprintf(stderr, "Seek RMFF\n");
  
  rm = (rm_private_t*)(ctx->priv);
    
  real_time = (time * 1000) / GAVL_TIME_SCALE;
  
  /* First step: Seek the index records for each stream */

  /* Seek the pointers for the index records for each stream */
  /* We also calculate the file position where we will start again */
  
  for(i = 0; i < ctx->supported_video_streams; i++)
    {
    stream = &(ctx->video_streams[ctx->video_stream_index[i]]);
    vs = (rm_video_stream_t*)(stream->priv);
    vs->index_record = seek_indx(&(vs->indx), real_time,
                                 &position, &start_packet, &end_packet);
    stream->time = ((int64_t)(vs->indx.records[vs->index_record].timestamp) *
                    GAVL_TIME_SCALE) / 1000;
    vs->kf_pts = vs->indx.records[vs->index_record].timestamp;
    }
  for(i = 0; i < ctx->supported_audio_streams; i++)
    {
    stream = &(ctx->audio_streams[ctx->audio_stream_index[i]]);
    as = (rm_audio_stream_t*)(stream->priv);
    as->index_record = seek_indx(&(as->indx), real_time,
                                 &position, &start_packet, &end_packet);
    stream->time = ((int64_t)(as->indx.records[as->index_record].timestamp) *
                    GAVL_TIME_SCALE) / 1000;
    }

  //  fprintf(stderr, "Position: %u, Start Packet: %u, End Packet: %u\n",
  //          position, start_packet, end_packet);
    
  /* Seek to the position */
  
  bgav_input_seek(ctx->input, position, SEEK_SET);

  /* Skip unused packets */

  rm->do_seek = 1;
  rm->next_packet = start_packet;
  while(rm->next_packet < end_packet)
    next_packet_rmff(ctx);
  rm->do_seek = 0;
  }

void close_rmff(bgav_demuxer_context_t * ctx)
  {
  rm_audio_stream_t * as;
  rm_video_stream_t * vs;
  rm_private_t * priv;
  int i;
  priv = (rm_private_t *)ctx->priv;
  
  for(i = 0; i < ctx->num_audio_streams; i++)
    {
    as = (rm_audio_stream_t*)(ctx->audio_streams[i].priv);
    if(ctx->audio_streams[i].ext_data)
      free(ctx->audio_streams[i].ext_data);
    if(as->has_index)
      {
      //      fprintf(stderr, "Audio stream index\n");
      //      dump_indx(&(as->indx));
      free_indx(&(as->indx));
      }
    free(as);
    } 
  for(i = 0; i < ctx->num_video_streams; i++)
    {
    vs = (rm_video_stream_t*)(ctx->video_streams[i].priv);
    if(ctx->video_streams[i].ext_data)
      free(ctx->video_streams[i].ext_data);
    if(vs->has_index)
      {
      //      fprintf(stderr, "Video stream index\n");
      //      dump_indx(&(vs->indx));
      free_indx(&(vs->indx));
      }
    free(vs);
    }
  free(priv);
  }

bgav_demuxer_t bgav_demuxer_rmff =
  {
    probe:       probe_rmff,
    open:        open_rmff,
    next_packet: next_packet_rmff,
    seek:        seek_rmff,
    close:       close_rmff
  };
