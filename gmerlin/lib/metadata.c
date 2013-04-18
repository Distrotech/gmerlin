/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>
#include <gmerlin/utils.h>

#include <gavl/metatags.h>

#define PARAM_LABEL \
    { \
      .name =      "label", \
      .long_name = TRS("Label"), \
      .type =      BG_PARAMETER_STRING, \
      .help_string = TRS("Used for generation of output file names"),   \
    }

#define PARAM_ARTIST \
    { \
      .name =      "artist", \
      .long_name = TRS("Artist"), \
      .type =      BG_PARAMETER_STRING, \
    }

#define PARAM_ALBUMARTIST \
    { \
      .name =      "albumartist", \
      .long_name = TRS("Album artist"), \
      .type =      BG_PARAMETER_STRING, \
    }

#define PARAM_TITLE \
    { \
      .name =      "title", \
      .long_name = TRS("Title"), \
      .type =      BG_PARAMETER_STRING, \
    }

#define PARAM_ALBUM \
    { \
      .name =      "album", \
      .long_name = TRS("Album"), \
      .type =      BG_PARAMETER_STRING, \
    }

#define PARAM_TRACK \
    { \
      .name =      "track", \
      .long_name = TRS("Track"), \
      .type =      BG_PARAMETER_INT, \
    }

#define PARAM_GENRE \
    { \
      .name =      "genre", \
      .long_name = TRS("Genre"), \
      .type =      BG_PARAMETER_STRING, \
    }

#define PARAM_AUTHOR \
    { \
      .name =      "author", \
      .long_name = TRS("Author"), \
      .type =      BG_PARAMETER_STRING, \
    }

#define PARAM_COPYRIGHT \
    { \
      .name =      "copyright", \
      .long_name = TRS("Copyright"), \
      .type =      BG_PARAMETER_STRING, \
    }

#define PARAM_DATE \
    { \
      .name =        "date", \
      .long_name =   TRS("Year"), \
      .type =        BG_PARAMETER_STRING, \
    }

#define PARAM_COMMENT \
    { \
      .name =      "comment", \
      .long_name = TRS("Comment"), \
      .type =      BG_PARAMETER_STRING, \
    }

static const bg_parameter_info_t parameters[] =
  {
    PARAM_LABEL,
    PARAM_ARTIST,
    PARAM_TITLE,
    PARAM_ALBUM,
    PARAM_TRACK,
    PARAM_GENRE,
    PARAM_AUTHOR,
    PARAM_ALBUMARTIST,
    PARAM_COPYRIGHT,
    PARAM_DATE,
    PARAM_COMMENT,
    { /* End of parameters */ }
  };

static const bg_parameter_info_t parameters_common[] =
  {
    PARAM_ARTIST,
    PARAM_ALBUM,
    PARAM_GENRE,
    PARAM_AUTHOR,
    PARAM_ALBUMARTIST,
    PARAM_COPYRIGHT,
    PARAM_DATE,
    PARAM_COMMENT,
    { /* End of parameters */ }
  };

#define SP_STR(s, gavl_name)                \
  if(!strcmp(ret[i].name, s))               \
    ret[i].val_default.val_str =            \
      gavl_strrep(ret[i].val_default.val_str, \
                gavl_metadata_get(m, gavl_name))

#define SP_INT(s, gavl_name)                        \
  if(!strcmp(ret[i].name, s))                       \
    {                                               \
    if(gavl_metadata_get_int(m, gavl_name, &val_i)) \
      ret[i].val_default.val_i = val_i;             \
    }

