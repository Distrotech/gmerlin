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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <avdec_private.h>

#include <rmff.h>
#define LOG_DOMAIN "demux_rm"


#define PN_SAVE_ENABLED 0x0001
#define PN_PERFECT_PLAY_ENABLED 0x0002
#define PN_LIVE_BROADCAST 0x0004

#define PN_KEYFRAME_FLAG 0x0002

typedef struct
  {
  bgav_rmff_header_t * header;
  
  //  uint32_t data_start;
  //  uint32_t data_size;
  int do_seek;
  uint32_t next_packet;

  uint32_t first_timestamp;
  int need_first_timestamp;
  
  int is_multirate;
  } rm_private_t;


static uint32_t seek_indx(bgav_rmff_indx_t * indx, uint32_t millisecs,
                          uint32_t * position, uint32_t * start_packet,
                          uint32_t * end_packet)
  {
  uint32_t ret;
  ret = indx->num_indices - 1;
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
    
  
  return ret;
  }

/* Get position for multirate files */

static int get_multirate_offsets(bgav_demuxer_context_t * ctx,
                                 bgav_rmff_stream_t * stream, uint32_t * data_start,
                                 uint32_t * data_end);

typedef struct
  {
  bgav_rmff_stream_t * stream;
  uint32_t index_record;
  uint32_t data_start, data_end, data_pos;
  } rm_stream_t;
  
/* Audio and video stream specific stuff */

typedef struct
  {
  rm_stream_t com;
  
  uint32_t coded_framesize;
  uint16_t sub_packet_h;
  //  uint16_t frame_size;
  uint16_t sub_packet_size;
  int sub_packet_cnt;
  uint16_t audiopk_size;
  
  
  //  uint8_t * extradata;
  //  int bytes_to_read;
  
  uint8_t * audio_buf;
  } rm_audio_stream_t;
#if 0
static void dump_audio(rm_audio_stream_t*s)
  {
  bgav_dprintf( "Audio stream:\n");
  bgav_dprintf( "coded_framesize: %d\n", s->coded_framesize);
  bgav_dprintf( "sub_packet_h      %d\n", s->sub_packet_h);
  bgav_dprintf( "audiopk_size      %d\n", s->audiopk_size);
  bgav_dprintf( "sub_packet_size   %d\n", s->sub_packet_size);
  //  bgav_dprintf( "Bytes to read     %d\n", s->bytes_to_read);
  
  //  uint8_t * extradata;

  }
