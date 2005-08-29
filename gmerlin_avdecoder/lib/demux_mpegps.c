/*****************************************************************
 
  demux_mpegps.c
 
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

#include <avdec_private.h>
#include <stdlib.h>
#include <stdio.h>

#include <pes_header.h>

#define SYSTEM_HEADER 0x000001bb
#define PACK_HEADER   0x000001ba
#define PROGRAM_END   0x000001b9

#define CDXA_SECTOR_SIZE_RAW 2352
#define CDXA_SECTOR_SIZE     2324
#define CDXA_HEADER_SIZE       24

/* Synchronization routines */

#define IS_START_CODE(h)  ((h&0xffffff00)==0x00000100)

//#define NDEBUG

/* We scan at most one megabyte */

#define SYNC_SIZE (1024*1024)

static uint32_t next_start_code(bgav_input_context_t * ctx)
  {
  int bytes_skipped = 0;
  uint32_t c;
  while(1)
    {
    if(!bgav_input_get_32_be(ctx, &c))
      return 0;
    if(IS_START_CODE(c))
      {
      return c;
      }
    bgav_input_skip(ctx, 1);
    bytes_skipped++;
    if(bytes_skipped > SYNC_SIZE)
      return 0;
    }
  return 0;
  }

static uint32_t previous_start_code(bgav_input_context_t * ctx)
  {
  uint32_t c;
  //  int skipped = 0;
  bgav_input_seek(ctx, -1, SEEK_CUR);

  while(ctx->position >= 0)
    {
//    fprintf(stderr, "previous_start_code, pos: %lld\n", 
//            ctx->position);
    if(!bgav_input_get_32_be(ctx, &c))
      {
#if 0
      fprintf(stderr, "previous_start_code: EOF %lld %lld\n",
              ctx->position, ctx->total_bytes);
#endif
      return 0;
      }
    if(IS_START_CODE(c))
      {
#if 0
      fprintf(stderr, "previous_start_code: skipped %d bytes\n", skipped);
#endif
      return c;
      }
    bgav_input_seek(ctx, -1, SEEK_CUR);
    //    skipped++;
    }
  return 0;
  }

/* Probe */

static int probe_mpegps(bgav_input_context_t * input)
  {
  uint8_t probe_data[12];
  if(bgav_input_get_data(input, probe_data, 12) < 12)
    return 0;
#if 0
  fprintf(stderr, "Probe mpegps: ");
  bgav_hexdump(probe_data, 12, 12);
#endif
  
  /* Check for pack header */

  if((probe_data[0] == 0x00) &&
     (probe_data[1] == 0x00) &&
     (probe_data[2] == 0x01) &&
     (probe_data[3] == 0xba))
    return 1;

  /* Check for CDXA header */
    
  if((probe_data[0] == 'R') &&
     (probe_data[1] == 'I') &&
     (probe_data[2] == 'F') &&
     (probe_data[3] == 'F') &&
     (probe_data[8] == 'C') &&
     (probe_data[9] == 'D') &&
     (probe_data[10] == 'X') &&
     (probe_data[11] == 'A'))
    return 1;
  return 0;
  }

/* Pack header */

typedef struct
  {
  int64_t scr;
  int mux_rate;
  int version;
  } pack_header_t;

