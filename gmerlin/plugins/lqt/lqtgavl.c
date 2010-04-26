/*****************************************************************
 * lqtgavl - gavl Bindings for libquicktime
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

#include <stdlib.h>
#include <string.h>

#include <lqtgavl.h>
#include <colormodels.h>
#include <lqt_version.h>

/* Sampleformat conversion */

static const struct
  {
  lqt_sample_format_t lqt;
  gavl_sample_format_t gavl;
  }
sampleformats[] =
  {
    { LQT_SAMPLE_INT8,  GAVL_SAMPLE_S8 },
    { LQT_SAMPLE_UINT8, GAVL_SAMPLE_U8 },
    { LQT_SAMPLE_INT16, GAVL_SAMPLE_S16 },
    { LQT_SAMPLE_INT32, GAVL_SAMPLE_S32 },
    { LQT_SAMPLE_FLOAT, GAVL_SAMPLE_FLOAT },
    { LQT_SAMPLE_DOUBLE, GAVL_SAMPLE_DOUBLE }
  };

static gavl_sample_format_t
sampleformat_lqt_2_gavl(lqt_sample_format_t format)
  {
  int i;
  for(i = 0; i < sizeof(sampleformats) / sizeof(sampleformats[0]); i++)
    {
    if(sampleformats[i].lqt == format)
      return sampleformats[i].gavl;
    }
  return GAVL_SAMPLE_NONE;
  }

/* Chroma placement */

static const struct
  {
  lqt_chroma_placement_t lqt;
  gavl_chroma_placement_t gavl;
  }
chroma_placements[] =
  {
    {  LQT_CHROMA_PLACEMENT_DEFAULT, GAVL_CHROMA_PLACEMENT_DEFAULT },
    {  LQT_CHROMA_PLACEMENT_MPEG2,   GAVL_CHROMA_PLACEMENT_MPEG2   },
    {  LQT_CHROMA_PLACEMENT_DVPAL,   GAVL_CHROMA_PLACEMENT_DVPAL   },
  };

static gavl_chroma_placement_t
chroma_placement_lqt_2_gavl(lqt_chroma_placement_t chroma_placement)
  {
  int i;
  for(i = 0; i < sizeof(chroma_placements) / sizeof(chroma_placements[0]); i++)
    {
    if(chroma_placements[i].lqt == chroma_placement)
      return chroma_placements[i].gavl;
    }
  return GAVL_CHROMA_PLACEMENT_DEFAULT;
  }

/* Interlace mode */

static const struct
  {
  lqt_interlace_mode_t lqt;
  gavl_interlace_mode_t gavl;
  }
interlace_modes[] =
  {
    { LQT_INTERLACE_NONE,         GAVL_INTERLACE_NONE},
    { LQT_INTERLACE_TOP_FIRST,    GAVL_INTERLACE_TOP_FIRST },
    { LQT_INTERLACE_BOTTOM_FIRST, GAVL_INTERLACE_BOTTOM_FIRST },
  };

static gavl_interlace_mode_t
interlace_mode_lqt_2_gavl(lqt_interlace_mode_t interlace_mode)
  {
  int i;
  for(i = 0; i < sizeof(interlace_modes) / sizeof(interlace_modes[0]); i++)
    {
    if(interlace_modes[i].lqt == interlace_mode)
      return interlace_modes[i].gavl;
    }
  return GAVL_INTERLACE_NONE;
  }

static lqt_interlace_mode_t
interlace_mode_gavl_2_lqt(gavl_interlace_mode_t interlace_mode)
  {
  int i;
  for(i = 0; i < sizeof(interlace_modes) / sizeof(interlace_modes[0]); i++)
    {
    if(interlace_modes[i].gavl == interlace_mode)
      return interlace_modes[i].lqt;
    }
  return LQT_INTERLACE_NONE;
  }

/* Pixelformats */

static const struct
  {
  int lqt;
  gavl_pixelformat_t gavl;
  }
pixelformats[] =
  {
    { BC_RGB565,       GAVL_RGB_16 },
    { BC_BGR565,       GAVL_BGR_16 },
    { BC_BGR888,       GAVL_BGR_24 },
    { BC_BGR8888,      GAVL_BGR_32 },
    { BC_RGB888,       GAVL_RGB_24 },
    { BC_RGBA8888,     GAVL_RGBA_32 },
    { BC_RGB161616,    GAVL_RGB_48 },
    { BC_RGBA16161616, GAVL_RGBA_64 },
    { BC_YUVA8888,     GAVL_YUVA_32 },
    { BC_YUV422,       GAVL_YUY2 },
    { BC_YUV420P,      GAVL_YUV_420_P },
    { BC_YUV422P,      GAVL_YUV_422_P },
    { BC_YUV444P,      GAVL_YUV_444_P },
    { BC_YUVJ420P,     GAVL_YUVJ_420_P },
    { BC_YUVJ422P,     GAVL_YUVJ_422_P },
    { BC_YUVJ444P,     GAVL_YUVJ_444_P },
    { BC_YUV411P,      GAVL_YUV_411_P },
    { BC_YUV422P16,    GAVL_YUV_422_P_16 },
    { BC_YUV444P16,    GAVL_YUV_444_P_16 },
  };