#endif
static void init_audio_stream(bgav_demuxer_context_t * ctx,
                              bgav_rmff_stream_t * stream,
                              uint8_t * data)
  {
  uint16_t codecdata_length;
  uint16_t samplesize;
  uint16_t frame_size;
  int version;
#if 0
  int flavor;
#endif
  int coded_framesize;
  int sub_packet_size;
  int sub_packet_h;

  bgav_stream_t * bg_as;
  rm_audio_stream_t * rm_as;
  uint8_t desc_len;
  bgav_track_t * track = ctx->tt->cur;

  rm_private_t * priv;
  priv = (rm_private_t*)(ctx->priv);

    
  bg_as = bgav_track_add_audio_stream(track, ctx->opt);

  /* Set container bitrate */
  bg_as->container_bitrate = stream->mdpr.avg_bit_rate;

  rm_as = calloc(1, sizeof(*rm_as));

  bg_as->priv = rm_as;

  /* Skip initial header */
  data += 4;
  
  version = BGAV_PTR_2_16BE(data);data+=2;

  if(version == 3)
    {
    int len;
    data += 16;
    len = *data; data += len+1; // Name
    len = *data; data += len+1; // Author
    len = *data; data += len+1; // Copyright
    data += 2;
    len = *data;
    data++;
    bg_as->fourcc = BGAV_PTR_2_FOURCC(data);
    bg_as->data.audio.format.num_channels = 1;
    bg_as->data.audio.bits_per_sample = 16;
    bg_as->data.audio.format.samplerate = 8000;
    }
  else
    {
    data += 2; // 00 00
    data += 4; /* .ra4 or .ra5 */
    data += 4; // ???
    data += 2; /* version (4 or 5) */
    data += 4; // header size == 0x4E
    bg_as->subformat = BGAV_PTR_2_16BE(data);data+=2;/* codec flavor id */
    coded_framesize = BGAV_PTR_2_32BE(data);data+=4;/* needed by codec */
    data += 4; // big number
    data += 4; // bigger number
    data += 4; // 2 || -''-
    // data += 2; // 0x10
    sub_packet_h = BGAV_PTR_2_16BE(data);data+=2;
    //		    data += 2; /* coded frame size */
    frame_size = BGAV_PTR_2_16BE(data);data+=2;
    sub_packet_size = BGAV_PTR_2_16BE(data);data+=2;
    data += 2; // 0
      
    if (version == 5)
      data += 6; //0,srate,0
  
    bg_as->data.audio.format.samplerate = BGAV_PTR_2_16BE(data);data+=2;
    data += 2;  // 0
    samplesize = BGAV_PTR_2_16BE(data);data += 2;
    samplesize /= 8;
    bg_as->data.audio.bits_per_sample = samplesize * 8;
    bg_as->data.audio.format.num_channels = BGAV_PTR_2_16BE(data);data+=2;
    if (version == 5)
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
  
    switch(bg_as->fourcc)
      {
      case BGAV_MK_FOURCC('d', 'n', 'e', 't'):
        /* AC3 byteswapped */
        break;
      case BGAV_MK_FOURCC('1', '4', '_', '4'):
        bg_as->data.audio.block_align = 0x14;
        break;
      case BGAV_MK_FOURCC('2', '8', '_', '8'):
        bg_as->data.audio.block_align = coded_framesize;

        rm_as->sub_packet_size = sub_packet_size;
        rm_as->sub_packet_h = sub_packet_h;
        rm_as->coded_framesize = coded_framesize;
        rm_as->audiopk_size = frame_size;
        break;
      case BGAV_MK_FOURCC('s', 'i', 'p', 'r'):
      case BGAV_MK_FOURCC('a', 't', 'r', 'c'):
      case BGAV_MK_FOURCC('c', 'o', 'o', 'k'):
        /* Get extradata for codec */
        data += 3;
        if(version == 5)
          data++;

        codecdata_length = BGAV_PTR_2_32BE(data);data+=4;
      
        bg_as->ext_size = codecdata_length;
        if(bg_as->ext_size)
          {
          bg_as->ext_data = malloc(bg_as->ext_size);
          memcpy(bg_as->ext_data, data, codecdata_length);
          }

        if((bg_as->fourcc == BGAV_MK_FOURCC('a', 't', 'r', 'c')) ||
           (bg_as->fourcc == BGAV_MK_FOURCC('c', 'o', 'o', 'k')))
          bg_as->data.audio.block_align = sub_packet_size;
        else
          bg_as->data.audio.block_align = coded_framesize;
      
        rm_as->sub_packet_size = sub_packet_size;
        rm_as->sub_packet_h = sub_packet_h;
        rm_as->coded_framesize = coded_framesize;
        rm_as->audiopk_size = frame_size;
        break;
      case BGAV_MK_FOURCC('r', 'a', 'a', 'c'):
      case BGAV_MK_FOURCC('r', 'a', 'c', 'p'):
        data += 3;
        if(version == 5)
          data++;

        codecdata_length = BGAV_PTR_2_32BE(data);data+=4;
        if(codecdata_length>=1)
          {
          bg_as->ext_size = codecdata_length-1;
          data++;
          if(bg_as->ext_size)
            {
            bg_as->ext_data = malloc(bg_as->ext_size);
            memcpy(bg_as->ext_data, data, bg_as->ext_size);
            }
          }
        break;
      default:
      
        break;
      }

    if(rm_as->sub_packet_h && rm_as->audiopk_size)
      rm_as->audio_buf = malloc(rm_as->sub_packet_h * rm_as->audiopk_size);
    } // Version > 3

  //  dump_audio(rm_as);

  bg_as->stream_id = stream->mdpr.stream_number;
  bg_as->timescale = 1000;
    
  rm_as->com.stream = stream;

  /* Init multirate */

  if(priv->is_multirate)
    {
    if(!get_multirate_offsets(ctx, stream, &rm_as->com.data_start,
                              &rm_as->com.data_end))
      {
      }
    else
      {
      rm_as->com.data_pos = rm_as->com.data_start;
      }
    
    }
  
  }

static void init_audio_stream_mp3(bgav_demuxer_context_t * ctx, bgav_rmff_stream_t * stream)
  {
  bgav_stream_t * bg_as;
  bgav_track_t * track = ctx->tt->cur;
  rm_private_t * priv;
  rm_audio_stream_t * rm_as;
  rm_as = calloc(1, sizeof(*rm_as));

  priv = (rm_private_t*)(ctx->priv);
  
  bg_as = bgav_track_add_audio_stream(track, ctx->opt);

  /* Set container bitrate */
  bg_as->container_bitrate = stream->mdpr.avg_bit_rate;

  /* RTP-packed mp3 */
  bg_as->fourcc = BGAV_MK_FOURCC('r','m','p','3');
  bg_as->stream_id = stream->mdpr.stream_number;
  bg_as->timescale = 1000;

  bg_as->priv = rm_as;
  
  rm_as->com.stream = stream;


  }

typedef struct
  {
  rm_stream_t com;
  uint32_t kf_pts;
  uint32_t kf_base;
  } rm_video_stream_t;