static int pack_header_read(bgav_input_context_t * input,
                            pack_header_t * ret)
  {
  uint8_t c;
  uint16_t tmp_16;
  uint32_t tmp_32;

  int stuffing;
  
  bgav_input_skip(input, 4);
    
  if(!bgav_input_read_8(input, &c))
    return 0;

  if((c & 0xf0) == 0x20) /* MPEG-1 */
    {
    //    fprintf(stderr, "MPEG-1");
    //    bgav_input_read_8(input, &c);
    
    ret->scr = ((c >> 1) & 7) << 30;
    bgav_input_read_16_be(input, &tmp_16);
    ret->scr |= ((tmp_16 >> 1) << 15);
    bgav_input_read_16_be(input, &tmp_16);
    ret->scr |= (tmp_16 >> 1);
    
    bgav_input_read_8(input, &c);
    ret->mux_rate = (c & 0x7F) << 15;
                                                                               
    bgav_input_read_8(input, &c);
    ret->mux_rate |= ((c & 0x7F) << 7);
                                                                               
    bgav_input_read_8(input, &c);
    ret->mux_rate |= (((c & 0xFE)) >> 1);
    ret->version = 1;
    //    fprintf(stderr, "SCR: %f\n", demuxer->pack_header.scr / 90000.0);
    }
  else if(c & 0x40) /* MPEG-2 */
    {
    /* SCR */
    //    fprintf(stderr, "MPEG-2");
    if(!bgav_input_read_32_be(input, &tmp_32))
      return 0;
    
    ret->scr = c & 0x03;

    ret->scr <<= 13;
    ret->scr |= ((tmp_32 & 0xfff80000) >> 19);
    
    ret->scr <<= 15;
    ret->scr |= ((tmp_32 & 0x0003fff8) >> 3);

    /* Skip SCR extension (would give 27 MHz resolution) */

    bgav_input_skip(input, 1);
        
    /* Mux rate (22 bits) */
    
    if(!bgav_input_read_8(input, &c))
      return 0;
    ret->mux_rate = c;

    ret->mux_rate <<= 8;
    if(!bgav_input_read_8(input, &c))
      return 0;
    ret->mux_rate |= c;

    ret->mux_rate <<= 6;
    if(!bgav_input_read_8(input, &c))
      return 0;
    ret->mux_rate |= (c>>2);

    ret->version = 2;
    
    /* Now, some stuffing bytes might come.
       They are set to 0xff and will be skipped by the
       next_start_code function */
    bgav_input_read_8(input, &c);
    stuffing = c & 0x03;
    bgav_input_skip(input, stuffing);

    }
  //  else
  //    {
  //    fprintf(stderr, "MPEG-X %02x\n", c);
  //    }
  return 1;
  }
#if 0
static void pack_header_dump(pack_header_t * h)
  {
  fprintf(stderr,
          "Pack header: MPEG-%d, SCR: %lld (%f secs), Mux rate: %d bits/s\n",
          h->version, h->scr, (float)(h->scr)/90000.0,
          h->mux_rate * 400);
  }
#endif
/* System header */

typedef struct
  {
  uint16_t size;
  } system_header_t;

static int system_header_read(bgav_input_context_t * input,
                              system_header_t * ret)
  {
  bgav_input_skip(input, 4); /* Skip start code */  
  if(!bgav_input_read_16_be(input, &(ret->size)))
    return 0;
  bgav_input_skip(input, ret->size);
  return 1;
  }
#if 0
static void system_header_dump(system_header_t * h)
  {
  fprintf(stderr, "System header (skipped %d bytes)\n", h->size);
  }
#endif
/* Demuxer structure */

typedef struct
  {
  /* For sector based access */
  bgav_input_context_t * input_mem;

  int sector_size;
  int sector_size_raw;
  int sector_header_size;
  int64_t total_sectors;
  int64_t sector_position;
  int start_sector; /* First nonempty sector */

  uint8_t * sector_buffer;
    
  int is_cdxa; /* Nanosoft VCD rip */
    
  int64_t data_start;
  int64_t data_size;
  
  /* Actions for next_packet */
  
  int find_streams;
  int do_sync;
  
  /* Headers */

  pack_header_t     pack_header;
  bgav_pes_header_t pes_header;

  /* Timing stuff */

  int64_t start_pts;

  /* Sector based access functions */

  void (*goto_sector)(bgav_demuxer_context_t * ctx, int64_t sector);
  int (*read_sector)(bgav_demuxer_context_t * ctx);
  
  } mpegps_priv_t;

static const int lpcm_freq_tab[4] = { 48000, 96000, 44100, 32000 };

/* Sector based utilities */

static void goto_sector_cdxa(bgav_demuxer_context_t * ctx, int64_t sector)
  {
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);

  //  fprintf(stderr, "Start: %lld, Pos: %lld\n", priv->data_start,
  //          priv->data_start + priv->sector_size_raw * (sector + priv->start_sector));

  bgav_input_seek(ctx->input, priv->data_start + priv->sector_size_raw * (sector + priv->start_sector),
                  SEEK_SET);
  bgav_input_reopen_memory(priv->input_mem,
                           NULL, 0);
  
  }

