/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <avdec_private.h>

#define AUDIO_ID 8
#define VIDEO_ID 9

#define LOG_DOMAIN "flv"

#define TYPE_NUMBER       0
#define TYPE_BOOL         1
#define TYPE_STRING       2
#define TYPE_OBJECT       3
#define TYPE_MIXED_ARRAY  8
#define TYPE_ARRAY       10
#define TYPE_DATE        11
#define TYPE_TERMINATOR   9

#define FOURCC_AAC  BGAV_MK_FOURCC('m', 'p', '4', 'a')
// #define FOURCC_H264 BGAV_MK_FOURCC('h', '2', '6', '4')

/* H.264 data are the same as in mp4 files */
#define FOURCC_H264 BGAV_MK_FOURCC('a', 'v', 'c', '1')

typedef struct meta_object_s meta_object_t;

struct meta_object_s
  {
  char * name;
  uint8_t type;
    
  union
    {
    double number;
    uint8_t bool;
    char * string;

    struct
      {
      double date1;
      uint16_t date2;
      } date;
    
    struct
      {
      uint32_t num_children;
      meta_object_t * children;
      } object;

    struct
      {
      uint32_t num_elements;
      meta_object_t * elements;
      } array;
    } data;
  
  };

typedef struct
  {
  int init;
  int have_metadata;
  meta_object_t metadata;
  int64_t audio_sample_counter;

  int need_audio_extradata;
  int need_video_extradata;
  int resync;
  } flv_priv_t;

static int probe_flv(bgav_input_context_t * input)
  {
  uint8_t test_data[12];
  if(bgav_input_get_data(input, test_data, 12) < 12)
    return 0;
  if((test_data[0] ==  'F') &&
     (test_data[1] ==  'L') &&
     (test_data[2] ==  'V') &&
     (test_data[3] ==  0x01))
    return 1;
  return 0;
  }

/* Process one FLV chunk, initialize streams if init flag is set */

typedef struct
  {
  uint8_t type;
  uint32_t data_size;
  uint32_t timestamp;
  uint32_t reserved;
  } flv_tag;

static int flv_tag_read(bgav_input_context_t * ctx, flv_tag * ret)
  {
  if(!bgav_input_read_data(ctx, &ret->type, 1) ||
     !bgav_input_read_24_be(ctx, &ret->data_size) ||
     !bgav_input_read_24_be(ctx, &ret->timestamp) ||
     !bgav_input_read_32_be(ctx, &ret->reserved))
    return 0;
  return 1;
  }

#if 0
static void flv_tag_dump(flv_tag * t)
  {
  bgav_dprintf("FLVTAG\n");
  bgav_dprintf("  type:      %d\n", t->type);
  bgav_dprintf("  data_size: %d\n", t->data_size);
  bgav_dprintf("  timestamp: %d\n", t->timestamp);
  bgav_dprintf("  reserved:  %d\n", t->reserved);
  }
#endif

static void add_audio_stream(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * as = NULL;
  as = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
  as->stream_id = AUDIO_ID;
  as->timescale = 1000;
  as->flags |= STREAM_NO_DURATIONS;
  }

static void add_video_stream(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * vs = NULL;
  vs = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
  vs->flags |= STREAM_NO_DURATIONS;

  vs->stream_id = VIDEO_ID;
  vs->timescale = 1000;
  }