static void init_video_stream(bgav_demuxer_context_t * ctx,
                              bgav_rmff_stream_t * stream,
                              uint8_t * _data, int len)
  {
  uint32_t tmp;
  bgav_stream_t * bg_vs;
  rm_video_stream_t * rm_vs;
  uint32_t version;
  rm_private_t * priv;
  
  bgav_track_t * track = ctx->tt->cur;

  uint8_t * data = _data;
  
  priv = (rm_private_t*)(ctx->priv);
  
  bg_vs = bgav_track_add_video_stream(track, ctx->opt);

  bg_vs->data.video.frametime_mode = BGAV_FRAMETIME_PTS;
  
  rm_vs = calloc(1, sizeof(*rm_vs));

  /* Set container bitrate */
  bg_vs->container_bitrate = stream->mdpr.avg_bit_rate;
    
  bg_vs->ext_size = 16;
  bg_vs->ext_data = calloc(bg_vs->ext_size, 1);
  bg_vs->priv = rm_vs;

  data += 4;

  bg_vs->fourcc = BGAV_PTR_2_FOURCC(data);data+=4;

  /* emulate BITMAPINFOHEADER */

  bg_vs->data.video.format.frame_width = BGAV_PTR_2_16BE(data);data+=2;
  bg_vs->data.video.format.frame_height = BGAV_PTR_2_16BE(data);data+=2;

  bg_vs->data.video.format.image_width = bg_vs->data.video.format.frame_width;
  bg_vs->data.video.format.image_height = bg_vs->data.video.format.frame_height;

  bg_vs->data.video.format.pixel_width = 1;
  bg_vs->data.video.format.pixel_height = 1;
    
  // bg_vs->data.video.format.framerate_num = BGAV_PTR_2_16BE(data);data+=2;
  //  bg_vs->data.video.format.framerate_den = 1;
  // we probably won't even care about fps
  //  if (bg_vs->data.video.format.framerate_num<=0) bg_vs->data.video.format.framerate_num=24; 
  data+=2; /* Skip framerate */

  bg_vs->data.video.format.timescale = 1000;
  bg_vs->data.video.format.frame_duration = 40;
  bg_vs->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
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
    //    if(tmp>0){
    //    bg_vs->data.video.format.framerate_num = tmp;
    //    }
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
      break;
    }

  version = ((bg_vs->fourcc & 0x000000FF) - '0') +
    (((bg_vs->fourcc & 0x0000FF00) >> 8)- '0') * 10;

  if((version < 30) && (tmp>=0x20200002))
    {
    int cnt = len - (data - _data);

    if(cnt < 2)
      {
      bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "Video extradata too short: %d", cnt);
      }
    else
      {
      int ii;
      if(cnt > 6)
        {
        bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                  "Video extradata too long: %d", cnt);
        cnt = 6;
        }
      //      memcpy(bg_vs->ext_data + 8, data, cnt);
      for (ii = 0; ii < cnt; ii++)
        ((unsigned char*)bg_vs->ext_data)[8+ii]=
          (unsigned short)data[ii];
      }
    }
  
  bg_vs->stream_id = stream->mdpr.stream_number;
  rm_vs->com.stream = stream;
  
  if(priv->is_multirate)
    {
    if(!get_multirate_offsets(ctx, stream, &rm_vs->com.data_start,
                              &rm_vs->com.data_end))
      {
      }
    else
      {
      rm_vs->com.data_pos = rm_vs->com.data_start;
      }
    }

  }


static int get_multirate_offsets(bgav_demuxer_context_t * ctx,
                                 bgav_rmff_stream_t * stream, uint32_t * data_start,
                                 uint32_t * data_end)
  {
  bgav_rmff_chunk_t chunk;
  bgav_rmff_data_header_t data_header;
  
  uint32_t fourcc;
  int64_t old_pos;
  rm_private_t * priv;
  int i, j;
  *data_start = 0;

  old_pos = ctx->input->position;
    
  priv = (rm_private_t*)(ctx->priv);
  
  /* Look in each logical stream description for the stream number */
  for(i = 0; i < priv->header->num_streams; i++)
    {
    if(priv->header->streams[i].mdpr.is_logical_stream)
      {
      for(j = 0; j < priv->header->streams[i].mdpr.logical_stream.num_physical_streams; j++)
        {
        if(priv->header->streams[i].mdpr.logical_stream.physical_stream_numbers[j] ==
           stream->mdpr.stream_number)
          {
          *data_start = priv->header->streams[i].mdpr.logical_stream.data_offsets[j];
          break;
          }
        }
      }
    }
  if(*data_start) /* Got it */
    {
    bgav_input_seek(ctx->input, *data_start, SEEK_SET);
    }
  else
    {
    if(stream->has_indx) /* Try index */
      {
      /* 18 is the size of a version 0 data chunk header */
      *data_start = stream->indx.records[0].offset - 18;
      if(*data_start < 0)
        return 0;

      bgav_input_seek(ctx->input, *data_start, SEEK_SET);
      while(1)
        {
        if(!bgav_input_get_fourcc(ctx->input, &fourcc))
          return 0;

        if(fourcc == BGAV_MK_FOURCC('D', 'A', 'T', 'A'))
          break;
        else
          {
          (*data_start)--;
          if(*data_start < 0)
            return 0;
          bgav_input_seek(ctx->input, *data_start, SEEK_SET);
          }
        }
      }
    else
      return 0;
    }

  /* Try to read chunk */
  if(!bgav_rmff_chunk_header_read(&chunk, ctx->input))
    return 0;
  if(chunk.id != BGAV_MK_FOURCC('D', 'A', 'T', 'A'))
    return 0;
  if(!bgav_rmff_data_header_read(ctx->input, &data_header))
    return 0;

  *data_start = ctx->input->position;
  *data_end = *data_start + chunk.size - (ctx->input->position - chunk.start_position);
    
  if(stream->has_indx)
    *data_start = stream->indx.records[0].offset;
  
  bgav_input_seek(ctx->input, old_pos, SEEK_SET);
  
  return 1;
  }



