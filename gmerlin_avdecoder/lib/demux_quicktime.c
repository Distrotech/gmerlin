/*****************************************************************
 
  demux_quicktime.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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

  int skip_first_frame; /* Enabled only if the first frame has a different codec */
  } stream_priv_t;

typedef struct
  {
  uint32_t ftyp_fourcc;
  qt_moov_t moov;

  stream_priv_t * streams;
  
  int seeking;

  
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
  int i;
  s->trak = trak;
  s->stbl = &(trak->mdia.minf.stbl);

  s->stts_pos = (s->stbl->stts.num_entries > 1) ? 0 : -1;
  /* stsz_pos is -1 if all samples have the same size */
  s->stsz_pos = (s->stbl->stsz.sample_size) ? -1 : 0;

  for(i = 0; i < s->stbl->stts.num_entries;i++)
    {
    s->total_tics += s->stbl->stts.entries[i].count *
      s->stbl->stts.entries[i].duration;
    }
  }

static gavl_time_t stream_get_duration(bgav_stream_t * s)
  {
  stream_priv_t * priv = (stream_priv_t*)(s->priv);
  return (priv->total_tics * GAVL_TIME_SCALE) / s->timescale;
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
  if(header == BGAV_MK_FOURCC('f','r','e','e'))
    return 1;
  else if(header == BGAV_MK_FOURCC('m','d','a','t'))
    return 1;
  
  return 0;
  }

static bgav_stream_t * find_stream(bgav_demuxer_context_t * ctx,
                                   qt_trak_t * trak)
  {
  //  qt_priv_t * priv; 
  stream_priv_t * priv;
  int i;
  bgav_track_t * track = ctx->tt->current_track;
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    priv = (stream_priv_t *)(track->audio_streams[i].priv);
    if(priv->trak == trak)
      return &track->audio_streams[i];
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    priv = (stream_priv_t *)(track->video_streams[i].priv);
    if(priv->trak == trak)
      return &track->video_streams[i];
    }
  return (bgav_stream_t*)0;
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

static void add_packet(bgav_demuxer_context_t * ctx,
                       qt_priv_t * priv,
                       bgav_stream_t * s,
                       int index,
                       int64_t offset,
                       int stream_id,
                       int64_t timestamp,
                       int keyframe, int samples, int chunk_size)
  {
  if(stream_id >= 0)
    bgav_superindex_add_packet(ctx->si, s,
                               offset, chunk_size, stream_id, timestamp, keyframe, samples);
  
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
        priv->current_mdat++;
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

  priv = (qt_priv_t *)(ctx->priv);

  /* 1 step: Count the total number of chunks */
  for(i = 0; i < priv->moov.num_tracks; i++)
    {
    if(priv->moov.tracks[i].mdia.minf.has_vmhd) /* One video chunk can be more packets (=frames) */
      {
      num_packets += bgav_qt_trak_samples(&priv->moov.tracks[i]);
      if(priv->streams[i].skip_first_frame)
        num_packets--;
      }
    /* Some audio frames will be read as "samples" (-> VBR audio!) */
    else if(priv->moov.tracks[i].mdia.minf.has_smhd &&
            !priv->moov.tracks[i].mdia.minf.stbl.stsz.sample_size)
      {
      num_packets += bgav_qt_trak_samples(&priv->moov.tracks[i]);
      }
    else /* Other packets will be complete quicktime chunks */
      num_packets += bgav_qt_trak_chunks(&priv->moov.tracks[i]);
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
      if((priv->streams[j].stco_pos < priv->moov.tracks[j].mdia.minf.stbl.stco.num_entries) &&
         (priv->moov.tracks[j].mdia.minf.stbl.stco.entries[priv->streams[j].stco_pos] < chunk_offset))
        {
        stream_id = j;
        chunk_offset = priv->moov.tracks[j].mdia.minf.stbl.stco.entries[priv->streams[j].stco_pos];
        }
      }

    bgav_s = find_stream(ctx, &(priv->moov.tracks[stream_id]));

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
                     s->tics,
                     check_keyframe(s), chunk_samples, packet_size);
          
          s->tics += chunk_samples;
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
                   s->tics,
                   check_keyframe(s), chunk_samples, 0);
        /* Time to sample */
        s->tics += chunk_samples;
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
        
        if(s->skip_first_frame && !s->stco_pos)
          add_packet(ctx,
                     priv,
                     bgav_s,
                     i, chunk_offset,
                     -1,
                     s->tics,
                     check_keyframe(s), 1, packet_size);
        else
          {
          add_packet(ctx,
                     priv,
                     bgav_s,
                     i, chunk_offset,
                     stream_id,
                     s->tics,
                     check_keyframe(s), 1, packet_size);
          i++;
          }
        chunk_offset += packet_size;
        
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
      add_packet(ctx, priv, (bgav_stream_t*)0, i, chunk_offset, -1, -1, 0, 0, 0);
      i++;
      priv->streams[stream_id].stco_pos++;
      }
    }
  ctx->si->entries[ctx->si->num_entries-1].size =
    priv->mdats[priv->current_mdat].start +
  priv->mdats[priv->current_mdat].size - ctx->si->entries[ctx->si->num_entries-1].offset;
  
  free(chunk_indices);
  }

