/*****************************************************************
 
  demuxer.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <avdec_private.h>

extern bgav_demuxer_t bgav_demuxer_asf;
extern bgav_demuxer_t bgav_demuxer_avi;
extern bgav_demuxer_t bgav_demuxer_rmff;
extern bgav_demuxer_t bgav_demuxer_quicktime;

extern bgav_demuxer_t bgav_demuxer_wav;
extern bgav_demuxer_t bgav_demuxer_au;
extern bgav_demuxer_t bgav_demuxer_aiff;
extern bgav_demuxer_t bgav_demuxer_ra;
extern bgav_demuxer_t bgav_demuxer_mpegaudio;
extern bgav_demuxer_t bgav_demuxer_mpegps;
extern bgav_demuxer_t bgav_demuxer_flac;

#ifdef HAVE_VORBIS
extern bgav_demuxer_t bgav_demuxer_ogg;
#endif

typedef struct
  {
  bgav_demuxer_t * demuxer;
  char * format_name;
  } demuxer_t;

static demuxer_t demuxers[] =
  {
    { &bgav_demuxer_asf, "Microsoft ASF" },
    { &bgav_demuxer_avi, "Microsoft AVI" },
    { &bgav_demuxer_rmff, "Real Media" },
    { &bgav_demuxer_quicktime, "Apple Quicktime" },
    { &bgav_demuxer_wav, "Microsoft WAV" },
    { &bgav_demuxer_au, "AU" },
    { &bgav_demuxer_aiff, "AIFF" },
    { &bgav_demuxer_ra, "Real Audio" },
    { &bgav_demuxer_flac, "FLAC" },
#ifdef HAVE_VORBIS
    { &bgav_demuxer_ogg, "Ogg Bitstream" },
#endif
  };

static demuxer_t sync_demuxers[] =
  {
    { &bgav_demuxer_mpegaudio, "Mpeg Audio" },
    { &bgav_demuxer_mpegps, "Mpeg System" },
  };

static struct
  {
  bgav_demuxer_t * demuxer;
  char * mimetype;
  }
mimetypes[] =
  {
    { &bgav_demuxer_mpegaudio, "audio/mpeg" }
  };

static int num_demuxers = sizeof(demuxers)/sizeof(demuxers[0]);
static int num_sync_demuxers = sizeof(sync_demuxers)/sizeof(sync_demuxers[0]);
static int num_mimetypes = sizeof(mimetypes)/sizeof(mimetypes[0]);

#define SYNC_BYTES 1024

bgav_demuxer_t * bgav_demuxer_probe(bgav_input_context_t * input)
  {
  int i;
  int bytes_skipped;
  uint8_t skip;
    
  //  uint8_t header[32];
  if(input->mimetype)
    {
    for(i = 0; i < num_mimetypes; i++)
      {
      if(!strcmp(mimetypes[i].mimetype, input->mimetype))
        {
        return mimetypes[i].demuxer;
        }
      }
    }
    
  for(i = 0; i < num_demuxers; i++)
    {
    if(demuxers[i].demuxer->probe(input))
      {
      fprintf(stderr, "Detected %s format\n", demuxers[i].format_name);
      return demuxers[i].demuxer;
      }
    }
  
  for(i = 0; i < num_sync_demuxers; i++)
    {
    if(sync_demuxers[i].demuxer->probe(input))
      {
      fprintf(stderr, "Detected %s format\n", sync_demuxers[i].format_name);
      return sync_demuxers[i].demuxer;
      }
    }

  /* Try again with skipping initial bytes */

  bytes_skipped = 0;

  while(bytes_skipped < SYNC_BYTES)
    {
    bytes_skipped++;
    if(!bgav_input_read_data(input, &skip, 1))
      return (bgav_demuxer_t *)0;

    for(i = 0; i < num_sync_demuxers; i++)
      {
      if(sync_demuxers[i].demuxer->probe(input))
        {
        fprintf(stderr, "Detected %s format after skipping %d bytes\n",
                sync_demuxers[i].format_name, bytes_skipped);
        return sync_demuxers[i].demuxer;
        }
      }
    
    }
  
  
  return (bgav_demuxer_t *)0;
  }

bgav_demuxer_context_t *
bgav_demuxer_create(bgav_demuxer_t * demuxer,
                    bgav_input_context_t * input)
  {
  bgav_demuxer_context_t * ret;
  
  ret = calloc(1, sizeof(*ret));
  
  ret->demuxer = demuxer;
  ret->input = input;
    
  return ret;
  }

#define FREE(p) if(p){free(p);p=NULL;}

void bgav_demuxer_destroy(bgav_demuxer_context_t * ctx)
  {
  ctx->demuxer->close(ctx);
  if(ctx->tt)
    bgav_track_table_unref(ctx->tt);
  FREE(ctx->stream_description);
  free(ctx);
  }


int bgav_demuxer_start(bgav_demuxer_context_t * ctx,
                       bgav_redirector_context_t ** redir)
  {
  return ctx->demuxer->open(ctx, redir);
  }

void bgav_demuxer_stop(bgav_demuxer_context_t * ctx)
  {
  ctx->demuxer->close(ctx);
  ctx->priv = NULL;
  FREE(ctx->stream_description);
  }

bgav_packet_t *
bgav_demuxer_get_packet_read(bgav_demuxer_context_t * demuxer,
                             bgav_stream_t * s)
  {
  bgav_packet_t * ret = (bgav_packet_t*)0;
  if(!s->packet_buffer)
    return (bgav_packet_t*)0;
  while(!(ret = bgav_packet_buffer_get_packet_read(s->packet_buffer)))
    {
    if(!demuxer->demuxer->next_packet(demuxer))
      return (bgav_packet_t*)0;
    }
  return ret;
  }

void 
bgav_demuxer_done_packet_read(bgav_demuxer_context_t * demuxer,
                              bgav_packet_t * p)
  {
  p->valid = 0;
  }

/* Maximum allowed seek tolerance, decrease if you want it more exact */

#define SEEK_TOLERANCE (GAVL_TIME_SCALE/2)

void
bgav_seek(bgav_t * b, gavl_time_t time)
  {
  gavl_time_t diff_time;
  gavl_time_t sync_time;
  gavl_time_t seek_time;
  
  bgav_track_t * track = b->tt->current_track;
  int num_iterations = 0;
  seek_time = time;
  
  while(1)
    {
    num_iterations++;
    /* First step: Flush all fifos */
    
    bgav_track_clear(track);
    
    /* Second step: Let the demuxer seek */
    /* This will set the "time" members of all streams */
    
    b->demuxer->demuxer->seek(b->demuxer, seek_time);
   
    sync_time = bgav_track_resync_decoders(track);

    if(sync_time == GAVL_TIME_UNDEFINED)
      return;
    
    diff_time = time - sync_time;
    fprintf(stderr, "Seeked, time: %f sync_time: %f, diff: %f\n",
            gavl_time_to_seconds(time),
            gavl_time_to_seconds(sync_time),
            gavl_time_to_seconds(diff_time));
    
    if(diff_time > 0)
      {
      bgav_track_skipto(track, time);
      break;
      }
    else if(diff_time < -SEEK_TOLERANCE)
      {
      /* We get a bit more back than the error was */
      seek_time += (diff_time * 3)/2;
      if(seek_time < 0)
        seek_time = 0;
      }
    else
      {
      bgav_track_skipto(track, sync_time);
      break;
      }
    }
  //  fprintf(stderr, "Seek done, %d iterations\n", num_iterations);
  }

