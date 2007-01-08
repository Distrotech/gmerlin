/*****************************************************************
 
  in_dvb_linux.c
 
  Copyright (c) 2007 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <avdec_private.h>

#ifdef HAVE_LINUXDVB
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

#include <mpegts_common.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <dvb_channels.h>

#include <stdio.h>

#define LOG_DOMAIN "dvb"

extern bgav_demuxer_t bgav_demuxer_mpegts;

#define FILTER_AUDIO 0
#define FILTER_PAT   0

#define FILTER_VIDEO 1

/* Limitation from demuxer API */
#define MAX_AUDIO_STREAMS 4
#define MAX_VIDEO_STREAMS 4
#define MAX_AC3_STREAMS 1

typedef struct
  {
  int fe_fd;
  int dvr_fd;

  int * filter_fds;
  int num_filter_fds;
  int filter_fds_alloc;
  
  struct dvb_frontend_info fe_info;

  bgav_dvb_channel_info_t * channels;
  int num_channels;

  char * device_directory;
  char * filter_filename;
  char * dvr_filename;
  char * frontend_filename;
  
  } dvb_priv_t;


static void set_num_filters(dvb_priv_t * priv, int num)
  {
  int i;
  for(i = 0; i < priv->num_filter_fds; i++)
    {
    if(priv->filter_fds[i] >= 0)
      close(priv->filter_fds[i]);
    priv->filter_fds[i] = -1;
    }
  if(priv->filter_fds_alloc < num)
    {
    priv->filter_fds_alloc = num;
    priv->filter_fds =
      realloc(priv->filter_fds,
              priv->filter_fds_alloc * sizeof(*priv->filter_fds));
    }
  priv->num_filter_fds = num;
  
  for(i = 0; i < num; i++)
    priv->filter_fds[i] = open(priv->filter_filename, O_RDWR);
  }

#if 0

static struct
  {
  enum fe_caps cap;
  char * name;
  }
dvb_caps[] =
  {
    { FE_CAN_INVERSION_AUTO, "INVERSION_AUTO" },
    { FE_CAN_FEC_1_2,        "FEC_1_2"        },
    { FE_CAN_FEC_2_3,        "FEC_2_3"        },
    { FE_CAN_FEC_3_4,        "FEC_3_4"        },
    { FE_CAN_FEC_4_5,        "FEC_4_5"        },
    { FE_CAN_FEC_5_6,        "FEC_5_6"        },
    { FE_CAN_FEC_6_7,        "FEC_6_7"        },
    { FE_CAN_FEC_7_8,        "FEC_7_8"        }, 
    { FE_CAN_FEC_8_9,        "FEC_8_9"        },
    { FE_CAN_FEC_AUTO,       "FEC_AUTO"       },
    { FE_CAN_QPSK,           "QPSK"           },
    { FE_CAN_QAM_16,         "QAM_16"         },
    { FE_CAN_QAM_32,         "QAM_32"         },
    { FE_CAN_QAM_64,         "QAM_64"         },
    { FE_CAN_QAM_128,        "QAM_128"        },
    { FE_CAN_QAM_256,        "QAM_256"        },
    { FE_CAN_QAM_AUTO,       "QAM_AUTO"       },
    { FE_CAN_TRANSMISSION_MODE_AUTO, "TRANSMISSION_MODE_AUTO" },
    { FE_CAN_BANDWIDTH_AUTO,         "BANDWIDTH_AUTO" },
    { FE_CAN_GUARD_INTERVAL_AUTO,    "GUARD_INTERVAL_AUTO" },
    { FE_CAN_HIERARCHY_AUTO,         "HIERARCHY_AUTO" },
    { FE_CAN_8VSB,                   "CAN_8VSB" },
    { FE_CAN_16VSB,                  "CAN_16VSB" },
    { FE_CAN_RECOVER,                "CAN_RECOVER" },
    { FE_CAN_MUTE_TS,                "CAN_MUTE_TS" },
  };