static int init_audio_stream(bgav_demuxer_context_t * ctx, bgav_stream_t * s, 
                              uint8_t flags)
  {
  uint8_t tmp_8;
  int adpcm_bits;
  flv_priv_t * priv;

  priv = ctx->priv;

  s->flags |= STREAM_NEED_START_TIME;
    
  if(!s->fourcc) /* Initialize */
    {
    s->data.audio.bits_per_sample = (flags & 2) ? 16 : 8;
      
    s->data.audio.format.samplerate = (44100<<((flags>>2)&3))>>3;
    s->data.audio.format.num_channels = (flags&1)+1;
      
    switch(flags >> 4)
      {
      case 0: /* Uncompressed, Big endian */
        s->fourcc = BGAV_MK_FOURCC('t', 'w', 'o', 's');
        s->index_mode = INDEX_MODE_SIMPLE;
        s->data.audio.block_align = s->data.audio.format.num_channels *
          (s->data.audio.bits_per_sample / 8);
        s->duration = 0;
        break;
      case 1: /* Flash ADPCM */
        s->fourcc = BGAV_MK_FOURCC('F', 'L', 'A', '1');
        ctx->index_mode = 0;
        /* Set block align for the case, adpcm frames are split over
           several packets */
        if(!bgav_input_get_data(ctx->input, &tmp_8, 1))
          return 0;
        adpcm_bits = 2 +
          s->data.audio.format.num_channels * (((tmp_8 >> 6) + 2) * 4096 + 16 + 6);
        break;
      case 2: /* MP3 */
        s->fourcc = BGAV_MK_FOURCC('.', 'm', 'p', '3');
        s->index_mode = INDEX_MODE_MPEG;
        s->flags |= STREAM_PARSE_FULL;
        s->duration = 0;
        break;
      case 3: /* Uncompressed, Little endian */
        s->fourcc = BGAV_MK_FOURCC('s', 'o', 'w', 't');
        s->index_mode = INDEX_MODE_SIMPLE;
        s->data.audio.block_align = s->data.audio.format.num_channels *
          (s->data.audio.bits_per_sample / 8);
        s->duration = 0;
        break;
      case 5: /* NellyMoser */
        s->data.audio.format.samplerate = 8000;
        s->data.audio.format.num_channels = 1;
          
        s->fourcc = BGAV_MK_FOURCC('N', 'E', 'L', 'L');
        ctx->index_mode = 0;
        break;
      case 6: /* NellyMoser */
        s->fourcc = BGAV_MK_FOURCC('N', 'E', 'L', 'L');
        ctx->index_mode = 0;
        break;
      case 10:
        s->fourcc = FOURCC_AAC;
        //        s->index_mode = INDEX_MODE_MPEG;
        s->flags |= STREAM_PARSE_FULL | STREAM_NEED_START_TIME;
        s->index_mode = INDEX_MODE_MPEG;
        // ctx->index_mode = 0;
        s->duration = 0;
        priv->need_audio_extradata = 1;
        break;
      default: /* Set some nonsense so we can finish initializing */
        s->fourcc = BGAV_MK_FOURCC('?', '?', '?', '?');
        bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Unknown audio codec tag: %d",
                 flags >> 4);
        ctx->index_mode = 0;
        break;
      }
    }
  return 1;
  }

static int init_video_stream(bgav_demuxer_context_t * ctx, bgav_stream_t * s, 
                              uint8_t flags)
  {
  flv_priv_t * priv;
  uint8_t header[1];
  priv = ctx->priv;

  s->flags |= STREAM_NEED_START_TIME;
  
  switch(flags & 0xF)
    {
    case 2: /* H263 based */
      s->fourcc = BGAV_MK_FOURCC('F', 'L', 'V', '1');
      break;
    case 3: /* Screen video (unsupported) */
      s->fourcc = BGAV_MK_FOURCC('F', 'L', 'V', 'S');
      break;
    case 4: /* VP6 (Flash 8?) */
      s->fourcc = BGAV_MK_FOURCC('V', 'P', '6', 'F');

      if(bgav_input_get_data(ctx->input, header, 1) < 1)
        return 0;
      /* ffmpeg needs that as extrdata */
      s->ext_data = malloc(1);
      *s->ext_data = header[0];
      s->ext_size = 1;
      break;
    case 5: /* VP6A */
      s->fourcc = BGAV_MK_FOURCC('V', 'P', '6', 'A');
      if(bgav_input_get_data(ctx->input, header, 1) < 1)
        return 0;
      /* ffmpeg needs that as extrdata */
      s->ext_data = malloc(1);
      *s->ext_data = header[0];
      s->ext_size = 1;
      break;
    case 7:
      s->fourcc = FOURCC_H264;
      priv->need_video_extradata = 1;
      s->flags |= (STREAM_B_FRAMES | STREAM_HAS_DTS);
      
      //          s->data.video.wrong_b_timestamps = 1;
      break;
    default: /* Set some nonsense so we can finish initializing */
      s->fourcc = BGAV_MK_FOURCC('?', '?', '?', '?');
      bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Unknown video codec tag: %d",
               flags & 0x0f);
      break;
    }
      
  /* Set the framerate */
  s->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
  s->data.video.format.timescale = 1000;
  
  /* Hopefully, FLV supports square pixels only */
  s->data.video.format.pixel_width = 1;
  s->data.video.format.pixel_height = 1;

  return 1;
  }


