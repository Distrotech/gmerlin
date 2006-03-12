/*****************************************************************
 
  avdec_common.c
 
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <avdec.h>
#include "avdec_common.h"

static void convert_metadata(bg_metadata_t * dst,
                             const bgav_metadata_t * m)
  {
  const char * str;
  
  str = bgav_metadata_get_author(m);
  if(str)
    dst->author =
      bg_strdup(dst->author, str);

  str = bgav_metadata_get_artist(m);
  if(str)
    dst->artist =
      bg_strdup(dst->artist, str);

  str = bgav_metadata_get_artist(m);
  if(str)
    dst->artist =
      bg_strdup(dst->artist, str);

  str = bgav_metadata_get_album(m);
  if(str)
    dst->album =
      bg_strdup(dst->album, str);

  str = bgav_metadata_get_genre(m);
  if(str)
    dst->genre =
      bg_strdup(dst->genre, str);
    
  str = bgav_metadata_get_title(m);
  //    fprintf(stderr, "Title: %s\n", str);
  if(str)
    dst->title =
      bg_strdup(dst->title, str);
    
  str = bgav_metadata_get_copyright(m);
  if(str)
    dst->copyright = bg_strdup(dst->copyright, str);
    
  str = bgav_metadata_get_comment(m);
  if(str)
    dst->comment = bg_strdup(dst->comment, str);

  str = bgav_metadata_get_date(m);
  if(str)
    dst->date = bg_strdup(dst->date, str);

  dst->track =
    bgav_metadata_get_track(m);
  
  }

void * bg_avdec_create()
  {
  avdec_priv * ret = calloc(1, sizeof(*ret));
  ret->opt = bgav_options_create();
  return ret;
  }

void bg_avdec_close(void * priv)
  {
  avdec_priv * avdec;
  int i;
  avdec = (avdec_priv*)(priv);
  if(avdec->dec)
    {
    bgav_close(avdec->dec);
    avdec->dec = (bgav_t*)0;
    }
  if(avdec->track_info)
    {
    for(i = 0; i < avdec->num_tracks; i++)
      bg_track_info_free(&(avdec->track_info[i]));
    free(avdec->track_info);
    avdec->track_info = (bg_track_info_t*)0;
    }
  }

void bg_avdec_destroy(void * priv)
  {
  avdec_priv * avdec;
  bg_avdec_close(priv);
  avdec = (avdec_priv*)(priv);

  if(avdec->dec)
    {
    bgav_close(avdec->dec);
    }
  if(avdec->opt)
    {
    bgav_options_destroy(avdec->opt);
    }
  
  free(avdec);
  }

bg_track_info_t * bg_avdec_get_track_info(void * priv, int track)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  return &(avdec->track_info[track]);
  }

int bg_avdec_read_video(void * priv,
                            gavl_video_frame_t * frame,
                            int stream)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  return bgav_read_video(avdec->dec, frame, stream);
  }

int bg_avdec_read_audio(void * priv,
                            gavl_audio_frame_t * frame,
                            int stream,
                            int num_samples)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  return bgav_read_audio(avdec->dec, frame, stream, num_samples);
  }

int bg_avdec_has_subtitle(void * priv, int stream)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  return bgav_has_subtitle(avdec->dec, stream);
  }

int bg_avdec_read_subtitle_overlay(void * priv,
                                   gavl_overlay_t * ovl, int stream)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  return bgav_read_subtitle_overlay(avdec->dec, ovl, stream);
  }
  
int bg_avdec_read_subtitle_text(void * priv,
                                char ** text, int * text_alloc,
                                int64_t * start_time,
                                int64_t * duration,
                                int stream)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);

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
    case BG_STREAM_ACTION_BYPASS:
      return -1;
      break;
    }
  return -1;
  }

int bg_avdec_set_audio_stream(void * priv,
                                  int stream,
                                  bg_stream_action_t action)
  {
  bgav_stream_action_t act;
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  act = get_stream_action(action);
  return bgav_set_audio_stream(avdec->dec, stream, act);
  }

int bg_avdec_set_video_stream(void * priv,
                              int stream,
                              bg_stream_action_t action)
  {
  avdec_priv * avdec;
  bgav_stream_action_t  act;
  avdec = (avdec_priv*)(priv);
  act = get_stream_action(action);

  //  fprintf(stderr, "bg_avdec_set_video_stream %d %d\n", stream, action);
  return bgav_set_video_stream(avdec->dec, stream, act);
  }

int bg_avdec_set_subtitle_stream(void * priv,
                              int stream,
                              bg_stream_action_t action)
  {
  avdec_priv * avdec;
  bgav_stream_action_t  act;
  avdec = (avdec_priv*)(priv);
  act = get_stream_action(action);

  //  fprintf(stderr, "bg_avdec_set_video_stream %d %d\n", stream, action);
  return bgav_set_subtitle_stream(avdec->dec, stream, act);
  }

int bg_avdec_start(void * priv)
  {
  int i;
  const char * str;
  avdec_priv * avdec;
  const gavl_video_format_t * format;
  avdec = (avdec_priv*)(priv);
  
  if(!bgav_start(avdec->dec))
    {
    return 0;
    }
  for(i = 0; i < avdec->current_track->num_video_streams; i++)
    {
    gavl_video_format_copy(&(avdec->current_track->video_streams[i].format),
                           bgav_get_video_format(avdec->dec, i));

    str = bgav_get_video_description(avdec->dec, i);
    if(str)
      avdec->current_track->video_streams[i].description = bg_strdup(NULL, str);
    }
  for(i = 0; i < avdec->current_track->num_audio_streams; i++)
    {
    gavl_audio_format_copy(&(avdec->current_track->audio_streams[i].format),
                           bgav_get_audio_format(avdec->dec, i));
    str = bgav_get_audio_description(avdec->dec, i);
    if(str)
      avdec->current_track->audio_streams[i].description = bg_strdup(NULL, str);

    str = bgav_get_audio_language(avdec->dec, i);
    if(str)
      avdec->current_track->audio_streams[i].language = bg_strdup(NULL, str);

    }
  for(i = 0; i < avdec->current_track->num_subtitle_streams; i++)
    {
    str = bgav_get_subtitle_language(avdec->dec, i);
    if(str)
      avdec->current_track->subtitle_streams[i].language = bg_strdup(NULL, str);
    format = bgav_get_subtitle_format(avdec->dec, i);
    if(!format)
      avdec->current_track->subtitle_streams[i].is_text = 1;
    else
      gavl_video_format_copy(&avdec->current_track->subtitle_streams[i].format,
                             format);
    }
    
  //  bgav_dump(avdec->dec);
  return 1;
  }

void bg_avdec_seek(void * priv, gavl_time_t * t)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  //  fprintf(stderr, "seek_bgav: percentage: %f, duration: %f, pos: %f\n",
  //          percentage, gavl_time_to_seconds(avdec->track_info->duration),
  //          gavl_time_to_seconds(pos));
  bgav_seek(avdec->dec, t);
  }


int bg_avdec_init(avdec_priv * avdec)
  {
  int i;
  const bgav_metadata_t * m;
  
  avdec->num_tracks = bgav_num_tracks(avdec->dec);
  avdec->track_info = calloc(avdec->num_tracks, sizeof(*(avdec->track_info)));

  for(i = 0; i < avdec->num_tracks; i++)
    {
    avdec->track_info[i].num_audio_streams = bgav_num_audio_streams(avdec->dec, i);
    avdec->track_info[i].num_video_streams = bgav_num_video_streams(avdec->dec, i);
    avdec->track_info[i].num_subtitle_streams =
      bgav_num_subtitle_streams(avdec->dec, i);
    avdec->track_info[i].seekable = bgav_can_seek(avdec->dec);

    //    fprintf(stderr, "bg_avdec_init: subtitles: %d\n", 
    //            avdec->track_info[i].num_subtitle_streams);
                
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
    avdec->track_info[i].name =
      bg_strdup(avdec->track_info[i].name,
                bgav_get_track_name(avdec->dec, i));

    /* Get metadata */
    
    m = bgav_get_metadata(avdec->dec, i);
    convert_metadata(&(avdec->track_info[i].metadata), m);
    }
  return 1;
  }

