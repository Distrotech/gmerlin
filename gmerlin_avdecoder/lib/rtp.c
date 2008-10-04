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
#include <ctype.h>
#include <string.h>
#include <poll.h>


#include <avdec_private.h>
// #include <inttypes.h>
#include <rtp.h>

/* Stream specific init and parse functions are at the end of the file */

/* mpeg4-generic */
static int init_mpeg4_generic_audio(bgav_stream_t * s);

/* H264 */
static int init_h264(bgav_stream_t * s);

static int init_mpa(bgav_stream_t * s);


static int rtp_header_read(bgav_input_context_t * ctx,
                           rtp_header_t * ret)
  {
  int i;
  uint32_t h;
  if(!bgav_input_read_32_be(ctx, &h))
    return 0;
  ret->version         = (h >> 30) & 0x03;
  ret->padding         = (h >> 29) & 0x01;
  ret->extension       = (h >> 28) & 0x01;
  ret->csrc_count      = (h >> 24) & 0x0f;
  ret->marker          = (h >> 23) & 0x01;
  ret->payload_type    = (h >> 16) & 0x7f;
  ret->sequence_number = (h)       & 0xffff;

  if(!bgav_input_read_32_be(ctx, &ret->timestamp))
    return 0;
  if(!bgav_input_read_32_be(ctx, &ret->ssrc))
    return 0;
  for(i = 0; i < ret->csrc_count; i++)
    {
    if(!bgav_input_read_32_be(ctx, &ret->csrc_list[i]))
      return 0;
    }
  return 1;
  }

static void rtp_header_dump(rtp_header_t * h)
  {
  int i;
  bgav_dprintf("RTP Header\n");
  bgav_dprintf("  version:      %d\n", h->version);
  bgav_dprintf("  padding:      %d\n", h->padding);
  bgav_dprintf("  extension:    %d\n", h->extension);
  bgav_dprintf("  csrc_count:   %d\n", h->csrc_count);
  bgav_dprintf("  marker:       %d\n", h->marker);
  bgav_dprintf("  payload_type: %d\n", h->payload_type);
  bgav_dprintf("  seq:          %d\n", h->sequence_number);
  bgav_dprintf("  timestamp:    %u\n", h->timestamp);
  for(i = 0; i < h->csrc_count; i++)
    {
    bgav_dprintf("  csrc[%d]:    %d\n", i, h->csrc_list[i]);
    }
  }

/* RTCP Sender report */

#define MAX_RTCP_REPORTS 31

typedef struct
  {
  uint8_t version;
  uint8_t padding;
  uint8_t rc;
  uint8_t type;
  uint16_t length;
  uint32_t ssrc;

  /* These miss in Receiver report */
  uint64_t ntp_time;
  uint32_t rtp_time;
  uint32_t packet_count;
  uint32_t octet_count;

  /* Report blocks */
  
  struct
    {
    uint32_t ssrc;
    uint8_t fraction_lost;
    uint32_t cumulative_lost;
    uint32_t highest_ext_seq;
    uint32_t jitter;
    uint32_t lsr;
    uint32_t dlsr;
    } reports[MAX_RTCP_REPORTS];
  } rtcp_rr_t;

static int rtcp_rr_read(bgav_input_context_t * ctx, rtcp_rr_t * ret)
  {
  uint16_t h;
  int i;
  if(!bgav_input_read_16_be(ctx, &h))
    return 0;
  ret->version         = (h >> 14) & 0x03;
  ret->padding         = (h >> 13) & 0x01;
  ret->rc              = (h >>  8) & 0x1f;
  ret->type            = (h) & 0xff;

  if(!bgav_input_read_16_be(ctx, &ret->length) ||
     !bgav_input_read_32_be(ctx, &ret->ssrc) ||
     !bgav_input_read_64_be(ctx, &ret->ntp_time) ||
     !bgav_input_read_32_be(ctx, &ret->rtp_time) ||
     !bgav_input_read_32_be(ctx, &ret->packet_count) ||
     !bgav_input_read_32_be(ctx, &ret->octet_count))
    return 0;

  for(i = 0; i < ret->rc; i++)
    {
    if(!bgav_input_read_32_be(ctx, &ret->reports[i].ssrc) ||
       !bgav_input_read_8(ctx, &ret->reports[i].fraction_lost) ||
       !bgav_input_read_24_be(ctx, &ret->reports[i].cumulative_lost) ||
       !bgav_input_read_32_be(ctx, &ret->reports[i].highest_ext_seq) ||
       !bgav_input_read_32_be(ctx, &ret->reports[i].jitter) ||
       !bgav_input_read_32_be(ctx, &ret->reports[i].lsr) ||
       !bgav_input_read_32_be(ctx, &ret->reports[i].dlsr))
      return 0;
    }
  return 1;
  }