static gavl_pixelformat_t
pixelformat_lqt_2_gavl(int pixelformat)
  {
  int i;
  for(i = 0; i < sizeof(pixelformats) / sizeof(pixelformats[0]); i++)
    {
    if(pixelformats[i].lqt == pixelformat)
      return pixelformats[i].gavl;
    }
  return GAVL_PIXELFORMAT_NONE;
  }

static int
pixelformat_gavl_2_lqt(gavl_pixelformat_t pixelformat)
  {
  int i;
  for(i = 0; i < sizeof(pixelformats) / sizeof(pixelformats[0]); i++)
    {
    if(pixelformats[i].gavl == pixelformat)
      return pixelformats[i].lqt;
    }
  return LQT_COLORMODEL_NONE;
  }

static const struct
  {
  gavl_channel_id_t gavl;
  lqt_channel_t lqt;
  }
channels[] =
  {
    { GAVL_CHID_NONE,               LQT_CHANNEL_UNKNOWN            },
    { GAVL_CHID_FRONT_CENTER,       LQT_CHANNEL_FRONT_CENTER       },
    { GAVL_CHID_FRONT_LEFT,         LQT_CHANNEL_FRONT_LEFT         },
    { GAVL_CHID_FRONT_RIGHT,        LQT_CHANNEL_FRONT_RIGHT        },
    { GAVL_CHID_FRONT_CENTER_LEFT,  LQT_CHANNEL_FRONT_CENTER_LEFT  },
    { GAVL_CHID_FRONT_CENTER_RIGHT, LQT_CHANNEL_FRONT_CENTER_RIGHT },
    { GAVL_CHID_REAR_LEFT,          LQT_CHANNEL_BACK_LEFT          },
    { GAVL_CHID_REAR_RIGHT,         LQT_CHANNEL_BACK_RIGHT         },
    { GAVL_CHID_REAR_CENTER,        LQT_CHANNEL_BACK_CENTER        },
    { GAVL_CHID_SIDE_LEFT,          LQT_CHANNEL_SIDE_LEFT          },
    { GAVL_CHID_SIDE_RIGHT,         LQT_CHANNEL_SIDE_RIGHT         },
    { GAVL_CHID_LFE,                LQT_CHANNEL_LFE                },
    { GAVL_CHID_AUX,                LQT_CHANNEL_UNKNOWN            },
  };

static gavl_channel_id_t
channel_lqt_2_gavl(lqt_channel_t ch)
  {
  int i;
  for(i = 0; i < sizeof(channels) / sizeof(channels[0]); i++)
    {
    if(channels[i].lqt == ch)
      return channels[i].gavl;
    }
  return GAVL_CHID_NONE;
  }

static lqt_channel_t
channel_gavl_2_lqt(gavl_channel_id_t ch)
  {
  int i;
  for(i = 0; i < sizeof(channels) / sizeof(channels[0]); i++)
    {
    if(channels[i].gavl == ch)
      return channels[i].lqt;
    }
  return LQT_CHANNEL_UNKNOWN;
  }

/* Timecode flags */

static const struct
  {
  int gavl;
  uint32_t lqt;
  }
timecode_flags[] =
  {
    { GAVL_TIMECODE_DROP_FRAME, LQT_TIMECODE_DROP },
  };

static int timecode_flags_lqt_2_gavl(uint32_t lqt)
  {
  int i;
  int ret = 0;
  for(i = 0; i < sizeof(timecode_flags)/sizeof(timecode_flags[0]); i++)
    {
    if(lqt & timecode_flags[i].lqt)
      ret |= timecode_flags[i].gavl;
    }
  return ret;
  }

static uint32_t timecode_flags_gavl_2_lqt(int gavl)
  {
  int i;
  uint32_t ret = 0;
  for(i = 0; i < sizeof(timecode_flags)/sizeof(timecode_flags[0]); i++)
    {
    if(gavl & timecode_flags[i].gavl)
      ret |= timecode_flags[i].lqt;
    }
  return ret;
  }

/* Audio encoding */

