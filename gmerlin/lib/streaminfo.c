/*****************************************************************
 
  streaminfo.c
 
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
#include <stdio.h>
#include <ctype.h>

#include <gavl/gavl.h>
#include <streaminfo.h>
#include <utils.h>

#define my_free(ptr) \
  if(ptr) \
    { \
    free(ptr); \
    ptr = NULL; \
    }


void bg_track_info_free(bg_track_info_t * info)
  {
  int i;

  if(info->audio_streams)
    {
    for(i = 0; i < info->num_audio_streams; i++)
      my_free(info->audio_streams[i].language);
    my_free(info->audio_streams);
    }
  
  my_free(info->video_streams);
  if(info->subpicture_streams)
    {
    for(i = 0; i < info->num_subpicture_streams; i++)
      my_free(info->subpicture_streams[i].language);
    my_free(info->subpicture_streams);
    }

  my_free(info->metadata.artist);
  my_free(info->metadata.title);
  my_free(info->metadata.album);
  my_free(info->metadata.genre);
  my_free(info->metadata.comment);
  my_free(info->metadata.author);
  my_free(info->metadata.copyright);
  }

/*
 *  %p:    Artist
 *  %a:    Album
 *  %g:    Genre
 *  %t:    Track name
 *  %<d>n: Track number (d = number of digits, 1-9)
 *  %y:    Year
 *  %c:    Comment
 */

char * bg_create_track_name(const bg_track_info_t * info,
                            const char * format)
  {
  char * buf;
  const char * end;
  const char * f;
  char * ret = (char*)0;
  char track_format[5];
  f = format;

  while(*f != '\0')
    {
    end = f;
    while((*end != '%') && (*end != '\0'))
      end++;
    if(end != f)
      ret = bg_strncat(ret, f, end);

    if(*end == '%')
      {
      end++;

      /* %p:    Artist */
      if(*end == 'p')
        {
        end++;
        if(info->metadata.artist)
          {
          fprintf(stderr,  "Artist: %s ", info->metadata.artist);
          ret = bg_strcat(ret, info->metadata.artist);
          fprintf(stderr,  "%s\n", ret);
          }
        else
          {
          fprintf(stderr, "No Artist\n");
          goto fail;
          }
        }
      /* %a:    Album */
      else if(*end == 'a')
        {
        end++;
        if(info->metadata.album)
          ret = bg_strcat(ret, info->metadata.album);
        else
          goto fail;
        }
      /* %g:    Genre */
      else if(*end == 'g')
        {
        end++;
        if(info->metadata.genre)
          ret = bg_strcat(ret, info->metadata.genre);
        else
          goto fail;
        }
      /* %t:    Track name */
      else if(*end == 't')
        {
        end++;
        if(info->metadata.title)
          {
          fprintf(stderr, "Ret: %s\n", ret);
          ret = bg_strcat(ret, info->metadata.title);
          fprintf(stderr, "Title: %s\n", info->metadata.title);
          }
        else
          {
          fprintf(stderr, "No Title\n");
          goto fail;
          }
        }
      /* %c:    Comment */
      else if(*end == 'c')
        {
        end++;
        if(info->metadata.comment)
          ret = bg_strcat(ret, info->metadata.comment);
        else
          goto fail;
        }
      /* %y:    Year */
      else if(*end == 'y')
        {
        end++;
        if(info->metadata.year)
          {
          buf = bg_sprintf("%d", info->metadata.year);
          ret = bg_strcat(ret, buf);
          free(buf);
          }
        else
          goto fail;
        }
      /* %<d>n: Track number (d = number of digits, 1-9) */
      else if(isdigit(*end) && end[1] == 'n')
        {
        if(info->metadata.track)
          {
          track_format[0] = '%';
          track_format[1] = '0';
          track_format[2] = *end;
          track_format[3] = 'd';
          track_format[4] = '\0';
          
          buf = bg_sprintf(track_format, info->metadata.track);
          ret = bg_strcat(ret, buf);
          free(buf);
          end+=2;
          }
        else
          goto fail;
        }
      else
        {
        ret = bg_strcat(ret, "%");
        end++;
        }
      f = end;
      }
    }
  return ret;
  fail:
  if(ret)
    free(ret);
  return (char*)0;
  }
