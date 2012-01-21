/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>
#include <gmerlin/utils.h>

#define MY_FREE(ptr) \
  if(ptr) \
    { \
    free(ptr); \
    ptr = NULL; \
    }

void bg_metadata_free(bg_metadata_t * m)
  {
  MY_FREE(m->artist);
  MY_FREE(m->title);
  MY_FREE(m->album);
  MY_FREE(m->genre);
  MY_FREE(m->comment);
  MY_FREE(m->author);
  MY_FREE(m->copyright);
  MY_FREE(m->date);

  if(m->ext)
    {
    int i = 0;
    while(m->ext[i].key)
      {
      MY_FREE(m->ext[i].key);
      MY_FREE(m->ext[i].value);
      i++;
      }
    free(m->ext);
    }
  
  memset(m, 0, sizeof(*m));
  }

#define MY_STRCPY(s) \
  dst->s = bg_strdup(dst->s, src->s);

#define MY_INTCOPY(i) \
  dst->i = src->i;

void bg_metadata_copy(bg_metadata_t * dst, const bg_metadata_t * src)
  {
  MY_STRCPY(artist);
  MY_STRCPY(title);
  MY_STRCPY(album);

  MY_STRCPY(date);
  MY_STRCPY(genre);
  MY_STRCPY(comment);

  MY_STRCPY(author);
  MY_STRCPY(copyright);
  MY_INTCOPY(track);

  if(src->ext)
    {
    int num = 0;
    int i;
    while(src->ext[num].key)
      num++;
    dst->ext = calloc(num+1, sizeof(*dst->ext));
    for(i = 0; i < num; i++)
      {
      dst->ext[i].key   = bg_strdup(dst->ext[i].key, src->ext[i].key);
      dst->ext[i].value = bg_strdup(dst->ext[i].value, src->ext[i].value);
      }
    }
  }

#undef MY_STRCPY
#undef MY_INTCPY

#define PARAM_ARTIST \
    { \
      .name =      "artist", \
      .long_name = TRS("Artist"), \
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
      .long_name =   TRS("Date"), \
      .type =        BG_PARAMETER_STRING, \
      .help_string = TRS("Complete date or year only") \
    }

#define PARAM_COMMENT \
    { \
      .name =      "comment", \
      .long_name = TRS("Comment"), \
      .type =      BG_PARAMETER_STRING, \
    }

static const bg_parameter_info_t parameters[] =
  {
    PARAM_ARTIST,
    PARAM_TITLE,
    PARAM_ALBUM,
    PARAM_TRACK,
    PARAM_GENRE,
    PARAM_AUTHOR,
    PARAM_COPYRIGHT,
    PARAM_DATE,
    PARAM_COMMENT,
    { /* End of parameters */ }
  };

static const bg_parameter_info_t parameters_common[] =
  {
    PARAM_ARTIST,
    // PARAM_TITLE,
    PARAM_ALBUM,
    // PARAM_TRACK,
    PARAM_GENRE,
    PARAM_AUTHOR,
    PARAM_COPYRIGHT,
    PARAM_DATE,
    PARAM_COMMENT,
    { /* End of parameters */ }
  };

#define SP_STR(s) if(!strcmp(ret[i].name, # s)) \
    ret[i].val_default.val_str = bg_strdup(ret[i].val_default.val_str, m->s)

#define SP_INT(s) if(!strcmp(ret[i].name, # s)) \
    ret[i].val_default.val_i = m->s;

static
bg_parameter_info_t * get_parameters(bg_metadata_t * m, int common)
  {
  int i;
  
  bg_parameter_info_t * ret;

  ret = bg_parameter_info_copy_array(common ? parameters_common : parameters);

  if(!m)
    return ret;
  
  i = 0;
  while(ret[i].name)
    {
    SP_STR(artist);
    SP_STR(title);
    SP_STR(album);
      
    SP_INT(track);
    SP_STR(date);
    SP_STR(genre);
    SP_STR(comment);

    SP_STR(author);
    SP_STR(copyright);

    i++;
    }
  return ret;
  }

