/*****************************************************************
 
  demux_ogg.c
 
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
#include <string.h>
#include <string.h>
#include <vorbis/codec.h>

#define BYTES_TO_READ 8500 /* Same as in vorbisfile */

static char * _artist_key = "ARTIST=";
static char * _album_key = "ALBUM=";
static char * _title_key = "TITLE=";
// static char * _version_key = "VERSION=";
static char * _track_number_key = "TRACKNUMBER=";
// static char * _organization_key = "ORGANIZATION=";
static char * _genre_key = "GENRE=";
// static char * _description_key = "DESCRIPTION=";
// static char * _date_key = "DATE=";
// static char * _location_key = "LOCATION=";
// static char * _copyright_key = "COPYRIGHT=";

static void set_metadata(bgav_metadata_t * ret, vorbis_comment * vc)
  {
  int key_len;
  int j;
  int comment_added = 0;

  if(vc->comments)
    {
    for(j = 0; j < vc->comments; j++)
      {
      key_len = strlen(_artist_key);
      if(!strncasecmp(vc->user_comments[j], _artist_key,
                      key_len))
        {
        ret->artist =
          bgav_strndup(&(vc->user_comments[j][key_len]), NULL);
        continue;
        }
      key_len = strlen(_album_key);
      if(!strncasecmp(vc->user_comments[j], _album_key,
                      key_len))
        {
        ret->album =
          bgav_strndup(&(vc->user_comments[j][key_len]), NULL);
        continue;
        }
      key_len = strlen(_title_key);
      if(!strncasecmp(vc->user_comments[j], _title_key,
                      key_len))
        {
        ret->title =
          bgav_strndup(&(vc->user_comments[j][key_len]), NULL);
        continue;
        }
      
      key_len = strlen(_genre_key);
      if(!strncasecmp(vc->user_comments[j], _genre_key,
                      key_len))
        {
        ret->genre =
          bgav_strndup(&(vc->user_comments[j][key_len]), NULL);
        continue;
        }
      key_len = strlen(_track_number_key);
      if(!strncasecmp(vc->user_comments[j], _track_number_key,
                      key_len))
        {
        ret->track =
          atoi(&(vc->user_comments[j][key_len]));
        continue;
        }
      if(!(comment_added) && !strchr(vc->user_comments[j], '='))
        {
        ret->comment =
          bgav_strndup(vc->user_comments[j], NULL);
        comment_added = 1;
        continue;
        }
      }
    }
  }


/* Currently, we support one single vorbis encoded audio stream */

static int probe_ogg(bgav_input_context_t * input)
  {
  uint8_t probe_data[4];

  if(bgav_input_get_data(input, probe_data, 4) < 4)
    return 0;

  if((probe_data[0] == 'O') &&
     (probe_data[1] == 'g') &&
     (probe_data[2] == 'g') &&
     (probe_data[3] == 'S'))
    return 1;
  return 0;
  }

typedef struct
  {
  int64_t start_position;
  int64_t end_position;
  long serialno;
  int64_t last_pos;
  int ext_size; /* Extradata size for audio stream */
  }  bitstream_info;

typedef struct
  {
  int num_bitstreams;
  bitstream_info * bitstreams;
  } bitstream_table;

static uint8_t * ogg_2_ptr(ogg_packet * op, uint8_t * data)
  {
  memcpy(data, op, sizeof(*op));
  memcpy(data + sizeof(*op), op->packet, op->bytes);
  return data + sizeof(*op) + op->bytes;
  }

static void insert_bitstream(bitstream_table * t, int index)
  {
  t->num_bitstreams++;
  t->bitstreams =
    realloc(t->bitstreams, t->num_bitstreams * sizeof(*(t->bitstreams)));

  if(index < t->num_bitstreams-1)
    {
    memmove(&(t->bitstreams[index+1]), &(t->bitstreams[index]),
            sizeof(*(t->bitstreams)) * (t->num_bitstreams-1-index));
    }
  memset(&(t->bitstreams[index]), 0, sizeof(*(t->bitstreams)));
  t->bitstreams[index].start_position = -1;
  t->bitstreams[index].end_position = -1;
  }