void lqt_gavl_set_audio_codec(quicktime_t * file,
                              int track,
                              gavl_audio_format_t * format,
                              lqt_codec_info_t * codec)
  {
  int i;
  const lqt_channel_t * chans_1;
  lqt_channel_t * chans_2;

  lqt_set_audio_codec(file, track, codec);
  
  format->sample_format =
    sampleformat_lqt_2_gavl(lqt_get_sample_format(file, track));
  format->interleave_mode = GAVL_INTERLEAVE_ALL;

  /* Negotiate the channel setup */

  /* 1st try: The codec already knows (we cannot change that) */
  chans_1 = lqt_get_channel_setup(file, track);
  if(chans_1)
    {
    for(i = 0; i < format->num_channels; i++)
      format->channel_locations[i] = channel_lqt_2_gavl(chans_1[i]);
    }
  else
    {
    /* Set our channel setup */
    chans_2 = calloc(format->num_channels, sizeof(*chans_2));
    for(i = 0; i < format->num_channels; i++)
      chans_2[i] = channel_gavl_2_lqt(format->channel_locations[i]);
    lqt_set_channel_setup(file, track, chans_2);
    free(chans_2);

    /* Copy reordered setup back */
    chans_1 = lqt_get_channel_setup(file, track);
    for(i = 0; i < format->num_channels; i++)
      format->channel_locations[i] = channel_lqt_2_gavl(chans_1[i]);
    }
  }


void lqt_gavl_add_audio_track(quicktime_t * file,
                               gavl_audio_format_t * format,
                               lqt_codec_info_t * codec)
  {
  
  int track = quicktime_audio_tracks(file);
  
  lqt_add_audio_track(file, format->num_channels, format->samplerate,
                      16, NULL);

  if(codec)
    lqt_gavl_set_audio_codec(file, track, format, codec);
  }

void lqt_gavl_set_video_codec(quicktime_t * file,
                              int track,
                              gavl_video_format_t * format,
                              lqt_codec_info_t * codec)
  {
  lqt_set_video_codec(file, track, codec);
  
#if LQT_BUILD < LQT_MAKE_BUILD(1,1,2)
  format->pixelformat = pixelformat_lqt_2_gavl(lqt_get_cmodel(file, track));
#else
  if(codec->num_encoding_colormodels)
    {
    gavl_pixelformat_t * pixelformats;
    int i;
    
    pixelformats = malloc((codec->num_encoding_colormodels+1) *
                          sizeof(*pixelformats));

    for(i = 0; i < codec->num_encoding_colormodels; i++)
      {
      pixelformats[i] =
        pixelformat_lqt_2_gavl(codec->encoding_colormodels[i]);
      }
    pixelformats[codec->num_encoding_colormodels] = GAVL_PIXELFORMAT_NONE;
    format->pixelformat =
      gavl_pixelformat_get_best(format->pixelformat, pixelformats, NULL);

    lqt_set_cmodel(file, track, pixelformat_gavl_2_lqt(format->pixelformat));
    free(pixelformats);
    }
  else
    format->pixelformat = pixelformat_lqt_2_gavl(lqt_get_cmodel(file, track));
#endif
  }

void lqt_gavl_add_video_track(quicktime_t * file,
                              gavl_video_format_t * format,
                              lqt_codec_info_t * codec)
  {
  int track = quicktime_video_tracks(file);
  
  if(lqt_add_video_track(file, format->image_width, format->image_height,
                          format->frame_duration, format->timescale,
                          NULL))
    return;
  lqt_set_pixel_aspect(file, track, format->pixel_width, format->pixel_height);
  lqt_set_interlace_mode(file, track,
                         interlace_mode_gavl_2_lqt(format->interlace_mode));

  /* Add timecode track */
  if(format->timecode_format.int_framerate > 0)
    {
    lqt_add_timecode_track(file, track,
                           timecode_flags_gavl_2_lqt(format->timecode_format.flags),
                           format->timecode_format.int_framerate);
    }

  if(codec)
    lqt_gavl_set_video_codec(file, track, format, codec);
  
  }