int bgav_demux_rm_open_with_header(bgav_demuxer_context_t * ctx,
                                   bgav_rmff_header_t * h)
  {
  rm_private_t * priv;
  uint8_t * pos;
  int i, j;
  bgav_rmff_mdpr_t * mdpr;
  uint32_t header = 0;
  bgav_track_t * track;

  bgav_charset_converter_t * cnv;

  //  bgav_rmff_header_dump(h);
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  priv->header = h;
  
  /* Create track */
  ctx->tt = bgav_track_table_create(1);
  track = ctx->tt->cur;

  if(ctx->input->metadata.title)
    track->name = bgav_strdup(ctx->input->metadata.title);
    
  /* Create streams */

  for(i = 0; i < h->num_streams; i++)
    {
    mdpr = &(h->streams[i].mdpr);

    if(mdpr->is_logical_stream)
      {
      if(!ctx->input->input->seek_byte)
        {
        bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Cannot play multirate real from non seekable source");
        return 0;
        }
      else
        {
        priv->is_multirate = 1;
        bgav_log(ctx->opt, BGAV_LOG_DEBUG, LOG_DOMAIN,
                 "Detected multirate real");
        }
      continue;
      }

    if(mdpr->type_specific_len)
      {
      j = mdpr->type_specific_len - 4;
      pos = mdpr->type_specific_data;
      while(j)
        {
        header = BGAV_PTR_2_FOURCC(pos);
        if((header == BGAV_MK_FOURCC('.', 'r', 'a', 0xfd)) ||
           (header == BGAV_MK_FOURCC('V', 'I', 'D', 'O')))
          break;
        pos++;
        j--;
        }
      if(header == BGAV_MK_FOURCC('.', 'r', 'a', 0xfd))
        {
        init_audio_stream(ctx, &(h->streams[i]), pos);
        }
      else if(header == BGAV_MK_FOURCC('V', 'I', 'D', 'O'))
        {
        init_video_stream(ctx, &(h->streams[i]), pos,
                          mdpr->type_specific_len - (int)(pos - mdpr->type_specific_data));
        }
      }
    else if(!strcmp(mdpr->mime_type, "audio/X-MP3-draft-00"))
      {
      init_audio_stream_mp3(ctx, &(h->streams[i]));
      }
    
    }
  
  /* Update global fields */

  if(!h->prop.duration)
    track->duration = GAVL_TIME_UNDEFINED;
  else
    track->duration = h->prop.duration * (GAVL_TIME_SCALE / 1000); 
    
  priv->need_first_timestamp = 1;
  
  if(ctx->input->input->seek_byte)
    {
    ctx->flags |= BGAV_DEMUXER_CAN_SEEK;
    for(i = 0; i < track->num_audio_streams; i++)
      {
      if(!((rm_stream_t*)(track->audio_streams[i].priv))->stream->has_indx)
        ctx->flags &= ~BGAV_DEMUXER_CAN_SEEK;
      }
    for(i = 0; i < track->num_video_streams; i++)
      {
      if(!((rm_stream_t*)(track->video_streams[i].priv))->stream->has_indx)
        ctx->flags &= ~BGAV_DEMUXER_CAN_SEEK;
      }
    }

  ctx->stream_description = bgav_sprintf("Real Media file format");

  /* Handle metadata */

  cnv = bgav_charset_converter_create(ctx->opt, "ISO-8859-1", "UTF-8");

  if(priv->header->cont.title_len)
    track->metadata.title = bgav_convert_string(cnv,
                                                priv->header->cont.title,
                                                priv->header->cont.title_len,
                                                NULL);
  if(priv->header->cont.author_len)
    track->metadata.author = bgav_convert_string(cnv,
                                                priv->header->cont.author,
                                                priv->header->cont.author_len,
                                                NULL);
  if(priv->header->cont.copyright_len)
    track->metadata.copyright = bgav_convert_string(cnv,
                                                priv->header->cont.copyright,
                                                priv->header->cont.copyright_len,
                                                NULL);

  if(priv->header->cont.comment_len)
    track->metadata.comment = bgav_convert_string(cnv,
                                                priv->header->cont.comment,
                                                priv->header->cont.comment_len,
                                                NULL);
  bgav_charset_converter_destroy(cnv);

  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;
  
  return 1;
  }

static int open_rmff(bgav_demuxer_context_t * ctx,
              bgav_redirector_context_t ** redir)
  {
  bgav_rmff_header_t * h =
    bgav_rmff_header_read(ctx->input);
    
  if(!h)
    return 0;
  
  return bgav_demux_rm_open_with_header(ctx, h);
  }
  