/* From libavutil */
static double int2dbl(int64_t v){
    if(v+v > 0xFFEULL<<52)
        return 0.0/0.0;
    return ldexp(((v&((1LL<<52)-1)) + (1LL<<52)) * (v>>63|1), (v>>52&0x7FF)-1075);
}

static void dump_meta_object(meta_object_t * obj, int num_spaces)
  {
  int i;
  for(i = 0; i < num_spaces; i++)
    bgav_dprintf(" ");
  if(obj->name) bgav_dprintf("N: %s, ", obj->name);
  switch(obj->type)
    {
    case TYPE_NUMBER:
      bgav_dprintf("%f\n", obj->data.number);
      break;
    case TYPE_BOOL:
      bgav_dprintf("%s\n", (obj->data.bool ? "True" : "False"));
      break;
    case TYPE_STRING:
      bgav_dprintf("%s\n", obj->data.string);
      break;
    case TYPE_OBJECT:
      bgav_dprintf("\n");
      for(i = 0; i < obj->data.object.num_children; i++)
        dump_meta_object(&obj->data.object.children[i], num_spaces + 2);
      break;
    case TYPE_MIXED_ARRAY:
      bgav_dprintf("\n");
      for(i = 0; i < obj->data.array.num_elements; i++)
        dump_meta_object(&obj->data.array.elements[i], num_spaces + 2);
      break;
    case TYPE_ARRAY:
      bgav_dprintf("\n");
      for(i = 0; i < obj->data.array.num_elements; i++)
        dump_meta_object(&obj->data.array.elements[i], num_spaces + 2);
      break;
    case TYPE_DATE:
      bgav_dprintf("%f %d\n", obj->data.date.date1, obj->data.date.date2);
      break;
    case TYPE_TERMINATOR:
      bgav_dprintf("TERMINATOR\n");
      break;
    }
  }

static int read_meta_object(bgav_input_context_t * input,
                            meta_object_t * ret, int read_name, int read_type, int64_t end_pos)
  {
  int64_t  i64;
  uint16_t len;
  int i;
  
  if(read_name)
    {
    if(!bgav_input_read_16_be(input, &len))
      return 0;

    if(len)
      {
      ret->name = malloc(len+1);
      if(bgav_input_read_data(input, (uint8_t*)ret->name, len) < len)
        return 0;
      ret->name[len] = '\0';
      }

    }

  if(read_type)
    {
    if(!bgav_input_read_data(input, &ret->type, 1))
      return 0;
    }
  
  switch(ret->type)
    {
    case TYPE_NUMBER:
      if(!bgav_input_read_64_be(input, (uint64_t*)&i64))
        return 0;
      ret->data.number = int2dbl(i64);
      break;
    case TYPE_BOOL:
      if(!bgav_input_read_data(input, &ret->data.bool, 1))
        return 0;
      break;
    case TYPE_STRING:
      if(!bgav_input_read_16_be(input, &len))
        return 0;
      ret->data.string = malloc(len+1);
      if(bgav_input_read_data(input, (uint8_t*)ret->data.string, len) < len)
        return 0;
      ret->data.string[len] = '\0';
      break;
    case TYPE_OBJECT:
      while(1)
        {
        if(input->position >= end_pos)
          break;
        
        ret->data.object.num_children++;
        ret->data.object.children =
          realloc(ret->data.object.children,
                  ret->data.object.num_children *
                  sizeof(*ret->data.object.children));

        memset(&ret->data.object.children[ret->data.object.num_children-1], 0,
               sizeof(ret->data.object.children[ret->data.object.num_children-1]));
        
        if(!read_meta_object(input,
                             &ret->data.object.children[ret->data.object.num_children-1],
                             1, 1, end_pos))
          return 0;
        
        if(ret->data.object.children[ret->data.object.num_children-1].type ==
           TYPE_TERMINATOR)
          break;
        }
      break;
    case TYPE_ARRAY:
      
      if(!bgav_input_read_32_be(input, &ret->data.array.num_elements))
        return 0;

      if(ret->data.array.num_elements)
        {
        ret->data.array.elements = malloc(ret->data.array.num_elements *
                                          sizeof(*ret->data.array.elements));
        
        memset(ret->data.array.elements, 0, ret->data.array.num_elements *
               sizeof(*ret->data.array.elements));
        
        for(i = 0; i < ret->data.array.num_elements; i++)
          {
          read_meta_object(input,
                           &ret->data.object.children[i],
                           0, 1, end_pos);
          }
        }
      
      break;
    case TYPE_DATE:
      
      if(!bgav_input_read_64_be(input, (uint64_t*)&i64))
        return 0;
      ret->data.date.date1 = int2dbl(i64);

      if(!bgav_input_read_16_be(input, &ret->data.date.date2))
        return 0;
      break;
    case TYPE_TERMINATOR:
      break;
    default:
      bgav_log(input->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Unknown type %d for metadata object %s",
               ret->type, ret->name);
      return 0;
    }
  return 1;
  }

