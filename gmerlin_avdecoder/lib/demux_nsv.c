/*****************************************************************
 
  demux_nsv.c
 
  Copyright (c) 2005-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <avdec_private.h>

#define AUDIO_ID 0
#define VIDEO_ID 1

// #define DUMP_HEADERS

/*
 *  Straight forward implementation of the NSV spec from
 *  http://ultravox.aol.com/
 */

#define NSV_FILE_HEADER BGAV_MK_FOURCC('N','S','V','f')
#define NSV_SYNC_HEADER BGAV_MK_FOURCC('N','S','V','s')

static int next_packet_nsv(bgav_demuxer_context_t * ctx);


typedef struct
  {
  uint32_t header_size;
  uint32_t file_size;
  uint32_t file_len; /* Milliseconds */
  uint32_t metadata_len;
  uint32_t toc_alloc;
  uint32_t toc_size;

  struct
    {
    char * title;
    char * url;
    char * creator;
    char * aspect;
    char * framerate;
    } metadata;

  struct
    {
    uint32_t * offsets;
    uint32_t * frames; /* Only present for Version 2 TOCs */
    } toc;
  
  } nsv_file_header_t;

static uint8_t * parse_metadata(char * buf,
                                char ** name,
                                char ** value)
  {
  char delim;
  char * start, *end;
  while(isspace(*buf) && (*buf != '\0'))
    buf++;
  if(*buf == '\0')
    return (char*)0;
  start = buf;
  end = strchr(start, '=');
  if(!end)
    return (char*)0;
  *name = bgav_strndup(start, end);
  start = end;
  start++; /* Start now points to the delimiter */
  delim = *start;
  start++;
  end = strchr(start, delim);
  if(!end)
    return (char*)0;
  *value = bgav_strndup(start, end);
  end++;
  start = end;
  start++;
  return start;
  }

static int nsv_file_header_read(bgav_input_context_t * ctx,
                                nsv_file_header_t * ret)
  {
  int i;
  uint8_t * buf, *pos;
  char * meta_name, *meta_value;
  
  bgav_input_skip(ctx, 4); /* NSVf */

  if(!bgav_input_read_32_le(ctx, &ret->header_size) ||
     !bgav_input_read_32_le(ctx, &ret->file_size) ||
     !bgav_input_read_32_le(ctx, &ret->file_len) || /* Milliseconds */
     !bgav_input_read_32_le(ctx, &ret->metadata_len) ||
     !bgav_input_read_32_le(ctx, &ret->toc_alloc) ||
     !bgav_input_read_32_le(ctx, &ret->toc_size))
    return 0;

  /* Allocate buffer */

  buf = malloc(ret->metadata_len+1 > ret->toc_alloc*4 ?
               ret->metadata_len+1 : ret->toc_alloc*4);
  
  /* Read metadata */
  if(bgav_input_read_data(ctx, buf, ret->metadata_len) < ret->metadata_len)
    return 0;
  buf[ret->metadata_len] = '\0';
  
  pos = buf;
  while(pos && (pos - buf < ret->metadata_len))
    {
    meta_name  = (char*)0;
    meta_value = (char*)0;
    pos = parse_metadata(pos, &meta_name, &meta_value);

    if(!pos)
      {
      if(meta_name) free(meta_name);
      if(meta_value) free(meta_value);
      break;
      }
    
    if(!strcasecmp(meta_name, "Title"))
      ret->metadata.title = meta_value;
    else if(!strcasecmp(meta_name, "URL"))
      ret->metadata.url = meta_value;
    else if(!strcasecmp(meta_name, "Creator"))
      ret->metadata.creator = meta_value;
    else if(!strcasecmp(meta_name, "Aspect"))
      ret->metadata.aspect = meta_value;
    else if(!strcasecmp(meta_name, "Framerate"))
      ret->metadata.framerate = meta_value;
    else
      free(meta_value);
    free(meta_name);
    }

  /* Read TOC */

  if(ret->toc_alloc)
    {
    if(bgav_input_read_data(ctx, buf, ret->toc_alloc*4) < ret->toc_alloc*4)
      return 0;

    /* Read TOC version 1 */
    ret->toc.offsets = malloc(ret->toc_size * sizeof(*ret->toc.offsets));
    pos = buf;

    for(i = 0; i < ret->toc_size; i++)
      {
      ret->toc.offsets[i] = BGAV_PTR_2_32LE(pos); pos+=4;
      //      fprintf(stderr, "Got offset %d\n", ret->toc.offsets[i]);
      }
    /* Read TOC version 2 */
    if((ret->toc_alloc > ret->toc_size * 2) &&
       (pos[0] == 'T') &&
       (pos[1] == 'O') &&
       (pos[2] == 'C') &&
       (pos[3] == '2'))
      {
      pos+=4;
      ret->toc.frames = malloc(ret->toc_size * sizeof(*ret->toc.frames));
      for(i = 0; i < ret->toc_size; i++)
        ret->toc.frames[i] = BGAV_PTR_2_32LE(pos); pos+=4;
      }
    }
  
  return 1;
  }

