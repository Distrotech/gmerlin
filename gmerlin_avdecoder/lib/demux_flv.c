/*****************************************************************
 
  demux_flv.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

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
  meta_object_t metadata;
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

static void init_audio_stream(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * as = (bgav_stream_t*)0;
  as = bgav_track_add_audio_stream(ctx->tt->current_track, ctx->opt);
  as->stream_id = AUDIO_ID;
  as->timescale = 1000;
  }

static void init_video_stream(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * vs = (bgav_stream_t*)0;
  vs = bgav_track_add_video_stream(ctx->tt->current_track, ctx->opt);
  vs->vfr_timestamps = 1;
  
  vs->stream_id = VIDEO_ID;
  vs->timescale = 1000;
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
      //      fprintf(stderr, "Got number %s\n", ret->name);
      if(!bgav_input_read_64_be(input, (uint64_t*)&i64))
        return 0;
      ret->data.number = int2dbl(i64);
      break;
    case TYPE_BOOL:
      //      fprintf(stderr, "Got bool\n");
      if(!bgav_input_read_data(input, &ret->data.bool, 1))
        return 0;
      break;
    case TYPE_STRING:
      //      fprintf(stderr, "Got string\n");
      if(!bgav_input_read_16_be(input, &len))
        return 0;
      ret->data.string = malloc(len+1);
      if(bgav_input_read_data(input, (uint8_t*)ret->data.string, len) < len)
        return 0;
      ret->data.string[len] = '\0';
      break;
    case TYPE_OBJECT:
      //      fprintf(stderr, "Got Object\n");
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
      //      fprintf(stderr, "Got Array\n");
      
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
      //      fprintf(stderr, "Got Date\n");
      
      if(!bgav_input_read_64_be(input, (uint64_t*)&i64))
        return 0;
      ret->data.date.date1 = int2dbl(i64);

      if(!bgav_input_read_16_be(input, &ret->data.date.date2))
        return 0;
      break;
    case TYPE_TERMINATOR:
      //      fprintf(stderr, "Got Terminator\n");
      break;
    default:
      bgav_log(input->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Unknown type: %d for metadata object %s\n",
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
  return (meta_object_t*)0;
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
  
  priv = (flv_priv_t*)(ctx->priv);

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
  
  double number;
  uint8_t flags;
  uint8_t header[16];
  bgav_packet_t * p;
  bgav_stream_t * s;
  flv_priv_t * priv;
  flv_tag t;

  int packet_size = 0;
  int keyframe = 1;
  int adpcm_bits;
  uint8_t tmp_8;
  priv = (flv_priv_t *)(ctx->priv);
  
  /* Previous tag size (ignore this?) */
  bgav_input_skip(ctx->input, 4);
  
  if(!flv_tag_read(ctx->input, &t))
    return 0;

  if(t.type == 0x12)
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
    
    read_metadata(ctx, &t);

    obj = priv->metadata.data.object.children;
    num_obj = priv->metadata.data.object.num_children;
    
    /* Check if we can detect an audio stream */

    if(!ctx->tt->current_track->num_audio_streams)
      {
      if((meta_object_find_number(obj, num_obj, "audiodatarate", &number) && (number != 0.0)) ||
         (meta_object_find_number(obj, num_obj, "audiosamplerate", &number) && (number != 0.0)) ||
         (meta_object_find_number(obj, num_obj, "audiosamplesize", &number) && (number != 0.0)) ||
         (meta_object_find_number(obj, num_obj, "audiosize", &number) && (number != 0.0)))
        init_audio_stream(ctx);
      }
    if(!ctx->tt->current_track->num_video_streams)
      {
      if((meta_object_find_number(obj, num_obj, "videodatarate", &number) && (number != 0.0)) ||
         (meta_object_find_number(obj, num_obj, "width", &number) && (number != 0.0)) ||
         (meta_object_find_number(obj, num_obj, "height", &number) && (number != 0.0)))
        init_video_stream(ctx);
      }
    
    //    dump_meta_object(&priv->metadata, 0);
    //    bgav_input_skip_dump(ctx->input, t.data_size);
    return 1;
    }

  //  flv_tag_dump(&t);
  
  if(priv->init)
    s = bgav_track_find_stream_all(ctx->tt->current_track, t.type);
  else
    s = bgav_track_find_stream(ctx->tt->current_track, t.type);

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
      s->data.audio.bits_per_sample = (flags & 2) ? 16 : 8;
      
      if((flags >> 4) == 5)
        {
        s->data.audio.format.samplerate = 8000;
        s->data.audio.format.num_channels = 1;
        }
      else
        {
        s->data.audio.format.samplerate = (44100<<((flags>>2)&3))>>3;
        s->data.audio.format.num_channels = (flags&1)+1;
        }

      switch(flags >> 4)
        {
        case 0: /* Uncompressed, Big endian */
          s->fourcc = BGAV_MK_FOURCC('t', 'w', 'o', 's');
          break;
        case 1: /* Flash ADPCM */
          s->fourcc = BGAV_MK_FOURCC('F', 'L', 'A', '1');

          /* Set block align for the case, adpcm frames are split over
             several packets */
          if(!bgav_input_get_data(ctx->input, &tmp_8, 1))
            return 0;
          adpcm_bits = 2 +
            s->data.audio.format.num_channels * (((tmp_8 >> 6) + 2) * 4096 + 16 + 6);
          
          s->data.audio.block_align = (adpcm_bits + 7) / 8;
          break;
        case 2: /* MP3 */
          s->fourcc = BGAV_MK_FOURCC('.', 'm', 'p', '3');
          break;
        case 3: /* Uncompressed, Little endian */
          s->fourcc = BGAV_MK_FOURCC('s', 'o', 'w', 't');
          break;
        case 5: /* NellyMoser (unsupported) */
          s->fourcc = BGAV_MK_FOURCC('F', 'L', 'A', '2');
          break;
        default: /* Set some nonsense so we can finish initializing */
          s->fourcc = BGAV_MK_FOURCC('?', '?', '?', '?');
          bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Unknown audio codec tag: %d",
                   flags >> 4);
          break;
        }
      }
    packet_size = t.data_size - 1;
    }
  else if(s->stream_id == VIDEO_ID)
    {
    if(!bgav_input_read_data(ctx->input, &flags, 1))
      return 0;
    
    if(!s->fourcc) /* Initialize */
      {
      switch(flags & 0xF)
        {
        case 2: /* H263 based */
          s->fourcc = BGAV_MK_FOURCC('F', 'L', 'V', '1');
          break;
        case 3: /* Screen video (unsupported) */
          s->fourcc = BGAV_MK_FOURCC('F', 'L', 'V', 'S');
          break;
        case 4: /* VP6 (Flash 8?) */
          s->fourcc = BGAV_MK_FOURCC('V', 'P', '6', '2');
#if 1
          if(bgav_input_get_data(ctx->input, header, 5) < 5)
            return 0;
          /* Get the video size. The image is flipped vertically,
             so we set a negative height */
          s->data.video.format.image_height = -header[3] * 16;
          s->data.video.format.image_width  = header[4] * 16;
          
          s->data.video.format.frame_height = -s->data.video.format.image_height;
          s->data.video.format.frame_width  = s->data.video.format.image_width;
          
          /* Crop */
          s->data.video.format.image_width  -= (header[0] >> 4);
          s->data.video.format.image_height += header[0] & 0x0f;
          
#endif
          break;
        default: /* Set some nonsense so we can finish initializing */
          s->fourcc = BGAV_MK_FOURCC('?', '?', '?', '?');
          bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Unknown video codec tag: %d",
                   flags & 0x0f);
          break;
        }
      /* Set the framerate */
      s->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
      /* Set some nonsense values */
      s->data.video.format.timescale = 1000;
      s->data.video.format.frame_duration = 40;

      /* Hopefully, FLV supports square pixels only */
      s->data.video.format.pixel_width = 1;
      s->data.video.format.pixel_height = 1;
      
      }

    if(s->fourcc == BGAV_MK_FOURCC('V', 'P', '6', '2'))
      {
      bgav_input_skip(ctx->input, 1);
      packet_size = t.data_size - 2;
      }
    else
      packet_size = t.data_size - 1;
    if((flags >> 4) != 1)
      keyframe = 0;
    }

  if(!packet_size)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Got zero packet size (somethings wrong?)");
    return 0;
    }

  p = bgav_stream_get_packet_write(s);
  bgav_packet_alloc(p, packet_size);
  p->data_size = bgav_input_read_data(ctx->input, p->data, packet_size);

  if(p->data_size < packet_size) /* Got EOF in the middle of a packet */
    return 0; 
  
  p->pts = t.timestamp;

  if(s->time_scaled < 0)
    s->time_scaled = p->pts;

  p->keyframe = keyframe;
  bgav_packet_done_write(p);
  
  return 1;
  }

