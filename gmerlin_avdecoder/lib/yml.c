/*****************************************************************
 
  yml.c
 
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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <avdec_private.h>
#include <yml.h>

typedef struct
  {
  bgav_input_context_t * input;
  char * buffer;
  char * buffer_ptr;
  int buffer_size;
  int buffer_alloc;
  } parser_t;

static bgav_yml_node_t * parse_node(parser_t * p, int * is_comment);


#define BYTES_TO_READ 32

static int more_data(parser_t * p)
  {
  int buffer_pos;
  int bytes_read;
  if(p->buffer_size + BYTES_TO_READ + 1 > p->buffer_alloc)
    {
    p->buffer_alloc = p->buffer_size + BYTES_TO_READ + 1 + 128;
    buffer_pos = (int)(p->buffer_ptr - p->buffer);
    p->buffer = realloc(p->buffer, p->buffer_alloc);
    p->buffer_ptr = p->buffer + buffer_pos;
    }
  bytes_read = bgav_input_read_data(p->input, p->buffer + p->buffer_size, BYTES_TO_READ);
  if(!bytes_read)
    return 0;

  p->buffer_size += bytes_read;

  /* Zero terminate so we can use the string.h functions */
  
  p->buffer[p->buffer_size] = '\0';
  
  return 1;
  }

static void advance(parser_t * p, int bytes)
  {
  if(bytes > p->buffer_size)
    {
    fprintf(stderr, "bytes > p->buffer_size!!!\n");
    return;
    }
  if(bytes < p->buffer_size)
    memmove(p->buffer, p->buffer + bytes, p->buffer_size - bytes);
  p->buffer_size -= bytes;
  p->buffer[p->buffer_size] = '\0';
  }

static char * find_chars(parser_t * p, const char * chars)
  {
  char * pos = (char*)0;
  while(1)
    {
    pos = strpbrk(p->buffer, chars);
    if(pos)
      return pos;
    if(!more_data(p))
      break;
    }
  return (char*)0;
  }

int skip_space(parser_t * p)
  {
  if(!p->buffer_size)
    {
    if(!more_data(p))
      return 0;
    }
  while(isspace(*(p->buffer)))
    {
    advance(p, 1);
    if(!p->buffer_size)
      {
      if(!more_data(p))
        return 0;
      }
    }
  return 1;
  }

static char * parse_tag_name(parser_t * p)
  {
  char * pos;
  char * ret;
  pos = find_chars(p, "> /");
  if(!pos)
    return (char*)0;
  ret = bgav_strndup(p->buffer, pos);
  advance(p, pos - p->buffer);
  return ret;
  }

static char * parse_attribute_name(parser_t * p)
  {
  char * pos;
  char * ret;
  pos = find_chars(p, "= ");
  if(!pos)
    return (char*)0;
  ret = bgav_strndup(p->buffer, pos);
  advance(p, pos - p->buffer);
  return ret;
  }

static char * parse_attribute_value(parser_t * p)
  {
  char * ret;
  char * end;
  char accept[2];

  //  fprintf(stderr, "Parse attribute value: %s %d\n",
  //          p->buffer, p->buffer_size);

  if(!p->buffer_size)
    {
    if(!more_data(p))
      return (char*)0;
    }
  accept[0] = *(p->buffer);
  accept[1] = '\0';
  if((accept[0] != '\'') && (accept[0] != '"'))
    return (char*)0;
  advance(p, 1);

  end = find_chars(p, accept);

  if(!end)
    return(char*)0;
  ret = bgav_strndup(p->buffer, end);
  
  advance(p, (int)(end - p->buffer) + 1);
  return ret;
  }

static int parse_text_node(parser_t * p, bgav_yml_node_t * ret)
  {
  char * pos;
  //  fprintf(stderr, "Parse text node: %s\n", p->buffer);
  pos = find_chars(p, "<");
  if(!pos)
    return 0;
  ret->str = bgav_strndup(p->buffer, pos);
  advance(p, (int)(pos - p->buffer));
  return 1;
  }