#define SET_UDTA_STRING(dst, src) \
if(!(ctx->tt->current_track->metadata.dst) && moov->udta.src)\
  {                                                                     \
  if(moov->udta.have_ilst) \
    ctx->tt->current_track->metadata.dst = bgav_strdup(moov->udta.src); \
  else \
    ctx->tt->current_track->metadata.dst = bgav_convert_string(cnv, moov->udta.src, -1, NULL);\
  }

#define SET_UDTA_INT(dst, src) \
  if(!(ctx->tt->current_track->metadata.dst) && moov->udta.src && isdigit(*(moov->udta.src))) \
    { \
    ctx->tt->current_track->metadata.dst = atoi(moov->udta.src);\
    }

static void set_metadata(bgav_demuxer_context_t * ctx)
  {
  qt_priv_t * priv;
  qt_moov_t * moov;
  
  bgav_charset_converter_t * cnv = (bgav_charset_converter_t *)0;
  
  priv = (qt_priv_t*)(ctx->priv);
  moov = &(priv->moov);

  if(!moov->udta.have_ilst)
    cnv = bgav_charset_converter_create("ISO-8859-1", "UTF-8");
    
  
  SET_UDTA_STRING(artist,    ART);
  SET_UDTA_STRING(title,     nam);
  SET_UDTA_STRING(album,     alb);
  SET_UDTA_STRING(genre,     gen);
  SET_UDTA_STRING(copyright, cpy);
  SET_UDTA_INT(track,     trk);
  SET_UDTA_STRING(comment,   cmt);
  SET_UDTA_STRING(comment,   inf);
  SET_UDTA_STRING(author,    aut);

  if(!ctx->tt->current_track->metadata.track && moov->udta.trkn)
    ctx->tt->current_track->metadata.track = moov->udta.trkn;

  if(cnv)
    bgav_charset_converter_destroy(cnv);

  //  bgav_metadata_dump(&ctx->tt->current_track->metadata);
  }