static void rtcp_rr_dump(rtcp_rr_t * r)
  {
  int i;
  bgav_dprintf("RTCP RR\n");
  bgav_dprintf("  version:      %d\n", r->version);
  bgav_dprintf("  padding:      %d\n", r->padding);
  bgav_dprintf("  rc:           %d\n", r->rc);
  bgav_dprintf("  type:         %d\n", r->type);
  bgav_dprintf("  length:       %d\n", r->length);
  bgav_dprintf("  ssrc:         %u\n", r->ssrc);
  bgav_dprintf("  ntp_time:     %"PRIu64"\n", r->ntp_time);
  bgav_dprintf("  rtp_time:     %u\n", r->rtp_time);
  bgav_dprintf("  packet_count: %u\n", r->packet_count);
  bgav_dprintf("  octet_count:  %u\n", r->octet_count);

  for(i = 0; i < r->rc; i++)
    {
    bgav_dprintf("  Report %d\n", i+1);
    bgav_dprintf("    ssrc:            %d\n", r->reports[i].ssrc);
    bgav_dprintf("    fraction_lost:   %d\n", r->reports[i].fraction_lost);
    bgav_dprintf("    cumulative_lost: %d\n", r->reports[i].cumulative_lost);
    bgav_dprintf("    highest_ext_seq: %d\n", r->reports[i].highest_ext_seq);
    bgav_dprintf("    jitter:          %d\n", r->reports[i].jitter);
    bgav_dprintf("    lsr:             %d\n", r->reports[i].lsr);
    bgav_dprintf("    dlsr:            %d\n", r->reports[i].dlsr);
    }
  }
/* */

typedef struct
  {
  const char * name;
  const uint32_t fourcc;
  int (*init)(bgav_stream_t * s);
  } dynamic_payload_t;

typedef struct
  {
  int format;
  const uint32_t fourcc;
  int bits;
  int timescale;
  int (*init)(bgav_stream_t * s);
  } static_payload_t;

typedef struct
  {
  struct pollfd * pollfds;
  int num_pollfds;

  bgav_input_context_t * input_mem;
  } rtp_priv_t;

static const static_payload_t static_payloads[] =
  {
    // 0          PCMU          A              8000          1     [RFC3551]
    { 0, BGAV_MK_FOURCC('u','l','a','w'), 8, 8000 },
    // 1          Reserved	  
    // 2          Reserved
    // 3          GSM           A              8000          1     [RFC3551]
    { 3, BGAV_MK_FOURCC('G', 'S', 'M', ' '), 0, 8000 },
    // 4          G723          A              8000          1     [Kumar]
    // 5          DVI4          A              8000          1     [RFC3551]
    // 6          DVI4          A              16000         1     [RFC3551]
    // 7          LPC           A              8000          1     [RFC3551]
    // 8          PCMA          A              8000          1     [RFC3551]
    { 8, BGAV_MK_FOURCC('a','l','a','w'), 8, 8000 },
    // 9          G722          A              8000          1     [RFC3551]
    // 10         L16           A              44100         2     [RFC3551]
    { 10, BGAV_MK_FOURCC('t','w','o','s'), 16, 44100 },
    // 11         L16           A              44100         1     [RFC3551]
    { 11, BGAV_MK_FOURCC('t','w','o','s'), 16, 44100 },
    // 12         QCELP         A              8000          1 
    { 11, BGAV_MK_FOURCC('q','c','l','p'), 0, 8000 },
    // 13         CN            A              8000          1     [RFC3389]
    // 14         MPA           A              90000               [RFC3551][RFC2250]
    { 14, BGAV_MK_FOURCC('.','m','p','3'), 0, 90000, init_mpa },
    // 15         G728          A              8000          1     [RFC3551]
    // 16         DVI4          A              11025         1     [DiPol]
    // 17         DVI4          A              22050         1     [DiPol]
    // 18         G729          A              8000          1
    // 19         reserved      A
    // 20         unassigned    A
    // 21         unassigned    A
    // 22         unassigned    A
    // 23         unassigned    A
    // 24         unassigned    V
    // 25         CelB          V              90000               [RFC2029]
    // 26         JPEG          V              90000               [RFC2435]
    { 26, BGAV_MK_FOURCC('j','p','e','g'), 0, 90000 },
    // 27         unassigned    V
    // 28         nv            V              90000               [RFC3551]
    // 29         unassigned    V
    // 30         unassigned    V
    // 31         H261          V              90000               [RFC2032]
    { 31, BGAV_MK_FOURCC('h','2','6','1'), 0, 90000 },
    // 32         MPV           V              90000               [RFC2250]
    { 32, BGAV_MK_FOURCC('m','p','g','v'), 0, 90000 },
    // 33         MP2T          AV             90000               [RFC2250]
    // 34         H263          V              90000               [Zhu]
    { 34, BGAV_MK_FOURCC('h','2','6','3'), 0, 90000 },
    { -1, },
  };

