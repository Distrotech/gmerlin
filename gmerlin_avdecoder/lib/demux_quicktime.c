/*****************************************************************
 
  demux_quicktime.c
 
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
#include <qt.h>
#include <utils.h>

typedef struct
  {
  qt_moov_t moov;
  
  int32_t num_packets;
  int32_t current_packet;
  
  struct
    {
    int64_t offset;
    int32_t size;
    int stream_id;
    int keyframe;
    int64_t time;
    } * packet_table;

  int64_t mdat_start;
  int64_t mdat_size;

  int seeking;

  /* Ohhhh, no, MPEG-4 can have multiple mdats... */
  
  int num_mdats;
  int mdats_alloc;
  qt_atom_header_t * mdats;
  } qt_priv_t;

/*
 *  We support all 3 types of quicktime audio encapsulation:
 *
 *  1: stsd version 0: Uncompressed audio
 *  2: stsd version 1: CBR encoded audio: Additional fields added
 *  3: stsd version 1, _Compression_id == -2: VBR audio, one "sample"
 *                     equals one frame of compressed audio data
 */

typedef struct
  {
  qt_trak_t * trak;
  qt_stbl_t * stbl; /* For convenience */
  int64_t stts_pos;
  int64_t stss_pos;
  int64_t stsd_pos;
  int64_t stsc_pos;
  int64_t stco_pos;
  int64_t stsz_pos;

  int64_t stts_count;
  int64_t stss_count;
  int64_t stsd_count;
  //  int64_t stsc_count;

  int64_t tics; /* Time in tics (depends on time scale of this stream) */
  int64_t total_tics;
  
  int time_scale;
  int type; /* So generic functions know what this stream is */
  int has_key_frames;           /* 1 if stream has keyframes */

  int32_t packet_index; /* Index in master index */
  } stream_priv_t;

/* Intitialize everything */

static void stream_rewind(stream_priv_t  * s)
  {
  s->stss_pos = 0;
  s->stsd_pos = 0;
  s->stsc_pos = 0;
  s->stco_pos = 0;
  /* stts_pos is -1 if all samples have same duration */
  s->stts_pos = (s->stbl->stts.num_entries > 1) ? 0 : -1;
  /* stsz_pos is -1 if all samples have the same size */
  s->stsz_pos = (s->stbl->stsz.sample_size) ? -1 : 0;
  
  s->stts_count = 0;
  s->stss_count = 0;
  s->stsd_count = 0;
  //  s->stsc_count = 0;
  }

static stream_priv_t * stream_create(qt_trak_t * trak, int type)
  {
  int i;
  stream_priv_t * s = calloc(1, sizeof(*s));
  s->trak = trak;
  s->stbl = &(trak->mdia.minf.stbl);
  s->time_scale = trak->mdia.mdhd.time_scale;

  stream_rewind(s);
  
  for(i = 0; i < s->stbl->stts.num_entries;i++)
    {
    s->total_tics += s->stbl->stts.entries[i].count *
      s->stbl->stts.entries[i].duration;
    }
  s->type = type;
  
  return s;
  }

static gavl_time_t stream_get_duration(stream_priv_t * s)
  {
  return (s->total_tics * GAVL_TIME_SCALE) / s->time_scale;
  }

static int probe_quicktime(bgav_input_context_t * input)
  {
  uint32_t header;
  uint8_t test_data[16];
  uint8_t * pos;
  
  if(bgav_input_get_data(input, test_data, 16) < 16)
    return 0;

  pos = test_data + 4;

  header = BGAV_PTR_2_FOURCC(pos);

  if(header == BGAV_MK_FOURCC('w','i','d','e'))
    {
    pos = test_data + 12;
    header = BGAV_PTR_2_FOURCC(pos);
    }
  
  if(header == BGAV_MK_FOURCC('m','o','o','v'))
    return 1;
  if(header == BGAV_MK_FOURCC('f','t','y','p'))
    return 1;
  else if((header == BGAV_MK_FOURCC('m','d','a','t')) &&
          input->input->seek_byte)
    return 1;
  
  return 0;
  }

static stream_priv_t * find_stream(bgav_demuxer_context_t * ctx,
                                   qt_trak_t * trak)
  {
  //  qt_priv_t * priv; 
  stream_priv_t * ret;
  int i;
  bgav_track_t * track = ctx->tt->current_track;
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    ret = (stream_priv_t *)(track->audio_streams[i].priv);
    if(ret->trak == trak)
      return ret;
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    ret = (stream_priv_t *)(track->video_streams[i].priv);
    if(ret->trak == trak)
      return ret;
    }
  return (stream_priv_t*)0;
  }