static void dump_frontend_info(struct dvb_frontend_info * info)
  {
  int i;
  bgav_dprintf("DVB frontend info\n");
  bgav_dprintf("  name: %s\n", info->name);
  bgav_dprintf("  type: ");
  
  switch(info->type)
    {
    case FE_QPSK:
      bgav_dprintf("QPSK (DVB-S)\n");
      break;
    case FE_QAM:
      bgav_dprintf("QAM (DVB-C)\n");
      break;
    case FE_OFDM:
      bgav_dprintf("QFDM (DVB-T)\n");
      break;
    case FE_ATSC:
      bgav_dprintf("ATSC (DBV-A)\n");
      break;
    }
  bgav_dprintf("  Frequency range: [%d..%d] (step: %d, tol: %d)\n",
               info->frequency_min,
               info->frequency_max,
               info->frequency_stepsize,
               info->frequency_tolerance);
  
  bgav_dprintf("  Symbol rate range: [%d..%d] (tol: %d)\n",
               info->symbol_rate_min,
               info->symbol_rate_max,
               info->symbol_rate_tolerance);
  
  bgav_dprintf("  capabilities: ");
  for(i = 0; i < sizeof(dvb_caps)/sizeof(dvb_caps[0]); i++)
    {
    if(info->caps & dvb_caps[i].cap)
      bgav_dprintf("%s ", dvb_caps[i].name);
    }
  bgav_dprintf("\n");
  }

#endif

static int tune_in(bgav_input_context_t * ctx,
                   bgav_dvb_channel_info_t * channel)
  {
  gavl_time_t delay_time;
  fe_status_t status = 0;
  dvb_priv_t * priv;
  struct dvb_frontend_event event;
  struct pollfd pfd[1];
  
  priv = (dvb_priv_t *)(ctx->priv);

  /* TODO: tuner_set_diseqc for satellite tuners */
  
  /* Flush events */
  while (ioctl(priv->fe_fd, FE_GET_EVENT, &event) != -1);
  
  /* Tune to the channel */
  if(ioctl(priv->fe_fd, FE_SET_FRONTEND, &channel->front_param) <0)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Setting channel failed: %s",
             strerror(errno));
    
    return 0;
    }

  /* Wait until the frontend tuned to the channel */
  pfd[0].fd = priv->fe_fd;
  pfd[0].events = POLLIN;

  if(poll(pfd,1,3000))
    {
    if(pfd[0].revents & POLLIN)
      {
      if(ioctl(priv->fe_fd, FE_GET_EVENT, &event) == -EOVERFLOW)
        {
        bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Setting channel failed: Event queue overflow");
        return 0;
        }
      if(event.parameters.frequency <= 0)
        {
        bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Setting frequency failed");
        return 0;
        }
      }
    }

  /* Wait for frontend to be locked */

  delay_time = GAVL_TIME_SCALE / 10;
  
  do{
    status = 0;
    if(ioctl(priv->fe_fd, FE_READ_STATUS, &status) < 0)
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Reading status failed: %s",
               strerror(errno));
      return 0;
      }
    if(status & FE_HAS_LOCK)
      break;
    
    gavl_time_delay(&delay_time);
    bgav_log(ctx->opt, BGAV_LOG_INFO, LOG_DOMAIN, "Waiting for lock");
    } while (!(status & FE_TIMEDOUT));

  if(status & FE_TIMEDOUT)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Locking timed out");
    return 0;
    }

  bgav_log(ctx->opt, BGAV_LOG_INFO, LOG_DOMAIN, "Frontend locked successfully");
  return 1;
  }

static int get_streams(bgav_input_context_t * ctx, bgav_dvb_channel_info_t * channel)
  {
  int i;
  int program_index;
  int bytes_read;
  uint8_t buffer[4096];
  pat_section_t pats;
  pmt_section_t pmts;
  struct dmx_sct_filter_params params;
  dvb_priv_t * priv;
  priv = (dvb_priv_t *)(ctx->priv);
    
  if(channel->initialized)
    return 1;
  
  if(!tune_in(ctx, channel))
    return 0;
  
  /* Get PAT */
  
  set_num_filters(priv, 1);
  
  memset(&params, 0, sizeof(params));

  params.pid              = 0;
  params.filter.filter[0] = 0x00;
  params.filter.mask[0]   = 0xFF;
  params.flags             = DMX_IMMEDIATE_START | DMX_ONESHOT;

  if(ioctl(priv->filter_fds[0],
           DMX_SET_FILTER, &params) < 0)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Setting PAT filter failed: %s", strerror(errno));
    
    return 0;
    }
  bytes_read = read(priv->filter_fds[0], buffer, 4096);
  fprintf(stderr, "Read %d bytes\n", bytes_read);
  bgav_hexdump(buffer, bytes_read, 16);
  
  if(!bgav_pat_section_read(buffer, bytes_read, &pats))
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Parsing PAT failed");
    return 0;
    }
  
  fprintf(stderr, "Got PAT:\n");
  bgav_pat_section_dump(&pats);

  /* For each program, get the PMT */
  
  for(i = 0; i < pats.num_programs; i++)
    {
    program_index = 0;

    if(pats.programs[i].program_number == 0)
      continue;
    
    while(program_index < priv->num_channels)
      {
      if(pats.programs[i].program_number == priv->channels[program_index].service_id)
        break;
      program_index++;
      }
    
    if(program_index == priv->num_channels)
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Cannot find service ID %d in PAT (recreate channels.conf)");
      continue;
      }
    /* Get PMT */
    
    params.pid              = pats.programs[i].program_map_pid;
    params.filter.filter[0] = 0x02;
    params.filter.mask[0]   = 0xFF;
    params.flags             = DMX_IMMEDIATE_START | DMX_ONESHOT;
  
    if(ioctl(priv->filter_fds[0],
             DMX_SET_FILTER, &params) < 0)
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Setting PMT filter failed: %s", strerror(errno));
      return 0;
      }

    bytes_read = read(priv->filter_fds[0],
                      buffer, 4096);
    //    fprintf(stderr, "Read %d bytes\n", bytes_read);
    //    bgav_hexdump(buffer, bytes_read, 16);
  
    if(!bgav_pmt_section_read(buffer, bytes_read, &pmts))
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Parsing PMT failed");
      return 0;
      }
    bgav_pmt_section_dump(&pmts);  

    /* Setup streams from pmt */
    bgav_pmt_section_setup_track(&pmts,
                                 &ctx->tt->tracks[program_index],
                                 ctx->opt,
                                 MAX_AUDIO_STREAMS,
                                 MAX_VIDEO_STREAMS,
                                 MAX_AC3_STREAMS,
                                 &priv->channels[program_index].num_ac3_streams,
                                 &priv->channels[program_index].extra_pcr_pid);
    priv->channels[program_index].pcr_pid = pmts.pcr_pid;
    priv->channels[program_index].initialized = 1;
    }
  
  set_num_filters(priv, 0);
  return 1;
  }


