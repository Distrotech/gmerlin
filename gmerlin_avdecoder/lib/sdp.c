
/*****************************************************************
 
  sdp.c
 
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <avdec_private.h>
#include <sdp.h>

#define ISSEP(c) ((*c == '\n') || (*c == '\r') || (*c == '\0'))
#define MY_FREE(p) if(p){free(p);p=NULL;}


static void print_string(const char * label, const char * str)
  {
  fprintf(stderr, "%s: %s\n", label, (str ? str : "(NULL)"));
  }


/* Parse Origin */

#if 0
typedef struct
  {
  char * username;        /* NULL if "-" */
  uint64_t   session_id;
  uint64_t   session_version;
  
  char * network_type;
  char * addr_type;
  char * addr;
  } bgav_sdp_origin_t;
#endif

static void free_origin(bgav_sdp_origin_t*o)
  {
  MY_FREE(o->username);
  MY_FREE(o->network_type);
  MY_FREE(o->addr_type);
  MY_FREE(o->addr_type);
  }

static int parse_origin(const char * line, bgav_sdp_origin_t * ret)
  {
  char ** strings;

  strings = bgav_stringbreak(line, ' ');

  if(!strings)
    return 0;

  if(strings[0])
    {
    if((strings[0][0] != '-') || (strlen(strings[0]) > 1))
      ret->username = bgav_strndup(strings[0], NULL);
    }
  else
    goto fail;

  if(strings[1])
    {
    if(strlen(strings[1]))
      ret->session_id = strtoll(strings[1], NULL, 10);
    }
  else
    goto fail;

  if(strings[2])
    {
    if(strlen(strings[2]))
      ret->session_version = strtoll(strings[2], NULL, 10);
    }
  else
    goto fail;

  if(strings[3])
    {
    if(strlen(strings[3]))
      ret->network_type = bgav_strndup(strings[3], NULL);
    }
  else
    goto fail;

  if(strings[4])
    {
    if(strlen(strings[4]))
      ret->addr_type = bgav_strndup(strings[4], NULL);
    }
  else
    goto fail;

  if(strings[5])
    {
    if(strlen(strings[5]))
      ret->addr = bgav_strndup(strings[5], NULL);
    }
  else
    goto fail;
  
  bgav_stringbreak_free(strings);
  return 1;

  fail:
    bgav_stringbreak_free(strings);
    return 0;
  }

/* Parse connection description */

static int parse_connection_desc(const char * line,
                                 bgav_sdp_connection_desc_t * ret)
  {
  
  return 1;
  }

static void dump_connection_desc(bgav_sdp_connection_desc_t * c)
  {
  fprintf(stderr, "Connection: type: %s addr: %s ttl: %d num: %d\n",
          c->type,
          c->addr,
          c->ttl,
          c->num_addr);
  }

static void free_connection_desc(bgav_sdp_connection_desc_t * c)
  {
  MY_FREE(c->type);
  MY_FREE(c->addr);
  }

/* Parse bandwidth description */

static int parse_bandwidth_desc(const char * line,
                                bgav_sdp_bandwidth_desc_t * ret)
  {
  if((line[0] == 'A') && (line[1] == 'S') && (line[2] == ':'))
    ret->modifier = BGAV_SDP_BANDWIDTH_MODIFIER_AS;
  else if((line[0] == 'C') && (line[1] == 'T') && (line[2] == ':'))
    ret->modifier = BGAV_SDP_BANDWIDTH_MODIFIER_CT;
  else if((line[0] == 'X') && (line[1] == '-'))
    ret->modifier = BGAV_SDP_BANDWIDTH_MODIFIER_USER;
  else
    return 1;

  switch(ret->modifier)
    {
    case BGAV_SDP_BANDWIDTH_MODIFIER_NONE:
      return 1; /* Handled above */
      break;
    case BGAV_SDP_BANDWIDTH_MODIFIER_CT:
    case BGAV_SDP_BANDWIDTH_MODIFIER_AS:
      ret->bandwidth = strtoul(line + 3, NULL, 10);
      break;
    case BGAV_SDP_BANDWIDTH_MODIFIER_USER:
      ret->user_bandwidth = bgav_strndup(line, NULL);
      break;
    }
  return 1;
  }