static int probe_rmff(bgav_input_context_t * input)
  {
  uint32_t header;

  if(!bgav_input_get_fourcc(input, &header))
    return 0;
  
  if(header == BGAV_MK_FOURCC('.', 'R', 'M', 'F'))
    {
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

static int64_t
fix_timestamp(bgav_stream_t * stream, uint8_t * s, uint32_t timestamp, int * keyframe)
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
      kf=timestamp;
      *keyframe = 1;
      }
    else
      {
      // P/B frame, merge timestamps:
      int tmp=timestamp-priv->kf_base;
      kf|=tmp&(~0x1fff);        // combine with packet timestamp
      if(kf<tmp-4096) kf+=8192; else // workaround wrap-around problems
      if(kf>tmp+4096) kf-=8192;
      kf+=priv->kf_base;
      *keyframe = 0;
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
  return (int64_t)kf;
  }

#define PAYLOAD_LENGTH(HEADER) ((HEADER)->length - (((HEADER)->object_version == 0) ? 12 : 13))

#define IS_KEYFRAME(HEADER) \
  (((HEADER)->object_version == 0) ? ((HEADER)->flags & 0x02) : ((HEADER)->asm_flags & 0x02))

static int process_video_chunk(bgav_demuxer_context_t * ctx,
                               bgav_rmff_packet_header_t * h,
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
  int len = PAYLOAD_LENGTH(h);

  while(len > 2)
    {
    //    vpkg_length = 0;

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
      }
    else
      {
      if (0==(vpkg_header&0x40))
        {
        // sub-seqnum (bits 0-6: number of fragment. bit 7: ???)
        READ_8(tmp_8);
        vpkg_subseq=tmp_8;
        vpkg_subseq&=0x7f;
        }
      
      // size of the complete packet
      // bit 14 is always one (same applies to the offset)

      READ_16(tmp_16);
      vpkg_length=tmp_16;

      if (!(vpkg_length&0xC000))
        {
        vpkg_length<<=16;
        READ_16(tmp_16);
        vpkg_length|=tmp_16;
        }
      else
        vpkg_length&=0x3fff;

      // offset of the following data inside the complete packet
      // Note: if (hdr&0xC0)==0x80 then offset is relative to the
      // _end_ of the packet, so it's equal to fragment size!!!

      READ_16(tmp_16);
      vpkg_offset = tmp_16;

      if (!(vpkg_offset&0xC000))
        {
        vpkg_offset<<=16;
        READ_16(tmp_16);
        vpkg_offset|=tmp_16;
        }
      else
        vpkg_offset&=0x3fff;
      READ_8(tmp_8);
      vpkg_seqnum=tmp_8;
      //      vpkg_seqnum=stream_read_char(demuxer->stream); --len;
      }
    if(stream->packet)
      {
      p=stream->packet;
      dp_hdr=(dp_hdr_t*)p->data;
      dp_data=p->data+sizeof(*dp_hdr);
      extra=(uint32_t*)(p->data+dp_hdr->chunktab);
      // we have an incomplete packet:
      if(stream->packet_seq!=vpkg_seqnum)
        {
        // this fragment is for new packet, close the old one
        p->pts=(dp_hdr->len<3)?0:
          fix_timestamp(stream,dp_data,dp_hdr->timestamp, &p->keyframe);
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
          bgav_packet_alloc(p, dp_hdr->chunktab+8*(4+dp_hdr->chunks));
          p->data_size = dp_hdr->chunktab+8*(4+dp_hdr->chunks);
          //          dp->len=dp_hdr->chunktab+8*(4+dp_hdr->chunks);
          //          dp->buffer=realloc(dp->buffer,dp->len);
          // re-calc pointers:
          dp_hdr=(dp_hdr_t*)p->data;
          dp_data=p->data+sizeof(*dp_hdr);
          extra=(uint32_t*)(p->data+dp_hdr->chunktab);
          }
        extra[2*dp_hdr->chunks+0]=1;
        extra[2*dp_hdr->chunks+1]=dp_hdr->len;
        if(0x80==(vpkg_header&0xc0))
          {
          // last fragment!
          if(bgav_input_read_data(ctx->input, dp_data+dp_hdr->len,
                                  vpkg_offset) < vpkg_offset)
            return 0;
          
          if((dp_data[dp_hdr->len]&0x20) &&
             (stream->fourcc == BGAV_MK_FOURCC('R','V','3','0')))
            --dp_hdr->chunks;
          else
            dp_hdr->len+=vpkg_offset;
          
          len-=vpkg_offset;

          //          stream_read(demuxer->stream, dp_data+dp_hdr->len, vpkg_offset);
          //          mp_dbg(MSGT_DEMUX,MSGL_DBG2,
          //                 "fragment (%d bytes) appended, %d bytes left\n",
          //                 vpkg_offset,len);
          // we know that this is the last fragment -> we can close the packet!
#if 1
          p->pts=(dp_hdr->len<3)?0:
            fix_timestamp(stream,dp_data,dp_hdr->timestamp, &p->keyframe);
#endif
          bgav_packet_done_write(p);
          stream->packet = (bgav_packet_t*)0;
          // ds_add_packet(ds,dp);
          // ds->asf_packet=NULL;
          // continue parsing
          continue;
          }
        // non-last fragment:
        if(bgav_input_read_data(ctx->input, dp_data+dp_hdr->len, len) < len)
          return 0;
        //        stream_read(demuxer->stream, dp_data+dp_hdr->len, len);
        if((dp_data[dp_hdr->len]&0x20) &&
           (stream->fourcc == BGAV_MK_FOURCC('R','V','3','0')))
          --dp_hdr->chunks;
        else
          dp_hdr->len+=len;
        len=0;
        break; // no more fragments in this chunk!
        }
      }
    // create new packet!

    p = bgav_stream_get_packet_write(stream);
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

    if(vpkg_length > len)
      {
      bgav_packet_done_write(p);
      break;
      }

    dp_hdr->len=vpkg_length; len-=vpkg_length;

    if(bgav_input_read_data(ctx->input, dp_data, vpkg_length) < vpkg_length)
      return 0;

    //    stream_read(demuxer->stream, dp_data, vpkg_length);
    p->pts=(dp_hdr->len<3)?0:
      fix_timestamp(stream,dp_data,dp_hdr->timestamp, &p->keyframe);

    bgav_packet_done_write(p);
    //    stream->packet = (bgav_packet_t*)0;
    
    // ds_add_packet(ds,dp);
    
    } // while(len>0)
  
  if(len)
    {
    if(len>0)
      bgav_input_skip(ctx->input, len);
      // stream_skip(demuxer->stream, len);
    else
      return 0;
    }
  return 1;
  }

