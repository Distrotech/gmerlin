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

  bg_cfg_section_t * cfg_section;
  const bg_plugin_info_t * info;  
  char * tmp_path;
  the_input = calloc(1, sizeof(*the_input));
  
  
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
  //  fprintf(stderr, "Input format:\n");
  //  gavl_audio_format_dump(&input_format);
  gavl_audio_format_copy(&format, &input_format);

  format.num_channels = 2;
  gavl_set_channel_setup(&format);
  format.interleave_mode = GAVL_INTERLEAVE_NONE;
  format.samples_per_frame = 1024;
  format.sample_format = GAVL_SAMPLE_S16;

  //  fprintf(stderr, "Internal format:\n");
  //  gavl_audio_format_dump(&format);


  the_input->cnv = gavl_audio_converter_create();
  the_input->do_convert_gavl = 
    gavl_audio_converter_init(the_input->cnv, &input_format, &format);

  if(the_input->do_convert_gavl)
    {
    /* Correct this because it might have been changed by the input plugin */
    if(input_format.samples_per_frame < 512)
      input_format.samples_per_frame = 512;
    
    the_input->input_frame = gavl_audio_frame_create(&input_format);
    }
  the_input->frame = gavl_audio_frame_create(NULL);

  the_input->frame->channels.s_16[0] = the_input->audio_frame.pcm_data[0];
  the_input->frame->channels.s_16[1] = the_input->audio_frame.pcm_data[1];

#ifdef HAVE_LIBVISUAL
  the_input->audio_frame.vis_audio = visual_audio_new();
#endif

  return 1;
  }


void input_add_plugin(input_t * c, vis_plugin_handle_t * plugin)
  {
  plugin->next = c->active_plugins;
  c->active_plugins = plugin;

  if(plugin->start)
    plugin->start(plugin);
  
  /* fprintf(stderr, "done\n"); */
  }

vis_plugin_handle_t * list_remove(vis_plugin_handle_t * l, vis_plugin_handle_t * handle)
  {
  vis_plugin_handle_t * tmp_handle;
  
  if(handle == l)
    {
    l = l->next;
    return l;
    }
  
  tmp_handle = l;
  while(tmp_handle->next != handle)
    tmp_handle = tmp_handle->next;

  tmp_handle->next = handle->next;
  return l;
  }


void input_remove_plugin(input_t * c, const vis_plugin_info_t * info)
  {
  vis_plugin_handle_t * plugin;
  plugin = c->active_plugins;
  while(plugin)
    {
    if(plugin->info == info)
      break;
    plugin = plugin->next;
    }
  if(!plugin)
    return;
  
  c->active_plugins = list_remove(c->active_plugins, plugin);

  if(plugin->stop)
    plugin->stop(plugin);
  plugin->unload(plugin);
  }

int input_iteration(void * data)
  {
#ifndef HAVE_LIBVISUAL
  gint16 * ptr1;
  gint16 * ptr2;
  gint16 * ptr3;
  int i;
#endif
  input_t * c = (input_t*)data;

  vis_plugin_handle_t * tmp_plugin;
  
  /* Read data from the input */

  //  fprintf(stderr, "Input iteration\n");
  
  if(c->do_convert_gavl)
    {
    c->input->read_frame(c->input_handle->priv, c->input_frame, 512);
    gavl_audio_convert(c->cnv, c->input_frame, c->frame);
    //    fprintf(stderr, "input_iteration %d\n", c->input_frame->channels.s_16[0][0]);
    }
  else
    {
    c->input->read_frame(c->input_handle->priv, c->frame, 512);
    }

  //  fprintf(stderr, "input_iteration %d\n", c->frame->channels.s_16[0][0]);

#ifdef HAVE_LIBVISUAL
  memcpy(c->audio_frame.vis_audio->plugpcm[0], c->audio_frame.pcm_data[0], 512 * 2);
  memcpy(c->audio_frame.vis_audio->plugpcm[1], c->audio_frame.pcm_data[1], 512 * 2);
  visual_audio_analyze(c->audio_frame.vis_audio);

  /* Copy data for xmms */
    
  memcpy(c->audio_frame.pcm_data_mono, c->audio_frame.vis_audio->pcm[2], 512 * 2);

  memcpy(c->audio_frame.freq_data[0], c->audio_frame.vis_audio->freq[0], 256 * 2);
  memcpy(c->audio_frame.freq_data[1], c->audio_frame.vis_audio->freq[1], 256 * 2);

  memcpy(c->audio_frame.freq_data_mono[0], c->audio_frame.vis_audio->freq[2], 256 * 2);
  memcpy(c->audio_frame.freq_data_mono[1], c->audio_frame.vis_audio->freq[3], 256 * 2);
#else
  
  /* Check, if we need mono samples */

  ptr1 = c->audio_frame.pcm_data[0];
  ptr2 = c->audio_frame.pcm_data[1];
  ptr3 = c->audio_frame.pcm_data_mono[0];
  for(i = 0; i < 512; i++)
    *(ptr3++) = (*(ptr1++) + *(ptr2++)) >> 1;
  
  /* Check if we need the fft of the stereo samples */
  
  fft_perform(c->audio_frame.pcm_data[0], c->fft_scratch, c->state);
  for(i = 0; i < 256; i++)
    c->audio_frame.freq_data[0][i] = ((gint)sqrt(c->fft_scratch[i + 1])) >> 8;
  
  fft_perform(c->audio_frame.pcm_data[1], c->fft_scratch, c->state);
  for(i = 0; i < 256; i++)
    c->audio_frame.freq_data[1][i] = ((gint)sqrt(c->fft_scratch[i + 1])) >> 8;
  
  /* Check if we need the fft of the mono samples */

  fft_perform(c->audio_frame.pcm_data_mono[0], c->fft_scratch, c->state);
  for(i = 0; i < 256; i++)
    c->audio_frame.freq_data_mono[0][i] = ((gint)sqrt(c->fft_scratch[i + 1])) >> 8;
#endif

  tmp_plugin = c->active_plugins;
  
  while(tmp_plugin)
    {
    if(tmp_plugin->render)
      tmp_plugin->render(tmp_plugin, &(c->audio_frame));

    tmp_plugin = tmp_plugin->next;
    }

  /* Handle gtk events */

  while(gtk_events_pending())
    gtk_main_iteration();
  return 1;
  }

void input_cleanup()
  {
  while(the_input->active_plugins)
    {
    input_remove_plugin(the_input, the_input->active_plugins->info);
    }
  }
