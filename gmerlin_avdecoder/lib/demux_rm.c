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

#include <rmff.h>

#define PN_SAVE_ENABLED 0x0001
#define PN_PERFECT_PLAY_ENABLED 0x0002
#define PN_LIVE_BROADCAST 0x0004

#define PN_KEYFRAME_FLAG 0x0002

/* Data chunk header */



static uint32_t seek_indx(bgav_rmff_indx_t * indx, uint32_t millisecs,
                          int32_t * position, int32_t * start_packet,
                          int32_t * end_packet)
  {
  uint32_t ret;
  ret = indx->num_indices - 1;
  //  fprintf(stderr, "Seek Indx %d\n", millisecs);
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
    
  //  fprintf(stderr, "Ret: %d, Time: %d, offset: %d, count: %d\n",
  //          ret,
  //          indx->records[ret].timestamp,
  //          indx->records[ret].offset,
  //          indx->records[ret].packet_count_for_this_packet);
  
  return ret;
  }


/* Audio and video stream specific stuff */

typedef struct
  {
  bgav_rmff_stream_t * stream;
  
  uint16_t version;
  uint16_t flavor;
  uint32_t coded_frame_size;
  uint16_t sub_packet_h;
  uint16_t frame_size;
  uint16_t sub_packet_size;
  
  //  uint8_t * extradata;
  int bytes_to_read;
  uint32_t index_record;
  } rm_audio_stream_t;
#if 0
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
#endif
static void init_audio_stream(bgav_demuxer_context_t * ctx,
                              bgav_rmff_stream_t * stream,
                              uint8_t * data)
  {
  uint16_t codecdata_length;
  uint16_t samplesize;
  bgav_stream_t * bg_as;
  rm_audio_stream_t * rm_as;
  uint8_t desc_len;
  bgav_track_t * track = ctx->tt->current_track;
  
  bg_as = bgav_track_add_audio_stream(track);
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
  
  //  fprintf(stderr, "Codecdata length: %d\n", codecdata_length);
  
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
  
  if(bg_as->fourcc == BGAV_MK_FOURCC('1','4','_','4'))
    {
    rm_as->bytes_to_read = bg_as->data.audio.block_align;
    }
  else
    {
    rm_as->bytes_to_read = bg_as->data.audio.block_align * rm_as->sub_packet_h;
    }
  //  dump_audio(rm_as);

  bg_as->stream_id = stream->mdpr.stream_number;
  rm_as->stream = stream;
  }

typedef struct
  {
  bgav_rmff_stream_t * stream;
  uint32_t index_record;
  
  uint32_t kf_pts;
  uint32_t kf_base;
  
  } rm_video_stream_t;

static void init_video_stream(bgav_demuxer_context_t * ctx,
                              bgav_rmff_stream_t * stream,
                              uint8_t * data)
  {
  uint32_t tmp;
  bgav_stream_t * bg_vs;
  rm_video_stream_t * rm_vs;
  uint32_t format_le;
  bgav_track_t * track = ctx->tt->current_track;
  
  bg_vs = bgav_track_add_video_stream(track);
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
    
  // bg_vs->data.video.format.framerate_num = BGAV_PTR_2_16BE(data);data+=2;
  //  bg_vs->data.video.format.framerate_den = 1;
  // we probably won't even care about fps
  //  if (bg_vs->data.video.format.framerate_num<=0) bg_vs->data.video.format.framerate_num=24; 
  data+=2; /* Skip framerate */

  bg_vs->data.video.format.timescale = 1000;
  bg_vs->data.video.format.frame_duration = 40;
  bg_vs->data.video.format.free_framerate = 1;
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
  
  bg_vs->stream_id = stream->mdpr.stream_number;
  rm_vs->stream = stream;
  }

typedef struct
  {
  bgav_rmff_header_t * header;
  
  uint32_t data_start;
  uint32_t data_size;
  int do_seek;
  uint32_t next_packet;

  uint32_t first_timestamp;
  int need_first_timestamp;
  } rm_private_t;

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
  track = ctx->tt->current_track;

  /* Create streams */

  for(i = 0; i < h->num_streams; i++)
    {
    mdpr = &(h->streams[i].mdpr);
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
      //      fprintf(stderr, "Found audio stream\n");
      init_audio_stream(ctx, &(h->streams[i]), pos);
      }
    else if(header == BGAV_MK_FOURCC('V', 'I', 'D', 'O'))
      {
      //      fprintf(stderr, "Found video stream\n");
      init_video_stream(ctx, &(h->streams[i]), pos);
      }
    else
      {
      fprintf(stderr, "Unknown stream\n");
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
    ctx->can_seek = 1;
    for(i = 0; i < track->num_audio_streams; i++)
      {
      if(!((rm_audio_stream_t*)(track->audio_streams[i].priv))->stream->has_indx)
        ctx->can_seek = 0;
      }
    for(i = 0; i < track->num_video_streams; i++)
      {
      if(!((rm_video_stream_t*)(track->video_streams[i].priv))->stream->has_indx)
        ctx->can_seek = 0;
      }
    }

  //  fprintf(stderr, "Can seek: %d\n", ctx->can_seek);

  ctx->stream_description = bgav_sprintf("Real Media file format");

  /* Handle metadata */

  cnv = bgav_charset_converter_create("ISO-8859-1", "UTF-8");

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
  
  //  fprintf(stderr, "Header size: %d\n", chunk.size);


