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

#include <string.h>

#include <gmerlin/parameter.h>
#include <avdec.h>
#include "options.h"

void
bg_avdec_option_set_parameter(bgav_options_t * opt, const char * name,
                              const bg_parameter_value_t * val)
  {
  if(!name)
    return;
  else if(!strcmp(name, "connect_timeout"))
    {
    /* Set parameters */
  
    bgav_options_set_connect_timeout(opt, val->val_i);
    }
  else if(!strcmp(name, "read_timeout"))
    {
    bgav_options_set_read_timeout(opt, val->val_i);
    }
  else if(!strcmp(name, "network_buffer_size"))
    {
    bgav_options_set_network_buffer_size(opt, val->val_i * 1024);
    }
  else if(!strcmp(name, "network_bandwidth"))
    {
    bgav_options_set_network_bandwidth(opt, atoi(val->val_str));
    }
  else if(!strcmp(name, "http_shoutcast_metadata"))
    {
    bgav_options_set_http_shoutcast_metadata(opt, val->val_i);
    }
  else if(!strcmp(name, "http_use_proxy"))
    {
    bgav_options_set_http_use_proxy(opt, val->val_i);
    }
  else if(!strcmp(name, "http_proxy_host"))
    {
    bgav_options_set_http_proxy_host(opt, val->val_str);
    }
  else if(!strcmp(name, "http_proxy_port"))
    {
    bgav_options_set_http_proxy_port(opt, val->val_i);
    }
  else if(!strcmp(name, "http_proxy_auth"))
    {
    bgav_options_set_http_proxy_auth(opt, val->val_i);
    }
  else if(!strcmp(name, "http_proxy_user"))
    {
    bgav_options_set_http_proxy_user(opt, val->val_str);
    }
  else if(!strcmp(name, "http_proxy_pass"))
    {
    bgav_options_set_http_proxy_pass(opt, val->val_str);
    }
  else if(!strcmp(name, "rtp_try_tcp"))
    {
    bgav_options_set_rtp_try_tcp(opt, val->val_i);
    }
  else if(!strcmp(name, "rtp_port_base"))
    {
    bgav_options_set_rtp_port_base(opt, val->val_i);
    }
  else if(!strcmp(name, "ftp_anonymous_password"))
    {
    bgav_options_set_ftp_anonymous_password(opt, val->val_str);
    }
  else if(!strcmp(name, "ftp_anonymous"))
    {
    bgav_options_set_ftp_anonymous(opt, val->val_i);
    }
  else if(!strcmp(name, "default_subtitle_encoding"))
    {
    bgav_options_set_default_subtitle_encoding(opt, val->val_str);
    }
#if 0
  else if(!strcmp(name, "dvd_chapters_as_tracks"))
    {
    bgav_options_set_dvd_chapters_as_tracks(opt, val->val_i);
    }
#endif
  else if(!strcmp(name, "audio_dynrange"))
    {
    bgav_options_set_audio_dynrange(opt, val->val_i);
    }
  else if(!strcmp(name, "seek_subtitles"))
    {
    if(!strcmp(val->val_str, "video"))
      bgav_options_set_seek_subtitles(opt, 1);
    else if(!strcmp(val->val_str, "always"))
      bgav_options_set_seek_subtitles(opt, 2);
    else
      bgav_options_set_seek_subtitles(opt, 0);
    }
  else if(!strcmp(name, "video_postprocessing_level"))
    {
    bgav_options_set_postprocessing_level(opt, val->val_f);
    }
  else if(!strcmp(name, "dvb_channels_file"))
    {
    bgav_options_set_dvb_channels_file(opt, val->val_str);
    }
  else if(!strcmp(name, "sample_accuracy"))
    {
    if(!strcmp(val->val_str, "never"))
      bgav_options_set_sample_accurate(opt, 0);
    else if(!strcmp(val->val_str, "always"))
      bgav_options_set_sample_accurate(opt, 1);
    else if(!strcmp(val->val_str, "when_necessary"))
      bgav_options_set_sample_accurate(opt, 2);
    }
  else if(!strcmp(name, "cache_size"))
    {
    bgav_options_set_cache_size(opt, val->val_i);
    }
  else if(!strcmp(name, "cache_time"))
    {
    bgav_options_set_cache_time(opt, val->val_i);
    }
  else if(!strcmp(name, "dv_datetime"))
    {
    bgav_options_set_dv_datetime(opt, val->val_i);
    }
  else if(!strcmp(name, "shrink"))
    {
    bgav_options_set_shrink(opt, val->val_i);
    }
  else if(!strcmp(name, "vdpau"))
    {
    bgav_options_set_vdpau(opt, val->val_i);
    }
  else if(!strcmp(name, "threads"))
    {
    bgav_options_set_threads(opt, val->val_i);
    }
  }