int lqt_gavl_encode_video(quicktime_t * file, int track,
                           gavl_video_frame_t * frame, uint8_t ** rows)
  {
  int i, height;
  int result;
  int tc_framerate;
  uint32_t tc_flags;
    
  /* Write timecode */

  if(lqt_has_timecode_track(file, track, &tc_flags, &tc_framerate) &&
     (frame->timecode != GAVL_TIMECODE_UNDEFINED))
    {
    gavl_timecode_format_t f;
    f.int_framerate = tc_framerate;
    f.flags         = timecode_flags_lqt_2_gavl(tc_flags);
    
    lqt_write_timecode(file, track,
                       gavl_timecode_to_framecount(&f, frame->timecode));
    
    }
  
  if(lqt_colormodel_is_planar(lqt_get_cmodel(file, track)))
    {
    lqt_set_row_span(file, track, frame->strides[0]);
    lqt_set_row_span_uv(file, track, frame->strides[1]);
#if LQT_BUILD >= LQT_MAKE_BUILD(1,1,2)
    if(frame->duration > 0)
      result = lqt_encode_video_d(file, frame->planes, track, frame->timestamp,
                                  frame->duration);
    else
#endif
      result = lqt_encode_video(file, frame->planes, track, frame->timestamp);

    }
  else
    {
    height = quicktime_video_height(file, track);
    for(i = 0; i < height; i++)
      {
      lqt_set_row_span(file, track, frame->strides[0]);
      rows[i] = frame->planes[0] + i * frame->strides[0];
      }
#if LQT_BUILD >= LQT_MAKE_BUILD(1,1,2)
    if(frame->duration > 0)
      result = lqt_encode_video_d(file, rows, track, frame->timestamp, frame->duration);
    else
#endif
      result = lqt_encode_video(file, rows, track, frame->timestamp);
    }
  return result;
  }

void lqt_gavl_encode_audio(quicktime_t * file, int track,
                           gavl_audio_frame_t * frame)
  {
  lqt_encode_audio_raw(file, frame->samples.s_8, frame->valid_samples,
                       track);
  }

/* Decoding */

int lqt_gavl_get_audio_format(quicktime_t * file,
                              int track,
                              gavl_audio_format_t * format)
  {
  int i;
  const lqt_channel_t * channel_setup;

  if(track >= quicktime_audio_tracks(file) ||
     track < 0)
    return 0;
  
  format->num_channels = quicktime_track_channels(file, track);
  format->samplerate = quicktime_sample_rate(file, track);
  format->sample_format =
    sampleformat_lqt_2_gavl(lqt_get_sample_format(file, track));
  format->interleave_mode = GAVL_INTERLEAVE_ALL;

  format->samples_per_frame = 1024; // Meaningless but better than 0
  channel_setup = lqt_get_channel_setup(file, track);

  if(channel_setup)
    {
    for(i = 0; i < format->num_channels; i++)
      {
      format->channel_locations[i] = channel_lqt_2_gavl(channel_setup[i]);
      }
    }
  else
    gavl_set_channel_setup(format);
  return 1;
  }

int lqt_gavl_get_video_format(quicktime_t * file,
                              int track,
                              gavl_video_format_t * format, int encode)
  {
  int constant_framerate;
  int tc_framerate;
  uint32_t tc_flags;
  if(track >= quicktime_video_tracks(file) ||
     track < 0)
    return 0;

  format->image_width = quicktime_video_width(file, track);
  format->image_height = quicktime_video_height(file, track);

  format->frame_width = format->image_width;
  format->frame_height = format->image_height;

  lqt_get_pixel_aspect(file, track, &format->pixel_width,
                       &format->pixel_height);

  format->timescale = lqt_video_time_scale(file, track);
  format->frame_duration = lqt_frame_duration(file, track,
                                              &constant_framerate);

  if(lqt_has_timecode_track(file, track, &tc_flags, &tc_framerate))
    {
    format->timecode_format.int_framerate = tc_framerate;
    format->timecode_format.flags = timecode_flags_lqt_2_gavl(tc_flags);
    }
  
  if(encode)
    {
    if((lqt_get_file_type(file) & (LQT_FILE_AVI|LQT_FILE_AVI_ODML)))
      format->framerate_mode = GAVL_FRAMERATE_CONSTANT;
    }
  else
    {
    if(!constant_framerate)
      format->framerate_mode = GAVL_FRAMERATE_VARIABLE;
    else
      format->framerate_mode = GAVL_FRAMERATE_CONSTANT;
    }
  
  format->chroma_placement =
    chroma_placement_lqt_2_gavl(lqt_get_chroma_placement(file, track));
  
  format->interlace_mode =
    interlace_mode_lqt_2_gavl(lqt_get_interlace_mode(file, track));

  format->pixelformat = pixelformat_lqt_2_gavl(lqt_get_cmodel(file, track));
  
  return 1;
  }