static void quicktime_init(bgav_demuxer_context_t * ctx)
  {
  int i, j;
  uint32_t atom_size, fourcc;
  uint8_t * ptr;
  bgav_stream_t * bg_as;
  bgav_stream_t * bg_vs;
  stream_priv_t * stream_priv;
  gavl_time_t     stream_duration;
  qt_sample_description_t * desc;
  bgav_track_t * track;
  int skip_first_frame = 0;
  qt_priv_t * priv = (qt_priv_t*)(ctx->priv);
  qt_moov_t * moov = &(priv->moov);

  track = ctx->tt->current_track;
    
  ctx->tt->current_track->duration = 0;

  priv->streams = calloc(moov->num_tracks, sizeof(*(priv->streams)));
  
  
  for(i = 0; i < moov->num_tracks; i++)
    {
    /* Audio stream */
    if(moov->tracks[i].mdia.minf.has_smhd)
      {
      if(!moov->tracks[i].mdia.minf.stbl.stsd.entries)
        {
        continue;
        }
      bg_as = bgav_track_add_audio_stream(track, ctx->opt);
      desc = &(moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc);

      stream_priv = &(priv->streams[i]);
      stream_init(stream_priv, &(moov->tracks[i]));
      
      bg_as->priv = stream_priv;
      
      bg_as->timescale = moov->tracks[i].mdia.mdhd.time_scale;
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
      
      if((desc->has_esds) &&
         (desc->esds.decoderConfigLen))
        {
        bg_as->ext_size = desc->esds.decoderConfigLen;
        bg_as->ext_data = desc->esds.decoderConfig;
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
      bg_as->stream_id = i;
      
      stream_duration = stream_get_duration(bg_as);
      if(ctx->tt->current_track->duration < stream_duration)
        ctx->tt->current_track->duration = stream_duration;

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
    else if(moov->tracks[i].mdia.minf.has_vmhd)
      {
      skip_first_frame = 0;

      if(!moov->tracks[i].mdia.minf.stbl.stsd.entries)
        {
        continue;
        }
      if(moov->tracks[i].mdia.minf.stbl.stsd.num_entries > 1)
        {
        if((moov->tracks[i].mdia.minf.stbl.stsd.num_entries == 2) &&
           (moov->tracks[i].mdia.minf.stbl.stsc.num_entries >= 2) &&
           (moov->tracks[i].mdia.minf.stbl.stsc.entries[0].samples_per_chunk == 1) &&
           (moov->tracks[i].mdia.minf.stbl.stsc.entries[1].sample_description_id == 2) &&
           (moov->tracks[i].mdia.minf.stbl.stsc.entries[1].first_chunk == 2))
          {
          skip_first_frame = 1;
          }
        else
          {
          continue;
          }
        
        }
      
      bg_vs = bgav_track_add_video_stream(track, ctx->opt);
      bg_vs->vfr_timestamps = 1;
            
      desc = &(moov->tracks[i].mdia.minf.stbl.stsd.entries[skip_first_frame].desc);
      stream_priv = &(priv->streams[i]);
      
      stream_init(stream_priv, &(moov->tracks[i]));

      if(skip_first_frame)
        {
        stream_priv->skip_first_frame = 1;
#if 0
        /* Sample size */
        if(stream_priv->stsz_pos >= 0)
          stream_priv->stsz_pos++;
        
        /* Time to sample */
        if(stream_priv->stts_pos >= 0)
          {
          stream_priv->tics += stream_priv->stbl->stts.entries[stream_priv->stts_pos].duration;
          stream_priv->stts_count++;
          if(stream_priv->stts_count >= stream_priv->stbl->stts.entries[stream_priv->stts_pos].count)
            {
            stream_priv->stts_pos++;
            stream_priv->stts_count = 0;
            }
          }
        else
          {
          stream_priv->tics += stream_priv->stbl->stts.entries[0].duration;
          }
        stream_priv->stco_pos++;
        /* Update sample to chunk */
        if(stream_priv->stsc_pos < stream_priv->stbl->stsc.num_entries - 1)
          {
          if(stream_priv->stbl->stsc.entries[stream_priv->stsc_pos+1].first_chunk - 1 == stream_priv->stco_pos)
            stream_priv->stsc_pos++;
          }
#endif
        }

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
      
      bg_vs->data.video.format.timescale = moov->tracks[i].mdia.mdhd.time_scale;

      /* We set the timescale here, because we need it before the dmuxer sets it. */

      bg_vs->timescale = moov->tracks[i].mdia.mdhd.time_scale;
      
      bg_vs->data.video.format.frame_duration =
        moov->tracks[i].mdia.minf.stbl.stts.entries[0].duration;

      if((moov->tracks[i].mdia.minf.stbl.stts.num_entries == 1) ||
         ((moov->tracks[i].mdia.minf.stbl.stts.num_entries == 2) &&
          (moov->tracks[i].mdia.minf.stbl.stts.entries[1].count == 1)))
        bg_vs->data.video.format.framerate_mode = GAVL_FRAMERATE_CONSTANT;
      else
        bg_vs->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
            
      bg_vs->data.video.palette_size = desc->format.video.ctab_size;
      if(bg_vs->data.video.palette_size)
        bg_vs->data.video.palette = desc->format.video.ctab;
      
      /* Set extradata suitable for Sorenson 3 */
      
      if(bg_vs->fourcc == BGAV_MK_FOURCC('S', 'V', 'Q', '3'))
        {
        ptr = moov->tracks[i].mdia.minf.stbl.stsd.entries[skip_first_frame].data + 82;
        atom_size = BGAV_PTR_2_32BE(ptr); ptr+=4;
        
        if((BGAV_PTR_2_FOURCC(ptr)) == BGAV_MK_FOURCC('S', 'M', 'I', ' '))
          {
          bg_vs->ext_size = 82 + atom_size;
          bg_vs->ext_data = moov->tracks[i].mdia.minf.stbl.stsd.entries[0].data;
          }
        }
      else if((bg_vs->fourcc == BGAV_MK_FOURCC('a', 'v', 'c', '1')) &&
              (moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc.avcC_offset))
        {
        bg_vs->ext_data = moov->tracks[i].mdia.minf.stbl.stsd.entries[skip_first_frame].data +
          moov->tracks[i].mdia.minf.stbl.stsd.entries[0].desc.avcC_offset;
        bg_vs->ext_size = moov->tracks[i].mdia.minf.stbl.stsd.entries[skip_first_frame].desc.avcC_size;
        }
      
      /* Set mp4 extradata */

      if((moov->tracks[i].mdia.minf.stbl.stsd.entries[skip_first_frame].desc.has_esds) &&
         (moov->tracks[i].mdia.minf.stbl.stsd.entries[skip_first_frame].desc.esds.decoderConfigLen))
        {
        bg_vs->ext_size =
          moov->tracks[i].mdia.minf.stbl.stsd.entries[skip_first_frame].desc.esds.decoderConfigLen;
        bg_vs->ext_data =
          moov->tracks[i].mdia.minf.stbl.stsd.entries[skip_first_frame].desc.esds.decoderConfig;
        }
      bg_vs->stream_id = i;

      stream_duration = stream_get_duration(bg_vs);
      if(ctx->tt->current_track->duration < stream_duration)
        ctx->tt->current_track->duration = stream_duration;
      //      bgav_qt_trak_dump(&moov->tracks[i]);
      }
    }
  
  set_metadata(ctx);
  
  }

static int handle_rmra(bgav_demuxer_context_t * ctx,
                        bgav_redirector_context_t ** redir)
  {
  char * basename, *pos;
  
  int i, index;
  qt_priv_t * priv = (qt_priv_t*)(ctx->priv);
  int num_urls = 0;
  for(i = 0; i < priv->moov.rmra.num_rmda; i++)
    {
    if(priv->moov.rmra.rmda[i].rdrf.fourcc == BGAV_MK_FOURCC('u','r','l',' '))
      num_urls++;
    }

  (*redir) = calloc(1, sizeof(**redir));
  (*redir)->num_urls = num_urls;

  (*redir)->urls = calloc(num_urls, sizeof(*((*redir)->urls)));

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
        (*redir)->urls[index].url =
          bgav_strdup((char*)priv->moov.rmra.rmda[i].rdrf.data_ref);
        
        }
      /* Relative url */
      else
        {
        fprintf(stderr, "Relative: %s %s\n", basename,
                priv->moov.rmra.rmda[i].rdrf.data_ref);

        (*redir)->urls[index].url = bgav_strdup(basename);
        (*redir)->urls[index].url =
          bgav_strncat((*redir)->urls[index].url,
                       (char*)priv->moov.rmra.rmda[i].rdrf.data_ref,
                       (char*)0);
        }
      index++;
      }
    }
  }