static
bg_parameter_info_t * get_parameters(gavl_metadata_t * m, int common)
  {
  int i, year;
  int val_i;
  bg_parameter_info_t * ret;

  ret = bg_parameter_info_copy_array(common ? parameters_common : parameters);

  if(!m)
    return ret;
  
  i = 0;
  while(ret[i].name)
    {
    SP_STR("label", GAVL_META_LABEL);
    SP_STR("artist", GAVL_META_ARTIST);
    SP_STR("albumartist", GAVL_META_ALBUMARTIST);
    SP_STR("title" , GAVL_META_TITLE);
    SP_STR("album", GAVL_META_ALBUM);
    
    SP_INT("track", GAVL_META_TRACKNUMBER);

    if(!strcmp(ret[i].name, "date"))
      {
      year = bg_metadata_get_year(m);
      if(year)
        ret[i].val_default.val_str = bg_sprintf("%d", year);
      }
    SP_STR("genre", GAVL_META_GENRE);
    SP_STR("comment", GAVL_META_COMMENT);

    SP_STR("author", GAVL_META_AUTHOR);
    SP_STR("copyright", GAVL_META_COPYRIGHT);
    
    i++;
    }
  
  return ret;
  }

bg_parameter_info_t * bg_metadata_get_parameters(gavl_metadata_t * m)
  {
  return get_parameters(m, 0);
  }

bg_parameter_info_t * bg_metadata_get_parameters_common(gavl_metadata_t * m)
  {
  return get_parameters(m, 1);
  }

#undef SP_STR
#undef SP_INT

#define SP_STR(s, gavl_name) \
  if(!strcmp(name, s))       \
    { \
    gavl_metadata_set(m, gavl_name, val->val_str); \
    return; \
    }
    
#define SP_INT(s, gavl_name) \
  if(!strcmp(name, s))       \
    { \
    if(val->val_i) \
      gavl_metadata_set_int(m, gavl_name, val->val_i);    \
    return; \
    }

void bg_metadata_set_parameter(void * data, const char * name,
                               const bg_parameter_value_t * val)
  {
  gavl_metadata_t * m = (gavl_metadata_t*)data;
  
  if(!name)
    return;

  SP_STR("artist",      GAVL_META_ARTIST);
  SP_STR("albumartist", GAVL_META_ALBUMARTIST);
  SP_STR("title", GAVL_META_TITLE);
  SP_STR("album", GAVL_META_ALBUM);
  
  SP_INT("track", GAVL_META_TRACKNUMBER);
  SP_STR("date",  GAVL_META_YEAR);
  SP_STR("genre",  GAVL_META_GENRE);
  SP_STR("comment",  GAVL_META_COMMENT);
  
  SP_STR("author",  GAVL_META_AUTHOR);
  SP_STR("copyright",  GAVL_META_COPYRIGHT);
  }

#undef SP_STR
#undef SP_INT

/* Tries to get a 4 digit year from an arbitrary formatted
   date string.
   Return 0 is this wasn't possible.
*/

static int check_year(const char * pos1)
  {
  if(isdigit(pos1[0]) &&
     isdigit(pos1[1]) &&
     isdigit(pos1[2]) &&
     isdigit(pos1[3]))
    {
    return
      (int)(pos1[0] -'0') * 1000 + 
      (int)(pos1[1] -'0') * 100 + 
      (int)(pos1[2] -'0') * 10 + 
      (int)(pos1[3] -'0');
    }
  return 0;
  }

