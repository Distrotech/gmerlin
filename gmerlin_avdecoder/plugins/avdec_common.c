/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#include <avdec.h>
#include "avdec_common.h"

static void log_callback(void*data, bgav_log_level_t level,
                         const char * log_domain,
                         const char * message)
  {
  char * domain;
  bg_log_level_t l = 0;
  
  switch(level)
    {
    case BGAV_LOG_DEBUG:
      l = BG_LOG_DEBUG;
      break;
    case BGAV_LOG_WARNING:
      l = BG_LOG_WARNING;
      break;
    case BGAV_LOG_ERROR:
      l = BG_LOG_ERROR;
      break;
    case BGAV_LOG_INFO:
      l = BG_LOG_INFO;
      break;
    }

  domain = bg_sprintf("avdecoder.%s", log_domain);
  bg_log(l, domain, "%s", message);
  free(domain);
  }
     
void * bg_avdec_create()
  {
  avdec_priv * ret = calloc(1, sizeof(*ret));
  ret->opt = bgav_options_create();
  bgav_options_set_log_callback(ret->opt, log_callback, NULL);
  return ret;
  }

void bg_avdec_close(void * priv)
  {
  avdec_priv * avdec = priv;
  int i;
  if(avdec->dec)
    {
    bgav_close(avdec->dec);
    avdec->dec = NULL;
    }
  if(avdec->track_info)
    {
    for(i = 0; i < avdec->num_tracks; i++)
      bg_track_info_free(&(avdec->track_info[i]));
    free(avdec->track_info);
    avdec->track_info = NULL;
    }
  }

int bg_avdec_get_audio_compression_info(void * priv, int stream,
                                        gavl_compression_info_t * info)
  {
  avdec_priv * avdec = priv;
  return bgav_get_audio_compression_info(avdec->dec, stream, info);
  }


int bg_avdec_get_video_compression_info(void * priv, int stream,
                                        gavl_compression_info_t * info)
  {
  avdec_priv * avdec = priv;
  return bgav_get_video_compression_info(avdec->dec, stream, info);
  }

int bg_avdec_read_audio_packet(void * priv, int stream, gavl_packet_t * p)
  {
  avdec_priv * avdec = priv;
  return bgav_read_audio_packet(avdec->dec, stream, p);
  }

int bg_avdec_read_video_packet(void * priv, int stream, gavl_packet_t * p)
  {
  avdec_priv * avdec = priv;
  return bgav_read_video_packet(avdec->dec, stream, p);
  }

gavl_video_source_t *
bg_avdec_get_video_source(void * priv, int stream)
  {
  avdec_priv * avdec = priv;
  return bgav_get_video_source(avdec->dec, stream);
  }

gavl_audio_source_t *
bg_avdec_get_audio_source(void * priv, int stream)
  {
  avdec_priv * avdec = priv;
  return bgav_get_audio_source(avdec->dec, stream);
  }

void bg_avdec_destroy(void * priv)
  {
  avdec_priv * avdec = priv;
  bg_avdec_close(priv);

  if(avdec->dec)
    {
    bgav_close(avdec->dec);
    }
  if(avdec->opt)
    {
    bgav_options_destroy(avdec->opt);
    }
  if(avdec->edl)
    bg_edl_destroy(avdec->edl);
  free(avdec);
  }

bg_track_info_t * bg_avdec_get_track_info(void * priv, int track)
  {
  avdec_priv * avdec = priv;
  if((track < 0) || (track >= avdec->num_tracks))
    return NULL;
  return &(avdec->track_info[track]);
  }

const bg_edl_t * bg_avdec_get_edl(void * priv)
  {
  avdec_priv * avdec = priv;
  return avdec->edl;
  }

int bg_avdec_read_video(void * priv,
                            gavl_video_frame_t * frame,
                            int stream)
  {
  avdec_priv * avdec = priv;
  return bgav_read_video(avdec->dec, frame, stream);
  }

void bg_avdec_skip_video(void * priv, int stream, int64_t * time,
                         int scale, int exact)
  {
  avdec_priv * avdec = priv;
  bgav_skip_video(avdec->dec, stream, time, scale, exact);
  }

int bg_avdec_has_still(void * priv,
                       int stream)
  {
  avdec_priv * avdec = priv;
  return bgav_video_has_still(avdec->dec, stream);
  }


