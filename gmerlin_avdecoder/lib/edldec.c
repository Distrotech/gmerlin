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
#include <codecs.h>

typedef struct
  {
  /* One decoder for each segment */
  struct
    {
    bgav_t * dec;

    int dec_index; /* If -1 we own the decoder */
    int is_open;
    } * decoders;
  
  const bgav_edl_t * e;
  const bgav_edl_stream_t * es;
  
  } stream_priv_t;

/* Called by codec init */
static int open_decoders(bgav_stream_t * s)
  {
  bgav_options_t * opt;
  stream_priv_t * sp;
  int i;
  const char * url;
  
  sp = (stream_priv_t *)s->priv;
  for(i = 0; i < sp->es->num_segments; i++)
    {
    if(sp->decoders[i].dec_index < 0)
      {
      /* Create instance */
      sp->decoders[i].dec = bgav_create();
      opt = bgav_get_options(sp->decoders[i].dec);
      bgav_options_copy(opt, s->opt);
      
      if(sp->es->segments[i].url)
        url = sp->es->segments[i].url;
      else
        url = sp->e->url;
      if(!url)
        return 0;
            
      if(!bgav_open(sp->decoders[i].dec, url))
        return 0;

      if(!bgav_select_track(sp->decoders[i].dec,
                            sp->es->segments[i].track))
        return 0;

      switch(s->type)
        {
        case BGAV_STREAM_AUDIO:
          bgav_set_audio_stream(sp->decoders[i].dec,
                                sp->es->segments[i].stream,
                                BGAV_STREAM_DECODE);
          break;
        case BGAV_STREAM_VIDEO:
          bgav_set_video_stream(sp->decoders[i].dec,
                                sp->es->segments[i].stream,
                                BGAV_STREAM_DECODE);
          break;
        case BGAV_STREAM_SUBTITLE_TEXT:
        case BGAV_STREAM_SUBTITLE_OVERLAY:
          bgav_set_subtitle_stream(sp->decoders[i].dec,
                                   sp->es->segments[i].stream,
                                   BGAV_STREAM_DECODE);
          break;
        case BGAV_STREAM_UNKNOWN:
          break;
        }
      if(!bgav_start(sp->decoders[i].dec))
        return 0;
      }
    else
      sp->decoders[i].dec =
        sp->decoders[sp->decoders[i].dec_index].dec;
    }
  return 1;
  }

static void close_decoders(bgav_stream_t * s)
  {
  stream_priv_t * sp;
  int i;
  sp = (stream_priv_t *)s->priv;
  for(i = 0; i < sp->es->num_segments; i++)
    {
    if(sp->decoders[i].dec_index < 0)
      {
      bgav_close(sp->decoders[0].dec);
      sp->decoders[0].dec = (bgav_t*)0;
      }
    }
  }


typedef struct
  {
  gavl_audio_converter_t * cnv;
  gavl_audio_frame_t * frame;
  int current_segment;
  } edl_audio_priv_t;

static int init_audio_edl(bgav_stream_t * s)
  {
  const gavl_audio_format_t * format;
  edl_audio_priv_t * priv;
  stream_priv_t * sp;

  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;

  if(!open_decoders(s))
    return 0;

  sp = (stream_priv_t*)s->priv;
  
  format =
    bgav_get_audio_format(sp->decoders[0].dec,
                          sp->es->segments[0].stream);

  gavl_audio_format_copy(&s->data.audio.format, format);
  
  return 1;
  }

static void close_audio_edl(bgav_stream_t * s)
  {
  edl_audio_priv_t * priv;
  priv = (edl_audio_priv_t*)s->data.audio.decoder->priv;
  close_decoders(s);
  return;
  }

static int decode_audio_internal(bgav_stream_t * s,
                                 gavl_audio_frame_t * frame,
                                 int num_samples)
  {
  return 0;
  }
                                 

static int decode_audio_edl(bgav_stream_t * s, gavl_audio_frame_t * frame,
                            int num_samples)
  {
  int samples_decoded = 0;
  stream_priv_t * sp;
  edl_audio_priv_t * priv;
  int samples_to_read;
  const bgav_edl_segment_t * seg;
  gavl_audio_format_t tmp_format;
  int64_t out_time;
  priv = (edl_audio_priv_t*)s->data.audio.decoder->priv;

  sp = (stream_priv_t*)(s->priv);

  /* Mute the entire frame so we handle "holes" correctly */
  gavl_audio_format_copy(&tmp_format, &s->data.audio.format);
  tmp_format.samples_per_frame = num_samples;
  gavl_audio_frame_mute(frame, &tmp_format);

  out_time = s->out_time;
  
  while(out_time < s->out_time + num_samples)
    {
    samples_to_read = num_samples - samples_decoded;
    seg = &sp->es->segments[priv->current_segment];

    /* Silence */
    if(seg->dst_time > s->out_time)
      {
      /* Check where silence ends */
      if(out_time + samples_to_read > seg->dst_time)
        samples_to_read = seg->dst_time - out_time;

      frame->valid_samples += samples_to_read;
      out_time += samples_to_read;
      }
    else
      {
      if(out_time + samples_to_read > seg->dst_time + seg->dst_duration)
        samples_to_read = seg->dst_time + seg->dst_duration - out_time;

      
      }
    }
  
  return 0;
  }