static void nsv_file_header_free(nsv_file_header_t * h)
  {
  if(h->metadata.title)     free(h->metadata.title);
  if(h->metadata.url)       free(h->metadata.url);
  if(h->metadata.creator)   free(h->metadata.creator);
  if(h->metadata.aspect)    free(h->metadata.aspect);
  if(h->metadata.framerate) free(h->metadata.framerate);

  if(h->toc.offsets)        free(h->toc.offsets);
  if(h->toc.frames)         free(h->toc.frames);
  }

#ifdef DUMP_HEADERS
static void nsv_file_header_dump(nsv_file_header_t * h)
  {
  int i;
  fprintf(stderr, "file_header\n");

  fprintf(stderr, "  header_size:  %d\n", h->header_size);
  fprintf(stderr, "  file_size:    %d\n", h->file_size);
  fprintf(stderr, "  file_len:     %d\n", h->file_len); /* Milliseconds */
  fprintf(stderr, "  metadata_len: %d\n", h->metadata_len);
  fprintf(stderr, "  toc_alloc:    %d\n", h->toc_alloc);
  fprintf(stderr, "  toc_size:     %d\n", h->toc_size);
  fprintf(stderr, "  title:        %s\n",
          (h->metadata.title ? h->metadata.title : "[not set]"));
  fprintf(stderr, "  url:          %s\n",
          (h->metadata.url ? h->metadata.url : "[not set]"));
  fprintf(stderr, "  creator:      %s\n",
          (h->metadata.creator ? h->metadata.creator : "[not set]"));
  fprintf(stderr, "  aspect:       %s\n",
          (h->metadata.aspect ? h->metadata.aspect : "[not set]"));
  fprintf(stderr, "  framerate:    %s\n",
          (h->metadata.framerate ? h->metadata.framerate : "[not set]"));

  if(h->toc_size)
    {
    if(h->toc.frames)
      {
      fprintf(stderr, "  TOC version 2 (%d entries)\n", h->toc_size);
      for(i = 0; i < h->toc_size; i++)
        fprintf(stderr, "    frame: %d, offset: %d\n",
                h->toc.frames[i], h->toc.offsets[i]);
      }
    else
      {
      fprintf(stderr, "  TOC version 1 (%d entries)\n", h->toc_size);
      for(i = 0; i < h->toc_size; i++)
        fprintf(stderr, "    offset: %d\n", h->toc.offsets[i]);
      }
    }
  else
    fprintf(stderr, "  No TOC\n");
  }
#endif

typedef struct
  {
  uint32_t vidfmt;
  uint32_t audfmt;
  uint16_t width;
  uint16_t height;
  uint8_t  framerate;
  int16_t  syncoffs;
  } nsv_sync_header_t;

static int nsv_sync_header_read(bgav_input_context_t * ctx,
                                nsv_sync_header_t * ret)
  {
  bgav_input_skip(ctx, 4); /* NSVs */

  if(!bgav_input_read_fourcc(ctx, &ret->vidfmt) ||
     !bgav_input_read_fourcc(ctx, &ret->audfmt) ||
     !bgav_input_read_16_le(ctx, &ret->width) ||
     !bgav_input_read_16_le(ctx, &ret->height) ||
     !bgav_input_read_8(ctx, &ret->framerate) ||
     !bgav_input_read_16_le(ctx, (uint16_t*)(&ret->syncoffs)))
    return 0;
  return 1;
  }

