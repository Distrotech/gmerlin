/*****************************************************************
 
  demux_mpegts.c
 
  Copyright (c) 2005-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/* System includes */
#include <avdec_private.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Package includes */

#include <pes_header.h>

#define MAX_PAT_SECTION_LENGTH 1021

/* Maximum number of programs in one pat section */
#define MAX_PROGRAMS ((MAX_PAT_SECTION_LENGTH-9)/4)

#define MAX_PMT_SECTION_LENGTH 1021

/* Maximum number of streams in one pmt section */
#define MAX_STREAMS ((MAX_PMT_SECTION_LENGTH-13)/5)

/* Stream types (from libavformat) */

#define STREAM_TYPE_VIDEO_MPEG1     0x01
#define STREAM_TYPE_VIDEO_MPEG2     0x02
#define STREAM_TYPE_AUDIO_MPEG1     0x03
#define STREAM_TYPE_AUDIO_MPEG2     0x04
#define STREAM_TYPE_PRIVATE_SECTION 0x05
#define STREAM_TYPE_PRIVATE_DATA    0x06
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_VIDEO_MPEG4     0x10
#define STREAM_TYPE_VIDEO_H264      0x1b

#define STREAM_TYPE_AUDIO_AC3       0x81
#define STREAM_TYPE_AUDIO_DTS       0x8a

typedef struct
  {
  int      ts_type;   /* From pmt */
  int      bgav_type; /* #defines from avdec_private.h */
  uint32_t fourcc;
  char * description; /* For debugging only! */
  } stream_type_t;

stream_type_t stream_types[] =
  {
    {
      ts_type:     STREAM_TYPE_VIDEO_MPEG1,
      bgav_type:   BGAV_STREAM_VIDEO,
      fourcc:      BGAV_MK_FOURCC('m', 'p', 'g', 'v'),
      description: "MPEG-1 Video",
    },
    {
      ts_type:     STREAM_TYPE_VIDEO_MPEG2,
      bgav_type:   BGAV_STREAM_VIDEO,
      fourcc:      BGAV_MK_FOURCC('m', 'p', 'g', 'v'),
      description: "MPEG-2 Video",
    },
    {
      ts_type:     STREAM_TYPE_AUDIO_MPEG1,
      bgav_type:   BGAV_STREAM_AUDIO,
      fourcc:      BGAV_MK_FOURCC('.', 'm', 'p', '3'),
      description: "MPEG-1 Audio",
    },
    {
      ts_type:     STREAM_TYPE_AUDIO_MPEG2,
      bgav_type:   BGAV_STREAM_AUDIO,
      fourcc:      BGAV_MK_FOURCC('.', 'm', 'p', '3'),
      description: "MPEG-1 Audio",
    },
    {
      ts_type:     STREAM_TYPE_AUDIO_AAC,
      bgav_type:   BGAV_STREAM_AUDIO,
      fourcc:      BGAV_MK_FOURCC('a', 'a', 'c', ' '),
      description: "MPEG-4 Audio (AAC)",
    },
    {
      ts_type:     STREAM_TYPE_VIDEO_MPEG4,
      bgav_type:   BGAV_STREAM_VIDEO,
      fourcc:      BGAV_MK_FOURCC('m', 'p', '4', 'v'),
      description: "MPEG-4 Video",
    },
    {
      ts_type:     STREAM_TYPE_VIDEO_H264,
      bgav_type:   BGAV_STREAM_VIDEO,
      fourcc:      BGAV_MK_FOURCC('H', '2', '6', '4'),
      description: "H264 Video (AVC)",
    },
    {
      ts_type:     STREAM_TYPE_AUDIO_AC3,
      bgav_type:   BGAV_STREAM_AUDIO,
      fourcc:      BGAV_MK_FOURCC('.', 'a', 'c', '3'),
      description: "AC3 Audio",
    },
    {
      ts_type:     STREAM_TYPE_AUDIO_DTS,
      bgav_type:   BGAV_STREAM_AUDIO,
      fourcc:      BGAV_MK_FOURCC('.', 'd', 't', 's'),
      description: "DTS Audio",
    },
  };