static int read_sector_cdxa(bgav_demuxer_context_t * ctx)
  {
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);
  if(bgav_input_read_data(ctx->input, priv->sector_buffer, priv->sector_size_raw) <
     priv->sector_size_raw)
    return 0;
  //  fprintf(stderr, "Sector ");
  //  bgav_hexdump(priv->sector_buffer + priv->sector_header_size, 16, 16);
  
  bgav_input_reopen_memory(priv->input_mem,
                           priv->sector_buffer + priv->sector_header_size,
                           priv->sector_size);
  return 1;
  }

static void goto_sector_input(bgav_demuxer_context_t * ctx, int64_t sector)
  {
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);
  //  fprintf(stderr, "Seek sector %lld\n", sector + priv->start_sector);
  bgav_input_seek_sector(ctx->input, sector + priv->start_sector);
  bgav_input_reopen_memory(priv->input_mem,
                           NULL, 0);

  }

static int read_sector_input(bgav_demuxer_context_t * ctx)
  {
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);
  if(!bgav_input_read_sector(ctx->input, priv->sector_buffer))
    return 0;
  bgav_input_reopen_memory(priv->input_mem,
                           priv->sector_buffer + priv->sector_header_size,
                           priv->sector_size);
  return 1;
  }

/* Generic initialization function for sector based access: Get the 
   empty sectors at the beginning and the end, and the duration of the track */

static void init_sector_mode(bgav_demuxer_context_t * ctx)
  {
  int64_t scr_start, scr_end;
  uint32_t start_code = 0;
  
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);

  priv->input_mem          = bgav_input_open_memory(NULL, 0);

  //  fprintf(stderr, "init_sector_mode 1: %lld\n",
  //          priv->total_sectors);
    
  priv->goto_sector(ctx, 0);
  while(1)
    {
    if(!priv->read_sector(ctx))
      return;
    if(!bgav_input_get_32_be(priv->input_mem, &start_code))
      return;
    if(start_code == PACK_HEADER)
      break;
#if 0
    else
      {
      fprintf(stderr, "Start code: ");
      bgav_dump_fourcc( start_code);
      fprintf(stderr, "\n");
      }
#endif
    priv->start_sector++;
    priv->total_sectors--;
    }
  
  /* Read first scr */
  
  if(!pack_header_read(priv->input_mem, &(priv->pack_header)))
    {
#if 1
    fprintf(stderr, "pack_header_read failed %lld %lld\n",
            ctx->input->position, ctx->input->total_bytes);
#endif
    return;
    }
  //  pack_header_dump(&(priv->pack_header));
  
  scr_start = priv->pack_header.scr;

  //  fprintf(stderr, "scr_start: %lld\n", scr_start);

  /* If we already have the duration, stop here. */

  if(ctx->tt->current_track->duration != GAVL_TIME_UNDEFINED)
    {
    priv->goto_sector(ctx, 0);
    bgav_input_reopen_memory(priv->input_mem, (uint8_t*)0, 0);
    return;
    }
    
  while(1)
    {
    priv->goto_sector(ctx, priv->total_sectors - 1);
    if(!priv->read_sector(ctx))
      {
      fprintf(stderr, "Read sector failed\n");
      priv->total_sectors--;
      continue;
      }
    if(!bgav_input_get_32_be(priv->input_mem, &start_code))
      return;
    if(start_code == PACK_HEADER)
      break;
#if 0
    else
      {
      fprintf(stderr, "Start code: ");
      bgav_dump_fourcc( start_code);
      fprintf(stderr, "\n");
      }
#endif
    priv->total_sectors--;
    }

  if(!pack_header_read(priv->input_mem, &(priv->pack_header)))
    {
#if 1
    fprintf(stderr, "pack_header_read failed %lld %lld\n",
            ctx->input->position, ctx->input->total_bytes);
#endif
    return;
    }
  //  pack_header_dump(&(priv->pack_header));
  scr_end = priv->pack_header.scr;

  fprintf(stderr, "scr_start: %lld, scr_end: %lld\n", scr_start, scr_end);

