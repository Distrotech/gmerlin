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
#include <unistd.h>
#include <netdb.h>


#include <avdec_private.h>
// #include <inttypes.h>
#include <rtp.h>

// #define FOURCC_UNKNOWN BGAV_MK_FOURCC('?','?','?','?')
#define LOG_DOMAIN "rtp"

/* Stream specific init and parse functions are at the end of the file */

/* mpeg4-generic */
static int init_mpeg4_generic_audio(bgav_stream_t * s);

/* H264 */
static int init_h264(bgav_stream_t * s);

static int init_mpa(bgav_stream_t * s);
static int init_mpv(bgav_stream_t * s);


static int init_mp4v_es(bgav_stream_t * s);

static int init_h263_1998(bgav_stream_t * s);

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

  if(!bgav_input_read_32_be(ctx, &h))
    return 0;
  ret->timestamp = h;
  if(!bgav_input_read_32_be(ctx, &ret->ssrc))
    return 0;
  for(i = 0; i < ret->csrc_count; i++)
    {
    if(!bgav_input_read_32_be(ctx, &ret->csrc_list[i]))
      return 0;
    }
  return 1;
  }

#if 0
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
  bgav_dprintf("  timestamp:    %"PRId64"\n", h->timestamp);
  for(i = 0; i < h->csrc_count; i++)
    {
    bgav_dprintf("  csrc[%d]:    %d\n", i, h->csrc_list[i]);
    }
  }
#endif

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
    { 32, BGAV_MK_FOURCC('m','p','g','v'), 0, 90000, init_mpv },
    // 33         MP2T          AV             90000               [RFC2250]
    // 34         H263          V              90000               [Zhu]
    { 34, BGAV_MK_FOURCC('h','2','6','3'), 0, 90000 },
    { -1, },
  };

static const dynamic_payload_t dynamic_video_payloads[] =
  {
    { "H264", BGAV_MK_FOURCC('h', '2', '6', '4'), init_h264 },
    { "MP4V-ES", BGAV_MK_FOURCC('m', 'p', '4', 'v'), init_mp4v_es },
    { "H263-1998", BGAV_MK_FOURCC('h', '2', '6', '3'), init_h263_1998 },
    { "H263-2000", BGAV_MK_FOURCC('h', '2', '6', '3'), init_h263_1998 },
    { },
  };

static const dynamic_payload_t dynamic_audio_payloads[] =
  {
    { "mpeg4-generic", BGAV_MK_FOURCC('m', 'p', '4', 'a'), init_mpeg4_generic_audio },
    { },
  };

typedef struct
  {
  struct pollfd * pollfds;
  int num_pollfds;

  bgav_input_context_t * input_mem;
  int tcp;
  } rtp_priv_t;


static void cleanup_stream_rtp(bgav_stream_t * s)
  {
  rtp_stream_priv_t * priv = s->priv;
  
  if(s->ext_data)
    free(s->ext_data);

  if(!priv) return;

  if(priv->rtp_fd >= 0)
    close(priv->rtp_fd);
  if(priv->rtcp_fd >= 0)
    close(priv->rtcp_fd);
  
  if(priv->control_url) free(priv->control_url);
  if(priv->fmtp) bgav_stringbreak_free(priv->fmtp);
  if(priv->buf) bgav_rtp_packet_buffer_destroy(priv->buf);
  if(priv->stats.timer) gavl_timer_destroy(priv->stats.timer);
  if(priv->rtcp_addr) freeaddrinfo(priv->rtcp_addr);
  if(priv->free_priv)
    priv->free_priv(priv);
  free(priv);
  }

static void check_dynamic(bgav_stream_t * s,
                          const dynamic_payload_t * dynamic_payloads,
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
       !strncasecmp(dynamic_payloads[i].name, pos1, pos2 - pos1))
      {
      s->fourcc = dynamic_payloads[i].fourcc;
      if(dynamic_payloads[i].init)
        if(!dynamic_payloads[i].init(s))
          {
          /* Make the stream undecodable */
          s->fourcc = 0;
          return;
          }
      break;
      }
    i++;
    }
  }

