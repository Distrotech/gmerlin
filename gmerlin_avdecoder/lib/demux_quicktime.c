/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <avdec_private.h>
#include <qt.h>
#include <utils.h>

#define LOG_DOMAIN "quicktime"

// #define DUMP_MOOV

typedef struct
  {
  qt_trak_t * trak;
  qt_stbl_t * stbl; /* For convenience */
  int64_t ctts_pos;
  int64_t stts_pos;
  int64_t stss_pos;
  int64_t stps_pos;
  int64_t stsd_pos;
  int64_t stsc_pos;
  int64_t stco_pos;
  int64_t stsz_pos;

  int64_t stts_count;
  int64_t ctts_count;
  int64_t stss_count;
  int64_t stsd_count;
  //  int64_t stsc_count;

  //  int64_t tics; /* Time in tics (depends on time scale of this stream) */
  //  int64_t total_tics;

  int skip_first_frame; /* Enabled only if the first frame has a different codec */
  int skip_last_frame; /* Enabled only if the last frame has a different codec */
  } stream_priv_t;

typedef struct
  {
  uint32_t ftyp_fourcc;
  qt_moov_t moov;

  stream_priv_t * streams;
  
  int seeking;

  int has_edl;
  /* Ohhhh, no, MPEG-4 can have multiple mdats... */
  
  int num_mdats;
  int mdats_alloc;
  int current_mdat;
  
  struct
    {
    int64_t start;
    int64_t size;
    } * mdats;
  } qt_priv_t;

/*
 *  We support all 3 types of quicktime audio encapsulation:
 *
 *  1: stsd version 0: Uncompressed audio
 *  2: stsd version 1: CBR encoded audio: Additional fields added
 *  3: stsd version 1, _Compression_id == -2: VBR audio, one "sample"
 *                     equals one frame of compressed audio data
 */

/* Intitialize everything */

static void stream_init(stream_priv_t * s, qt_trak_t * trak)
  {
  s->trak = trak;
  s->stbl = &(trak->mdia.minf.stbl);

  s->stts_pos = (s->stbl->stts.num_entries > 1) ? 0 : -1;
  s->ctts_pos = (s->stbl->has_ctts) ? 0 : -1;
  /* stsz_pos is -1 if all samples have the same size */
  s->stsz_pos = (s->stbl->stsz.sample_size) ? -1 : 0;
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

  switch(header)
    {
    case BGAV_MK_FOURCC('m','o','o','v'):
    case BGAV_MK_FOURCC('f','t','y','p'):
    case BGAV_MK_FOURCC('f','r','e','e'):
    case BGAV_MK_FOURCC('m','d','a','t'):
      return 1;
    }
  return 0;
  }


static int check_keyframe(stream_priv_t * s)
  {
  int ret;
  if(!s->stbl->stss.num_entries)
    return 1;
  if((s->stss_pos >= s->stbl->stss.num_entries) &&
     (s->stps_pos >= s->stbl->stps.num_entries))
    return 0;

  s->stss_count++;

  /* Try stts */
  if(s->stss_pos < s->stbl->stss.num_entries)
    {
    ret = (s->stbl->stss.entries[s->stss_pos] == s->stss_count) ? 1 : 0;
    if(ret)
      {
      s->stss_pos++;
      return ret;
      }
    }
  /* Try stps */
  if(s->stps_pos < s->stbl->stps.num_entries)
    {
    ret = (s->stbl->stps.entries[s->stps_pos] == s->stss_count) ? 1 : 0;
    if(ret)
      {
      s->stps_pos++;
      return ret;
      }
    }
  return ret;
  }

static void add_packet(bgav_demuxer_context_t * ctx,
                       qt_priv_t * priv,
                       bgav_stream_t * s,
                       int index,
                       int64_t offset,
                       int stream_id,
                       int64_t timestamp,
                       int keyframe,
                       int duration,
                       int chunk_size)
  {
  if(stream_id >= 0)
    bgav_superindex_add_packet(ctx->si, s,
                               offset, chunk_size,
                               stream_id, timestamp, keyframe, duration);
  
  if(index && !ctx->si->entries[index-1].size)
    {
    /* Check whether to advance the mdat */

    if(offset >= priv->mdats[priv->current_mdat].start +
       priv->mdats[priv->current_mdat].size)
      {
      if(!ctx->si->entries[index-1].size)
        {
        ctx->si->entries[index-1].size =
          priv->mdats[priv->current_mdat].start +
          priv->mdats[priv->current_mdat].size - ctx->si->entries[index-1].offset;
        }
      while(offset >= priv->mdats[priv->current_mdat].start +
            priv->mdats[priv->current_mdat].size)
        {
#if 0
        fprintf(stderr, "offset: %lld, cur: %d, .start = %lld, size: %lld\n",
                offset, priv->current_mdat,
                priv->mdats[priv->current_mdat].start,
                priv->mdats[priv->current_mdat].size);
#endif
        priv->current_mdat++;
        }
      }
    else
      {
      if(!ctx->si->entries[index-1].size)
        {
        ctx->si->entries[index-1].size =
        offset - ctx->si->entries[index-1].offset;
        }
      }
    }
  }

