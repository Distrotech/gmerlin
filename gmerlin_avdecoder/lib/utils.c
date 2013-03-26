/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#define _GNU_SOURCE /* vasprintf */

#include <avdec_private.h>

#define LOG_DOMAIN "utils"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <regex.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#endif

#include <utils.h>

void bgav_dump_fourcc(uint32_t fourcc)
  {
  if((fourcc & 0xffff0000) || !(fourcc))
    bgav_dprintf( "%c%c%c%c (%08x)",
            (fourcc & 0xFF000000) >> 24,
            (fourcc & 0x00FF0000) >> 16,
            (fourcc & 0x0000FF00) >> 8,
            (fourcc & 0x000000FF),
            fourcc);
  else
    bgav_dprintf( "WavID: 0x%04x", fourcc);
    
  }

int bgav_check_fourcc(uint32_t fourcc, const uint32_t * fourccs)
  {
  int i = 0;
  while(fourccs[i])
    {
    if(fourccs[i] == fourcc)
      return 1;
    else
      i++;
    }
  return 0;
  }


void bgav_hexdump(const uint8_t * data, int len, int linebreak)
  {
  int i;
  int bytes_written = 0;
  int imax;
  
  while(bytes_written < len)
    {
    imax = (bytes_written + linebreak > len) ? len - bytes_written : linebreak;
    for(i = 0; i < imax; i++)
      bgav_dprintf( "%02x ", data[bytes_written + i]);
    for(i = imax; i < linebreak; i++)
      bgav_dprintf( "   ");
    for(i = 0; i < imax; i++)
      {
      if((data[bytes_written + i] < 0x7f) && (data[bytes_written + i] >= 32))
        bgav_dprintf( "%c", data[bytes_written + i]);
      else
        bgav_dprintf( ".");
      }
    bytes_written += imax;
    bgav_dprintf( "\n");
    }
  }

char * bgav_sprintf(const char * format,...)
  {
  va_list argp; /* arg ptr */
#ifndef HAVE_VASPRINTF
  int len;
#else
  int result;
#endif
  char * ret;
  va_start( argp, format);

#ifndef HAVE_VASPRINTF
  len = vsnprintf(NULL, 0, format, argp);
  ret = malloc(len+1);
  vsnprintf(ret, len+1, format, argp);
#else
  result = vasprintf(&ret, format, argp);
#endif
  va_end(argp);
  return ret;
  }

void bgav_dprintf(const char * format, ...)
  {
  va_list argp; /* arg ptr */
  va_start( argp, format);
  vfprintf(stderr, format, argp);
  va_end(argp);
  }

void bgav_diprintf(int indent, const char * format, ...)
  {
  int i;
  va_list argp; /* arg ptr */
  for(i = 0; i < indent; i++)
    bgav_dprintf( " ");
  
  va_start( argp, format);
  vfprintf(stderr, format, argp);
  va_end(argp);
  }



char * bgav_strndup(const char * start, const char * end)
  {
  char * ret;
  int len;

  if(!start)
    return NULL;

  len = (end) ? (end - start) : strlen(start);
  ret = malloc(len+1);
  strncpy(ret, start, len);
  ret[len] = '\0';
  return ret;
  }

char * bgav_strdup(const char * str)
  {
  return (bgav_strndup(str, NULL));
  }

char * bgav_strncat(char * old, const char * start, const char * end)
  {
  int len, old_len;
  old_len = old ? strlen(old) : 0;
  
  len = (end) ? (end - start) : strlen(start);
  old = realloc(old, len + old_len + 1);
  strncpy(old + old_len, start, len);
  old[old_len + len] = '\0';
  return old;
  }

static char * remove_spaces(char * old)
  {
  char * pos1, *pos2, *ret;
  int num_spaces = 0;

  pos1 = old;
  while(*pos1 != '\0')
    {
    if(*pos1 == ' ')
      num_spaces++;
    pos1++;
    }
  if(!num_spaces)
    return old;
  
  ret = malloc(strlen(old) + 1 + 2 * num_spaces);

  pos1 = old;
  pos2 = ret;

  while(*pos1 != '\0')
    {
    if(*pos1 == ' ')
      {
      *(pos2++) = '%';
      *(pos2++) = '2';
      *(pos2++) = '0';
      }
    else
      *(pos2++) = *pos1;
    pos1++;
    }
  *pos2 = '\0';
  free(old);
  return ret;
  }