static bgav_audio_decoder_t audio_decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('_','e','d','l'),
                             0x00 },
    .name   = "EDL meta decoder (audio)",
    .init   = init_audio_edl,
    .close  = close_audio_edl,
    //    .resync = resync_audio_edl,
    .decode = decode_audio_edl,
    //    .parse  = parse_audio_edl,
  };

void bgav_init_audio_decoders_edl()
  {
  bgav_audio_decoder_register(&audio_decoder);
  }

typedef struct
  {
  gavl_video_converter_t * cnv;
  gavl_video_frame_t * frame;

} edl_video_priv_t;

static int init_video_edl(bgav_stream_t * s)
  {
  const gavl_video_format_t * format;
  stream_priv_t * sp;
  edl_video_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;
  
  if(!open_decoders(s))
    return 0;

  sp = (stream_priv_t*)s->priv;
  
  format =
    bgav_get_video_format(sp->decoders[0].dec,
                          sp->es->segments[0].stream);

  gavl_video_format_copy(&s->data.video.format, format);
  
  return 1;
  }

static void close_video_edl(bgav_stream_t * s)
  {
  edl_video_priv_t * priv;
  priv = (edl_video_priv_t*)s->data.video.decoder->priv;

  close_decoders(s);
  return;
  }

static int decode_video_edl(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  edl_video_priv_t * priv;
  priv = (edl_video_priv_t*)s->data.audio.decoder->priv;
  return 0;
  }

static bgav_video_decoder_t video_decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('_','e','d','l'),
                             0x00 },
    .name   = "EDL meta decoder (video)",
    .init   = init_video_edl,
    .close  = close_video_edl,
    .decode = decode_video_edl,
  };

void bgav_init_video_decoders_edl()
  {
  bgav_video_decoder_register(&video_decoder);
  }

typedef struct
  {
  int dummy;
  } edl_subtitle_priv;

/* EDL "decoder" */

struct bgav_edl_dec_s
  {
  const bgav_edl_t * edl;
  
  };

static int match_segments(const bgav_edl_segment_t * s1,
                          const bgav_edl_segment_t * s2)
  {
  if(s1->url && s2->url && strcmp(s1->url, s2->url))
    return 0;

  if((s1->url && !s2->url) || (s2->url && !s1->url))
    return 0;

  if((s1->track != s2->track) || (s1->stream != s2->stream))
    return 0;
  
  return 1;
  }

static void init_stream_common(bgav_stream_t * s,
                               const bgav_edl_stream_t * es,
                               const bgav_edl_t * edl)
  {
  int i, j;
  stream_priv_t * sp;
  s->fourcc = BGAV_MK_FOURCC('_', 'e', 'd', 'l');
  
  sp = calloc(1, sizeof(*sp));
  s->priv = sp;
  sp->e = edl;
  sp->es = es;

  sp->decoders = calloc(sp->es->num_segments, sizeof(*sp->decoders));
  
  /* First decoder is always created */
  sp->decoders[0].dec_index = -1;
  
  for(i = 1; i < es->num_segments; i++)
    {
    /* Look if we already have a decoder for that stream */
    sp->decoders[i].dec_index = -1;
    
    for(j = 0; j < i; j++)
      {
      if(match_segments(&es->segments[j], &es->segments[i]))
        {
        sp->decoders[i].dec_index = j;
        break;
        }
      }
    }
  }

int bgav_open_edl(bgav_t * bgav, const bgav_edl_t * edl)
  {
  int i, j;
  bgav_stream_t * s;

  bgav->edl_dec = calloc(1, sizeof(*bgav->edl_dec));
  bgav->edl_dec->edl = edl;
  bgav->tt = bgav_track_table_create(edl->num_tracks);
  
  for(i = 0; i < edl->num_tracks; i++)
    {
    for(j = 0; j < edl->tracks[i].num_audio_streams; j++)
      {
      if(!edl->tracks[i].audio_streams[j].num_segments)
        continue;
      s = bgav_track_add_audio_stream(&bgav->tt->tracks[i], &bgav->opt);
      init_stream_common(s, &edl->tracks[i].audio_streams[j], edl);
      }
    for(j = 0; j < edl->tracks[i].num_video_streams; j++)
      {
      if(!edl->tracks[i].video_streams[j].num_segments)
        continue;
      s = bgav_track_add_video_stream(&bgav->tt->tracks[i], &bgav->opt);
      init_stream_common(s, &edl->tracks[i].video_streams[j], edl);
      }
    for(j = 0; j < edl->tracks[i].num_subtitle_text_streams; j++)
      {
      if(!edl->tracks[i].subtitle_text_streams[j].num_segments)
        continue;
      s =
        bgav_track_add_subtitle_stream(&bgav->tt->tracks[i], &bgav->opt,
                                       1, "UTF-8");
      init_stream_common(s, &edl->tracks[i].subtitle_text_streams[j], edl);
      }
    for(j = 0; j < edl->tracks[i].num_subtitle_overlay_streams; j++)
      {
      if(!edl->tracks[i].subtitle_overlay_streams[j].num_segments)
        continue;
      s =
        bgav_track_add_subtitle_stream(&bgav->tt->tracks[i], &bgav->opt,
                                       0, (const char*)0);
      init_stream_common(s, &edl->tracks[i].subtitle_overlay_streams[j], edl);
      }
    }
  
  return 0;
  }