int lqt_gavl_decode_video(quicktime_t * file, int track,
                          gavl_video_frame_t * frame, uint8_t ** rows)
  {
  int i, height;
  uint32_t tc;
  uint32_t tc_flags;
  int      tc_framerate;
  
  if(quicktime_video_position(file, track) >=
     quicktime_video_length(file, track))
    return 0;
  
  frame->timestamp = lqt_frame_time(file, track);

  if(lqt_has_timecode_track(file, track, &tc_flags,
                            &tc_framerate) &&
     lqt_read_timecode(file, track, &tc))
    {
    gavl_timecode_format_t f;
    f.int_framerate = tc_framerate;
    f.flags = timecode_flags_lqt_2_gavl(tc_flags);
    frame->timecode = gavl_timecode_from_framecount(&f, tc);
    }
  else
    frame->timecode = GAVL_TIMECODE_UNDEFINED;
  
  if(lqt_colormodel_is_planar(lqt_get_cmodel(file, track)))
    {
    lqt_set_row_span(file, track, frame->strides[0]);
    lqt_set_row_span_uv(file, track, frame->strides[1]);
    lqt_decode_video(file, frame->planes, track);
    }
  else
    {
    height = quicktime_video_height(file, track);
    for(i = 0; i < height; i++)
      {
      lqt_set_row_span(file, track, frame->strides[0]);
      rows[i] = frame->planes[0] + i * frame->strides[0];
      }
    lqt_decode_video(file, rows, track);
    }
  return 1;
  }

int lqt_gavl_decode_audio(quicktime_t * file, int track,
                          gavl_audio_frame_t * frame,
                          int samples)
  {
  frame->timestamp = quicktime_audio_position(file, track);
  lqt_decode_audio_raw(file, frame->samples.s_8, samples, track);
  frame->valid_samples = lqt_last_audio_position(file, track) - frame->timestamp;
  return frame->valid_samples;
  }

void lqt_gavl_seek(quicktime_t * file, gavl_time_t * time)
  {
  lqt_gavl_seek_scaled(file, time, GAVL_TIME_SCALE);
  }

void lqt_gavl_seek_scaled(quicktime_t * file, gavl_time_t * time, int scale)
  {
  int imax;
  int i;
  int64_t video_time_save  = -1;
  int video_timescale_save = -1;

  int64_t time_scaled;
  int timescale;

  /* We synchronize to the first video track */

  imax = quicktime_video_tracks(file);

  for(i = 0; i < imax; i++)
    {
    timescale = lqt_video_time_scale(file, i);
    time_scaled = gavl_time_rescale(scale, timescale, *time);
    lqt_seek_video(file, i, time_scaled);
    if(!i)
      {
      video_timescale_save = timescale;
      video_time_save      = time_scaled;
      }
    }

  if(video_time_save >= 0)
    *time = gavl_time_unscale(video_timescale_save, video_time_save);

  imax = quicktime_audio_tracks(file);
  for(i = 0; i < imax; i++)
    {
    timescale = quicktime_sample_rate(file, i);
    time_scaled = gavl_time_rescale(scale, timescale, *time);
    quicktime_set_audio_position(file, time_scaled, i);
    }

  imax = lqt_text_tracks(file);
  for(i = 0; i < imax; i++)
    {
    if(lqt_is_chapter_track(file, i))
      continue;
    
    timescale = lqt_text_time_scale(file, i);
    time_scaled = gavl_time_rescale(scale, timescale, *time);
    lqt_set_text_time(file, i, time_scaled);
    }
  }


uint8_t ** lqt_gavl_rows_create(quicktime_t * file, int track)
  {
  uint8_t ** ret;

  if(lqt_colormodel_is_planar(lqt_get_cmodel(file, track)))
    return (uint8_t**)0;
  
  ret = malloc(sizeof(*ret) * quicktime_video_height(file, track));
  return ret;
  }


void lqt_gavl_rows_destroy(uint8_t** rows)
  {
  if(rows)
    free(rows);
  }

gavl_time_t lqt_gavl_duration(quicktime_t * file)
  {
  gavl_time_t ret = 0, time;
  int i, imax;
  int timescale;
  int64_t time_scaled;
  
  imax = quicktime_audio_tracks(file);
  for(i = 0; i < imax; i++)
    {
    timescale = quicktime_sample_rate(file, i);
    time_scaled = quicktime_audio_length(file, i);

    time = gavl_time_unscale(timescale, time_scaled);
    if(ret < time)
      ret = time;
    }

  imax = quicktime_video_tracks(file);
  for(i = 0; i < imax; i++)
    {
    timescale = lqt_video_time_scale(file, i);
    time_scaled = lqt_video_duration(file, i);
    
    time = gavl_time_unscale(timescale, time_scaled);
    if(ret < time)
      ret = time;
    }
  return ret;
  }

/* Compression support */