static int probe_rmff(bgav_input_context_t * input)
  {
  uint32_t header;

  if(!bgav_input_get_fourcc(input, &header))
    return 0;
  
  if(header == BGAV_MK_FOURCC('.', 'R', 'M', 'F'))
    {
    //    fprintf(stderr, "Detected Real video format\n");
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
fix_timestamp(bgav_stream_t * stream, uint8_t * s, uint32_t timestamp)
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

        p->timestamp=(dp_hdr->len<3)?0:
          fix_timestamp(stream,dp_data,dp_hdr->timestamp);
        p->timestamp_scaled = (p->timestamp*1000)/GAVL_TIME_SCALE;
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
          p->timestamp_scaled = (p->timestamp*1000)/GAVL_TIME_SCALE;
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
    p->timestamp_scaled = (p->timestamp * 1000)/GAVL_TIME_SCALE;

    bgav_packet_done_write(p);
    
    // ds_add_packet(ds,dp);
    
    } // while(len>0)
  
  if(len)
    {
    fprintf(stderr, "\n******** !!!!!!!! BUG!! len=%d !!!!!!!!!!! ********\n",len);
    if(len>0)
      bgav_input_skip(ctx->input, len);
      // stream_skip(demuxer->stream, len);
    else
      return 0;
    }
  return 1;
  }

static int process_audio_chunk(bgav_demuxer_context_t * ctx,
                               bgav_rmff_packet_header_t * h,
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
  bgav_rmff_packet_header_t h;
  rm_audio_stream_t * as;
  rm_video_stream_t * vs;
  int result = 0;
  bgav_track_t * track;
  
  rm = (rm_private_t*)(ctx->priv);
  track = ctx->tt->current_track;
  
  //  fprintf(stderr, "Data size: %d\n", rm->data_size);
    
  if(rm->data_size && (ctx->input->position + 10 >= rm->data_start + rm->data_size))
    return 0;
  if(!bgav_rmff_packet_header_read(ctx->input, &h))
    return 0;

  if(rm->need_first_timestamp)
    {
    rm->first_timestamp = h.timestamp;
    rm->need_first_timestamp = 0;
    //    fprintf(stderr, "First timestamp: %d\n", rm->first_timestamp);
    }
  if(h.timestamp > rm->first_timestamp)
    h.timestamp -= rm->first_timestamp;
  else
    h.timestamp = 0;

  //  fprintf(stderr, "Got packet\n");
  //  bgav_rmff_packet_header_dump(&h);
  stream = bgav_track_find_stream(track, h.stream_number);

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
      if(vs->stream->indx.records[vs->index_record].packet_count_for_this_packet > rm->next_packet)
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
      if(as->stream->indx.records[as->index_record].packet_count_for_this_packet > rm->next_packet)
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

static void seek_rmff(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  uint32_t i;
  bgav_stream_t * stream;
  rm_audio_stream_t * as;
  rm_video_stream_t * vs;
  rm_private_t * rm;
  bgav_track_t * track;
  uint32_t real_time; /* We'll never be more accurate than milliseconds */
  uint32_t position = ~0x0;

  uint32_t start_packet = ~0x00;
  uint32_t end_packet =    0x00;
  
  //  fprintf(stderr, "Seek RMFF\n");
  
  rm = (rm_private_t*)(ctx->priv);
  track = ctx->tt->current_track;
  
  real_time = (time * 1000) / GAVL_TIME_SCALE;
  
  /* First step: Seek the index records for each stream */

  /* Seek the pointers for the index records for each stream */
  /* We also calculate the file position where we will start again */
  
  for(i = 0; i < track->num_video_streams; i++)
    {
    stream = &(track->video_streams[i]);
    vs = (rm_video_stream_t*)(stream->priv);
    vs->index_record = seek_indx(&(vs->stream->indx), real_time,
                                 &position, &start_packet, &end_packet);
    stream->time = ((int64_t)(vs->stream->indx.records[vs->index_record].timestamp) *
                    GAVL_TIME_SCALE) / 1000;
    vs->kf_pts = vs->stream->indx.records[vs->index_record].timestamp;
    }
  for(i = 0; i < track->num_audio_streams; i++)
    {
    stream = &(track->audio_streams[i]);
    as = (rm_audio_stream_t*)(stream->priv);
    as->index_record = seek_indx(&(as->stream->indx), real_time,
                                 &position, &start_packet, &end_packet);
    stream->time = ((int64_t)(as->stream->indx.records[as->index_record].timestamp) *
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

static void close_rmff(bgav_demuxer_context_t * ctx)
  {
  rm_audio_stream_t * as;
  rm_video_stream_t * vs;
  rm_private_t * priv;
  bgav_track_t * track;
  int i;
  priv = (rm_private_t *)ctx->priv;
  track = ctx->tt->current_track;
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    as = (rm_audio_stream_t*)(track->audio_streams[i].priv);
    if(track->audio_streams[i].ext_data)
      free(track->audio_streams[i].ext_data);
    free(as);
    } 
  for(i = 0; i < track->num_video_streams; i++)
    {
    vs = (rm_video_stream_t*)(track->video_streams[i].priv);
    if(track->video_streams[i].ext_data)
      free(track->video_streams[i].ext_data);
    free(vs);
    }

  if(priv->header)
    bgav_rmff_header_destroy(priv->header);
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