static int find_codec(bgav_stream_t * s, bgav_sdp_media_desc_t * md,
                      int format_index)
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
        if(static_payloads[i].init)
          {
          if(!static_payloads[i].init(s))
            s->fourcc = 0;
          }
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
  //  int i;
  s->cleanup = cleanup_stream_rtp;
  //  fprintf(stderr, "Got stream (format: %s)\n", md->formats[format_index]);

  sp = calloc(1, sizeof(*sp));
  s->priv = sp;
  sp->stats.timer = gavl_timer_create();
  sp->client_ssrc = rand();
  if(!find_codec(s, md, format_index))
    return 0;
  
  //  fprintf(stderr, "fourcc: ");
  //  bgav_dump_fourcc(s->fourcc);
  //  fprintf(stderr, " timescale: %d\n", s->timescale);
  
  sp->buf = bgav_rtp_packet_buffer_create(s->opt, &sp->stats, s->timescale);
  
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
  // fprintf(stderr, "Control: %s\n", sp->control_url);
  return 1;
  }

static int read_rtp_packet(bgav_demuxer_context_t * ctx,
                           bgav_stream_t * s, int len)
  {
  rtp_packet_t * p;
  rtp_stream_priv_t * sp;
  int bytes_read;
  rtp_priv_t * priv;
  priv = ctx->priv;
  
  sp = s->priv;
  /* Read packet */
  p = bgav_rtp_packet_buffer_get_write(sp->buf);

  if(!len)
    {
    bytes_read = bgav_udp_read(sp->rtp_fd,
                               p->buffer, RTP_MAX_PACKET_LENGTH);
    }
  else
    {
    if(len > RTP_MAX_PACKET_LENGTH)
      return 0;
    if(bgav_input_read_data(ctx->input, p->buffer, len) < len)
      return 0;
    bytes_read = len;
    }
  
  if(s->action != BGAV_STREAM_DECODE)
    return 1;
  
  bgav_input_reopen_memory(priv->input_mem, p->buffer, bytes_read);
  
  if(!rtp_header_read(priv->input_mem, &p->h))
    return 0;

  p->buf = p->buffer + priv->input_mem->position;
  p->len = bytes_read - priv->input_mem->position;

  /* Handle padding */
  if(p->h.padding)
    p->len -= p->buf[p->len-1];
  
  p->h.timestamp -= sp->first_rtptime;

  //  if(s->type == BGAV_STREAM_VIDEO)
  //    fprintf(stderr, "Write: %d %08x\n", p->h.sequence_number, p->h.ssrc);

  bgav_rtp_packet_buffer_done_write(sp->buf, p);

  /* Flush packet buffer */
  while((p = bgav_rtp_packet_buffer_get_read(sp->buf)))
    {
    //    if(s->type == BGAV_STREAM_VIDEO)
    //      fprintf(stderr, "Read: %d %08x\n", p->h.sequence_number, p->h.ssrc);

    if(sp->process)
      sp->process(s, &p->h, p->buf, p->len);
    bgav_rtp_packet_buffer_done_read(sp->buf, p);
    }
  return 1;
  }