static const unsigned char sipr_swaps[38][2]={
    {0,63},{1,22},{2,44},{3,90},{5,81},{7,31},{8,86},{9,58},{10,36},{12,68},
    {13,39},{14,73},{15,53},{16,69},{17,57},{19,88},{20,34},{21,71},{24,46},
    {25,94},{26,54},{28,75},{29,50},{32,70},{33,92},{35,74},{38,85},{40,56},
    {42,87},{43,65},{45,59},{48,79},{49,93},{51,89},{55,95},{61,76},{67,83},
    {77,80} };

static int process_audio_chunk(bgav_demuxer_context_t * ctx,
                               bgav_rmff_packet_header_t * h,
                               bgav_stream_t * stream)
  {
  bgav_packet_t * p;
  //  int bytes_to_read;
  int packet_size;
  rm_audio_stream_t * as;
  int x, sps, cfs, sph, spc, w;
  uint8_t swp;

  uint16_t aac_packet_lengths[16];
  uint8_t num_aac_packets;
  
  as = (rm_audio_stream_t*)stream->priv;
  
  packet_size = PAYLOAD_LENGTH(h);

  if((stream->fourcc == BGAV_MK_FOURCC('2', '8', '_', '8')) ||
     (stream->fourcc == BGAV_MK_FOURCC('c', 'o', 'o', 'k')) ||
     (stream->fourcc == BGAV_MK_FOURCC('a', 't', 'r', 'c')) ||
     (stream->fourcc == BGAV_MK_FOURCC('s', 'i', 'p', 'r')))
    {
    sps = as->sub_packet_size;
    sph = as->sub_packet_h;
    cfs = as->coded_framesize;
    w   = as->audiopk_size;
    spc = as->sub_packet_cnt;

    if(IS_KEYFRAME(h))
      as->sub_packet_cnt = 0;
    
    switch(stream->fourcc)
      {
      case BGAV_MK_FOURCC('2', '8', '_', '8'):
        for (x = 0; x < sph / 2; x++)
          bgav_input_read_data(ctx->input, as->audio_buf + x * 2 * w + spc * cfs, cfs);
        break;
      case BGAV_MK_FOURCC('c', 'o', 'o', 'k'):
      case BGAV_MK_FOURCC('a', 't', 'r', 'c'):
        for (x = 0; x < w / sps; x++)
          bgav_input_read_data(ctx->input,
                               as->audio_buf + sps * (sph * x + ((sph + 1) / 2) * (spc & 1) +
                                                      (spc >> 1)),
                               sps);
        break;
      case BGAV_MK_FOURCC( 's', 'i', 'p', 'r'):
        bgav_input_read_data(ctx->input, as->audio_buf + spc * w, w);
        if (spc == sph - 1)
          {
          int n;
          int bs = sph * w * 2 / 96;  // nibbles per subpacket
          // Perform reordering
          for(n=0; n < 38; n++)
            {
            int j;
            int i = bs * sipr_swaps[n][0];
            int o = bs * sipr_swaps[n][1];
            // swap nibbles of block 'i' with 'o'      TODO: optimize
            for(j = 0;j < bs; j++)
              {
              int x = (i & 1) ? (as->audio_buf[i >> 1] >> 4) : (as->audio_buf[i >> 1] & 0x0F);
              int y = (o & 1) ? (as->audio_buf[o >> 1] >> 4) : (as->audio_buf[o >> 1] & 0x0F);
              if(o & 1)
                as->audio_buf[o >> 1] = (as->audio_buf[o >> 1] & 0x0F) | (x << 4);
              else
                as->audio_buf[o >> 1] = (as->audio_buf[o >> 1] & 0xF0) | x;
              if(i & 1)
              as->audio_buf[i >> 1] = (as->audio_buf[i >> 1] & 0x0F) | (y << 4);
              else
                as->audio_buf[i >> 1] = (as->audio_buf[i >> 1] & 0xF0) | y;
              ++i; ++o;
              }
            }
          }
        break;
      }
    /* Release all packets */
    
    if(++(as->sub_packet_cnt) >= sph)
      {
      int apk_usize = stream->data.audio.block_align;
      as->sub_packet_cnt = 0;
    
      for (x = 0; x < sph*w/apk_usize; x++)
        {
        p = bgav_stream_get_packet_write(stream);
        bgav_packet_alloc(p, apk_usize);
        p->data_size = apk_usize;
        memcpy(p->data, as->audio_buf + x * apk_usize, apk_usize);
        if(!x)
          p->keyframe = 1;
        else
          p->keyframe = 0;
        bgav_packet_done_write(p);
        }
      }
    }
  /* Byte swapped AC3 */
  else if(stream->fourcc == BGAV_MK_FOURCC('d', 'n', 'e', 't')) 
    {
    p = bgav_stream_get_packet_write(stream);
    bgav_packet_alloc(p, packet_size);
    if(bgav_input_read_data(ctx->input, p->data, packet_size) < packet_size)
      return 0;

    for(x = 0; x < packet_size; x += 2)
      {
      swp = p->data[x];
      p->data[x] = p->data[x+1];
      p->data[x+1] = swp;
      }
    
    p->data_size = packet_size;
    bgav_packet_done_write(p);
    }
  else if((stream->fourcc == BGAV_MK_FOURCC('r', 'a', 'a', 'c')) ||
          (stream->fourcc == BGAV_MK_FOURCC('r', 'a', 'c', 'p')))
    {
    bgav_input_skip(ctx->input, 1);
    if(!bgav_input_read_data(ctx->input, &num_aac_packets, 1))
      return 0;
    num_aac_packets >>= 4;
    for(x = 0; x < num_aac_packets; x++)
      {
      if(!bgav_input_read_16_be(ctx->input, &aac_packet_lengths[x]))
        return 0;
      }
    for(x = 0; x < num_aac_packets; x++)
      {
      p = bgav_stream_get_packet_write(stream);
      bgav_packet_alloc(p, packet_size);
      
      bgav_input_read_data(ctx->input, p->data, aac_packet_lengths[x]);
      p->data_size = aac_packet_lengths[x];
      bgav_packet_done_write(p);
      }
    }
  else /* No reordering needed */
    {
    p = bgav_stream_get_packet_write(stream);
    bgav_packet_alloc(p, packet_size);
    
    bgav_input_read_data(ctx->input, p->data, packet_size);
    p->data_size = packet_size;
    bgav_packet_done_write(p);

    
    }
  
  return 1;
  }