/* Split an URL, returned pointers should be free()d after */

int bgav_url_split(const char * url,
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
    *protocol = bgav_strndup(pos1, pos2);

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
      *user = bgav_strndup(pos1, colon_pos);
    pos1 = colon_pos + 1;
    if(password)
      *password = bgav_strndup(pos1, at_pos);
    pos1 = at_pos + 1;
    pos2 = pos1;
    }
  
  /* Hostname */

  while((*pos2 != '\0') && (*pos2 != ':') && (*pos2 != '/'))
    pos2++;

  if(hostname)
    *hostname = bgav_strndup(pos1, pos2);

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
      *path = bgav_strndup(pos1, pos2);
    else
      *path = NULL;
    }

  /* Fix whitespaces in path */
  if(path && *path)
    *path = remove_spaces(*path);
  return 1;
  }

/*
 *  Read a single line from a filedescriptor
 *
 *  ret will be reallocated if neccesary and ret_alloc will
 *  be updated then
 *
 *  The string will be 0 terminated, a trailing \r or \n will
 *  be removed
 */
#define BYTES_TO_ALLOC 1024

int bgav_read_line_fd(const bgav_options_t * opt,
                      int fd, char ** ret, int * ret_alloc, int milliseconds)
  {
  char * pos;
  char c;
  int bytes_read;
  bytes_read = 0;
  /* Allocate Memory for the case we have none */
  if(!(*ret_alloc))
    {
    *ret_alloc = BYTES_TO_ALLOC;
    *ret = realloc(*ret, *ret_alloc);
    **ret = '\0';
    }
  pos = *ret;
  while(1)
    {
    if(!bgav_read_data_fd(opt, fd, (uint8_t*)(&c), 1, milliseconds))
      {
      if(!bytes_read)
        return 0;
      break;
      }
    /*
     *  Line break sequence
     *  is starting, remove the rest from the stream
     */
    if(c == '\n')
      {
      break;
      }
    /* Reallocate buffer */
    else if(c != '\r')
      {
      if(bytes_read+2 >= *ret_alloc)
        {
        *ret_alloc += BYTES_TO_ALLOC;
        *ret = realloc(*ret, *ret_alloc);
        pos = &((*ret)[bytes_read]);
        }
      /* Put the byte and advance pointer */
      *pos = c;
      pos++;
      bytes_read++;
      }
    }
  *pos = '\0';
  return 1;
  }

int bgav_read_data_fd(const bgav_options_t * opt,
                      int fd, uint8_t * ret, int len, int milliseconds)
  {
  int bytes_read = 0;
  int result;
  fd_set rset;
  struct timeval timeout;

  int flags = 0;


  //  if(milliseconds < 0)
  //    flags = MSG_WAITALL;
  
  while(bytes_read < len)
    {
    if(milliseconds >= 0)
      { 
      FD_ZERO (&rset);
      FD_SET  (fd, &rset);
     
      timeout.tv_sec  = milliseconds / 1000;
      timeout.tv_usec = (milliseconds % 1000) * 1000;
    
      if((result = select (fd+1, &rset, NULL, NULL, &timeout)) <= 0)
        {
//        fprintf(stderr, "Read timed out %d\n", milliseconds);
        return bytes_read;
        }
      }

    result = recv(fd, ret + bytes_read, len - bytes_read, flags);
    if(result > 0)
      bytes_read += result;
    else if(result <= 0)
      {
      if(result <0)
        bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "recv failed: %s", strerror(errno));
      return bytes_read;
      }
    }
  return bytes_read;
  }

/* Break string into substrings separated by spaces */

char ** bgav_stringbreak(const char * str, char sep)
  {
  int len, i;
  int num;
  int index;
  char ** ret;
  len = strlen(str);
  if(!len)
    {
    num = 0;
    return NULL;
    }
  
  num = 1;
  for(i = 0; i < len; i++)
    {
    if(str[i] == sep)
      num++;
    }
  ret = calloc(num+1, sizeof(char*));

  index = 1;
  ret[0] = bgav_strdup(str);
  
  for(i = 0; i < len; i++)
    {
    if(ret[0][i] == sep)
      {
      if(index < num)
        ret[index] = ret[0] + i + 1;
      ret[0][i] = '\0';
      index++;
      }
    }
  return ret;
  }

void bgav_stringbreak_free(char ** str)
  {
  free(str[0]);
  free(str);
  }

