/*****************************************************************
 
  plugininfo.c
 
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

#include <pluginregistry.h>
#include <gui_gtk/plugin.h>
#include <gui_gtk/textview.h>


static struct
  {
  char * name;
  bg_plugin_type_t type;
  }
type_names[] =
  {
    { "Input",          BG_PLUGIN_INPUT },
    { "Audio output",   BG_PLUGIN_OUTPUT_AUDIO },
    { "Video output",   BG_PLUGIN_OUTPUT_VIDEO },
    { "Audio recorder", BG_PLUGIN_RECORDER_AUDIO },
    { "Video recorder", BG_PLUGIN_RECORDER_VIDEO },
    { "Audio encoder",  BG_PLUGIN_ENCODER_AUDIO },
    { "Video encoder",  BG_PLUGIN_ENCODER_VIDEO },
    { "Text subtitle exporter",  BG_PLUGIN_ENCODER_SUBTITLE_TEXT },
    { "Overlay subtitle exporter",  BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY },
    { "Audio/Video encoder",  BG_PLUGIN_ENCODER },
    { "Image Reader",   BG_PLUGIN_IMAGE_READER  },
    { "Image Writer",   BG_PLUGIN_IMAGE_WRITER  },
    { "Encoding postprocessor",   BG_PLUGIN_ENCODER_PP  },
    { (char*)0,         BG_PLUGIN_NONE }
  };

static struct
  {
  char * name;
  uint32_t flag;
  }
flag_names[] =
  {
    { "Removable Device", BG_PLUGIN_REMOVABLE }, /* Removable media (CD, DVD etc.) */
    { "Recorder",    BG_PLUGIN_RECORDER       }, /* Plugin can record              */
    { "File",        BG_PLUGIN_FILE           }, /* Plugin reads/writes files      */
    { "URL",         BG_PLUGIN_URL            }, /* Plugin reads URLs or streams   */
    { "Playback",    BG_PLUGIN_PLAYBACK       }, /* Output plugins for playback    */
    { "Bypass",      BG_PLUGIN_BYPASS         }, /* Bypass                         */
    { "Keep Running", BG_PLUGIN_KEEP_RUNNING   }, /* Plugin should not be stopped and restarted if tracks change */
    { "Has Sync",     BG_PLUGIN_INPUT_HAS_SYNC }, /* FOR INPUTS ONLY: Plugin will set the time via callback */
    { "Tuner",       BG_PLUGIN_TUNER           }, /* Plugin has tuner */
    { (char*)0,      0                        },
  };

static char * get_flag_string(uint32_t flags)
  {
  char * ret;
  int i, j, index, num_flags;
  uint32_t flag;

  ret = malloc(1024);
  *ret = '\0';
  
  /* Count the flags */
    
  num_flags = 0;
  for(i = 0; i < 32; i++)
    {
    flag = (1<<i);
    if(flags & flag)
      num_flags++;
    }

  /* Create the string */
  
  index = 0;
  
  for(i = 0; i < 32; i++)
    {
    flag = (1<<i);
    if(flags & flag)
      {
      j = 0;
      while(flag_names[j].name)
        {
        if(flag_names[j].flag == flag)
          {
          strcat(ret, flag_names[j].name);
          if(index < num_flags - 1)
            strcat(ret, ", ");
          index++;
          break;
          }
        j++;
        }
      }
    }
  return ret;
  }

static const char * get_type_string(bg_plugin_type_t type)
  {
  int i = 0;
  while(type_names[i].name)
    {
    if(type_names[i].type == type)
      return type_names[i].name;
    i++;
    }
  return (char*)0;
  }

void bg_gtk_plugin_info_show(const bg_plugin_info_t * info)
  {
  char * text;
  char * flag_string;
  
  bg_gtk_textwindow_t * win;

  flag_string = get_flag_string(info->flags);
  text = bg_sprintf("Name:\t %s\nLong name:\t %s\nType:\t %s\nFlags:\t %s\nPriority:\t %d\nDLL Filename:\t %s",
                    info->name, info->long_name, get_type_string(info->type),
                    flag_string, info->priority, info->module_filename);
  win = bg_gtk_textwindow_create(text, info->long_name);
  
  free(text);
  free(flag_string);
  
  bg_gtk_textwindow_show(win, 1);
  }