#ifdef DUMP_HEADERS
static void nsv_sync_header_dump(nsv_sync_header_t * h)
  {
  fprintf(stderr, "sync_header\n");
  fprintf(stderr, "  vidfmt: ");
  bgav_dump_fourcc(h->vidfmt);
  fprintf(stderr, "\n");

  fprintf(stderr, "  audfmt: ");
  bgav_dump_fourcc(h->audfmt);
  fprintf(stderr, "\n");

  fprintf(stderr, "  width:         %d\n", h->width);
  fprintf(stderr, "  height:        %d\n", h->height);
  fprintf(stderr, "  framerate_idx: %d\n", h->framerate);
  fprintf(stderr, "  syncoffs:      %d\n", h->syncoffs);
  }
#endif

typedef struct
  {
  nsv_file_header_t fh;
  int has_fh;
  
  int payload_follows; /* Header already read, payload follows */

  int64_t frame_counter;

  int is_pcm;
  int need_pcm_format;
  } nsv_priv_t;

static int probe_nsv(bgav_input_context_t * input)
  {
  uint32_t fourcc;
  char * pos;
  
  /* Check for video/nsv */

  if(input->mimetype && !strcmp(input->mimetype, "video/nsv"))
    return 1;


  /* Probing a stream without any usable headers at the
     beginning isn't save enough so we check for the extension */
    
  if(input->filename)
    {
    pos = strrchr(input->filename, '.');
    if(pos && !strcasecmp(pos, ".nsv"))
      return 1;
    }
  
  if(!bgav_input_get_fourcc(input, &fourcc))
    return 0;

  if((fourcc != NSV_FILE_HEADER) &&
     (fourcc != NSV_SYNC_HEADER))
    return 0;

  return 1;
  }

static void simplify_rational(int * num, int * den)
  {
  int i = 2;

  while(i <= *den)
    {
    if(!(*num % i) && !(*den % i))
      {
      *num /= i;
      *den /= i;
      }
    else
      i++;
    }
  }

static void calc_framerate(int code, int * num, int * den)
  {
  int t, s_num, s_den;
  if(!(code & 0x80))
    {
    *num = code;
    *den = 1;
    return;
    }
  t = (code & 0x7f) >> 2;
  if(t < 16)
    {
    s_num = 1;
    s_den = t+1;
    }
  else
    {
    s_num = t - 15;
    s_den = 1;
    }

  if(code & 1)
    {
    s_num *= 1000;
    s_den *= 1001;
    }

  if((code & 3) == 3)
    {
    *num = s_num * 24;
    *den = s_den;
    }
  else if((code & 3) == 2)
    {
    *num = s_num * 25;
    *den = s_den;
    }
  else
    {
    *num = s_num * 30;
    *den = s_den;
    }
  simplify_rational(num, den);
  }
#if 0
static void test_framerate()
  {
  int i, num, den;
  for(i = 0; i < 256; i++)
    {
    calc_framerate(i, &num, &den);
    fprintf(stderr, "%d=%.4f [%d:%d]\n", i, (float)num/(float)den, num, den);
    }
  }