static const dynamic_payload_t dynamic_video_payloads[] =
  {
    { "H264", BGAV_MK_FOURCC('h', '2', '6', '4'), init_h264 },
    { },
  };

static const dynamic_payload_t dynamic_audio_payloads[] =
  {
    { "mpeg4-generic", BGAV_MK_FOURCC('m', 'p', '4', 'a'), init_mpeg4_generic_audio },
    { },
  };

static void check_dynamic(bgav_stream_t * s, const dynamic_payload_t * dynamic_payloads,
                          const char * rtpmap)
  {
  const char * pos1, *pos2;
  int i;
  pos1 = rtpmap;
  pos2 = strchr(pos1, '/');
  if(!pos2)
    return;
  
  i = 0;

  while(dynamic_payloads[i].name)
    {
    if((strlen(dynamic_payloads[i].name) == pos2 - pos1) &&
       !strncmp(dynamic_payloads[i].name, pos1, pos2 - pos1))
      {
      s->fourcc = dynamic_payloads[i].fourcc;
      if(dynamic_payloads[i].init)
        dynamic_payloads[i].init(s);
      break;
      }
    i++;
    }
  }


static int find_codec(bgav_stream_t * s, bgav_sdp_media_desc_t * md, int format_index)
  {
  int format = atoi(md->formats[0]);
  int i;
  char * fmtp;
  rtp_stream_priv_t * sp = (rtp_stream_priv_t*)s->priv;

  if(bgav_sdp_get_attr_fmtp(md->attributes, md->num_attributes,
                            format, &fmtp))
    sp->fmtp = bgav_stringbreak(fmtp, ';');
  
  if(format >= 96) // Dynamic
    {
    char * rtpmap;
    char * pos;
    if(!bgav_sdp_get_attr_rtpmap(md->attributes, md->num_attributes,
                                 format, &rtpmap))
      {
      fprintf(stderr, "Got no rtpmap\n");
      return 0;
      }
    // fprintf(stderr, "rtpmap: %s\n", rtpmap);


    if(s->type == BGAV_STREAM_AUDIO)
      {
      check_dynamic(s, dynamic_audio_payloads, rtpmap);
      }
    else
      {
      check_dynamic(s, dynamic_video_payloads, rtpmap);
      }
    
    if(!s->fourcc)
      return 0;

    pos = strchr(rtpmap, '/');

    if(pos)
      {
      pos++;
      s->timescale = atoi(pos);
      }

    if(s->type == BGAV_STREAM_AUDIO)
      {
      pos = strchr(pos, '/');
      if(pos)
        {
        pos++;
        s->data.audio.format.num_channels = atoi(pos);
        }
      }

    return 1;
    }
  else
    {
    i = 0;
    while(static_payloads[i].format >= 0)
      {
      if(static_payloads[i].format == format)
        {
        s->fourcc = static_payloads[i].fourcc;
        s->timescale = static_payloads[i].timescale;
        return 1;
        }
      i++;
      }
    return 0;
    }
  }

static int
init_stream(bgav_demuxer_context_t * ctx,
            bgav_stream_t * s, bgav_sdp_media_desc_t * md,
            int format_index)
  {
  rtp_stream_priv_t * sp;
  char * control, *tmp_string;
  int i;
  fprintf(stderr, "Got stream (format: %s)\n", md->formats[format_index]);

  sp = calloc(1, sizeof(*sp));
  s->priv = sp;
  
  if(!find_codec(s, md, format_index))
    return 0;
  
  fprintf(stderr, "fourcc: ");
  bgav_dump_fourcc(s->fourcc);
  fprintf(stderr, " timescale: %d\n", s->timescale);

  i = 0;
  if(sp->fmtp)
    {
    while(sp->fmtp[i])
      {
      fprintf(stderr, "fmtp[%d]: %s\n", i, sp->fmtp[i]);
      i++;
      }
    }
  
  /* Get control URL */
  if(!bgav_sdp_get_attr_string(md->attributes, md->num_attributes,
                               "control", &control))
    {
    fprintf(stderr, "Got no control URL\n");
    return 0;
    }
  tmp_string = (char*)0;
  if(bgav_url_split(control,
                    &tmp_string,
                    (char**)0,
                    (char**)0,
                    (char**)0,
                    (int *)0,
                    (char **)0))
    {
    if(tmp_string) /* Complete URL */
      sp->control_url = bgav_strdup(control);
    }

  if(!sp->control_url)
    sp->control_url = bgav_sprintf("%s/%s", ctx->input->url, control);
  fprintf(stderr, "Control: %s\n", sp->control_url);
  return 1;
  }