static int check_keyframe(stream_priv_t * s)
  {
  int ret;
  if(!s->stbl->stss.num_entries)
    return 1;
  if(s->stss_pos >= s->stbl->stss.num_entries)
    return 0;

  s->stss_count++;
  ret = (s->stbl->stss.entries[s->stss_pos] == s->stss_count) ? 1 : 0;
  if(ret)
    s->stss_pos++;
  return ret;
  }

static void add_packet(qt_priv_t * priv,
                       int index,
                       int64_t offset,
                       int stream_id,
                       int64_t timestamp,
                       int keyframe)
  {
  priv->packet_table[index].offset = offset;
  priv->packet_table[index].stream_id = stream_id;
  priv->packet_table[index].time = timestamp;
  priv->packet_table[index].keyframe = keyframe;
  if(index)
    priv->packet_table[index-1].size =
      priv->packet_table[index].offset - priv->packet_table[index-1].offset;
  }

static void build_index(bgav_demuxer_context_t * ctx)
  {
  int i, j;
  int stream_id = 0;
  int64_t chunk_offset;
  int64_t * chunk_indices;
  stream_priv_t * s;
  qt_priv_t * priv;
  priv = (qt_priv_t *)(ctx->priv);

  /* 1 step: Count the total number of chunks */
  for(i = 0; i < priv->moov.num_tracks; i++)
    {
    s = find_stream(ctx, &(priv->moov.tracks[i]));
    
    if(!s)
      continue;
    if(s->type == BGAV_STREAM_VIDEO) /* One video chunk can be more packets (=frames) */
      {
      for(j = 0; j < priv->moov.tracks[i].mdia.minf.stbl.stco.num_entries; j++)
        {
        priv->num_packets += s->stbl->stsc.entries[s->stsc_pos].samples_per_chunk;

        s->stco_pos++;
        /* Update sample to chunk */
        if( s->stsc_pos < s->stbl->stsc.num_entries - 1)
          {
          if(s->stbl->stsc.entries[s->stsc_pos+1].first_chunk - 1 == s->stco_pos)
            s->stsc_pos++;
          }
        }
      stream_rewind(s);
      }
    else /* Audio packets are quicktime chunks */
      priv->num_packets += priv->moov.tracks[i].mdia.minf.stbl.stco.num_entries;
    }
  
  if(!priv->num_packets)
    return;
  
  priv->packet_table = calloc(priv->num_packets, sizeof(*priv->packet_table));
  chunk_indices = calloc(priv->moov.num_tracks, sizeof(*chunk_indices));
  i = 0;
  while(i < priv->num_packets)
    {
    /* Find the track with the lowest chunk offset */

    chunk_offset = 9223372036854775807LL; /* Maximum */
    
    for(j = 0; j < priv->moov.num_tracks; j++)
      {
      s = find_stream(ctx, &(priv->moov.tracks[j]));
      if(!s)
        continue;
      if((s->stco_pos < s->stbl->stco.num_entries) &&
         (priv->moov.tracks[j].mdia.minf.stbl.stco.entries[s->stco_pos] < chunk_offset))
        {
        stream_id = j;
        chunk_offset = priv->moov.tracks[j].mdia.minf.stbl.stco.entries[s->stco_pos];
        }

      }
    s = find_stream(ctx, &(priv->moov.tracks[stream_id]));
    if(!s)
      continue;

    if(s->type == BGAV_STREAM_AUDIO)
      {
      /* Fill in the stream_id and position */
      add_packet(priv,
                 i, chunk_offset,
                 stream_id,
                 s->tics,
                 check_keyframe(s));
      /* Time to sample */
      if(s->stts_pos >= 0)
        {
        s->tics +=
          s->stbl->stts.entries[s->stts_pos].duration *
          s->stbl->stsc.entries[s->stsc_pos].samples_per_chunk;
        s->stts_count++;
        if(s->stts_count >= s->stbl->stts.entries[s->stts_pos].count)
          {
          s->stts_pos++;
          s->stts_count = 0;
          }
        }
      else
        {
        s->tics += s->stbl->stts.entries[0].duration *
          s->stbl->stsc.entries[s->stsc_pos].samples_per_chunk;
        }

      s->stco_pos++;
      /* Update sample to chunk */
      if(s->stsc_pos < s->stbl->stsc.num_entries - 1)
        {
        if(s->stbl->stsc.entries[s->stsc_pos+1].first_chunk - 1 == s->stco_pos)
          s->stsc_pos++;
        }

      
      i++;
      }
    else if(s->type == BGAV_STREAM_VIDEO)
      {
      for(j = 0; j < s->stbl->stsc.entries[s->stsc_pos].samples_per_chunk; j++)
        {
        add_packet(priv,
                   i, chunk_offset,
                   stream_id,
                   s->tics,
                   check_keyframe(s));
        chunk_offset += (s->stsz_pos >= 0) ? s->stbl->stsz.entries[s->stsz_pos]:
          s->stbl->stsz.sample_size;
        
        /* Sample size */
        if(s->stsz_pos >= 0)
          s->stsz_pos++;
        
        /* Time to sample */
        if(s->stts_pos >= 0)
          {
          s->tics += s->stbl->stts.entries[s->stts_pos].duration;
          s->stts_count++;
          if(s->stts_count >= s->stbl->stts.entries[s->stts_pos].count)
            {
            s->stts_pos++;
            s->stts_count = 0;
            }
          }
        else
          {
          s->tics += s->stbl->stts.entries[0].duration;
          }
        i++;
        }
      s->stco_pos++;
      /* Update sample to chunk */
      if(s->stsc_pos < s->stbl->stsc.num_entries - 1)
        {
        if(s->stbl->stsc.entries[s->stsc_pos+1].first_chunk - 1 == s->stco_pos)
          s->stsc_pos++;
        }
      }
    else
      {
      /* Fill in dummy packet */
      add_packet(priv, i, chunk_offset, -1, -1, 0);
      i++;
      }
    }
  priv->packet_table[priv->num_packets-1].size =
    priv->mdat_start + priv->mdat_size - priv->packet_table[priv->num_packets-1].offset;
  
  /* Dump this */
#if 0
  fprintf(stderr, "Mdat: %lld %lld\n", priv->mdat_start, priv->mdat_size);
  for(i = 0; i < priv->num_packets; i++)
    {
    fprintf(stderr, "Chunk %d, Stream %d, offset: %lld, size: %d time: %f, Key: %d\n",
            i, priv->packet_table[i].stream_id,
            priv->packet_table[i].offset,
            priv->packet_table[i].size,
            gavl_time_to_seconds(priv->packet_table[i].timestamp),
            priv->packet_table[i].keyframe);
    }
#endif
  }