#endif
static int open_nsv(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  bgav_stream_t * s;
  nsv_priv_t * p;

  nsv_sync_header_t sh;
  uint32_t fourcc;
  int done = 0;

  //  test_framerate();
  
  p = calloc(1, sizeof(*p));
  ctx->priv = p;
  
  while(!done)
    {
    /* Find the first chunk we can handle */
    while(1)
      {
      if(!bgav_input_get_fourcc(ctx->input, &fourcc))
        return 0;
      
      if((fourcc == NSV_FILE_HEADER) || (fourcc == NSV_SYNC_HEADER))
        break;
      bgav_input_skip(ctx->input, 1);
      }

    if(fourcc == NSV_FILE_HEADER)
      {
      //      fprintf(stderr, "Got file header\n");

      if(!nsv_file_header_read(ctx->input, &(p->fh)))
        return 0;
      else
        {
        p->has_fh = 1;
#ifdef DUMP_HEADERS
        nsv_file_header_dump(&p->fh);
#endif
        }
      }
    else if(fourcc == NSV_SYNC_HEADER)
      {
      //      fprintf(stderr, "Got sync header\n");

      if(!nsv_sync_header_read(ctx->input, &sh))
        return 0;
#ifdef DUMP_HEADERS
      nsv_sync_header_dump(&sh);
#endif
      done = 1;
      }
    }
  ctx->tt = bgav_track_table_create(1);

  /* Set up streams */

  if(sh.vidfmt != BGAV_MK_FOURCC('N','O','N','E'))
    {
    s = bgav_track_add_video_stream(ctx->tt->current_track, ctx->opt);
    s->fourcc = sh.vidfmt;
    
    s->data.video.format.image_width  = sh.width;
    s->data.video.format.image_height = sh.height;
    
    s->data.video.format.frame_width =
      s->data.video.format.image_width;
    s->data.video.format.frame_height =
      s->data.video.format.image_height;
    
    s->data.video.format.pixel_width  = 1;
    s->data.video.format.pixel_height = 1;
    s->stream_id = VIDEO_ID;

    /* Calculate framerate */

    calc_framerate(sh.framerate,
                   &s->data.video.format.timescale,
                   &s->data.video.format.frame_duration);
    /* Get depth for RGB3 */
    if(sh.vidfmt == BGAV_MK_FOURCC('R','G','B','3'))
      s->data.video.depth = 24;
    
    }
  
  if(sh.audfmt != BGAV_MK_FOURCC('N','O','N','E'))
    {
    s = bgav_track_add_audio_stream(ctx->tt->current_track, ctx->opt);
    s->fourcc = sh.audfmt;
    s->stream_id = AUDIO_ID;
    if(sh.audfmt == BGAV_MK_FOURCC('P','C','M',' '))
      {
      p->is_pcm = 1;
      p->need_pcm_format = 1;
      }
    }

  /* Handle File header */

  if(p->has_fh)
    {
    /* Duration */
    if(p->fh.file_len != 0xFFFFFFFF)
      // ctx->tt->current_track->duration =
      //  (GAVL_TIME_SCALE * (int64_t)(p->fh.file_len)) / 1000;
      ctx->tt->current_track->duration = gavl_time_unscale(1000, p->fh.file_len);
    /* Metadata */

    if(p->fh.metadata.title)
      ctx->tt->current_track->metadata.title =
        bgav_strndup(p->fh.metadata.title, (char*)0);

    if(p->fh.metadata.url)
      ctx->tt->current_track->metadata.comment =
        bgav_strndup(p->fh.metadata.url, (char*)0);
    if(p->fh.metadata.creator)
      ctx->tt->current_track->metadata.author =
        bgav_strndup(p->fh.metadata.creator, (char*)0);

    /* Decide whether we can seek */

    if(ctx->input->input->seek_byte && p->fh.toc.offsets)
      ctx->can_seek = 1;
    }
  
  p->payload_follows = 1;

  if(!ctx->tt->tracks[0].name && ctx->input->metadata.title)
    {
    ctx->tt->tracks[0].name = bgav_strndup(ctx->input->metadata.title,
                                           NULL);
    }

  ctx->stream_description = bgav_sprintf("Nullsoft Video (NSV)");

  while(p->need_pcm_format)
    {
    if(!next_packet_nsv(ctx)) /* This lets us get the format for PCM audio */
      return 0;
    }
  return 1;
  }

static int get_pcm_format(bgav_demuxer_context_t * ctx, bgav_stream_t * s)
  {
  uint8_t  tmp_8;
  uint16_t tmp_16;
  /* Bits */
  if(!bgav_input_read_8(ctx->input, &tmp_8))
    return 0;
  s->data.audio.bits_per_sample = tmp_8;

  /* Channels */
  if(!bgav_input_read_8(ctx->input, &tmp_8))
    return 0;
  s->data.audio.format.num_channels = tmp_8;
  
  /* Samplerate */
  if(!bgav_input_read_16_le(ctx->input, &tmp_16))
    return 0;
  s->data.audio.format.samplerate = tmp_16;
#if 0
  fprintf(stderr, "get_pcm_format %d %d %d\n", s->data.audio.bits_per_sample,
          s->data.audio.format.num_channels, s->data.audio.format.samplerate);
#endif
  s->data.audio.block_align = (s->data.audio.bits_per_sample * s->data.audio.format.num_channels) / 8;

#if 1 /* What's that???? */
  s->data.audio.bits_per_sample = 8;
  s->data.audio.format.num_channels = 1;
  s->data.audio.format.samplerate /= 4;
#endif
  return 1;
  }