static void free_meta_object(meta_object_t * obj)
  {
  int i;
  if(obj->name) free(obj->name);
  
  switch(obj->type)
    {
    case TYPE_STRING:
      if(obj->data.string)
        free(obj->data.string);
      break;
    case TYPE_OBJECT:
      for(i = 0; i < obj->data.object.num_children; i++)
        free_meta_object(&obj->data.object.children[i]);
      if(obj->data.object.children)
        free(obj->data.object.children);
      break;
    case TYPE_MIXED_ARRAY:
    case TYPE_ARRAY:
      for(i = 0; i < obj->data.array.num_elements; i++)
        free_meta_object(&obj->data.array.elements[i]);
      if(obj->data.array.elements)
        free(obj->data.array.elements);
      break;
    }
  }

static meta_object_t * meta_object_find(meta_object_t * obj, int num, const char * name)
  {
  int i;
  for(i = 0; i < num; i++)
    {
    if(obj[i].name && !strcasecmp(obj[i].name, name))
      return &obj[i];
    }
  return NULL;
  }

static int
meta_object_find_number(meta_object_t * obj, int num, const char * name, double * ret)
  {
  meta_object_t *o;
  o = meta_object_find(obj, num, name);
  if(o && (o->type == TYPE_NUMBER))
    {
    *ret = o->data.number;
    return 1;
    }
  return 0;
  }

static int read_metadata(bgav_demuxer_context_t * ctx, flv_tag * t)
  {
  uint8_t u8;
  flv_priv_t * priv;
  int64_t end_pos;
  int ret;
  
  priv = ctx->priv;

  end_pos = ctx->input->position + t->data_size;
  
  bgav_input_skip(ctx->input, 13); //onMetaData blah
  bgav_input_read_data(ctx->input, &u8, 1);
  if(u8 == 8)
    bgav_input_skip(ctx->input, 4);

  priv->metadata.type = TYPE_OBJECT;

  ret = read_meta_object(ctx->input, &priv->metadata, 0, 0, end_pos);
  
  if(ret && (ctx->input->position < end_pos))
    {
    bgav_input_skip(ctx->input, end_pos - ctx->input->position);
    }
  return ret;
  }

                              
