/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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


/* Messages of the transcoder */

/*
 *  0: Number of streams
 */

#define BG_TRANSCODER_MSG_NUM_AUDIO_STREAMS 1

/*
 *  0: stream index
 *  1: input format
 *  2: output format
 */

#define BG_TRANSCODER_MSG_AUDIO_FORMAT      2



/*
 *  0: Number of streams
 */

#define BG_TRANSCODER_MSG_NUM_VIDEO_STREAMS 4

/*
 *  0: stream index
 *  1: input format
 *  2: output format
 */

#define BG_TRANSCODER_MSG_VIDEO_FORMAT      5

/* Output file (0: Filename, 1: pp_only) */

#define BG_TRANSCODER_MSG_FILE              7

/*
 *  arg 1: float percentage_done
 *  arg 2: gavl_time_t remaining_time
 */

#define BG_TRANSCODER_MSG_PROGRESS          8

#define BG_TRANSCODER_MSG_FINISHED          9

/*
 *  arg 1: What started? (transcoding etc.)
 */

#define BG_TRANSCODER_MSG_START            10

/*
 *  arg 1: Metadata
 */

#define BG_TRANSCODER_MSG_METADATA         11

/*
 *  arg 1: Error message
 */

#define BG_TRANSCODER_MSG_ERROR            12
