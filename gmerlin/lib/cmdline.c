/*****************************************************************
 
  cmdline.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include <config.h>
#include <translation.h>

#include <cfg_registry.h>
#include <cmdline.h>
#include <utils.h>

/* Terminal related functions */

#define MAX_COLS 79 /* For line-wrapping */

static void dump_string_term(FILE * out, const char * str, int indent,
                             const char * translation_domain)
  {
  const char * start;
  const char * end;
  const char * pos;
  
  char * spaces;
  spaces = calloc(indent+1, 1);
  memset(spaces, ' ', indent);

  str = TR_DOM(str);
  
  start = str;
  pos   = str;
  end   = str;
  
  while(1)
    {
    if(isspace(*pos))
      end = pos;

    if(pos - start + strlen(spaces) + 2 > MAX_COLS)
      {
      fprintf(out, spaces);
      fprintf(out, "  ");
      
      fwrite(start, 1, end - start, out);
      fprintf(out, "\n");
      
      while(isspace(*end))
        end++;
      start = end;
      }
    else if(*pos == '\0')
      {
      fprintf(out, spaces);
      fprintf(out, "  ");
      
      fwrite(start, 1, pos - start, out);
      fprintf(out, "\n");
      break;
      }
    else if(*pos == '\n')
      {
      fprintf(out, spaces);
      fprintf(out, "  ");
      
      fwrite(start, 1, pos - start, out);
      fprintf(out, "\n");

      pos++;
      
      start = pos;
      end = pos;
      }
    
    else
      pos++;
    }

  free(spaces);
  }

static void font_bold(FILE * out)
  {
  char ansi_bold[] = { 27, '[', '1', 'm', '\0' };
  //  if(isatty(fileno(out)))
    fprintf(out, ansi_bold);
  }

static void font_underline(FILE * out)
  {
  char ansi_underline[] = { 27, '[', '4', 'm', '\0' };
  //  if(isatty(fileno(out)))
    fprintf(out, ansi_underline);
  }

static void font_normal(FILE * out)
  {
  char ansi_normal[] = { 27, '[', '0', 'm', '\0' };
  //  if(isatty(fileno(out)))
    fprintf(out, ansi_normal);
  }

/* */

void bg_cmdline_remove_arg(int * argc, char *** _argv, int arg)
  {
  char ** argv = *_argv;
  /* Move the upper args down */
  if(arg < *argc - 1)
    memmove(argv + arg, argv + arg + 1, 
            (*argc - arg - 1) * sizeof(*argv));
  (*argc)--;
  }

void bg_cmdline_parse(bg_cmdline_arg_t * args, int * argc, char *** _argv,
                      void * callback_data)
  {
  int found;
  int i, j;
  char ** argv = *_argv;

  i = 1;

  while(i < *argc)
    {
    j = 0;
    found = 0;

    if(!strcmp(argv[i], "--"))
      break;
    
    while(args[j].arg)
      {
      if(!strcmp(args[j].arg, argv[i]))
        {
        bg_cmdline_remove_arg(argc, _argv, i);
        if(args[j].callback)
          args[j].callback(callback_data, argc, _argv, i);
        else if(args[j].parameters)
          {
          }
          
        found = 1;
        break;
        }
      else
        j++;
      }
    if(!found)
      i++;
    }
  }

char ** bg_cmdline_get_locations_from_args(int * argc, char *** _argv)
  {
  char ** ret;
  int seen_dashdash;
  char ** argv;
  int i, index;
  int num_locations = 0;
  argv = *_argv;
  
  /* Count the locations */

  for(i = 1; i < *argc; i++)
    {
    if(!strcmp(argv[i], "--"))
      {
      num_locations += *argc - 1 - i;
      break;
      }
    else if(argv[i][0] != '-')
      num_locations++;
    }

  if(!num_locations)
    return (char**)0;
  
  /* Allocate return value */

  ret = calloc(num_locations + 1, sizeof(*ret));

  i = 1;
  index = 0;
  seen_dashdash = 0;
  
  while(i < *argc)
    {
    if(seen_dashdash || (argv[i][0] != '-'))
      {
      ret[index++] = argv[i];
      bg_cmdline_remove_arg(argc, _argv, i);
      }
    else if(!strcmp(argv[i], "--"))
      {
      seen_dashdash = 1;
      bg_cmdline_remove_arg(argc, _argv, i);
      }
    else
      {
      i++;
      }
    }
  return ret;
  }

void bg_cmdline_print_help(bg_cmdline_arg_t* args)
  {
  int i = 0;
  FILE * out = stdout;
  
  while(args[i].arg)
    {
    font_bold(out);
    fprintf(out, "%s", args[i].arg);
    font_normal(out);
    
    if(args[i].help_arg)
      {
      fprintf(out, " ");
      
      font_underline(out);
      fprintf(out, "%s\n", args[i].help_arg);
      font_normal(out);
      }
    else
      fprintf(out, "\n");
    
    dump_string_term(out, args[i].help_string, 0, (const char*)0);
    
    if(args[i].parameters)
      bg_cmdline_print_help_parameters(args[i].parameters);
    else
      fprintf(out, "\n");
    
    i++;
    }
  
  }

