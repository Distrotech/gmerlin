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

#define _GNU_SOURCE

#include <config.h>


#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <language_table.h>
#include <wctype.h>
#include <errno.h>
#include <limits.h>

/* stat stuff */
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <gmerlin/charset.h>

#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "utils"

#include <gmerlin/translation.h>

#include <gavl/metatags.h>


#include <md5.h>


char * bg_fix_path(char * path)
  {
  char * ret;
  int len;
  
  if(!path)
    return path;

  len = strlen(path);
  
  if(!len)
    {
    free(path);
    return NULL;
    }
  if(path[len-1] != '/')
    {
    ret = malloc(len+2);
    strcpy(ret, path);
    free(path);
    ret[len] = '/';
    ret[len+1] = '\0';
    
    return ret;
    }
  else
    return path;
  }

char * bg_strdup(char * old_string, const char * new_string)
  {
  char * ret;
  int len;
  if(!new_string || (*new_string == '\0'))
    {
    if(old_string)
      free(old_string);
    return NULL;
    }

  if(old_string)
    {
    if(!strcmp(old_string, new_string))
      return old_string;
    else
      free(old_string);
    }
  len = ((strlen(new_string)+1 + 3) / 4) * 4 ;
  
  ret = malloc(len);
  strcpy(ret, new_string);
  return ret;
  }

char * bg_strndup(char * old_string,
                  const char * new_string_start,
                  const char * new_string_end)
  {
  char * ret;
  if(!new_string_start || (*new_string_start == '\0'))
    {
    if(old_string)
      free(old_string);
    return NULL;
    }

  if(old_string)
    {
    if(!strncmp(old_string, new_string_start,
                new_string_end - new_string_start))
      return old_string;
    else
      free(old_string);
    }
  ret = malloc(new_string_end - new_string_start + 1);
  strncpy(ret, new_string_start, new_string_end - new_string_start);
  ret[new_string_end - new_string_start] = '\0';
  return ret;
  }

char * bg_sprintf(const char * format,...)
  {
  va_list argp; /* arg ptr */
#ifndef HAVE_VASPRINTF
  int len;
#endif
  char * ret;
  va_start( argp, format);

#ifndef HAVE_VASPRINTF
  len = vsnprintf(NULL, 0, format, argp);
  ret = malloc(len+1);
  vsnprintf(ret, len+1, format, argp);
#else
  vasprintf(&ret, format, argp);
#endif
  va_end(argp);
  return ret;
  }
  
char * bg_create_unique_filename(char * template)
  {
  char * filename;
  struct stat stat_buf;
  FILE * file;
  int err = 0;
  uint32_t count;

  count = 0;

  filename = bg_sprintf(template, 0);

  while(1)
    {
    
    if(stat(filename, &stat_buf) == -1)
      {
      /* Create empty file */
      file = fopen(filename, "w");
      if(file)
        {
        fclose(file);
        }
      else
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open file \"%s\" for writing",
               filename);
        err = 1;
        }
      if(err)
        {
        free(filename);
        return NULL;
        }
      else
        return filename;
      }
    count++;
    sprintf(filename, template, count);
    }
  }

char * bg_strcat(char * old_string, const char * tail)
  {
  if(!old_string)
    return bg_strdup(NULL, tail);

  old_string = realloc(old_string, strlen(old_string) + strlen(tail) + 1);
  strcat(old_string, tail);
  return old_string;
  }

char * bg_strncat(char * old_string, const char * start, const char * end)
  {
  int old_len;
  
  if(!old_string)
    return bg_strndup(NULL, start, end);

  old_len = strlen(old_string);
  
  old_string = realloc(old_string, old_len + end - start + 1);
  strncpy(&old_string[old_len], start, end - start);
  old_string[old_len + end - start] = '\0';
  return old_string;
  }

char ** bg_strbreak(const char * str, char delim)
  {
  int num_entries;
  char *pos, *end = NULL;
  const char *pos_c;
  char ** ret;
  int i;
  if(!str || (*str == '\0'))
    return NULL;
    
  pos_c = str;
  
  num_entries = 1;
  while((pos_c = strchr(pos_c, delim)))
    {
    num_entries++;
    pos_c++;
    }
  ret = calloc(num_entries+1, sizeof(char*));

  ret[0] = bg_strdup(NULL, str);
  pos = ret[0];
  for(i = 0; i < num_entries; i++)
    {
    if(i)
      ret[i] = pos;
    if(i < num_entries-1)
      {
      end = strchr(pos, delim);
      *end = '\0';
      }
    end++;
    pos = end;
    }
  return ret;
  }

void bg_strbreak_free(char ** retval)
  {
  free(retval[0]);
  free(retval);
  }

