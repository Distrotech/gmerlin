/*****************************************************************
 
  subtitle.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>

int bgav_num_subtitle_streams(bgav_t * bgav, int track)
  {
  return bgav->tt->tracks[track].num_subtitle_streams;
  }

int bgav_set_subtitle_stream(bgav_t * b, int stream, bgav_stream_action_t action)
  {
  if((stream >= b->tt->current_track->num_subtitle_streams) || (stream < 0))
    return 0;
  b->tt->current_track->subtitle_streams[stream].action = action;
  return 1;
  }

const gavl_video_format_t * bgav_get_subtitle_format(bgav_t * bgav, int stream)
  {
  return &(bgav->tt->current_track->subtitle_streams[stream].data.subtitle.format);
  }

int bgav_subtitle_is_text(bgav_t * bgav, int stream)
  {
  if(bgav->tt->current_track->subtitle_streams[stream].type == BGAV_STREAM_SUBTITLE_TEXT)
    return 1;
  else
    return 0;
  }


const char * bgav_get_subtitle_language(bgav_t * b, int s)
  {
  return (b->tt->current_track->subtitle_streams[s].language[0] != '\0') ?
    b->tt->current_track->subtitle_streams[s].language : (char*)0;
  }

int bgav_read_subtitle_overlay(bgav_t * b, gavl_overlay_t * ovl, int stream)
  {
  bgav_stream_t * s = &(b->tt->current_track->subtitle_streams[stream]);

  if(bgav_has_subtitle(b, stream))
    {
    if(s->data.subtitle.eof)
      return 0;
    }
  else
    return 0;
  
  if(s->data.subtitle.subreader)
    {
    return bgav_subtitle_reader_read_overlay(s, ovl);
    }
  return s->data.subtitle.decoder->decoder->decode(s, ovl);
  }

static void remove_cr(char * str)
  {
  char * dst;
  int i;
  int len = strlen(str);
  dst = str;
  
  for(i = 0; i <= len; i++)
    {
    if(str[i] != '\r')
      {
      *dst = str[i];
      dst++;
      }
    }
  }

int bgav_read_subtitle_text(bgav_t * b, char ** ret, int *ret_alloc,
                            gavl_time_t * start_time, gavl_time_t * duration,
                            int stream)
  {
  int out_len;
  bgav_packet_t * p = (bgav_packet_t*)0;
  bgav_stream_t * s = &(b->tt->current_track->subtitle_streams[stream]);

  if(bgav_has_subtitle(b, stream))
    {
    if(s->data.subtitle.eof)
      return 0;
    }
  else
    return 0;
  
  if(s->packet_buffer)
    {
    p = bgav_demuxer_get_packet_read(s->demuxer, s);
    bgav_packet_get_text_subtitle(p, ret, ret_alloc, start_time, duration);
    }
  else if(s->data.subtitle.subreader)
    {
    p = bgav_subtitle_reader_read_text(s);
    }
  else
    return 0; /* Never get here */
  
  /* Convert packet to subtitle */


  if(s->data.subtitle.cnv)
    {
    if(!bgav_convert_string_realloc(s->data.subtitle.cnv,
                                    (const char *)p->data, p->data_size,
                                    &out_len,
                                    ret, ret_alloc))
      return 0;
    }
  else
    {
    if(*ret_alloc < p->data_size+1)
      {
      *ret_alloc = p->data_size + 128;
      *ret = realloc(*ret, *ret_alloc);
      }
    memcpy(*ret, p->data, p->data_size);
    (*ret)[p->data_size] = '\0';
    }
  
  *start_time = gavl_time_unscale(s->timescale, p->timestamp_scaled);
  *duration   = gavl_time_unscale(s->timescale, p->duration_scaled);

  remove_cr(*ret);
    
  if(s->packet_buffer)
    {
    bgav_demuxer_done_packet_read(s->demuxer, p);
    }

  
  return 1;
  }