static int next_packet_rmff(bgav_demuxer_context_t * ctx)
  {
  //  bgav_packet_t * p;
  bgav_stream_t * stream = (bgav_stream_t*)0;
  rm_private_t * rm;
  bgav_rmff_packet_header_t h;
  rm_stream_t * rs = (rm_stream_t*)0;
  int result = 0;
  uint32_t stream_pos = 0;
  int i;
  
  rm = (rm_private_t*)(ctx->priv);

  if(!rm->is_multirate)
    {
    if(rm->header->data_size && (ctx->input->position + 10 >=
                                 rm->header->data_start + rm->header->data_size))
      {
      return 0;
      }
    if(!bgav_rmff_packet_header_read(ctx->input, &h))
      {
      return 0;
      }
  
    if(rm->need_first_timestamp)
      {
      rm->first_timestamp = h.timestamp;
      rm->need_first_timestamp = 0;
      }
    if(h.timestamp > rm->first_timestamp)
      h.timestamp -= rm->first_timestamp;
    else
      h.timestamp = 0;

    stream = bgav_track_find_stream(ctx, h.stream_number);

    if(!stream) /* Skip unknown stuff */
      {
      bgav_input_skip(ctx->input, PAYLOAD_LENGTH(&h));
      return 1;
      }
    }
  else /* multirate */
    {
    if(ctx->request_stream)
      {
      stream = ctx->request_stream;

      rs = (rm_stream_t*)(stream->priv);

      if(rs->data_pos >= rs->data_end)
        return 0;
      else
        stream_pos = rs->data_pos;
      }
    else
      {
      /* Find the first stream with empty packetbuffer */
      for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
        {
        stream = &(ctx->tt->cur->audio_streams[i]);
        rs = (rm_stream_t*)(stream->priv);
        
        if((stream->action == BGAV_STREAM_MUTE) ||
           !bgav_packet_buffer_is_empty(stream->packet_buffer) ||
           rs->data_pos >= rs->data_end)
          {
          stream = (bgav_stream_t*)0;
          }
        else
          {
          stream_pos = rs->data_pos;
          break;
          }
        }
      if(!stream)
        {
        for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
          {
          stream = &(ctx->tt->cur->video_streams[i]);
          rs = (rm_stream_t*)(stream->priv);
          
          if((stream->action == BGAV_STREAM_MUTE) ||
             !bgav_packet_buffer_is_empty(stream->packet_buffer) ||
             rs->data_pos >= rs->data_end)
            {
            stream = (bgav_stream_t*)0;
            }
          else
            {
            stream_pos = rs->data_pos;
            break;
            }
          }
        }
      }
    
    /* If we have no more stream, eof is reached */
    if(!stream)
      return 0;

    /* Seek to the place we've been before */
    bgav_input_seek(ctx->input, stream_pos, SEEK_SET);

    while(1)
      {
      if(!bgav_rmff_packet_header_read(ctx->input, &h))
        {
        return 0;
        }
      //      bgav_rmff_packet_header_dump(&h);
      if(stream->stream_id == h.stream_number)
        break;
      else if(rs->data_start == rs->data_pos)
        { 
        /* If this is the first packet in the stream and
           the stream id of the first packet is different,
           this means, that these packets are meant for us */
        stream->stream_id = h.stream_number;
        break;
        }
      else
        {
        bgav_input_skip(ctx->input, PAYLOAD_LENGTH(&h));
        }
      }
    }
  
  if(stream->type == BGAV_STREAM_VIDEO)
    {
    rs = (rm_stream_t*)(stream->priv);
    if(rm->do_seek)
      {
      if(rs->stream->indx.records[rs->index_record].packet_count_for_this_packet > rm->next_packet)
        {
        bgav_input_skip(ctx->input, PAYLOAD_LENGTH(&h));
        result = 1;
        }
      else
        result = process_video_chunk(ctx, &h, stream);
      }
    else
      result = process_video_chunk(ctx, &h, stream);
    rs->data_pos = ctx->input->position;
    }
  else if(stream->type == BGAV_STREAM_AUDIO)
    {
    rs = (rm_stream_t*)(stream->priv);
    if(rm->do_seek)
      {
      if(rs->stream->indx.records[rs->index_record].packet_count_for_this_packet > rm->next_packet)
        {
        bgav_input_skip(ctx->input, PAYLOAD_LENGTH(&h));
        result = 1;
        }
      else
        result = process_audio_chunk(ctx, &h, stream);
      }
    else
      result = process_audio_chunk(ctx, &h, stream);
    rs->data_pos = ctx->input->position;
    }
  rm->next_packet++;
  return result;
  }

