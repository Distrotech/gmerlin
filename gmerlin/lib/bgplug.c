/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_MQ
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#define _XOPEN_SOURCE 600
#include <mqueue.h>
#include <errno.h>
#include <time.h>

#define MQ_NAME_MAX 32


#endif


#include <bgshm.h>

#include <gmerlin/plugin.h>

#include <gmerlin/bgplug.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "plug"

#define META_SHM_SIZE     "bgplug_shm_size"
#define META_RETURN_QUEUE "bgplug_return_queue"

// #define SHM_THRESHOLD 1024 // Minimum max_packet_size to switch to shm


typedef struct
  {
  int id;  /* ID of the segment */
  int len; /* Real length       */
  } shm_info_t;

typedef struct
  {
  gavf_stream_header_t * h;

  bg_shm_pool_t * sp;
  bg_shm_t * shm_segment;

  int shm_size; // Size of an shm seqment */
  
  /* Reading */
  gavl_packet_source_t * src_int;
  gavl_packet_source_t * src_ext;

  /* Writing */
  gavl_packet_sink_t * sink_ext;
  gavl_packet_sink_t * sink_int;
  
  bg_plug_t * plug;

  gavl_packet_t p_shm;
  gavl_packet_t * p_ext;
  
  int index; // Index inside the program header

  /* Audio stuff */
  gavl_audio_frame_t * aframe;
  gavl_audio_source_t * asrc;
  gavl_audio_sink_t   * asink;
  gavl_audio_format_t afmt;
  
  /* Video stuff */
  gavl_video_frame_t * vframe;
  gavl_video_source_t * vsrc;
  gavl_video_sink_t   * vsink;
  gavl_video_format_t vfmt;
  gavl_compression_info_t ci;

  gavl_metadata_t m;
  uint32_t timescale;
  
  bg_plugin_handle_t * codec_handle;
  bg_stream_action_t action;
  } stream_t;

struct bg_plug_s
  {
  int wr;
  gavf_t * g;
  gavf_io_t * io;
  gavf_program_header_t * ph;
  
  int num_audio_streams;
  stream_t * audio_streams;

  int num_video_streams;
  stream_t * video_streams;

  int num_text_streams;
  stream_t * text_streams;

  int num_overlay_streams;
  stream_t * overlay_streams;
  int io_flags;  
  
  bg_plugin_registry_t * plugin_reg;
  gavf_options_t * opt;

  const bg_parameter_info_t * ac_params;
  const bg_parameter_info_t * vc_params;
  const bg_parameter_info_t * oc_params;

  pthread_mutex_t mutex;
  
  gavl_packet_t skip_packet;
  
  int got_error;
  int shm_threshold;
  
#ifdef HAVE_MQ
  mqd_t mq;
  int mq_id;

  int packets;
#endif
  };

/* Message queue stuff */

#ifdef HAVE_MQ
static void gen_mq_name(int id, char * ret)
  {
  snprintf(ret, MQ_NAME_MAX, "/gmerlin-mq-%08x", id);
  }

static void create_messate_queue(bg_plug_t * p, gavl_metadata_t * m)
  {
  char name[128];
  struct mq_attr attr =
    {
      .mq_flags = 0,             /* Flags: 0 or O_NONBLOCK */
      .mq_maxmsg = 2,            /* Max. # of messages on queue */
      .mq_msgsize = sizeof(int), /* Max. message size (bytes) */
      .mq_curmsgs = 0,           /* # of messages currently in queue */
    };
 
  p->mq_id = 0;
  
  if(p->wr)
    {
    while(1)
      {
      p->mq_id++;
    
      gen_mq_name(p->mq_id, name);
    
      if((p->mq = mq_open(name, O_RDONLY | O_CREAT | O_EXCL,
                           S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, &attr)) < 0)
        {
        if(errno != EEXIST)
          {
          bg_log(BG_LOG_ERROR, LOG_DOMAIN,
                 "mq_open of %s failed: %s", name, strerror(errno));
          return;
          }
        }
      else
        break;
      }
    gavl_metadata_set_int(m, META_RETURN_QUEUE, p->mq_id);
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN,
           "Opened message queue %s for reading", name);

    }
  else
    {
    if(!gavl_metadata_get_int(m, META_RETURN_QUEUE, &p->mq_id))
      return;

    gavl_metadata_set(m, META_RETURN_QUEUE, NULL); /* Clear this */
    gen_mq_name(p->mq_id, name);

    if((p->mq = mq_open(name, O_WRONLY)) < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "mq_open of %s failed: %s", name, strerror(errno));
      return;
      }
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN,
           "Opened message queue %s for writing", name);
    /* It's save to unlink the queue now */
    mq_unlink(name);
    }
  
  }

static int io_cb_read(void * priv, int type, const void * data)
  {
  bg_plug_t * p = priv;
  /* Write confirmation */

  //  fprintf(stderr, "io_cb_read %d %d\n", type, p->packets++);

  if((type == GAVF_IO_CB_PROGRAM_HEADER) && (p->mq < 0))
    create_messate_queue(p, &p->ph->m);
#if 0  
  if(type == GAVF_IO_CB_PACKET)
    {
    fprintf(stderr, "Got packet\n");
    if(data)
      gavl_packet_dump(data);
    }
#endif

  if(p->mq < 0)
    return 1;
  
  if(mq_send(p->mq, (const char *)&type, sizeof(type), 0))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "mq_send failed: %s",
           strerror(errno));
    p->got_error = 1;
    return 0;
    }
  return 1;
  }

static int io_cb_write(void * priv, int type, const void * data)
  {
  int type_ret;
  ssize_t size_ret;
  bg_plug_t * p = priv;
  unsigned msg_prio;

  struct timespec timeout;

  clock_gettime(CLOCK_REALTIME, &timeout);
  timeout.tv_sec += 1;
  
  /* Read confirmation */
  
  // fprintf(stderr, "io_cb_write type: %d %d\n", type, p->packets++);

  //  if((type == GAVF_IO_CB_PACKET) && data)
  //    gavl_packet_dump(data);
  
  size_ret =
    mq_timedreceive(p->mq, (char *)&type_ret,
                    sizeof(type_ret), &msg_prio, &timeout);

  if(size_ret < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Reading message failed: %s",
           strerror(errno));
    p->got_error = 1;
    }
  
  if((size_ret != sizeof(type_ret)) ||
     (type_ret != type))
    return 0;
  return 1;
  }

