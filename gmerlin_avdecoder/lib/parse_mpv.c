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
#include <stdlib.h>


#include <avdec_private.h>
#include <parser.h>
#include <videoparser_priv.h>

#include <mpv_header.h>

#define LOG_DOMAIN "parse_mpv"

/* MPEG-1/2 */

#define MPEG_NEED_SYNC                        0
#define MPEG_NEED_STARTCODE                   1
#define MPEG_HAS_PICTURE_CODE                 2
#define MPEG_HAS_PICTURE_HEADER               3
#define MPEG_HAS_PICTURE_EXT_CODE             4
#define MPEG_HAS_PICTURE_EXT_HEADER           5
#define MPEG_HAS_GOP_CODE                     6
#define MPEG_HAS_SEQUENCE_CODE                7
#define MPEG_HAS_SEQUENCE_EXT_CODE            8
#define MPEG_HAS_SEQUENCE_DISPLAY_EXT_CODE    9

typedef struct
  {
  /* Sequence header */
  bgav_mpv_sequence_header_t sh;
  int have_sh;
  int has_picture_start;
  int state;

  /* Frames since sequence header (for intra slice refresh) */
  int frames_since_sh;
  
  /* Framecounts for I and P/B frames, reset at sequence end
     (for MPEG still images) */
  int pb_count;
  int i_count;

  int d10; // Special handling for D10 */
  
  int framerate_from_container;
  
  } mpeg12_priv_t;

static void reset_mpeg12(bgav_video_parser_t * parser)
  {
  mpeg12_priv_t * priv = parser->priv;
  priv->state = MPEG_NEED_SYNC;
  priv->has_picture_start = 0;
  }