static int parse_attributes(parser_t * p, bgav_yml_node_t * ret)
  {
  bgav_yml_attr_t * attr_end;

  /* We have at least one attribute here */

  ret->attributes = calloc(1, sizeof(*(ret->attributes)));
  attr_end = ret->attributes;

  //  fprintf(stderr, "p1: %s\n", p->buffer);
  
  attr_end->name = parse_attribute_name(p);
  //  fprintf(stderr, "p2: %s\n", p->buffer);
      
  if(!skip_space(p))
    return 0;
  //  fprintf(stderr, "p3: %s\n", p->buffer);
  if(p->buffer[0] != '=')
    return 0;
  //  fprintf(stderr, "p4: %s\n", p->buffer);
  advance(p, 1); /* '=' */
  //  fprintf(stderr, "p5: %s\n", p->buffer);
  if(!skip_space(p))
    return 0;
  //  fprintf(stderr, "p6: %s\n", p->buffer);
  attr_end->value = parse_attribute_value(p);

  //  fprintf(stderr, "Attribute: %s: %s\n", attr_end->name, attr_end->value);
  
  while(1)
    {
    skip_space(p);
    if((*(p->buffer) == '>') || (*(p->buffer) == '/'))
      return 1;
    else
      {
      attr_end->next = calloc(1, sizeof(*(attr_end->next)));
      attr_end = attr_end->next;
      
      attr_end->name = parse_attribute_name(p);

      if(!skip_space(p))
        return 0;
      if(p->buffer[0] != '=')
        return 0;
      advance(p, 1); /* '=' */
      if(!skip_space(p))
        return 0;
      attr_end->value = parse_attribute_value(p);
      //      fprintf(stderr, "Attribute: %s %s\n", attr_end->name, attr_end->value);
      }
    }
  return 1;
  }

static int parse_children(parser_t * p, bgav_yml_node_t * ret)
  {
  int is_comment;
  bgav_yml_node_t * child_end = (bgav_yml_node_t*)0;
  bgav_yml_node_t * new_node;
  while(1)
    {
    if(p->buffer_size < 2)
      {
      if(!more_data(p))
        return 0;
      }
    if((p->buffer[0] == '<') && (p->buffer[1] == '/'))
      break;
    
    new_node = parse_node(p, &is_comment);

    if(!new_node)
      {
      if(!is_comment)
        return 0;
      }
    else
      {
      if(!ret->children)
        {
        ret->children = new_node;
        child_end = new_node;
        }
      else
        {
        child_end->next = new_node;
        child_end = child_end->next;
        }
      }
    }
  return 1;
  }

static int parse_tag_node(parser_t * p, bgav_yml_node_t * ret)
  {
  char * pos;
  advance(p, 1);
  //  fprintf(stderr, "Parse tag node: %s\n", p->buffer);
  ret->name = parse_tag_name(p);

  switch(*(p->buffer))
    {
    case ' ': /* Attributes might come */
      if(!skip_space(p))
        return 0;
      if(*(p->buffer) != '>')
        {
        if(!parse_attributes(p, ret))
          return 0;
        
        switch(*(p->buffer))
          {
          case '>':
            advance(p, 1);
            break;
          case '/':
            advance(p, 1);
            if(!p->buffer_size)
              {
              if(!more_data(p))
                return 0;
              }
            if(*(p->buffer) == '>')
              {
              advance(p, 1);
              return 1;
              }
            else
              return 0;
            break;
          }
        }
      else
        {
        advance(p, 1);
        if(!parse_children(p, ret))
          return 0;
        }
      break;
    case '>': /* End of open tag */
      advance(p, 1);
      break;
    case '/': /* Node finished here? */
      advance(p, 1);
      if(!p->buffer_size)
        {
        if(!more_data(p))
          return 0;
        if(*(p->buffer) == '>')
          {
          advance(p, 1);
          return 1;
          }
        else
          return 0;
        }
      break;
    }
  /* Parse the children */

  if(!parse_children(p, ret))
    return 0;

  /* Now, we MUST have a closing tag */

  if((*p->buffer) != '<')
    return 0;
  advance(p, 1);
  
  if(!p->buffer_size)
    {
    if(!more_data(p))
      return 0;
    }
  if((*p->buffer) != '/')
    return 0;
  advance(p, 1);
  pos = find_chars(p, ">");
  if(!pos)
    return 0;
  if(strncasecmp(ret->name, p->buffer, (int)(pos - p->buffer)))
    return 0;
  advance(p, (int)(pos - p->buffer) + 1);
  return 1;
  }