#define SET_UDTA_STRING(dst, src) \
if(!(ctx->tt->current_track->metadata.dst) && moov->udta.src)\
  { \
  ctx->tt->current_track->metadata.dst=bgav_convert_string(cnv, moov->udta.src, -1, NULL);\
  }

static void set_metadata(bgav_demuxer_context_t * ctx)
  {
  bgav_charset_converter_t * cnv;

  qt_priv_t * priv;
  qt_moov_t * moov;
  priv = (qt_priv_t*)(ctx->priv);
  moov = &(priv->moov);

  cnv = bgav_charset_converter_create("ISO-8859-1", "UTF-8");
    
  SET_UDTA_STRING(copyright, cpy);
  SET_UDTA_STRING(title,     nam);
  SET_UDTA_STRING(comment,   cmt);
  SET_UDTA_STRING(comment,   inf);
  SET_UDTA_STRING(author,    aut);
  SET_UDTA_STRING(author,    ART);
  bgav_charset_converter_destroy(cnv);
  }

static void quicktime_init(bgav_demuxer_context_t * ctx)
  {
  int i;
  uint32_t atom_size;
  uint8_t * ptr;
  bgav_stream_t * bg_as;
  bgav_stream_t * bg_vs;
  stream_priv_t * stream_priv;
  gavl_time_t     stream_duration;
  qt_sample_description_t * desc;
  bgav_track_t * track;
    
  qt_priv_t * priv = (qt_priv_t*)(ctx->priv);
  qt_moov_t * moov = &(priv->moov);

  track = ctx->tt->current_track;
    
  ctx->tt->current_track->duration = 0;
  for(i = 0; i < moov->num_tracks; i++)
    {
    /* Audio stream */
    if(moov->tracks[i].mdia.minf.has_smhd)
      {
      //      fprintf(stderr, "Found audio stream\n");
      bg_as = bgav_track_add_audio_stream(track);
      desc = &(moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc);
      stream_priv = stream_create(&(moov->tracks[i]), BGAV_STREAM_AUDIO);
      
      stream_duration = stream_get_duration(stream_priv);
      if(ctx->tt->current_track->duration < stream_duration)
        ctx->tt->current_track->duration = stream_duration;
      bg_as->priv = stream_priv;
      
      bg_as->fourcc = desc->fourcc;
      bg_as->data.audio.format.num_channels = desc->format.audio.num_channels;
      bg_as->data.audio.format.samplerate = (int)(desc->format.audio.samplerate+0.5);
      bg_as->data.audio.bits_per_sample = desc->format.audio.bits_per_sample;
      if(desc->version == 1)
        {
        if(desc->format.audio.bytes_per_frame)
          bg_as->data.audio.block_align =
            desc->format.audio.bytes_per_frame;
        }

      /* Set mp4 extradata */
      
      if((moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc.has_esds) &&
         (moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc.esds.decoderConfigLen))
        {
        bg_as->ext_size =
          moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc.esds.decoderConfigLen;
        bg_as->ext_data =
          moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc.esds.decoderConfig;
        }
      else if(desc->format.audio.has_wave_atom)
        {
        bg_as->ext_data = desc->format.audio.wave_atom.data;
        bg_as->ext_size = desc->format.audio.wave_atom.size;
        }
      bg_as->stream_id = i;
            
      }
    /* Video stream */
    else if(moov->tracks[i].mdia.minf.has_vmhd)
      {
      //      fprintf(stderr, "Found video stream\n");
      bg_vs = bgav_track_add_video_stream(track);
      desc = &(moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc);
      stream_priv = stream_create(&(moov->tracks[i]), BGAV_STREAM_VIDEO);
      
      stream_duration = stream_get_duration(stream_priv);
      if(ctx->tt->current_track->duration < stream_duration)
        ctx->tt->current_track->duration = stream_duration;
      
      bg_vs->priv = stream_priv;
      
      bg_vs->fourcc = desc->fourcc;
      bg_vs->data.video.format.image_width = desc->format.video.width;
      bg_vs->data.video.format.image_height = desc->format.video.height;
      bg_vs->data.video.format.frame_width = desc->format.video.width;
      bg_vs->data.video.format.frame_height = desc->format.video.height;
      bg_vs->data.video.format.pixel_width = 1;
      bg_vs->data.video.format.pixel_height = 1;
      bg_vs->data.video.depth = desc->format.video.depth;
      
      bg_vs->data.video.format.timescale = moov->tracks[i].mdia.mdhd.time_scale;
      bg_vs->data.video.format.frame_duration =
        moov->tracks[i].mdia.minf.stbl.stts.entries[0].duration;

      if((moov->tracks[i].mdia.minf.stbl.stts.num_entries == 1) ||
         ((moov->tracks[i].mdia.minf.stbl.stts.num_entries == 2) &&
          (moov->tracks[i].mdia.minf.stbl.stts.entries[1].count == 1)))
        bg_vs->data.video.format.free_framerate = 0;
      else
        bg_vs->data.video.format.free_framerate = 1;
            
      bg_vs->data.video.palette_size = desc->format.video.ctab_size;
      if(bg_vs->data.video.palette_size)
        bg_vs->data.video.palette = desc->format.video.ctab;
      
      /* Set extradata suitable for Sorenson 3 */
      
      if(bg_vs->fourcc == BGAV_MK_FOURCC('S', 'V', 'Q', '3'))
        {
        ptr = moov->tracks[i].mdia.minf.stbl.stsd.entries[0].data + 82;
        atom_size = BGAV_PTR_2_32BE(ptr); ptr+=4;
        
        if((BGAV_PTR_2_FOURCC(ptr)) == BGAV_MK_FOURCC('S', 'M', 'I', ' '))
          {
          bg_vs->ext_size = 82 + atom_size;
          //          fprintf(stderr, "Found SMI Atom, %d bytes\n", atom_size);
          bg_vs->ext_data = moov->tracks[i].mdia.minf.stbl.stsd.entries[0].data;
          //          bgav_hexdump(bg_vs->ext_data, bg_vs->ext_size, 16);
          }
        }

      /* Set mp4 extradata */

      if((moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc.has_esds) &&
         (moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc.esds.decoderConfigLen))
        {
        bg_vs->ext_size =
          moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc.esds.decoderConfigLen;
        bg_vs->ext_data =
          moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc.esds.decoderConfig;
        }
        
      
      
      bg_vs->stream_id = i;
      }
    //    else
    //      fprintf(stderr, "Steam %d has an unhandled type\n", i);
    }
  set_metadata(ctx);
  }

