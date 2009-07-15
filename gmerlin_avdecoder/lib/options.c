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


#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>

/* Configuration stuff */

void bgav_options_set_connect_timeout(bgav_options_t *b, int timeout)
  {
  b->connect_timeout = timeout;
  }

void bgav_options_set_read_timeout(bgav_options_t *b, int timeout)
  {
  b->read_timeout = timeout;
  }

/*
 *  Set network bandwidth (in bits per second)
 */

void bgav_options_set_network_bandwidth(bgav_options_t *b, int bandwidth)
  {
  b->network_bandwidth = bandwidth;
  }

void bgav_options_set_network_buffer_size(bgav_options_t *b, int size)
  {
  b->network_buffer_size = size;
  }


void bgav_options_set_http_use_proxy(bgav_options_t*b, int use_proxy)
  {
  b->http_use_proxy = use_proxy;
  }

void bgav_options_set_http_proxy_host(bgav_options_t*b, const char * h)
  {
  if(b->http_proxy_host)
    free(b->http_proxy_host);
  b->http_proxy_host = bgav_strdup(h);
  }

void bgav_options_set_http_proxy_port(bgav_options_t*b, int p)
  {
  b->http_proxy_port = p;
  }

void bgav_options_set_rtp_port_base(bgav_options_t*b, int p)
  {
  b->rtp_port_base = p;
  }

void bgav_options_set_rtp_try_tcp(bgav_options_t*b, int p)
  {
  b->rtp_try_tcp = p;
  }

void bgav_options_set_sample_accurate(bgav_options_t*b, int p)
  {
  b->sample_accurate = p;
  }

void bgav_options_set_cache_time(bgav_options_t*opt, int t)
  {
  opt->cache_time = t;
  }

void bgav_options_set_cache_size(bgav_options_t*opt, int s)
  {
  opt->cache_size = s;
  }

void bgav_options_set_http_proxy_auth(bgav_options_t*b, int i)
  {
  b->http_proxy_auth = i;
  }

void bgav_options_set_http_proxy_user(bgav_options_t*b, const char * h)
  {
  if(b->http_proxy_user)
    free(b->http_proxy_user);
  b->http_proxy_user = bgav_strdup(h);
  }

void bgav_options_set_http_proxy_pass(bgav_options_t*b, const char * h)
  {
  if(b->http_proxy_pass)
    free(b->http_proxy_pass);
  b->http_proxy_pass = bgav_strdup(h);
  }


void bgav_options_set_http_shoutcast_metadata(bgav_options_t*b, int m)
  {
  b->http_shoutcast_metadata = m;
  }

void bgav_options_set_ftp_anonymous_password(bgav_options_t*b, const char * h)
  {
  if(b->ftp_anonymous_password)
    free(b->ftp_anonymous_password);
  b->ftp_anonymous_password = bgav_strdup(h);
  }

void bgav_options_set_ftp_anonymous(bgav_options_t*b, int anonymous)
  {
  b->ftp_anonymous = anonymous;
  }


void bgav_options_set_audio_dynrange(bgav_options_t* opt, int audio_dynrange)
  {
  opt->audio_dynrange = audio_dynrange;
  }



void bgav_options_set_default_subtitle_encoding(bgav_options_t* b,
                                                const char* encoding)
  {
  if(b->default_subtitle_encoding)
    free(b->default_subtitle_encoding);
  b->default_subtitle_encoding = bgav_strdup(encoding);
  }

void bgav_options_set_seamless(bgav_options_t* opt,
                               int seamless)
  {
  opt->seamless = seamless;
  }

void bgav_options_set_seek_subtitles(bgav_options_t* opt,
                                     int seek_subtitles)
  {
  opt->seek_subtitles = seek_subtitles;
  }

void bgav_options_set_pp_level(bgav_options_t* opt,
                               int pp_level)
  {
  opt->pp_level = pp_level;
  if(opt->pp_level < 0)
    opt->pp_level = 0;
  if(opt->pp_level > 6)
    opt->pp_level = 6;
  }

void bgav_options_set_dvb_channels_file(bgav_options_t* opt,
                                        const char * file)
  {
  if(opt->dvb_channels_file)
    free(opt->dvb_channels_file);
  opt->dvb_channels_file = bgav_strdup(file);
  }

void bgav_options_set_prefer_ffmpeg_demuxers(bgav_options_t* opt,
                                             int prefer)
  {
  opt->prefer_ffmpeg_demuxers = prefer;
  }

void bgav_options_set_dv_datetime(bgav_options_t* opt,
                                  int datetime)
  {
  opt->dv_datetime = datetime;
  }

void bgav_options_set_shrink(bgav_options_t* opt,
                             int shrink)
  {
  opt->shrink = shrink;
  }

void bgav_options_set_vdpau(bgav_options_t* opt,
                            int vdpau)
  {
  opt->vdpau = vdpau;
  }

#define FREE(ptr) if(ptr) free(ptr)