static int parse_mpeg12(bgav_video_parser_t * parser)
  {
  const uint8_t * sc;
  int len;
  mpeg12_priv_t * priv = parser->priv;
  //  cache_t * c;
  bgav_mpv_picture_extension_t pe;
  bgav_mpv_picture_header_t    ph;
  bgav_mpv_gop_header_t        gh;
  int duration;
  int start_code;

  int timescale;
  int frame_duration;
  
  switch(priv->state)
    {
    case MPEG_NEED_SYNC:
      sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                   parser->buf.buffer + parser->buf.size);
      if(!sc)
        return PARSER_NEED_DATA;
      bgav_video_parser_flush(parser, sc - parser->buf.buffer);
      parser->pos = 0;
      priv->state = MPEG_NEED_STARTCODE;
      break;
    case MPEG_NEED_STARTCODE:
      sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                     parser->buf.buffer + parser->buf.size);
      if(!sc)
        return PARSER_NEED_DATA;
      
      start_code = bgav_mpv_get_start_code(sc);
      parser->pos = sc - parser->buf.buffer;

      switch(start_code)
        {
        case MPEG_CODE_SEQUENCE:
          priv->frames_since_sh = 0;
          if(!priv->has_picture_start)
            {
            if(!bgav_video_parser_set_picture_start(parser))
              return PARSER_ERROR;
            priv->has_picture_start = 1;
            }
          
          if(!priv->have_sh)
            priv->state = MPEG_HAS_SEQUENCE_CODE;
          else
            {
            priv->state = MPEG_NEED_STARTCODE;
            parser->pos+=4;
            }
          break;
        case MPEG_CODE_SEQUENCE_EXT:
          if(priv->have_sh && !priv->sh.mpeg2)
            priv->state = MPEG_HAS_SEQUENCE_EXT_CODE;
          else
            {
            priv->state = MPEG_NEED_STARTCODE;
            parser->pos+=4;
            }
          break;
        case MPEG_CODE_SEQUENCE_DISPLAY_EXT:
          if(priv->have_sh && !priv->sh.has_dpy_ext)
            {
            priv->state = MPEG_HAS_SEQUENCE_DISPLAY_EXT_CODE;
            // fprintf(stderr, "Got sequence display extenstion\n");
            }
          else
            {
            priv->state = MPEG_NEED_STARTCODE;
            parser->pos+=4;
            }
          break;
        case MPEG_CODE_PICTURE:

          /* Adjust fourcc */
          if((parser->s->fourcc == BGAV_MK_FOURCC('m','p','g','v')) &&
             priv->have_sh)
            {
            if(priv->sh.mpeg2)
              {
              parser->s->fourcc = BGAV_MK_FOURCC('m','p','v','2');
              gavl_metadata_set(&parser->s->m, GAVL_META_FORMAT,
                                bgav_sprintf("MPEG-2"));
              }
            else
              {
              parser->s->fourcc = BGAV_MK_FOURCC('m','p','v','1');
              gavl_metadata_set(&parser->s->m, GAVL_META_FORMAT,
                                bgav_sprintf("MPEG-1"));
              }
            }

          if(!parser->format->pixel_width)
            bgav_mpv_get_pixel_aspect(&priv->sh,
                                      parser->format);

          if(parser->s->data.video.format.pixelformat == GAVL_PIXELFORMAT_NONE)
            {
            parser->s->data.video.format.pixelformat =
              bgav_mpv_get_pixelformat(&priv->sh);
            }
          
          if(!priv->has_picture_start)
            {
            if(!bgav_video_parser_set_picture_start(parser))
              return PARSER_ERROR;
            }
          priv->has_picture_start = 0;

          bgav_video_parser_set_header_end(parser);
          
          /* Need the picture header */
          priv->state = MPEG_HAS_PICTURE_CODE;
          
          if(!parser->s->ext_data && priv->have_sh)
            {
            bgav_video_parser_extract_header(parser);
            return PARSER_CONTINUE;
            }

          
          break;
        case MPEG_CODE_PICTURE_EXT:
          /* Need picture extension */
          priv->state = MPEG_HAS_PICTURE_EXT_CODE;
          break;
        case MPEG_CODE_GOP:
          if(!priv->has_picture_start)
            {
            if(!bgav_video_parser_set_picture_start(parser))
              return PARSER_ERROR;
            priv->has_picture_start = 1;
            }
          bgav_video_parser_set_header_end(parser);
          priv->state = MPEG_HAS_GOP_CODE;
          if(!parser->s->ext_data)
            {
            bgav_video_parser_extract_header(parser);
            return PARSER_CONTINUE;
            }
          break;
        case MPEG_CODE_END:
          parser->pos += 4;
          // fprintf(stderr, "Detected sequence end\n");
          /* Set sequence end  */
          bgav_video_parser_set_sequence_end(parser, 4);

          /* Reset timestamp for still mode */
          if(!priv->pb_count && (priv->i_count == 1))
            parser->timestamp = BGAV_TIMESTAMP_UNDEFINED;

          /* */

          if(parser->cache[parser->cache_size-1].header_size &&
             parser->cache[parser->cache_size-1].sequence_end_pos &&
             !(parser->s->flags & STREAM_STILL_MODE))
            {
            parser->s->data.video.format.framerate_mode = GAVL_FRAMERATE_STILL;
            parser->s->flags |= STREAM_STILL_MODE;
            bgav_log(parser->s->opt, BGAV_LOG_INFO, LOG_DOMAIN,
                     "Detected MPEG still mode");
            }
          priv->pb_count = 0;
          priv->i_count = 0;
          //          fprintf(stderr, "Detected sequence end\n");
          break;
        default:
          parser->pos += 4;
          priv->state = MPEG_NEED_STARTCODE;
          break;
        }
      
      break;
    case MPEG_HAS_PICTURE_CODE:

      /* Now we know for sure that we have MPEG-1 */
      if(priv->have_sh &&
         !priv->sh.mpeg2 && (parser->s->data.video.format.framerate_mode == GAVL_FRAMERATE_UNKNOWN))
        parser->s->data.video.format.framerate_mode = GAVL_FRAMERATE_CONSTANT;
      
      /* Try to get the picture header */
      len = bgav_mpv_picture_header_parse(parser->s->opt,
                                          &ph,
                                          parser->buf.buffer + parser->pos,
                                          parser->buf.size - parser->pos);
      
      if(len < 0)
        {
        bgav_log(parser->s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Bogus picture header or broken frame");
        return PARSER_ERROR;
        }
      if(!len)
        return PARSER_NEED_DATA;
      
      bgav_video_parser_set_coding_type(parser, ph.coding_type);

      if(priv->have_sh)
        {
        if(!(parser->flags & PARSER_NO_I_FRAMES) &&
           (ph.coding_type == BGAV_CODING_TYPE_P) &&
           (!priv->frames_since_sh))
          {
          parser->flags |= PARSER_NO_I_FRAMES;
          bgav_log(parser->s->opt, BGAV_LOG_DEBUG, LOG_DOMAIN,
                   "Detected Intra slice refresh");
          }
        priv->frames_since_sh++;
        }

      if(ph.coding_type == BGAV_CODING_TYPE_I)
        priv->i_count++;
      else
        priv->pb_count++;
        
      
      //        fprintf(stderr, "Pic type: %c\n", ph.coding_type);
      parser->pos += len;
      priv->state = MPEG_NEED_STARTCODE;
      
      break;
    case MPEG_HAS_PICTURE_EXT_CODE:
      /* Try to get the picture extension */
      len = bgav_mpv_picture_extension_parse(parser->s->opt,
                                             &pe,
                                             parser->buf.buffer + parser->pos,
                                             parser->buf.size - parser->pos);
      if(!len)
        return PARSER_NEED_DATA;

      /* Set interlacing stuff */
      switch(pe.picture_structure)
        {
        case MPEG_PICTURE_TOP_FIELD:
          parser->cache[parser->cache_size-1].field_pic = 1;
          parser->cache[parser->cache_size-1].ilace = GAVL_INTERLACE_TOP_FIRST;
          break;
        case MPEG_PICTURE_BOTTOM_FIELD:
          parser->cache[parser->cache_size-1].field_pic = 1;
          parser->cache[parser->cache_size-1].ilace = GAVL_INTERLACE_BOTTOM_FIRST;
          break;
        case  MPEG_PICTURE_FRAME:
          if(pe.repeat_first_field)
            {
            duration = 0;
            if(priv->sh.ext.progressive_sequence)
              {
              if(pe.top_field_first)
                duration = parser->format->frame_duration * 2;
              else
                duration = parser->format->frame_duration;
              }
            else if(pe.progressive_frame)
              duration = parser->format->frame_duration / 2;
        
            parser->cache[parser->cache_size-1].duration += duration;
            }
          else if(!priv->sh.ext.progressive_sequence)
            {
            if(pe.progressive_frame)
              parser->cache[parser->cache_size-1].ilace = GAVL_INTERLACE_NONE;
            else if(pe.top_field_first)
              parser->cache[parser->cache_size-1].ilace = GAVL_INTERLACE_TOP_FIRST;
            else
              parser->cache[parser->cache_size-1].ilace = GAVL_INTERLACE_BOTTOM_FIRST;
            }
          break;
        }
      
      parser->pos += len;
      priv->state = MPEG_NEED_STARTCODE;
      break;
    case MPEG_HAS_GOP_CODE:
      len = bgav_mpv_gop_header_parse(parser->s->opt,
                                      &gh,
                                      parser->buf.buffer + parser->pos,
                                      parser->buf.size - parser->pos);
      if(!len)
        return PARSER_NEED_DATA;
      parser->pos += len;
      priv->state = MPEG_NEED_STARTCODE;
      
      if(!parser->s->data.video.format.timecode_format.int_framerate)
        {
        parser->s->data.video.format.timecode_format.int_framerate =
          parser->s->data.video.format.timescale / parser->s->data.video.format.frame_duration;
        if(gh.drop)
          parser->s->data.video.format.timecode_format.flags |=
            GAVL_TIMECODE_DROP_FRAME;
        }

      gavl_timecode_from_hmsf(&parser->cache[parser->cache_size-1].tc,
                              gh.hours,
                              gh.minutes,
                              gh.seconds,
                              gh.frames);
      break;
    case MPEG_HAS_SEQUENCE_CODE:
      /* Try to get the sequence header */

      if(!priv->have_sh)
        {
        len = bgav_mpv_sequence_header_parse(parser->s->opt,
                                             &priv->sh,
                                             parser->buf.buffer + parser->pos,
                                             parser->buf.size - parser->pos);
        if(!len)
          return PARSER_NEED_DATA;
        parser->pos += len;

        bgav_mpv_get_framerate(priv->sh.frame_rate_index, &timescale, &frame_duration);

        parser->format->timescale = timescale;
        parser->format->frame_duration = frame_duration;
        bgav_video_parser_set_framerate(parser);
        
        parser->format->image_width  = priv->sh.horizontal_size_value;
        parser->format->image_height = priv->sh.vertical_size_value;
        parser->format->frame_width  =
          (parser->format->image_width + 15) & ~15;
        parser->format->frame_height  =
          (parser->format->image_height + 15) & ~15;
        
        parser->s->codec_bitrate = priv->sh.bitrate * 400;
        
        priv->have_sh = 1;
        }
      else
        parser->pos += 4;
      
      priv->state = MPEG_NEED_STARTCODE;
      
      break;
    case MPEG_HAS_SEQUENCE_EXT_CODE:
      if(!priv->sh.mpeg2)
        {
        /* Try to get the sequence extension */
        len =
          bgav_mpv_sequence_extension_parse(parser->s->opt,
                                            &priv->sh.ext,
                                            parser->buf.buffer + parser->pos,
                                            parser->buf.size - parser->pos);
        if(!len)
          return PARSER_NEED_DATA;
        
        priv->sh.mpeg2 = 1;
        
        if(parser->s->data.video.format.interlace_mode == GAVL_INTERLACE_UNKNOWN)
          {
          if(priv->sh.ext.progressive_sequence)
            parser->s->data.video.format.interlace_mode = GAVL_INTERLACE_NONE;
          else
            parser->s->data.video.format.interlace_mode = GAVL_INTERLACE_MIXED;
          }
        
        if(parser->s->data.video.format.framerate_mode == GAVL_FRAMERATE_UNKNOWN)
          parser->s->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
        
        parser->format->timescale *=  (priv->sh.ext.timescale_ext+1) * 2;
        parser->format->frame_duration *= (priv->sh.ext.frame_duration_ext+1) * 2;
        
        bgav_video_parser_set_framerate(parser);
        
        parser->format->image_width  += priv->sh.ext.horizontal_size_ext;
        parser->format->image_height += priv->sh.ext.vertical_size_ext;
        
        parser->format->frame_width  =
          (parser->format->image_width + 15) & ~15;
        parser->format->frame_height  =
          (parser->format->image_height + 15) & ~15;
        
        if(priv->sh.ext.low_delay)
          parser->s->flags &= ~STREAM_B_FRAMES;

        parser->s->codec_bitrate += (priv->sh.ext.bitrate_ext << 18) * 400;
        
        parser->pos += len;
        
        }
      else
        parser->pos += 4;
      
      priv->state = MPEG_NEED_STARTCODE;
      break;
    case MPEG_HAS_SEQUENCE_DISPLAY_EXT_CODE:
      if(!priv->sh.has_dpy_ext)
        {
        /* Try to get the sequence extension */
        len =
          bgav_mpv_sequence_display_extension_parse(parser->s->opt,
                                                    &priv->sh.dpy_ext,
                                                    parser->buf.buffer + parser->pos,
                                                    parser->buf.size - parser->pos);
        if(!len)
          return PARSER_NEED_DATA;
        
        priv->sh.has_dpy_ext = 1;
        parser->pos += len;
        }
      else
        parser->pos += 4;
      
      priv->state = MPEG_NEED_STARTCODE;
      break;
      
    }
  return PARSER_CONTINUE;
  }