static void bitstreams_free(bitstream_table * t)
  {
  free(t->bitstreams);
  }

static void bitstreams_dump(bitstream_table * t)
  {
  int i;
  fprintf(stderr, "Bitstream table, %d entries\n", t->num_bitstreams);
  for(i = 0; i < t->num_bitstreams; i++)
    {
    fprintf(stderr, "Stream %d, start: %lld, End: %lld, size: %lld, Serialno: %ld, Last position: %lld\n",
            i+1,
            t->bitstreams[i].start_position,
            t->bitstreams[i].end_position,
            t->bitstreams[i].end_position - t->bitstreams[i].start_position,
            t->bitstreams[i].serialno,
            t->bitstreams[i].last_pos);
    }
  }

typedef struct
  {
  ogg_sync_state  oy;
  ogg_stream_state os;
  ogg_page        current_page;
  ogg_packet      current_packet;
    
  bitstream_table streams;
  int64_t position;
  int current_page_size;
  long current_page_serialno;
  int64_t current_page_granulepos;
  
  int stream_initialized;
  int track;
  //  int stream_changed;
  
  gavl_time_t timestamp;
  } ogg_priv;

static void seek_byte(bgav_demuxer_context_t * ctx, int64_t pos)
  {
  ogg_priv * priv = (ogg_priv*)(ctx->priv);
  ogg_sync_reset(&(priv->oy));
  ogg_stream_clear(&(priv->os));
  //  ogg_page_clear(&(priv->os));
  bgav_input_seek(ctx->input, pos, SEEK_SET);
  priv->position = pos;
  priv->stream_initialized = 0;
  }

static int get_data(bgav_demuxer_context_t * ctx)
  {
  char * buf;
  int result;
  
  ogg_priv * priv = (ogg_priv*)(ctx->priv);
  
  buf = ogg_sync_buffer(&(priv->oy), BYTES_TO_READ);
  result = bgav_input_read_data(ctx->input, buf, BYTES_TO_READ);

  //  fprintf(stderr, "Get data\n");
  //  bgav_hexdump(buf, 512, 16);
  
  ogg_sync_wrote(&(priv->oy), result);
  return result;
  }

/* Find the next page, return file position */

static int64_t find_next_page(bgav_demuxer_context_t * ctx, int64_t pos,
                              int64_t end_pos)
  {
  long result;
  ogg_priv * priv = (ogg_priv*)(ctx->priv);

  if(ctx->input->input->seek_byte)
    seek_byte(ctx, pos);
  
  if((end_pos >= 0) && (pos > end_pos))
    return -1;
  
  //  memset(&(priv->current_page), 0, sizeof(priv->current_page));

  //  fprintf(stderr, "Find page: %lld %lld\n",
  //          pos, end_pos);
    
  while(1)
    {
    result = ogg_sync_pageseek(&(priv->oy),&(priv->current_page));
    //    fprintf(stderr, "Result: %ld\n", result);
    if(!result) /* Need more data */
      {
      //      if((end_pos > 0) && (priv->position > end_pos))
      //        return -1;
      //      else
      //        {
      if(!get_data(ctx))
        return -1;
      //        }
      }
    else if(result < 0) /* Skipped that many bytes */
      {
      priv->position -= result;
      if((end_pos > 0) && (priv->position > end_pos))
        return -1;
      }
    else /* Page is there, page size is result */
      {
      priv->current_page_serialno =
        ogg_page_serialno(&(priv->current_page));

      priv->current_page_granulepos =
        ogg_page_granulepos(&(priv->current_page));
      
      priv->current_page_size = priv->current_page.header_len +
        priv->current_page.body_len;
      break;
      }
    }

  //  fprintf(stderr, "Found page: %lld\n", priv->position);
  
  return priv->position;
  }

/* Find the previous page, return file position */