static int open_dvb(bgav_input_context_t * ctx, const char * url)
  {
  int i;
  fe_status_t status;
  char * tmp_string;
  dvb_priv_t * priv;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  priv->device_directory  = bgav_strdup(url);
  priv->filter_filename   = bgav_sprintf("%s/demux0", url);
  priv->dvr_filename      = bgav_sprintf("%s/dvr0", url);
  priv->frontend_filename = bgav_sprintf("%s/frontend0", url);
  
  /* Open frontend */
  priv->fe_fd = open(priv->frontend_filename, O_RDWR | O_NONBLOCK);
  if(priv->fe_fd < 0)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot open frontent device %s\n", tmp_string);
    return 0;
    }
  /* Get frontend info */
  if((ioctl(priv->fe_fd, FE_GET_INFO, &priv->fe_info)) < 0)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot get frontent info\n");
    return 0;
    }
  //  dump_frontend_info(&priv->fe_info);

  /* Get status */
  if((ioctl(priv->fe_fd, FE_READ_STATUS, &status)) < 0)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot get frontend status\n");
    return 0;
    }

  //  bgav_dprintf("Frontend status: 0x%02x\n", status);

  /* Load channels file */
  priv->channels = bgav_dvb_channels_load(ctx->opt, priv->fe_info.type,
                                          &priv->num_channels);

  // Add channels to the tracks

  ctx->tt = bgav_track_table_create(priv->num_channels);
  for(i = 0; i < priv->num_channels; i++)
    {
    ctx->tt->tracks[i].name = bgav_strdup(priv->channels[i].name);
    if(!get_streams(ctx, &priv->channels[i]))
      return 0;
    }
  
  /* Create demuxer */
  
  ctx->demuxer =
    bgav_demuxer_create(ctx->opt, &bgav_demuxer_mpegts, ctx);

  ctx->demuxer->tt = ctx->tt;
  
  priv->dvr_fd = -1;
  
  return 1;
  }

static void close_dvb(bgav_input_context_t * ctx)
  {
  dvb_priv_t * priv;
  priv = (dvb_priv_t *)(ctx->priv);
  
  set_num_filters(priv, 0);
  if(priv->dvr_fd >= 0)
    close(priv->dvr_fd);
  if(priv->fe_fd >= 0)
    close(priv->fe_fd);
  
  if(priv->filter_fds)
    free(priv->filter_fds);
  
  if(priv->device_directory)
    free(priv->device_directory);
  if(priv->frontend_filename)
    free(priv->frontend_filename);
  if(priv->filter_filename)
    free(priv->filter_filename);
  if(priv->dvr_filename)
    free(priv->dvr_filename);
  
  if(priv->channels)
    dvb_channels_destroy(priv->channels, priv->num_channels);
  free(priv);
  }