static void dump_bandwidth_desc(bgav_sdp_bandwidth_desc_t * b)
  {
  switch(b->modifier)
    {
    case BGAV_SDP_BANDWIDTH_MODIFIER_NONE:
      return;
      break;
    case BGAV_SDP_BANDWIDTH_MODIFIER_CT:
    case BGAV_SDP_BANDWIDTH_MODIFIER_AS:
      fprintf(stderr, "Bandwidth: %s:%lu\n",
              ((b->modifier == BGAV_SDP_BANDWIDTH_MODIFIER_CT) ? "CT" : "AS"),
              b->bandwidth);
      break;
    case BGAV_SDP_BANDWIDTH_MODIFIER_USER:
      fprintf(stderr, "Bandwidth (user defined): %s\n",
              b->user_bandwidth);
      break;
    }
  
  }

static void free_bandwidth_desc(bgav_sdp_bandwidth_desc_t * b)
  {
  MY_FREE(b->user_bandwidth);
  }

/* Key description */

static int parse_key_desc(const char * line, bgav_sdp_key_desc_t * ret)
  {
  const char * pos;
  pos = strchr(line, ':');
  if(!pos)
    pos = &(line[strlen(pos)]);

  if(strncmp(line, "clear", (int)(pos - line)))
    {
    ret->type = BGAV_SDP_KEY_TYPE_CLEAR;
    }
  else if(strncmp(line, "base64", (int)(pos - line)))
    {
    ret->type = BGAV_SDP_KEY_TYPE_BASE64;
    }
  else if(strncmp(line, "uri", (int)(pos - line)))
    {
    ret->type = BGAV_SDP_KEY_TYPE_URI;
    }
  else if(strncmp(line, "prompt", (int)(pos - line)))
    {
    ret->type = BGAV_SDP_KEY_TYPE_PROMPT;
    }
  else
    return 0;
  
  if(*pos == ':')
    {
    pos++;
    ret->key = bgav_strndup(pos, NULL);
    }
  return 1;
  }

static void dump_key_desc(bgav_sdp_key_desc_t * k)
  {
  switch(k->type)
    {
    case BGAV_SDP_KEY_TYPE_NONE:
      return;
    case BGAV_SDP_KEY_TYPE_CLEAR:
      fprintf(stderr, "Key (clear)");
      break;
    case BGAV_SDP_KEY_TYPE_BASE64:
      fprintf(stderr, "Key (base64)");
      break;
    case BGAV_SDP_KEY_TYPE_URI:
      fprintf(stderr, "Key (uri)");
      break;
    case BGAV_SDP_KEY_TYPE_PROMPT:
      fprintf(stderr, "Key (prompt)");
      break;
    }
  if(k->key)
    fprintf(stderr, ": %s\n", k->key);
  else
    fprintf(stderr, "\n");
  }

static void free_key_desc(bgav_sdp_key_desc_t * k)
  {
  MY_FREE(k->key);
  }

/* Parse attr */

static int parse_attr(const char * line,
                      bgav_sdp_attr_t * ret)
  {
  const char * pos1, *pos2;
  char * dst;
  int data_len;
  /* First step: parse the name */

  pos1 = line + 2;
  pos2 = strchr(pos1, ':');
  if(!pos2)
    pos2 = &(pos1[strlen(pos1)]);

  ret->name = bgav_strndup(pos1, pos2);

  /* Check what comes after */

  if(*pos2 == '\0')
    {
    ret->type = BGAV_SDP_TYPE_BOOLEAN;
    return 1;
    }

  pos1 = pos2;
  pos1++;

  pos2 = pos1;  
  while(isalnum(*pos2))
    pos2++;

  if(*pos2 == ';')
    {
    /* Predefined types */
    //    fprintf(stderr, "*** POS1: %s POS2: %s\n", pos1, pos2);
    if(!strncmp(pos1, "string", (int)(pos2 - pos1)))
      {
      ret->type = BGAV_SDP_TYPE_STRING;
      pos1 = strchr(pos2, '"');
      pos1++;
      pos2 = strrchr(pos1, '"');

      ret->val.str = malloc((int)(pos2 - pos1) + 1);

      dst = ret->val.str;
      while(pos1 < pos2)
        {
        if((pos1[0] == '\\') && (pos1[1] == '"'))
          {
          *dst = '"';
          dst++;
          pos1 += 2;
          }
        else
          {
          *dst = *pos1;
          *pos1++;
          *dst++;
          }
        }
      *dst = '\0';
      }
    else if(!strncmp(pos1, "buffer", (int)(pos2 - pos1)))
      {
      ret->type = BGAV_SDP_TYPE_DATA;
      
      pos1 = strchr(pos2, '"');
      pos1++;
      pos2 = strrchr(pos1, '"');
      data_len = ((pos2 - pos1) / 4) * 3;
      //      fprintf(stderr, "BUFFER %d %d\n", (int)(pos2 - pos1), data_len);

      ret->val.data = malloc(data_len);

      ret->data_len = bgav_base64decode(pos1, (int)(pos2 - pos1),
                                          ret->val.data, data_len);
      if(!ret->data_len)
        {
        free(ret->val.data);
        ret->val.data = NULL;
        }
      }
    else if(!strncmp(pos1, "integer", (int)(pos2 - pos1)))
      {
      ret->type = BGAV_SDP_TYPE_INT;
      pos2++;
      ret->val.i = atoi(pos2);
      }
    }
  else
    {
    ret->type = BGAV_SDP_TYPE_GENERIC;
    ret->val.str = bgav_strndup(pos1, NULL);
    }
  return 1;
  }