bg_parameter_info_t * bg_metadata_get_parameters(bg_metadata_t * m)
  {
  return get_parameters(m, 0);
  }

bg_parameter_info_t * bg_metadata_get_parameters_common(bg_metadata_t * m)
  {
  return get_parameters(m, 1);
  }

#undef SP_STR
#undef SP_INT

#define SP_STR(s) if(!strcmp(name, # s))                               \
    { \
    m->s = bg_strdup(m->s, val->val_str); \
    return; \
    }
    
#define SP_INT(s) if(!strcmp(name, # s)) \
    { \
    m->s = val->val_i; \
    }

void bg_metadata_set_parameter(void * data, const char * name,
                               const bg_parameter_value_t * val)
  {
  bg_metadata_t * m = (bg_metadata_t*)data;
  
  if(!name)
    return;

  SP_STR(artist);
  SP_STR(title);
  SP_STR(album);
  
  SP_INT(track);
  SP_STR(date);
  SP_STR(genre);
  SP_STR(comment);
  
  SP_STR(author);
  SP_STR(copyright);
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

int bg_metadata_get_year(const bg_metadata_t * m)
  {
  int result;

  const char * pos1;
  
  if(!m->date)
    return 0;

  pos1 = m->date;

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

char * bg_create_track_name(const bg_metadata_t * metadata,
                            const char * format)
  {
  char * buf;
  const char * end;
  const char * f;
  char * ret = NULL;
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
        if(metadata->artist)
          {
          ret = bg_strcat(ret, metadata->artist);
          }
        else
          {
          goto fail;
          }
        }
      /* %a:    Album */
      else if(*end == 'a')
        {
        end++;
        if(metadata->album)
          ret = bg_strcat(ret, metadata->album);
        else
          goto fail;
        }
      /* %g:    Genre */
      else if(*end == 'g')
        {
        end++;
        if(metadata->genre)
          ret = bg_strcat(ret, metadata->genre);
        else
          goto fail;
        }
      /* %t:    Track name */
      else if(*end == 't')
        {
        end++;
        if(metadata->title)
          {
          ret = bg_strcat(ret, metadata->title);
          }
        else
          {
          goto fail;
          }
        }
      /* %c:    Comment */
      else if(*end == 'c')
        {
        end++;
        if(metadata->comment)
          ret = bg_strcat(ret, metadata->comment);
        else
          goto fail;
        }
      /* %y:    Year */
      else if(*end == 'y')
        {
        end++;
        if(metadata->date)
          {
          buf = bg_sprintf("%d", bg_metadata_get_year(metadata));
          ret = bg_strcat(ret, buf);
          free(buf);
          }
        else
          goto fail;
        }
      /* %<d>n: Track number (d = number of digits, 1-9) */
      else if(isdigit(*end) && end[1] == 'n')
        {
        if(metadata->track)
          {
          track_format[0] = '%';
          track_format[1] = '0';
          track_format[2] = *end;
          track_format[3] = 'd';
          track_format[4] = '\0';
          
          buf = bg_sprintf(track_format, metadata->track);
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
  return NULL;
  }

#define META_STRCAT() ret = bg_strcat(ret, tmp); free(tmp)

char * bg_metadata_to_string(const bg_metadata_t * m, int use_tabs)
  {
  char * ret = NULL;
  char * tmp;

  char * sep;

  if(use_tabs)
    sep = ":\t ";
  else
    sep = ": ";
  
  if(m->author)
    {
    tmp = bg_sprintf(TR("Author%s%s\n"), sep, m->author);
    META_STRCAT();
    }
  if(m->artist)
    {
    tmp = bg_sprintf(TR("Artist%s%s\n"), sep, m->artist);
    META_STRCAT();
    }
  if(m->title)
    {
    tmp = bg_sprintf(TR("Title%s%s\n"), sep, m->title);
    META_STRCAT();
    }
  if(m->album)
    {
    tmp = bg_sprintf(TR("Album%s%s\n"), sep, m->album);
    META_STRCAT();
    }
  if(m->copyright)
    {
    tmp = bg_sprintf(TR("Copyright%s%s\n"), sep, m->copyright);
    META_STRCAT();
    }
  if(m->genre)
    {
    tmp = bg_sprintf(TR("Genre%s%s\n"), sep, m->genre);
    META_STRCAT();
    }
  if(m->date)
    {
    tmp = bg_sprintf(TR("Date%s%s\n"), sep, m->date);
    META_STRCAT();
    }
  if(m->track)
    {
    tmp = bg_sprintf(TR("Track%s%d\n"), sep, m->track);
    META_STRCAT();
    }
  if(m->comment)
    {
    tmp = bg_sprintf(TR("Comment%s%s\n"), sep, m->comment);
    META_STRCAT();
    }
  /* Remove trailing '\n' */
  if(ret)
    ret[strlen(ret) - 1] = '\0';
  return ret;
  }

void bg_metadata_append_ext(bg_metadata_t * m, const char * key, const char * value)
  {
  int num_ext = 0;

  if(m->ext)
    {
    while(m->ext[num_ext].key)
      num_ext++;
    }

  m->ext = realloc(m->ext, (num_ext+2) * sizeof(*m->ext));

  memset(m->ext + num_ext, 0, 2 * sizeof(*m->ext));
  
  m->ext[num_ext].key = bg_strdup(m->ext[num_ext].key, key);
  m->ext[num_ext].value = bg_strdup(m->ext[num_ext].value, value);
  }

void bg_metadata_dump(const bg_metadata_t * m)
  {
  int len, max_len, i;
  bg_dprintf("Metadata:\n");

  if(m->artist)    bg_diprintf(2, "Artist:    %s\n", m->artist);
  if(m->title)     bg_diprintf(2, "Title:     %s\n", m->title);
  if(m->album)     bg_diprintf(2, "Album:     %s\n", m->album);
  if(m->track)     bg_diprintf(2, "Track:     %d\n", m->track);
  if(m->date)      bg_diprintf(2, "Date:      %s\n", m->date);
  if(m->genre)     bg_diprintf(2, "Genre:     %s\n", m->genre);
  if(m->comment)   bg_diprintf(2, "Comment:   %s\n", m->comment);
  if(m->author)    bg_diprintf(2, "Author:    %s\n", m->author);
  if(m->copyright) bg_diprintf(2, "Copyright: %s\n", m->copyright);

  if(m->ext)
    {
    i = 0;
    max_len = 0;

    bg_diprintf(2, "Extended metadata:\n");
    
    while(m->ext[i].key)
      {
      len = strlen(m->ext[i].key);
      if(len > max_len)
        max_len = len;
      i++;
      }
    
    i = 0;

    while(m->ext[i].key)
      {
      bg_diprintf(4, "%s: ", m->ext[i].key);
      bg_diprintf(max_len - strlen(m->ext[i].key), "%s\n", m->ext[i].value);
      i++;
      }
    }
  
  }

#define CMP_STR(s)                             \
  if((m1->s && !m2->s) || (!m1->s && m2->s) || \
     (m1->s && m2->s && strcmp(m1->s, m2->s))) \
    return 0

#define CMP_INT(s) \
  if(m1->s != m2->s) \
    return 0

int bg_metadata_equal(const bg_metadata_t * m1,
                      const bg_metadata_t * m2)
  {
  CMP_STR(artist);
  CMP_STR(title);
  CMP_STR(album);
  CMP_STR(date);
  CMP_STR(genre);
  CMP_STR(comment);
  CMP_STR(author);
  CMP_STR(copyright);
  CMP_INT(track);
  return 1;
  }
                        