static void handle_metadata(bgav_demuxer_context_t * ctx)
  {
  double number;
  meta_object_t * obj, *obj1;
  int num_obj;
  flv_priv_t * priv;
  priv = (flv_priv_t *)(ctx->priv);
  
  obj = priv->metadata.data.object.children;
  num_obj = priv->metadata.data.object.num_children;

  /* Data rates */
  
  if(meta_object_find_number(obj, num_obj, "audiodatarate", &number) && (number != 0.0))
    ctx->tt->current_track->audio_streams[0].container_bitrate = (int)(number*1000);

  if(meta_object_find_number(obj, num_obj, "videodatarate", &number) && (number != 0.0))
    ctx->tt->current_track->video_streams[0].container_bitrate = (int)(number*1000);

  if(meta_object_find_number(obj, num_obj, "duration", &number) && (number != 0.0))
    ctx->tt->current_track->duration = gavl_seconds_to_time(number);

  if(ctx->input->input->seek_byte)
    {
    obj1 = meta_object_find(obj, num_obj, "keyframes");
    
    if(obj1 && (obj1->type == TYPE_OBJECT) &&
       meta_object_find(obj1->data.object.children, obj1->data.object.num_children,
                        "filepositions") &&
       meta_object_find(obj1->data.object.children, obj1->data.object.num_children,
                        "times"))
      ctx->can_seek = 1;
    }
  }