#endif

void bg_plug_set_compressor_config(bg_plug_t * p,
                                   const bg_parameter_info_t * ac_params,
                                   const bg_parameter_info_t * vc_params,
                                   const bg_parameter_info_t * oc_params)
  {
  p->ac_params = ac_params;
  p->vc_params = vc_params;
  p->oc_params = oc_params;
  }

static bg_plug_t * create_common()
  {
  bg_plug_t * ret = calloc(1, sizeof(*ret));
  ret->g = gavf_create();
  ret->ph = gavf_get_program_header(ret->g);
  ret->opt = gavf_get_options(ret->g);

#ifdef HAVE_MQ
  ret->mq = -1;
#endif
  
  //  gavf_options_set_flags(ret->opt,
  //                         GAVF_OPT_FLAG_DUMP_HEADERS |
  //                         GAVF_OPT_FLAG_DUMP_METADATA);
  
  pthread_mutex_init(&ret->mutex, NULL);

  return ret;
  }

static void lock_func(void * priv)
  {
  bg_plug_t * p = priv;
  pthread_mutex_lock(&p->mutex);
  }

static void unlock_func(void * priv)
  {
  bg_plug_t * p = priv;
  pthread_mutex_unlock(&p->mutex);
  }

bg_plug_t * bg_plug_create_reader(bg_plugin_registry_t * plugin_reg)
  {
  bg_plug_t * ret;
  ret = create_common();
  ret->plugin_reg = plugin_reg;
  return ret;
  }

bg_plug_t * bg_plug_create_writer(bg_plugin_registry_t * plugin_reg)
  {
  bg_plug_t * ret = bg_plug_create_reader(plugin_reg);
  ret->wr = 1;
  return ret;
  }

static void flush_streams(stream_t * streams, int num)
  {
  int i;
  stream_t * s;
  for(i = 0; i < num; i++)
    {
    s = streams + i;

    /* Flush the codecs, might write some final packets */
    if(s->codec_handle)
      bg_plugin_unref(s->codec_handle);
    else
      {
      /* Sinks and sources are owned by the codec if there is one */
      if(s->asink)
        gavl_audio_sink_destroy(s->asink);
      if(s->vsink)
        gavl_video_sink_destroy(s->vsink);
      if(s->asrc)
        gavl_audio_source_destroy(s->asrc);
      if(s->vsrc)
        gavl_video_source_destroy(s->vsrc);
      }
    }
  }

static void free_streams(stream_t * streams, int num)
  {
  int i;
  stream_t * s;

  for(i = 0; i < num; i++)
    {
    s = streams + i;
    if(s->sp)
      bg_shm_pool_destroy(s->sp);

    if(s->src_ext && (s->src_ext != s->src_int))
      gavl_packet_source_destroy(s->src_ext);
    if(s->sink_ext && (s->sink_ext != s->sink_int))
      gavl_packet_sink_destroy(s->sink_ext);
    
    if(s->aframe)
      {
      gavl_audio_frame_null(s->aframe);
      gavl_audio_frame_destroy(s->aframe);
      }
    if(s->vframe)
      {
      gavl_video_frame_null(s->vframe);
      gavl_video_frame_destroy(s->vframe);
      }
    gavl_compression_info_free(&s->ci);
    gavl_metadata_free(&s->m);
    }
  if(streams)
    free(streams);
  }

void bg_plug_destroy(bg_plug_t * p)
  {
  flush_streams(p->audio_streams, p->num_audio_streams);
  flush_streams(p->video_streams, p->num_video_streams);
  flush_streams(p->text_streams, p->num_text_streams);
  flush_streams(p->overlay_streams, p->num_overlay_streams);
  
  gavf_close(p->g);
  if(p->io)
    gavf_io_destroy(p->io);

  free_streams(p->audio_streams, p->num_audio_streams);
  free_streams(p->video_streams, p->num_video_streams);
  free_streams(p->text_streams, p->num_text_streams);
  free_streams(p->overlay_streams, p->num_overlay_streams);

#if HAVE_MQ
  if(p->mq >= 0)
    mq_close(p->mq);
#endif
  
  gavl_packet_free(&p->skip_packet);
  
  pthread_mutex_destroy(&p->mutex);

  free(p);
  }