/* Returns the number of attributes (==lines) or 0 */

static int parse_attributes(char ** lines, bgav_sdp_attr_t ** ret)
  {
  int num_lines, i;
  bgav_sdp_attr_t * attributes;
  num_lines = 1;
  while(lines[num_lines] && (lines[num_lines][0] == 'a'))
    num_lines++;

  attributes = calloc(num_lines + 1, sizeof(*attributes));
  for(i = 0; i < num_lines; i++)
    {
    parse_attr(lines[i], &(attributes[i]));
    }
  *ret = attributes;
  return num_lines;
  }

static void dump_attributes(bgav_sdp_attr_t * attr)
  {
  int index;
  if(!attr || (attr[0].type == BGAV_SDP_TYPE_NONE))
    return;

  fprintf(stderr, "Attributes:\n");
  index = 0;
  while(attr[index].type != BGAV_SDP_TYPE_NONE)
    {
    fprintf(stderr, "  %s ", attr[index].name);
    
    switch(attr[index].type)
      {
      case BGAV_SDP_TYPE_NONE:
        return;
      case BGAV_SDP_TYPE_INT:
        fprintf(stderr, "(integer): %d\n", attr[index].val.i);
        break;
      case BGAV_SDP_TYPE_STRING:
        fprintf(stderr, "(string): %s\n", attr[index].val.str);
        break;
      case BGAV_SDP_TYPE_GENERIC:
        fprintf(stderr, "(generic): %s\n", attr[index].val.str);
        break;
      case BGAV_SDP_TYPE_DATA:
        fprintf(stderr, "Binary data %d bytes, hexdump follows\n",
                attr[index].data_len);
        bgav_hexdump(attr[index].val.data, attr[index].data_len, 16);
        break;
      case BGAV_SDP_TYPE_BOOLEAN:
        fprintf(stderr, "\n");
      }
    index++;
    }
  
  }

static void free_attributes(bgav_sdp_attr_t ** attr)
  {
  int index;
  if(!attr)
    return;
  index = 0;
  while((*attr)[index].type != BGAV_SDP_TYPE_NONE)
    {
    //    fprintf(stderr, "  %s ", (*attr)[index].name);
    MY_FREE((*attr)[index].name);
    switch((*attr)[index].type)
      {
      case BGAV_SDP_TYPE_NONE:
        return;
      case BGAV_SDP_TYPE_INT:
        break;
      case BGAV_SDP_TYPE_STRING:
      case BGAV_SDP_TYPE_GENERIC:
        MY_FREE((*attr)[index].val.str);
        break;
      case BGAV_SDP_TYPE_DATA:
        MY_FREE((*attr)[index].val.data);
        break;
      case BGAV_SDP_TYPE_BOOLEAN:
        break;
      }
    index++;
    }
  free(*attr);
  *attr = NULL;
  }

/* Returns the number of lines used */

