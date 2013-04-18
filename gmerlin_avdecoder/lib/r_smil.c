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

#include <string.h>
#include <ctype.h>
#include <avdec_private.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_DOMAIN "r_smil"

static const struct
  {
  const char * code;
  const char * language;
  } languages[] =
  {
    { "af",    "Afrikaans" },
    { "sq",    "Albanian" },
    { "ar-iq", "Arabic ( Iraq)" },
    { "ar-dz", "Arabic (Algeria)" },
    { "ar-bh", "Arabic (Bahrain)" },
    { "ar-eg", "Arabic (Egypt)" },
    { "ar-jo", "Arabic (Jordan)" },
    { "ar-kw", "Arabic (Kuwait)" },
    { "ar-lb", "Arabic (Lebanon)" },
    { "ar-ly", "Arabic (Libya)" },
    { "ar-ma", "Arabic (Morocco)" },
    { "ar-om", "Arabic (Oman)" },
    { "ar-qa", "Arabic (Qatar)" },
    { "ar-sa", "Arabic (Saudi Arabia)" },
    { "ar-sy", "Arabic (Syria)" },
    { "ar-tn", "Arabic (Tunisia)" },
    { "ar-ae", "Arabic (U.A.E.)" },
    { "ar-ye", "Arabic (Yemen)" },
    { "eu",    "Basque" },
    { "bg",    "Bulgarian" },
    { "ca",    "Catalan" },
    { "zh-hk", "Chinese (Hong Kong)" },
    { "zh-cn", "Chinese (People's Republic)" },
    { "zh-sg", "Chinese (Singapore)" },
    { "zh-tw", "Chinese (Taiwan)" },
    { "hr",    "Croatian" },
    { "cs",    "Czech" },
    { "da",    "Danish" },
    { "nl",    "Dutch (Standard)" },
    { "nl-be", "Dutch (Belgian)" },
    { "en",    "English" },
    { "en-au", "English (Australian)" },
    { "en-bz", "English (Belize)" },
    { "en-gb", "English (British)" },
    { "en-ca", "English (Canadian)" },
    { "en",    "English (Caribbean)" },
    { "en-ie", "English (Ireland)" },
    { "en-jm", "English (Jamaica)" },
    { "en-nz", "English (New Zealand)" },
    { "en-za", "English (South Africa)" },
    { "en-tt", "English (Trinidad)" },
    { "en-us", "English (United States)" },
    { "et",    "Estonian" },
    { "fo",    "Faeroese" },
    { "fi",    "Finnish" },
    { "fr-be", "French (Belgian)" },
    { "fr-ca", "French (Canadian)" },
    { "fr-lu", "French (Luxembourg)" },
    { "fr",    "French (Standard)" },
    { "fr-ch", "French (Swiss)" },
    { "de-at", "German (Austrian)" },
    { "de-li", "German (Liechtenstein)" },
    { "de-lu", "German (Luxembourg)" },
    { "de",    "German (Standard)" },
    { "de-ch", "German (Swiss)" },
    { "el",    "Greek" },
    { "he",    "Hebrew" },
    { "hu",    "Hungarian" },
    { "is",    "Icelandic" },
    { "in",    "Indonesian" },
    { "it",    "Italian (Standard)" },
    { "it-ch", "Italian (Swiss)" },
    { "ja",    "Japanese" },
    { "ko",    "Korean" },
    { "ko",    "Korean (Johab)" },
    { "lv",    "Latvian" },
    { "lt",    "Lithuanian" },
    { "no",    "Norwegian" },
    { "pl",    "Polish" },
    { "pt-br", "Portuguese (Brazilian)" },
    { "pt",    "Portuguese (Standard)" },
    { "ro",    "Romanian" },
    { "sr",    "Serbian" },
    { "sk",    "Slovak" },
    { "sl",    "Slovenian" },
    { "es-ar", "Spanish (Argentina)" },
    { "es-bo", "Spanish (Bolivia)" },
    { "es-cl", "Spanish (Chile)" },
    { "es-co", "Spanish (Colombia)" },
    { "es-cr", "Spanish (Costa Rica)" },
    { "es-do", "Spanish (Dominican Republic)" },
    { "es-ec", "Spanish (Ecuador)" },
    { "es-sv", "Spanish (El Salvador)" },
    { "es-gt", "Spanish (Guatemala)" },
    { "es-hn", "Spanish (Honduras)" },
    { "es-mx", "Spanish (Mexican)" },
    { "es-ni", "Spanish (Nicaragua)" },
    { "es-pa", "Spanish (Panama)" },
    { "es-py", "Spanish (Paraguay)" },
    { "es-pe", "Spanish (Peru)" },
    { "es-pr", "Spanish (Puerto Rico)" },
    { "es",    "Spanish (Spain)" },
    { "es-uy", "Spanish (Uruguay)" },
    { "es-ve", "Spanish (Venezuela)" },
    { "sv",    "Swedish" },
    { "sv-fi", "Swedish (Finland)" },
    { "th",    "Thai" },
    { "tr",    "Turkish" },
    { "uk",    "Ukrainian" },
    { "vi",    "Vietnamese" },
  };

static const char * get_language(const char * code)
  {
  int i;
  for(i = 0; i < sizeof(languages)/sizeof(languages[0]); i++)
    {
    if(!strcmp(languages[i].code, code))
      return languages[i].language;
    }
  return NULL;
  }