static int next_packet_nsv(bgav_demuxer_context_t * ctx)
  {
  //  uint8_t test_data[32];
  int i;
  bgav_packet_t * p;
  bgav_stream_t * s;
  nsv_priv_t * priv;
  uint8_t num_aux;
  uint16_t tmp_16;
  uint32_t aux_plus_video_len;
  uint32_t video_len;
  uint16_t audio_len;

  uint16_t aux_chunk_len;
  uint32_t aux_chunk_type;
  int skipped_bytes;
  
  nsv_sync_header_t sh;
  uint32_t fourcc;
  int have_sync_header = 0;
  
  priv = (nsv_priv_t *)(ctx->priv);

  if(!priv->payload_follows)
    {
    /* Read header */

    if(!bgav_input_get_16_le(ctx->input, &tmp_16))
      return 0;

    if(tmp_16 != 0xbeef)
      {
      skipped_bytes = 0;
      while(1)
        {
        if(!bgav_input_get_fourcc(ctx->input, &fourcc))
          return 0;
        
        if(fourcc == NSV_SYNC_HEADER)
          break;
        bgav_input_skip(ctx->input, 1);
        skipped_bytes++;
        }
      if(!nsv_sync_header_read(ctx->input, &sh))
        return 0;
      //      nsv_sync_header_dump(&sh);
      have_sync_header = 1;
      }
    else
      bgav_input_skip(ctx->input, 2); /* Skip beef */
    }
  else
    have_sync_header = 1;
  

  //  bgav_input_get_data(ctx->input, test_data, 32);
  //  bgav_hexdump(test_data, 32, 16);
  /* Parse payload */
  
  if(!bgav_input_read_8(ctx->input, &num_aux))
    return 0;

  if(!bgav_input_read_16_le(ctx->input, &tmp_16))
    return 0;

  if(!bgav_input_read_16_le(ctx->input, &audio_len))
    return 0;

  aux_plus_video_len = tmp_16;
  
  aux_plus_video_len = (aux_plus_video_len << 4) | (num_aux >> 4);
  num_aux &= 0x0f;
#if 0 
  fprintf(stderr, "num_aux: %d, aux_plus_video_len: %d, audio_len: %d\n",
          num_aux, aux_plus_video_len, audio_len);
#endif
  video_len = aux_plus_video_len;

  /* Skip aux packets */
  for(i = 0; i < num_aux; i++)
    {
    if(!bgav_input_read_16_le(ctx->input, &aux_chunk_len) ||
       !bgav_input_read_fourcc(ctx->input, &aux_chunk_type))
      return 0;

    fprintf(stderr, "Skipping %d bytes of aux data type ", aux_chunk_len);
    bgav_dump_fourcc(aux_chunk_type);
    fprintf(stderr, "\n");
    bgav_input_skip(ctx->input, aux_chunk_len);

    video_len -= (aux_chunk_len+6);
    }

  /* Video data */

  if(video_len)
    {
    if(priv->need_pcm_format)
      s = ctx->tt->current_track->video_streams;
    else
      s = bgav_track_find_stream(ctx->tt->current_track, VIDEO_ID);
    if(s)
      {
      p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
      bgav_packet_alloc(p, video_len);
      if(bgav_input_read_data(ctx->input, p->data, video_len) < video_len)
        return 0;
      p->data_size = video_len;
      p->timestamp_scaled =
        priv->frame_counter * s->data.video.format.frame_duration;

      p->keyframe = 0;
      
      if(s->fourcc == BGAV_MK_FOURCC('V','P','6','1'))
        {
        if(p->data[1] == 0x36)
          p->keyframe = 1;
        }
      else if(s->fourcc == BGAV_MK_FOURCC('V','P','6','2'))
        {
        if(p->data[1] == 0x46)
          p->keyframe = 1;
        }
      else
        {
        if(have_sync_header)
          p->keyframe = 1;
        }
      
      bgav_packet_done_write(p);
      priv->frame_counter++;
      }
    else
      bgav_input_skip(ctx->input, video_len);
    }

  if(audio_len)
    {
    if(priv->need_pcm_format)
      s = ctx->tt->current_track->audio_streams;
    else
      s = bgav_track_find_stream(ctx->tt->current_track, AUDIO_ID);
    if(s)
      {
      /* Special treatment for PCM */
      if(priv->is_pcm)
        {
        if(priv->need_pcm_format)
          {
          if(!get_pcm_format(ctx, s))
            return 0;
          priv->need_pcm_format = 0;
          }
        else
          bgav_input_skip(ctx->input, 4);
        audio_len -= 4;
        }
      if(audio_len)
        {
        p = bgav_packet_buffer_get_packet_write(s->packet_buffer, s);
        bgav_packet_alloc(p, audio_len);
        if(bgav_input_read_data(ctx->input, p->data, audio_len) < audio_len)
          return 0;
        p->data_size = audio_len;
        bgav_packet_done_write(p);
        }
      }
    else
      bgav_input_skip(ctx->input, audio_len);
    }
  priv->payload_follows = 0;
  return 1;
  }