static stream_type_t * get_stream_type(int ts_type)
  {
  int i;

  for(i = 0; i < sizeof(stream_types)/sizeof(stream_types[0]); i++)
    {
    if(stream_types[i].ts_type == ts_type)
      return &(stream_types[i]);
    }
  return (stream_type_t*)0;
  }

/* Transport packet */

typedef struct
  {
  uint16_t pid;

  int has_adaption_field;
  int has_payload;

  int payload_start; /* Payload start indicator */
      
  uint8_t continuity_counter;

  int payload_size;

  /* Adaption field */
  
  int64_t pcr;
  
  } transport_packet_t;

#if 0
static void transport_packet_dump(transport_packet_t * p)
  {
  fprintf(stderr, "Transport packet:\n");
  fprintf(stderr, "  Payload start:      %d\n", p->payload_start);
  fprintf(stderr, "  PID:                0x%04x\n", p->pid);
  fprintf(stderr, "  Adaption field:     %s\n",
          (p->has_adaption_field ? "Yes" : "No"));
  fprintf(stderr, "  Payload:            %s\n",
          (p->has_payload ? "Yes" : "No"));
  fprintf(stderr, "  Continuity counter: %d\n", p->continuity_counter);
  fprintf(stderr, "  Payload size: %d\n", p->payload_size);
  if(p->pcr >= 0)
    fprintf(stderr, "  PCR: %f\n", (float)p->pcr / 90000.0);
  else
    fprintf(stderr, "  PCR: None\n");
  }

#endif

static int transport_packet_read(bgav_input_context_t * input,
                                 transport_packet_t * ret)
  {
  uint32_t header, tmp;
  uint8_t adaption_field_length, adaption_field_flags, c_tmp;

  ret->pcr = -1;
  
  if(!bgav_input_read_32_be(input, &header))
    return 0;

  if((header & 0xff000000) != 0x47000000)
    {
    fprintf(stderr, "transport_packet_read: Out of sync %08x\n", header);
    return 0;
    }
  ret->payload_start = !!(header & 0x00400000);
  
  ret->pid = (header & 0x001fff00) >> 8;

  tmp = (header & 0x00000030) >> 4;

  switch(tmp)
    {
    case 0x01:
      ret->has_adaption_field = 0;
      ret->has_payload        = 1;
      break;
    case 0x02:
      ret->has_adaption_field = 1;
      ret->has_payload        = 0;
      break;
    case 0x03:
      ret->has_adaption_field = 1;
      ret->has_payload        = 1;
      break;
    default:
      return 0;
    }
  ret->continuity_counter = header & 0x0000000f;

  if(ret->has_adaption_field)
    {
    if(!bgav_input_read_data(input, &adaption_field_length, 1))
      return 0;
    ret->payload_size = 184 - adaption_field_length - 1;

    if(adaption_field_length)
      {
      if(!bgav_input_read_data(input, &adaption_field_flags, 1))
        return 0;

      adaption_field_length--;
      //      fprintf(stderr, "Adaption field flags: 0x%02x\n", adaption_field_flags);
      if(adaption_field_flags & 0x10) /* PCR_flag */
        {
        if(!bgav_input_read_32_be(input, &tmp))
          return 0;
        ret->pcr = tmp;
        ret->pcr <<= 1;
        if(!bgav_input_read_data(input, &c_tmp, 1))
          return 0;
        ret->pcr |= (c_tmp >> 7);
        bgav_input_skip(input, 1);
        adaption_field_length -= 6;
        }
      }
    
    /* TODO: Something useful in the adaption field?? */
    bgav_input_skip(input, adaption_field_length);
    }
  else
    ret->payload_size = 184;
  
  return 1;
  }

/* Program map section */