static const bg_parameter_info_t input_parameters[] =
  {
    {
      .name = "dh",
      .long_name = TRS("Dump gavf headers"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .help_string = TRS("Use this for debugging"),
    },
    {
      .name = "dp",
      .long_name = TRS("Dump gavf packets"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .help_string = TRS("Use this for debugging"),
    },
    {
      .name = "dm",
      .long_name = TRS("Dump inline metadata"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .help_string = TRS("Use this for debugging"),
    },
    { /* End */ },
  };

static const bg_parameter_info_t output_parameters[] =
  {
    {
      .name = "shm",
      .long_name = TRS("SHM threshold"),
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 1024 },
      .help_string = TRS("Select the minimum packet size for using shared memory for a stream. Non-positive values disable shared memory completely"),
    },
    { /* End */ }
  };

const bg_parameter_info_t *
bg_plug_get_input_parameters()
  {
  return input_parameters;
  }

const bg_parameter_info_t *
bg_plug_get_output_parameters()
  {
  return output_parameters;
  }

#define SET_GAVF_FLAG(f) \
  int flags = gavf_options_get_flags(p->opt); \
  if(val->val_i) \
    flags |= f; \
  else \
    flags &= ~f; \
  gavf_options_set_flags(p->opt, flags);

void bg_plug_set_parameter(void * data, const char * name,
                           const bg_parameter_value_t * val)
  {
  bg_plug_t * p = data;
  
  if(!name)
    return;
  else if(!strcmp(name, "dh"))
    {
    SET_GAVF_FLAG(GAVF_OPT_FLAG_DUMP_HEADERS);
    }
  else if(!strcmp(name, "dp"))
    {
    SET_GAVF_FLAG(GAVF_OPT_FLAG_DUMP_PACKETS);
    }
  else if(!strcmp(name, "dm"))
    {
    SET_GAVF_FLAG(GAVF_OPT_FLAG_DUMP_METADATA);
    }
  else if(!strcmp(name, "shm"))
    p->shm_threshold = val->val_i;
  }

#undef SET_GAVF_FLAG

/* Read/write */

static void init_streams_read(bg_plug_t * p)
  {
  int i;
  int audio_idx = 0;
  int video_idx = 0;
  int text_idx = 0;
  int overlay_idx = 0;
  stream_t * s;

  for(i = 0; i < p->ph->num_streams; i++)
    {
    switch(p->ph->streams[i].type)
      {
      case GAVF_STREAM_AUDIO:
        p->num_audio_streams++;
        break;
      case GAVF_STREAM_VIDEO:
        p->num_video_streams++;
        break;
      case GAVF_STREAM_TEXT:
        p->num_text_streams++;
        break;
      case GAVF_STREAM_OVERLAY:
        p->num_overlay_streams++;
        break;
      }
    }

  if(p->num_audio_streams)
    p->audio_streams =
      calloc(p->num_audio_streams, sizeof(*p->audio_streams));

  if(p->num_video_streams)
    p->video_streams =
      calloc(p->num_video_streams, sizeof(*p->video_streams));

  if(p->num_text_streams)
    p->text_streams =
      calloc(p->num_text_streams, sizeof(*p->text_streams));

  if(p->num_overlay_streams)
    p->overlay_streams =
      calloc(p->num_overlay_streams, sizeof(*p->overlay_streams));
  
  
  /* Initialize streams for a reader, the action can be changed
     later on */
  
  for(i = 0; i < p->ph->num_streams; i++)
    {
    switch(p->ph->streams[i].type)
      {
      case GAVF_STREAM_AUDIO:
        {
        s = p->audio_streams + audio_idx;
        s->h = p->ph->streams + i;
        s->index = i;
        s->plug = p;
        if(s->h->ci.id == GAVL_CODEC_ID_NONE)
          s->action = BG_STREAM_ACTION_DECODE;
        else
          s->action = BG_STREAM_ACTION_READRAW;
        
        audio_idx++;
        }
        break;
      case GAVF_STREAM_VIDEO:
        {
        s = p->video_streams + video_idx;
        s->h = p->ph->streams + i;
        s->index = i;
        s->plug = p;

        if(s->h->ci.id == GAVL_CODEC_ID_NONE)
          s->action = BG_STREAM_ACTION_DECODE;
        else
          s->action = BG_STREAM_ACTION_READRAW;

        video_idx++;
        }
        break;
      case GAVF_STREAM_TEXT:
        {
        s = p->text_streams + text_idx;
        s->h = p->ph->streams + i;
        s->index = i;
        s->plug = p;

        if(s->h->ci.id == GAVL_CODEC_ID_NONE)
          s->action = BG_STREAM_ACTION_DECODE;
        else
          s->action = BG_STREAM_ACTION_READRAW;
        
        text_idx++;
        }
        break;
      case GAVF_STREAM_OVERLAY:
        {
        s = p->overlay_streams + overlay_idx;
        s->h = p->ph->streams + i;
        s->index = i;
        s->plug = p;

        if(s->h->ci.id == GAVL_CODEC_ID_NONE)
          s->action = BG_STREAM_ACTION_DECODE;
        else
          s->action = BG_STREAM_ACTION_READRAW;
        
        overlay_idx++;
        }
        break;
      }
    }
  }

/* Read support */

static gavl_source_status_t read_packet_shm(void * priv,
                                            gavl_packet_t ** ret)
  {
  shm_info_t si;
  gavl_source_status_t st;
  gavl_packet_t * p = NULL;
  stream_t * s = priv;

  if((st = gavl_packet_source_read_packet(s->src_int, &p)) !=
     GAVL_SOURCE_OK)
    return st;

  /* Sanity check */
  if(p->data_len != sizeof(si))
    return GAVL_SOURCE_EOF;

  memcpy(&si, p->data, sizeof(si));

  /* Unref the old segment if there is one */
  if(s->shm_segment)
    bg_shm_unref(s->shm_segment);
  
  /* Obtain memory segment */
  s->shm_segment = bg_shm_pool_get_read(s->sp, si.id);

  if(!s->shm_segment)
    return GAVL_SOURCE_EOF;
  
  /* Copy metadata */
  
  memcpy(&s->p_shm, p, sizeof(*p));
  
  /* Exchange pointers */
  s->p_shm.data       = bg_shm_get_buffer(s->shm_segment, &s->p_shm.data_alloc);
  s->p_shm.data_len   = si.len;
  
  *ret = &s->p_shm;
  return GAVL_SOURCE_OK;
  }

static void skip_shm_packet(gavf_t * gavf,
                            const gavf_stream_header_t * h,
                            void * priv)
  {
  shm_info_t si;
  stream_t * s = priv;
  
  if(!gavf_packet_read_packet(s->plug->g, &s->plug->skip_packet))
    return;
  
  /* Sanity check */
  if(s->plug->skip_packet.data_len != sizeof(si))
    return;
  
  memcpy(&si, s->plug->skip_packet.data, sizeof(si));
  
  /* Obtain memory segment */
  s->shm_segment = bg_shm_pool_get_read(s->sp, si.id);
  
  /* Unref the segment if there is one */
  if(s->shm_segment)
    bg_shm_unref(s->shm_segment);
  
  }

static int init_read_common(bg_plug_t * p,
                            stream_t * s)
  {
  gavl_metadata_get_int(&s->h->m, META_SHM_SIZE, &s->shm_size);
  
  if(s->shm_size)
    {
    /* Create shared memory pool */
    s->sp = bg_shm_pool_create(s->shm_size, 0);
    /* Clear metadata tags */
    gavl_metadata_set(&s->h->m, META_SHM_SIZE, NULL);

    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Using shm for reading (segment size: %d)",
           s->shm_size);
    }

  if(s->action == BG_STREAM_ACTION_OFF)
    {
    gavf_stream_set_skip(p->g, s->h->id, s->shm_size ? skip_shm_packet : NULL, s);
    return 0;
    }
  
  s->src_int = gavf_get_packet_source(p->g, s->h->id);
  gavl_packet_source_set_lock_funcs(s->src_int, lock_func, unlock_func, p);

  if(s->shm_size)
    s->src_ext =
      gavl_packet_source_create_source(read_packet_shm, s,
                                       GAVL_SOURCE_SRC_ALLOC,
                                       s->src_int);
  else
    s->src_ext = s->src_int;
  return 1;
  }

/* Uncompressed source funcs */
static gavl_source_status_t read_audio_func(void * priv,
                                            gavl_audio_frame_t ** f)
  {
  gavl_source_status_t st;
  gavl_packet_t * p = NULL;
  stream_t * s = priv;

  if((st = gavl_packet_source_read_packet(s->src_ext, &p))
     != GAVL_SOURCE_OK)
    return st;

  gavf_packet_to_audio_frame(p, s->aframe,
                             &s->h->format.audio);
  *f = s->aframe;
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t read_video_func(void * priv,
                                            gavl_video_frame_t ** f)
  {
  gavl_source_status_t st;
  gavl_packet_t * p = NULL;
  stream_t * s = priv;
  // fprintf(stderr, "Read video func\n");
  if((st = gavl_packet_source_read_packet(s->src_ext, &p))
     != GAVL_SOURCE_OK)
    return st;
  
  gavf_packet_to_video_frame(p, s->vframe,
                             &s->h->format.video);
  *f = s->vframe;

  // fprintf(stderr, "Read video func done\n");
  
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t read_overlay_func(void * priv,
                                              gavl_video_frame_t ** f)
  {
  gavl_source_status_t st;
  gavl_packet_t * p = NULL;
  stream_t * s = priv;
  // fprintf(stderr, "Read video func\n");
  if((st = gavl_packet_source_read_packet(s->src_ext, &p))
     != GAVL_SOURCE_OK)
    return st;
  gavf_packet_to_overlay(p, *f, &s->h->format.video);
  // fprintf(stderr, "Read video func done\n");
  return GAVL_SOURCE_OK;
  }

static bg_plugin_handle_t * load_decompressor(bg_plugin_registry_t * plugin_reg,
                                              gavl_codec_id_t id,
                                              int flag_mask)
  {
  /* Add decoder */
  const bg_plugin_info_t * info;
  bg_plugin_handle_t * ret;
  info = bg_plugin_find_by_compression(plugin_reg, id,
                                       BG_PLUGIN_CODEC,
                                       flag_mask);
  if(!info)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cannot open %s decompressor for %s",
           (flag_mask == BG_PLUGIN_VIDEO_DECOMPRESSOR ? "video" : "audio"),
           gavl_compression_get_short_name(id));
    return NULL;
    }
  ret = bg_plugin_load(plugin_reg, info);
  if(!ret)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cannot load %s decompressor for %s",
           (flag_mask == BG_PLUGIN_VIDEO_DECOMPRESSOR ? "video" : "audio"),
           gavl_compression_get_short_name(id));
    return NULL;
    }
  return ret;
  }