int bg_avdec_read_audio(void * priv,
                            gavl_audio_frame_t * frame,
                            int stream,
                            int num_samples)
  {
  avdec_priv * avdec = priv;
  return bgav_read_audio(avdec->dec, frame, stream, num_samples);
  }

int bg_avdec_has_subtitle(void * priv, int stream)
  {
  avdec_priv * avdec = priv;
  return bgav_has_subtitle(avdec->dec, stream);
  }

int bg_avdec_read_subtitle_overlay(void * priv,
                                   gavl_overlay_t * ovl, int stream)
  {
  avdec_priv * avdec = priv;
  return bgav_read_subtitle_overlay(avdec->dec, ovl, stream);
  }
  
int bg_avdec_read_subtitle_text(void * priv,
                                char ** text, int * text_alloc,
                                int64_t * start_time,
                                int64_t * duration,
                                int stream)
  {
  avdec_priv * avdec = priv;
  return bgav_read_subtitle_text(avdec->dec, text, text_alloc,
                                 start_time, duration, stream);
  }

static bgav_stream_action_t get_stream_action(bg_stream_action_t action)
  {
  switch(action)
    {
    case BG_STREAM_ACTION_OFF:
      return BGAV_STREAM_MUTE;
      break;
    case BG_STREAM_ACTION_DECODE:
      return BGAV_STREAM_DECODE;
      break;
    case BG_STREAM_ACTION_READRAW:
      return BGAV_STREAM_READRAW;
      break;
    }
  return -1;
  }

int bg_avdec_set_audio_stream(void * priv,
                                  int stream,
                                  bg_stream_action_t action)
  {
  bgav_stream_action_t act;
  avdec_priv * avdec = priv;
  act = get_stream_action(action);
  return bgav_set_audio_stream(avdec->dec, stream, act);
  }

int bg_avdec_set_video_stream(void * priv,
                              int stream,
                              bg_stream_action_t action)
  {
  avdec_priv * avdec = priv;
  bgav_stream_action_t  act;
  act = get_stream_action(action);

  return bgav_set_video_stream(avdec->dec, stream, act);
  }

int bg_avdec_set_subtitle_stream(void * priv,
                              int stream,
                              bg_stream_action_t action)
  {
  bgav_stream_action_t  act;
  avdec_priv * avdec = priv;
  act = get_stream_action(action);

  return bgav_set_subtitle_stream(avdec->dec, stream, act);
  }

int bg_avdec_start(void * priv)
  {
  int i;
  const gavl_video_format_t * format;
  avdec_priv * avdec = priv;
  
  if(!bgav_start(avdec->dec))
    {
    return 0;
    }
  for(i = 0; i < avdec->current_track->num_video_streams; i++)
    {
    gavl_video_format_copy(&(avdec->current_track->video_streams[i].format),
                           bgav_get_video_format(avdec->dec, i));

    gavl_metadata_copy(&avdec->current_track->video_streams[i].m,
                       bgav_get_video_metadata(avdec->dec, i));
    
    avdec->current_track->video_streams[i].duration =
      bgav_video_duration(avdec->dec, i);
    
    }
  for(i = 0; i < avdec->current_track->num_audio_streams; i++)
    {
    gavl_audio_format_copy(&(avdec->current_track->audio_streams[i].format),
                           bgav_get_audio_format(avdec->dec, i));

    gavl_metadata_copy(&avdec->current_track->audio_streams[i].m,
                       bgav_get_audio_metadata(avdec->dec, i));
    
    avdec->current_track->audio_streams[i].duration =
      bgav_audio_duration(avdec->dec, i);
    }
  for(i = 0; i < avdec->current_track->num_subtitle_streams; i++)
    {
    gavl_metadata_copy(&avdec->current_track->subtitle_streams[i].m,
                       bgav_get_subtitle_metadata(avdec->dec, i));
    
    if(bgav_subtitle_is_text(avdec->dec, i))
      {
      avdec->current_track->subtitle_streams[i].is_text = 1;
      }
    avdec->current_track->subtitle_streams[i].duration =
      bgav_subtitle_duration(avdec->dec, i);


    format = bgav_get_subtitle_format(avdec->dec, i);
    gavl_video_format_copy(&avdec->current_track->subtitle_streams[i].format,
                           format);
    }
  return 1;
  }

void bg_avdec_seek(void * priv, int64_t * t, int scale)
  {
  avdec_priv * avdec = priv;
  bgav_seek_scaled(avdec->dec, t, scale);
  }