typedef struct
  {
  uint8_t table_id;
  uint16_t section_length;
  uint16_t program_number;
  int current_next_indicator;
  uint8_t section_number;
  uint8_t last_section_number;

  uint16_t pcr_pid;
  char descriptor[4096];
  int descriptor_len;
  
  int num_streams;
  struct
    {
    uint8_t type;
    uint16_t pid;
    
    char descriptor[4096];
    int descriptor_len;
    } streams[MAX_STREAMS];
  } pmt_section_t;

static int pmt_section_read(uint8_t * data, int size,
                            pmt_section_t * ret)
  {
  int len;
  uint8_t * ptr, *ptr_start;

  memset(ret, 0, sizeof(*ret));

  ptr = data+1+data[0];

  ret->table_id = *ptr; ptr++;
  ret->section_length = BGAV_PTR_2_16BE(ptr) & 0x0fff; ptr+=2;

  ptr_start = ptr;
  
  if(ret->section_length > size - (ptr - data))
    return 0; /* Not enough data */

  ret->program_number = BGAV_PTR_2_16BE(ptr); ptr+=2;
  ret->current_next_indicator = (*ptr) & 0x01; ptr++;
  ret->section_number = *ptr; ptr++;
  ret->last_section_number = *ptr; ptr++;
  ret->pcr_pid = BGAV_PTR_2_16BE(ptr) & 0x1fff; ptr+=2;

  len = BGAV_PTR_2_16BE(ptr) & 0x0fff; ptr += 2;

  if(len)
    {
    memcpy(ret->descriptor, ptr, len);
    ret->descriptor[len] = '\0';
    ret->descriptor_len = len;
    ptr += len;
    }
  ret->num_streams = 0;

  /* 3 bytes before section_length, 4 bytes CRC */
  while((ptr - ptr_start) < ret->section_length - 4)
    {
    ret->streams[ret->num_streams].type = *ptr; ptr++;
    ret->streams[ret->num_streams].pid  =
        ret->pcr_pid = BGAV_PTR_2_16BE(ptr) & 0x1fff; ptr+=2;
    len = BGAV_PTR_2_16BE(ptr) & 0x0fff; ptr += 2;

    if(len)
      {
      memcpy(ret->streams[ret->num_streams].descriptor, ptr, len);
      ret->streams[ret->num_streams].descriptor[len] = '\0';
      ret->streams[ret->num_streams].descriptor_len = len;      
      ptr += len;
      }
    ret->num_streams++;
    }
  return 1;
  }

static void pmt_section_dump(pmt_section_t * pmts)
  {
  int i;
  stream_type_t * stream_type;
  
  fprintf(stderr, "PMT section:\n");
  fprintf(stderr, "  table_id:               %02x\n",   pmts->table_id);
  fprintf(stderr, "  section_length:         %d\n",     pmts->section_length);
  fprintf(stderr, "  program_number:         %d\n",     pmts->program_number);
  fprintf(stderr, "  current_next_indicator: %d\n",     pmts->current_next_indicator);
  fprintf(stderr, "  section_number:         %d\n",     pmts->section_number);
  fprintf(stderr, "  last_section_number:    %d\n",     pmts->last_section_number);
  fprintf(stderr, "  pcr_pid:                0x%04x\n", pmts->pcr_pid);
  fprintf(stderr, "  descriptor:             ");
  if(pmts->descriptor_len)
    bgav_hexdump((uint8_t*)pmts->descriptor, pmts->descriptor_len, pmts->descriptor_len);
  else
    fprintf(stderr, "[none]\n");
  fprintf(stderr, "  Number of streams:      %d\n",     pmts->num_streams);

  for(i = 0; i < pmts->num_streams; i++)
    {
    stream_type = get_stream_type(pmts->streams[i].type);
    
    fprintf(stderr, "  Stream %d\n", i+1);
    fprintf(stderr, "    type:       0x%02x (%s)\n",
            pmts->streams[i].type,
            (stream_type ? stream_type->description : "Unknown"));
    fprintf(stderr, "    PID:        0x%04x\n", pmts->streams[i].pid);
    fprintf(stderr, "    descriptor: ");
    
    if(pmts->streams[i].descriptor_len)
      bgav_hexdump((uint8_t*)pmts->streams[i].descriptor, pmts->streams[i].descriptor_len,
                   pmts->streams[i].descriptor_len);
    else
      fprintf(stderr, "[none]\n");

    }
  
  }