static int open_quicktime(bgav_demuxer_context_t * ctx,
                          bgav_redirector_context_t ** redir)
  {
  qt_atom_header_t h;
  qt_priv_t * priv = (qt_priv_t*)0;
  int have_moov = 0;
  int have_mdat = 0;

  /* Create track */

  ctx->tt = bgav_track_table_create(1);
  
  /* Read moov atom */

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  while(!have_mdat || !have_moov)
    {
    if(!bgav_qt_atom_read_header(ctx->input, &h))
      return 0;
    switch(h.fourcc)
      {
      case BGAV_MK_FOURCC('m','d','a','t'):
        /* Reached the movie data atom, stop here */
        fprintf(stderr, "Found mdat atom\n");
        bgav_qt_atom_dump_header(&h);
        if(!have_mdat)
          {
          have_mdat = 1;
          priv->mdat_start = ctx->input->position;
          priv->mdat_size  = h.size - (ctx->input->position - h.start_position);
          }
        if(!have_moov)
          {
          /* Some files have the moov atom at the end */
          if(ctx->input->input->seek_byte)
            {
            bgav_qt_atom_skip(ctx->input, &h);
            }
          else
            {
            fprintf(stderr,
                    "Non streamable file on non seekable source\n");
            return 0;
            }
          }
        break;
      case BGAV_MK_FOURCC('m','o','o','v'):
        if(!bgav_qt_moov_read(&h, ctx->input, &(priv->moov)))
          return 0;
        have_moov = 1;
        break;
      default:
        bgav_qt_atom_skip(ctx->input, &h);
      }
    }
  quicktime_init(ctx);
  
  build_index(ctx);
  
  if((ctx->input->position != priv->mdat_start) &&
     (ctx->input->input->seek_byte))
    bgav_input_seek(ctx->input, priv->mdat_start, SEEK_SET);
  

  if(ctx->input->input->seek_byte)
    ctx->can_seek = 1;
  
  /* Skip until first chunk */

  if(priv->mdat_start < priv->packet_table[0].offset)
     bgav_input_skip(ctx->input,
                     priv->packet_table[0].offset -
                     priv->mdat_start);
  //  fprintf(stderr, "Start offset: %lld\n", ctx->input->position);

  ctx->stream_description = bgav_sprintf("Quicktime format");
  
  return 1;
  }