static int parse_media(char ** lines, bgav_sdp_media_desc_t * ret)
  {
  int num_lines, line_index, i, i_tmp;
  char ** strings = (char**)0;
  char * pos;
  memset(ret, 0, sizeof(*ret));
  
  /* 1. count lines */
  num_lines = 1;
  while(lines[num_lines] && (lines[num_lines][0] != 'm'))
    num_lines++;
  //  fprintf(stderr, "Num lines: %d %c %p\n", num_lines, lines[num_lines-1][0], lines[num_lines]);
  
  /* 2. Parse "m=" header */

  line_index = 0;

  strings = bgav_stringbreak(lines[0]+2, ' ');

  if(strings[0])
    ret->media = bgav_strndup(strings[0], NULL);
  else
    goto fail;

  if(strings[1])
    {
    ret->port = atoi(strings[1]);
    pos = strchr(strings[1], '/');
    if(pos)
      {
      pos++;
      ret->num_ports = atoi(pos);
      }
    }
  else
    goto fail;

  if(strings[2])
    {
    ret->protocol = bgav_strndup(strings[2], NULL);
    }
  else
    goto fail;

  if(strings[3])
    {
    while(strings[3+ret->num_formats])
      ret->num_formats++;

    ret->formats = malloc(ret->num_formats * sizeof(char*));
    for(i = 0; i < ret->num_formats; i++)
      {
      ret->formats[i] = bgav_strndup(strings[3+i], NULL);
      }
    }
  else
    goto fail;
  
  bgav_stringbreak_free(strings);
  strings = (char**)0;
  line_index++;

  /* 3. Parse the rest */
  
  while(line_index < num_lines)
    {
    if(lines[line_index][1] != '=')
      {
      fprintf(stderr, "sdp: Invalid line %d: %s\n",
              line_index, lines[line_index]);
      goto fail;
      }
    switch(lines[line_index][0])
      {
      case 'i': //  i=* (session information)
        ret->media_title = bgav_strndup(lines[line_index] + 2, NULL);
        line_index++;
        break;
      case 'c': //  c=* (connection information - not required if included in all media)
        line_index++;
        break;
      case 'b': //  b=* (bandwidth information)
        if(!parse_bandwidth_desc(lines[line_index] + 2, &(ret->bandwidth)))
          goto fail;
        line_index++;
        break;
      case 'k': //  k=* (encryption key)
        if(!parse_key_desc(lines[line_index] + 2, &(ret->key)))
          goto fail;
        line_index++;
        break;
      case 'a': //  a=* (zero or more session attribute lines)
        i_tmp = parse_attributes(&(lines[line_index]), &(ret->attributes));
        line_index += i_tmp;
        ret->num_attributes = i_tmp;
        break;
      default:
        fprintf(stderr, "Unknown line: %s\n", lines[line_index]);
        line_index++;
      }
    }
  return num_lines;
  fail:
  if(strings)
    bgav_stringbreak_free(strings);
  //  fprintf(stderr, "parse_media failed\n");
  return 0;    
  }

static void dump_media(bgav_sdp_media_desc_t * m)
  {
  int i;
  print_string("Media", m->media); /* audio, video etc */

  fprintf(stderr, "Port %d\n", m->port);
  fprintf(stderr, "Num Ports %d\n", m->num_ports);
  print_string("Protocol", m->protocol);

  fprintf(stderr, "Formats: ");
  
  for(i = 0; i < m->num_formats; i++)
    {
    fprintf(stderr, "%s ", m->formats[i]);
    }
  fprintf(stderr, "\n");

  /* Additional stuff */

  print_string("Title", m->media_title);                    /* i=* (media title) */

  dump_connection_desc(&(m->connection));
  dump_bandwidth_desc(&(m->bandwidth));
  dump_key_desc(&(m->key));
  
  /* a= */

  dump_attributes(m->attributes);
  
  }

static void free_media(bgav_sdp_media_desc_t * m)
  {
  int i;
  MY_FREE(m->media); /* audio, video etc */
  MY_FREE(m->protocol);

  for(i = 0; i < m->num_formats; i++)
    MY_FREE(m->formats[i]);
  MY_FREE(m->formats);
  
  /* Additional stuff */

  MY_FREE(m->media_title);               /* i=* (media title) */
  free_connection_desc(&(m->connection));
  free_bandwidth_desc(&(m->bandwidth));
  free_key_desc(&(m->key));
  free_attributes(&(m->attributes));
  }

/* Parse everything */
   