/* Program association table section */

typedef struct
  {
  uint8_t table_id;
  uint16_t section_length;
  uint16_t transport_stream_id;
  int current_next_indicator;
  uint8_t section_number;
  uint8_t last_section_number;

  int num_programs; /* Number of program definitions in this section */

  struct
    {
    uint16_t program_number;
    uint16_t program_map_pid;
    } programs[MAX_PROGRAMS];
  } pat_section_t;

static int pat_section_read(uint8_t * data, int size,
                            pat_section_t * ret)
  {
  int i;
  uint8_t * ptr;

  memset(ret, 0, sizeof(*ret));
  
  ptr = data+1+data[0];

  ret->table_id = *ptr; ptr++;
  
  ret->section_length = BGAV_PTR_2_16BE(ptr) & 0x0fff; ptr+=2;
  if(ret->section_length > size - (ptr - data))
    return 0; /* Not enough data */

  ret->transport_stream_id = BGAV_PTR_2_16BE(ptr);ptr+=2;
  ret->current_next_indicator = *ptr & 0x01; ptr++;
  ret->section_number = *ptr; ptr++;
  ret->last_section_number = *ptr; ptr++;

  ret->num_programs = (ret->section_length - 9) / 4;

  for(i = 0; i < ret->num_programs; i++)
    {
    ret->programs[i].program_number = BGAV_PTR_2_16BE(ptr); ptr+=2;
    ret->programs[i].program_map_pid = BGAV_PTR_2_16BE(ptr) & 0x1fff; ptr+=2;
    }
  return 1;
  }

static void pat_section_dump(pat_section_t * pats)
  {
  int i;
  fprintf(stderr, "PAT section:\n");
  fprintf(stderr, "  table_id:               %d\n", pats->table_id);
  fprintf(stderr, "  section_length:         %d\n", pats->section_length);
  fprintf(stderr, "  transport_stream_id:    %d\n", pats->transport_stream_id);
  fprintf(stderr, "  current_next_indicator: %d\n", pats->current_next_indicator);
  fprintf(stderr, "  section_number:         %d\n", pats->section_number);
  fprintf(stderr, "  last_section_number:    %d\n", pats->last_section_number);
  
  fprintf(stderr, "  Number of programs: %d\n", pats->num_programs);
  for(i = 0; i < pats->num_programs; i++)
    {
    fprintf(stderr, "    Program: %d ", pats->programs[i].program_number);
    fprintf(stderr, "Program map PID: 0x%04x\n", pats->programs[i].program_map_pid);
    }
  }

/* Demuxer functions */

typedef struct
  {
  uint16_t pid;

  bgav_pes_header_t pes_header;

  //  uint8_t * buffer;
  //  int buffer_alloc;
  //  int buffer_size;
  
  } stream_priv_t;

typedef struct
  {
  int initialized;

  int num_audio_streams;
  stream_priv_t * audio_streams;

  int num_video_streams;
  stream_priv_t * video_streams;

  uint16_t program_map_pid;
  
  int64_t start_pcr;
  int64_t end_pcr;

  uint16_t pcr_pid;

  } program_priv_t;

typedef struct
  {
  int num_programs;
  program_priv_t * programs;

  /* Input needed for pes header parsing */

  bgav_input_context_t * input_mem;

  /* Timing stuff */

  int64_t first_packet_pos;

  int current_program;
  } mpegts_t;