int bg_cmdline_apply_options(bg_cfg_section_t * section,
                             bg_set_parameter_func_t set_parameter,
                             void * data,
                             bg_parameter_info_t * parameters,
                             const char * option_string)
  {
  if(!bg_cfg_section_set_parameters_from_string(section,
                                                parameters,
                                                option_string))
    return 0;
  /* Now, apply the section */
  
  bg_cfg_section_apply(section, parameters, set_parameter, data);
  return 1;
  }

static void print_help_parameters(int indent, bg_parameter_info_t * parameters)
  {
  int i = 0;
  int j;
  FILE * out = stdout;
  
  int pos;
  
  char * spaces;
  char time_string[GAVL_TIME_STRING_LEN];
  char * tmp_string;

  const char * translation_domain = (const char *)0;
  
  spaces = calloc(indent + 1, 1);
  memset(spaces, ' ', indent);
  
  if(!indent)
    {
    fprintf(out, spaces);
    fprintf(out, TR("\n  Supported options:\n\n"));
    }
  
  while(parameters[i].name)
    {
    if(parameters[i].gettext_domain)
      translation_domain = parameters[i].gettext_domain;
    if(parameters[i].gettext_directory)
      bindtextdomain(translation_domain, parameters[i].gettext_directory);
    
    if((parameters[i].type == BG_PARAMETER_SECTION) ||
       (parameters[i].flags & BG_PARAMETER_HIDE_DIALOG))
      {
      i++;
      continue;
      }
    pos = 0;
    pos += fprintf(out, spaces);

    font_bold(out);
        
    if(parameters[i].opt)
      pos += fprintf(out, "  %s", parameters[i].opt);
    else
      pos += fprintf(out, "  %s", parameters[i].name);
    
    font_normal(out);
    fprintf(out, "=");
    switch(parameters[i].type)
      {
      case BG_PARAMETER_SECTION:
        break;
      case BG_PARAMETER_CHECKBUTTON:
        pos += fprintf(out, TR("[1|0] (default: %d)\n"), parameters[i].val_default.val_i);
        break;
      case BG_PARAMETER_INT:
      case BG_PARAMETER_SLIDER_INT:
        pos += fprintf(out, TR("<number> ("));
        if(parameters[i].val_min.val_i < parameters[i].val_max.val_i)
          {
          pos += fprintf(out, "%d..%d, ",
                         parameters[i].val_min.val_i, parameters[i].val_max.val_i);
          }
        fprintf(out, "default: %d)\n", parameters[i].val_default.val_i);
        break;
      case BG_PARAMETER_SLIDER_FLOAT:
      case BG_PARAMETER_FLOAT:
        pos += fprintf(out, TR("<number> ("));
        if(parameters[i].val_min.val_f < parameters[i].val_max.val_f)
          {
          tmp_string = bg_sprintf("%%.%df..%%.%df, ",
                                  parameters[i].num_digits,
                                  parameters[i].num_digits);
          pos += fprintf(out, tmp_string,
                         parameters[i].val_min.val_f, parameters[i].val_max.val_f);
          free(tmp_string);
          }
        tmp_string =
          bg_sprintf(TR("default: %%.%df)\n"),
                     parameters[i].num_digits);
        fprintf(out, tmp_string,
                parameters[i].val_default.val_f);
        free(tmp_string);
        break;
      case BG_PARAMETER_STRING:
      case BG_PARAMETER_FONT:
      case BG_PARAMETER_DEVICE:
      case BG_PARAMETER_FILE:
      case BG_PARAMETER_DIRECTORY:
        pos += fprintf(out, TR("<string>"));
        if(parameters[i].val_default.val_str)
          pos += fprintf(out, TR(" (Default: %s)\n"), parameters[i].val_default.val_str);
        else
          pos += fprintf(out, "\n");
        break;
      case BG_PARAMETER_STRING_HIDDEN:
        pos += fprintf(out, TR("<string>\n"));
        break;
      case BG_PARAMETER_STRINGLIST:
        pos += fprintf(out, TR("<string>\n"));
        j = 0;

        pos = 0;
        pos += fprintf(out, spaces);
        pos += fprintf(out, "  ");
        pos += fprintf(out, TR("Supported strings: "));
        
        while(parameters[i].multi_names[j])
          {
          if(j) pos += fprintf(out, " ");

          if(pos + strlen(parameters[i].multi_names[j]+1) > MAX_COLS)
            {
            fprintf(out, "\n");
            pos = 0;
            pos += fprintf(out, spaces);
            pos += fprintf(out, "    ");
            }
          
          pos += fprintf(out, parameters[i].multi_names[j]);
          j++;
          }
        fprintf(out, "\n");
        fprintf(out, spaces);
        fprintf(out, "  ");
        fprintf(out, TR("Default: %s\n"), parameters[i].val_default.val_str);
        break;
      case BG_PARAMETER_COLOR_RGB:
        fprintf(out, TR("<r>,<g>,<b> (default: %.3f,%.3f,%.3f)\n"),
                parameters[i].val_default.val_color[0],
                parameters[i].val_default.val_color[1],
                parameters[i].val_default.val_color[2]);
        fprintf(out, spaces);
        fprintf(out, TR("  <r>, <g> and <b> are in the range 0.0..1.0\n"));
        break;
      case BG_PARAMETER_COLOR_RGBA:
        fprintf(out, TR("<r>,<g>,<b>,<a> (default: %.3f,%.3f,%.3f,%.3f)\n"),
                parameters[i].val_default.val_color[0],
                parameters[i].val_default.val_color[1],
                parameters[i].val_default.val_color[2],
                parameters[i].val_default.val_color[3]);
        fprintf(out, spaces);
        fprintf(out, TR("  <r>, <g>, <b> and <a> are in the range 0.0..1.0\n"));
        break;
      case BG_PARAMETER_MULTI_MENU:
        fprintf(out, TR("option[{suboptions}]\n"));
        pos = 0;
        pos += fprintf(out, spaces);
        pos += fprintf(out, "  ");
        pos += fprintf(out, TR("Supported options: "));
        j = 0;
        while(parameters[i].multi_names[j])
          {
          if(j) pos += fprintf(out, " ");

          if(pos + strlen(parameters[i].multi_names[j]) > MAX_COLS)
            {
            fprintf(out, "\n");
            pos = 0;
            pos += fprintf(out, spaces);
            pos += fprintf(out, "    ");
            }
          pos += fprintf(out, parameters[i].multi_names[j]);
          j++;
          }
        fprintf(out, "\n");
        fprintf(out, spaces);
        fprintf(out, "  ");
        fprintf(out, TR("Default: %s\n"), parameters[i].val_default.val_str);
        break;
      case BG_PARAMETER_MULTI_LIST:
      case BG_PARAMETER_MULTI_CHAIN:
        fprintf(out, TR("{option[{suboptions}][:option[{suboptions}]...]}\n"));
        pos = 0;
        pos += fprintf(out, spaces);
        pos += fprintf(out, "  ");
        pos += fprintf(out, TR("Supported options: "));
        j = 0;
        while(parameters[i].multi_names[j])
          {
          if(j) pos += fprintf(out, " ");
          
          if(pos + strlen(parameters[i].multi_names[j]+1) > MAX_COLS)
            {
            fprintf(out, "\n");
            pos = 0;
            pos += fprintf(out, spaces);
            pos += fprintf(out, "    ");
            }
          pos += fprintf(out, parameters[i].multi_names[j]);
          j++;
          }
        fprintf(out, "\n\n");
        
        break;
      case BG_PARAMETER_TIME:
        fprintf(out, TR("{[[HH:]MM:]SS} ("));
        if(parameters[i].val_min.val_time < parameters[i].val_max.val_time)
          {
          gavl_time_prettyprint(parameters[i].val_min.val_time, time_string);
          fprintf(out, "%s..", time_string);
          
          gavl_time_prettyprint(parameters[i].val_max.val_time, time_string);
          fprintf(out, "%s, ", time_string);
          }
        gavl_time_prettyprint(parameters[i].val_default.val_time, time_string);
        fprintf(out, TR("default: %s)\n"), time_string);
        fprintf(out, spaces);
        fprintf(out, TR("  Seconds can be fractional (i.e. with decimal point)\n"));
        
        break;
      }

    fprintf(out, spaces);
    fprintf(out, "  %s\n", TR_DOM(parameters[i].long_name));

    if(parameters[i].help_string)
      dump_string_term(out, parameters[i].help_string, indent, translation_domain);

    fprintf(out, "\n");

    /* Print suboptions */

    if(parameters[i].multi_parameters)
      {
      j = 0;
      while(parameters[i].multi_names[j])
        {
        if(parameters[i].multi_parameters[j])
          {
          fprintf(out, spaces);

          if(parameters[i].type == BG_PARAMETER_MULTI_LIST)
            {
            fprintf(out, TR("  Suboptions for %s\n\n"),
                    parameters[i].multi_names[j]);
            
            }
          else
            {
            if(parameters[i].opt)
              fprintf(out, TR("  Suboptions for %s=%s\n\n"),
                      parameters[i].opt,parameters[i].multi_names[j]);
            else
              fprintf(out, TR("  Suboptions for %s=%s\n\n"),
                      parameters[i].name,parameters[i].multi_names[j]);
            }
          print_help_parameters(indent+2, parameters[i].multi_parameters[j]);
          }
        j++;
        }
      }
    
    i++;
    }
  
  free(spaces);
  }

void bg_cmdline_print_help_parameters(bg_parameter_info_t * parameters)
  {
  print_help_parameters(0, parameters);
  }