void
bg_avdec_set_parameter(void * p, char * name,
                    bg_parameter_value_t * val)
  {
  int i_tmp;
  avdec_priv * avdec;
  avdec = (avdec_priv*)(p);
  if(!name)
    return;
  else if(!strcmp(name, "connect_timeout"))
    {
    /* Set parameters */
  
    bgav_options_set_connect_timeout(avdec->opt, val->val_i);
    }
  else if(!strcmp(name, "read_timeout"))
    {
    bgav_options_set_read_timeout(avdec->opt, val->val_i);
    }
  else if(!strcmp(name, "network_buffer_size"))
    {
    bgav_options_set_network_buffer_size(avdec->opt, val->val_i * 1024);
    }
  else if(!strcmp(name, "network_bandwidth"))
    {
    if(!strcmp(val->val_str, "14.4 Kbps (Modem)"))
      i_tmp = 14400;
    else if(!strcmp(val->val_str, "19.2 Kbps (Modem)"))
      i_tmp = 19200;
    else if(!strcmp(val->val_str, "28.8 Kbps (Modem)"))
      i_tmp = 28800;
    else if(!strcmp(val->val_str, "33.6 Kbps (Modem)"))
      i_tmp = 33600;
    else if(!strcmp(val->val_str, "34.4 Kbps (Modem)"))
      i_tmp = 34430;
    else if(!strcmp(val->val_str, "57.6 Kbps (Modem)"))
      i_tmp = 57600;
    else if(!strcmp(val->val_str, "115.2 Kbps (ISDN)"))
      i_tmp = 115200;
    else if(!strcmp(val->val_str, "262.2 Kbps (Cable/DSL)"))
      i_tmp = 262200;
    else if(!strcmp(val->val_str, "393.2 Kbps (Cable/DSL)"))
      i_tmp = 393216;
    else if(!strcmp(val->val_str, "524.3 Kbps (Cable/DSL)"))
      i_tmp = 524300;
    else if(!strcmp(val->val_str, "1.5 Mbps (T1)"))
      i_tmp = 1544000;
    else if(!strcmp(val->val_str, "10.5 Mbps (LAN)"))
      i_tmp = 10485800;
    bgav_options_set_network_bandwidth(avdec->opt, i_tmp);
    }
  else if(!strcmp(name, "http_shoutcast_metadata"))
    {
    bgav_options_set_http_shoutcast_metadata(avdec->opt, val->val_i);
    }
  else if(!strcmp(name, "http_use_proxy"))
    {
    bgav_options_set_http_use_proxy(avdec->opt, val->val_i);
    }
  else if(!strcmp(name, "http_proxy_host"))
    {
    bgav_options_set_http_proxy_host(avdec->opt, val->val_str);
    }
  else if(!strcmp(name, "http_proxy_port"))
    {
    bgav_options_set_http_proxy_port(avdec->opt, val->val_i);
    }
  else if(!strcmp(name, "http_proxy_auth"))
    {
    bgav_options_set_http_proxy_auth(avdec->opt, val->val_i);
    }
  else if(!strcmp(name, "http_proxy_user"))
    {
    bgav_options_set_http_proxy_user(avdec->opt, val->val_str);
    }
  else if(!strcmp(name, "http_proxy_pass"))
    {
    bgav_options_set_http_proxy_pass(avdec->opt, val->val_str);
    }
  else if(!strcmp(name, "ftp_anonymous_password"))
    {
    bgav_options_set_ftp_anonymous_password(avdec->opt, val->val_str);
    }
  else if(!strcmp(name, "ftp_anonymous"))
    {
    bgav_options_set_ftp_anonymous(avdec->opt, val->val_i);
    }
  else if(!strcmp(name, "default_subtitle_encoding"))
    {
    bgav_options_set_default_subtitle_encoding(avdec->opt, val->val_str);
    }
  else if(!strcmp(name, "dvd_chapters_as_tracks"))
    {
    bgav_options_set_dvd_chapters_as_tracks(avdec->opt, val->val_i);
    }
          
  }