//  fprintf(stderr, "init_sector_mode 2: %lld\n",
//          priv->total_sectors);
  
  ctx->tt->current_track->duration =
    ((int64_t)(scr_end - scr_start) * GAVL_TIME_SCALE) / 90000;

  priv->goto_sector(ctx, 0);

  bgav_input_reopen_memory(priv->input_mem, (uint8_t*)0, 0);
  }

/* Get one packet */

static int next_packet(bgav_demuxer_context_t * ctx, bgav_input_context_t * input)
  {
  uint8_t c;
  system_header_t system_header;
  int got_packet = 0;
  uint32_t start_code;

  bgav_packet_t * p;
  
  mpegps_priv_t * priv;
  bgav_stream_t * stream = (bgav_stream_t*)0;

  priv = (mpegps_priv_t*)(ctx->priv);
  
  while(!got_packet)
    {
    if(!(start_code = next_start_code(input)))
      return 0;
    if(start_code == SYSTEM_HEADER)
      {
      if(!system_header_read(input, &system_header))
        return 0;
      //      system_header_dump(&system_header);
      }
    else if(start_code == PACK_HEADER)
      {
      if(!pack_header_read(input, &(priv->pack_header)))
        return 0;
      //      pack_header_dump(&(priv->pack_header));
      }

    else /* PES Packet */
      {
      if(!bgav_pes_header_read(input, &(priv->pes_header)))
        return 0;
      
      //      bgav_pes_header_dump(&(priv->pes_header));
      
      /* Private stream 1 (non MPEG audio, subpictures) */
      if(priv->pes_header.stream_id == 0xbd)
        {
        if(!bgav_input_read_8(input, &c))
          return 0;
        priv->pes_header.payload_size--;

        priv->pes_header.stream_id <<= 8;
        priv->pes_header.stream_id |= c;

        if((c >= 0x20) && (c <= 0x3f))  /* Subpicture */
          {
          
          }
        else if((c >= 0x80) && (c <= 0x87)) /* AC3 Audio */
          {
          bgav_input_skip(input, 3);
          priv->pes_header.payload_size -= 3;
          
          if(priv->find_streams)
            stream = bgav_track_find_stream_all(ctx->tt->current_track,
                                                priv->pes_header.stream_id);
          else
            stream = bgav_track_find_stream(ctx->tt->current_track,
                                            priv->pes_header.stream_id);
          if(!stream && priv->find_streams)
            {
            stream = bgav_track_add_audio_stream(ctx->tt->current_track);
            stream->timescale = 90000;
            stream->fourcc = BGAV_MK_FOURCC('.', 'a', 'c', '3');
            stream->stream_id = priv->pes_header.stream_id;
            //            fprintf(stderr, "Found AC3 audio stream\n");
            }
          }
        else if((c >= 0xA0) && (c <= 0xA7)) /* LPCM Audio */
          {
          //          fprintf(stderr, "LPCM Audio!\n");
          bgav_input_skip(input, 3);
          priv->pes_header.payload_size -= 3;

          if(priv->find_streams)
            stream = bgav_track_find_stream_all(ctx->tt->current_track,
                                                priv->pes_header.stream_id);
          else
            stream = bgav_track_find_stream(ctx->tt->current_track,
                                            priv->pes_header.stream_id);
          if(!stream && priv->find_streams)
            {
            stream = bgav_track_add_audio_stream(ctx->tt->current_track);
            stream->timescale = 90000;
            stream->fourcc = BGAV_MK_FOURCC('l', 'p', 'c', 'm');
            stream->stream_id = priv->pes_header.stream_id;
            //            fprintf(stderr, "Found LPCM stream\n");
            }

          /* Set stream format */

          if(stream && !stream->data.audio.format.samplerate)
            {
            /* emphasis (1), muse(1), reserved(1), frame number(5) */
            bgav_input_skip(input, 1);
            /* quant (2), freq(2), reserved(1), channels(3) */
            if(!bgav_input_read_data(input, &c, 1))
              return 0;
                        
            stream->data.audio.format.samplerate = lpcm_freq_tab[(c >> 4) & 0x03];
            stream->data.audio.format.num_channels = 1 + (c & 7);
            stream->data.audio.bits_per_sample = 16;
            
            switch ((c>>6) & 3)
              {
              case 0: stream->data.audio.bits_per_sample = 16; break;
              case 1: stream->data.audio.bits_per_sample = 20; break;
              case 2: stream->data.audio.bits_per_sample = 24; break;
              }

            /* Dynamic range control */
            bgav_input_skip(input, 1);
            
            priv->pes_header.payload_size -= 3;
            }
          else /* lpcm header (3 bytes) */
            {
            bgav_input_skip(input, 3);
            priv->pes_header.payload_size -= 3;
            }
          }
        }
      /* Audio stream */
      else if((priv->pes_header.stream_id & 0xE0) == 0xC0)
        {
        if(priv->find_streams)
          stream = bgav_track_find_stream_all(ctx->tt->current_track,
                                              priv->pes_header.stream_id);
        else
          stream = bgav_track_find_stream(ctx->tt->current_track,
                                          priv->pes_header.stream_id);
        if(!stream && priv->find_streams)
          {
          stream = bgav_track_add_audio_stream(ctx->tt->current_track);
          stream->fourcc = BGAV_MK_FOURCC('.', 'm', 'p', '3');
          stream->timescale = 90000;
          stream->stream_id = priv->pes_header.stream_id;
          //          fprintf(stderr, "Found audio stream\n");
          }
        }
      /* Video stream */
      else if((priv->pes_header.stream_id & 0xF0) == 0xE0)
        {
        if(priv->find_streams)
          stream = bgav_track_find_stream_all(ctx->tt->current_track,
                                              priv->pes_header.stream_id);
        else
          stream = bgav_track_find_stream(ctx->tt->current_track,
                                          priv->pes_header.stream_id);
        if(!stream && priv->find_streams)
          {
          stream = bgav_track_add_video_stream(ctx->tt->current_track);
          stream->fourcc = BGAV_MK_FOURCC('m', 'p', 'g', 'v');
          stream->stream_id = priv->pes_header.stream_id;
          stream->timescale = 90000;
          //          fprintf(stderr, "Found video stream\n");
          }
        }

      if(stream)
        {
        if((priv->do_sync) &&
           (stream->time_scaled < 0) &&
           (priv->pes_header.pts < 0))
          {
          bgav_input_skip(input, priv->pes_header.payload_size);
          return 1;
          }
        p = bgav_packet_buffer_get_packet_write(stream->packet_buffer, stream);
        
        bgav_packet_alloc(p, priv->pes_header.payload_size);
        if(bgav_input_read_data(input, p->data, 
                                 priv->pes_header.payload_size) <
           priv->pes_header.payload_size)
          {
          bgav_packet_done_write(p);
          return 0;
          }
        p->data_size = priv->pes_header.payload_size;
        
        if(priv->pes_header.pts >= 0)
          {
          if(priv->start_pts < 0)
            {
            if(ctx->tt->current_track->num_audio_streams &&
               ctx->tt->current_track->num_video_streams)
              {
              if(stream->type == BGAV_STREAM_AUDIO)
                {
                priv->start_pts = priv->pes_header.pts;
#if 0
                fprintf(stderr, "Start PTS: %f\n",
                        priv->start_pts / 90000.0);
#endif
                }
              }
            else
              {
              priv->start_pts = priv->pes_header.pts;
#if 0
              fprintf(stderr, "Start PTS 1: %f %d %d\n",
                      priv->start_pts / 90000.0,
                      ctx->tt->current_track->num_audio_streams,
                      ctx->tt->current_track->num_video_streams);
#endif
              }
            }
          
          if((priv->pes_header.pts > priv->start_pts) && (priv->start_pts >= 0))
            {
            p->timestamp_scaled = (priv->pes_header.pts - priv->start_pts);
            }
          else
            {
            p->timestamp_scaled = 0;
            }
          

          if(priv->do_sync && (stream->time_scaled < 0))
            stream->time_scaled = p->timestamp_scaled;
          
          //          fprintf(stderr, "Stream: %04x, pts: %lld\n",
          //                  stream->stream_id, priv->pes_header.pts - priv->start_pts);
          }
        
        bgav_packet_done_write(p);
        got_packet = 1;
        }
      else
        {
        //        fprintf(stderr, "Skipping %d bytes of stream %02x\n",
        //                priv->pes_header.payload_size,
        //                priv->pes_header.stream_id);
        bgav_input_skip(input, priv->pes_header.payload_size);
        }
      
      }
    }
  
  return 1;
  }