static int probe_mpegts(bgav_input_context_t * input)
  {
  uint8_t probe_data[189];

  /* We check, if we have the sync bytes (0x47) in 2
     transport packets */
  
  if(bgav_input_get_data(input, probe_data, 189) < 189)
    return 0;

  if((probe_data[0] == 0x47) && (probe_data[188] == 0x47))
    return 1;
  return 0;
  }

/* Get program durations */

static void get_program_durations(bgav_demuxer_context_t * ctx)
  {
  int64_t total_packets, position;

  int i, j;
  uint8_t data[188];
    
  int keep_going;
  
  mpegts_t * priv;
  transport_packet_t tp;

  priv = (mpegts_t*)(ctx->priv);

  bgav_input_seek(ctx->input, priv->first_packet_pos,
                  SEEK_SET);

  /* Get the start timestamps of all programs */

  keep_going = 1;

  fprintf(stderr, "Getting start timestamps...\n");
  
  while(keep_going)
    {

    if(bgav_input_read_data(ctx->input, data, 188) < 188)
      {
      fprintf(stderr, "Premature EOF\n");
      break;
      }
    bgav_input_reopen_memory(priv->input_mem, data, 188);
    
    if(!transport_packet_read(priv->input_mem, &tp))
      {
      fprintf(stderr, "Reading transport packet failed\n");
      break;
      }

    //    transport_packet_dump(&tp);
      
    if(tp.pcr < 0)
      continue;
    
    /* Check what this packet is for */
    
    for(i = 0; i < priv->num_programs; i++)
      {
      if(tp.pid == priv->programs[i].pcr_pid)
        {
        if(priv->programs[i].start_pcr < 0)
          priv->programs[i].start_pcr = tp.pcr;
        
        /* Check if we are done */
        keep_going = 0;
        for(j = 0; j < priv->num_programs; j++)
          {
          if(priv->programs[j].start_pcr < 0)
            {
            keep_going = 1;
            break;
            }
          }
        break;
        }
      }
    }

  fprintf(stderr, "Getting start timestamps done\n");
  
  /* Now, get the end timestamps */

  total_packets = (ctx->input->total_bytes - priv->first_packet_pos) / 188;

  position = priv->first_packet_pos + (total_packets - 1) * 188;

  keep_going = 1;

  fprintf(stderr, "Getting end timestamps...\n");

  while(keep_going)
    {
    bgav_input_seek(ctx->input, position, SEEK_SET);

    if(bgav_input_read_data(ctx->input, data, 188) < 188)
      break;
    
    bgav_input_reopen_memory(priv->input_mem, data, 188);
    
    if(!transport_packet_read(priv->input_mem, &tp))
      {
      fprintf(stderr, "Reading transport packet failed\n");
      break;
      }
    
    if(tp.pcr < 0)
      {
      position -= 188;

      if(position < 0)
        break;
      else
        continue;
      }
    
    for(i = 0; i < priv->num_programs; i++)
      {
      if(tp.pid == priv->programs[i].pcr_pid)
        {
        if(priv->programs[i].end_pcr < 0)
          priv->programs[i].end_pcr = tp.pcr;
        
        /* Checkl if we are done */
        keep_going = 0;
        for(j = 0; j < priv->num_programs; j++)
          {
          if(priv->programs[j].end_pcr < 0)
            {
            keep_going = 1;
            break;
            }
          }
        break;
        }
      }
        
    position -= 188;
    if(position < 0)
      break;
    }

  fprintf(stderr, "Getting end timestamps done\n");
    
  /* Dump this */

  for(i = 0; i < priv->num_programs; i++)
    {
    fprintf(stderr, "Program %d: [%.2f .. %.2f], %.2f\n",
            i+1,
            (float)(priv->programs[i].start_pcr)/90000.0,
            (float)(priv->programs[i].end_pcr)/90000.0,
            (float)(priv->programs[i].end_pcr -
                    priv->programs[i].start_pcr)/90000.0);
    }

  /* Get the start timestamps of all streams and the durations */

  /* Seek to the beginning */
    
  bgav_input_seek(ctx->input, priv->first_packet_pos,
                  SEEK_SET);
  
  }