static int read_dvb(bgav_input_context_t* ctx,
                         uint8_t * buffer, int len)
  {
  dvb_priv_t * priv;
  priv = (dvb_priv_t *)(ctx->priv);
#if 0
  bytes_read = read(priv->dvr_fd, data, 188);

  fprintf(stderr, "Read %d bytes\n", bytes_read);
  bgav_hexdump(data,bytes_read, 16);
#else
  return read(priv->dvr_fd, buffer, len);
#endif  
  //  return 0;
  }

static int setup_pes_filter(const bgav_options_t * opt,
                            int fd, int pes_type, int stream_id)
  {
  struct dmx_pes_filter_params params;
  
  params.pid      = stream_id;
  params.input    = DMX_IN_FRONTEND;
  params.output   = DMX_OUT_TS_TAP;
  params.pes_type = pes_type;
  params.flags    = DMX_IMMEDIATE_START;
  
  if(ioctl(fd, DMX_SET_PES_FILTER, &params) < 0)
    {
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Setting filter failed: %s", strerror(errno));
    return 0;
    }
  return 1;
  }


static int setup_filters(bgav_input_context_t * ctx,
                         bgav_dvb_channel_info_t * channel,
                         bgav_track_t * track)
  {
  dvb_priv_t * priv;
  int filter_index;
  int num_audio_streams;
  int audio_index;
  
  priv = (dvb_priv_t *)(ctx->priv);

  set_num_filters(priv, track->num_audio_streams +
                  track->num_video_streams + channel->extra_pcr_pid);
  
  filter_index = 0;

  num_audio_streams = track->num_audio_streams -
    channel->num_ac3_streams;
  audio_index = 0;
  /* Audio filters */
  if(num_audio_streams >= 1)
    {
    if(channel->num_ac3_streams)
      {
      while(track->audio_streams[audio_index].fourcc ==
            BGAV_MK_FOURCC('.','a','c','3'))
        audio_index++;
      }
    fprintf(stderr, "Set audio filter 1 ID: %d\n",
            track->audio_streams[audio_index].stream_id);
    if(!setup_pes_filter(ctx->opt,
                         priv->filter_fds[filter_index++],
                         DMX_PES_AUDIO0,
                         track->audio_streams[audio_index++].stream_id))
      return 0;
    }
  if(num_audio_streams >= 2)
    {
    if(channel->num_ac3_streams)
      {
      while(track->audio_streams[audio_index].fourcc ==
            BGAV_MK_FOURCC('.','a','c','3'))
        audio_index++;
      }
    fprintf(stderr, "Set audio filter 2 ID: %d\n",
            track->audio_streams[audio_index].stream_id);
    if(!setup_pes_filter(ctx->opt,
                         priv->filter_fds[filter_index++],
                         DMX_PES_AUDIO1,
                         track->audio_streams[audio_index++].stream_id))
      return 0;
    }
  if(num_audio_streams >= 3)
    {
    if(channel->num_ac3_streams)
      {
      while(track->audio_streams[audio_index].fourcc ==
            BGAV_MK_FOURCC('.','a','c','3'))
        audio_index++;
      }
    fprintf(stderr, "Set audio filter 3 ID: %d\n",
            track->audio_streams[audio_index].stream_id);
    if(!setup_pes_filter(ctx->opt,
                         priv->filter_fds[filter_index++],
                         DMX_PES_AUDIO2,
                         track->audio_streams[audio_index++].stream_id))
      return 0;
    }
  if(num_audio_streams >= 4)
    {
    if(channel->num_ac3_streams)
      {
      while(track->audio_streams[audio_index].fourcc ==
            BGAV_MK_FOURCC('.','a','c','3'))
        audio_index++;
      }
    fprintf(stderr, "Set audio filter 4 ID: %d\n",
            track->audio_streams[audio_index].stream_id);
    if(!setup_pes_filter(ctx->opt,
                         priv->filter_fds[filter_index++],
                         DMX_PES_AUDIO3,
                         track->audio_streams[audio_index++].stream_id))
      return 0;
    }

  /* AC3 filter */

  if(channel->num_ac3_streams)
    {
    audio_index = 0;
    while(track->audio_streams[audio_index].fourcc !=
          BGAV_MK_FOURCC('.','a','c','3'))
      audio_index++;

    fprintf(stderr, "Set AC3 filter ID: %d\n",
            track->audio_streams[audio_index].stream_id);
    
    if(!setup_pes_filter(ctx->opt,
                         priv->filter_fds[filter_index++],
                         DMX_PES_OTHER,
                         track->audio_streams[audio_index].stream_id))
      return 0;
    
    }
  
  /* Video filter */

  if(track->num_video_streams >= 1)
    {
    if(!setup_pes_filter(ctx->opt,
                         priv->filter_fds[filter_index++],
                         DMX_PES_VIDEO0,
                         track->video_streams[0].stream_id))
      return 0;
    }
  if(track->num_video_streams >= 2)
    {
    if(!setup_pes_filter(ctx->opt,
                         priv->filter_fds[filter_index++],
                         DMX_PES_VIDEO1,
                         track->video_streams[1].stream_id))
      return 0;
    }
  if(track->num_video_streams >= 3)
    {
    if(!setup_pes_filter(ctx->opt,
                         priv->filter_fds[filter_index++],
                         DMX_PES_VIDEO2,
                         track->video_streams[2].stream_id))
      return 0;
    }
  if(track->num_video_streams >= 4)
    {
    if(!setup_pes_filter(ctx->opt,
                         priv->filter_fds[filter_index++],
                         DMX_PES_VIDEO3,
                         track->video_streams[3].stream_id))
      return 0;
    }

  /* PCR filter */
  if(channel->extra_pcr_pid)
    {
    if(!setup_pes_filter(ctx->opt,
                         priv->filter_fds[filter_index++],
                         DMX_PES_PCR0,
                         channel->pcr_pid))
      return 0;
    }
  
  bgav_log(ctx->opt, BGAV_LOG_INFO, LOG_DOMAIN,
           "Filters initialized successfully");
  
  return 1;
  
  }