static int next_packet_mpegps(bgav_demuxer_context_t * ctx)
  {
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);
  if(priv->sector_size)
    {
    while(1)
      {
      if(!next_packet(ctx, priv->input_mem))
        {
        if(!priv->read_sector(ctx))
          return 0;
        }
      else
        return 1;
      }
    }
  else
    {
    return next_packet(ctx, ctx->input);
    }
  return 0;
  }

#define NUM_PACKETS 200

static void find_streams(bgav_demuxer_context_t * ctx)
  {
  int i;
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);
  priv->find_streams = 1;
  priv->do_sync = 1;
  
  for(i = 0; i < NUM_PACKETS; i++)
    {
    if(!next_packet_mpegps(ctx))
      {
      priv->find_streams = 0;
      return;
      }
    }
  priv->find_streams = 0;
  priv->do_sync = 0;
  return;
  }

static void get_duration(bgav_demuxer_context_t * ctx)
  {
  int64_t scr_start, scr_end;
  uint32_t start_code = 0;
  
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);

  //  fprintf(stderr, "get duration...\n");
  
  if(!ctx->input->total_bytes && !ctx->input->total_sectors)
    return;
  
  if(ctx->input->input->seek_byte)
    {
    /* We already have the first pack header */
    scr_start = priv->pack_header.scr;

    /* Get the last scr */
    bgav_input_seek(ctx->input, -3, SEEK_END);

    while(start_code != PACK_HEADER)
      {
      start_code = previous_start_code(ctx->input);
#if 0
      fprintf(stderr, "previous_start_code %lld %lld ",
              ctx->input->position, ctx->input->total_bytes);
      bgav_dump_fourcc(start_code);
      fprintf(stderr, "\n");
#endif
      }
    
    
    if(!pack_header_read(ctx->input, &(priv->pack_header)))
      {
#if 0
      fprintf(stderr, "pack_header_read failed %lld %lld\n",
              ctx->input->position, ctx->input->total_bytes);
#endif
      return;
      }
    scr_end = priv->pack_header.scr;

    ctx->tt->current_track->duration =
      ((int64_t)(scr_end - scr_start) * GAVL_TIME_SCALE) / 90000;

    bgav_input_seek(ctx->input, priv->data_start, SEEK_SET);
    }
  else if(ctx->input->total_bytes && priv->pack_header.mux_rate)
    {
    ctx->tt->current_track->duration =
      (ctx->input->total_bytes * GAVL_TIME_SCALE)/
      (priv->pack_header.mux_rate*50);
    }
  //  fprintf(stderr, "get duration done\n");
  }