int bg_avdec_init(avdec_priv * avdec)
  {
  int i;
  const bgav_metadata_t * m;
  const bgav_edl_t * edl;
  const gavl_chapter_list_t * cl;
  
  edl = bgav_get_edl(avdec->dec);
  if(edl)
    avdec->edl = bg_avdec_convert_edl(edl);
    
  avdec->num_tracks = bgav_num_tracks(avdec->dec);
  avdec->track_info = calloc(avdec->num_tracks, sizeof(*(avdec->track_info)));
  
  for(i = 0; i < avdec->num_tracks; i++)
    {
    avdec->track_info[i].num_audio_streams = bgav_num_audio_streams(avdec->dec, i);
    avdec->track_info[i].num_video_streams = bgav_num_video_streams(avdec->dec, i);
    avdec->track_info[i].num_subtitle_streams =
      bgav_num_subtitle_streams(avdec->dec, i);

    if(bgav_can_seek(avdec->dec))
      avdec->track_info[i].flags |= BG_TRACK_SEEKABLE;
    if(bgav_can_pause(avdec->dec))
      avdec->track_info[i].flags |= BG_TRACK_PAUSABLE;
    
    if(avdec->track_info[i].num_audio_streams)
      {
      avdec->track_info[i].audio_streams =
        calloc(avdec->track_info[i].num_audio_streams,
               sizeof(*avdec->track_info[i].audio_streams));
      }
    
    if(avdec->track_info[i].num_video_streams)
      {
      avdec->track_info[i].video_streams =
        calloc(avdec->track_info[i].num_video_streams,
               sizeof(*avdec->track_info[i].video_streams));
      }
    if(avdec->track_info[i].num_subtitle_streams)
      {
      avdec->track_info[i].subtitle_streams =
        calloc(avdec->track_info[i].num_subtitle_streams,
               sizeof(*avdec->track_info[i].subtitle_streams));
      }
    avdec->track_info[i].duration = bgav_get_duration(avdec->dec, i);
    
    /* Get metadata */
    
    m = bgav_get_metadata(avdec->dec, i);
    
    gavl_metadata_copy(&avdec->track_info[i].metadata, m);

    /* Get chapters */

    cl = bgav_get_chapter_list(avdec->dec, i);
    if(cl)
      {
      avdec->track_info[i].chapter_list =
        gavl_chapter_list_copy(cl);
      bg_chapter_list_set_default_names(avdec->track_info[i].chapter_list);
      }
    }
  return 1;
  }

void
bg_avdec_set_parameter(void * p, const char * name,
                       const bg_parameter_value_t * val)
  {
  avdec_priv * avdec = p;
  bg_avdec_option_set_parameter(avdec->opt, name, val);
  }

int bg_avdec_get_num_tracks(void * p)
  {
  avdec_priv * avdec = p;
  return avdec->num_tracks;
  }



int bg_avdec_set_track(void * priv, int track)
  {
  int i;
  avdec_priv * avdec = priv;
  
  if(!bgav_select_track(avdec->dec, track))
    return 0;
  avdec->current_track = &(avdec->track_info[track]);
  

  /* Get formats (need them for compressed output */
  for(i = 0; i < avdec->current_track->num_audio_streams; i++)
    gavl_audio_format_copy(&(avdec->current_track->audio_streams[i].format),
                           bgav_get_audio_format(avdec->dec, i));

  for(i = 0; i < avdec->current_track->num_video_streams; i++)
    gavl_video_format_copy(&(avdec->current_track->video_streams[i].format),
                           bgav_get_video_format(avdec->dec, i));

  
  return 1;
  }

gavl_frame_table_t * bg_avdec_get_frame_table(void * priv, int stream)
  {
  avdec_priv * avdec = priv;
  return bgav_get_frame_table(avdec->dec, stream);
  }


static void metadata_change_callback(void * priv,
                                     const gavl_metadata_t * metadata)
  {
  avdec_priv * avdec;
  avdec = priv;

  /* Merge metadata */

  gavl_metadata_copy(&avdec->current_track->metadata,
                     metadata);
  
  if(avdec->bg_callbacks && avdec->bg_callbacks->metadata_changed)
    {
    avdec->bg_callbacks->metadata_changed(avdec->bg_callbacks->data,
                                          &avdec->current_track->metadata);
    }
  }