int bgav_has_subtitle(bgav_t * b, int stream)
  {
  bgav_stream_t * s = &(b->tt->current_track->subtitle_streams[stream]);

  if(s->data.subtitle.eof)
    return 1;
  
  if(s->packet_buffer)
    {
    if(s->type == BGAV_STREAM_SUBTITLE_TEXT)
      {
      if(bgav_demuxer_peek_packet_read(s->demuxer, s, 0))
        return 1;
      else
        {
        if(s->demuxer->eof)
          {
          s->data.subtitle.eof = 1;
          return 1;
          }
        else
          return 0;
        }
      }
    else
      {
      if(s->data.subtitle.decoder->decoder->has_subtitle(s))
        return 1;
      else if(s->demuxer->eof)
        {
        s->data.subtitle.eof = 1;
        return 1;
        }
      else
        return 0;
      }
    }
  else if(s->data.subtitle.subreader)
    {
    if(bgav_subtitle_reader_has_subtitle(s))
      return 1;
    else
      {
      s->data.subtitle.eof = 1;
      return 1;
      }
    }
  else
    return 0;
  }


void bgav_subtitle_dump(bgav_stream_t * s)
  {
  if(s->type == BGAV_STREAM_SUBTITLE_OVERLAY)
    {
    bgav_dprintf( "Format:\n");
    gavl_video_format_dump(&(s->data.subtitle.format));
    }
  else
    {
    bgav_dprintf( "Character set: %s\n", s->data.subtitle.charset);
    }
  }

int bgav_subtitle_start(bgav_stream_t * s)
  {
  bgav_subtitle_overlay_decoder_t * dec;
  bgav_subtitle_overlay_decoder_context_t * ctx;

  s->data.subtitle.eof = 0;
  
  if(s->type == BGAV_STREAM_SUBTITLE_TEXT)
    {
    if(s->data.subtitle.subreader)
      if(!bgav_subtitle_reader_start(s))
        return 0;
    
    if(s->data.subtitle.charset)
      {
      if(strcmp(s->data.subtitle.charset, "UTF-8"))
        s->data.subtitle.cnv =
          bgav_charset_converter_create(s->data.subtitle.charset, "UTF-8");
      }
    else if(strcmp(s->opt->default_subtitle_encoding, "UTF-8"))
      s->data.subtitle.cnv =
        bgav_charset_converter_create(s->opt->default_subtitle_encoding,
                                      "UTF-8");
    return 1;
    }
  else
    {
    if(s->data.subtitle.subreader)
      {
      if(!bgav_subtitle_reader_start(s))
        return 0;
      else
        return 1;
      }
    
    dec = bgav_find_subtitle_overlay_decoder(s);
    if(!dec)
      {
      fprintf(stderr, "No subtitle overlay decoder found for fourcc ");
      bgav_dump_fourcc(s->fourcc);
      fprintf(stderr, "\n");
      return 0;
      }
    ctx = calloc(1, sizeof(*ctx));
    s->data.subtitle.decoder = ctx;
    s->data.subtitle.decoder->decoder = dec;
    
    if(!dec->init(s))
      return 0;
    }
  return 1;
  }

void bgav_subtitle_stop(bgav_stream_t * s)
  {
  if(s->data.subtitle.cnv)
    bgav_charset_converter_destroy(s->data.subtitle.cnv);
  if(s->data.subtitle.charset)
    free(s->data.subtitle.charset);
  if(s->data.subtitle.subreader)
    bgav_subtitle_reader_close(s);
  }

void bgav_subtitle_resync(bgav_stream_t * s)
  {
  /* Nothing to do here */
  if(s->type == BGAV_STREAM_SUBTITLE_TEXT)
    return;

  if(s->data.subtitle.decoder &&
     s->data.subtitle.decoder->decoder->resync)
    s->data.subtitle.decoder->decoder->resync(s);
  }

int bgav_subtitle_skipto(bgav_stream_t * s, gavl_time_t * time)
  {
  return 0;
  }

const char * bgav_get_subtitle_info(bgav_t * b, int s)
  {
  return b->tt->current_track->subtitle_streams[s].info;
  }