int bg_string_is_url(const char * str)
  {
  const char * pos, * end_pos;
  pos = str;
  end_pos = strstr(str, "://");

  if(!end_pos)
    return 0;
  
  while(pos != end_pos)
    {
    if(!isalnum(*pos))
      return 0;
    pos++;
    }
  return 1;
  }

int bg_url_split(const char * url,
                 char ** protocol,
                 char ** user,
                 char ** password,
                 char ** hostname,
                 int * port,
                 char ** path)
  {
  const char * pos1;
  const char * pos2;

  /* For detecting user:pass@blabla.com/file */

  const char * colon_pos;
  const char * at_pos;
  const char * slash_pos;
  
  pos1 = url;

  /* Sanity check */
  
  pos2 = strstr(url, "://");
  if(!pos2)
    return 0;

  /* Protocol */
    
  if(protocol)
    *protocol = bg_strndup(NULL, pos1, pos2);

  pos2 += 3;
  pos1 = pos2;

  /* Check for user and password */

  colon_pos = strchr(pos1, ':');
  at_pos = strchr(pos1, '@');
  slash_pos = strchr(pos1, '/');

  if(colon_pos && at_pos && at_pos &&
     (colon_pos < at_pos) && 
     (at_pos < slash_pos))
    {
    if(user)
      *user = bg_strndup(NULL, pos1, colon_pos);
    pos1 = colon_pos + 1;
    if(password)
      *password = bg_strndup(NULL, pos1, at_pos);
    pos1 = at_pos + 1;
    pos2 = pos1;
    }
  
  /* Hostname */

  while((*pos2 != '\0') && (*pos2 != ':') && (*pos2 != '/'))
    pos2++;

  if(hostname)
    *hostname = bg_strndup(NULL, pos1, pos2);

  switch(*pos2)
    {
    case '\0':
      if(port)
        *port = -1;
      return 1;
      break;
    case ':':
      /* Port */
      pos2++;
      if(port)
        *port = atoi(pos2);
      while(isdigit(*pos2))
        pos2++;
      break;
    default:
      if(port)
        *port = -1;
      break;
    }

  if(path)
    {
    pos1 = pos2;
    pos2 = pos1 + strlen(pos1);
    if(pos1 != pos2)
      *path = bg_strndup(NULL, pos1, pos2);
    else
      *path = NULL;
    }
  return 1;
  }



/* Scramble and descramble password (taken from gftp) */

char * bg_scramble_string(const char * str)
  {
  char *newstr, *newpos;
  
  newstr = malloc (strlen (str) * 2 + 2);
  newpos = newstr;
  
  *newpos++ = '$';

  while (*str != 0)
    {
    *newpos++ = ((*str >> 2) & 0x3c) | 0x41;
    *newpos++ = ((*str << 2) & 0x3c) | 0x41;
    str++;
    }
  *newpos = 0;

  return (newstr);
  }

char * bg_descramble_string(const char *str)
  {
  const char *strpos;
  char *newstr, *newpos;
  int error;
  
  if (*str != '$')
    return (bg_strdup (NULL, str));
  
  strpos = str + 1;
  newstr = malloc (strlen (strpos) / 2 + 1);
  newpos = newstr;
  
  error = 0;
  while (*strpos != '\0' && (*strpos + 1) != '\0')
    {
    if ((*strpos & 0xc3) != 0x41 ||
        (*(strpos + 1) & 0xc3) != 0x41)
      {
      error = 1;
      break;
      }
    
    *newpos++ = ((*strpos & 0x3c) << 2) |
      ((*(strpos + 1) & 0x3c) >> 2);
    
    strpos += 2;
    }
  
  if(error)
    {
    free (newstr);
    return (bg_strdup(NULL, str));
    }
  
  *newpos = '\0';
  return (newstr);
  }

const char * bg_get_language_name(const char * iso)
  {
  int i = 0;
  while(bg_language_codes[i])
    {
    if((bg_language_codes[i][0] == iso[0]) &&
       (bg_language_codes[i][1] == iso[1]) &&
       (bg_language_codes[i][2] == iso[2]))
      return bg_language_labels[i];
    i++;
    }
  return NULL;
  }

int bg_string_match(const char * key,
                    const char * key_list)
  {
  const char * pos;
  const char * end;

  pos = key_list;
      

  if(!key_list)
    return 0;
  
  while(1)
    {
    end = pos;
    while(!isspace(*end) && (*end != '\0'))
      end++;
    if(end == pos)
      break;

    if((strlen(key) == (int)(end-pos)) &&
       !strncasecmp(pos, key, (int)(end-pos)))
      {
      return 1;
      }
    pos = end;
    if(pos == '\0')
      break;
    else
      {
      while(isspace(*pos) && (pos != '\0'))
        pos++;
      }
    }
  return 0;
  }