void bg_avdec_set_callbacks(void * priv,
                            bg_input_callbacks_t * callbacks)
  {
  bgav_options_t * opt;
  avdec_priv * avdec = priv;
  avdec->bg_callbacks = callbacks;

  if(!callbacks)
    return;
  
  
  bgav_options_set_buffer_callback(avdec->opt,
                           avdec->bg_callbacks->buffer_notify,
                           avdec->bg_callbacks->data);

  bgav_options_set_user_pass_callback(avdec->opt,
                             avdec->bg_callbacks->user_pass,
                             avdec->bg_callbacks->data);
  
  bgav_options_set_aspect_callback(avdec->opt,
                                   avdec->bg_callbacks->aspect_changed,
                                   avdec->bg_callbacks->data);
  
  if(avdec->bg_callbacks->metadata_changed)
    {
    bgav_options_set_metadata_change_callback(avdec->opt,
                                      metadata_change_callback,
                                      priv);
    }
  if(avdec->dec)
    {
    opt = bgav_get_options(avdec->dec);
    bgav_options_copy(opt, avdec->opt);
    }
  }

bg_device_info_t * bg_avdec_get_devices(bgav_device_info_t * info)
  {
  int i = 0;
  bg_device_info_t * ret = NULL;

  if(!info)
    return ret;
  
  while(info[i].device)
    {
    ret = bg_device_info_append(ret,
                                info[i].device,
                                info[i].name);
    i++;
    }
  return ret;
  }

const char * bg_avdec_get_disc_name(void * priv)
  {
  avdec_priv * avdec = priv;
  if(avdec->dec)
    return bgav_get_disc_name(avdec->dec);
  return NULL;
  }

static bg_edl_segment_t * copy_segments(const bgav_edl_segment_t * src, int len)
  {
  int i;
  bg_edl_segment_t * ret;
  ret = calloc(len, sizeof(*ret));
  
  for(i = 0; i < len; i++)
    {
    /* Copy pointers */
    ret[i].url = bg_strdup(ret[i].url, src[i].url);
    ret[i].track = src[i].track;
    ret[i].stream = src[i].stream;
    ret[i].timescale = src[i].timescale;
    ret[i].src_time = src[i].src_time;
    ret[i].dst_time = src[i].dst_time;
    ret[i].dst_duration = src[i].dst_duration;
    ret[i].speed_num = src[i].speed_num;
    ret[i].speed_den = src[i].speed_den;
    }
  return ret;
  }


static bg_edl_stream_t * copy_streams(const bgav_edl_stream_t * src, int len)
  {
  int i;
  bg_edl_stream_t * ret;
  ret = calloc(len, sizeof(*ret));
  
  for(i = 0; i < len; i++)
    {
    ret[i].num_segments = src[i].num_segments;
    ret[i].timescale = src[i].timescale;
    /* Copy pointers */
    ret[i].segments = copy_segments(src[i].segments, src[i].num_segments);
    }
  return ret;
  }

static bg_edl_track_t * copy_tracks(const bgav_edl_track_t * src, int len)
  {
  int i;
  bg_edl_track_t * ret;
  ret = calloc(len, sizeof(*ret));
  
  for(i = 0; i < len; i++)
    {
    /* Metadata */
    if(src[i].metadata)
      gavl_metadata_copy(&ret[i].metadata, src[i].metadata);
    
    /* Copy pointers */
    ret[i].audio_streams = copy_streams(src[i].audio_streams,
                                        src[i].num_audio_streams);
    ret[i].video_streams = copy_streams(src[i].video_streams,
                                        src[i].num_video_streams);
    ret[i].subtitle_text_streams = copy_streams(src[i].subtitle_text_streams,
                                           src[i].num_subtitle_text_streams);
    ret[i].subtitle_overlay_streams = copy_streams(src[i].subtitle_overlay_streams,
                                           src[i].num_subtitle_overlay_streams);
    ret[i].num_audio_streams = src[i].num_audio_streams;
    ret[i].num_video_streams = src[i].num_video_streams;
    ret[i].num_subtitle_text_streams = src[i].num_subtitle_text_streams;
    ret[i].num_subtitle_overlay_streams = src[i].num_subtitle_overlay_streams;
    
    }
  return ret;
  }

bg_edl_t * bg_avdec_convert_edl(const bgav_edl_t * e)
  {
  bg_edl_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->num_tracks = e->num_tracks;
  
  /* Copy pointers */
  ret->tracks = copy_tracks(e->tracks, e->num_tracks);
  ret->url = bg_strdup(ret->url, e->url);
  return ret;
  }