void bgav_options_free(bgav_options_t*opt)
  {
  FREE(opt->ftp_anonymous_password);
  FREE(opt->http_proxy_host);
  FREE(opt->http_proxy_user);
  FREE(opt->http_proxy_pass);
  FREE(opt->default_subtitle_encoding);
  FREE(opt->dvb_channels_file);
  }

void bgav_options_set_defaults(bgav_options_t * b)
  {
  memset(b, 0, sizeof(*b));
  b->connect_timeout = 10000;
  b->read_timeout = 10000;
  b->ftp_anonymous = 1;
  b->default_subtitle_encoding = bgav_strdup("LATIN1");
  b->audio_dynrange = 1;
  b->cache_time = 500;
  b->cache_size = 20;
  // Test
  //  b->rtp_try_tcp = 1;
  }

bgav_options_t * bgav_options_create()
  {
  bgav_options_t * ret;
  bgav_translation_init();
  ret = calloc(1, sizeof(*ret));
  bgav_options_set_defaults(ret);
  return ret;
  }

void bgav_options_destroy(bgav_options_t * opt)
  {
  bgav_options_free(opt);
  free(opt);
  }

#define CP_INT(i) dst->i = src->i
#define CP_STR(s) if(dst->s) free(dst->s); dst->s = bgav_strdup(src->s)

void bgav_options_copy(bgav_options_t * dst, const bgav_options_t * src)
  {
  CP_INT(sample_accurate);
  CP_INT(cache_time);
  CP_INT(cache_size);
  /* Generic network options */
  CP_INT(connect_timeout);
  CP_INT(read_timeout);

  CP_INT(network_bandwidth);
  CP_INT(network_buffer_size);

  CP_INT(rtp_try_tcp);
  CP_INT(rtp_port_base);
  
  /* http options */

  CP_INT(http_use_proxy);
  CP_STR(http_proxy_host);
  CP_INT(http_proxy_port);

  CP_INT(http_proxy_auth);
  
  CP_STR(http_proxy_user);
  CP_STR(http_proxy_pass);
  
  CP_INT(http_shoutcast_metadata);

  /* ftp options */
    
  CP_STR(ftp_anonymous_password);
  CP_INT(ftp_anonymous);

  /* Subtitle */
  
  CP_STR(default_subtitle_encoding);
  CP_INT(seek_subtitles);

  /* Postprocessing */
  
  CP_INT(pp_level);
  
  /* DVD */

  /* DVB */
  
  CP_STR(dvb_channels_file);
  
  /* Audio */

  CP_INT(audio_dynrange);
  
  CP_INT(prefer_ffmpeg_demuxers);
  CP_INT(dv_datetime);
  CP_INT(shrink);

  CP_INT(vdpau);

  /* Callbacks */
  
  CP_INT(name_change_callback);
  CP_INT(name_change_callback_data);

  CP_INT(log_callback);
  CP_INT(log_callback_data);
    
  CP_INT(metadata_change_callback);
  CP_INT(metadata_change_callback_data);

  CP_INT(track_change_callback);
  CP_INT(track_change_callback_data);

  CP_INT(buffer_callback);
  CP_INT(buffer_callback_data);

  CP_INT(user_pass_callback);
  CP_INT(user_pass_callback_data);

  CP_INT(aspect_callback);
  CP_INT(aspect_callback_data);

  CP_INT(index_callback);
  CP_INT(index_callback_data);
  
  }

#undef CP_INT
#undef CP_STR

void
bgav_options_set_name_change_callback(bgav_options_t * opt,
                                      bgav_name_change_callback callback,
                                      void * data)
  {
  opt->name_change_callback      = callback;
  opt->name_change_callback_data = data;
  }

void
bgav_options_set_metadata_change_callback(bgav_options_t * opt,
                                          bgav_metadata_change_callback callback,
                                          void * data)
  {
  opt->metadata_change_callback      = callback;
  opt->metadata_change_callback_data = data;
  }

void
bgav_options_set_user_pass_callback(bgav_options_t * opt,
                                    bgav_user_pass_callback callback,
                                    void * data)
  {
  opt->user_pass_callback      = callback;
  opt->user_pass_callback_data = data;
  }


void
bgav_options_set_track_change_callback(bgav_options_t * opt,
                                       bgav_track_change_callback callback,
                                       void * data)
  {
  opt->track_change_callback      = callback;
  opt->track_change_callback_data = data;
  }


void
bgav_options_set_buffer_callback(bgav_options_t * opt,
                         bgav_buffer_callback callback,
                         void * data)
  {
  opt->buffer_callback      = callback;
  opt->buffer_callback_data = data;
  }

void
bgav_options_set_log_callback(bgav_options_t * opt,
                              bgav_log_callback callback,
                              void * data)
  {
  opt->log_callback      = callback;
  opt->log_callback_data = data;
  }

void
bgav_options_set_aspect_callback(bgav_options_t * opt,
                              bgav_aspect_callback callback,
                              void * data)
  {
  opt->aspect_callback      = callback;
  opt->aspect_callback_data = data;
  }

void
bgav_options_set_index_callback(bgav_options_t * opt,
                                bgav_index_callback callback,
                                void * data)
  {
  opt->index_callback      = callback;
  opt->index_callback_data = data;
  }