static int64_t find_previous_page(bgav_demuxer_context_t * ctx,
                                  int64_t pos, int64_t start_pos)
  {
  int64_t byte_pos;
  int64_t page_pos;
  int64_t old_pos = -1;
  
  ogg_priv * priv = (ogg_priv*)(ctx->priv);

  byte_pos = pos - BYTES_TO_READ;

  //  fprintf(stderr, "Find previous page %lld %lld\n",
  //          pos, start_pos);
  
  while(1)
    {
    if(byte_pos < start_pos)
      byte_pos = start_pos;
    page_pos = find_next_page(ctx, byte_pos, pos);
    
    //    fprintf(stderr, "find_previous_page: %lld %lld %lld\n",
    //             page_pos, byte_pos, pos);
    
    if(page_pos == -1)
      {
      if(old_pos > 0) /* Return last page start */
        return old_pos;
      /* No page found, try to decrement the position */
      if((start_pos > 0) && (byte_pos > start_pos))
        {
        byte_pos -= BYTES_TO_READ;
        
        }
      else /* Already at the beginning, return Old position */
        return -1;
      }
    else
      {
      byte_pos = page_pos + priv->current_page_size;
      old_pos = page_pos;
      //      fprintf(stderr, "Serialno: %d\n",
      //              ogg_page_serialno(&(priv->current_page)));
      
      }
    }
  return -1;
  }

static int get_next_packet(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  ogg_priv * priv = (ogg_priv*)(ctx->priv);
  long serialno;
  
  //  fprintf(stderr, "get_next_packet...");
  
  while(!ogg_stream_packetout(&(priv->os), &(priv->current_packet)))
    {
    //    fprintf(stderr, "Need new page");
    
    /* Now, it's time to catch the timestamp, it't gonna be the timestamp
       for the next packet of the next page */
    
    if((ctx->tt->current_track->audio_streams) &&
       (priv->timestamp == GAVL_TIME_UNDEFINED))
      {
      s = &(ctx->tt->current_track->audio_streams[0]);
      priv->timestamp =
        gavl_samples_to_time(s->data.audio.format.samplerate,
                             priv->current_page_granulepos);
      }
    
    /* Read the next page */

    //    fprintf(stderr, "Next page...");
    
    while(!ogg_sync_pageout(&(priv->oy), &(priv->current_page)))
      if(!get_data(ctx))
        return 0;

    //    fprintf(stderr, "done\n");
    
    /* Check, if we have the proper serial number of the stream */

    serialno = ogg_page_serialno(&(priv->current_page));
    
    if((priv->track >= 0) &&
       (serialno !=
        priv->streams.bitstreams[priv->track].serialno))
      {
      //      fprintf(stderr, "Logical bitstream changed!!!\n");
      if(ctx->input->input->seek_byte)
        return 0;
      else
        {
        priv->stream_initialized = 0;
        priv->streams.bitstreams[priv->track].serialno = serialno;
        }
      }
    
    priv->current_page_granulepos =
      ogg_page_granulepos(&(priv->current_page));
    
    //    fprintf(stderr, "ogg stream pagein\n");

    if(!priv->stream_initialized)
      {
      ogg_stream_reset_serialno(&priv->os,
                                ogg_page_serialno(&priv->current_page));
      priv->stream_initialized = 1;
      }
    ogg_stream_pagein(&(priv->os), &(priv->current_page));
    }
  //  fprintf(stderr, "Done %ld %ld\n", priv->current_packet.bytes,
  //          priv->current_packet.b_o_s);
  return 1;
  }

static void set_packet(ogg_priv * priv, bgav_packet_t * p)
  {
  bgav_packet_alloc(p, sizeof(priv->current_packet) +
                    priv->current_packet.bytes);
  ogg_2_ptr(&(priv->current_packet), p->data);
  p->data_size = sizeof(ogg_packet) + priv->current_packet.bytes;
  p->keyframe = 1;

  if(priv->timestamp != GAVL_TIME_UNDEFINED)
    {
    p->timestamp = priv->timestamp ;
    priv->timestamp = GAVL_TIME_UNDEFINED;
    }
  }