static int read_rtcp_packet(bgav_demuxer_context_t * ctx,
                             bgav_stream_t * s, int len)
  {
  rtp_priv_t * priv;

  rtp_stream_priv_t * sp;
  int bytes_read;
  rtcp_sr_t rr;
  uint8_t buf[RTP_MAX_PACKET_LENGTH];
  sp = s->priv;
  priv = ctx->priv;
  /* Read packet */

  if(!len)
    {
    bytes_read = bgav_udp_read(sp->rtcp_fd,
                               buf, RTP_MAX_PACKET_LENGTH);
    }
  else
    {
    if(len > RTP_MAX_PACKET_LENGTH)
      return 0;
    if(bgav_input_read_data(ctx->input, buf, len) < len)
      return 0;
    bytes_read = len;
    }
  
  // fprintf(stderr, "Got Audio RTCP Data: %d bytes\n", bytes_read);
      
  if(buf[1] == 200)
    {
    bgav_input_reopen_memory(priv->input_mem, buf, bytes_read);
    if(!bgav_rtcp_sr_read(priv->input_mem, &rr))
      return 0;

    //    bgav_rtcp_sr_dump(&rr);
    sp->lsr = (rr.ntp_time >> 16) && 0xFFFFFFFF;
    sp->sr_count++;

    /* TODO: The standard has more sophisticated formulas
     * for calculating the interval for receiver reports.
     * Here we assume a constant fraction of the number
     * (not bandwidth!) of the sender reports
     */
    
    if((sp->sr_count >= 5) && !len)
      {
      sp->sr_count = 0;
      memset(&rr, 0, sizeof(rr));
      /* Send seceiver report */
      bgav_rtcp_rr_setup(s, &rr);
      bytes_read = bgav_rtcp_rr_write(&rr, buf);
      if(sendto(sp->rtcp_fd, buf, bytes_read, 0,
                sp->rtcp_addr->ai_addr, sp->rtcp_addr->ai_addrlen) < bytes_read)
        fprintf(stderr, "Sending receiver report failed\n");
      //      else
      //        fprintf(stderr, "Sent receiver report\n");
      }
    }
  return 1;
  }

