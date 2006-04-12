/*****************************************************************
 
  lqt_common.c
 
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
#include <plugin.h>
#include <utils.h>

#include "lqt_common.h"

void bg_lqt_create_codec_info(bg_parameter_info_t * info,
                              int audio, int video, int encode, int decode)
  {
  int num_codecs;
  int i, j, k;
  int num_parameters;
  
  lqt_codec_info_t ** codec_info;

  lqt_parameter_info_t * lqt_parameter_info;
    
  codec_info = lqt_query_registry(audio, video, encode, decode);
  info->type = (encode) ? BG_PARAMETER_MULTI_MENU :
    BG_PARAMETER_MULTI_LIST;

  num_codecs = 0;
  while(codec_info[num_codecs])
    num_codecs++;

  info->multi_names        = calloc(num_codecs + 1, sizeof(char*));
  info->multi_labels       = calloc(num_codecs + 1, sizeof(char*));
  info->multi_descriptions = calloc(num_codecs + 1, sizeof(char*));
  info->multi_parameters   = calloc(num_codecs + 1,
                                    sizeof(bg_parameter_info_t*));

  for(i = 0; i < num_codecs; i++)
    {
    lqt_parameter_info = (encode) ? codec_info[i]->encoding_parameters :
      codec_info[i]->decoding_parameters;

    if(!i)
      info->val_default.val_str = bg_strdup((char*)0,
                                            codec_info[i]->name);
    
    info->multi_names[i] = bg_strdup((char*)0,
                                     codec_info[i]->name);
    info->multi_labels[i] = bg_strdup((char*)0,
                                         codec_info[i]->long_name);
    info->multi_descriptions[i] = bg_sprintf("%s Use for", codec_info[i]->description);
    
    if(codec_info[i]->compatibility_flags & (LQT_FILE_QT | LQT_FILE_QT_OLD))
      info->multi_descriptions[i] = bg_strcat(info->multi_descriptions[i], " QT");
    if(codec_info[i]->compatibility_flags & ( LQT_FILE_MP4))
      info->multi_descriptions[i] = bg_strcat(info->multi_descriptions[i], " MP4");
    if(codec_info[i]->compatibility_flags & ( LQT_FILE_M4A))
      info->multi_descriptions[i] = bg_strcat(info->multi_descriptions[i], " M4A");
    if(codec_info[i]->compatibility_flags & ( LQT_FILE_AVI))
      info->multi_descriptions[i] = bg_strcat(info->multi_descriptions[i], " AVI");
    
    num_parameters = (encode) ? codec_info[i]->num_encoding_parameters :
      codec_info[i]->num_decoding_parameters;

    if(num_parameters)
      info->multi_parameters[i] = calloc(num_parameters + 1,
                                         sizeof(bg_parameter_info_t));
    
    for(j = 0; j < num_parameters; j++)
      {
      info->multi_parameters[i][j].name = bg_strdup(info->multi_parameters[i][j].name,
                                                    lqt_parameter_info[j].name);
      //        bg_sprintf("codec_%s_%s", info->multi_names[i],
      //                   lqt_parameter_info[j].name);
      info->multi_parameters[i][j].long_name = 
        bg_strdup((char*)0, lqt_parameter_info[j].real_name);

      if(lqt_parameter_info[j].help_string)
        {
        info->multi_parameters[i][j].help_string = 
          bg_strdup((char*)0, lqt_parameter_info[j].help_string);
        
        }
      
      switch(lqt_parameter_info[j].type)
        {
        case LQT_PARAMETER_INT:
          if(lqt_parameter_info[j].val_min.val_int <
             lqt_parameter_info[j].val_max.val_int)
            {
            if((lqt_parameter_info[j].val_min.val_int == 0) &&
               (lqt_parameter_info[j].val_max.val_int == 1))
              {
              info->multi_parameters[i][j].type = BG_PARAMETER_CHECKBUTTON;
              }
            else
              {
              info->multi_parameters[i][j].type = BG_PARAMETER_SLIDER_INT;
              info->multi_parameters[i][j].val_min.val_i =
                lqt_parameter_info[j].val_min.val_int;
              info->multi_parameters[i][j].val_max.val_i =
                lqt_parameter_info[j].val_max.val_int;
              }
            }
          else
            {
            info->multi_parameters[i][j].type = BG_PARAMETER_INT;
            }
          info->multi_parameters[i][j].val_default.val_i =
            lqt_parameter_info[j].val_default.val_int;
          break;
        case LQT_PARAMETER_FLOAT:
          if(lqt_parameter_info[j].val_min.val_float <
             lqt_parameter_info[j].val_max.val_float)
            {
            info->multi_parameters[i][j].type = BG_PARAMETER_SLIDER_FLOAT;
            info->multi_parameters[i][j].val_min.val_f =
              lqt_parameter_info[j].val_min.val_float;
            info->multi_parameters[i][j].val_max.val_f =
              lqt_parameter_info[j].val_max.val_float;
            }
          else
            {
            info->multi_parameters[i][j].type = BG_PARAMETER_FLOAT;
            }
          info->multi_parameters[i][j].num_digits =
            lqt_parameter_info[j].num_digits;

          info->multi_parameters[i][j].val_default.val_f =
            lqt_parameter_info[j].val_default.val_float;
          break;
        case LQT_PARAMETER_STRING:
          info->multi_parameters[i][j].type = BG_PARAMETER_STRING;
          info->multi_parameters[i][j].val_default.val_str =
            bg_strdup((char*)0,
                      lqt_parameter_info[j].val_default.val_string);
          
          break;
        case LQT_PARAMETER_STRINGLIST:
          info->multi_parameters[i][j].type = BG_PARAMETER_STRINGLIST;
          info->multi_parameters[i][j].val_default.val_str =
            bg_strdup((char*)0,
                      lqt_parameter_info[j].val_default.val_string);

          info->multi_parameters[i][j].multi_names =
            calloc(lqt_parameter_info[j].num_stringlist_options+1,
                   sizeof(char*));
          
          for(k = 0; k < lqt_parameter_info[j].num_stringlist_options; k++)
            {
            info->multi_parameters[i][j].multi_names[k] =
              bg_strdup((char*)0, lqt_parameter_info[j].stringlist_options[k]);
            }
          break;
        case LQT_PARAMETER_SECTION:
          info->multi_parameters[i][j].type = BG_PARAMETER_SECTION;
          break;
        }
      
      }
    }
  lqt_destroy_codec_info(codec_info);
  }

static void * get_value(lqt_parameter_info_t * lqt_parameter_info,
                        const char * name,
                        bg_parameter_value_t * val)
  {
  int index;
  index = 0;

  while(lqt_parameter_info[index].name)
    {
    if(!strcmp(lqt_parameter_info[index].name, name))
      {
      switch(lqt_parameter_info[index].type)
        {
        case LQT_PARAMETER_INT:
          return &(val->val_i);
          break;
        case LQT_PARAMETER_FLOAT:
          return &(val->val_f);
          break;
        case LQT_PARAMETER_STRING:
        case LQT_PARAMETER_STRINGLIST:
          return val->val_str;
          break;
        case LQT_PARAMETER_SECTION:
          return (void*)0;
        }
      break;
      }
    index++;
    }
  return (void*)0;
  }
                        
void bg_lqt_set_audio_parameter(quicktime_t * file,
                                int stream,
                                char * name,
                                bg_parameter_value_t * val,
                                lqt_parameter_info_t * lqt_parameter_info)
  {
  void * val_ptr;
  val_ptr = get_value(lqt_parameter_info, name, val);
  if(val_ptr)
    {
    lqt_set_audio_parameter(file, stream, name, val_ptr);
    }
  
  }

void bg_lqt_set_video_parameter(quicktime_t * file,
                                int stream,
                                char * name,
                                bg_parameter_value_t * val,
                                lqt_parameter_info_t * lqt_parameter_info)
  {
  void * val_ptr;
  val_ptr = get_value(lqt_parameter_info, name, val);
  if(val_ptr)
    {
    lqt_set_video_parameter(file, stream, name, val_ptr);
    }
  }