static int next_packet_ogg(bgav_demuxer_context_t * ctx)
  {
  int i;
  char * name;
  bgav_packet_t * p;
  bgav_stream_t * s;
  bgav_metadata_t metadata;
  vorbis_info vi;
  vorbis_comment vc;
  
  ogg_priv * priv = (ogg_priv*)(ctx->priv);

  s = &(ctx->tt->current_track->audio_streams[0]);
 
  if(!get_next_packet(ctx))
    return 0;

  /* New stream is there, update name and metadata */
  if(priv->current_packet.b_o_s)
    {
    //    fprintf(stderr, "New stream!!!\n");
    vorbis_info_init(&vi);
    vorbis_comment_init(&vc);

    p = bgav_packet_buffer_get_packet_write(s->packet_buffer);
    set_packet(priv, p);
    bgav_packet_done_write(p);

    if(vorbis_synthesis_headerin(&vi, &vc, &(priv->current_packet)))
      {
      fprintf(stderr, "Stream %d: No vorbis header %ld %ld\n",
              i+1, priv->current_packet.bytes, priv->current_packet.b_o_s);
      bgav_hexdump(priv->current_packet.packet,
                   priv->current_packet.bytes, 16);
      return 0;
      }
    
    if(!get_next_packet(ctx))
      return 0;
    p = bgav_packet_buffer_get_packet_write(s->packet_buffer);
    set_packet(priv, p);
    bgav_packet_done_write(p);
    vorbis_synthesis_headerin(&vi, &vc, &(priv->current_packet));

    if(!get_next_packet(ctx))
      return 0;
    p = bgav_packet_buffer_get_packet_write(s->packet_buffer);
    set_packet(priv, p);
    bgav_packet_done_write(p);
    vorbis_synthesis_headerin(&vi, &vc, &(priv->current_packet));

    memset(&metadata, 0, sizeof(metadata));
    set_metadata(&metadata, &vc);

    //    fprintf(stderr, "Metadata:\n");
    //    bgav_metadata_dump(&metadata);
    
    if(ctx->name_change_callback)
      {
      if(metadata.artist && metadata.title)
        {
        name = bgav_sprintf("%s - %s",
                            metadata.artist, metadata.title);
        ctx->name_change_callback(ctx->name_change_callback_data,
                                  name);
        free(name);
        }
      else if(metadata.title)
        {
        ctx->name_change_callback(ctx->name_change_callback_data,
                                  metadata.title);
        }
      }
    //    else
    //      fprintf(stderr, "NO NAME CHANGE CALLBACK\n");
    if(ctx->metadata_change_callback)
      {
      bgav_metadata_merge2(&metadata, &(ctx->input->metadata));
      ctx->metadata_change_callback(ctx->metadata_change_callback_data,
                                    &metadata);
      }
    //    else
    //      fprintf(stderr, "NO METADATA CHANGE CALLBACK\n");
    bgav_metadata_free(&metadata);

    vorbis_info_clear(&vi);
    vorbis_comment_clear(&vc);
    
    }
  else
    {
    p = bgav_packet_buffer_get_packet_write(s->packet_buffer);
    set_packet(priv, p);
    bgav_packet_done_write(p);
    }
  
  
  
  //  fprintf(stderr, "Granulepos: %lld\n", priv->current_packet.granulepos);
  
  return 1;
  }

/*
 *  Do a bisection search for the specified stream
 *  It is assumed, that the stream already has the start position set
 */