static int next_packet_flv(bgav_demuxer_context_t * ctx)
  {
  meta_object_t * obj;
  int num_obj;
  
  //  double number;
  uint8_t flags;
  uint8_t header[16];
  bgav_packet_t * p;
  bgav_stream_t * s;
  flv_priv_t * priv;
  flv_tag t;
  int64_t position;
  int packet_size = 0;
  int keyframe = 1;
  //  int adpcm_bits;
  //  uint8_t tmp_8;
  int32_t cts;
  int has_cts = 0;

  priv = ctx->priv;

  position = ctx->input->position;
  /* Previous tag size (ignore this?) */
  bgav_input_skip(ctx->input, 4);
  
  if(!flv_tag_read(ctx->input, &t))
    return 0;

  if(t.type == 0x12)
    {
    if(priv->init)
      {
      /* onMetaData */
      //    flv_tag_dump(&t);

      if(bgav_input_get_data(ctx->input, header, 13) < 13)
        return 0;

      if(strncmp((char*)(&header[3]), "onMetaData", 10))
        {
        bgav_input_skip(ctx->input, t.data_size);
        return 1;
        }
    
      priv->have_metadata = read_metadata(ctx, &t);

      obj = priv->metadata.data.object.children;
      num_obj = priv->metadata.data.object.num_children;
    
#if 0
      /* Check if we can detect an audio stream */
      if(!ctx->tt->cur->num_audio_streams)
        {
        if((meta_object_find_number(obj, num_obj, "audiodatarate", &number) && (number != 0.0)) ||
           (meta_object_find_number(obj, num_obj, "audiosamplerate", &number) && (number != 0.0)) ||
           (meta_object_find_number(obj, num_obj, "audiosamplesize", &number) && (number != 0.0)) ||
           (meta_object_find_number(obj, num_obj, "audiosize", &number) && (number != 0.0)))
          {
          init_audio_stream(ctx);
          }
        
        }
      if(!ctx->tt->cur->num_video_streams)
        {
        if((meta_object_find_number(obj, num_obj, "videodatarate", &number) && (number != 0.0)) ||
           (meta_object_find_number(obj, num_obj, "width", &number) && (number != 0.0)) ||
           (meta_object_find_number(obj, num_obj, "height", &number) && (number != 0.0)))
          init_video_stream(ctx);
        }
#endif
      if(ctx->opt->dump_headers)
        dump_meta_object(&priv->metadata, 0);
      }
    else
      bgav_input_skip(ctx->input, t.data_size);
    return 1;
    }
  
  //  flv_tag_dump(&t);
  
  if(priv->init)
    s = bgav_track_find_stream_all(ctx->tt->cur, t.type);
  else
    s = bgav_track_find_stream(ctx, t.type);

  if(!s)
    {
    bgav_input_skip(ctx->input, t.data_size);
    return 1;
    }
  
  if(s->stream_id == AUDIO_ID)
    {
    if(!bgav_input_read_data(ctx->input, &flags, 1))
      return 0;
    
    if(!s->fourcc) /* Initialize */
      {
      if(!init_audio_stream(ctx, s, flags))
        return 0;
      }
    packet_size = t.data_size - 1;
    }
  else if(s->stream_id == VIDEO_ID)
    {
    if(!bgav_input_read_data(ctx->input, &flags, 1))
      return 0;
    
    if(!s->fourcc) /* Initialize */
      {
      if(!init_video_stream(ctx, s, flags))
        return 0;
      }

    if((s->fourcc == BGAV_MK_FOURCC('V', 'P', '6', 'F')) ||
       (s->fourcc == BGAV_MK_FOURCC('V', 'P', '6', 'A')))
      {
      bgav_input_skip(ctx->input, 1);
      packet_size = t.data_size - 2;
      }
    else
      packet_size = t.data_size - 1;
    if((flags >> 4) != 1)
      keyframe = 0;
    }

  if((s->fourcc == FOURCC_AAC) ||
     (s->fourcc == FOURCC_H264))
    {
    uint8_t type;
    if(!bgav_input_read_8(ctx->input, &type))
      return 0;
    packet_size--;

    if(packet_size <= 0)
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Packet size is %d (somethings wrong?)",
               packet_size);
      return 0;
      }

    if(s->fourcc == FOURCC_H264)
      {
#if 1
      if(!bgav_input_read_24_be(ctx->input, (uint32_t*)(&cts)))
        return 0;
      //  cts = (cts + 0xff800000)^0xff800000; // sign extension
      //      t.timestamp += cts;
      packet_size -= 3;
      has_cts = 1;
#else
      bgav_input_skip(ctx->input, 3);
      packet_size -= 3;
#endif
      }
    
    if(type == 0)
      {
      if(!s->ext_size)
        {
        s->ext_size = packet_size;
        s->ext_data = malloc(packet_size);
        if(bgav_input_read_data(ctx->input, s->ext_data, packet_size) < packet_size)
          return 0;
        if(s->type == BGAV_STREAM_AUDIO)
          priv->need_audio_extradata = 0;
        else
          priv->need_video_extradata = 0;
        }
      else
        bgav_input_skip(ctx->input, packet_size);
      return 1;
      }
    }
  
  if(packet_size <= 0)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Packet size is %d (somethings wrong?)",
             packet_size);
    return 0;
    }
  
  if(!priv->init)
    {
    p = bgav_stream_get_packet_write(s);
    bgav_packet_alloc(p, packet_size);
    p->data_size = bgav_input_read_data(ctx->input, p->data, packet_size);
    p->position = position;
    if(p->data_size < packet_size) /* Got EOF in the middle of a packet */
      return 0;
    if(s->type == BGAV_STREAM_AUDIO)
      {
      if(s->data.audio.block_align)
        {
        if(priv->resync && !STREAM_HAS_SYNC(s))
          {
          priv->audio_sample_counter =
            gavl_time_rescale(1000, s->data.audio.format.samplerate,
                              p->pts);
          STREAM_SET_SYNC(s, priv->audio_sample_counter);
          }
        
        p->pts = priv->audio_sample_counter;
        p->duration = p->data_size / s->data.audio.block_align;
        priv->audio_sample_counter += p->duration;
        }
      else
        {
        p->pts = t.timestamp;
        }
      }
    else
      {
      if(!has_cts)
        p->pts = t.timestamp;
      else
        {
        p->dts = t.timestamp;
        p->pts = t.timestamp + cts;
        }
      }
    if(!STREAM_HAS_SYNC(s))
      STREAM_SET_SYNC(s, p->pts);

    if(keyframe)
      PACKET_SET_KEYFRAME(p);
    
    bgav_stream_done_packet_write(s, p);
    }
  else
    bgav_input_skip(ctx->input, packet_size);
  
  return 1;
  }