int bgav_slurp_file(const char * location,
                    bgav_packet_t * p,
                    const bgav_options_t * opt)
  {
  bgav_input_context_t * input;
  input = bgav_input_create(opt);
  if(!bgav_input_open(input, location))
    {
    bgav_input_destroy(input);
    return 0;
    }
  if(!input->total_bytes)
    {
    bgav_input_destroy(input);
    return 0;
    }

  bgav_packet_alloc(p, input->total_bytes);
  
  if(bgav_input_read_data(input, p->data, input->total_bytes) <
     input->total_bytes)
    {
    bgav_input_destroy(input);
    return 0;
    }
  p->data_size = input->total_bytes;
  bgav_input_destroy(input);
  return 1;
  }

int bgav_check_file_read(const char * filename)
  {
  if(access(filename, R_OK))
    return 0;
  return 1;
  }

/* Search file for writing */

char * bgav_search_file_write(const bgav_options_t * opt,
                              const char * directory, const char * file)
  {
  char * home_dir;
  char * testpath;
  char * pos1;
  char * pos2;
  FILE * testfile;

  if(!file)
    return NULL;
  
  testpath = malloc((PATH_MAX+1) * sizeof(char));
  
  home_dir = getenv("HOME");

  /* Try to open the file */

  snprintf(testpath, PATH_MAX,
           "%s/.%s/%s/%s", home_dir, PACKAGE, directory, file);
  testfile = fopen(testpath, "a");
  if(testfile)
    {
    fclose(testfile);
    return testpath;
    }
  
  if(errno != ENOENT)
    {
    free(testpath);
    return NULL;
    }

  /*
   *  No such file or directory can mean, that the directory 
   *  doesn't exist
   */
  
  pos1 = &testpath[strlen(home_dir)+1];
  
  while(1)
    {
    pos2 = strchr(pos1, '/');

    if(!pos2)
      break;

    *pos2 = '\0';

#ifdef _WIN32
    if(mkdir(testpath) == -1)
#else
    if(mkdir(testpath, S_IRUSR|S_IWUSR|S_IXUSR) == -1)
#endif
      {
      if(errno != EEXIST)
        {
        *pos2 = '/';
        break;
        }
      }
    else
      bgav_log(opt, BGAV_LOG_INFO, LOG_DOMAIN,
               "Created directory %s", testpath);
    
    *pos2 = '/';
    pos1 = pos2;
    pos1++;
    }

  /* Try once more to open the file */

  testfile = fopen(testpath, "w");

  if(testfile)
    {
    fclose(testfile);
    return testpath;
    }
  free(testpath);
  return NULL;
  }

char * bgav_search_file_read(const bgav_options_t * opt,
                             const char * directory, const char * file)
  {
  char * home_dir;
  char * test_path;

  home_dir = getenv("HOME");

  test_path = malloc((PATH_MAX+1) * sizeof(*test_path));
  snprintf(test_path, PATH_MAX, "%s/.%s/%s/%s",
           home_dir, PACKAGE, directory, file);

  if(bgav_check_file_read(test_path))
    return test_path;

  free(test_path);
  return NULL;
  }

int bgav_match_regexp(const char * str, const char * regexp)
  {
  int ret = 0;
  regex_t exp;
  memset(&exp, 0, sizeof(exp));

  /*
    `REG_EXTENDED'
     Treat the pattern as an extended regular expression, rather than
     as a basic regular expression.

     `REG_ICASE'
     Ignore case when matching letters.

     `REG_NOSUB'
     Don't bother storing the contents of the MATCHES-PTR array.

     `REG_NEWLINE'
     Treat a newline in STRING as dividing STRING into multiple lines,
     so that `$' can match before the newline and `^' can match after.
     Also, don't permit `.' to match a newline, and don't permit
     `[^...]' to match a newline.
  */
  
  regcomp(&exp, regexp, REG_NOSUB|REG_EXTENDED);
  if(!regexec(&exp, str, 0, NULL, 0))
    ret = 1;
  regfree(&exp);
  return ret;
  }

static const char * coding_type_strings[4] =
  {
  "?",
  "I",
  "P",
  "B"
  };

const char * bgav_coding_type_to_string(int type)
  {
  return coding_type_strings[type & GAVL_PACKET_TYPE_MASK];
  }

