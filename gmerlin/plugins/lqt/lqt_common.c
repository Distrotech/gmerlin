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
    
    info->multi_names[i] = bg_strdup((char*)0,
                                     codec_info[i]->name);
    info->multi_labels[i] = bg_strdup((char*)0,
                                         codec_info[i]->long_name);
    info->multi_descriptions[i] = bg_strdup((char*)0,
                                           codec_info[i]->description);

    num_parameters = (encode) ? codec_info[i]->num_encoding_parameters :
      codec_info[i]->num_decoding_parameters;

    if(num_parameters)
      info->multi_parameters[i] = calloc(num_parameters + 1,
                                         sizeof(bg_parameter_info_t));
    
    for(j = 0; j < num_parameters; j++)
      {
      info->multi_parameters[i][j].name =
        bg_sprintf("codec_%s_%s", info->multi_names[i],
                   lqt_parameter_info[j].name);
      info->multi_parameters[i][j].long_name = 
        bg_strdup((char*)0, lqt_parameter_info[j].real_name);

      switch(lqt_parameter_info[j].type)
        {
        case LQT_PARAMETER_INT:
          if(lqt_parameter_info[j].val_min < lqt_parameter_info[j].val_max)
            {
            if((lqt_parameter_info[j].val_min == 0) &&
               (lqt_parameter_info[j].val_max == 1))
              {
              info->multi_parameters[i][j].type = BG_PARAMETER_CHECKBUTTON;
              }
            else
              {
              info->multi_parameters[i][j].type = BG_PARAMETER_SLIDER_INT;
              info->multi_parameters[i][j].val_min.val_i =
                lqt_parameter_info[j].val_min;
              info->multi_parameters[i][j].val_max.val_i =
                lqt_parameter_info[j].val_max;
              }
            }
          else
            {
            info->multi_parameters[i][j].type = BG_PARAMETER_INT;
            }
          info->multi_parameters[i][j].val_default.val_i =
            lqt_parameter_info[j].val_default.val_int;
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

          info->multi_parameters[i][j].options =
            calloc(lqt_parameter_info[j].num_stringlist_options+1,
                   sizeof(char*));
          
          for(k = 0; k < lqt_parameter_info[j].num_stringlist_options; k++)
            {
            info->multi_parameters[i][j].options[k] =
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

int bg_lqt_set_parameter(const char * name, bg_parameter_value_t * val,
                         bg_parameter_info_t * info)
  {
  int i, j, done;
  if(strncmp(name, "codec_", 6))
    return 0;
    
  done = 0;
  i = 0;
  while(info->multi_names[i])
    {
    if(info->multi_parameters[i])
      {
      j = 0;
      
      while(info->multi_parameters[i][j].name)
        {
        if(!strcmp(name, info->multi_parameters[i][j].name))
          {
          bg_parameter_value_copy(&(info->multi_parameters[i][j].val_default),
                                  val,
                                  &(info->multi_parameters[i][j]));
          done = 1;
          }
        j++;
        }
      }
    i++;
    if(done)
      break;
    }
  return done;
  }

static int _colormodels[] =
  {
    // Colormodels
    //    BC_TRANSPARENCY,
    //    BC_COMPRESSED,
    //    BC_RGB8,
    BC_RGB565,
    BC_BGR565,
    BC_BGR888,
    BC_BGR8888,
    // Working bitmaps are packed to simplify processing
    BC_RGB888,
    BC_RGBA8888,
    //    BC_ARGB8888,
    //    BC_ABGR8888,
    //    BC_RGB161616,
    //    BC_RGBA16161616,
    //    BC_YUV888,
    //    BC_YUVA8888,
    //    BC_YUV161616,
    //    BC_YUVA16161616,
    BC_YUV422,
    //    BC_A8,
    //    BC_A16,
    //    BC_YUV101010,
    //    BC_VYU888,
    //    BC_UYVA8888,
// Planar
    BC_YUV420P,
    BC_YUV422P,
    BC_YUV444P,
    //    BC_YUV411P,
    LQT_COLORMODEL_NONE
  };

int * bg_lqt_supported_colormodels = _colormodels;

static struct
  {
  int lqt;
  gavl_colorspace_t gavl;
  }
colorspace_table[] =
  {
    { BC_RGB565,   GAVL_RGB_16 },
    { BC_BGR565,   GAVL_BGR_16 },
    { BC_BGR888,   GAVL_BGR_24 },
    { BC_BGR8888,  GAVL_BGR_32 },
    { BC_RGB888,   GAVL_RGB_24 },
    { BC_RGBA8888, GAVL_RGBA_32 },
    { BC_YUV422,   GAVL_YUY2 },
    { BC_YUV420P,  GAVL_YUV_420_P },
    { BC_YUV422P,  GAVL_YUV_422_P },
    { BC_YUV444P,  GAVL_YUV_422_P },
  };

static int colorspace_table_size =
  sizeof(colorspace_table) / sizeof(colorspace_table[0]);

gavl_colorspace_t bg_lqt_get_gavl_colorspace(int quicktime_colorspace)
  {
  int i;

  for(i = 0; i < colorspace_table_size; i++)
    {
    if(colorspace_table[i].lqt == quicktime_colorspace)
      return colorspace_table[i].gavl;
    }
  return GAVL_COLORSPACE_NONE;
  }

int bg_lqt_get_lqt_colorspace(gavl_colorspace_t gavl_colorspace)
  {
  int i;

  for(i = 0; i < colorspace_table_size; i++)
    {
    if(colorspace_table[i].gavl == gavl_colorspace)
      return colorspace_table[i].lqt;
    }
  return LQT_COLORMODEL_NONE;
  }