static void handle_metadata(bgav_demuxer_context_t * ctx)
  {
  double number;
  meta_object_t * obj, *obj1;
  int num_obj;
  flv_priv_t * priv;
  int as, vs;
  priv = ctx->priv;
  
  obj = priv->metadata.data.object.children;
  num_obj = priv->metadata.data.object.num_children;
  as = ctx->tt->cur->num_audio_streams;
  vs = ctx->tt->cur->num_video_streams;
  /* Data rates */
  
  if(as && meta_object_find_number(obj, num_obj, "audiodatarate", &number) && (number != 0.0))
    ctx->tt->cur->audio_streams[0].container_bitrate = (int)(number*1000);

  if(vs && meta_object_find_number(obj, num_obj, "videodatarate", &number) && (number != 0.0))
    ctx->tt->cur->video_streams[0].container_bitrate = (int)(number*1000);

  if(meta_object_find_number(obj, num_obj, "duration", &number) && (number != 0.0))
    ctx->tt->cur->duration = gavl_seconds_to_time(number);

  if(ctx->input->input->seek_byte)
    {
    obj1 = meta_object_find(obj, num_obj, "keyframes");
    
    if(obj1 && (obj1->type == TYPE_OBJECT) &&
       meta_object_find(obj1->data.object.children, obj1->data.object.num_children,
                        "filepositions") &&
       meta_object_find(obj1->data.object.children, obj1->data.object.num_children,
                        "times"))
      ctx->flags |= BGAV_DEMUXER_CAN_SEEK;
    }
  }

static void seek_flv(bgav_demuxer_context_t * ctx, int64_t time, int scale)
  {
  double time_float;
  meta_object_t * obj;
  int num_obj;
  
  meta_object_t * times;
  meta_object_t * filepositions;
  meta_object_t * keyframes;
  int64_t file_pos;
  uint32_t i;
  flv_priv_t * priv;
  priv = ctx->priv;
  
  /* Get index objects */

  obj = priv->metadata.data.object.children;
  num_obj = priv->metadata.data.object.num_children;

  keyframes = meta_object_find(obj, num_obj, "keyframes");

  if(!keyframes)
    return;
  
  obj = keyframes->data.object.children;
  num_obj = keyframes->data.object.num_children;
  
  times = meta_object_find(obj, num_obj, "times");
  if(!times)
    return;
  filepositions = meta_object_find(obj, num_obj, "filepositions");
  if(!filepositions)
    return;
  
  /* Search index position */
  time_float = gavl_time_to_seconds(gavl_time_unscale(scale, time));
  for(i = times->data.array.num_elements-1; i >= 0; i--)
    {
    if(times->data.array.elements[i].data.number < time_float)
      break;
    }
  
  /* Seek to position */
  file_pos = (int64_t)(filepositions->data.array.elements[i].data.number)-4;
  bgav_input_seek(ctx->input, file_pos, SEEK_SET);

  /* Resync */

  priv->resync = 1;
  
  while(1)
    {
    if(!next_packet_flv(ctx) ||
       bgav_track_has_sync(ctx->tt->cur))
      break;
    }
  priv->resync = 0;
  }
     