int bg_metadata_get_year(const gavl_metadata_t * m)
  {
  int result;

  const char * pos1;

  pos1 = gavl_metadata_get(m, GAVL_META_YEAR);
  if(pos1)
    return atoi(pos1);

  pos1 = gavl_metadata_get(m, GAVL_META_DATE);
  
  if(!pos1)
    return 0;

  while(1)
    {
    /* Skip nondigits */
    
    while(!isdigit(*pos1) && (*pos1 != '\0'))
      pos1++;
    if(*pos1 == '\0')
      return 0;

    /* Check if we have a 4 digit number */
    result = check_year(pos1);
    if(result)
      return result;

    /* Skip digits */

    while(isdigit(*pos1) && (*pos1 != '\0'))
      pos1++;
    if(*pos1 == '\0')
      return 0;
    }
  return 0;
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

char * bg_create_track_name(const gavl_metadata_t * metadata,
                            const char * format)
  {
  int tag_i;
  char * buf;
  const char * end;
  const char * f;
  const char * tag;
  
  char * ret = NULL;
  char track_format[5];
  f = format;

  while(*f != '\0')
    {
    end = f;
    while((*end != '%') && (*end != '\0'))
      end++;
    if(end != f)
      ret = gavl_strncat(ret, f, end);

    if(*end == '%')
      {
      end++;

      /* %p:    Artist */
      if(*end == 'p')
        {
        end++;
        tag = gavl_metadata_get(metadata, GAVL_META_ARTIST);
        if(tag)
          ret = gavl_strcat(ret, tag);
        else
          goto fail;
        }
      /* %a:    Album */
      else if(*end == 'a')
        {
        end++;
        tag = gavl_metadata_get(metadata, GAVL_META_ALBUM);
        if(tag)
          ret = gavl_strcat(ret, tag);
        else
          goto fail;
        }
      /* %g:    Genre */
      else if(*end == 'g')
        {
        end++;
        tag = gavl_metadata_get(metadata, GAVL_META_GENRE);
        if(tag)
          ret = gavl_strcat(ret, tag);
        else
          goto fail;
        }
      /* %t:    Track name */
      else if(*end == 't')
        {
        end++;
        tag = gavl_metadata_get(metadata, GAVL_META_TITLE);
        if(tag)
          ret = gavl_strcat(ret, tag);
        else
          goto fail;
        }
      /* %c:    Comment */
      else if(*end == 'c')
        {
        end++;
        tag = gavl_metadata_get(metadata, GAVL_META_COMMENT);
        if(tag)
          ret = gavl_strcat(ret, tag);
        else
          goto fail;
        }
      /* %y:    Year */
      else if(*end == 'y')
        {
        end++;
        tag_i = bg_metadata_get_year(metadata);
        if(tag_i > 0)
          {
          buf = bg_sprintf("%d", tag_i);
          ret = gavl_strcat(ret, buf);
          free(buf);
          }
        else
          goto fail;
        }
      /* %<d>n: Track number (d = number of digits, 1-9) */
      else if(isdigit(*end) && end[1] == 'n')
        {
        if(gavl_metadata_get_int(metadata, GAVL_META_TRACKNUMBER, &tag_i))
          {
          track_format[0] = '%';
          track_format[1] = '0';
          track_format[2] = *end;
          track_format[3] = 'd';
          track_format[4] = '\0';
          
          buf = bg_sprintf(track_format, tag_i);
          ret = gavl_strcat(ret, buf);
          free(buf);
          end+=2;
          }
        else
          goto fail;
        }
      else
        {
        ret = gavl_strcat(ret, "%");
        end++;
        }
      f = end;
      }
    }
  return ret;
  fail:
  if(ret)
    free(ret);
  return NULL;
  }

#define META_STRCAT() ret = gavl_strcat(ret, tmp); free(tmp)

char * bg_metadata_to_string(const gavl_metadata_t * m, int use_tabs)
  {
  int i;
  char * ret = NULL;
  char * tmp;

  char * sep;

  if(use_tabs)
    sep = ":\t ";
  else
    sep = ": ";

  for(i = 0; i < m->num_tags; i++)
    {
    tmp = bg_sprintf("%s%s%s\n", m->tags[i].key, sep, m->tags[i].val);
    META_STRCAT();
    }
  /* Remove trailing '\n' */
  if(ret)
    ret[strlen(ret) - 1] = '\0';
  return ret;
  }

void bg_metadata_date_now(gavl_metadata_t * m, const char * key)
  {
  struct tm tm;
  time_t t = time(NULL);
  localtime_r(&t, &tm);
  
  tm.tm_mon++;
  tm.tm_year+=1900;
  gavl_metadata_set_date_time(m, key,
                              tm.tm_year,
                              tm.tm_mon,
                              tm.tm_mday,
                              tm.tm_hour,
                              tm.tm_min,
                              tm.tm_sec);
  }