int bgav_sdp_parse(const char * data, bgav_sdp_t * ret)
  {
  char *  buf;
  char ** lines;
  char * pos;
  int num_lines;
  int line_index;
  int i_tmp;
  
  memset(ret, 0, sizeof(*ret));
    
  /* 1. Copy the buffer */
  i_tmp = strlen(data);
  buf = malloc(i_tmp+1);
  strcpy(buf, data);

  /* 2. count lines */
  num_lines = 0;
  pos = buf;

  while(1)
    {
    while(!ISSEP(pos))
      pos++;
    if(*pos == '\0')
      break;
    num_lines++;
    while(ISSEP(pos))
      pos++;
    }

  /* 3. Create lines array */
  lines = calloc(num_lines + 1, sizeof(char*));
  //  fprintf(stderr, "Num lines: %d\n", num_lines);  
  pos = buf;
  line_index = 0;
  while(1)
    {
    if(line_index >= num_lines)
      break;
    
    lines[line_index] = pos;
    line_index++;
    while(!ISSEP(pos))
      pos++;
    if(*pos == '\0')
      break;
    *pos = '\0';
    while(ISSEP(pos))
      pos++;
    }

  /* 4. Parse the stuff */

  line_index = 0;

  while(line_index < num_lines)
    {
    if(lines[line_index][1] != '=')
      {
      fprintf(stderr, "sdp: Invalid line %d: %s\n",
              line_index, lines[line_index]);
      goto fail;
      }
    switch(lines[line_index][0])
      {
      case 'v': //  v=  (protocol version)
        ret->protocol_version = atoi(lines[line_index] + 2);
        line_index++;
        break;
      case 'o': //  o=  (owner/creator and session identifier).
        if(!parse_origin(lines[line_index] + 2, &(ret->origin)))
          goto fail;
        line_index++;
        break;
      case 's': //  s=  (session name)
        ret->session_name = bgav_strndup(lines[line_index] + 2, NULL);
        line_index++;
        break;
      case 'i': //  i=* (session information)
        ret->session_description = bgav_strndup(lines[line_index] + 2, NULL);
        line_index++;
        break;
      case 'u': //  u=* (URI of description)
        ret->uri = bgav_strndup(lines[line_index] + 2, NULL);
        line_index++;
        break;
      case 'e': //  e=* (email address)
        ret->email = bgav_strndup(lines[line_index] + 2, NULL);
        line_index++;
        break;
      case 'p': //  p=* (phone number)
        ret->phone = bgav_strndup(lines[line_index] + 2, NULL);
        line_index++;
        break;
      case 'c': //  c=* (connection information - not required if included in all media)
        line_index++;
        break;
      case 'b': //  b=* (bandwidth information)
        if(!parse_bandwidth_desc(lines[line_index] + 2, &(ret->bandwidth)))
          goto fail;
        line_index++;
        break;
      case 't': //  One or more time descriptions (see below)
        line_index++;
        break;
      case 'z': //  z=* (time zone adjustments)
        line_index++;
        break;
      case 'k': //  k=* (encryption key)
        if(!parse_key_desc(lines[line_index] + 2, &(ret->key)))
          goto fail;
        line_index++;
        break;
      case 'a': //  a=* (zero or more session attribute lines)
        i_tmp = parse_attributes(&(lines[line_index]), &(ret->attributes));
        line_index += i_tmp;
        ret->num_attributes = i_tmp;
        break;
      case 'm': //  Zero or more media descriptions (see below)
        //        fprintf(stderr, "Parse media %d\n", line_index);
        ret->num_media++;
        ret->media = realloc(ret->media, ret->num_media * sizeof(*(ret->media)));
        line_index += parse_media(&(lines[line_index]), &(ret->media[ret->num_media-1]));
        break;
      default: //  Zero or more media descriptions (see below)
        fprintf(stderr, "Unknown sdp specifier: %c\n", lines[line_index][0]);
        line_index++;
        break;
      }
    }
  
  return 1;
  
  fail:
  return 0;    
  }

void bgav_sdp_free(bgav_sdp_t * s)
  {
  int i;
  free_origin(&(s->origin));

  MY_FREE(s->session_name);                   /* s= */
  MY_FREE(s->session_description);            /* i= */
  MY_FREE(s->uri);                            /* u= */
  MY_FREE(s->email);                          /* e= */
  MY_FREE(s->phone);                          /* p= */

  free_connection_desc(&(s->connection));
  free_bandwidth_desc(&(s->bandwidth));
  free_key_desc(&(s->key));
  free_attributes(&(s->attributes));

  for(i = 0; i < s->num_media; i++)
    free_media(&(s->media[i]));
  MY_FREE(s->media);
  }
  
void bgav_sdp_dump(bgav_sdp_t * s)
  {
  int i;
  fprintf(stderr, "Protcol Version: %d\n", s->protocol_version);        /* v= */

  fprintf(stderr, "Origin:\n");

  print_string("  Useraname", s->origin.username);
  fprintf(stderr, "  Session ID: %lld\n", s->origin.session_id);
  fprintf(stderr, "  Session Version: %lld\n", s->origin.session_version);
  print_string("  Network Type", s->origin.network_type);
  print_string("  Address Type", s->origin.addr_type);
  print_string("  Address", s->origin.addr);
  
  print_string("Session name", s->session_name);               /* s= */
  print_string("Session description", s->session_description); /* i= */
  print_string("URI", s->uri);                                 /* u= */
  print_string("email", s->email);                             /* e= */
  print_string("phone", s->phone);                             /* p= */
  //  bgav_sdp_connection_desc_t connection; /* c= */

  dump_bandwidth_desc(&(s->bandwidth));

  /* TODO: Time, repeat, zone */
  
  dump_key_desc(&(s->key));

  dump_attributes(s->attributes);
  
  /* Media */

  for(i = 0; i < s->num_media; i++)
    {
    dump_media(&(s->media[i]));
    }
  
  }