static void build_index(bgav_demuxer_context_t * ctx)
  {
  int i, j;
  int stream_id = 0;
  int64_t chunk_offset;
  int64_t * chunk_indices;
  stream_priv_t * s;
  qt_priv_t * priv;
  int num_packets = 0;
  bgav_stream_t * bgav_s;
  int chunk_samples;
  int packet_size;
  int duration;
  qt_trak_t * trak;
  int pts_offset;
  priv = (qt_priv_t *)(ctx->priv);

  /* 1 step: Count the total number of chunks */
  for(i = 0; i < priv->moov.num_tracks; i++)
    {
    trak = &priv->moov.tracks[i];
    if(trak->mdia.minf.has_vmhd) /* One video chunk can be more packets (=frames) */
      {
      num_packets += bgav_qt_trak_samples(&priv->moov.tracks[i]);
      if(priv->streams[i].skip_first_frame)
        num_packets--;
      if(priv->streams[i].skip_last_frame)
        num_packets--;
      }
    else if(trak->mdia.minf.has_smhd)
      {
      /* Some audio frames will be read as "samples" (-> VBR audio!) */
      if(!trak->mdia.minf.stbl.stsz.sample_size)
        num_packets += bgav_qt_trak_samples(trak);
      else /* Other packets (uncompressed) will be complete quicktime chunks */
        num_packets += bgav_qt_trak_chunks(trak);
      }
    else if(!strncmp((char*)trak->mdia.minf.stbl.stsd.entries[0].data, "text", 4) ||
            !strncmp((char*)trak->mdia.minf.stbl.stsd.entries[0].data, "tx3g", 4))
      {
      num_packets += bgav_qt_trak_samples(trak);
      }
    else // For other tracks, we count entire chunks
      {
      num_packets += bgav_qt_trak_chunks(&priv->moov.tracks[i]);
      }
    }
  if(!num_packets)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "No packets in movie");
    return;
    }
  ctx->si = bgav_superindex_create(num_packets);
  
  chunk_indices = calloc(priv->moov.num_tracks, sizeof(*chunk_indices));

  /* Skip empty mdats */

  while(!priv->mdats[priv->current_mdat].size)
    priv->current_mdat++;
  
  i = 0;

  while(i < num_packets)
    {
    /* Find the stream with the lowest chunk offset */

    chunk_offset = 9223372036854775807LL; /* Maximum */
    
    for(j = 0; j < priv->moov.num_tracks; j++)
      {
      if((priv->streams[j].stco_pos <
          priv->moov.tracks[j].mdia.minf.stbl.stco.num_entries) &&
         (priv->moov.tracks[j].mdia.minf.stbl.stco.entries[priv->streams[j].stco_pos] < chunk_offset))
        {
        stream_id = j;
        chunk_offset = priv->moov.tracks[j].mdia.minf.stbl.stco.entries[priv->streams[j].stco_pos];
        }
      }

    
    //    if(j == priv->moov.num_tracks)
    //      return;
    
    bgav_s = bgav_track_find_stream_all(ctx->tt->cur, stream_id);

    if(bgav_s && (bgav_s->type == BGAV_STREAM_AUDIO))
      {
      s = (stream_priv_t*)(bgav_s->priv);

      if(!s->stbl->stsz.sample_size)
        {
        /* Read single packets of a chunk. We do this, when the stsz atom has more than
           zero entries */

        for(j = 0; j < s->stbl->stsc.entries[s->stsc_pos].samples_per_chunk; j++)
          {
          packet_size   = s->stbl->stsz.entries[s->stsz_pos];
          s->stsz_pos++;
          
          chunk_samples = (s->stts_pos >= 0) ?
            s->stbl->stts.entries[s->stts_pos].duration :
            s->stbl->stts.entries[0].duration;

          add_packet(ctx,
                     priv,
                     bgav_s,
                     i, chunk_offset,
                     stream_id,
                     bgav_s->duration,
                     check_keyframe(s), chunk_samples, packet_size);
          
          bgav_s->duration += chunk_samples;
          chunk_offset += packet_size;
          /* Advance stts */
          if(s->stts_pos >= 0)
            {
            s->stts_count++;
            if(s->stts_count >= s->stbl->stts.entries[s->stts_pos].count)
              {
              s->stts_pos++;
              s->stts_count = 0;
              }
            }
          
          i++;
          }
        }
      else
        {
        /* Usual case: We have to guess the chunk size from subsequent chunks */
        if(s->stts_pos >= 0)
          {
          chunk_samples =
            s->stbl->stts.entries[s->stts_pos].duration *
            s->stbl->stsc.entries[s->stsc_pos].samples_per_chunk;
          }
        else
          {
          chunk_samples =
            s->stbl->stts.entries[0].duration *
            s->stbl->stsc.entries[s->stsc_pos].samples_per_chunk;
          }
      
        add_packet(ctx,
                   priv,
                   bgav_s,
                   i, chunk_offset,
                   stream_id,
                   bgav_s->duration,
                   check_keyframe(s), chunk_samples, 0);
        /* Time to sample */
        bgav_s->duration += chunk_samples;
        if(s->stts_pos >= 0)
          {
          s->stts_count++;
          if(s->stts_count >= s->stbl->stts.entries[s->stts_pos].count)
            {
            s->stts_pos++;
            s->stts_count = 0;
            }
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
    else if(bgav_s && (bgav_s->type == BGAV_STREAM_VIDEO))
      {
      s = (stream_priv_t*)(bgav_s->priv);
      for(j = 0; j < s->stbl->stsc.entries[s->stsc_pos].samples_per_chunk; j++)
        {
        packet_size = (s->stsz_pos >= 0) ? s->stbl->stsz.entries[s->stsz_pos]:
          s->stbl->stsz.sample_size;

        /* Sample size */
        if(s->stsz_pos >= 0)
          s->stsz_pos++;

        if(s->stts_pos >= 0)
          duration = s->stbl->stts.entries[s->stts_pos].duration;
        else
          duration = s->stbl->stts.entries[0].duration;

        /*
         *  We must make sure, that the pts starts at 0. This means, that
         *  ISO compliant values will be shifted to the (wrong)
         *  values produced by Apple Quicktime
         */
        
        if(s->ctts_pos >= 0)
          pts_offset =
            (int32_t)s->stbl->ctts.entries[s->ctts_pos].duration;
        else
          pts_offset = 0;
        
        if((s->skip_first_frame && !s->stco_pos) ||
           (s->skip_last_frame &&
            (s->stco_pos == s->stbl->stco.num_entries)))
          {
          add_packet(ctx,
                     priv,
                     bgav_s,
                     i, chunk_offset,
                     -1,
                     bgav_s->duration + pts_offset,
                     check_keyframe(s),
                     duration,
                     packet_size);
          }
        else
          {
          add_packet(ctx,
                     priv,
                     bgav_s,
                     i, chunk_offset,
                     stream_id,
                     bgav_s->duration + pts_offset,
                     check_keyframe(s),
                     duration,
                     packet_size);
          i++;
          }
        chunk_offset += packet_size;

        bgav_s->duration += duration;

        /* Time to sample */
        if(s->stts_pos >= 0)
          {
          s->stts_count++;
          if(s->stts_count >= s->stbl->stts.entries[s->stts_pos].count)
            {
            s->stts_pos++;
            s->stts_count = 0;
            }
          }
        /* Composition time to sample */
        if(s->ctts_pos >= 0)
          {
          s->ctts_count++;
          if(s->ctts_count >= s->stbl->ctts.entries[s->ctts_pos].count)
            {
            s->ctts_pos++;
            s->ctts_count = 0;
            }
          }
        }
      s->stco_pos++;
      /* Update sample to chunk */
      if(s->stsc_pos < s->stbl->stsc.num_entries - 1)
        {
        if(s->stbl->stsc.entries[s->stsc_pos+1].first_chunk - 1 == s->stco_pos)
          s->stsc_pos++;
        }
      }
    else if(bgav_s && (bgav_s->type == BGAV_STREAM_SUBTITLE_TEXT))
      {
      s = (stream_priv_t*)(bgav_s->priv);
      
      /* Read single samples of a chunk */
      
      for(j = 0; j < s->stbl->stsc.entries[s->stsc_pos].samples_per_chunk; j++)
        {
        packet_size   = s->stbl->stsz.entries[s->stsz_pos];
        s->stsz_pos++;

        if(s->stts_pos >= 0)
          duration = s->stbl->stts.entries[s->stts_pos].duration;
        else
          duration = s->stbl->stts.entries[0].duration;

        
        add_packet(ctx,
                   priv,
                   bgav_s,
                   i, chunk_offset,
                   stream_id,
                   bgav_s->duration,
                   check_keyframe(s), duration,
                   packet_size);
        
        chunk_offset += packet_size;

        bgav_s->duration += duration;
        
        /* Time to sample */
        if(s->stts_pos >= 0)
          {
          s->stts_count++;
          if(s->stts_count >= s->stbl->stts.entries[s->stts_pos].count)
            {
            s->stts_pos++;
            s->stts_count = 0;
            }
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
      add_packet(ctx, priv, (bgav_stream_t*)0, i, chunk_offset, stream_id, -1, 0, 0, 0);
      i++;
      priv->streams[stream_id].stco_pos++;
      }
    }
  /* Set the final packet size to the end of the mdat */
  ctx->si->entries[ctx->si->num_entries-1].size =
    priv->mdats[priv->current_mdat].start +
  priv->mdats[priv->current_mdat].size -
    ctx->si->entries[ctx->si->num_entries-1].offset;
  
  free(chunk_indices);
  }

#define SET_UDTA_STRING(dst, src) \
if(!(ctx->tt->cur->metadata.dst) && moov->udta.src)\
  {                                                                     \
  if(moov->udta.have_ilst) \
    ctx->tt->cur->metadata.dst = bgav_strdup(moov->udta.src); \
  else \
    ctx->tt->cur->metadata.dst = bgav_convert_string(cnv, moov->udta.src, -1, NULL);\
  }

#define SET_UDTA_INT(dst, src) \
  if(!(ctx->tt->cur->metadata.dst) && moov->udta.src && isdigit(*(moov->udta.src))) \
    { \
    ctx->tt->cur->metadata.dst = atoi(moov->udta.src);\
    }

static void set_metadata(bgav_demuxer_context_t * ctx)
  {
  qt_priv_t * priv;
  qt_moov_t * moov;
  
  bgav_charset_converter_t * cnv = (bgav_charset_converter_t *)0;
  
  priv = (qt_priv_t*)(ctx->priv);
  moov = &(priv->moov);

  if(!moov->udta.have_ilst)
    cnv = bgav_charset_converter_create(ctx->opt, "ISO-8859-1", "UTF-8");
    
  
  SET_UDTA_STRING(artist,    ART);
  SET_UDTA_STRING(title,     nam);
  SET_UDTA_STRING(album,     alb);
  SET_UDTA_STRING(genre,     gen);
  SET_UDTA_STRING(copyright, cpy);
  SET_UDTA_INT(track,     trk);
  SET_UDTA_STRING(comment,   cmt);
  SET_UDTA_STRING(comment,   inf);
  SET_UDTA_STRING(author,    aut);

  if(!ctx->tt->cur->metadata.track && moov->udta.trkn)
    ctx->tt->cur->metadata.track = moov->udta.trkn;

  if(cnv)
    bgav_charset_converter_destroy(cnv);

  //  bgav_metadata_dump(&ctx->tt->cur->metadata);
  }

/*
 *  This struct MUST match the channel locations assumed by
 *  ffmpeg (libavcodec/mpegaudiodec.c: mp3Frames[16], mp3Channels[16], chan_offset[9][5])
 */

static const struct
  {
  int num_channels;
  gavl_channel_id_t channels[8];
  }
mp3on4_channels[] =
  {
    { 0 }, /* Custom */
    { 1, { GAVL_CHID_FRONT_CENTER } }, // C
    { 2, { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT } }, // FLR
    { 3, { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT, GAVL_CHID_FRONT_CENTER } },
    { 4, { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT,
           GAVL_CHID_FRONT_CENTER, GAVL_CHID_REAR_CENTER } }, // C FLR BS
    { 5, { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT,
           GAVL_CHID_REAR_LEFT, GAVL_CHID_REAR_RIGHT,
           GAVL_CHID_FRONT_CENTER } }, // C FLR BLRS
    { 6, { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT,
           GAVL_CHID_REAR_LEFT, GAVL_CHID_REAR_RIGHT,
           GAVL_CHID_FRONT_CENTER, GAVL_CHID_LFE } }, // C FLR BLRS LFE a.k.a 5.1
    { 8, { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT,
           GAVL_CHID_SIDE_LEFT, GAVL_CHID_SIDE_RIGHT,
           GAVL_CHID_FRONT_CENTER, GAVL_CHID_LFE,
           GAVL_CHID_REAR_LEFT, GAVL_CHID_REAR_RIGHT } }, // C FLR BLRS BLR LFE a.k.a 7.1 
    { 4, { GAVL_CHID_FRONT_LEFT, GAVL_CHID_FRONT_RIGHT,
           GAVL_CHID_REAR_LEFT, GAVL_CHID_REAR_RIGHT } }, // FLR BLRS (Quadrophonic)
  };


static int init_mp3on4(bgav_stream_t * s)
  {
  int channel_config;
  s->fourcc = BGAV_MK_FOURCC('m', '4', 'a', 29);
  
  channel_config = (s->ext_data[1] >> 3) & 0x0f;
  if(!channel_config || (channel_config > 8))
    return 0;
  s->data.audio.format.num_channels = mp3on4_channels[channel_config].num_channels;
  memcpy(s->data.audio.format.channel_locations,
         mp3on4_channels[channel_config].channels,
         s->data.audio.format.num_channels * sizeof(mp3on4_channels[channel_config].channels[0]));
  return 1;
  }

/* the audio fourcc mp4a doesn't necessarily mean, that we actually
   have AAC audio */

static const struct
  {
  int objectTypeId;
  uint32_t fourcc;
  }
audio_object_ids[] =
  {
    { 105, BGAV_MK_FOURCC('.','m','p','3') },
    { 107, BGAV_MK_FOURCC('.','m','p','2') }
  };

static void set_audio_from_esds(bgav_stream_t * s, qt_esds_t * esds)
  {
  int i;
  for(i = 0; i < sizeof(audio_object_ids)/sizeof(audio_object_ids[0]); i++)
    {
    if(audio_object_ids[i].objectTypeId == esds->objectTypeId)
      {
      s->fourcc = audio_object_ids[i].fourcc;
      return;
      }
    }
  }

static void process_packet_subtitle_qt(bgav_stream_t * s, bgav_packet_t * p)
  {
  int i;
  uint16_t len;


  len = BGAV_PTR_2_16BE(p->data);

  if(!len)
    {
    *(p->data) = '\0'; // Empty string
    p->data_size = 1;
    }
  else
    {
    memmove(p->data, p->data+2, len);

    /* De-Macify linebreaks */
    for(i = 0; i < len; i++)
      {
      if(p->data[i] == '\r')
        p->data[i] = '\n';
      }
    }
  p->data_size = len;
  // p->duration = -1;
  }

static void process_packet_subtitle_tx3g(bgav_stream_t * s, bgav_packet_t * p)
  {
  //  int i;
  uint16_t len;

  len = BGAV_PTR_2_16BE(p->data);
  
  if(len)
    {
    memmove(p->data, p->data+2, len);
    p->data_size = len;
    }
  else
    {
    *(p->data) = '\0'; // Empty string
    p->data_size = 1;
    }
  //  p->duration = -1;

#if 0  
  /* De-Macify linebreaks */
  for(i = 0; i < len; i++)
    {
    if(p->data[i] == '\r')
      p->data[i] = '\n';
    }
#endif
  }

static void setup_chapter_track(bgav_demuxer_context_t * ctx, qt_trak_t * trak)
  {
  int64_t old_pos;
  int64_t pos;
  uint8_t * buffer = (uint8_t *)0;
  int buffer_alloc = 0;
  
  int total_chapters;

  int chunk_index;
  int stts_index;
  int stts_count;
  int stsc_index;
  int stsc_count;
  int64_t tics = 0;
  int i;
  qt_stts_t * stts;
  qt_stsc_t * stsc;
  qt_stsz_t * stsz;
  qt_stco_t * stco;
  uint32_t len;
  bgav_charset_converter_t * cnv;
  const char * charset;
  
  if(!ctx->input->input->seek_byte)
    {
    bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Chapters detected but stream is not seekable");
    return;
    }
  if(ctx->tt->cur->chapter_list)
    {
    bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "More than one chapter track, choosing first");
    return;
    }

  old_pos = ctx->input->position;

  if(trak->mdia.minf.stbl.stsd.entries[0].desc.fourcc == BGAV_MK_FOURCC('t','x','3','g'))
    charset = "bgav_unicode";
  else
    charset = bgav_qt_get_charset(trak->mdia.mdhd.language);

  if(charset)
    cnv = bgav_charset_converter_create(ctx->opt, charset, "UTF-8");
  else
    {
    cnv = (bgav_charset_converter_t*)0;
    bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Unknown encoding for chapter names");
    }
  
  total_chapters = bgav_qt_trak_samples(trak);
  ctx->tt->cur->chapter_list =
    bgav_chapter_list_create(trak->mdia.mdhd.time_scale, total_chapters);
  
  chunk_index = 0;
  stts_index = 0;
  stts_count = 0;
  stsc_index = 0;
  stsc_count = 0;

  stts = &trak->mdia.minf.stbl.stts;
  stsc = &trak->mdia.minf.stbl.stsc;
  stsz = &trak->mdia.minf.stbl.stsz;
  stco = &trak->mdia.minf.stbl.stco;
  
  pos = stco->entries[chunk_index];
  
  for(i = 0; i < total_chapters; i++)
    {
    ctx->tt->cur->chapter_list->chapters[i].time = tics;

    /* Increase tics */
    tics += stts->entries[stts_index].duration;
    stts_count++;
    if(stts_count >= stts->entries[stts_index].count)
      {
      stts_index++;
      stts_count = 0;
      }
    
    /* Read sample */
    if(stsz->entries[i] > buffer_alloc)
      {
      buffer_alloc = stsz->entries[i] + 128;
      buffer = realloc(buffer, buffer_alloc);
      }
    bgav_input_seek(ctx->input, pos, SEEK_SET);
    if(bgav_input_read_data(ctx->input, buffer, stsz->entries[i]) < stsz->entries[i])
      {
      bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "Read error while setting up chapter list");
      return;
      }
    /* Set chapter name */

    len = BGAV_PTR_2_16BE(buffer);
    if(len)
      {
      ctx->tt->cur->chapter_list->chapters[i].name =
        bgav_convert_string(cnv, (char*)(buffer+2), len, (int*)0);
      }
    /* Increase file position */
    pos += stsz->entries[i];
    stsc_count++;
    if(stsc_count >= stsc->entries[stsc_index].samples_per_chunk)
      {
      chunk_index++;
      if((stsc_index < stsc->num_entries-1) &&
         (chunk_index +1 >= stsc->entries[stsc_index+1].first_chunk))
        {
        stsc_index++;
        }
      stsc_count = 0;
      pos = stco->entries[chunk_index];
      }
    }
  
  if(buffer) free(buffer);
  bgav_input_seek(ctx->input, old_pos, SEEK_SET);
  }

static void quicktime_init(bgav_demuxer_context_t * ctx)
  {
  int i, j;
  uint32_t atom_size, fourcc;
  bgav_stream_t * bg_as;
  bgav_stream_t * bg_vs;
  bgav_stream_t * bg_ss;
  stream_priv_t * stream_priv;
  qt_sample_description_t * desc;
  bgav_track_t * track;
  int skip_first_frame = 0;
  int skip_last_frame = 0;
  qt_priv_t * priv = (qt_priv_t*)(ctx->priv);
  qt_moov_t * moov = &(priv->moov);

  qt_trak_t * trak;
  qt_stsd_t * stsd;
  
  
  track = ctx->tt->cur;
  
  ctx->tt->cur->duration = 0;

  priv->streams = calloc(moov->num_tracks, sizeof(*(priv->streams)));
  
  
  for(i = 0; i < moov->num_tracks; i++)
    {
    trak = &moov->tracks[i];
    stsd = &trak->mdia.minf.stbl.stsd;
    /* Audio stream */
    if(trak->mdia.minf.has_smhd)
      {
      if(!stsd->entries)
        {
        continue;
        }
      bg_as = bgav_track_add_audio_stream(track, ctx->opt);

      if(trak->edts.elst.num_entries > 1)
        priv->has_edl = 1;
      
      bgav_qt_mdhd_get_language(&trak->mdia.mdhd,
                                bg_as->language);
      
      desc = &(stsd->entries[0].desc);

      stream_priv = &(priv->streams[i]);
      stream_init(stream_priv, &(moov->tracks[i]));
      
      bg_as->priv = stream_priv;
      
      bg_as->timescale = trak->mdia.mdhd.time_scale;
      bg_as->fourcc    = desc->fourcc;
      bg_as->data.audio.format.num_channels = desc->format.audio.num_channels;
      bg_as->data.audio.format.samplerate = (int)(desc->format.audio.samplerate+0.5);
      bg_as->data.audio.bits_per_sample = desc->format.audio.bits_per_sample;
      if(desc->version == 1)
        {
        if(desc->format.audio.bytes_per_frame)
          bg_as->data.audio.block_align =
            desc->format.audio.bytes_per_frame;
        }

      /* Set channel configuration (if present) */

      if(desc->format.audio.has_chan)
        {
        bgav_qt_chan_get(&(desc->format.audio.chan),
                         &(bg_as->data.audio.format));
        }
      
      /* Set mp4 extradata */
      
      if(desc->has_esds)
        {
        bg_as->ext_size = desc->esds.decoderConfigLen;
        bg_as->ext_data = desc->esds.decoderConfig;
        
        /* Check for mp3on4 */
        if((desc->esds.objectTypeId == 64) &&
           (desc->esds.decoderConfigLen >= 2) &&
           (bg_as->ext_data[0] >> 3 == 29))
          {
          if(!init_mp3on4(bg_as))
            {
            bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                     "Invalid mp3on4 channel configuration");
            bg_as->fourcc = 0;
            }
          }
        else
          set_audio_from_esds(bg_as, &desc->esds);
        }
      else if(bg_as->fourcc == BGAV_MK_FOURCC('l', 'p', 'c', 'm'))
        {
        /* Quicktime 7 lpcm: extradata contains formatSpecificFlags in native byte order */
        bg_as->ext_data = (uint8_t*)(&(desc->format.audio.formatSpecificFlags));
        bg_as->ext_size = sizeof(desc->format.audio.formatSpecificFlags);
        }
      else if(desc->format.audio.has_wave)
        {
        if((desc->format.audio.wave.has_esds) &&
           (desc->format.audio.wave.esds.decoderConfigLen))
          {
          bg_as->ext_size = desc->format.audio.wave.esds.decoderConfigLen;
          bg_as->ext_data = desc->format.audio.wave.esds.decoderConfig;
          }
        else if(desc->format.audio.wave.num_user_atoms)
          {
          for(j = 0; j < desc->format.audio.wave.num_user_atoms; j++)
            {
            atom_size = BGAV_PTR_2_32BE(desc->format.audio.wave.user_atoms[j]);
            fourcc = BGAV_PTR_2_FOURCC(desc->format.audio.wave.user_atoms[j]+4);

            switch(fourcc)
              {
              case BGAV_MK_FOURCC('O','V','H','S'):
              case BGAV_MK_FOURCC('a','l','a','c'):
              case BGAV_MK_FOURCC('g','l','b','l'):
                bg_as->ext_size = atom_size;
                bg_as->ext_data = desc->format.audio.wave.user_atoms[j];
                break;
              default:
                break;
              }
            if(bg_as->ext_data)
              break;
            }
          }
        
        if(!bg_as->ext_size)
          {
          /* Raw wave atom needed by win32 decoders (QDM2) */
          bg_as->ext_data = desc->format.audio.wave.raw;
          bg_as->ext_size = desc->format.audio.wave.raw_size;
          }
        }
      else if(desc->has_glbl)
        {
        bg_as->ext_data = desc->glbl.data;
        bg_as->ext_size = desc->glbl.size;
        }
      
      bg_as->stream_id = i;
      

      /* Check endianess */

      if(desc->format.audio.wave.has_enda &&
         desc->format.audio.wave.enda.littleEndian)
        {
        bg_as->data.audio.endianess = BGAV_ENDIANESS_LITTLE;
        }
      else
        bg_as->data.audio.endianess = BGAV_ENDIANESS_BIG;
      
      }
    /* Video stream */
    else if(trak->mdia.minf.has_vmhd)
      {
      skip_first_frame = 0;
      skip_last_frame = 0;

      if(!stsd->entries)
        continue;
      
      if(stsd->num_entries > 1)
        {
        if((trak->mdia.minf.stbl.stsc.num_entries >= 2) &&
           (trak->mdia.minf.stbl.stsc.entries[0].samples_per_chunk == 1) &&
           (trak->mdia.minf.stbl.stsc.entries[1].sample_description_id == 2) &&
           (trak->mdia.minf.stbl.stsc.entries[1].first_chunk == 2))
          {
          skip_first_frame = 1;
          }
        if((trak->mdia.minf.stbl.stsc.num_entries >= 2) &&
           (trak->mdia.minf.stbl.stsc.entries[trak->mdia.minf.stbl.stsc.num_entries-1].samples_per_chunk == 1) &&
           (trak->mdia.minf.stbl.stsc.entries[trak->mdia.minf.stbl.stsc.num_entries-1].sample_description_id == stsd->num_entries) &&
           (trak->mdia.minf.stbl.stsc.entries[trak->mdia.minf.stbl.stsc.num_entries-1].first_chunk == trak->mdia.minf.stbl.stco.num_entries))
          {
          skip_last_frame = 1;
          }

        if(stsd->num_entries > 1 + skip_first_frame + skip_last_frame)
          continue;
        }
      
      bg_vs = bgav_track_add_video_stream(track, ctx->opt);

      if(trak->edts.elst.num_entries > 1)
        priv->has_edl = 1;
            
      desc = &(stsd->entries[skip_first_frame].desc);
      stream_priv = &(priv->streams[i]);
      
      stream_init(stream_priv, &(moov->tracks[i]));

      if(skip_first_frame)
        stream_priv->skip_first_frame = 1;
      if(skip_last_frame)
        stream_priv->skip_last_frame = 1;
      
      bg_vs->priv = stream_priv;
      
      bg_vs->fourcc = desc->fourcc;
      bg_vs->data.video.format.image_width = desc->format.video.width;
      bg_vs->data.video.format.image_height = desc->format.video.height;
      bg_vs->data.video.format.frame_width = desc->format.video.width;
      bg_vs->data.video.format.frame_height = desc->format.video.height;

      if(desc->format.video.has_pasp)
        {
        bg_vs->data.video.format.pixel_width = desc->format.video.pasp.hSpacing;
        bg_vs->data.video.format.pixel_height = desc->format.video.pasp.vSpacing;
        }
      else
        {
        bg_vs->data.video.format.pixel_width = 1;
        bg_vs->data.video.format.pixel_height = 1;
        }
      if(desc->format.video.has_fiel)
        {
        if(desc->format.video.fiel.fields == 2)
          {
          if((desc->format.video.fiel.detail == 14) || (desc->format.video.fiel.detail == 6))
            bg_vs->data.video.format.interlace_mode = GAVL_INTERLACE_BOTTOM_FIRST;
          else if((desc->format.video.fiel.detail == 9) || (desc->format.video.fiel.detail == 1))
            bg_vs->data.video.format.interlace_mode = GAVL_INTERLACE_TOP_FIRST;
          }
        }
      bg_vs->data.video.depth = desc->format.video.depth;
      
      bg_vs->data.video.format.timescale = trak->mdia.mdhd.time_scale;

      /* We set the timescale here, because we need it before the dmuxer sets it. */

      bg_vs->timescale = trak->mdia.mdhd.time_scale;
      
      bg_vs->data.video.format.frame_duration =
        trak->mdia.minf.stbl.stts.entries[0].duration;

      if((trak->mdia.minf.stbl.stts.num_entries == 1) ||
         ((trak->mdia.minf.stbl.stts.num_entries == 2) &&
          (trak->mdia.minf.stbl.stts.entries[1].count == 1)))
        bg_vs->data.video.format.framerate_mode = GAVL_FRAMERATE_CONSTANT;
      else
        {
        bg_vs->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
        bg_vs->data.video.frametime_mode = BGAV_FRAMETIME_PACKET;
        }
      bg_vs->data.video.palette_size = desc->format.video.ctab_size;
      if(bg_vs->data.video.palette_size)
        bg_vs->data.video.palette = desc->format.video.ctab;
      
      /* Set extradata suitable for Sorenson 3 */
      
      if(bg_vs->fourcc == BGAV_MK_FOURCC('S', 'V', 'Q', '3'))
        {
        if(stsd->entries[skip_first_frame].desc.format.video.has_SMI)
          {
          bg_vs->ext_size = stsd->entries[0].data_size;
          bg_vs->ext_data = stsd->entries[0].data;
          }
        }
      else if((bg_vs->fourcc == BGAV_MK_FOURCC('a', 'v', 'c', '1')) &&
              (stsd->entries[0].desc.format.video.avcC_offset))
        {
        bg_vs->ext_data = stsd->entries[skip_first_frame].data +
          stsd->entries[0].desc.format.video.avcC_offset;
        bg_vs->ext_size = stsd->entries[skip_first_frame].desc.format.video.avcC_size;
        }
      
      /* Set mp4 extradata */

      if((stsd->entries[skip_first_frame].desc.has_esds) &&
         (stsd->entries[skip_first_frame].desc.esds.decoderConfigLen))
        {
        bg_vs->ext_size =
          stsd->entries[skip_first_frame].desc.esds.decoderConfigLen;
        bg_vs->ext_data =
          stsd->entries[skip_first_frame].desc.esds.decoderConfig;
        }
      else if(desc->has_glbl)
        {
        bg_vs->ext_data = desc->glbl.data;
        bg_vs->ext_size = desc->glbl.size;
        }
      bg_vs->stream_id = i;

      }
    /* Quicktime subtitles */
    else if(stsd->entries[0].desc.fourcc == BGAV_MK_FOURCC('t','e','x','t'))
      {
      const char * charset;
      if(!stsd->entries)
        continue;

      if(bgav_qt_is_chapter_track(moov, trak))
      //      if(0)
        {
        setup_chapter_track(ctx, trak);
        }
      else
        {
        charset = bgav_qt_get_charset(trak->mdia.mdhd.language);
        
        /* TODO: Quicktime text subtitles can also be chapter tracks!! */
        
        bg_ss =
          bgav_track_add_subtitle_stream(track, ctx->opt, 1, charset);

        if(trak->edts.elst.num_entries > 1)
          priv->has_edl = 1;
        
        bg_ss->description = bgav_sprintf("Quicktime subtitles");
        bg_ss->fourcc = stsd->entries[0].desc.fourcc;
        
        bgav_qt_mdhd_get_language(&trak->mdia.mdhd,
                                  bg_ss->language);
        
        bg_ss->timescale = trak->mdia.mdhd.time_scale;
        bg_ss->stream_id = i;
        
        stream_priv = &(priv->streams[i]);
        stream_init(stream_priv, &(moov->tracks[i]));
        bg_ss->priv = stream_priv;
        bg_ss->process_packet = process_packet_subtitle_qt;
        }
      }
    /* MPEG-4 subtitles (3gpp timed text?) */
    else if(stsd->entries[0].desc.fourcc == BGAV_MK_FOURCC('t','x','3','g'))
      {
      if(!stsd->entries)
        continue;

      if(bgav_qt_is_chapter_track(moov, trak))
        setup_chapter_track(ctx, trak);
      else
        {
        bg_ss =
          bgav_track_add_subtitle_stream(track, ctx->opt, 1, "bgav_unicode");

        if(trak->edts.elst.num_entries > 1)
          priv->has_edl = 1;
        
        bg_ss->description = bgav_sprintf("3gpp subtitles");
        bg_ss->fourcc = stsd->entries[0].desc.fourcc;
        bgav_qt_mdhd_get_language(&trak->mdia.mdhd,
                                  bg_ss->language);
      
        bg_ss->timescale = trak->mdia.mdhd.time_scale;
        bg_ss->stream_id = i;

        stream_priv = &(priv->streams[i]);
        stream_init(stream_priv, &(moov->tracks[i]));
        bg_ss->priv = stream_priv;
        bg_ss->process_packet = process_packet_subtitle_tx3g;
        }
      }
    }
  
  set_metadata(ctx);
  
  }

static int handle_rmra(bgav_demuxer_context_t * ctx)
  {
  char * basename = (char*)0, *pos;
  
  int i, index;
  qt_priv_t * priv = (qt_priv_t*)(ctx->priv);
  int num_urls = 0;
  for(i = 0; i < priv->moov.rmra.num_rmda; i++)
    {
    if(priv->moov.rmra.rmda[i].rdrf.fourcc == BGAV_MK_FOURCC('u','r','l',' '))
      num_urls++;
    }

  ctx->redirector           = calloc(1, sizeof(*ctx->redirector));
  ctx->redirector->num_urls = num_urls;
  ctx->redirector->opt      = ctx->opt;
  ctx->redirector->urls     = calloc(num_urls, sizeof(*(ctx->redirector->urls)));

  /* Some urls are relative urls */
  if(ctx->input->url)
    basename = bgav_strdup(ctx->input->url);
  else if(ctx->input->filename)
    basename = bgav_strdup(ctx->input->filename);
  
  if(!basename)
    return 0;

  pos = strrchr(basename, '/');
  if(!pos)
    *basename = '\0';
  else
    {
    pos++;
    *pos = '\0';
    }
  
  index = 0;
  for(i = 0; i < priv->moov.rmra.num_rmda; i++)
    {
    if(priv->moov.rmra.rmda[i].rdrf.fourcc == BGAV_MK_FOURCC('u','r','l',' '))
      {
      /* Absolute url */
      if(strstr((char*)priv->moov.rmra.rmda[i].rdrf.data_ref, "://"))
        {
        ctx->redirector->urls[index].url =
          bgav_strdup((char*)priv->moov.rmra.rmda[i].rdrf.data_ref);
        
        }
      /* Relative url */
      else
        {
        ctx->redirector->urls[index].url = bgav_strdup(basename);
        ctx->redirector->urls[index].url =
          bgav_strncat(ctx->redirector->urls[index].url,
                       (char*)priv->moov.rmra.rmda[i].rdrf.data_ref,
                       (char*)0);
        }
      index++;
      }
    }
  return 1;
  }

static void set_stream_edl(qt_priv_t * priv, bgav_stream_t * s,
                      bgav_edl_stream_t * es)
  {
  int i;
  qt_elst_t * elst;
  stream_priv_t * sp;
  int64_t duration = 0;
  bgav_edl_segment_t * seg;

  int mdhd_ts, mvhd_ts;
  
  sp = (stream_priv_t *)s->priv;
  elst = &sp->trak->edts.elst;

  mvhd_ts = priv->moov.mvhd.time_scale;
  mdhd_ts = sp->trak->mdia.mdhd.time_scale;

  es->timescale = mdhd_ts;
  
  for(i = 0; i < elst->num_entries; i++)
    {
    if((int32_t)elst->table[i].media_time > -1)
      {
      seg = bgav_edl_add_segment(es);
      seg->timescale    = mdhd_ts;
      seg->src_time     = elst->table[i].media_time;
      seg->dst_time     = duration;
      seg->dst_duration = gavl_time_rescale(mvhd_ts, mdhd_ts,
                                            elst->table[i].duration);

      seg->speed_num = elst->table[i].media_rate;
      seg->speed_den = 65536;
      
      duration += seg->dst_duration;
      }
    else
      duration += gavl_time_rescale(mvhd_ts, mdhd_ts,
                                    elst->table[i].duration);
    }
  
  }

static void build_edl(bgav_demuxer_context_t * ctx)
  {
  bgav_edl_stream_t * es;
  bgav_edl_track_t * t;
  
  qt_priv_t * priv = (qt_priv_t*)ctx->priv;
 
  int i;

  if(!ctx->input->filename)
    return;
  
  ctx->edl = bgav_edl_create();

  ctx->edl->url = bgav_strdup(ctx->input->filename);

  t = bgav_edl_add_track(ctx->edl);
  
  for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
    {
    es = bgav_edl_add_audio_stream(t);
    set_stream_edl(priv, &ctx->tt->cur->audio_streams[i], es);
    }
  for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
    {
    es = bgav_edl_add_video_stream(t);
    set_stream_edl(priv, &ctx->tt->cur->video_streams[i], es);
    }
  for(i = 0; i < ctx->tt->cur->num_subtitle_streams; i++)
    {
    es = bgav_edl_add_subtitle_text_stream(t);
    set_stream_edl(priv, &ctx->tt->cur->subtitle_streams[i], es);
    }
  
  }


static int open_quicktime(bgav_demuxer_context_t * ctx)
  {
  qt_atom_header_t h;
  qt_priv_t * priv = (qt_priv_t*)0;
  int have_moov = 0;
  int have_mdat = 0;
  int done = 0;
  gavl_time_t test_duration;
  int i;
  /* Create track */

  ctx->tt = bgav_track_table_create(1);
  
  /* Read moov atom */

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  while(!done)
    {
    if(!bgav_qt_atom_read_header(ctx->input, &h))
      {
      break;
      }
    switch(h.fourcc)
      {
      case BGAV_MK_FOURCC('m','d','a','t'):
        /* Reached the movie data atom, stop here */
        have_mdat = 1;

        priv->mdats = realloc(priv->mdats, (priv->num_mdats+1)*sizeof(*(priv->mdats)));
        memset(&(priv->mdats[priv->num_mdats]), 0, sizeof(*(priv->mdats)));

        priv->mdats[priv->num_mdats].start = ctx->input->position;
        priv->mdats[priv->num_mdats].size  = h.size - (ctx->input->position - h.start_position);
        priv->num_mdats++;
        
        /* Some files have the moov atom at the end */
        if(ctx->input->input->seek_byte)
          {
          bgav_qt_atom_skip(ctx->input, &h);
          }

        else if(!have_moov)
          {
          bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                   "Non streamable file on non seekable source");
          return 0;
          }
        
        break;
      case BGAV_MK_FOURCC('m','o','o','v'):
        if(!bgav_qt_moov_read(&h, ctx->input, &(priv->moov)))
          {
          bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                   "Reading moov atom failed");
          return 0;
          }
        have_moov = 1;
        bgav_qt_atom_skip(ctx->input, &h);
#ifdef DUMP_MOOV
        bgav_qt_moov_dump(0, &(priv->moov));
#endif

        break;
      case BGAV_MK_FOURCC('f','r','e','e'):
      case BGAV_MK_FOURCC('w','i','d','e'):
        bgav_qt_atom_skip(ctx->input, &h);
        break;
      case BGAV_MK_FOURCC('f','t','y','p'):
        if(!bgav_input_read_fourcc(ctx->input, &priv->ftyp_fourcc))
          return 0;
        bgav_qt_atom_skip(ctx->input, &h);
        break;
      default:
        bgav_qt_atom_skip_unknown(ctx->input, &h, 0);
      }

    if(ctx->input->input->seek_byte)
      {
      if(ctx->input->position >= ctx->input->total_bytes)
        done = 1;
      }
    else if(have_moov && have_mdat)
      done = 1;
    }

  /* Get for redirecting */
  if(!have_mdat)
    {
    if(priv->moov.has_rmra)
      {
      /* Redirector!!! */
      handle_rmra(ctx);
      return 1;
      }
    else
      return 0;
    }
  /* Initialize streams */
  quicktime_init(ctx);

  /* Build index */
  build_index(ctx);

  /* Get duration */
  for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
    {
    test_duration =
      gavl_time_unscale(ctx->tt->cur->audio_streams->data.audio.format.samplerate,
                        ctx->tt->cur->audio_streams->duration);
    if(ctx->tt->cur->duration < test_duration)
      ctx->tt->cur->duration = test_duration;
    }
  for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
    {
    test_duration =
      gavl_time_unscale(ctx->tt->cur->video_streams->data.video.format.timescale,
                        ctx->tt->cur->video_streams->duration);
    if(ctx->tt->cur->duration < test_duration)
      ctx->tt->cur->duration = test_duration;
    }
  
  /* No packets are found */
  if(!ctx->si)
    return 0;

  /* Check if we have an EDL */
  if(priv->has_edl)
    build_edl(ctx);
  
  priv->current_mdat = 0;
  
  if((ctx->input->position != priv->mdats[priv->current_mdat].start) &&
     (ctx->input->input->seek_byte))
    bgav_input_seek(ctx->input, priv->mdats[priv->current_mdat].start, SEEK_SET);
  
  /* Skip until first chunk */
  
  if(priv->mdats[priv->current_mdat].start < ctx->si->entries[0].offset)
    bgav_input_skip(ctx->input,
                    ctx->si->entries[0].offset -
                    priv->mdats[priv->current_mdat].start);

  switch(priv->ftyp_fourcc)
    {
    case BGAV_MK_FOURCC('M','4','A',' '):
      ctx->stream_description = bgav_sprintf("MPEG-4 audio (m4a)");
      break;
    case BGAV_MK_FOURCC('m','p','4','1'):
    case BGAV_MK_FOURCC('m','p','4','2'):
    case BGAV_MK_FOURCC('i','s','o','m'):
      ctx->stream_description = bgav_sprintf("MPEG-4 video (mp4)");
      break;
    case 0:
    case BGAV_MK_FOURCC('q','t',' ',' '):
      ctx->stream_description = bgav_sprintf("Quicktime");
      break;
    default:
      ctx->stream_description = bgav_sprintf("Quicktime/mp4/m4a format");
      break;
    
    }
  
  if(ctx->input->input->seek_byte)
    ctx->flags |= BGAV_DEMUXER_CAN_SEEK;

  /* Seem that quicktime is always sample accurate :) */
  ctx->index_mode = INDEX_MODE_SI_SA;
  
  return 1;
  }

static void close_quicktime(bgav_demuxer_context_t * ctx)
  {
  qt_priv_t * priv;

  priv = (qt_priv_t*)(ctx->priv);

  if(priv->streams)
    free(priv->streams);
    
  if(priv->mdats)
    free(priv->mdats);
  bgav_qt_moov_free(&(priv->moov));
  free(ctx->priv);
  }

const bgav_demuxer_t bgav_demuxer_quicktime =
  {
    .probe =       probe_quicktime,
    .open =        open_quicktime,
    //    .next_packet = next_packet_quicktime,
    //    .seek =        seek_quicktime,
    .close =       close_quicktime
  };