static void seek_rmff(bgav_demuxer_context_t * ctx, int64_t time, int scale)
  {
  uint32_t i;
  bgav_stream_t * stream;
  rm_stream_t * rs;
  rm_video_stream_t * vs;
  rm_private_t * rm;
  bgav_track_t * track;
  uint32_t real_time; /* We'll never be more accurate than milliseconds */
  uint32_t position = ~0x0;

  uint32_t start_packet = ~0x00;
  uint32_t end_packet =    0x00;
  
  
  rm = (rm_private_t*)(ctx->priv);
  track = ctx->tt->cur;
  
  real_time = gavl_time_rescale(scale, 1000, time);
  
  /* First step: Seek the index records for each stream */

  /* Seek the pointers for the index records for each stream */
  /* We also calculate the file position where we will start again */
  
  for(i = 0; i < track->num_video_streams; i++)
    {
    stream = &(track->video_streams[i]);
    vs = (rm_video_stream_t*)(stream->priv);
    vs->com.index_record = seek_indx(&(vs->com.stream->indx), real_time,
                                 &position, &start_packet, &end_packet);
    stream->in_time = (int64_t)(vs->com.stream->indx.records[vs->com.index_record].timestamp);
    vs->kf_pts = vs->com.stream->indx.records[vs->com.index_record].timestamp;
    vs->com.data_pos = vs->com.stream->indx.records[vs->com.index_record].offset;
    }
  for(i = 0; i < track->num_audio_streams; i++)
    {
    stream = &(track->audio_streams[i]);
    rs = (rm_stream_t*)(stream->priv);
    rs->index_record = seek_indx(&(rs->stream->indx), real_time,
                                 &position, &start_packet, &end_packet);
    stream->in_time = (int64_t)(rs->stream->indx.records[rs->index_record].timestamp);
    rs->data_pos = rs->stream->indx.records[rs->index_record].offset;
    }
  
  /* Seek to the position */

  if(!rm->is_multirate)
    {
    bgav_input_seek(ctx->input, position, SEEK_SET);

    /* Skip unused packets */
    
    rm->do_seek = 1;
    rm->next_packet = start_packet;
    while(rm->next_packet < end_packet)
      next_packet_rmff(ctx);
    rm->do_seek = 0;
    }
  }

static void close_rmff(bgav_demuxer_context_t * ctx)
  {
  rm_audio_stream_t * as;
  rm_video_stream_t * vs;
  rm_private_t * priv;
  bgav_track_t * track;
  int i;
  priv = (rm_private_t *)ctx->priv;

  if(ctx->tt)
    {
    track = ctx->tt->cur;
    
    for(i = 0; i < track->num_audio_streams; i++)
      {
      as = (rm_audio_stream_t*)(track->audio_streams[i].priv);
      if(as) free(as);
      
      if(track->audio_streams[i].ext_data)
        free(track->audio_streams[i].ext_data);
      }
    for(i = 0; i < track->num_video_streams; i++)
      {
      vs = (rm_video_stream_t*)(track->video_streams[i].priv);
      if(vs) free(vs);
      
      if(track->video_streams[i].ext_data)
        free(track->video_streams[i].ext_data);
      }
    }

  if(priv)
    {
    if(priv->header)
      bgav_rmff_header_destroy(priv->header);
    free(priv);
    }
  }

static int select_track_rmff(bgav_demuxer_context_t * ctx, int t)
  {
  rm_audio_stream_t * as;
  rm_video_stream_t * vs;
  rm_private_t * priv;
  bgav_track_t * track;
  int i;
  priv = (rm_private_t *)ctx->priv;
  
  priv->next_packet = 0;
  
  track = ctx->tt->cur;

#if 1
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    as = (rm_audio_stream_t*)(track->audio_streams[i].priv);
    if(as)
      {
      as->sub_packet_cnt = 0;
      as->com.data_pos = as->com.data_start;
      }
    }
  for(i = 0; i < track->num_video_streams; i++)
    {
    vs = (rm_video_stream_t*)(track->video_streams[i].priv);
    if(vs)
      {
      vs->kf_pts = 0;
      vs->kf_base = 0;
      vs->com.data_pos = vs->com.data_start;
      }
    }
#endif
  return 1;
  }


const bgav_demuxer_t bgav_demuxer_rmff =
  {
    .probe =        probe_rmff,
    .open =         open_rmff,
    .select_track = select_track_rmff,
    .next_packet =  next_packet_rmff,
    .seek =         seek_rmff,
    .close =        close_rmff
  };