static void select_track_dvb(bgav_input_context_t * ctx, int track)
  {
  dvb_priv_t * priv;
  priv = (dvb_priv_t *)(ctx->priv);
  
  if(priv->dvr_fd >= 0)
    {
    close(priv->dvr_fd);
    priv->dvr_fd = -1;
    }
  if(!tune_in(ctx, &priv->channels[track]))
    return;
  
  if(!setup_filters(ctx, &priv->channels[track], ctx->tt->current_track))
    return;
  
  priv->dvr_fd = open(priv->dvr_filename, O_RDONLY);
  ctx->sync_id = priv->channels[track].pcr_pid;
  }

bgav_input_t bgav_input_dvb =
  {
    name:          "dvb",
    open:          open_dvb,
    read:          read_dvb,
    close:         close_dvb,
    select_track:  select_track_dvb,
  };

static
bgav_input_context_t * bgav_input_open_dvb(const char * device,
                                           bgav_options_t * opt)
  {
  bgav_input_context_t * ret = (bgav_input_context_t *)0;
  ret = bgav_input_create(opt);
  ret->input = &bgav_input_dvb;

  if(!ret->input->open(ret, device))
    {
    goto fail;
    }
  return ret;
  fail:
  if(ret)
    free(ret);
  return (bgav_input_context_t *)0;
  }

int bgav_open_dvb(bgav_t * b, const char * device)
  {
  bgav_codecs_init(&b->opt);
  b->input = bgav_input_open_dvb(device, &(b->opt));
  if(!b->input)
    return 0;
  if(!bgav_init(b))
    goto fail;
  return 1;
  fail:
  return 0;
  }

int bgav_check_device_dvb(const char * device, char ** name)
  {
  struct dvb_frontend_info fe_info;
  int fd;
  char * tmp_string;

  tmp_string = bgav_sprintf("%s/frontend0", device);
  fd = open(tmp_string, O_RDONLY);
  free(tmp_string);
  
  if(fd < 0)
    {
    return 0;
    }
  /* Get frontend info */
  if((ioctl(fd, FE_GET_INFO, &fe_info)) < 0)
    {
    close(fd);
    return 0;
    }
  *name = bgav_strdup(fe_info.name);
  close(fd);
  return 1;
  }

bgav_device_info_t * bgav_find_devices_dvb()
  {
  int i;
  char * device_name;
  char * directory;
  
  bgav_device_info_t * ret = (bgav_device_info_t *)0;
  
  for(i = 0; i < 8; i++)
    {
    device_name = (char*)0;
    
    directory = bgav_sprintf("/dev/dvb/adapter%d", i);
    
    if(bgav_check_device_dvb(directory, &device_name))
      {
      ret = bgav_device_info_append(ret, directory, device_name);
      if(device_name)
        free(device_name);
      free(directory);
      }
    else
      {
      free(directory);
      break;
      }
    }
  return ret;
  }

#else // !HAVE_LINUXDVB

int bgav_check_device_dvb(const char * device, char ** name)
  {
  return 0;
  }

bgav_device_info_t * bgav_find_devices_dvb()
  {
  return (bgav_device_info_t*)0;
  }

int bgav_open_dvb(bgav_t * b, const char * device)
  {
  return 0;
  }

#endif