static int probe_smil(bgav_input_context_t * input)
  {
  char * pos;
  uint8_t buf[5];

  /* We accept all files, which end with .smil or .smi */

  if(input->filename)
    {
    pos = strrchr(input->filename, '.');
    if(pos)
      {
      if(!strcasecmp(pos, ".smi") ||
         !strcasecmp(pos, ".smil"))
        return 1;
      }
    }
  if(bgav_input_get_data(input, buf, 5) < 5)
    return 0;
  if((buf[0] == '<') &&
     (buf[1] == 's') && 
     (buf[2] == 'm') && 
     (buf[3] == 'i') &&
     (buf[4] == 'l'))
    return 1;

  
  return 0;
  }

static int sc(const char * str1, const char * str2)
  {
  if((!str1) || (!str2))
    return -1;
  return strcasecmp(str1, str2);
  }

static int count_urls(bgav_yml_node_t * n)
  {
  int ret = 0;

  while(n)
    {
    if(!sc(n->name, "audio") || !sc(n->name, "video"))
      {
      ret++;
      }
    else if(n->children)
      {
      ret += count_urls(n->children);
      }
    n = n->next;
    }
  return ret;
  }

#if 1

static void get_url(bgav_yml_node_t * n, bgav_url_info_t * ret,
                    const char * title, const char * url_base, int * index)
  {
  const char * location;
  const char * bitrate;
  const char * language;

  char * tmp_string;
  int i;
  
  location =
    gavl_strdup(bgav_yml_get_attribute_i(n, "src"));
  language = 
    gavl_strdup(bgav_yml_get_attribute_i(n, "system-language"));
  bitrate = 
    gavl_strdup(bgav_yml_get_attribute_i(n, "system-bitrate"));

  
  if(!location)
    return;

  /* Set URL */
  
  if(!strstr(location, "://") && url_base)
    {
    if(url_base[strlen(url_base)-1] == '/')
      ret->url = bgav_sprintf("%s%s", url_base, location);
    else
      ret->url = bgav_sprintf("%s/%s", url_base, location);
    }
  else
    ret->url = gavl_strdup(location);

  /* Set name */

  if(title)
    ret->name = bgav_sprintf("%s Stream %d", title, (*index)+1);
  else
    ret->name = bgav_sprintf("%s Stream %d", location, (*index)+1);

  if(bitrate || language)
    {
    ret->name = gavl_strcat(ret->name, " (");
    if(bitrate)
      {
      i = atoi(bitrate);
      tmp_string = bgav_sprintf("%d kbps", i / 1000);
      ret->name = gavl_strcat(ret->name, tmp_string);
      free(tmp_string);
      }

    if(bitrate && language)
      {
      ret->name = gavl_strcat(ret->name, ", ");
      }
    
    if(language)
      {
      ret->name = gavl_strcat(ret->name, get_language(language));
      }
    ret->name = gavl_strcat(ret->name, ")");
    }
  
  (*index)++;
  }

static int get_urls(bgav_yml_node_t * n,
                    bgav_redirector_context_t * r,
                    const char * title, const char * url_base, int * index)
  {
  while(n)
    {
    if(!sc(n->name, "audio") || !sc(n->name, "video"))
      {
      get_url(n, &r->urls[*index], title, url_base, index);
      }
    else if(n->children)
      {
      get_urls(n->children, r, title, url_base, index);
      }
    n = n->next;
    }
  return 1;
  }
#endif
static int xml_2_smil(bgav_redirector_context_t * r, bgav_yml_node_t * n)
  {
  int index;
  bgav_yml_node_t * node;
  bgav_yml_node_t * child_node;
  char * url_base = NULL;
  const char * title = NULL;
  char * pos;
  r->num_urls = 0;

  n = bgav_yml_find_by_name(n, "smil");
  
  if(!n)
    return 0;
  
  node = n->children;


  /* Get the header */
  
  while(node)
    {
    if(!sc(node->name, "head"))
      {
      child_node = node->children;
      //      title = gavl_strdup(node->children->str);

      while(child_node)
        {
        /* Parse meta info */
        
        if(!sc(child_node->name, "meta"))
          {
          
          /* Fetch url base */
          
          if(!url_base)
            {
            if(!sc(bgav_yml_get_attribute(child_node, "name"), "base"))
              {
              url_base =
                gavl_strdup(bgav_yml_get_attribute(child_node, "content"));
              }
            }

          /* Fetch title */

          if(!title)
            {
            if(!sc(bgav_yml_get_attribute(child_node, "name"), "title"))
              {
              title = bgav_yml_get_attribute(child_node, "content");
              }
            }


          }
        child_node = child_node->next;
        }
      }
    
    if(!sc(node->name, "body"))
      break;
    node = node->next;
    }

  if(!url_base && r->input->url)
    {
    pos = strrchr(r->input->url, '/');
    if(pos)
      {
      pos++;
      url_base = gavl_strndup(r->input->url, pos);
      }
    }
  
  /* Count the entries */

  r->num_urls = count_urls(node->children);
  
  r->urls = calloc(r->num_urls, sizeof(*(r->urls)));
  
  /* Now, loop through all streams and collect the values */
  
  index = 0;

  get_urls(node->children, r, title, url_base, &index);

  if(url_base)
    free(url_base);
  
  return 1;
  }


static int parse_smil(bgav_redirector_context_t * r)
  {
  int result;

  bgav_yml_node_t * node;

  node = bgav_yml_parse(r->input);


  if(!node)
    {
    bgav_log(r->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Parse smil failed (yml error)");
    return 0;
    }
  //  bgav_yml_dump(node);
  
  result = xml_2_smil(r, node);

  bgav_yml_free(node);
  if(!result)
    bgav_log(r->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Parse smil failed");
  return result;
  }

const bgav_redirector_t bgav_redirector_smil = 
  {
    .name =  "smil",
    .probe = probe_smil,
    .parse = parse_smil
  };