#define REALLOCZ(ptr, offset) \
  ptr = realloc(ptr, (offset+1)*sizeof(*(ptr))); \
  memset(&(ptr[offset]), 0, sizeof(*(ptr)))

static int open_mpegts(bgav_demuxer_context_t * ctx,
                       bgav_redirector_context_t ** redir)
  {
  int i, j, program;
  uint8_t data[184];
  transport_packet_t packet;
  pat_section_t pats;
  pmt_section_t pmts;
  int keep_going;
  mpegts_t * priv;
  stream_type_t * stream_type;
  bgav_stream_t * bgav_stream;
  stream_priv_t * stream_priv;
    
  /* Allocate private data */

  priv = calloc(1, sizeof(*priv));
  
  priv->input_mem = bgav_input_open_memory((uint8_t*)0, 0);
  
  ctx->priv = priv;

  priv->first_packet_pos = ctx->input->position;
  
  /* Skip everything until we get the first PAT */
  i = 0;
  while(1)
    {
    if(!transport_packet_read(ctx->input, &packet))
      {
      fprintf(stderr, "Read transport packet failed\n");
      return 0;
      }
    //    fprintf(stderr, "Got transport packet\n");
    if(packet.pid == 0x0000)
      break;
    bgav_input_skip(ctx->input, packet.payload_size);
    i++;
    }
  fprintf(stderr, "Got PAT after skipping %d packets\n", i);

  //  transport_packet_dump(&packet);
  bgav_input_read_data(ctx->input, data, packet.payload_size);
  //  bgav_hexdump(data, packet.payload_size, 16);

  if(!pat_section_read(data, packet.payload_size,
                       &pats))
    {
    fprintf(stderr, "PAT section spans multiple packets, please report\n");
    }
  pat_section_dump(&pats);

  if(pats.section_number || pats.last_section_number)
    {
    fprintf(stderr, "PAT has multiple sections, please report\n");
    return 0;
    }

  
  /* Count the programs */

  for(i = 0; i < pats.num_programs; i++)
    {
    if(pats.programs[i].program_number != 0x0000)
      priv->num_programs++;
    }

  /* Allocate programs and track table */
  
  priv->programs = calloc(priv->num_programs, sizeof(*(priv->programs)));
  ctx->tt = bgav_track_table_create(priv->num_programs);

  /* Assign program map pids */

  j = 0;

  for(i = 0; i < pats.num_programs; i++)
    {
    if(pats.programs[i].program_number != 0x0000)
      {
      priv->programs[j].program_map_pid = pats.programs[i].program_map_pid;
      j++;
      }
    }
  
  /* Next, we want to get all programs */

  keep_going = 1;

  while(keep_going)
    {
    if(!transport_packet_read(ctx->input, &packet))
      {
      fprintf(stderr, "Premature EOF\n");
      return 0;
      }
    bgav_input_read_data(ctx->input, data, packet.payload_size);
    //    bgav_hexdump(data, packet.payload_size, 16);

    for(program = 0; program < priv->num_programs; program++)
      {
      if(packet.pid == priv->programs[program].program_map_pid)
        break;
      }

    if(program == priv->num_programs)
      {
      //      fprintf(stderr, "Skipping packet with PID 0x%04x (%d %d)\n", packet.pid,
      //              program, priv->num_programs);
      continue;
      }
    
    if(!pmt_section_read(data, packet.payload_size, &pmts))
      {
      fprintf(stderr, "PMT section spans multiple packets, please report\n");
      return 0;
      }
    if(pmts.section_number || pmts.last_section_number)
      {
      fprintf(stderr, "PMT has multiple sections, please report\n");
      return 0;
      }
    
    pmt_section_dump(&pmts);

    /* Setup the streams */
    for(i = 0; i < pmts.num_streams; i++)
      {
      stream_type = get_stream_type(pmts.streams[i].type);
      if(!stream_type || (stream_type->bgav_type == BGAV_STREAM_UNKNOWN))
        {
        fprintf(stderr, "Ignoring stream of type %02x\n", pmts.streams[i].type);
        continue;
        }

      /* Add audio stream */
      else if(stream_type->bgav_type == BGAV_STREAM_AUDIO)
        {
        REALLOCZ(priv->programs[program].audio_streams,
                 priv->programs[program].num_audio_streams);

        bgav_stream = bgav_track_add_audio_stream(&(ctx->tt->tracks[program]), ctx->opt);
        bgav_stream->timescale = 90000;
        bgav_stream->fourcc = stream_type->fourcc;

        /*
         *  We save the pid twice, once in the generic stream structure,
         *  once in our private stream structure. It's bad but makes things
         *  easier afterwards
         */
        
        bgav_stream->stream_id = pmts.streams[i].pid;

        stream_priv =
          &(priv->programs[program].audio_streams[priv->programs[program].num_audio_streams]);
        stream_priv->pid = pmts.streams[i].pid;
        
        priv->programs[program].num_audio_streams++;

        // fprintf(stderr, "Got audio stream %d\n", priv->programs[program].num_audio_streams);
        // bgav_stream_dump(bgav_stream);
        }

      /* Add video stream */
      else if(stream_type->bgav_type == BGAV_STREAM_VIDEO)
        {
        REALLOCZ(priv->programs[program].video_streams,
                 priv->programs[program].num_video_streams);

        bgav_stream = bgav_track_add_video_stream(&(ctx->tt->tracks[program]), ctx->opt);

        bgav_stream->timescale = 90000;
        bgav_stream->fourcc = stream_type->fourcc;

        bgav_stream->stream_id = pmts.streams[i].pid;

        stream_priv =
          &(priv->programs[program].video_streams[priv->programs[program].num_video_streams]);
        stream_priv->pid = pmts.streams[i].pid;
        

        priv->programs[program].num_video_streams++;
        
        }

      
      }

    /* Assign private data for streams */

    for(i = 0; i < priv->programs[program].num_audio_streams; i++)
      {
      ctx->tt->tracks[program].audio_streams[i].priv =
        &(priv->programs[program].audio_streams[i]);
      }

    for(i = 0; i < priv->programs[program].num_video_streams; i++)
      {
      ctx->tt->tracks[program].video_streams[i].priv =
        &(priv->programs[program].video_streams[i]);
      }

    /* Assign program wide data */

    priv->programs[program].pcr_pid = pmts.pcr_pid;
    priv->programs[program].start_pcr = -1;
    priv->programs[program].end_pcr = -1;
    
    priv->programs[program].initialized = 1;
    //    priv->programs[program].start_pts = -1;
    //    priv->programs[program].end_pts = -1;
    
    /* Check if we are done */
    keep_going = 0;
    for(i = 0; i < priv->num_programs; i++)
      {
      if(!priv->programs[i].initialized)
        {
        keep_going = 1;
        break;
        }
      }
    }

  if(ctx->input->input->seek_byte && ctx->input->total_bytes)
    {
    get_program_durations(ctx);
    }
  
  //  fprintf(stderr, "Got transport packet:\n");
  //  transport_packet_dump(&packet);
  return 1;
  }