static int open_quicktime(bgav_demuxer_context_t * ctx,
                          bgav_redirector_context_t ** redir)
  {
  qt_atom_header_t h;
  qt_priv_t * priv = (qt_priv_t*)0;
  int have_moov = 0;
  int have_mdat = 0;
  int done = 0;

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
          ctx->error_msg =
            bgav_sprintf("Non streamable file on non seekable source");
          return 0;
          }
        
        break;
      case BGAV_MK_FOURCC('m','o','o','v'):
        if(!bgav_qt_moov_read(&h, ctx->input, &(priv->moov)))
          {
          fprintf(stderr, "Reading moov atom failed\n");
          ctx->error_msg =
            bgav_sprintf("Reading moov atom failed");
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
  if(!have_mdat && priv->moov.has_rmra)
    {
    /* Redirector!!! */
    //    fprintf(stderr, "Redirector!!\n");
    handle_rmra(ctx, redir);
    return 1;
    }
  
  quicktime_init(ctx);

  if(!have_mdat)
    {
    return 0;
    }
  
  build_index(ctx);

  /* No packets are found */
  if(!ctx->si)
    return 0;
  
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
    ctx->can_seek = 1;
  
  
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

bgav_demuxer_t bgav_demuxer_quicktime =
  {
    probe:       probe_quicktime,
    open:        open_quicktime,
    //    next_packet: next_packet_quicktime,
    //    seek:        seek_quicktime,
    close:       close_quicktime
  };