static int do_sync(bgav_demuxer_context_t * ctx)
  {
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);
  priv->do_sync = 1;
  while(!bgav_track_has_sync(ctx->tt->current_track))
    {
    //    fprintf(stderr, "next_packet_mpegps %lld %lld\n",
    //            ctx->input->position, ctx->input->total_bytes);
    if(!next_packet_mpegps(ctx))
      {
      priv->do_sync = 0;
      return 0;
      }
    }
  priv->do_sync = 0;
  return 1;
  }

/* Check for cdxa file, return 0 if there isn't one */

static int init_cdxa(bgav_demuxer_context_t * ctx)
  {
  bgav_track_t * track;
  bgav_stream_t * stream;

  uint32_t fourcc;
  uint32_t size;
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);

  if(ctx->input->input->read_sector)
    return 0;
  
  if(!bgav_input_get_fourcc(ctx->input, &fourcc) ||
     (fourcc != BGAV_MK_FOURCC('R', 'I', 'F', 'F')))
    return 0;

  /* The CDXA is already tested by probe_mpegps, to we can
     directly proceed to the interesting stuff */
  bgav_input_skip(ctx->input, 12);

  while(1)
    {
    /* Go throuth the RIFF chunks until we have a data chunk */

    if(!bgav_input_read_fourcc(ctx->input, &fourcc) ||
       !bgav_input_read_32_le(ctx->input, &size))
      return 0;
    if(fourcc == BGAV_MK_FOURCC('d', 'a', 't', 'a'))
      break;
#if 0
    fprintf(stderr, "Got fourcc ");
    bgav_dump_fourcc(fourcc);
    fprintf(stderr, " skipping %d bytes\n", size);
#endif
    bgav_input_skip(ctx->input, size);
    }
  priv->data_start = ctx->input->position;
  priv->data_size = size;

  priv->total_sectors      = priv->data_size / CDXA_SECTOR_SIZE_RAW;
  fprintf(stderr, "Got CDXA file, %lld sectors (rest: %d)\n",
          priv->total_sectors, (int)(priv->data_size % CDXA_SECTOR_SIZE_RAW));

  priv->sector_size        = CDXA_SECTOR_SIZE;
  priv->sector_size_raw    = CDXA_SECTOR_SIZE_RAW;
  priv->sector_header_size = CDXA_HEADER_SIZE;
  priv->sector_buffer      = malloc(priv->sector_size_raw);

  priv->goto_sector = goto_sector_cdxa;
  priv->read_sector = read_sector_cdxa;
  
  priv->is_cdxa = 1;

  /* Initialize track table */

  ctx->tt = bgav_track_table_create(1);

  track = ctx->tt->current_track;
  
  stream =  bgav_track_add_audio_stream(track);
  stream->fourcc = BGAV_MK_FOURCC('.', 'm', 'p', '2');
  stream->stream_id = 0xc0;
  stream->timescale = 90000;
  
  stream =  bgav_track_add_video_stream(track);
  stream->fourcc = BGAV_MK_FOURCC('m', 'p', 'g', 'v');
  stream->stream_id = 0xe0;
  stream->timescale = 90000;
  
  return 1;
  }

