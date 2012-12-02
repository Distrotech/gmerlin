#include <gavfprivate.h>

/* Packet source */

static gavl_source_status_t
read_packet_func_nobuffer(void * priv, gavl_packet_t ** p)
  {
  gavf_stream_t * s = priv;

  if(!s->g->have_pkt_header && !gavf_packet_read_header(s->g))
    return GAVL_SOURCE_EOF;

  if(s->g->pkthdr.stream_id != s->h->id)
    return GAVL_SOURCE_AGAIN;
  
  s->g->have_pkt_header = 0;
  
  gavf_buffer_reset(&s->g->pkt_buf);
  
  if(!gavf_read_gavl_packet(s->g->io, s, *p))
    return GAVL_SOURCE_EOF;
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t
read_packet_func_buffer_cont(void * priv, gavl_packet_t ** p)
  {
  int i;
  gavl_packet_t * read_packet;
  gavf_stream_t * read_stream;

  gavf_stream_t * s = priv;

  while(!(*p = gavf_packet_buffer_get_read(s->pb)))
    {
    /* Read header */
    if(!s->g->have_pkt_header && !gavf_packet_read_header(s->g))
      return GAVL_SOURCE_EOF;

    for(i = 0; i < s->g->ph.num_streams; i++)
      {
      if(s->g->pkthdr.stream_id != s->g->ph.streams[i].id)
        {
        read_stream = &s->g->streams[i];
        read_packet = gavf_packet_buffer_get_write(read_stream->pb);
        gavf_buffer_reset(&s->g->pkt_buf);
        if(!gavf_read_gavl_packet(s->g->io, s, read_packet))
          return GAVL_SOURCE_EOF;
        break;
        }
      }
    s->g->have_pkt_header = 0;
    }
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t
read_packet_func_buffer_discont(void * priv, gavl_packet_t ** p)
  {
  gavf_stream_t * s = priv;

  if((*p = gavf_packet_buffer_get_read(s->pb)))
    return GAVL_SOURCE_OK;
  else if(s->g->eof)
    return GAVL_SOURCE_EOF;
  else
    return GAVL_SOURCE_AGAIN;
  }


void gavf_stream_create_packet_src(gavf_t * g, gavf_stream_t * s,
                                   const gavl_compression_info_t * ci,
                                   const gavl_audio_format_t * afmt,
                                   const gavl_video_format_t * vfmt)
  {
  if(!(g->opt.flags & GAVF_OPT_BUFFER_READ))
    s->psrc = gavl_packet_source_create(read_packet_func_nobuffer, s, 0,
                                        ci, afmt, vfmt);
  else
    {
    if(s->flags & STREAM_FLAG_DISCONTINUOUS)
      s->psrc = gavl_packet_source_create(read_packet_func_buffer_discont, s,
                                          GAVL_SOURCE_SRC_ALLOC,
                                          ci, afmt, vfmt);
    else
      s->psrc = gavl_packet_source_create(read_packet_func_buffer_cont, s,
                                          GAVL_SOURCE_SRC_ALLOC,
                                          ci, afmt, vfmt);
    }
  }

/* Packet sink */

static gavl_sink_status_t
put_packet_func(void * priv, gavl_packet_t * p)
  {
  gavf_stream_t * s = priv;

  /* Update footer */
#if 0
  fprintf(stderr, "put packet %d\n", s->h->id);
  gavl_packet_dump(p);
#endif
  /* Fist packet */
  if(s->h->foot.pts_start == GAVL_TIME_UNDEFINED)
    {
    s->h->foot.pts_start    = p->pts;
    s->h->foot.pts_end      = p->pts + p->duration;
    s->h->foot.duration_min = p->duration;
    s->h->foot.duration_max = p->duration;
    s->h->foot.size_min     = p->data_len;
    s->h->foot.size_max     = p->data_len;
    s->next_sync_pts = p->pts;
    }
  /* Subsequent packets */
  else
    {
    if(s->h->foot.duration_min > p->duration)
      s->h->foot.duration_min = p->duration;
    
    if(s->h->foot.duration_max < p->duration)
      s->h->foot.duration_max = p->duration;

    if(s->h->foot.size_min > p->data_len)
      s->h->foot.size_min = p->data_len;

    if(s->h->foot.size_max > p->data_len)
      s->h->foot.size_max = p->data_len;
    
    if(s->h->foot.pts_end < p->pts + p->duration)
      s->h->foot.pts_end = p->pts + p->duration;
    }

  return gavf_flush_packets(s->g, s);
  
  // return gavf_write_packet(s->g, (int)(s - s->g->streams), p) ?
  //    GAVL_SINK_OK : GAVL_SINK_ERROR;
  }

static gavl_packet_t *
get_packet_func(void * priv)
  {
  gavf_stream_t * s = priv;
  return gavf_packet_buffer_get_write(s->pb);
  }

void gavf_stream_create_packet_sink(gavf_t * g, gavf_stream_t * s)
  {
  /* Create packet sink */
  s->psink = gavl_packet_sink_create(get_packet_func,
                                     put_packet_func, s);
  
  }