static int parse_frame_mpeg12(bgav_video_parser_t * parser, bgav_packet_t * p)
  {
  const uint8_t * sc;
  mpeg12_priv_t * priv = parser->priv;
  //  cache_t * c;
  bgav_mpv_picture_extension_t pe;
  bgav_mpv_picture_header_t    ph;
  int start_code;
  int len;
  int delta_d;
  int timescale;
  int frame_duration;

  const uint8_t * start =   p->data;
  const uint8_t * end = p->data + p->data_size;
  
  int ret = PARSER_CONTINUE;
  
  while(1)
    {
    sc = bgav_mpv_find_startcode(start, end);
    if(!sc)
      return PARSER_NEED_DATA;
    
    start_code = bgav_mpv_get_start_code(sc);

    /* Update position */
    start = sc;
    
    switch(start_code)
      {
      case MPEG_CODE_SEQUENCE:
        if(!priv->have_sh)
          {
          len = bgav_mpv_sequence_header_parse(parser->s->opt,
                                               &priv->sh,
                                               start, end - start);
          if(!len)
            return PARSER_ERROR;
          priv->have_sh = 1;

          if(!parser->format->timescale)
            {
            bgav_mpv_get_framerate(priv->sh.frame_rate_index,
                                   &timescale, &frame_duration);
          
            parser->format->timescale = timescale;
            parser->format->frame_duration = frame_duration;
            bgav_video_parser_set_framerate(parser);
            }
          else
            priv->framerate_from_container = 1;
          
          start += len;
          }
        else
          start += 4;
        break;
      case MPEG_CODE_SEQUENCE_EXT:
        if(priv->have_sh && !priv->sh.mpeg2)
          {
          len =
            bgav_mpv_sequence_extension_parse(parser->s->opt, &priv->sh.ext, start, end - start);
          if(!len)
            return PARSER_ERROR;
          priv->sh.mpeg2 = 1;

          if(!priv->framerate_from_container)
            {
            parser->format->timescale *= (priv->sh.ext.timescale_ext+1) * 2;
            parser->format->frame_duration *= (priv->sh.ext.frame_duration_ext+1) * 2;
            bgav_video_parser_set_framerate(parser);
            }
          start += len;
          }
        else
          start += 4;
        break;
      case MPEG_CODE_PICTURE:
        if(!parser->s->ext_data && priv->have_sh)
          {
          bgav_video_parser_extract_header(parser);
          ret = PARSER_CONTINUE;

          if(priv->sh.mpeg2)
            gavl_metadata_set(&parser->s->m, GAVL_META_FORMAT,
                              bgav_sprintf("MPEG-2"));
          else
            gavl_metadata_set(&parser->s->m, GAVL_META_FORMAT,
                              bgav_sprintf("MPEG-1"));
          }
        len = bgav_mpv_picture_header_parse(parser->s->opt,
                                            &ph, start, end - start);
        p->duration = parser->format->frame_duration;
        if(!len)
          return PARSER_ERROR;

        PACKET_SET_CODING_TYPE(p, ph.coding_type);
        if(ph.coding_type == BGAV_CODING_TYPE_I)
          PACKET_SET_KEYFRAME(p);
        
        start += len;
        
        if(parser->s->data.video.format.pixelformat == GAVL_PIXELFORMAT_NONE)
          {
          parser->s->data.video.format.pixelformat =
            bgav_mpv_get_pixelformat(&priv->sh);

          bgav_mpv_get_size(&priv->sh,
                            &parser->s->data.video.format);

          /* Special handling for D10 */
          if(priv->d10)
            {
            if(parser->s->data.video.format.image_height == 608)
              parser->s->data.video.format.image_height = 576;
            else if(parser->s->data.video.format.image_height == 512)
              parser->s->data.video.format.image_height = 486;
            }
            
          }
        break;
      case MPEG_CODE_PICTURE_EXT:
        len = bgav_mpv_picture_extension_parse(parser->s->opt,
                                               &pe, start, end - start);
        if(!len)
          return PARSER_ERROR;

        if(pe.repeat_first_field)
          {
          delta_d = 0;
          if(priv->sh.ext.progressive_sequence)
            {
            if(pe.top_field_first)
              delta_d = parser->format->frame_duration * 2;
            else
              delta_d = parser->format->frame_duration;
            }
          else if(pe.progressive_frame)
            delta_d = parser->format->frame_duration / 2;
          
          p->duration += delta_d;
          }
        start += len;
        break;
      case MPEG_CODE_GOP:
        {
        bgav_mpv_gop_header_t        gh;
        
        if(!parser->s->ext_data && priv->have_sh)
          {
          bgav_video_parser_extract_header(parser);
          ret = PARSER_CONTINUE;
          if(priv->sh.mpeg2)
            gavl_metadata_set(&parser->s->m, GAVL_META_FORMAT,
                              bgav_sprintf("MPEG-2"));
          else
            gavl_metadata_set(&parser->s->m, GAVL_META_FORMAT,
                              bgav_sprintf("MPEG-1"));
          }

        len = bgav_mpv_gop_header_parse(parser->s->opt,
                                        &gh, start, end - start);
        
        if(!len)
          return PARSER_ERROR;
        
        start += len;

        if(!parser->s->data.video.format.timecode_format.int_framerate)
          {
          parser->s->data.video.format.timecode_format.int_framerate =
            parser->s->data.video.format.timescale /
            parser->s->data.video.format.frame_duration;
          if(gh.drop)
            parser->s->data.video.format.timecode_format.flags |=
              GAVL_TIMECODE_DROP_FRAME;
          }
        
        gavl_timecode_from_hmsf(&p->tc,
                                gh.hours,
                                gh.minutes,
                                gh.seconds,
                                gh.frames);
        }
        break;
      case MPEG_CODE_SLICE:
        return ret;
      default:
        start += 4;
      }
    }
  }