static int init_mpegps(bgav_demuxer_context_t * ctx)
  {
  mpegps_priv_t * priv;
  uint32_t start_code;

  priv = (mpegps_priv_t*)(ctx->priv);
  while(1)
    {
    if(!(start_code = next_start_code(ctx->input)))
      return 0;
    if(start_code == PACK_HEADER)
      break;
    }

  //  fprintf(stderr, "Data start: %lld\n", ctx->input->position);

  priv->data_start = ctx->input->position;
  if(ctx->input->total_bytes)
    priv->data_size = ctx->input->total_bytes - priv->data_start;


  if(!pack_header_read(ctx->input, &(priv->pack_header)))
    return 0;

  return 1;
  }

static int open_mpegps(bgav_demuxer_context_t * ctx,
                       bgav_redirector_context_t ** redir)
  {
  mpegps_priv_t * priv;
  int need_streams = 0;
    
  priv = calloc(1, sizeof(*priv));
  priv->start_pts = -1;
  ctx->priv = priv;
  
  /* Check for sector based access */

  if(!init_cdxa(ctx))
    {
    if(ctx->input->sector_size)
      {
      priv->sector_size        = ctx->input->sector_size;
      priv->sector_size_raw    = ctx->input->sector_size_raw;
      priv->sector_header_size = ctx->input->sector_header_size;
      priv->total_sectors      = ctx->input->total_sectors;
      priv->sector_buffer      = malloc(ctx->input->sector_size_raw);
      
      priv->goto_sector = goto_sector_input;
      priv->read_sector = read_sector_input;
      }
    }

  if(priv->sector_size)
    init_sector_mode(ctx);
  else if(!init_mpegps(ctx))
    return 0;
  
  //  if(!pack_header_read(ctx->input, &(priv->pack_header)))
  //    return 0;
  
  if(!ctx->tt)
    {
    ctx->tt = bgav_track_table_create(1);
    need_streams = 1;
    }
  
  //  fprintf(stderr, "Duration: %lld, Mux rate: %d, total_bytes: %lld\n",
  //          ctx->tt->current_track->duration,
  //          priv->pack_header.mux_rate,
  //          ctx->input->total_bytes);
  if(ctx->tt->current_track->duration == GAVL_TIME_UNDEFINED)
    get_duration(ctx);
  
  if(need_streams)
    find_streams(ctx);
    
  //  fprintf(stderr, "Duration: %lld, Mux rate: %d, total_bytes: %lld\n",
  //          ctx->tt->current_track->duration,
  //          priv->pack_header.mux_rate,
  //          ctx->input->total_bytes);
  
  if((ctx->input->input->seek_byte) ||
     (ctx->input->input->seek_sector))
    ctx->can_seek = 1;
  
  if(!priv->pack_header.mux_rate)
    {
    ctx->stream_description =
      bgav_sprintf("MPEG-%d %s stream, bitrate: Unspecified",
                   priv->pack_header.version,
                   (priv->pack_header.version == 1 ?
                    "system" : "program"));
    }
  else
    ctx->stream_description =
      bgav_sprintf("MPEG-%d %s stream, bitrate: %.2f kb/sec",
                   priv->pack_header.version,
                   (priv->pack_header.version == 1 ?
                    "system" : "program"),
                   (float)priv->pack_header.mux_rate * 0.4);
  return 1;
  }

