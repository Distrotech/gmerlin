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

#include <avdec_private.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <dvframe.h>

#define AUDIO_ID 0
#define VIDEO_ID 1

/* DV demuxer */

typedef struct
  {
  bgav_dv_dec_t * d;
  int64_t frame_pos;
  int frame_size;
  uint8_t * frame_buffer;
  } dv_priv_t;

static int probe_dv(bgav_input_context_t * input)
  {
  char * pos;
  /* There seems to be no way to do proper probing of the stream.
     Therefore, we accept only local files with .dv as extension */

  if(input->filename)
    {
    pos = strrchr(input->filename, '.');
    if(!pos)
      return 0;
    if(!strcasecmp(pos, ".dv"))
      return 1;
    }
  return 0;
  }

static int open_dv(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  int64_t total_frames;
  uint8_t header[DV_HEADER_SIZE];
  bgav_stream_t * as, * vs;
  dv_priv_t * priv;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  priv->d = bgav_dv_dec_create();
  
  /* Read frame */
  if(bgav_input_get_data(ctx->input, header, DV_HEADER_SIZE) < DV_HEADER_SIZE)
    return 0;
  bgav_dv_dec_set_header(priv->d, header);
  priv->frame_size = bgav_dv_dec_get_frame_size(priv->d);
  priv->frame_buffer = malloc(priv->frame_size);

  if(bgav_input_get_data(ctx->input, priv->frame_buffer, 
                          priv->frame_size) < priv->frame_size)
    return 0;
  
  bgav_dv_dec_set_frame(priv->d, priv->frame_buffer);
  
  /* Create track */
  
  ctx->tt = bgav_track_table_create(1);
  
  /* Set up streams */
  as = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
  bgav_dv_dec_init_audio(priv->d, as);
  as->stream_id = AUDIO_ID;
  
  vs = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
  bgav_dv_dec_init_video(priv->d, vs);
  vs->stream_id = VIDEO_ID;
  
  /* Set duration */

  if(ctx->input->total_bytes)
    {
    total_frames = ctx->input->total_bytes / priv->frame_size;
    ctx->tt->cur->duration =
      gavl_frames_to_time(vs->data.video.format.timescale, vs->data.video.format.frame_duration,
                          total_frames);
    }
  
  if(ctx->input->input->seek_byte)
    ctx->flags |= BGAV_DEMUXER_CAN_SEEK;
  
  ctx->stream_description = bgav_sprintf("DV Format");

  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;
  ctx->index_mode = INDEX_MODE_SIMPLE;
  
  return 1;
  
  }

static int next_packet_dv(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t *ap = (bgav_packet_t *)0, *vp = (bgav_packet_t *)0;
  bgav_stream_t *as, *vs;
  dv_priv_t * priv;
  priv = (dv_priv_t *)(ctx->priv);
  
  /*
   *  demuxing dv is easy: we copy the video frame and
   *  extract the audio data
   */

  as = bgav_track_find_stream(ctx, AUDIO_ID);
  vs = bgav_track_find_stream(ctx, VIDEO_ID);
  
  if(vs)
    {
    vp = bgav_stream_get_packet_write(vs);
    vp->position = ctx->input->position;
    }
  if(as)
    {
    ap = bgav_stream_get_packet_write(as);
    ap->position = ctx->input->position;
    }
  if(bgav_input_read_data(ctx->input, priv->frame_buffer, priv->frame_size) < priv->frame_size)
    return 0;
  
  bgav_dv_dec_set_frame(priv->d, priv->frame_buffer);
  
  if(!bgav_dv_dec_get_audio_packet(priv->d, ap))
    return 0;
  
  bgav_dv_dec_get_video_packet(priv->d, vp);
  if(ap) bgav_packet_done_write(ap);
  if(vp) bgav_packet_done_write(vp);
  return 1;
  }

static void seek_dv(bgav_demuxer_context_t * ctx, int64_t time,
                    int scale)
  {
  int64_t file_position;
  dv_priv_t * priv;
  bgav_stream_t * as, * vs;
  int64_t frame_pos;
  
  priv = (dv_priv_t *)(ctx->priv);
  vs = ctx->tt->cur->video_streams;
  as = ctx->tt->cur->audio_streams;
  
  frame_pos = gavl_time_rescale(scale, vs->data.video.format.timescale,
                                time);
  frame_pos /= vs->data.video.format.frame_duration;
  
  file_position = frame_pos * priv->frame_size;

  bgav_dv_dec_set_frame_counter(priv->d, frame_pos,
                                gavl_time_rescale(vs->data.video.format.timescale,
                                                  vs->data.audio.format.samplerate,
                                                  vs->in_position *
                                                  vs->data.video.format.frame_duration));
  bgav_input_seek(ctx->input, file_position, SEEK_SET);

  vs->in_time = frame_pos * vs->data.video.format.frame_duration;
  as->in_time =
    gavl_time_rescale(vs->data.video.format.timescale,
                      as->data.audio.format.samplerate,
                      vs->in_time);
  }

static int select_track_dv(bgav_demuxer_context_t * ctx, int track)
  {
  dv_priv_t * priv;
  priv = (dv_priv_t *)(ctx->priv);
  bgav_dv_dec_set_frame_counter(priv->d, 0, 0);
  return 1;
  }

static void close_dv(bgav_demuxer_context_t * ctx)
  {
  dv_priv_t * priv;
  priv = (dv_priv_t *)(ctx->priv);
  if(priv->frame_buffer)
    free(priv->frame_buffer);
  if(priv->d)
    bgav_dv_dec_destroy(priv->d);
  
  free(priv);
  }

static int resync_dv(bgav_demuxer_context_t * ctx)
  {
  dv_priv_t * priv;
  priv = (dv_priv_t *)(ctx->priv);
  bgav_dv_dec_set_frame_counter(priv->d,
                                ctx->tt->cur->video_streams->in_time /
                                ctx->tt->cur->video_streams->data.video.format.frame_duration,
                                ctx->tt->cur->audio_streams->in_time);
  return 1;
  }


const bgav_demuxer_t bgav_demuxer_dv =
  {
    .probe        = probe_dv,
    .open         = open_dv,
    .select_track = select_track_dv,
    .next_packet  =  next_packet_dv,
    .resync       = resync_dv,
    .seek         = seek_dv,
    .close        = close_dv
  };