static void init_pollfds(bgav_demuxer_context_t * ctx)
  {
  rtp_priv_t * priv;
  int i, index;
  rtp_stream_priv_t * sp;
  priv = ctx->priv;

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

static void handle_pollfds(bgav_demuxer_context_t * ctx)
  {
  //  int bytes_read;
  rtp_priv_t * priv;
  int i, index;
  rtp_stream_priv_t * sp;
  bgav_stream_t * s;
  priv = ctx->priv;

  index = 0;
    
  for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
    {
    s = &ctx->tt->cur->audio_streams[i];
    sp = s->priv;
    if(priv->pollfds[index].revents & POLLIN)
      {
      read_rtp_packet(ctx, s, 0);
      }
    index++;
    if(priv->pollfds[index].revents & POLLIN)
      {
      /* Read Audio RTCP Data */
      read_rtcp_packet(ctx, s, 0);
      }
    index++;
    }
  for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
    {
    s = &ctx->tt->cur->video_streams[i];
    sp = s->priv;
    if(priv->pollfds[index].revents & POLLIN)
      {
      read_rtp_packet(ctx, s, 0);
      }
    index++;
    if(priv->pollfds[index].revents & POLLIN)
      {
      /* Read Video RTCP Data */
      read_rtcp_packet(ctx, s, 0);
      }
    index++;
    }
  }

static int read_tcp_packet(bgav_demuxer_context_t * ctx,
                           int channel, int len)
  {
  rtp_priv_t * priv;
  int i;
  rtp_stream_priv_t * sp;
  bgav_stream_t * s;
  priv = ctx->priv;
  
  for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
    {
    s = &ctx->tt->cur->audio_streams[i];
    sp = s->priv;
    if(sp->interleave_base == channel)
      return read_rtp_packet(ctx, s, len);
    else if(sp->interleave_base+1 == channel)
      return read_rtcp_packet(ctx, s, len);
    }
  for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
    {
    s = &ctx->tt->cur->video_streams[i];
    sp = s->priv;
    if(sp->interleave_base == channel)
      return read_rtp_packet(ctx, s, len);
    else if(sp->interleave_base+1 == channel)
      return read_rtcp_packet(ctx, s, len);
    }

  /* No stream found: Skip and return 1 */
  
  bgav_input_skip(ctx->input, len);
  return 1;
  }
                           

static int next_packet_rtp(bgav_demuxer_context_t * ctx)
  {
  int ret = 0;
  rtp_priv_t * priv;

  priv = (rtp_priv_t *)ctx->priv;

  if(priv->tcp)
    {
    uint8_t c = 0;
    uint16_t len;
    
    while(c != '$')
      {
      if(!bgav_input_read_8(ctx->input, &c))
        return 0;
      }
    if(!bgav_input_read_8(ctx->input, &c) ||
       !bgav_input_read_16_be(ctx->input, &len))
      return 0;
    
    /* Process packet */
    return read_tcp_packet(ctx, c, len);
    }
  else
    {
    if(!priv->pollfds)
      init_pollfds(ctx);
    
    if(poll(priv->pollfds, priv->num_pollfds, ctx->opt->read_timeout) > 0)
      {
      handle_pollfds(ctx);
      ret = 1;
      }
    if(ret)
      {
      while(poll(priv->pollfds, priv->num_pollfds, 0) > 0)
        handle_pollfds(ctx);
      }
    }
  return ret;
  }

static void close_rtp(bgav_demuxer_context_t * ctx)
  {
  rtp_priv_t * priv = ctx->priv;
  if(!priv)
    return;
  
  if(priv->input_mem)
    {
    bgav_input_close(priv->input_mem);
    bgav_input_destroy(priv->input_mem);
    }
  if(priv->pollfds)
    free(priv->pollfds);

  
  free(priv);
  }

void bgav_rtp_set_tcp(bgav_demuxer_context_t * ctx)
  {
  rtp_priv_t * priv = ctx->priv;
  if(!priv)
    return;
  priv->tcp = 1;
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


/* mpeg-4 AU parsing */

static int mpeg4_au_read(rtp_stream_priv_t * sp,
                         bgav_bitstream_t * b,
                         mpeg4_au_t * ret)
  {
  if(!bgav_bitstream_get(b, &ret->size, sp->priv.mpeg4_generic.sizelength) ||
     !bgav_bitstream_get(b, &ret->delta, sp->priv.mpeg4_generic.indexlength))
    {
    return 0;
    }
  return sp->priv.mpeg4_generic.sizelength +
    sp->priv.mpeg4_generic.indexlength;
  }

static int mpeg4_aus_read(bgav_stream_t * s,
                          uint8_t * data, int len)
  {
  bgav_bitstream_t bs;
  rtp_stream_priv_t * sp;
  int total_bits;

  //  fprintf(stderr, "mpeg4_aus_read: ");
  //  bgav_hexdump(data, 16, 16);
  
  total_bits = BGAV_PTR_2_16BE(data);data+=2;
  
  sp = s->priv;
  sp->priv.mpeg4_generic.num_aus = 0;
  bgav_bitstream_init(&bs, data, (total_bits+7)/8);
  while(bgav_bitstream_get_bits(&bs) >= 8)
    {
    if(sp->priv.mpeg4_generic.aus_alloc < sp->priv.mpeg4_generic.num_aus + 1)
      {
      sp->priv.mpeg4_generic.aus_alloc += 10;
      sp->priv.mpeg4_generic.aus =
        realloc(sp->priv.mpeg4_generic.aus,
                sp->priv.mpeg4_generic.aus_alloc *
                sizeof(*sp->priv.mpeg4_generic.aus));
      memset(sp->priv.mpeg4_generic.aus +
             sp->priv.mpeg4_generic.num_aus, 0,
             10 * sizeof(*sp->priv.mpeg4_generic.aus));
      }
    if(!mpeg4_au_read(sp, &bs,
                      &sp->priv.mpeg4_generic.aus[sp->priv.mpeg4_generic.num_aus]))
      return 0;
    sp->priv.mpeg4_generic.num_aus++;
    }
  return (total_bits+7)/8 + 2;
  }

#if 0
static void dump_aus(mpeg4_au_t * aus, int num)
  {
  int i;
  fprintf(stderr, "Access units: %d\n", num);
  for(i = 0; i < num; i++)
    {
    fprintf(stderr, "  AU %d, size: %d, delta: %d\n", i, aus[i].size, aus[i].delta);
    }
  }
#endif

static int
process_aac(bgav_stream_t * s, rtp_header_t * h,
            uint8_t * data, int len)
  {
  rtp_stream_priv_t * sp;
  int aus_len;
  sp = s->priv;

  aus_len = mpeg4_aus_read(s, data, len);
  if(!aus_len)
    return 0;

  data += aus_len;
  //  dump_aus(sp->priv.mpeg4_generic.aus,
  //           sp->priv.mpeg4_generic.num_aus);
  
  if(sp->priv.mpeg4_generic.num_aus == 1)
    {
    /* Single AU: Can be one fragment or one packet */
    if(s->packet && (s->packet->pts != h->timestamp))
      {
      bgav_packet_done_write(s->packet);
      s->packet = (bgav_packet_t*)0;
      }
    if(!s->packet)
      {
      s->packet = bgav_stream_get_packet_write(s);
      s->packet->data_size = 0;
      s->packet->pts = h->timestamp;
      }
    bgav_packet_alloc(s->packet, s->packet->data_size +
                      sp->priv.mpeg4_generic.aus[0].size);
    memcpy(s->packet->data + s->packet->data_size,
           data, sp->priv.mpeg4_generic.aus[0].size);
    s->packet->data_size += sp->priv.mpeg4_generic.aus[0].size;
    }
  return 0;
  }

static void cleanup_mpeg4_generic_audio(rtp_stream_priv_t * priv)
  {
  if(priv->priv.mpeg4_generic.aus)
    free(priv->priv.mpeg4_generic.aus);
  }

static int init_mpeg4_generic_audio(bgav_stream_t * s)
  {
  rtp_stream_priv_t * sp;
  char * var;

  sp = s->priv;
  if(!sp->fmtp)
    return 0;
  sp->free_priv = cleanup_mpeg4_generic_audio;
  
  var = find_fmtp(sp->fmtp, "mode");
  if(!var)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "No audio mode for mpeg4-generic");
    return 0;
    }
  if(!strcasecmp(var, "AAC-hbr"))
    {
    sp->process = process_aac;
    }
  else if(!strcasecmp(var, "AAC-lbr"))
    {
    sp->process = process_aac;
    }
  else
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Unknown audio mode for mpeg4-generic: %s", var);
    return 0;
    }

  var = find_fmtp(sp->fmtp, "maxDisplacement");
  if(var && atoi(var))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Interleaved audio not yet supported for mpeg4-generic");
    return 0;
    }
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
  return 1;
  }