static void seek_nsv(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  uint32_t index_position;
  int64_t file_position;
  nsv_priv_t * priv;
  gavl_time_t sync_time = GAVL_TIME_UNDEFINED;

  bgav_stream_t * vs, *as;
  nsv_sync_header_t sh;
  uint32_t fourcc;

  priv = (nsv_priv_t *)(ctx->priv);

  if(!priv->fh.toc.frames) /* TOC version 1 */
    {
    index_position =
      (uint32_t)((double)time / (double)(ctx->tt->current_track->duration) *
                 priv->fh.toc_size + 0.5);
    if(index_position >= priv->fh.toc_size)
      index_position = priv->fh.toc_size - 1;
    else if(index_position < 0)
      index_position = 0;
    sync_time = time;
    }
  else  /* TOC version 2 */
    {
    fprintf(stderr, "Seeking with verison 2 TOC not support due to lack of sample files. Contact the authors to solve this\n");
    }
  file_position = priv->fh.toc.offsets[index_position] + priv->fh.header_size;
  bgav_input_seek(ctx->input, file_position, SEEK_SET);

  /* Now, resync and update stream times */

  while(1)
    {
    if(!bgav_input_get_fourcc(ctx->input, &fourcc))
      return;
    
    if(fourcc == NSV_SYNC_HEADER)
      break;
    bgav_input_skip(ctx->input, 1);
    }
  
  if(!nsv_sync_header_read(ctx->input, &sh))
    return;

  /* We consider the video time to be exact and calculate the audio
     time from the sync offset */

  vs = bgav_track_find_stream(ctx->tt->current_track, VIDEO_ID);
  as = bgav_track_find_stream(ctx->tt->current_track, AUDIO_ID);

  if(vs)
    {
    priv->frame_counter =
      gavl_time_to_frames(vs->data.video.format.timescale,
                          vs->data.video.format.frame_duration,
                          sync_time);
    vs->time_scaled =
      priv->frame_counter * vs->data.video.format.frame_duration;
    }
  if(as)
    {
    as->time_scaled =
      gavl_time_to_samples(as->data.audio.format.samplerate, sync_time) +
      gavl_time_rescale(1000, as->data.audio.format.samplerate, sh.syncoffs);
    }
  }

static void close_nsv(bgav_demuxer_context_t * ctx)
  {
  nsv_priv_t * priv;
  priv = (nsv_priv_t *)(ctx->priv);
  nsv_file_header_free(&priv->fh);
  }

bgav_demuxer_t bgav_demuxer_nsv =
  {
    probe:       probe_nsv,
    open:        open_nsv,
    next_packet: next_packet_nsv,
    seek:        seek_nsv,
    close:       close_nsv
  };