static void bisect(bgav_demuxer_context_t * ctx, int index)
  {
  //  long serialno;

  int64_t start;
  int64_t end;
  int64_t pos;
  int64_t page_pos;
  int64_t last_page_pos_1 = -1;
  int64_t last_page_pos_2 = -1;
  ogg_priv * priv = (ogg_priv*)(ctx->priv);

  start = priv->streams.bitstreams[index].start_position;
  end   = priv->streams.bitstreams[index+1].end_position;
  
  while(1)
    {
    pos = (end + start)/2;
    page_pos = find_next_page(ctx, pos, end);

    //    fprintf(stderr, "Bisect %lld %lld %lld...", start, end, pos);
    
    if((end - start <= BYTES_TO_READ) || (page_pos < 0))
      {
      //      fprintf(stderr, "Done\n");
      break;
#if 0
      //      fprintf(stderr, "No page found\n");
      //      fprintf(stderr, "Page size: %d\n", priv->current_page_size);
      priv->streams.bitstreams[index].end_position = last_page_pos;
      
      /* Get stream duration */
      
      last_page_pos =
        find_previous_page(ctx, last_page_pos,
                           priv->streams.bitstreams[index].start_position);
      priv->streams.bitstreams[index].last_pos = priv->current_page_granulepos;
      //      fprintf(stderr, "Last Pos: %lld, Page pos: %lld\n",
      //              priv->streams.bitstreams[index].last_pos,
      //              last_page_pos);
      if(index < priv->streams.num_bitstreams - 1)
        priv->streams.bitstreams[index+1].start_position = 
          priv->streams.bitstreams[index].end_position;
      return;
#endif
      }
    else
      {
      priv->current_page_serialno = ogg_page_serialno(&(priv->current_page));
      //      fprintf(stderr, "Serialno at %lld, %ld\n", page_pos, serialno);
      //      last_page_pos = page_pos;
      if(priv->current_page_serialno == priv->streams.bitstreams[index].serialno)
        {
        //        end = page_pos;
        //        fprintf(stderr, "Lower stream at %lld\n", page_pos);
        start = page_pos;
        last_page_pos_1 = page_pos;
        }
      else if(priv->current_page_serialno == priv->streams.bitstreams[index+1].serialno)
        {
        //        start = pos;
        //        fprintf(stderr, "Upper stream at %lld\n", page_pos);
        last_page_pos_2 = page_pos;
        end = pos;
        }
      else
        {
        //        fprintf(stderr, "New Stream %ld %ld %ld\n",
        //                priv->streams.bitstreams[index].serialno,
        //                priv->streams.bitstreams[index+1].serialno,
        //                priv->current_page_serialno);
        insert_bitstream(&(priv->streams), index+1);
        priv->streams.bitstreams[index+1].serialno = priv->current_page_serialno;
        priv->streams.bitstreams[index+1].end_position = page_pos;
        end = pos;
        }
      }
    
    }

  /* Now we have the lower stream AFTER last_page_pos_1 and the
     Upper stream BEFORE last_page_pos_2 */

  pos = last_page_pos_1;
  
  while(1)
    {
    page_pos = find_next_page(ctx, pos, last_page_pos_2+1);
    if(page_pos < 0)
      {
      fprintf(stderr, "Dammit\n");
      return;
      }
    if(priv->current_page_serialno ==
       priv->streams.bitstreams[index].serialno)
      {
      priv->streams.bitstreams[index].last_pos =
        priv->current_page_granulepos;
      pos += priv->current_page_size;
      }
    else
      {
      //      fprintf(stderr, "Found first page of next stream (%ld) at %lld\n",
      //              priv->current_page_serialno, page_pos);
      priv->streams.bitstreams[index].end_position = page_pos;
      if(index < priv->streams.num_bitstreams - 1)
        priv->streams.bitstreams[index+1].start_position = 
          priv->streams.bitstreams[index].end_position;
      break;
      }
    }

  //  fprintf(stderr, "Size: %lld, Last pos: %lld\n",
  //          priv->streams.bitstreams[index].end_position -
  //          priv->streams.bitstreams[index].start_position,
  //          priv->streams.bitstreams[index].last_pos);
  
  }

