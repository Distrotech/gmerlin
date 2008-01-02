/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <lqt.h>
#include <lqt_codecinfo.h>
#include <colormodels.h>
#include <parameter.h>

void bg_lqt_create_codec_info(bg_parameter_info_t * parameter_info,
                              int audio, int video, int encode, int decode);

int bg_lqt_set_parameter(const char * name, bg_parameter_value_t * val,
                         bg_parameter_info_t * info);

extern int * bg_lqt_supported_colormodels;
                     
void bg_lqt_set_audio_parameter(quicktime_t * file,
                                int stream,
                                const char * name,
                                const bg_parameter_value_t * val,
                                lqt_parameter_info_t * lqt_parameter_info);

void bg_lqt_set_video_parameter(quicktime_t * file,
                                int stream,
                                const char * name,
                                const bg_parameter_value_t * val,
                                lqt_parameter_info_t * lqt_parameter_info);

void bg_lqt_set_audio_decoder_parameter(const char * codec_name,
                                        const char * parameter_name,
                                        const bg_parameter_value_t * val);
void bg_lqt_set_video_decoder_parameter(const char * codec_name,
                                        const char * parameter_name,
                                        const bg_parameter_value_t * val);

void bg_lqt_log(lqt_log_level_t level,
                const char * log_domain,
                const char * message,
                void * data);