/* Used mostly for generating manual pages,
   it's horribly inefficient */

char * bg_toupper(const char * str)
  {
  char * tmp_string_1;
  char * tmp_string_2;
  char * ret;
  int len;
  wchar_t * pos_1, * pos_2;
  
  bg_charset_converter_t * cnv1;
  bg_charset_converter_t * cnv2;
  
  cnv1 = bg_charset_converter_create("UTF-8", "WCHAR_T");
  cnv2 = bg_charset_converter_create("WCHAR_T", "UTF-8");
  
  tmp_string_1 = bg_convert_string(cnv1, str, -1, &len);

  tmp_string_2 = malloc(len + 4);

  pos_1 = (wchar_t*)tmp_string_1;
  pos_2 = (wchar_t*)tmp_string_2;

  while(*pos_1)
    {
    *pos_2 = towupper(*pos_1);
    pos_1++;
    pos_2++;
    }
  *pos_2 = 0;

  ret = bg_convert_string(cnv2, tmp_string_2, len, NULL);

  free(tmp_string_1);
  free(tmp_string_2);
  bg_charset_converter_destroy(cnv1);
  bg_charset_converter_destroy(cnv2);
  return ret;
  }

void bg_get_filename_hash(const char * gml, char ret[33])
  {
  char * uri;
  uint8_t md5sum[16];
    
  uri = bg_string_to_uri(gml, -1);
  
  bg_md5_buffer(uri, strlen(uri), md5sum);
  sprintf(ret,
          "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
          md5sum[0], md5sum[1], md5sum[2], md5sum[3], 
          md5sum[4], md5sum[5], md5sum[6], md5sum[7], 
          md5sum[8], md5sum[9], md5sum[10], md5sum[11], 
          md5sum[12], md5sum[13], md5sum[14], md5sum[15]);
  free(uri);
  }

void bg_dprintf(const char * format, ...)
  {
  va_list argp; /* arg ptr */
  va_start( argp, format);
  vfprintf(stderr, format, argp);
  va_end(argp);
  }

void bg_diprintf(int indent, const char * format, ...)
  {
  int i;
  va_list argp; /* arg ptr */
  for(i = 0; i < indent; i++)
    bg_dprintf( " ");
  
  va_start( argp, format);
  vfprintf(stderr, format, argp);
  va_end(argp);
  }

char * bg_filename_ensure_extension(const char * filename,
                                    const char * ext)
  {
  const char * pos;

  if((pos = strrchr(filename, '.')) &&
     (!strcasecmp(pos+1, ext)))
    return bg_strdup(NULL, filename);
  else
    return bg_sprintf("%s.%s", filename, ext);
  }

char * bg_get_stream_label(int index, const gavl_metadata_t * m)
  {
  char * label;
  const char * info;
  const char * language;
  
  info = gavl_metadata_get(m, GAVL_META_LABEL);
  language = gavl_metadata_get(m, GAVL_META_LANGUAGE);
  
  if(info && language)
    label = bg_sprintf("%s [%s]", info, bg_get_language_name(language));
  else if(info)
    label = bg_sprintf("%s", info);
  else if(language)
    label = bg_sprintf(TR("Stream %d [%s]"), index+1, bg_get_language_name(language));
  else
    label = bg_sprintf(TR("Stream %d"), index+1);
  return label;
  }

char * bg_canonical_filename(const char * name)
  {
#ifdef HAVE_CANONICALIZE_FILE_NAME
  return canonicalize_file_name(name);
#else
  char * ret = malloc(PATH_MAX);
  realpath(name, ret);
  return ret;
#endif
  }


static const struct
  {
  const char * bcode;
  const char * tcode;
  }
iso639tab[] =
  {
    { "alb", "sqi" },
    { "arm", "hye" },
    { "baq", "eus" },
    { "bur", "mya" },
    { "chi", "zho" },
    { "cze", "ces" },
    { "dut", "nld" },
    { "fre", "fra" },
    { "geo", "kat" },
    { "ger", "deu" },
    { "gre", "ell" },
    { "ice", "isl" },
    { "mac", "mkd" },
    { "mao", "mri" },
    { "may", "msa" },
    { "per", "fas" },
    { "rum", "ron" },
    { "slo", "slk" },
    { "tib", "bod" },
    { "wel", "cym" },
    { /* End */    }
  };

const char * bg_iso639_b_to_t(const char * code)
  {
  int i = 0;
  while(iso639tab[i].bcode)
    {
    if(!strcmp(code, iso639tab[i].bcode))
      return iso639tab[i].tcode;
    i++;
    }
  return code;
  }