int bg_avdec_get_num_tracks(void * p)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(p);
  return avdec->num_tracks;
  }



int bg_avdec_set_track(void * priv, int track)
  {
  const char * str;
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  
  bgav_select_track(avdec->dec, track);
  avdec->current_track = &(avdec->track_info[track]);
  
  str = bgav_get_description(avdec->dec);
  if(str)
    avdec->track_info[track].description =
      bg_strdup(avdec->track_info[track].description, str);


  return 1;
  }

static void metadata_change_callback(void * priv,
                                     const bgav_metadata_t * metadata)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);

  bg_metadata_t m;
  
  if(avdec->bg_callbacks && avdec->bg_callbacks->metadata_changed)
    {
    memset(&m, 0, sizeof(m));
    convert_metadata(&m, metadata);

    avdec->bg_callbacks->metadata_changed(avdec->bg_callbacks->data,
                                          &m);
    bg_metadata_free(&m);
    }
  }


void bg_avdec_set_callbacks(void * priv,
                            bg_input_callbacks_t * callbacks)
  {
  bgav_options_t * opt;
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  avdec->bg_callbacks = callbacks;

  if(!callbacks)
    return;
  
  bgav_options_set_name_change_callback(avdec->opt,
                                avdec->bg_callbacks->name_changed,
                                avdec->bg_callbacks->data);
  
  bgav_options_set_name_change_callback(avdec->opt,
                                avdec->bg_callbacks->name_changed,
                                avdec->bg_callbacks->data);
  
  bgav_options_set_track_change_callback(avdec->opt,
                                 avdec->bg_callbacks->track_changed,
                                 avdec->bg_callbacks->data);

  bgav_options_set_buffer_callback(avdec->opt,
                           avdec->bg_callbacks->buffer_notify,
                           avdec->bg_callbacks->data);

  bgav_options_set_user_pass_callback(avdec->opt,
                             avdec->bg_callbacks->user_pass,
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

const char * bg_avdec_get_error(void * priv)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  if(avdec->dec)
    return bgav_get_error(avdec->dec);
  return (const char *)0;
  }


bg_device_info_t * bg_avdec_get_devices(bgav_device_info_t * info)
  {
  int i = 0;
  bg_device_info_t * ret = (bg_device_info_t *)0;

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