static void send_nal(bgav_stream_t * s, uint8_t * nal, int len,
                     int64_t time)
  {
  uint8_t start_sequence[]= { 0, 0, 1 };
  if(s->packet && (s->packet->pts != time))
    {
    bgav_packet_done_write(s->packet);
    s->packet = (bgav_packet_t*)0;
    }
  
  if(!s->packet)
    {
    s->packet = bgav_stream_get_packet_write(s);
    s->packet->data_size = 0;
    s->packet->pts = time;
    }
  
  bgav_packet_alloc(s->packet, s->packet->data_size + sizeof(start_sequence) + len);
  memcpy(s->packet->data + s->packet->data_size, start_sequence, sizeof(start_sequence));
  s->packet->data_size += sizeof(start_sequence);

  memcpy(s->packet->data + s->packet->data_size, nal, len);
  s->packet->data_size += len;
  
  }

static void append_nal(bgav_stream_t * s, uint8_t * nal, int len)
  {
  if(!s->packet)
    return;
  bgav_packet_alloc(s->packet, s->packet->data_size + len);
  memcpy(s->packet->data + s->packet->data_size, nal, len);
  s->packet->data_size += len;
  }


static int process_h264(bgav_stream_t * s, rtp_header_t * h, uint8_t * data, int len)
  {
  uint8_t global_nal = (*data & 0x1f);

  if(global_nal >= 1 && global_nal <= 23)
    {
    /* One packet one nal */
    send_nal(s, data, len, h->timestamp);
    return 1;
    }

  switch(global_nal)
    {
    case 24: // STAP-A (one packet, multiple nals)
      //      fprintf(stderr, "STAP-A (one packet, multiple nals)\n");
      data++;
      len--;

      while(len > 2)
        {
        uint16_t nal_size;
        nal_size = BGAV_PTR_2_16BE(data);data+=2;
        send_nal(s, data, nal_size, h->timestamp);
        len -= (2 + nal_size);
        }
      break;
    case 28: // FU-A (fragmented nal)
      {
      uint8_t nal_header;
      //      fprintf(stderr, "FU-A (fragmented nal)\n");
      
      nal_header = *data & 0xe0;
      data++;
      len--;                  // skip the fu_indicator
      if(*data >> 7) /* Start bit set */
        {
        nal_header |= (*data & 0x1f);
        send_nal(s, &nal_header, 1, h->timestamp);
        }
      data++;
      len--;                  // skip the fu_indicator
      append_nal(s, data, len);
      }
      break;
    default:
      fprintf(stderr, "Unsupported NAL type: %d\n",
              global_nal);
      break;
    }
  
  return 1;
  }