static int init_read(bg_plug_t * p)
  {
  int i; 
  stream_t * s;
  
  //  init_streams_read(p);

  for(i = 0; i < p->num_audio_streams; i++)
    {
    s = p->audio_streams + i;
    
    if(!init_read_common(p, s))
      continue; // Stream is switched off
    
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->asrc = gavl_audio_source_create(read_audio_func, s,
                                         GAVL_SOURCE_SRC_ALLOC,
                                         &s->h->format.audio);
      s->aframe = gavl_audio_frame_create(NULL);
      }
    else if(s->action == BG_STREAM_ACTION_DECODE)
      {
      bg_codec_plugin_t * codec;
      /* Add decoder */
      if(!(s->codec_handle = load_decompressor(p->plugin_reg,
                                               s->h->ci.id,
                                               BG_PLUGIN_AUDIO_DECOMPRESSOR)))
        return 0;

      codec = (bg_codec_plugin_t*)s->codec_handle->plugin;

      s->asrc = codec->connect_decode_audio(s->codec_handle->priv,
                                            s->src_ext,
                                            &s->h->ci,
                                            &s->h->format.audio,
                                            &s->h->m);
      }
    }

  for(i = 0; i < p->num_video_streams; i++)
    {
    s = p->video_streams + i;

    if(!init_read_common(p, s))
      continue;
    
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->vsrc =
        gavl_video_source_create(read_video_func, s,
                                 GAVL_SOURCE_SRC_ALLOC,
                                 &s->h->format.video);
      s->vframe = gavl_video_frame_create(NULL);
      }
    else if(s->action == BG_STREAM_ACTION_DECODE)
      {
      bg_codec_plugin_t * codec;
      /* Add decoder */
      if(!(s->codec_handle = load_decompressor(p->plugin_reg,
                                               s->h->ci.id,
                                               BG_PLUGIN_VIDEO_DECOMPRESSOR)))
        return 0;
      codec = (bg_codec_plugin_t*)s->codec_handle->plugin;
      
      s->vsrc = codec->connect_decode_video(s->codec_handle->priv,
                                            s->src_ext,
                                            &s->h->ci,
                                            &s->h->format.video,
                                            &s->h->m);
      }
    }
  for(i = 0; i < p->num_overlay_streams; i++)
    {
    s = p->overlay_streams + i;

    if(!init_read_common(p, s))
      continue;
    
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->vsrc =
        gavl_video_source_create(read_overlay_func, s,
                                 0, &s->h->format.video);
      s->vframe = gavl_video_frame_create(NULL);
      }
    else if(s->action == BG_STREAM_ACTION_DECODE)
      {
      bg_codec_plugin_t * codec;
      /* Add decoder */
      if(!(s->codec_handle = load_decompressor(p->plugin_reg,
                                               s->h->ci.id,
                                               BG_PLUGIN_VIDEO_DECOMPRESSOR)))
        return 0;
      codec = (bg_codec_plugin_t*)s->codec_handle->plugin;
      
      s->vsrc = codec->connect_decode_video(s->codec_handle->priv,
                                            s->src_ext,
                                            &s->h->ci,
                                            &s->h->format.video,
                                            &s->h->m);
      }
    }
  
  for(i = 0; i < p->num_text_streams; i++)
    {
    s = p->text_streams + i;
    if(!init_read_common(p, s))
      continue;
    }
  return 1;
  }