static int bgav_rtp_read_packet(bgav_stream_t * s, bgav_packet_t * p)
  {
  return 0;

  }

static void bgav_rtp_cleanup(bgav_stream_t * s)
  {
  
  }

#define RTP_MAX_PACKET_LENGTH 1500

static int next_packet_rtp(bgav_demuxer_context_t * ctx)
  {
  uint8_t buf[RTP_MAX_PACKET_LENGTH];
  rtp_priv_t * priv;
  int i, index;
  int bytes_read;
  rtp_stream_priv_t * sp;
  rtp_header_t rh;
  rtcp_rr_t rr;

  priv = (rtp_priv_t *)ctx->priv;
  
  if(!priv->pollfds)
    {
    priv->num_pollfds = (ctx->tt->cur->num_audio_streams +
      ctx->tt->cur->num_video_streams)*2;
    priv->pollfds = calloc(priv->num_pollfds, sizeof(*priv->pollfds));
    
    index = 0;
    for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
      {
      sp = (rtp_stream_priv_t *)ctx->tt->cur->audio_streams[i].priv;
      priv->pollfds[index].fd = sp->rtp_fd;
      priv->pollfds[index].events = POLLIN;
      index++;
      priv->pollfds[index].fd = sp->rtcp_fd;
      priv->pollfds[index].events = POLLIN;
      index++;
      }
    for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
      {
      sp = (rtp_stream_priv_t *)ctx->tt->cur->video_streams[i].priv;
      priv->pollfds[index].fd = sp->rtp_fd;
      priv->pollfds[index].events = POLLIN;
      index++;
      priv->pollfds[index].fd = sp->rtcp_fd;
      priv->pollfds[index].events = POLLIN;
      index++;
      }
    }
  
  if(poll(priv->pollfds, priv->num_pollfds, ctx->opt->read_timeout) > 0)
    {
    index = 0;
    
    for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
      {
      if(priv->pollfds[index].revents & POLLIN)
        {
        /* Read Audio RTP Data */
        bytes_read = bgav_udp_read(priv->pollfds[index].fd,
                                   buf, RTP_MAX_PACKET_LENGTH);
        fprintf(stderr, "Got Audio RTP Data: %d bytes\n", bytes_read);

        bgav_input_reopen_memory(priv->input_mem, buf, bytes_read);
        
        if(rtp_header_read(priv->input_mem, &rh))
          {
          //          rtp_header_dump(&rh);
          }
        
        }
      index++;
      if(priv->pollfds[index].revents & POLLIN)
        {
        /* Read Audio RTCP Data */
        bytes_read = bgav_udp_read(priv->pollfds[index].fd,
                                   buf, RTP_MAX_PACKET_LENGTH);
        
        fprintf(stderr, "Got Audio RTCP Data: %d bytes\n", bytes_read);

        if(buf[1] == 200)
          {
          bgav_input_reopen_memory(priv->input_mem, buf, bytes_read);
          if(rtcp_rr_read(priv->input_mem, &rr))
            {
            rtcp_rr_dump(&rr);
            }
          }
        }
      index++;
      }
    for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
      {
      if(priv->pollfds[index].revents & POLLIN)
        {
        /* Read Video RTP Data */
        bytes_read = bgav_udp_read(priv->pollfds[index].fd,
                                   buf, RTP_MAX_PACKET_LENGTH);
        fprintf(stderr, "Got Video RTP Data: %d bytes\n", bytes_read);
        bgav_input_reopen_memory(priv->input_mem, buf, bytes_read);

        if(rtp_header_read(priv->input_mem, &rh))
          {
          //          rtp_header_dump(&rh);
          }
        
        }
      index++;
      if(priv->pollfds[index].revents & POLLIN)
        {
        /* Read Video RTCP Data */
        bytes_read = bgav_udp_read(priv->pollfds[index].fd,
                                   buf, RTP_MAX_PACKET_LENGTH);
        fprintf(stderr, "Got Video RTCP Data: %d bytes\n", bytes_read);
        bgav_input_reopen_memory(priv->input_mem, buf, bytes_read);
        
        }
      index++;
      }
    }
  return 0;
  }