static void skip_comment(parser_t * p)
  {
  char * pos;
  advance(p, 4);
  //  fprintf(stderr, "Skip comment\n");
  while(1)
    {
    pos = find_chars(p, "-");
    if(!pos)
      return;
    advance(p, (int)(pos - p->buffer));
    if(p->buffer_size < 3)
      {
      if(!more_data(p))
        return;
      }
    if((p->buffer[1] == '-') &&
       (p->buffer[2] == '>'))
      {
      advance(p, 3);
      return;
      }
    else
      advance(p, 1);
    }
  }

/* Parse one complete node, call this recursively for children */

static bgav_yml_node_t * parse_node(parser_t * p, int * is_comment)
  {
  bgav_yml_node_t * ret;
  
  *is_comment = 0;
  ret = calloc(1, sizeof(*ret));
  
  if(!p->buffer_size)
    {
    if(!more_data(p))
      goto fail;
    }

  if(*(p->buffer) == '<')
    {
    /* Skip comments */
    if(p->buffer_size < 4) 
      {
      if(!more_data(p))
        goto fail;
      }

    if((p->buffer[1] == '!') &&
       (p->buffer[2] == '-') &&
       (p->buffer[3] == '-'))
      {
      skip_comment(p);
      *is_comment = 1;
      goto fail;
      }
    
    if(!parse_tag_node(p, ret))
      goto fail;
    }
  else
    {
    if(!parse_text_node(p, ret))
      goto fail;
    }
  return ret;
  fail:
  bgav_yml_free(ret);
  return (bgav_yml_node_t*)0;
  
  }

bgav_yml_node_t * bgav_yml_parse(bgav_input_context_t * input)
  {
  bgav_yml_node_t * ret;
  parser_t parser;
  int is_comment = 1;
  
  memset(&parser, 0, sizeof(parser));
  parser.input = input;

  while(is_comment)
    ret = parse_node(&parser, &is_comment);
  
  if(parser.buffer)
    free(parser.buffer);
  return ret;
  }

void dump_attribute(bgav_yml_attr_t * a)
  {
  char * pos;

  fprintf(stderr, "%s=", a->name);
  
  pos = strchr(a->value, '"');

  if(pos)
    fprintf(stderr, "'%s'", a->value);
  else
    fprintf(stderr, "\"%s\"", a->value);
  
  }

void dump_node(bgav_yml_node_t * n)
  {
  bgav_yml_attr_t * attr;
  bgav_yml_node_t * child;

  
  if(n->name)
    {
    fprintf(stderr, "<%s", n->name);
    if(n->attributes)
      {
      fprintf(stderr, " ");
      attr = n->attributes;
      while(attr)
        {
        dump_attribute(attr);
        if(attr->next)
          fprintf(stderr, " ");
        attr = attr->next;
        }
      }
    if(!n->children)
      fprintf(stderr, "/>");
    else
      {
      fprintf(stderr, ">");
      child = n->children;
      while(child)
        {
        dump_node(child);
        child = child->next;
        }
      fprintf(stderr, "</%s>", n->name);
      }

    }
  
  else if(n->str)
    fprintf(stderr, "%s", n->str);
  }

void bgav_yml_dump(bgav_yml_node_t * n)
  {
  dump_node(n);
  }

#define FREE(p) if(p)free(p);

void bgav_yml_free(bgav_yml_node_t * n)
  {
  bgav_yml_node_t * child;
  bgav_yml_attr_t * attr;
  if(!n)
    return;
  while(n->children)
    {
    child = n->children->next;
    bgav_yml_free(n->children);
    n->children = child;
    }

  while(n->attributes)
    {
    attr = n->attributes->next;
    FREE(n->attributes->name);
    FREE(n->attributes->value);
    FREE(n->attributes);
    n->attributes = attr;
    }
  
  FREE(n->name);
  FREE(n->str);
  
  free(n);
  
  }

const char * bgav_yml_get_attribute(bgav_yml_node_t * n, const char * name)
  {
  bgav_yml_attr_t * attr;
  attr = n->attributes;
  while(attr)
    {
    if(attr->name && !strcmp(attr->name, name))
      return attr->value;
    }
  return (const char*)0;
  }

const char * bgav_yml_get_attribute_i(bgav_yml_node_t * n, const char * name)
  {
  bgav_yml_attr_t * attr;
  attr = n->attributes;
  while(attr)
    {
    if(attr->name && !strcasecmp(attr->name, name))
      return attr->value;
    }
  return (const char*)0;
  
  }
