/*****************************************************************

  input.c

  Copyright (c) 2003 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

/* This file is part of gmerlin_vizualizer */

#include <gtk/gtk.h>
#include <xmms/plugin.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "fft.h"
#include "input.h"

#include <stdio.h>

#include <stdlib.h>

input_t * the_input = (input_t*)0;

int input_create()
  {
  gavl_audio_format_t input_format;
  gavl_audio_format_t format;
  gavl_audio_options_t opt;

  bg_cfg_section_t * cfg_section;
  const bg_plugin_info_t * info;  
  char * tmp_path;
  the_input = calloc(1, sizeof(*the_input));
  
  the_input->active_plugins = (GList*)0;
  
  the_input->conversion_mode = 0;
  the_input->state = fft_init();

  /* Create plugin regsitry */
  the_input->cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(the_input->cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
   
  cfg_section = bg_cfg_registry_find_section(the_input->cfg_reg, "plugins");
  the_input->plugin_reg = bg_plugin_registry_create(cfg_section);

  /* Load plugin */
  info = bg_plugin_registry_get_default(the_input->plugin_reg, BG_PLUGIN_RECORDER_AUDIO);

  if(!info)
    return 0;
  the_input->input_handle = bg_plugin_load(the_input->plugin_reg, info);
  if(!the_input->input_handle)
    return 0;

  the_input->input = (bg_ra_plugin_t*)(the_input->input_handle->plugin);
  if(!the_input->input->open(the_input->input_handle->priv, &input_format))
    return 0;

  /* Set up format */

  gavl_audio_format_dump(&input_format);
  gavl_audio_format_copy(&format, &input_format);

  format.num_channels = 2;
  format.lfe = 0;
  gavl_set_channel_setup(&format);
  format.interleave_mode = GAVL_INTERLEAVE_NONE;
  format.samples_per_frame = 1024;
  format.sample_format = GAVL_SAMPLE_S16;
  gavl_audio_default_options(&opt);
  the_input->cnv = gavl_audio_converter_create();
  the_input->do_convert_gavl = 
    gavl_audio_converter_init(the_input->cnv, &opt, &input_format, &format);

  if(the_input->do_convert_gavl)
    {
    /* Correct this because it might have been changed by the input plugin */
    if(input_format.samples_per_frame < 512)
      input_format.samples_per_frame = 512;
    
    the_input->input_frame = gavl_audio_frame_create(&input_format);
    }
  the_input->frame = gavl_audio_frame_create(NULL);

  the_input->frame->channels.s_16[0] = the_input->pcm_data[0];
  the_input->frame->channels.s_16[1] = the_input->pcm_data[1];
  
  return 1;
  }

static int check_conversion_flags(VisPlugin * plugin)
  {
  int ret = 0;
  if((plugin->num_pcm_chs_wanted == 1) && (plugin->render_pcm))
    ret |= INPUT_NEED_MONO;
  if(plugin->render_freq)
    {
    if(plugin->num_freq_chs_wanted == 2)
      ret |= INPUT_NEED_FREQ_STEREO;
    else
      ret |= (INPUT_NEED_FREQ_MONO|INPUT_NEED_MONO);
    }
  return ret;
  }

void input_add_plugin(input_t * c, VisPlugin * plugin)
  {
  /*  fprintf(stderr, "esd_connection_add_plugin..."); */
  
  c->active_plugins = g_list_append(c->active_plugins, (gpointer)plugin);

  c->conversion_mode |= check_conversion_flags(plugin);

  if(plugin->init)
    plugin->init();
  
  if(plugin->playback_start)
    plugin->playback_start();
  /* fprintf(stderr, "done\n"); */
  }

void input_remove_plugin(input_t * c, VisPlugin * plugin)
  {
  VisPlugin * tmp_plugin;
  int num_plugins, i;
  
  c->active_plugins = g_list_remove(c->active_plugins, (gpointer)plugin);

  if(plugin->playback_stop)
    plugin->playback_stop();

  if(plugin->cleanup)
    plugin->cleanup();
    
  num_plugins = g_list_length(c->active_plugins);
  
  c->conversion_mode = 0;
  for(i = 0; i < num_plugins; i++)
    {
    tmp_plugin = (VisPlugin*)g_list_nth_data(c->active_plugins, i);
    
    c->conversion_mode |= check_conversion_flags(tmp_plugin);
    }
  }

int input_iteration(void * data)
  {
  gint16 * ptr1;
  gint16 * ptr2;
  gint16 * ptr3;
  int i, num_plugins;
  GList * node;
  VisPlugin* plugin;
  
  input_t * c = (input_t*)data;
  
  num_plugins = g_list_length(c->active_plugins);

  /* Read data from the input */
  
  if(c->do_convert_gavl)
    {
    c->input->read_frame(c->input_handle->priv, c->input_frame, 512);
    gavl_audio_convert(c->cnv, c->input_frame, c->frame);
    }
  else
    {
    c->input->read_frame(c->input_handle->priv, c->frame, 512);
    }
  
  /* Check, if we need mono samples */

  if(c->conversion_mode & INPUT_NEED_MONO)
    {
    ptr1 = c->pcm_data[0];
    ptr2 = c->pcm_data[1];
    ptr3 = c->pcm_data_mono[0];
    for(i = 0; i < 512; i++)
      *(ptr3++) = (*(ptr1++) + *(ptr2++)) >> 1;
    }

  /* Check if we need the fft of the stereo samples */
  
  if(c->conversion_mode & INPUT_NEED_FREQ_STEREO)
    {
    fft_perform(c->pcm_data[0], c->fft_scratch, c->state);
    for(i = 0; i < 256; i++)
      c->freq_data[0][i] = ((gint)sqrt(c->fft_scratch[i + 1])) >> 8;

    fft_perform(c->pcm_data[1], c->fft_scratch, c->state);
    for(i = 0; i < 256; i++)
      c->freq_data[1][i] = ((gint)sqrt(c->fft_scratch[i + 1])) >> 8;
    }
  
  /* Check if we need the fft of the mono samples */

  if(c->conversion_mode & INPUT_NEED_FREQ_MONO)
    {
    fft_perform(c->pcm_data_mono[0], c->fft_scratch, c->state);
    for(i = 0; i < 256; i++)
      c->freq_data_mono[0][i] = ((gint)sqrt(c->fft_scratch[i + 1])) >> 8;
    }

  node = g_list_first(c->active_plugins);

  for(i = 0; i < num_plugins; i++)
    {
    plugin = (VisPlugin*)node->data;
        
    if(plugin->render_pcm)
      {
      if(plugin->num_pcm_chs_wanted == 1)
        plugin->render_pcm(c->pcm_data_mono);
      else if(plugin->num_pcm_chs_wanted == 2)
        plugin->render_pcm(c->pcm_data);
      }
    if(plugin->render_freq)
      {
      if(plugin->num_freq_chs_wanted == 1)
        plugin->render_freq(c->freq_data_mono);
      else
        plugin->render_freq(c->freq_data);
      }
    node = node->next;
    }

  /* Handle gtk events */

  while(gtk_events_pending())
    gtk_main_iteration();
  return 1;
  }