/*
 *  We process an entire chunk at once. If one chunk contains more than one
 *  video frame, split it into multiple packets
 */

static int next_packet_quicktime(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * stream;

  qt_priv_t * priv = (qt_priv_t*)(ctx->priv);
  stream_priv_t * sp;
  
  if((priv->current_packet >= priv->num_packets) ||
     (ctx->input->position >= priv->mdat_start + priv->mdat_size))
    return 0;
  
  stream =
    bgav_track_find_stream(ctx->tt->current_track,
                           priv->packet_table[priv->current_packet].stream_id);
  
  if(!stream) /* Skip unused stream */
    {
    //    fprintf(stderr, "Skipping %d bytes of unused stream Nr %d\n",
    //            priv->packet_table[priv->current_packet].size,
    //            priv->packet_table[priv->current_packet].stream_id);
    bgav_input_skip(ctx->input, priv->packet_table[priv->current_packet].size);
    priv->current_packet++;
    return 1;
    }

  sp = (stream_priv_t*)(stream->priv);

  if(priv->seeking && (sp->packet_index > priv->current_packet))
    {
    bgav_input_skip(ctx->input,
                    priv->packet_table[priv->current_packet].size);
    priv->current_packet++;
    return 1;
    }
  
  p = bgav_packet_buffer_get_packet_write(stream->packet_buffer);
  p->data_size = priv->packet_table[priv->current_packet].size;
  bgav_packet_alloc(p, p->data_size);
  
  p->timestamp_scaled = priv->packet_table[priv->current_packet].time;
  p->timestamp = (p->timestamp_scaled * GAVL_TIME_SCALE) / sp->trak->mdia.mdhd.time_scale;
  
  if(bgav_input_read_data(ctx->input, p->data, p->data_size) <
     p->data_size)
    return 0;
  bgav_packet_done_write(p);
  
  //  fprintf(stderr, "Current chunk: %d\n", priv->current_packet);
  priv->current_packet++;
  return 1;
  }