static void cleanup_mpeg12(bgav_video_parser_t * parser)
  {
  free(parser->priv);
  }

void bgav_video_parser_init_mpeg12(bgav_video_parser_t * parser)
  {
  mpeg12_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  parser->priv        = priv;
  parser->parse       = parse_mpeg12;
  parser->parse_frame = parse_frame_mpeg12;
  parser->cleanup     = cleanup_mpeg12;
  parser->reset       = reset_mpeg12;
  
  if((parser->s->fourcc == BGAV_MK_FOURCC('m', 'x', '5', 'p')) ||
     (parser->s->fourcc == BGAV_MK_FOURCC('m', 'x', '4', 'p')) ||
     (parser->s->fourcc == BGAV_MK_FOURCC('m', 'x', '3', 'p')) ||
     (parser->s->fourcc == BGAV_MK_FOURCC('m', 'x', '5', 'n')) ||
     (parser->s->fourcc == BGAV_MK_FOURCC('m', 'x', '4', 'n')) ||
     (parser->s->fourcc == BGAV_MK_FOURCC('m', 'x', '3', 'n')))
    {
    parser->s->codec_bitrate =
      (((parser->s->fourcc & 0x0000FF00) >> 8) - '0') * 10000000;
    priv->d10 = 1;
    parser->s->flags |= STREAM_INTRA_ONLY;
    parser->s->data.video.format.interlace_mode = GAVL_INTERLACE_TOP_FIRST;
    }

  /* Set stream flags */
  if(!(parser->s->flags & STREAM_INTRA_ONLY))
    parser->s->flags |= STREAM_B_FRAMES;

  }
