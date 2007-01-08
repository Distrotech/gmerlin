/*****************************************************************
 
  languages.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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

#include <avdec_private.h>
#include <language_table.h>

static int num_lang = sizeof(language_codes)/sizeof(language_codes[0]);

const char * bgav_lang_from_name(const char * name)
  {
  int i;
  for(i = 0; i < num_lang; i++)
    {
    if(!strcmp(name, language_codes[i].name))
      return language_codes[i].iso_639_b;
    }
  return (char*)0;
  }

const char * bgav_lang_from_twocc(const char * twocc)
  {
  int i;

  for(i = 0; i < num_lang; i++)
    {
    if(language_codes[i].iso_639_2 &&
       (language_codes[i].iso_639_2[0] == twocc[0]) &&
       (language_codes[i].iso_639_2[1] == twocc[1]))
      return language_codes[i].iso_639_b;
    }
  return (char*)0;
  }

const char * bgav_lang_name(const char * lang)
  {
  int i;

  for(i = 0; i < num_lang; i++)
    {
    if(language_codes[i].iso_639_b &&
       (language_codes[i].iso_639_b[0] == lang[0]) &&
       (language_codes[i].iso_639_b[1] == lang[1]) && 
       (language_codes[i].iso_639_b[2] == lang[2]))
      return language_codes[i].name;
    }
  return (char*)0;
  }

void bgav_correct_language(char * lang)
  {
  int i;
  char lang_save[4];
  memcpy(lang_save, lang, 3);
  lang_save[3] = '\0';
  memset(lang, 0, 4);
  
  for(i = 0; i < num_lang; i++)
    {
    if(language_codes[i].iso_639_b &&
       !strcmp(language_codes[i].iso_639_b, lang_save))
      {
      memcpy(lang, language_codes[i].iso_639_b, 3);
      break;
      }
    else if(language_codes[i].iso_639_t &&
            language_codes[i].iso_639_b &&
            !strcmp(language_codes[i].iso_639_t,lang_save ))
      {
      memcpy(lang, language_codes[i].iso_639_b, 3);
      break;
      }
    }
  }