/* Uncompressed Sink funcs */

static gavl_audio_frame_t * get_audio_func(void * priv)
  {
  stream_t * as = priv;
  as->p_ext = gavl_packet_sink_get_packet(as->sink_ext);
  gavl_packet_alloc(as->p_ext, as->h->ci.max_packet_size);
  as->aframe->valid_samples = as->h->format.audio.samples_per_frame;
  gavl_audio_frame_set_channels(as->aframe, &as->h->format.audio,
                                as->p_ext->data);
  return as->aframe;
  }

static gavl_sink_status_t put_audio_func(void * priv,
                                         gavl_audio_frame_t * f)
  {
  stream_t * as = priv;
  gavf_audio_frame_to_packet_metadata(f, as->p_ext);
  as->p_ext->data_len = as->h->ci.max_packet_size;
  gavf_shrink_audio_frame(as->aframe, as->p_ext, &as->h->format.audio);

#ifdef DUMP_PACKETS
  fprintf(stderr, "Got audio packet\n");
  gavl_packet_dump(as->p_ext);
#endif
  return gavl_packet_sink_put_packet(as->sink_ext, as->p_ext);
  }

static gavl_video_frame_t * get_video_func(void * priv)
  {
  stream_t * vs = priv;
  vs->p_ext = gavl_packet_sink_get_packet(vs->sink_ext);
  gavl_packet_alloc(vs->p_ext, vs->h->ci.max_packet_size);
  gavl_video_frame_set_planes(vs->vframe, &vs->h->format.video,
                              vs->p_ext->data);
  return vs->vframe;
  }

static gavl_sink_status_t put_video_func(void * priv,
                                         gavl_video_frame_t * f)
  {
  stream_t * vs = priv;
  gavf_video_frame_to_packet_metadata(f, vs->p_ext);
  vs->p_ext->data_len = vs->h->ci.max_packet_size;
#ifdef DUMP_PACKETS
  fprintf(stderr, "Got video packet\n");
  gavl_packet_dump(vs->p_ext);
#endif
  return gavl_packet_sink_put_packet(vs->sink_ext, vs->p_ext);;
  }

static gavl_sink_status_t put_overlay_func(void * priv,
                                           gavl_video_frame_t * f)
  {
  stream_t * vs = priv;
  vs->p_ext = gavl_packet_sink_get_packet(vs->sink_ext);
  gavl_packet_alloc(vs->p_ext, vs->h->ci.max_packet_size);
  gavf_overlay_to_packet(f, vs->p_ext, &vs->h->format.video);
#ifdef DUMP_PACKETS
  fprintf(stderr, "Got video packet\n");
  gavl_packet_dump(vs->p_ext);
#endif
  return gavl_packet_sink_put_packet(vs->sink_ext, vs->p_ext);;
  }



/* Packet */

static gavl_packet_t * get_packet_shm(void * priv)
  {
  stream_t * s = priv;

  s->shm_segment = bg_shm_pool_get_write(s->sp);
  
  s->p_shm.data = bg_shm_get_buffer(s->shm_segment, &s->p_shm.data_alloc);
  gavl_packet_reset(&s->p_shm);
  return &s->p_shm;
  }

static gavl_sink_status_t put_packet_shm(void * priv, gavl_packet_t * pp)
  {
  gavl_packet_t * p;
  shm_info_t si;
  stream_t * s = priv;

  /* Exchange pointers */
  si.id = bg_shm_get_id(s->shm_segment);
  si.len = s->p_shm.data_len;

  p = gavl_packet_sink_get_packet(s->sink_int);
  
  gavl_packet_copy_metadata(p, &s->p_shm);
  gavl_packet_alloc(p, sizeof(si));
  memcpy(p->data, &si, sizeof(si));
  p->data_len = sizeof(si);
  return gavl_packet_sink_put_packet(s->sink_int, p);
  }

static void check_shm_write(bg_plug_t * p, stream_t * s)
  {
#ifdef HAVE_MQ
  if((p->io_flags & BG_PLUG_IO_IS_LOCAL) &&
     (p->shm_threshold > 0) &&
     (s->ci.max_packet_size > p->shm_threshold))
    {
    s->shm_size = s->ci.max_packet_size + GAVL_PACKET_PADDING;
    s->sp = bg_shm_pool_create(s->shm_size, 1);
    gavl_metadata_set_int(&s->m, META_SHM_SIZE, s->shm_size);
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Using shm for writing (segment size: %d)",
           s->shm_size);

    if(p->mq < 0)
      {
      create_messate_queue(p, &p->ph->m);
      if(p->mq >= 0)
        gavf_io_set_cb(p->io, io_cb_write, p);
      }
    
    }


#endif
  }

static int init_write_common(bg_plug_t * p, stream_t * s)
  {
  s->sink_int = gavf_get_packet_sink(p->g, s->h->id);
  gavl_packet_sink_set_lock_funcs(s->sink_int, lock_func, unlock_func, p);
  
  if(s->shm_size)
    {
    s->sink_ext = gavl_packet_sink_create(get_packet_shm,
                                          put_packet_shm,
                                          s);
    }
  else
    s->sink_ext = s->sink_int;

  if(s->codec_handle)
    {
    bg_codec_plugin_t * codec = (bg_codec_plugin_t*)s->codec_handle->plugin;
    codec->set_packet_sink(s->codec_handle->priv, s->sink_ext);
    }
  
  return 1;
  }