static int open_flv(bgav_demuxer_context_t * ctx)
  {
  int64_t pos;
  uint8_t flags;
  uint32_t data_offset, tmp;
  bgav_input_context_t * input_save = NULL;
  flv_priv_t * priv;
  flv_tag t;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  /* Create track */

  ctx->tt = bgav_track_table_create(1);
  ctx->tt->cur->duration = GAVL_TIME_UNDEFINED;
  
  /* Skip signature */
  bgav_input_skip(ctx->input, 4);

  /* Check what streams we have */
  if(!bgav_input_read_data(ctx->input, &flags, 1))
    goto fail;

  if(!flags)
    {
    bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Zero flags detected, assuming audio and video (might fail)");
    flags = 0x05;
    }
  
  if(flags & 0x04)
    add_audio_stream(ctx);
  if(flags & 0x01)
    add_video_stream(ctx);

  
  if(!bgav_input_read_32_be(ctx->input, &data_offset))
    goto fail;

  if(data_offset > ctx->input->position)
    {
    bgav_input_skip(ctx->input, data_offset - ctx->input->position);
    }

  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;

  ctx->index_mode = INDEX_MODE_MIXED;
  
  /* Get packets until we saw each stream at least once */
  priv->init = 1;

  if(!ctx->input->input->seek_byte)
    {
    input_save = ctx->input;
    ctx->input = bgav_input_open_as_buffer(ctx->input);
    }
  
  /* Sometimes, the header flags might report zero streams
     but we can parse the metadata to get this info */
  
  if(!next_packet_flv(ctx))
    goto fail;
  
  while(1)
    {
    if(!next_packet_flv(ctx))
      goto fail;
    if((!ctx->tt->cur->num_audio_streams || ctx->tt->cur->audio_streams[0].fourcc) &&
       (!ctx->tt->cur->num_video_streams || ctx->tt->cur->video_streams[0].fourcc) &&
       !priv->need_audio_extradata && !priv->need_video_extradata)
      break;
    }
  priv->init = 0;

  if(input_save)
    {
    bgav_input_close(ctx->input);
    bgav_input_destroy(ctx->input);
    ctx->input = input_save;
    }
  else
    {
    bgav_input_seek(ctx->input, ctx->data_start, SEEK_SET);
    }
  
  handle_metadata(ctx);

  if(ctx->tt->cur->num_video_streams)
    {
    if(ctx->tt->cur->duration != GAVL_TIME_UNDEFINED)
      {
      ctx->tt->cur->video_streams->index_mode = INDEX_MODE_SIMPLE;
      ctx->tt->cur->video_streams->duration = 0;
      }
    else
      ctx->index_mode = 0;
    }
  
  /* Get the duration from the timestamp of the last packet if
     the stream is seekable */
  
  if((ctx->tt->cur->duration == GAVL_TIME_UNDEFINED) &&
     ctx->input->input->seek_byte)
    {
    pos = ctx->input->position;
    bgav_input_seek(ctx->input, -4, SEEK_END);
    if(bgav_input_read_32_be(ctx->input, &tmp))
      {
      bgav_input_seek(ctx->input, -((int64_t)tmp+4), SEEK_END);
      
      if(flv_tag_read(ctx->input, &t))
        {
        ctx->tt->cur->duration =
          gavl_time_unscale(1000, t.timestamp);
        }
      else
        bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Getting duration from last timestamp failed");
      }
    bgav_input_seek(ctx->input, pos, SEEK_SET);
    }
  
  gavl_metadata_set(&ctx->tt->cur->metadata, 
                    GAVL_META_FORMAT, "FLV");

  return 1;
  
  fail:
  return 0;
  }

static void resync_flv(bgav_demuxer_context_t * ctx, bgav_stream_t * s)
  {
  flv_priv_t * priv;
  priv = ctx->priv;

  if(s->type == BGAV_STREAM_AUDIO)
    {
    priv->audio_sample_counter = s->in_position;
    }
  }  


static void close_flv(bgav_demuxer_context_t * ctx)
  {
  flv_priv_t * priv;
  priv = ctx->priv;

  free_meta_object(&priv->metadata);

  free(priv);
  }

const bgav_demuxer_t bgav_demuxer_flv =
  {
    .probe       = probe_flv,
    .open        = open_flv,
    .next_packet = next_packet_flv,
    .seek        = seek_flv,
    .resync      = resync_flv,
    .close       = close_flv
  };


