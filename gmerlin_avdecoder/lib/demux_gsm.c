
#include <avdec_private.h>


static int probe_gsm(bgav_input_context_t * input)
  {
  return 0;
  }

static int open_gsm(bgav_demuxer_context_t * ctx,
                   bgav_redirector_context_t ** redir)
  {
  return 0;
  }

static int next_packet_gsm(bgav_demuxer_context_t * ctx)
  {
  return 0;
  }

static void seek_gsm(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  }

static void close_gsm(bgav_demuxer_context_t * ctx)
  {
  }

bgav_demuxer_t bgav_demuxer_gsm =
  {
    probe:       probe_gsm,
    open:        open_gsm,
    next_packet: next_packet_gsm,
    seek:        seek_gsm,
    close:       close_gsm
  };