static int init_h264(bgav_stream_t * s)
  {
  rtp_stream_priv_t * sp;
  char * value;
  uint8_t start_sequence[]= { 0, 0, 1 };
  sp = s->priv;

  /* Get extradata from the sprop-parameter-sets */
  value = find_fmtp(sp->fmtp, "sprop-parameter-sets");
  if(!value)
    return 0;

  while(*value)
    {
    char base64packet[1024];
    uint8_t decoded_packet[1024];
    uint32_t packet_size;
    char *dst = base64packet;
    int len = 0;
    
    while (*value && *value != ','
           && (dst - base64packet) < sizeof(base64packet) - 1)
      {
      *dst++ = *value++;
      len++;
      }
    *dst++ = '\0';
    
    if (*value == ',')
      value++;

    packet_size=bgav_base64decode(base64packet,
                                  len,
                                  decoded_packet, sizeof(decoded_packet));
    if(packet_size)
      {
      s->ext_data = realloc(s->ext_data,
                            s->ext_size + sizeof(start_sequence) + packet_size);
      memcpy(s->ext_data + s->ext_size, start_sequence, sizeof(start_sequence));
      s->ext_size += sizeof(start_sequence);
      memcpy(s->ext_data + s->ext_size, decoded_packet, packet_size);

      s->ext_size += packet_size;
      
      }
    }
  // fprintf(stderr, "Got H.264 extradata %d bytes\n", s->ext_size);
  // bgav_hexdump(s->ext_data, s->ext_size, 16);

  /* TODO: Get packetization-mode */
  s->data.video.frametime_mode = BGAV_FRAMETIME_PTS;
  s->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
  sp->process = process_h264;
  return 1;
  }

/* MPEG Audio */

static int
process_mpa(bgav_stream_t * s, rtp_header_t * h, uint8_t * data, int len)
  {
  rtp_stream_priv_t * sp;
  bgav_packet_t * p;
  sp = s->priv;
  
  p = bgav_stream_get_packet_write(s);
  bgav_packet_alloc(p, len - 4);

  p->keyframe = 1;
  p->pts      = h->timestamp;
  memcpy(p->data, data + 4, len - 4);
  p->data_size = len - 4;
  bgav_packet_done_write(p);
  return 1;
  }

static int init_mpa(bgav_stream_t * s)
  {
  rtp_stream_priv_t * sp;
  sp = s->priv;
  sp->process = process_mpa;
  return 1;
  }

/* MPEG Video */

static int
process_mpv(bgav_stream_t * s, rtp_header_t * h, uint8_t * data, int len)
  {
  rtp_stream_priv_t * sp;
  bgav_packet_t * p;
  uint32_t i;
  sp = s->priv;
  
  p = bgav_stream_get_packet_write(s);

  i = BGAV_PTR_2_32BE(data);
  if(i & (1 << 26)) /* MPEG-2 */
    {
    data += 8;
    len -= 8;
    }
  else
    {
    data += 4;
    len -= 4;
    }
  
  bgav_packet_alloc(p, len);
  
  p->pts      = h->timestamp;
  memcpy(p->data, data, len);
  p->data_size = len;
  bgav_packet_done_write(p);
  return 1;
  }

static int init_mpv(bgav_stream_t * s)
  {
  rtp_stream_priv_t * sp;
  sp = s->priv;
  sp->process = process_mpv;
  return 1;
  }