static void seek_flv(bgav_demuxer_context_t * ctx, gavl_time_t time)
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
  priv = (flv_priv_t *)(ctx->priv);
  
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
  time_float = gavl_time_to_seconds(time);
  for(i = times->data.array.num_elements-1; i >= 0; i--)
    {
    if(times->data.array.elements[i].data.number < time_float)
      break;
    }
  
  /* Seek to position */
  file_pos = (int64_t)(filepositions->data.array.elements[i].data.number)-4;
  bgav_input_seek(ctx->input,
                  file_pos,
                  SEEK_SET);

  /* Resync */

  while(1)
    {
    if(!next_packet_flv(ctx))
      {
      break;
      }
    if(ctx->tt->current_track->num_audio_streams &&
       ctx->tt->current_track->audio_streams[0].time_scaled < 0)
      continue;
    else if(ctx->tt->current_track->num_video_streams &&
            ctx->tt->current_track->video_streams[0].time_scaled < 0)
      continue;
    else
      break;
    }

  
  }
     

static int open_flv(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  int64_t pos;
  uint8_t flags;
  uint32_t data_offset, tmp;
  
  flv_priv_t * priv;
  flv_tag t;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  /* Create track */

  ctx->tt = bgav_track_table_create(1);

  /* Skip signature */
  bgav_input_skip(ctx->input, 4);

  /* Check what streams we have */
  if(!bgav_input_read_data(ctx->input, &flags, 1))
    goto fail;

  if(flags & 0x04)
    {
    init_audio_stream(ctx);
    }
  if(flags & 0x01)
    {
    init_video_stream(ctx);
    }
  if(!bgav_input_read_32_be(ctx->input, &data_offset))
    goto fail;

  if(data_offset > ctx->input->position)
    {
    bgav_input_skip(ctx->input, data_offset - ctx->input->position);
    }

  /* Get packets until we saw each stream at least once */
  priv->init = 1;

  /* Sometimes, the header flags might report zero streams
     but we can parse the metadata to get this info */
  
  if(!next_packet_flv(ctx))
    goto fail;
  
  while(1)
    {
    if(!next_packet_flv(ctx))
      goto fail;
    if((!ctx->tt->current_track->num_audio_streams ||
        ctx->tt->current_track->audio_streams[0].fourcc) &&
       (!ctx->tt->current_track->num_video_streams ||
        ctx->tt->current_track->video_streams[0].fourcc))
      break;
    }
  priv->init = 0;

  handle_metadata(ctx);
  
  /* Get the duration from the timestamp of the last packet if
     the stream is seekable */
  
  if((ctx->tt->current_track->duration != GAVL_TIME_UNDEFINED) &&
     ctx->input->input->seek_byte)
    {
    pos = ctx->input->position;
    bgav_input_seek(ctx->input, -4, SEEK_END);
    if(bgav_input_read_32_be(ctx->input, &tmp))
      {
      bgav_input_seek(ctx->input, -((int64_t)tmp+4), SEEK_END);
      
      if(flv_tag_read(ctx->input, &t))
        {
        ctx->tt->current_track->duration =
          gavl_time_unscale(1000, t.timestamp);
        }
      else
        bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Getting duration from last timestamp failed");
      }
    bgav_input_seek(ctx->input, pos, SEEK_SET);
    }
  
  ctx->stream_description = bgav_sprintf("Flash video (FLV)");
  
  return 1;
  
  fail:
  return 0;
  }

static void close_flv(bgav_demuxer_context_t * ctx)
  {
  flv_priv_t * priv;
  priv = (flv_priv_t *)(ctx->priv);

  free_meta_object(&priv->metadata);

  free(priv);
  }

bgav_demuxer_t bgav_demuxer_flv =
  {
    probe:       probe_flv,
    open:        open_flv,
    next_packet: next_packet_flv,
    seek:        seek_flv,
    close:       close_flv
  };