static void close_rtp(bgav_demuxer_context_t * ctx)
  {
  
  }

int bgav_demuxer_rtp_open(bgav_demuxer_context_t * ctx,
                          bgav_sdp_t * sdp)
  {
  int i, j;
  bgav_stream_t * s;
  rtp_priv_t * priv;

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  priv->input_mem = bgav_input_open_memory(NULL, 0, ctx->opt);
  
  ctx->tt = bgav_track_table_create(1);

  for(i = 0; i < sdp->num_media; i++)
    {
    fprintf(stderr, "Media %d, media: %s, protocol: %s\n",
            i, sdp->media[i].media,
            sdp->media[i].protocol);

    if(!strcmp(sdp->media[i].media, "audio"))
      {
      //for(j = 0; j < sdp->media[i].num_formats; j++)
      for(j = 0; j < 1; j++)
        {
        s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
        init_stream(ctx, s, &sdp->media[i], j);

        if(s->fourcc != BGAV_MK_FOURCC('.','m','p','3'))
          s->data.audio.format.samplerate = s->timescale;
        }
      }
    else if(!strcmp(sdp->media[i].media, "video"))
      {
      // for(j = 0; j < sdp->media[i].num_formats; j++)
      for(j = 0; j < 1; j++)
        {
        s = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
        init_stream(ctx, s, &sdp->media[i], j);
        }
      }
    }
  return 1;
  }

bgav_demuxer_t bgav_demuxer_rtp =
  {
    //    .probe =        probe_rmff,
    //    .open =         open_rmff,
    // .select_track = select_track_rmff,
    .next_packet =  next_packet_rtp,
    //    .seek =         seek_rmff,
    .close =        close_rtp
  };


/* Format specific initialization and processing routines */

static uint8_t hex_to_byte(const char * s)
  {
  char c, ret;
  c = toupper(s[0]);

  if (c >= '0' && c <= '9')
    c = c - '0';
  else if (c >= 'A' && c <= 'F')
    c = c - 'A' + 10;

  ret = c << 4;

  c = toupper(s[1]);

  if (c >= '0' && c <= '9')
    c = c - '0';
  else if (c >= 'A' && c <= 'F')
    c = c - 'A' + 10;

  ret |= c;
  return ret;
  }

static char * find_fmtp(char ** fmtp, char * key)
  {
  int i = 0;
  char * pos;
  int len;
  while(fmtp[i])
    {
    pos = fmtp[i];
    while(isspace(*pos))
      pos++;
    len = strlen(key);
    if(!strncasecmp(key, pos, len) &&
       (pos[len] == '='))
      {
      pos += len+1;
      while(isspace(*pos))
        pos++;
      return pos;
      }
    i++;
    }
  return (char*)0;
  }

static int process_mpeg4_generic_audio(bgav_stream_t * s, rtp_header_t * h, uint8_t * data, int len)
  {
  return 0;
  }

static int init_mpeg4_generic_audio(bgav_stream_t * s)
  {
  rtp_stream_priv_t * sp;
  char * var;

  sp = s->priv;
  if(!sp->fmtp)
    return 0;

  var = find_fmtp(sp->fmtp, "sizelength");
  if(!var)
    return 0;
  sp->priv.mpeg4_generic.sizelength = atoi(var);

  var = find_fmtp(sp->fmtp, "indexlength");
  if(!var)
    return 0;
  sp->priv.mpeg4_generic.indexlength = atoi(var);
  
  var = find_fmtp(sp->fmtp, "indexdeltalength");
  if(!var)
    return 0;
  sp->priv.mpeg4_generic.indexdeltalength = atoi(var);

  var = find_fmtp(sp->fmtp, "config");
  if(var)
    {
    int i;
    s->ext_size = strlen(var)/2;
    s->ext_data = malloc(s->ext_size);
    i = 0;
    while(i < s->ext_size)
      {
      s->ext_data[i] = hex_to_byte(var);
      var += 2;
      i++;
      }
    }
  
  sp->process = process_mpeg4_generic_audio;
  
  return 1;
  }

static int process_h264(bgav_stream_t * s, rtp_header_t * h, uint8_t * data, int len)
  {
  return 0;
  }

static int init_h264(bgav_stream_t * s)
  {
  return 0;
  }

static int process_mpa(bgav_stream_t * s, rtp_header_t * h, uint8_t * data, int len)
  {
  rtp_stream_priv_t * sp;
  sp = s->priv;
  
  }

static int init_mpa(bgav_stream_t * s)
  {
  rtp_stream_priv_t * sp;
  sp = s->priv;
  sp->process = process_mpa;
  }