static int setup_streams(bgav_demuxer_context_t * ctx)
  {
  int result;
  vorbis_info vi;
  vorbis_comment vc;
  int i, j;
  bgav_stream_t * s;
  char * filename_base;
  char * extension_start;
  char * pos;
  int packet_size;
  uint8_t * ptr;
  if(ctx->input->filename)
    {
    extension_start = strrchr(ctx->input->filename, '.');
    pos = strrchr(ctx->input->filename, '/');
    if(!pos)
      pos = ctx->input->filename;
    else
      pos++;
    filename_base = bgav_strndup(pos, extension_start);
    }
  else
    filename_base = (char*)0;
  
  ogg_priv * priv = (ogg_priv*)(ctx->priv);

  ctx->tt = bgav_track_table_create(priv->streams.num_bitstreams);

  /* This way, we prevent errors from nonmatching serialnos */
  priv->track = -1; 
  
  for(i = 0; i < priv->streams.num_bitstreams; i++)
    {
    s = bgav_track_add_audio_stream(&(ctx->tt->tracks[i]));
    
    if(ctx->input->input->seek_byte)
      seek_byte(ctx, priv->streams.bitstreams[i].start_position);
    vorbis_info_init(&vi);
    vorbis_comment_init(&vc);

    priv->streams.bitstreams[i].ext_size = 3 * sizeof(priv->current_packet);
    
    for(j = 0; j < 3; j++)
      {
      /* Get the first packet */

      while(1)
        {
        if(!get_next_packet(ctx))
          {
          //          fprintf(stderr, "get_next_packet failed\n");
          return 0;
          }
        if(priv->current_packet.bytes)
          break;
        //        else
        //          fprintf(stderr, "Skipping packet\n");
        }
      packet_size = sizeof(ogg_packet) + priv->current_packet.bytes;
      s->ext_data = realloc(s->ext_data, s->ext_size + packet_size);
      ptr = s->ext_data + s->ext_size;
      
      s->ext_size += packet_size;
      
      ptr = ogg_2_ptr(&(priv->current_packet),
                      ptr);
      
      if((result = vorbis_synthesis_headerin(&vi, &vc,
                                             &(priv->current_packet))))
        {
        fprintf(stderr, "Stream %d: No vorbis header %ld %ld\n",
                i+1, priv->current_packet.bytes, priv->current_packet.b_o_s);
        bgav_hexdump(priv->current_packet.packet,
                     priv->current_packet.bytes, 16);
        return 0;
        }
      }
    /* Now, read the infos */
      
    s->data.audio.format.samplerate = vi.rate;

    /* Calculate duration */

    if(priv->streams.bitstreams[i].last_pos)
      ctx->tt->tracks[i].duration =
        gavl_samples_to_time(vi.rate, priv->streams.bitstreams[i].last_pos);
    else
      ctx->tt->tracks[i].duration = GAVL_TIME_UNDEFINED;
    s->data.audio.format.num_channels = vi.channels;

    s->fourcc = BGAV_MK_FOURCC('V','B','I','S');
    
    set_metadata(&(ctx->tt->tracks[i].metadata), &vc);

    /* Set the stream name. We do this only
       if we have more than one track. */
    
    if(priv->streams.num_bitstreams > 1)
      {
      
      /* 1. Variant: Input has metadata
         (probably the radio station?) */
    
      if(ctx->input->metadata.title)
        {
        ctx->tt->tracks[i].name =
          bgav_strndup(ctx->input->metadata.title, NULL);
        }

      /* 2. Variant: Metadata for track contain title */
      else if(ctx->tt->tracks[i].metadata.title)
        {
        ctx->tt->tracks[i].name =
          bgav_strndup(ctx->tt->tracks[i].metadata.title, NULL);
        }
      /* 3rd Variant: Take filename plus track number */
      else if(filename_base)
        {
        ctx->tt->tracks[i].name =
          bgav_sprintf("%s: Track %d", filename_base, i+1);
        }
      else
        ctx->tt->tracks[i].name =
          bgav_sprintf("Track %d", i+1);
      }
    else if(ctx->input->metadata.title)
      {
      ctx->tt->tracks[i].name =
        bgav_strndup(ctx->input->metadata.title, NULL);
      }
    
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
    }
  if(filename_base)
    free(filename_base);
  return 1;
  }