static int init_write(bg_plug_t * p)
  {
  int i;
  stream_t * s;

  /* 1. Set up compressors and add gavf streams */
  for(i = 0; i < p->num_audio_streams; i++)
    {
    s = p->audio_streams + i;

    if(s->codec_handle)
      {
      bg_codec_plugin_t * codec = (bg_codec_plugin_t*)s->codec_handle->plugin;

      s->asink = codec->open_encode_audio(s->codec_handle->priv,
                                          &s->ci, &s->afmt, &s->m);
      if(!s->asink)
        return 0;
      }

    s->ci.max_packet_size = gavf_get_max_audio_packet_size(&s->afmt, &s->ci);
    check_shm_write(p, s);
    
    if((s->index = gavf_add_audio_stream(p->g, &s->ci, &s->afmt, &s->m)) < 0)
      return 0;
    }
  for(i = 0; i < p->num_video_streams; i++)
    {
    s = p->video_streams + i;

    if(s->codec_handle)
      {
      bg_codec_plugin_t * codec = (bg_codec_plugin_t*)s->codec_handle->plugin;
      s->vsink = codec->open_encode_video(s->codec_handle->priv,
                                          &s->ci, &s->vfmt, &s->m);
      if(!s->vsink)
        return 0;
      }

    s->ci.max_packet_size = gavf_get_max_video_packet_size(&s->vfmt, &s->ci);
    check_shm_write(p, s);

    if((s->index = gavf_add_video_stream(p->g, &s->ci, &s->vfmt, &s->m)) < 0)
      return 0;
    }
  for(i = 0; i < p->num_overlay_streams; i++)
    {
    s = p->overlay_streams + i;

    if(s->codec_handle)
      {
      bg_codec_plugin_t * codec = (bg_codec_plugin_t*)s->codec_handle->plugin;
      s->vsink = codec->open_encode_overlay(s->codec_handle->priv,
                                            &s->ci, &s->vfmt, &s->m);
      if(!s->vsink)
        return 0;
      }

    s->ci.max_packet_size = gavf_get_max_video_packet_size(&s->vfmt, &s->ci);
    check_shm_write(p, s);

    if((s->index = gavf_add_overlay_stream(p->g, &s->ci, &s->vfmt, &s->m)) < 0)
      return 0;
    }
  for(i = 0; i < p->num_text_streams; i++)
    {
    s = p->text_streams + i;
    check_shm_write(p, s);
    if((s->index = gavf_add_text_stream(p->g, s->timescale, &s->m)) < 0)
      return 0;
    }

  /* 2. Now all streams are added, set the pointers to the stream headers */  
  for(i = 0; i < p->num_audio_streams; i++)
    {
    s = p->audio_streams + i;
    s->h = p->ph->streams + s->index;
    }
  for(i = 0; i < p->num_video_streams; i++)
    {
    s = p->video_streams + i;
    s->h = p->ph->streams + s->index;
    }
  for(i = 0; i < p->num_text_streams; i++)
    {
    s = p->text_streams + i;
    s->h = p->ph->streams + s->index;
    }
  for(i = 0; i < p->num_overlay_streams; i++)
    {
    s = p->overlay_streams + i;
    s->h = p->ph->streams + s->index;
    }
  
  if(!gavf_start(p->g))
    return 0;
  
  /* Create sinks */
  for(i = 0; i < p->num_audio_streams; i++)
    {
    s = p->audio_streams + i;
    init_write_common(p, s);
    
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->asink = gavl_audio_sink_create(get_audio_func,
                                        put_audio_func,
                                        s, &s->h->format.audio);
      s->aframe = gavl_audio_frame_create(NULL);
      }
    }
  for(i = 0; i < p->num_video_streams; i++)
    {
    s = p->video_streams + i;
    init_write_common(p, s);
    
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->vsink = gavl_video_sink_create(get_video_func,
                                        put_video_func,
                                       s, &s->h->format.video);
      s->vframe = gavl_video_frame_create(NULL);
      }
    }
  for(i = 0; i < p->num_overlay_streams; i++)
    {
    s = p->overlay_streams + i;
    init_write_common(p, s);
    
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->vsink = gavl_video_sink_create(NULL,
                                        put_overlay_func,
                                        s, &s->h->format.video);
      s->vframe = gavl_video_frame_create(NULL);
      }
    }
  for(i = 0; i < p->num_text_streams; i++)
    {
    s = p->text_streams + i;
    init_write_common(p, s);
    }
  
  return 1;
  }

int bg_plug_start(bg_plug_t * p)
  {
  if(!p->io)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot start %s plug (call open first)",
           (p->wr ? "output" : "input"));
    return 0;
    }
  
  if(p->wr)
    return init_write(p);
  else
    return init_read(p);
  }

int bg_plug_open(bg_plug_t * p, gavf_io_t * io,
                 const gavl_metadata_t * m,
                 const gavl_chapter_list_t * cl)
  {
  int flags;
  
  p->io = io;
  
  if(p->wr)
    {
    if(p->io_flags & BG_PLUG_IO_IS_REGULAR)
      {
      /* Force writing a sync index if we write to a regular file */
      flags = gavf_options_get_flags(p->opt);
      flags |= GAVF_OPT_FLAG_SYNC_INDEX;
      gavf_options_set_flags(p->opt, flags);
      }
    
    if(!gavf_open_write(p->g, p->io, m, cl))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "gavf_open_write failed");
      return 0;
      }
    return 1;
    }

  flags = gavf_options_get_flags(p->opt);
  flags |= GAVF_OPT_FLAG_BUFFER_READ;
  gavf_options_set_flags(p->opt, flags);

#ifdef HAVE_MQ
  gavf_io_set_cb(p->io, io_cb_read, p);
#endif
  
  if(!gavf_open_read(p->g, p->io))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "gavf_open_read failed");
    return 0;
    }
  
  init_streams_read(p);
  return 1;
  }

int bg_plug_open_location(bg_plug_t * p, const char * location,
                          const gavl_metadata_t * m,
                          const gavl_chapter_list_t * cl)
  {
  gavf_io_t * io = bg_plug_io_open_location(location, p->wr,
                                            &p->io_flags);
  if(io)
    {
    if(!bg_plug_open(p, io, m, cl))
      return 0;
    else
      return 1;
    }
  else
    return 0;
  }

gavf_t * bg_plug_get_gavf(bg_plug_t * p)
  {
  return p->g;
  }


