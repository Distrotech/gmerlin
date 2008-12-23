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


/** \brief Callback for \ref bg_msg_read
 *  \param priv The private data you passed to \ref bg_msg_read
 *  \param data A buffer
 *  \param len Number of bytes to read
 *  \returns The actual number of bytes read
 */

typedef int (*bg_serialize_read_callback_t)(void * priv, uint8_t * data, int len);

/** \brief Callback for \ref bg_msg_write
 *  \param priv The private data you passed to \ref bg_msg_write
 *  \param data A buffer
 *  \param len Number of bytes to write
 *  \returns The actual number of bytes write
 */

typedef int (*bg_serialize_write_callback_t)(void * priv, const uint8_t * data, int len);

/* Formats */

int
bg_serialize_audio_format(const gavl_audio_format_t * format,
                          uint8_t * data, int len);

int
bg_serialize_video_format(const gavl_video_format_t * format,
                          uint8_t * data, int len);


int
bg_deserialize_audio_format(gavl_audio_format_t * format,
                            const uint8_t * data, int len, int * big_endian);

int
bg_deserialize_video_format(gavl_video_format_t * format,
                            const uint8_t * data, int len, int * big_endian);

/* Frames */

int
bg_serialize_audio_frame_header(const gavl_audio_format_t * format,
                                const gavl_audio_frame_t * frame,
                                uint8_t * data, int len);

int
bg_serialize_video_frame_header(const gavl_video_format_t * format,
                                const gavl_video_frame_t * frame,
                                uint8_t * data, int len);


int
bg_serialize_audio_frame(const gavl_audio_format_t * format,
                         const gavl_audio_frame_t * frame,
                         bg_serialize_write_callback_t func, void * data);

int
bg_serialize_video_frame(const gavl_video_format_t * format,
                         const gavl_video_frame_t * frame,
                         bg_serialize_write_callback_t func, void * data);

/* */

int
bg_deserialize_audio_frame_header(const gavl_audio_format_t * format,
                                  gavl_audio_frame_t * frame,
                                  const uint8_t * data, int len);

int
bg_deserialize_video_frame_header(const gavl_video_format_t * format,
                                  gavl_video_frame_t * frame,
                                  const uint8_t * data, int len);


int
bg_deserialize_audio_frame(gavl_dsp_context_t * ctx,
                           const gavl_audio_format_t * format,
                           gavl_audio_frame_t * frame,
                           bg_serialize_read_callback_t func,
                           void * data, int big_endian);

int
bg_deserialize_video_frame(gavl_dsp_context_t * ctx,
                           const gavl_video_format_t * format,
                           gavl_video_frame_t * frame,
                           bg_serialize_read_callback_t func,
                           void * data, int big_endian);