static int compression_info_gavl_2_lqt(const gavl_compression_info_t * gci,
                                       const gavl_audio_format_t * afmt,
                                       const gavl_video_format_t * vfmt,
                                       lqt_compression_info_t * lci)
  {
  memset(lci, 0, sizeof(*lci));

  /* Field pictures are not supported */
  if(gci->flags & GAVL_COMPRESSION_HAS_FIELD_PICTURES)
    return 0;
  
  switch(gci->id)
    {
    case GAVL_CODEC_ID_NONE: //!< Unknown/unsupported compression format
      break;
    /* Audio */
    case GAVL_CODEC_ID_ALAW: //!< alaw 2:1
      lci->id = LQT_COMPRESSION_ALAW;
      break;
    case GAVL_CODEC_ID_ULAW:    //!< mu-law 2:1
      lci->id = LQT_COMPRESSION_ULAW;
      break;
    case GAVL_CODEC_ID_MP2:       //!< MPEG-1 audio layer II
      lci->id = LQT_COMPRESSION_MP2;
      break;
    case GAVL_CODEC_ID_MP3:       //!< MPEG-1/2 audio layer 3 CBR/VBR
      lci->id = LQT_COMPRESSION_MP3;
      break;
    case GAVL_CODEC_ID_AC3:       //!< AC3
      lci->id = LQT_COMPRESSION_AC3;
      break;
    case GAVL_CODEC_ID_AAC:      //!< AAC as stored in quicktime/mp4
      lci->id = LQT_COMPRESSION_AAC;
      break;
    case GAVL_CODEC_ID_VORBIS:    //!< Vorbis (segmented extradata and packets)
      break;
    
    /* Video */
    case GAVL_CODEC_ID_JPEG:      //!< JPEG image
      lci->id = LQT_COMPRESSION_JPEG;
      break;
    case GAVL_CODEC_ID_PNG:       //!< PNG image
      lci->id = LQT_COMPRESSION_PNG;
      break;
    case GAVL_CODEC_ID_TIFF:      //!< TIFF image
      lci->id = LQT_COMPRESSION_TIFF;
      break;
    case GAVL_CODEC_ID_TGA:       //!< TGA image
      lci->id = LQT_COMPRESSION_TGA;
      break;
    case GAVL_CODEC_ID_DV:       //!< DV
      lci->id = LQT_COMPRESSION_DV;
      break;
    case GAVL_CODEC_ID_MPEG1:     //!< MPEG-1 video
      break;
    case GAVL_CODEC_ID_MPEG2:     //!< MPEG-2 video
      /* Check for D10 */

      if(!(gci->flags & (GAVL_COMPRESSION_HAS_P_FRAMES|GAVL_COMPRESSION_HAS_B_FRAMES)) &&
         ((gci->bitrate == 30000000) ||
          (gci->bitrate == 40000000) ||
          (gci->bitrate == 50000000)) &&
         vfmt &&
         (vfmt->pixelformat == GAVL_YUV_422_P) &&
         (vfmt->image_width == 720) &&
         ((vfmt->image_height == 576) ||
          (vfmt->image_height == 608) ||
          (vfmt->image_height == 486) ||
          (vfmt->image_height == 512)))
        {
        lci->id = LQT_COMPRESSION_D10;
        }
      
      break;
    case GAVL_CODEC_ID_MPEG4_ASP: //!< MPEG-4 ASP (a.k.a. Divx4)
      lci->id = LQT_COMPRESSION_MPEG4_ASP;
      break;
    case GAVL_CODEC_ID_H264:      //!< H.264 (Annex B)
      lci->id = LQT_COMPRESSION_H264;
      break;
    case GAVL_CODEC_ID_THEORA:    //!< Theora (segmented extradata
      break;
    case GAVL_CODEC_ID_DIRAC:     //!< Complete DIRAC frames, sequence end code 
      lci->id = LQT_COMPRESSION_DIRAC;
      break;
    }

  if(lci->id == LQT_COMPRESSION_NONE)
    return 0;

  /* Set audio/video specific stuff */
  if(lci->id < 0x10000)
    {
    if(!afmt)
      return 0;

    lci->samplerate = afmt->samplerate;
    lci->num_channels = afmt->num_channels;
    
    }
  else
    {
    if(!vfmt)
      return 0;
    lci->width           = vfmt->image_width;
    lci->height          = vfmt->image_height;
    lci->pixel_width     = vfmt->pixel_width;
    lci->pixel_height    = vfmt->pixel_height;
    lci->colormodel      = pixelformat_gavl_2_lqt(vfmt->pixelformat);
    lci->video_timescale = vfmt->timescale;
    }
  /* Set generic stuff */
  lci->bitrate = gci->bitrate;

  if(gci->flags & GAVL_COMPRESSION_HAS_P_FRAMES)
    lci->flags |= LQT_COMPRESSION_HAS_P_FRAMES;
  if(gci->flags & GAVL_COMPRESSION_HAS_B_FRAMES)
    lci->flags |= LQT_COMPRESSION_HAS_B_FRAMES;
  if(gci->flags & GAVL_COMPRESSION_SBR)
    lci->flags |= LQT_COMPRESSION_SBR;
  
  lci->global_header = gci->global_header;
  lci->global_header_len = gci->global_header_len;
  
  return 1;
  }