const gavf_stream_header_t * bg_plug_next_packet_header(bg_plug_t * p)
  {
  int i;
  const gavf_packet_header_t * ph = gavf_packet_read_header(p->g);

  if(!ph)
    return NULL;

  for(i = 0; i < p->ph->num_streams; i++)
    {
    if(ph->stream_id == p->ph->streams[i].id)
      return &p->ph->streams[i];
    }
  return NULL;
  }

/* Utility functions */

const gavf_stream_header_t *
bg_plug_header_from_index(bg_plug_t * p, int index)
  {
  return &p->ph->streams[index];
  }

const gavf_stream_header_t *
bg_plug_header_from_id(bg_plug_t * p, uint32_t id)
  {
  int i;
  for(i = 0; i < p->ph->num_streams; i++)
    {
    if(p->ph->streams[i].id == id)
      return &p->ph->streams[i];
    }
  return NULL;
  }

/* Get stream sources */

static stream_t * find_stream_by_id(stream_t * streams, int num, int id)
  {
  int i;
  for(i = 0; i < num; i++)
    {
    if(streams[i].h->id == id)
      return &streams[i];
    }
  return NULL;
  }

static stream_t * find_stream_by_id_all(bg_plug_t * p, int id)
  {
  stream_t * ret;
  ret = find_stream_by_id(p->audio_streams, p->num_audio_streams, id);
  if(ret)
    return ret;
  ret = find_stream_by_id(p->video_streams, p->num_video_streams, id);
  if(ret)
    return ret;
  ret = find_stream_by_id(p->text_streams, p->num_text_streams, id);
  if(ret)
    return ret;
  ret = find_stream_by_id(p->overlay_streams, p->num_overlay_streams, id);
  if(ret)
    return ret;
  return NULL;
  }
  
int bg_plug_get_stream_source(bg_plug_t * p,
                              const gavf_stream_header_t * h,
                              gavl_audio_source_t ** as,
                              gavl_video_source_t ** vs,
                              gavl_packet_source_t ** ps)
  {
  stream_t * s = NULL;
  if(!(s = find_stream_by_id_all(p, h->id)))
    return 0;
  
  if(s->asrc && as)
    *as = s->asrc;
  else if(s->vsrc && vs)
    *vs = s->vsrc;
  else if(ps)
    *ps = s->src_ext;
  else
    return 0;
  return 1;
  }

int bg_plug_get_stream_sink(bg_plug_t * p,
                            const gavf_stream_header_t * h,
                            gavl_audio_sink_t ** as,
                            gavl_video_sink_t ** vs,
                            gavl_packet_sink_t ** ps)
  {
  stream_t * s = NULL;
  if(!(s = find_stream_by_id_all(p, h->id)))
    return 0;
     
  if(s->asink && as)
    *as = s->asink;
  else if(s->vsink && vs)
    *vs = s->vsink;
  else if(ps)
    *ps = s->sink_ext;
  else
    return 0;
  return 1;
  }

static stream_t *
append_stream(bg_plug_t * plug,
              stream_t ** streams, int * num, const gavl_metadata_t * m)
  {
  stream_t * ret;
  *streams = realloc(*streams, (*num + 1) * sizeof(**streams));
  ret = (*streams) + *num;
  memset(ret, 0, sizeof(*ret));
  gavl_metadata_copy(&ret->m, m);
  ret->plug = plug;
  (*num)++;
  return ret;
  }

/* Compression parameters */

typedef struct
  {
  stream_t * s;
  bg_plugin_registry_t * plugin_reg;
  } set_codec_parameter_t;

static void set_codec_parameter(void * data, const char * name,
                                const bg_parameter_value_t * v)
  {
  set_codec_parameter_t * p = data;
  bg_plugin_registry_set_compressor_parameter(p->plugin_reg,
                                              &p->s->codec_handle,
                                              name,
                                              v);
  }

int bg_plug_add_audio_stream(bg_plug_t * p,
                             const gavl_compression_info_t * ci,
                             const gavl_audio_format_t * format,
                             const gavl_metadata_t * m,
                             bg_cfg_section_t * encode_section)
  {
  stream_t * s;
  s = append_stream(p, &p->audio_streams, &p->num_audio_streams, m);
  gavl_compression_info_copy(&s->ci, ci);
  gavl_audio_format_copy(&s->afmt, format);

  if(encode_section && (s->ci.id == GAVL_CODEC_ID_NONE) &&
     p->ac_params)
    {
    set_codec_parameter_t sc;
    sc.s = s;
    sc.plugin_reg = p->plugin_reg;
    bg_cfg_section_apply(encode_section,
                         p->vc_params,
                         set_codec_parameter,
                         &sc);
    }
  return p->num_audio_streams-1;
  }

int bg_plug_add_video_stream(bg_plug_t * p,
                             const gavl_compression_info_t * ci,
                             const gavl_video_format_t * format,
                             const gavl_metadata_t * m,
                             bg_cfg_section_t * encode_section)
  {
  stream_t * s;
  s = append_stream(p, &p->video_streams, &p->num_video_streams, m);
  gavl_compression_info_copy(&s->ci, ci);
  gavl_video_format_copy(&s->vfmt, format);

  if(encode_section && (s->ci.id == GAVL_CODEC_ID_NONE) &&
     p->vc_params)
    {
    set_codec_parameter_t sc;
    sc.s = s;
    sc.plugin_reg = p->plugin_reg;
    bg_cfg_section_apply(encode_section,
                         p->vc_params,
                         set_codec_parameter,
                         &sc);
    }
  
  return p->num_video_streams-1;
  }

int bg_plug_add_overlay_stream(bg_plug_t * p,
                             const gavl_compression_info_t * ci,
                             const gavl_video_format_t * format,
                             const gavl_metadata_t * m,
                             bg_cfg_section_t * encode_section)
  {
  stream_t * s;
  s = append_stream(p, &p->overlay_streams, &p->num_overlay_streams, m);
  gavl_compression_info_copy(&s->ci, ci);
  gavl_video_format_copy(&s->vfmt, format);

  if(encode_section && (s->ci.id == GAVL_CODEC_ID_NONE) &&
     p->vc_params)
    {
    set_codec_parameter_t sc;
    sc.s = s;
    sc.plugin_reg = p->plugin_reg;
    bg_cfg_section_apply(encode_section,
                         p->oc_params,
                         set_codec_parameter,
                         &sc);
    }
  return p->num_overlay_streams-1;
  }