static void seek_quicktime(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  uint32_t i, j;
  uint32_t start_packet;
  uint32_t end_packet;
  int keep_going;
  stream_priv_t * s;
  bgav_track_t * track;
  int64_t time_scaled;
  
  //  fprintf(stderr, "Seek quicktime %f %lld\n", gavl_time_to_seconds(time), time);
  track = ctx->tt->current_track;
  
  qt_priv_t * priv = (qt_priv_t*)(ctx->priv);
  
  /* Set the packet indices of the streams to -1 */
  for(j = 0; j < track->num_audio_streams; j++)
    {
    s = (stream_priv_t*)(track->audio_streams[j].priv);
    s->packet_index = -1;
    }
  for(j = 0; j < track->num_video_streams; j++)
    {
    s = (stream_priv_t*)(track->video_streams[j].priv);
    s->packet_index = -1;
    }
    
  /* Seek the start chunks indices of all streams */

  i = priv->num_packets-1;
  keep_going = 1;
  
  while(keep_going)
    {
    keep_going = 0;
    for(j = 0; j < track->num_audio_streams; j++)
      {
      s = (stream_priv_t*)(track->audio_streams[j].priv);
      time_scaled = (time * s->trak->mdia.mdhd.time_scale)/GAVL_TIME_SCALE;
      if(s->packet_index < 0)
        {
        if((priv->packet_table[i].stream_id == track->audio_streams[j].stream_id) &&
           (priv->packet_table[i].keyframe) &&
           (priv->packet_table[i].time < time_scaled))
          {
          s->packet_index = i;
          track->audio_streams[j].time_scaled = priv->packet_table[i].time;
          track->audio_streams[j].time =
            (track->audio_streams[j].time_scaled *
             GAVL_TIME_SCALE)/s->trak->mdia.mdhd.time_scale;
          }
        else
          keep_going = 1;
        }
            
      }
    for(j = 0; j < track->num_video_streams; j++)
      {
      s = (stream_priv_t*)(track->video_streams[j].priv);
      time_scaled = (time * s->trak->mdia.mdhd.time_scale)/GAVL_TIME_SCALE;
      if(s->packet_index < 0)
        {
        if((priv->packet_table[i].stream_id == track->video_streams[j].stream_id) &&
           (priv->packet_table[i].keyframe) &&
           (priv->packet_table[i].time < time_scaled))
          {
          s->packet_index = i;

          track->video_streams[j].time_scaled = priv->packet_table[i].time;
          track->video_streams[j].time =
            (track->video_streams[j].time_scaled *
             GAVL_TIME_SCALE)/s->trak->mdia.mdhd.time_scale;
          }
        else
          keep_going = 1;
        }
      }
    i--;
    }

  /* Find the start and end packet */

  start_packet = ~0x0;
  end_packet   = 0x0;
  
  for(j = 0; j < track->num_audio_streams; j++)
    {
    s = (stream_priv_t*)(track->audio_streams[j].priv);
    if(start_packet > s->packet_index)
      start_packet = s->packet_index;
    if(end_packet < s->packet_index)
      end_packet = s->packet_index;
    }
  for(j = 0; j < track->num_video_streams; j++)
    {
    s = (stream_priv_t*)(track->video_streams[j].priv);
    if(start_packet > s->packet_index)
      start_packet = s->packet_index;
    if(end_packet < s->packet_index)
      end_packet = s->packet_index;
    }

  /* Do the seek */
  priv->current_packet = start_packet;
  bgav_input_seek(ctx->input, priv->packet_table[priv->current_packet].offset,
                  SEEK_SET);

  priv->seeking = 1;
  for(i = start_packet; i <= end_packet; i++)
    next_packet_quicktime(ctx);
  priv->seeking = 0;
  }

static void close_quicktime(bgav_demuxer_context_t * ctx)
  {
  int i;
  qt_priv_t * priv;

  bgav_track_t * track = ctx->tt->current_track;
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    if(track->audio_streams[i].priv)
      free(track->audio_streams[i].priv);
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    if(track->video_streams[i].priv)
      free(track->video_streams[i].priv);
    }
  
  priv = (qt_priv_t*)(ctx->priv);
  if(priv->packet_table)
    free(priv->packet_table);
  bgav_qt_moov_free(&(priv->moov));
  free(ctx->priv);
  }

bgav_demuxer_t bgav_demuxer_quicktime =
  {
    probe:       probe_quicktime,
    open:        open_quicktime,
    next_packet: next_packet_quicktime,
    seek:        seek_quicktime,
    close:       close_quicktime
  };