static int compression_info_lqt_2_gavl(const lqt_compression_info_t * lci,
                                       gavl_compression_info_t * gci)
  {
  switch(lci->id)
    {
    case LQT_COMPRESSION_ALAW:
      gci->id = GAVL_CODEC_ID_ALAW;
      break;
    case LQT_COMPRESSION_ULAW:
      gci->id = GAVL_CODEC_ID_ULAW;
      break;
    case LQT_COMPRESSION_MP2:
      gci->id = GAVL_CODEC_ID_MP2;
      break;
    case LQT_COMPRESSION_MP3:
      gci->id = GAVL_CODEC_ID_MP3;
      break;
    case LQT_COMPRESSION_AC3:
      gci->id = GAVL_CODEC_ID_AC3;
      break;
    case LQT_COMPRESSION_AAC:
      gci->id = GAVL_CODEC_ID_AAC;
      break;

    /* Video */
    case LQT_COMPRESSION_JPEG: //!< JPEG image
      gci->id = GAVL_CODEC_ID_JPEG;
      break;
    case LQT_COMPRESSION_PNG:            //!< PNG image
      gci->id = GAVL_CODEC_ID_PNG;
      break;
    case LQT_COMPRESSION_TIFF:           //!< TIFF image
      gci->id = GAVL_CODEC_ID_TIFF;
      break;
    case LQT_COMPRESSION_TGA:            //!< TGA image
      gci->id = GAVL_CODEC_ID_TGA;
      break;
    case LQT_COMPRESSION_MPEG4_ASP:      //!< MPEG-4 ASP (a.k.a. Divx4)
      gci->id = GAVL_CODEC_ID_MPEG4_ASP;
      break;
    case LQT_COMPRESSION_H264:           //!< H.264 (Annex B)
      gci->id = GAVL_CODEC_ID_H264;
      break;
    case LQT_COMPRESSION_DIRAC:          //!< Complete DIRAC frames
      gci->id = GAVL_CODEC_ID_DIRAC;
      break;
    case LQT_COMPRESSION_D10:            //!< D10 according to SMPTE 356M-2001
      gci->id = GAVL_CODEC_ID_MPEG2;
      break;
    case LQT_COMPRESSION_NONE:
      break;
    }

  if(gci->id == GAVL_CODEC_ID_NONE)
    return 0;
  
  gci->bitrate = lci->bitrate;
  gci->global_header_len = lci->global_header_len;

  if(gci->global_header_len)
    {
    gci->global_header = malloc(gci->global_header_len);
    memcpy(gci->global_header, lci->global_header, gci->global_header_len);
    }
  
  if(lci->flags & LQT_COMPRESSION_HAS_P_FRAMES)
    gci->flags |= GAVL_COMPRESSION_HAS_P_FRAMES;
  if(lci->flags & LQT_COMPRESSION_HAS_B_FRAMES)
    gci->flags |= GAVL_COMPRESSION_HAS_B_FRAMES;
  if(lci->flags & LQT_COMPRESSION_SBR)
    gci->flags |= GAVL_COMPRESSION_SBR;
  
  return 1;
  }
  
int lqt_gavl_get_audio_compression_info(quicktime_t * file, int track,
                                        gavl_compression_info_t * ci)
  {
  const lqt_compression_info_t * lci;
  lci = lqt_get_audio_compression_info(file, track);
  if(lci)
    {
    return compression_info_lqt_2_gavl(lci, ci);
    }
  return 0;
  }


int lqt_gavl_get_video_compression_info(quicktime_t * file, int track,
                                        gavl_compression_info_t * ci)
  {
  const lqt_compression_info_t * lci;
  lci = lqt_get_video_compression_info(file, track);
  if(lci)
    {
    return compression_info_lqt_2_gavl(lci, ci);
    }
  return 0;
  }

static void packet_lqt_2_gavl(lqt_packet_t * lp, gavl_packet_t * gp)
  {
  gavl_packet_alloc(gp, lp->data_len);
  memcpy(gp->data, lp->data, lp->data_len);
  gp->data_len = lp->data_len;
  
  gp->pts = lp->timestamp;
  gp->duration = lp->duration;
  gp->header_size = lp->header_size;

  if(lp->flags & LQT_PACKET_KEYFRAME)
    gp->flags |= GAVL_PACKET_KEYFRAME;

  }