static int open_ogg(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  int i;
  int64_t pos;
  int result;
  ogg_priv * priv;
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  ogg_sync_init(&(priv->oy));
  ogg_stream_init(&(priv->os),-1);
  
  /* Add the first bitstream.
     This is the standard procedure for all cases */
  
  insert_bitstream(&(priv->streams), 0);

  /* Get the first page and save the serial number */

  if(ctx->input->input->seek_byte && ctx->input->total_bytes)
    {
    priv->streams.bitstreams[0].start_position = find_next_page(ctx, 0, -1);
    
    //  fprintf(stderr, "Start Position: %lld\n",
    //          priv->streams.bitstreams[0].start_position);
    
    priv->streams.bitstreams[0].serialno =
      ogg_page_serialno(&(priv->current_page));
    
    //  fprintf(stderr, "First Serialno: %d\n",
    //          ogg_page_serialno(&(priv->current_page)));
    
    /* If input is seekable, we might get more than one logical bitstream */
    /* We check the serial number of the last page */
    
    pos = find_previous_page(ctx, ctx->input->total_bytes, 0);
    //    fprintf(stderr, "Last page starts at: %lld %lld\n", pos,
    //            pos + priv->current_page_size);
    
    if(priv->streams.bitstreams[0].serialno != priv->current_page_serialno)
      {
      /* More than one logical bitstreams in the file,
         search for the offsets */
      
      insert_bitstream(&(priv->streams), 1);
      priv->streams.bitstreams[1].serialno = priv->current_page_serialno;
      priv->streams.bitstreams[1].end_position = ctx->input->total_bytes;
      
      //      fprintf(stderr, "
      
      priv->streams.bitstreams[1].last_pos =
        priv->current_page_granulepos;
      
      //      fprintf(stderr, "Last page: Serialno: %ld, last page pos: %lld\n",
      //              priv->current_page_serialno,
      //              priv->streams.bitstreams[1].last_pos);
      
      /* Add intermediate bitstreams by bisection */
      
      i = 0;
      while(i < priv->streams.num_bitstreams-1)
        {
        //        fprintf(stderr, "Doing Bisection for stream %d\n",
        //                i+1);
        bisect(ctx, i);
        i++;
        }

      //      bitstreams_dump(&(priv->streams));
      }
    else
      {
      priv->streams.bitstreams[0].end_position = ctx->input->total_bytes;
      priv->streams.bitstreams[0].last_pos = priv->current_page_granulepos;
      }
    }
  else /* Non seekable case */
    {
    priv->streams.bitstreams[0].start_position = 0;

    //  fprintf(stderr, "Start Position: %lld\n",
    //          priv->streams.bitstreams[0].start_position);
    
    while((result = ogg_sync_pageout(&(priv->oy),
                                     &(priv->current_page))) <= 0)
      {
      if(!result)
        {
        if(!get_data(ctx))
          return 0;
        }
      //      else
      //        fprintf(stderr, "Result: %d\n", result);
      }
    priv->streams.bitstreams[0].serialno =
      ogg_page_serialno(&(priv->current_page));

    //    fprintf(stderr, "Serialno: %ld\n",
    //            priv->streams.bitstreams[0].serialno);

    ogg_stream_reset_serialno(&priv->os,
                              priv->streams.bitstreams[0].serialno);
    priv->stream_initialized = 1;
    ogg_stream_pagein(&(priv->os), &(priv->current_page));
    }

  /* Now, initialize all the streams */

  if(!setup_streams(ctx))
    return 0;
    
  if(ctx->input->input->seek_byte)
    ctx->can_seek = 1;
    
  return 1;
  }