int bg_plug_add_text_stream(bg_plug_t * p,
                            uint32_t timescale,
                            const gavl_metadata_t * m)
  {
  stream_t * s;
  s = append_stream(p, &p->text_streams, &p->num_text_streams, m);
  s->timescale = timescale;
  return p->num_text_streams-1;
  
  }

/* Setup writer */

int bg_plug_setup_writer(bg_plug_t * p, bg_mediaconnector_t * conn)
  {
  int i;
  gavl_compression_info_t ci_none;
  const gavl_audio_format_t * afmt;
  const gavl_video_format_t * vfmt;
  const gavl_compression_info_t * ci;
  bg_mediaconnector_stream_t * s;
  const gavf_stream_header_t * h;

  gavl_audio_sink_t * as;
  gavl_video_sink_t * vs;
  gavl_packet_sink_t * ps;
  
  memset(&ci_none, 0, sizeof(ci_none));
  
  for(i = 0; i < conn->num_streams; i++)
    {
    s = conn->streams[i];
    switch(s->type)
      {
      case GAVF_STREAM_AUDIO:
        if(s->psrc)
          {
          ci = gavl_packet_source_get_ci(s->psrc);
          afmt = gavl_packet_source_get_audio_format(s->psrc);
          }
        else if(s->asrc)
          {
          ci = &ci_none;
          afmt = gavl_audio_source_get_src_format(s->asrc);
          }
        else
          return 0;
        
        if(bg_plug_add_audio_stream(p, ci, afmt, &s->m, s->encode_section) < 0)
          return 0;
        break;
      case GAVF_STREAM_VIDEO:
        if(s->psrc)
          {
          ci = gavl_packet_source_get_ci(s->psrc);
          vfmt = gavl_packet_source_get_video_format(s->psrc);
          }
        else if(s->vsrc)
          {
          ci = &ci_none;
          vfmt = gavl_video_source_get_src_format(s->vsrc);
          }
        else
          return 0;
        
        if(bg_plug_add_video_stream(p, ci, vfmt, &s->m, s->encode_section) < 0)
          return 0;
        break;
      case GAVF_STREAM_OVERLAY:
        if(s->psrc)
          {
          ci = gavl_packet_source_get_ci(s->psrc);
          vfmt = gavl_packet_source_get_video_format(s->psrc);
          }
        else if(s->vsrc)
          {
          ci = &ci_none;
          vfmt = gavl_video_source_get_src_format(s->vsrc);
          }
        else
          return 0;
        
        if(bg_plug_add_overlay_stream(p, ci, vfmt, &s->m, s->encode_section) < 0)
          return 0;
        break;
      case GAVF_STREAM_TEXT:
        if(bg_plug_add_text_stream(p, s->timescale, &s->m) < 0)
          return 0;
        break;
      }
    }

  if(!bg_plug_start(p))
    return 0;


  /* Get sinks and connect them */

  for(i = 0; i < conn->num_streams; i++)
    {
    s = conn->streams[i];
    h = p->ph->streams + i;

    as = NULL;
    vs = NULL;
    ps = NULL;
    
    if(!bg_plug_get_stream_sink(p, h, &as, &vs, &ps))
      return 0;
    
    if(as && s->aconn)
      gavl_audio_connector_connect(s->aconn, as);
    else if(vs && s->vconn)
      gavl_video_connector_connect(s->vconn, vs);
    else if(ps && s->pconn)
      gavl_packet_connector_connect(s->pconn, ps);
    else
      return 0;
    }
  return 1;
  }

int bg_plug_setup_reader(bg_plug_t * p, bg_mediaconnector_t * conn)
  {
  int i;
  stream_t * s;
  bg_mediaconnector_stream_t * cs;
  if(!bg_plug_start(p))
    return 0;
  
  for(i = 0; i < p->num_audio_streams; i++)
    {
    s = p->audio_streams + i;

    if(s->asrc || s->src_ext)
      {
      cs = bg_mediaconnector_add_audio_stream(conn,
                                              &s->h->m,
                                              s->asrc,
                                              s->src_ext,
                                              NULL);
      cs->src_index = i;
      }
    else
      continue;
    }

  for(i = 0; i < p->num_video_streams; i++)
    {
    s = p->video_streams + i;

    if(s->vsrc || s->src_ext)
      {
      cs = bg_mediaconnector_add_video_stream(conn,
                                              &s->h->m,
                                              s->vsrc,
                                              s->src_ext,
                                              NULL);
      cs->src_index = i;
      }
    else
      continue;
    }

  for(i = 0; i < p->num_text_streams; i++)
    {
    s = p->text_streams + i;

    if(s->src_ext)
      {
      cs = bg_mediaconnector_add_text_stream(conn,
                                             &s->h->m,
                                             s->src_ext,
                                             s->h->format.text.timescale);
      cs->src_index = i;
      }
    else
      continue;
    }

  for(i = 0; i < p->num_overlay_streams; i++)
    {
    s = p->overlay_streams + i;

    if(s->vsrc || s->src_ext)
      {
      cs = bg_mediaconnector_add_overlay_stream(conn,
                                                &s->h->m,
                                                s->vsrc,
                                                s->src_ext,
                                                NULL);
      cs->src_index = i;
      }
    else
      continue;
    }

  return 1;
  }


void bg_plug_set_stream_action(bg_plug_t * p,
                               const gavf_stream_header_t * h,
                               bg_stream_action_t action)
  {
  stream_t * s = find_stream_by_id_all(p, h->id);
  if(!s)
    return;
  s->action = action;
  }

int bg_plug_got_error(bg_plug_t * p)
  {
  int ret;
  pthread_mutex_lock(&p->mutex);
  ret = p->got_error || gavf_io_got_error(p->io);
  pthread_mutex_unlock(&p->mutex);
  return ret;
  }
