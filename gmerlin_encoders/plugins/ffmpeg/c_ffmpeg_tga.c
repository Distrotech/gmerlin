/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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
#include <config.h>
#include <gmerlin/plugin.h>
#include <gmerlin/translation.h>

#include "ffmpeg_common.h"

#define CODEC_NAME "c_ffmpeg_tga"
#define CODEC_LONG_NAME TRS("Targa")
#define CODEC_ID CODEC_ID_TARGA
#define CODEC_DESC TRS("libavcodec Targa encoder")

#include "_codec_plugin.c"