static void seek_ogg(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  int64_t pos, page_pos;
  bgav_stream_t * s;
  
  ogg_priv * priv;
  int64_t sample_pos;
  
  priv = (ogg_priv *)ctx->priv;
  
  s = &(ctx->tt->current_track->audio_streams[0]);
  
  sample_pos =
    gavl_time_to_samples(s->data.audio.format.samplerate,
                         time);
  
  pos = priv->streams.bitstreams[priv->track].start_position +
    (int)((float)(priv->streams.bitstreams[priv->track].end_position -
                  priv->streams.bitstreams[priv->track].start_position) *
          (float)(time)/(float)(ctx->tt->current_track->duration));
  
  page_pos =
    find_previous_page(ctx, pos, 
                       priv->streams.bitstreams[priv->track].start_position);
  //  fprintf(stderr, "Previous page pos: %lld %lld %lld\n", pos, page_pos,
  //          priv->streams.bitstreams[priv->track].start_position);
    
  if(page_pos == -1)
    {
    //    fprintf(stderr, "Lost sync during seeking 1, %lld\n", pos);
    return;
    }
  pos = page_pos + priv->current_page_size;

          
  
  s->time = gavl_samples_to_time(s->data.audio.format.samplerate,
                                 priv->current_page_granulepos);
#if 0
  fprintf(stderr, "Samplerate: %d, granulepos: %lld, time: %f\n",
          s->data.audio.format.samplerate,
          priv->current_page_granulepos,
          gavl_time_to_seconds(s->time));
#endif        
  page_pos =
    find_next_page(ctx, pos, 
                   priv->streams.bitstreams[priv->track].end_position);
  
  if(page_pos == -1)
    {
    //    fprintf(stderr, "Lost sync during seeking 2, %lld\n", pos);
    s->time = GAVL_TIME_UNDEFINED;
    }
  
  //  seek_byte(
  }

static void close_ogg(bgav_demuxer_context_t * ctx)
  {
  int i;
  ogg_priv * priv;

  for(i = 0; i < ctx->tt->num_tracks; i++)
    {
    if(ctx->tt->tracks[i].audio_streams)
      if(ctx->tt->tracks[i].audio_streams[0].ext_data)
        free(ctx->tt->tracks[i].audio_streams[0].ext_data);
    }
  
  priv = (ogg_priv *)ctx->priv;
  ogg_stream_clear(&priv->os);
  ogg_sync_clear(&priv->oy);
  bitstreams_free(&priv->streams);
  free(priv);
  }

static void select_track_ogg(bgav_demuxer_context_t * ctx,
                             int track)
  {
  char * name;
  ogg_priv * priv;
  priv = (ogg_priv *)ctx->priv;
  //  fprintf(stderr, "Select track ogg\n");
  //   bgav_metadata_dump(&ctx->input.metadata);
  
  priv->track = track;

  if(ctx->input->input->seek_byte && ctx->input->total_bytes)
    seek_byte(ctx, priv->streams.bitstreams[track].start_position);

  /* Sent the track name via callback */
  
  else if(ctx->name_change_callback)
    {
    if(ctx->tt->current_track->metadata.artist &&
       ctx->tt->current_track->metadata.title) 
      {
      name = bgav_sprintf("%s - %s",
                          ctx->tt->current_track->metadata.artist,
                          ctx->tt->current_track->metadata.title);
      ctx->name_change_callback(ctx->name_change_callback_data,
                                name);
      free(name);
      }
    else if(ctx->tt->current_track->metadata.title)
      {
      ctx->name_change_callback(ctx->name_change_callback_data,
                                ctx->tt->current_track->metadata.title);
      }
    }
  //  else
  //    fprintf(stderr, "NO NAME CHANGE CALLBACK\n");
  
  
  /* Set extradata for the track */
  
  //  fprintf(stderr, "Select track ogg done\n");
  //  priv->streams.bitstreams[track].
  
  }


bgav_demuxer_t bgav_demuxer_ogg =
  {
    probe:        probe_ogg,
    open:         open_ogg,
    next_packet:  next_packet_ogg,
    seek:         seek_ogg,
    close:        close_ogg,
    select_track: select_track_ogg
  };