/* MP4V-ES */

static int process_mp4v_es(bgav_stream_t * s,
                           rtp_header_t * h, uint8_t * data, int len)
  {
#if 1
  if(!h->marker) /* No frame ends here */
    {
    if(!s->packet)
      {
      s->packet = bgav_stream_get_packet_write(s);
      s->packet->data_size = 0;
      s->packet->pts = h->timestamp;
      }
    bgav_packet_alloc(s->packet, s->packet->data_size + len);
    memcpy(s->packet->data + s->packet->data_size, data, len);
    s->packet->data_size += len;
    }
  else
    {
    if(s->packet)
      {
      bgav_packet_alloc(s->packet, s->packet->data_size + len);
      memcpy(s->packet->data + s->packet->data_size, data, len);
      s->packet->data_size += len;
      //      bgav_hexdump(s->packet->data, 16, 16);
      bgav_packet_done_write(s->packet);
      s->packet = (bgav_packet_t*)0;
      }
    else
      {
      bgav_packet_t * p;
      p = bgav_stream_get_packet_write(s);
      bgav_packet_alloc(p, len);
      memcpy(p->data, data, len);
      //      bgav_hexdump(p->data, 16, 16);
      p->data_size = len;
      bgav_packet_done_write(p);
      }
    }
#else
  bgav_packet_t * p;
  p = bgav_stream_get_packet_write(s);
  bgav_packet_alloc(p, len);
  memcpy(p->data, data, len);
  //  bgav_hexdump(p->data, 16, 16);
  p->data_size = len;
  p->pts = h->timestamp;
  bgav_packet_done_write(p);
#endif
  
  return 1;
  }

static int init_mp4v_es(bgav_stream_t * s)
  {
  char * value;
  rtp_stream_priv_t * sp;
  int i;

  sp = s->priv;
  
  value = find_fmtp(sp->fmtp, "config");
  if(!value)
    return 0;

  s->ext_size = strlen(value)/2;
  s->ext_data = malloc(s->ext_size);
  //  s->not_aligned = 1;
  i = 0;
  while(i < s->ext_size)
    {
    s->ext_data[i] = hex_to_byte(value);
    value += 2;
    i++;
    }
  sp->process = process_mp4v_es;
  return 1;
  }

/* H263-1998 */

static int process_h263_1998(bgav_stream_t * s,
                             rtp_header_t * h, uint8_t * data, int len)
  {
  rtp_stream_priv_t * sp;
  int p_bit;
  int skip = 2;

  //  fprintf(stderr, "process_h263_1998: %d\n", len);
  
  sp = s->priv;

  p_bit = !!(data[0] & 0x4);
  
  if(!s->packet)
    {
    // incomplete packet without initial fragment
    if(!p_bit)
      return 1;
    s->packet = bgav_stream_get_packet_write(s);
    s->packet->data_size = 0;
    s->packet->pts = h->timestamp;
    }

  /* Get length */
  if(data[0]&0x2) // v bit - skip one more
    ++skip;

  skip += (data[1]>>3)|((data[0]&0x1)<<5); // plen - skip that many bytes

  data += skip;
  len -= skip;

  if(p_bit)
    {
    bgav_packet_alloc(s->packet, s->packet->data_size + len + 2);
    /* Make startcodes complete */
    s->packet->data[s->packet->data_size]   = 0;
    s->packet->data[s->packet->data_size+1] = 0;
    s->packet->data_size += 2;
    }
  else
    bgav_packet_alloc(s->packet, s->packet->data_size + len);
  
  memcpy(s->packet->data + s->packet->data_size, data, len);
  //  bgav_hexdump(p->data, 16, 16);
  s->packet->data_size += len;
  
  if(h->marker)
    {
    bgav_packet_done_write(s->packet);
    s->packet = (bgav_packet_t*)0;
    }
  return 1;
  }

static int init_h263_1998(bgav_stream_t * s)
  {
  rtp_stream_priv_t * sp;
  sp = s->priv;

  sp->process = process_h263_1998;
  return 1;
  }