#undef REALLOCZ

#define NUM_PACKETS 1000 /* Packets to be processed at once */

static int next_packet_mpegts(bgav_demuxer_context_t * ctx)
  {
  uint8_t data[188];
  int i;
  transport_packet_t tp;
  bgav_stream_t * s;
  mpegts_t * priv;
  stream_priv_t * stream_priv;
    
  priv = (mpegts_t*)(ctx->priv);

  // fprintf(stderr, "Track: %d\n", (int)(ctx->tt->current_track - ctx->tt->tracks));
    
  for(i = 0; i < NUM_PACKETS; i++)
    {
    //    fprintf(stderr, "Data: %llx ", ctx->input->position);

    if(bgav_input_read_data(ctx->input, data, 188) < 188)
      {
      fprintf(stderr, "EOF\n");

      return !!i;
      }
    
    //    bgav_hexdump(data, 4, 4);
    
    bgav_input_reopen_memory(priv->input_mem, data, 188);

    //    bgav_hexdump(data, 188, 16);
    
    if(!transport_packet_read(priv->input_mem, &tp))
      {
      fprintf(stderr, "Reading transport packet failed\n");
      return !!i;
      }
    s = bgav_track_find_stream(ctx->tt->current_track, tp.pid);    
    
    if(!s)
      continue;
    
    stream_priv = (stream_priv_t*)(s->priv);
      
    if(tp.payload_start) /* New packet starts here */
      {
      if(s->packet)
        {
        //        fprintf(stderr, "Finished reading packet, size: %d\n", s->packet->data_size);

        bgav_packet_done_write(s->packet);
        s->packet = (bgav_packet_t*)0;
        }

      //      fprintf(stderr, "Got stream\n");
      //      bgav_stream_dump(s);
      
      s->packet = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);

      /* Read PES header */

      
      if(!bgav_pes_header_read(priv->input_mem, &(stream_priv->pes_header)))
        return !!i;
      
      /*
       *  Now the bad news: Some transport streams contain PES packets with a
       *  packet length field of zero. This means, we must use the
       *  payload_start bit to find out when a packet ended.
       *
       *  Here, we allocate 1024 bytes (>> transport payload) to reduce realloc
       *  overhead afterwards
       */
      
      //      fprintf(stderr, "Got PES header\n");
      //      bgav_pes_header_dump(&(stream_priv->pes_header));
      
      bgav_packet_alloc(s->packet, 1024);
#if 0
      if(stream_priv->pes_header.pts >= 0)
        {
        if(priv->programs[priv->current_program].start_pts < 0)
          priv->programs[priv->current_program].start_pts = stream_priv->pes_header.pts;

        s->packet->timestamp_scaled =
          stream_priv->pes_header.pts -
          priv->programs[priv->current_program].start_pts;

        if(s->packet->timestamp_scaled < 0)
          s->packet->timestamp_scaled = 0;
        }
#endif
      /* Read data */

      s->packet->data_size =
        bgav_input_read_data(priv->input_mem,
                             s->packet->data,
                             priv->input_mem->total_bytes -                      
                             priv->input_mem->position);
      
      }
    else if(s->packet)
      {
      /* Read data */

      bgav_packet_alloc(s->packet,
                        s->packet->data_size + tp.payload_size);

      s->packet->data_size += 
        bgav_input_read_data(priv->input_mem,
                             s->packet->data + s->packet->data_size,
                             tp.payload_size);
      
      }
    }

  
  return 1;
  }

static void seek_mpegts(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  
  }

static void close_mpegts(bgav_demuxer_context_t * ctx)
  {
  int i;

  mpegts_t * priv;
  priv = (mpegts_t*)(ctx->priv);

  for(i = 0; i < priv->num_programs; i++)
    {
    if(priv->programs[i].audio_streams)
      free(priv->programs[i].audio_streams);
    if(priv->programs[i].video_streams)
      free(priv->programs[i].video_streams);
    }
  if(priv->programs)
    free(priv->programs);
  free(priv);
  }

static void select_track_mpegts(bgav_demuxer_context_t * ctx,
                                int track)
  {
  mpegts_t * priv;
  priv = (mpegts_t*)(ctx->priv);
  priv->current_program = track;
  }

bgav_demuxer_t bgav_demuxer_mpegts =
  {
    probe:        probe_mpegts,
    open:         open_mpegts,
    next_packet:  next_packet_mpegts,
    seek:         seek_mpegts,
    close:        close_mpegts,
    select_track: select_track_mpegts
  };

