/*****************************************************************
 
  i_esd.h
 
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <plugin.h>
#include <utils.h>

#include <esd.h>

typedef struct
  {
  int esd_socket;
  char * hostname;
  int bytes_per_sample;

  int do_monitor;
  } esd_t;

bg_parameter_info_t parameters[] =
  {
    {
      name:      "esd_host",
      long_name: "Host (empty: local)",
      type:      BG_PARAMETER_STRING,
    },
    {
      name:        "input_mode",
      long_name:   "Input Mode:",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Record" },
      options:     (char*[]){ "Record", "Monitor", (char*)0 },
    },
    { /* End of parameters */ }
  };

static void set_parameter_esd(void * data, char * name,
                          bg_parameter_value_t * val)
  {
  esd_t * e = (esd_t *)data;

  if(!name)
    return;
  if(!strcmp(name, "esd_host"))
    {
    e->hostname = bg_strdup(e->hostname, val->val_str);
    }
  else if(!strcmp(name, "input_mode"))
    {
    if(!strcmp(val->val_str, "Monitor"))
      e->do_monitor = 1;
    else
      e->do_monitor = 0;
    }
  }

static void * create_esd()
  {
  esd_t * ret = calloc(1, sizeof(*ret));

  return ret;
  }

static void destroy_esd(void *data)
  {
  esd_t * e = (esd_t *)data;

  if(e->hostname)
    free(e->hostname);
  free(e);
  }

bg_parameter_info_t *
get_parameters_esd(void * priv)
  {
  return parameters;
  }

static int open_esd(void * data,
                    gavl_audio_format_t * format)
  {
  int esd_format;
  const char * esd_host;
  char * name;
  char hostname[128];
  
  esd_t * e = (esd_t*)data;
  
  /* Set up format */

  format->interleave_mode = GAVL_INTERLEAVE_ALL;
  format->num_channels = 2;
  format->channel_setup = GAVL_CHANNEL_STEREO;
  format->samplerate = 44100;
  format->sample_format = GAVL_SAMPLE_S16;
  format->lfe = 0;

  if(!e->hostname || (*(e->hostname) == '\0'))
    {
    esd_host = (const char*)0;
    }
  else
    esd_host = e->hostname;
    
  esd_format = ESD_STREAM | ESD_PLAY;
  if(e->do_monitor)
    esd_format |= ESD_MONITOR;
  else
    esd_format |= ESD_RECORD;
  
  esd_format |= (ESD_STEREO|ESD_BITS16);

  gethostname(hostname, 128);
    
  name = bg_sprintf("gmerlin@%s pid: %d", hostname, getpid());

  fprintf(stderr, "Stream name: %s\n", name);
    
  if(e->do_monitor)
    e->esd_socket = esd_monitor_stream(esd_format, format->samplerate,
                                       e->hostname, name);
  else
    e->esd_socket = esd_record_stream(esd_format, format->samplerate,
                                      e->hostname, name);
  free(name);
  if(e->esd_socket < 0)
    {
    fprintf(stderr, "i_esd: Cannot connect to daemon\n");
    return 0;
    }
  
  
  return 1;
  }

static void close_esd(void * data)
  {
  esd_t * e = (esd_t*)(data);
  esd_close(e->esd_socket);
  }

static void read_frame_esd(void * p, gavl_audio_frame_t * f)
  {
  esd_t * priv = (esd_t*)(p);
  
  f->valid_samples = read(priv->esd_socket,
                          f->samples.s_8,
                          ESD_BUF_SIZE);

  f->valid_samples /= priv->bytes_per_sample;
  }

bg_ra_plugin_t the_plugin =
  {
    common:
    {
      name:          "i_esd",
      long_name:     "EsounD input driver",
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      type:          BG_PLUGIN_RECORDER_AUDIO,
      flags:         BG_PLUGIN_RECORDER,
      create:        create_esd,
      destroy:       destroy_esd,

      get_parameters: get_parameters_esd,
      set_parameter:  set_parameter_esd
    },

    open:                open_esd,
    read_frame:          read_frame_esd,
    close:               close_esd,
  };