static void seek_normal(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  mpegps_priv_t * priv;
  int64_t file_position;
  uint32_t header = 0;
  
  priv = (mpegps_priv_t*)(ctx->priv);
  
  //  file_position = (priv->pack_header.mux_rate*50*time)/GAVL_TIME_SCALE;
  file_position = priv->data_start +
    (priv->data_size * time)/
    ctx->tt->current_track->duration;
  
  if(file_position <= priv->data_start)
    file_position = priv->data_start+1;
  if(file_position >= ctx->input->total_bytes)
    file_position = ctx->input->total_bytes - 4;

  while(1)
    {
    bgav_input_seek(ctx->input, file_position, SEEK_SET);
    
    while(header != PACK_HEADER)
      {
      //      fprintf(stderr, "Previous start code %lld..", ctx->input->position);
      header = previous_start_code(ctx->input);
      //      fprintf(stderr, "Done\n");
#if 0
      fprintf(stderr, "Start code: ");
      bgav_dump_fourcc(header);
      fprintf(stderr, "\n");
#endif
      }
    if(do_sync(ctx))
      break;
    else
      {
      file_position -= 2000; /* Go a bit back */
      if(file_position <= priv->data_start)
        {
        break; /* Escape from inifinite loop */
        }
      }
    }
  }

static void seek_sector(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  mpegps_priv_t * priv;
  int64_t sector;
  
  priv = (mpegps_priv_t*)(ctx->priv);
  
  //  file_position = (priv->pack_header.mux_rate*50*time)/GAVL_TIME_SCALE;
  sector = (priv->total_sectors * time)/
    ctx->tt->current_track->duration;

  if(sector < 0)
    sector = 0;
  if(sector >= priv->total_sectors)
    sector = priv->total_sectors - 1;

  while(1)
    {
    priv->goto_sector(ctx, sector);
    
    if(do_sync(ctx))
      break;
    else
      {
      sector--; /* Go a bit back */
      if(sector < 0)
        {
        break; /* Escape from inifinite loop */
        }
      }
    }
  }

static void seek_mpegps(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);
  if(priv->sector_size)
    seek_sector(ctx, time);
  else
    seek_normal(ctx, time);
  }

static void close_mpegps(bgav_demuxer_context_t * ctx)
  {
  mpegps_priv_t * priv;
  priv = (mpegps_priv_t*)(ctx->priv);
  if(!priv)
    return;
  if(priv->sector_buffer)
    free(priv->sector_buffer);
  free(priv);
  }


bgav_demuxer_t bgav_demuxer_mpegps =
  {
    seek_iterative: 1,
    probe:          probe_mpegps,
    open:           open_mpegps,
    next_packet:    next_packet_mpegps,
    seek:           seek_mpegps,
    close:          close_mpegps
  };