char * bgav_escape_string(char * old_string, const char * escape_chars)
  {
  char escape_seq[3];
  char * new_string = NULL;
  int i, done;

  const char * start;
  const char * end;
  const char * pos;
  
  int escape_len = strlen(escape_chars);
  
  /* 1st round: Check if the string can be passed unchanged */

  done = 1;
  for(i = 0; i < escape_len; i++)
    {
    if(index(old_string, escape_chars[i]))
      {
      done = 0;
      break;
      }
    }
  if(done)
    return old_string;

  /* 2nd round: Escape characters */

  escape_seq[0] = '\\';
  escape_seq[2] = '\0';
  
  start = old_string;
  end = start;

  done = 0;

  while(1)
    {
    /* Copy unescaped stuff */
    while(!index(escape_chars, *end) && (*end != '\0'))
      end++;

    if(end - start)
      {
      new_string = bgav_strncat(new_string, start, end);
      start = end;
      }

    if(*end == '\0')
      {
      free(old_string);
      return new_string;
      }
    /* Escape stuff */

    while((pos = index(escape_chars, *start)))
      {
      escape_seq[1] = *pos;
      new_string = bgav_strncat(new_string, escape_seq, NULL);
      start++;
      }
    end = start;
    if(*end == '\0')
      {
      free(old_string);
      return new_string;
      }
    }
  return NULL; // Never get here
  }

static const struct
  {
  gavl_codec_id_t id;
  uint32_t fourcc;
  }
fourccs[] =
  {
    { GAVL_CODEC_ID_NONE,      BGAV_MK_FOURCC('g','a','v','f') },
    /* Audio */
    { GAVL_CODEC_ID_ALAW,      BGAV_MK_FOURCC('a','l','a','w') }, 
    { GAVL_CODEC_ID_ULAW,      BGAV_MK_FOURCC('u','l','a','w') }, 
    { GAVL_CODEC_ID_MP2,       BGAV_MK_FOURCC('.','m','p','2') }, 
    { GAVL_CODEC_ID_MP3,       BGAV_MK_FOURCC('.','m','p','3') }, 
    { GAVL_CODEC_ID_AC3,       BGAV_MK_FOURCC('.','a','c','3') }, 
    { GAVL_CODEC_ID_AAC,       BGAV_MK_FOURCC('m','p','4','a') }, 
    { GAVL_CODEC_ID_VORBIS,    BGAV_MK_FOURCC('V','B','I','S') }, 
    { GAVL_CODEC_ID_FLAC,      BGAV_MK_FOURCC('F','L','A','C') }, 
    { GAVL_CODEC_ID_OPUS,      BGAV_MK_FOURCC('O','P','U','S') }, 
    { GAVL_CODEC_ID_SPEEX,     BGAV_MK_FOURCC('S','P','E','X') }, 
    
    /* Video */
    { GAVL_CODEC_ID_JPEG,      BGAV_MK_FOURCC('j','p','e','g') }, 
    { GAVL_CODEC_ID_PNG,       BGAV_MK_FOURCC('p','n','g',' ') }, 
    { GAVL_CODEC_ID_TIFF,      BGAV_MK_FOURCC('t','i','f','f') }, 
    { GAVL_CODEC_ID_TGA,       BGAV_MK_FOURCC('t','g','a',' ') }, 
    { GAVL_CODEC_ID_MPEG1,     BGAV_MK_FOURCC('m','p','v','1') }, 
    { GAVL_CODEC_ID_MPEG2,     BGAV_MK_FOURCC('m','p','v','2') },
    { GAVL_CODEC_ID_MPEG4_ASP, BGAV_MK_FOURCC('m','p','4','v') },
    { GAVL_CODEC_ID_H264,      BGAV_MK_FOURCC('H','2','6','4') },
    { GAVL_CODEC_ID_THEORA,    BGAV_MK_FOURCC('T','H','R','A') },
    { GAVL_CODEC_ID_DIRAC,     BGAV_MK_FOURCC('d','r','a','c') },
    { GAVL_CODEC_ID_DV,        BGAV_MK_FOURCC('D','V',' ',' ') },
    { GAVL_CODEC_ID_VP8,       BGAV_MK_FOURCC('V','P','8','0') },
    { /* End */                                                },
  };

uint32_t bgav_compression_id_2_fourcc(gavl_codec_id_t id)
  {
  int i = 0;
  while(fourccs[i].fourcc)
    {
    if(fourccs[i].id == id)
      return fourccs[i].fourcc;
    i++;
    }
  return 0;
  }