int lqt_gavl_read_audio_packet(quicktime_t * file, int track, gavl_packet_t * p)
  {
  lqt_packet_t lp;
  memset(&lp, 0, sizeof(lp));
  if(!lqt_read_audio_packet(file, &lp, track))
    return 0;
  packet_lqt_2_gavl(&lp, p);
  return 1;
  }

int lqt_gavl_read_video_packet(quicktime_t * file, int track, gavl_packet_t * p)
  {
  lqt_packet_t lp;
  memset(&lp, 0, sizeof(lp));
  if(!lqt_read_video_packet(file, &lp, track))
    return 0;
  packet_lqt_2_gavl(&lp, p);
  return 1;
  }

static lqt_codec_info_t * find_encoder(lqt_codec_info_t ** infos,
                                       lqt_compression_id_t id)
  {
  int i = 0;

  while(infos[i])
    {
    if(infos[i]->compression_id == id)
      return infos[i];
    i++;
    }
  return NULL;
  }


int lqt_gavl_writes_compressed_audio(lqt_file_type_t type,
                                     const gavl_audio_format_t * format,
                                     const gavl_compression_info_t * ci)
  {
  int ret = 0;
  lqt_compression_info_t lci;
  lqt_codec_info_t ** infos;
  lqt_codec_info_t * info;
  
  if(!compression_info_gavl_2_lqt(ci, format, NULL, &lci))
    return 0;

  infos = lqt_query_registry(1, 0, 1, 0);
  info = find_encoder(infos, lci.id);

  if(info)
    ret = lqt_writes_compressed(type, &lci, info);
  
  lqt_destroy_codec_info(infos);

  return ret;
  }

int lqt_gavl_writes_compressed_video(lqt_file_type_t type,
                                     const gavl_video_format_t * format,
                                     const gavl_compression_info_t * ci)
  {
  int ret = 0;
  lqt_compression_info_t lci;
  lqt_codec_info_t ** infos;
  lqt_codec_info_t * info;
  
  if(!compression_info_gavl_2_lqt(ci, NULL, format, &lci))
    return 0;
  
  infos = lqt_query_registry(0, 1, 1, 0);
  info = find_encoder(infos, lci.id);

  if(info)
    ret = lqt_writes_compressed(type, &lci, info);
  
  lqt_destroy_codec_info(infos);

  return ret;
  }

int lqt_gavl_add_audio_track_compressed(quicktime_t * file,
                                        const gavl_audio_format_t * format,
                                        const gavl_compression_info_t * ci)
  {
  lqt_codec_info_t ** infos;
  lqt_codec_info_t * info;
  lqt_compression_info_t lci;

  compression_info_gavl_2_lqt(ci, format, NULL, &lci);
  infos = lqt_query_registry(1, 0, 1, 0);
  info = find_encoder(infos, lci.id);
  lqt_add_audio_track_compressed(file, &lci, info);
  lqt_destroy_codec_info(infos);
  
  return 1;
  }


int lqt_gavl_add_video_track_compressed(quicktime_t * file,
                                        const gavl_video_format_t * format,
                                        const gavl_compression_info_t * ci)
  {
  lqt_codec_info_t ** infos;
  lqt_codec_info_t * info;
  lqt_compression_info_t lci;

  compression_info_gavl_2_lqt(ci, NULL, format, &lci);
  infos = lqt_query_registry(0, 1, 1, 0);
  info = find_encoder(infos, lci.id);
  lqt_add_video_track_compressed(file, &lci, info);
  lqt_destroy_codec_info(infos);
  
  return 1;
  }

static void packet_gavl_2_lqt(gavl_packet_t * gp, lqt_packet_t * lp)
  {
  memset(lp, 0, sizeof(*lp));
  lp->data_len = gp->data_len;
  lp->data = gp->data;
  lp->timestamp = gp->pts;
  lp->duration = gp->duration;
  lp->header_size = gp->header_size;

  if(gp->flags & GAVL_PACKET_KEYFRAME)
    lp->flags |= LQT_PACKET_KEYFRAME;
  }

int lqt_gavl_write_audio_packet(quicktime_t * file, int track, gavl_packet_t * p)
  {
  lqt_packet_t lp;
  packet_gavl_2_lqt(p, &lp);
  return lqt_write_audio_packet(file, &lp, track);
  }

int lqt_gavl_write_video_packet(quicktime_t * file, int track, gavl_packet_t * p)
  {
  lqt_packet_t lp;
  packet_gavl_2_lqt(p, &lp);
  return lqt_write_video_packet(file, &lp, track);
  }
