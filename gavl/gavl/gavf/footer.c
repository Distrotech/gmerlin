
#include <string.h>

#include <gavfprivate.h>

/* Footer structure
 *
 * GAVFFOOT
 * stream footer * num_streams
 * GAVFFOOT
 * Relative start position (64 bit)
 */


int gavf_footer_check(gavf_t * g)
  {
  uint8_t buf[8];
  int64_t last_pos;
  uint64_t footer_start_pos;
  int i;
  gavf_stream_header_t * s;
  char sig[8];
  int ret = 0;

  gavf_footer_init(g);
  
  if(!g->io->seek_func)
    return 0;
  
  last_pos = g->io->position;

  /* Read last 16 bytes */
  gavf_io_seek(g->io, -16, SEEK_END);
  if((gavf_io_read_data(g->io, buf, 8) < 8))
    goto end;

  if(memcmp(buf, GAVF_TAG_FOOTER, 8))
    goto end;
    
  if(!gavf_io_read_uint64f(g->io, &footer_start_pos))
    goto end;
  
  /* Seek to footer start */
  gavf_io_seek(g->io, footer_start_pos, SEEK_SET);
  if((gavf_io_read_data(g->io, buf, 8) < 8) ||
     memcmp(buf, GAVF_TAG_FOOTER, 8))
    goto end;

  for(i = 0; i < g->ph.num_streams; i++)
    {
    s = g->ph.streams + i;

    if(!gavf_io_read_uint32v(g->io, &s->foot.size_min) ||
       !gavf_io_read_uint32v(g->io, &s->foot.size_max) ||
       !gavf_io_read_int64v(g->io, &s->foot.duration_min) ||
       !gavf_io_read_int64v(g->io, &s->foot.duration_max) ||
       !gavf_io_read_int64v(g->io, &s->foot.pts_start) ||
       !gavf_io_read_int64v(g->io, &s->foot.pts_end))
      goto end;

    /* Set some useful values from the footer */
    gavf_stream_header_apply_footer(s);
    }

  /* Read remaining stuff */
  ret = 1;

  while(1)
    {
    if(gavf_io_read_data(g->io, (uint8_t*)sig, 8) < 8)
      return 0;

    if(!strncmp(sig, GAVF_TAG_SYNC_INDEX, 8))
      {
      if(gavf_sync_index_read(g->io, &g->si))
        g->opt.flags |= GAVF_OPT_FLAG_SYNC_INDEX;
      }
    else if(!strncmp(sig, GAVF_TAG_PACKET_INDEX, 8))
      {
      if(gavf_packet_index_read(g->io, &g->pi))
        g->opt.flags |= GAVF_OPT_FLAG_PACKET_INDEX;
      }
    else if(!strncmp(sig, GAVF_TAG_CHAPTER_LIST, 8))
      {
      if(!(g->cl = gavf_read_chapter_list(g->io)))
        return 0;
      }
    }
  
  end:
    
  gavf_io_seek(g->io, last_pos, SEEK_SET);
  return ret;
    
  }

void gavf_footer_init(gavf_t * g)
  {
  int i;
  gavf_stream_header_t * s;

  for(i = 0; i < g->ph.num_streams; i++)
    {
    s = g->ph.streams + i;
    
    /* Initialize footer */
    s->foot.duration_min = GAVL_TIME_UNDEFINED;
    s->foot.duration_max = GAVL_TIME_UNDEFINED;
    s->foot.pts_start    = GAVL_TIME_UNDEFINED;
    s->foot.pts_end      = GAVL_TIME_UNDEFINED;
    }
  }

int gavf_footer_write(gavf_t * g)
  {
  int i;
  gavf_stream_header_t * s;
  
  uint64_t footer_start_pos = g->io->position;
  if(gavf_io_write_data(g->io, (uint8_t*)GAVF_TAG_FOOTER, 8) < 8)
    return 0;

  for(i = 0; i < g->ph.num_streams; i++)
    {
    s = g->ph.streams + i;

    if(!gavf_io_write_uint32v(g->io, s->foot.size_min) ||
       !gavf_io_write_uint32v(g->io, s->foot.size_max) ||
       !gavf_io_write_int64v(g->io, s->foot.duration_min) ||
       !gavf_io_write_int64v(g->io, s->foot.duration_max) ||
       !gavf_io_write_int64v(g->io, s->foot.pts_start) ||
       !gavf_io_write_int64v(g->io, s->foot.pts_end))
      return 0;
    }

  /* Write indices */  
  if(g->opt.flags & GAVF_OPT_FLAG_SYNC_INDEX)
    {
    if(g->opt.flags & GAVF_OPT_FLAG_DUMP_INDICES)
      gavf_sync_index_dump(&g->si);
    if(!gavf_sync_index_write(g->io, &g->si))
      return 0;
    }
    
  if(g->opt.flags & GAVF_OPT_FLAG_PACKET_INDEX)
    {
    if(g->opt.flags & GAVF_OPT_FLAG_DUMP_INDICES)
      gavf_packet_index_dump(&g->pi);
    if(!gavf_packet_index_write(g->io, &g->pi))
      return 0;
    }

  if(g->cl)
    {
    if(!gavf_write_chapter_list(g->io, g->cl))
      return 0;
    }
  
  /* Write final tag */
  if((gavf_io_write_data(g->io, (uint8_t*)GAVF_TAG_FOOTER, 8) < 8) ||
     !gavf_io_write_uint64f(g->io, footer_start_pos))
    return 0;
  
  return 1;
  }
