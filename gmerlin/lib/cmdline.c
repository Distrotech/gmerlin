#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include <cfg_registry.h>
#include <cmdline.h>
#include <utils.h>

/* Terminal related functions */

#define MAX_COLS 79 /* For line-wrapping */

static void dump_string_term(FILE * out, const char * str, int indent)
  {
  const char * start;
  const char * end;
  const char * pos;

  char * spaces;
  spaces = calloc(indent+1, 1);
  memset(spaces, ' ', indent);
    
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
      //      fprintf(stderr, "argv[%d/%d]: %s\n", i, *argc, argv[i]);
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
    
    dump_string_term(out, args[i].help_string, 0);
    
    if(args[i].parameters)
      bg_cmdline_print_help_parameters(args[i].parameters);
    else
      fprintf(out, "\n");
    
    i++;
    }
  
  }

static int apply_options(bg_cfg_section_t * section,
                         bg_parameter_info_t * parameters,
                         const char * option_string)
  {
  bg_cfg_section_t * child_section;
  int parameter_index, child_index;
  int len, i;
  const char * start;
  const char * end;

  int result;
  
  char * tmp_string;
  
  start = option_string;
  
  while(1)
    {
    /* Get the options name */
    end = start;
    while((*end != '=') && (*end != '\0'))
      end++;

    if(*end == '\0')
      goto fail;
    
    /* Now, find the parameter info */
    parameter_index = 0;

    while(parameters[parameter_index].name)
      {
      if(!parameters[parameter_index].opt)
        {
        if((strlen(parameters[parameter_index].name) == (end - start)) &&
           !strncmp(parameters[parameter_index].name, start, end - start))
          break;
        }
      else
        {
        if((strlen(parameters[parameter_index].opt) == (end - start)) &&
           !strncmp(parameters[parameter_index].opt, start, end - start))
          break;
        }
      parameter_index++;
      }

    if(!parameters[parameter_index].name)
      {
      fprintf(stderr, "No such option %s\n", start);
      goto fail;
      }
    
    //    fprintf(stderr, "Found option %s\n", parameters[parameter_index].long_name);
    end++; // Skip '='
    start = end;
    //    fprintf(stderr, "start: %s\n", start);
    len = bg_cfg_section_set_parameter_from_string(section,
                                                   &(parameters[parameter_index]),
                                                   start);
    if(!len)
      goto fail;
    end = start + len;

    if(*end == ':')
      end++;
    else if(*end == '}')
      end++;
    else if(*end == '{')
      {
      /* Apply child parameters */
      if(!parameters[parameter_index].multi_parameters ||
         !parameters[parameter_index].multi_names)
        goto fail;
      
      tmp_string = bg_strndup((char*)0, start, end);

      child_index = 0;
      while(strcmp(parameters[parameter_index].multi_names[child_index], tmp_string))
        {
        if(!parameters[parameter_index].multi_names[child_index])
          goto fail;
        child_index++;
        }
      
      child_section = bg_cfg_section_find_subsection(section,
                                                     parameters[parameter_index].name);
      child_section = bg_cfg_section_find_subsection(child_section,
                                                     tmp_string);
      free(tmp_string);
      
      end++;
      start = end;
      
      if(!(result = apply_options(child_section,
                                  parameters[parameter_index].multi_parameters[child_index],
                                  start)))
        return 0;
      end = start + result;
      }
    else if((*end == '\0') || (*end == '}'))
      break;
    else
      goto fail;
    start = end;
    if(*start == '\0')
      break;
    }
  
  return end - option_string;

  fail:
  fprintf(stderr, "Error parsing option\n");
  fprintf(stderr, "%s\n", option_string);

  for(i = 0; i < (int)(end - option_string); i++)
    fprintf(stderr, " ");
  fprintf(stderr, "^\n");
  return 0;
  }

int bg_cmdline_apply_options(bg_cfg_section_t * section,
                             bg_set_parameter_func_t set_parameter,
                             void * data,
                             bg_parameter_info_t * parameters,
                             const char * option_string)
  {
  if(!apply_options(section, parameters, option_string))
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
  char tmp_string[GAVL_TIME_STRING_LEN];
    
  spaces = calloc(indent + 1, 1);
  memset(spaces, ' ', indent);
  
  
  if(!indent)
    {
    fprintf(out, spaces);
    fprintf(out, "\n  Supported options:\n\n");
    }
  
  while(parameters[i].name)
    {
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
        pos += fprintf(out, "[1|0] (default: %d)\n", parameters[i].val_default.val_i);
        break;
      case BG_PARAMETER_INT:
      case BG_PARAMETER_SLIDER_INT:
        pos += fprintf(out, "<num> (");
        if(parameters[i].val_min.val_i < parameters[i].val_max.val_i)
          {
          pos += fprintf(out, "%d..%d, ",
                         parameters[i].val_min.val_i, parameters[i].val_max.val_i);
          }
        fprintf(out, "default: %d)\n", parameters[i].val_default.val_i);
        break;
      case BG_PARAMETER_SLIDER_FLOAT:
      case BG_PARAMETER_FLOAT:
        pos += fprintf(out, "<num> (");
        if(parameters[i].val_min.val_f < parameters[i].val_max.val_f)
          {
          pos += fprintf(out, "%.2f..%.2f, ",
                         parameters[i].val_min.val_f, parameters[i].val_max.val_f);
          }
        fprintf(out, "default: %.2f)\n", parameters[i].val_default.val_f);
        break;
      case BG_PARAMETER_STRING:
      case BG_PARAMETER_FONT:
      case BG_PARAMETER_DEVICE:
      case BG_PARAMETER_FILE:
      case BG_PARAMETER_DIRECTORY:
        pos += fprintf(out, "<string>");
        if(parameters[i].val_default.val_str)
          pos += fprintf(out, " (Default: %s)\n", parameters[i].val_default.val_str);
        else
          pos += fprintf(out, "\n");
        break;
      case BG_PARAMETER_STRING_HIDDEN:
        pos += fprintf(out, "<string>\n");
        break;
      case BG_PARAMETER_STRINGLIST:
        j = 0;
        while(parameters[i].multi_names[j])
          {
          if(j) pos += fprintf(out, "|");

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
        fprintf(out, " (default: %s)\n", parameters[i].val_default.val_str);
        break;
      case BG_PARAMETER_COLOR_RGB:
        fprintf(out, "<r>,<g>,<b> (default: %.3f,%.3f,%.3f)\n",
                parameters[i].val_default.val_color[0],
                parameters[i].val_default.val_color[1],
                parameters[i].val_default.val_color[2]);
        fprintf(out, spaces);
        fprintf(out, "  <r>, <g> and <b> are in the range 0.0..1.0\n");
        break;
      case BG_PARAMETER_COLOR_RGBA:
        fprintf(out, "<r>,<g>,<b>,<a> (default: %.3f,%.3f,%.3f,%.3f)\n",
                parameters[i].val_default.val_color[0],
                parameters[i].val_default.val_color[1],
                parameters[i].val_default.val_color[2],
                parameters[i].val_default.val_color[3]);
        fprintf(out, spaces);
        fprintf(out, "  <r>, <g>, <b> and <a> are in the range 0.0..1.0\n");
        break;
      case BG_PARAMETER_MULTI_MENU:
        j = 0;
        while(parameters[i].multi_names[j])
          {
          if(j) fprintf(out, "|");

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
        fprintf(out, "[{<suboptions>}] (default: %s)\n", parameters[i].val_default.val_str);
        break;
      case BG_PARAMETER_MULTI_LIST:
        break;
      case BG_PARAMETER_TIME:
        fprintf(out, "{[[HH:]MM:]SS} (");
        if(parameters[i].val_min.val_time < parameters[i].val_max.val_time)
          {
          gavl_time_prettyprint(parameters[i].val_min.val_time, tmp_string);
          fprintf(out, "%s..", tmp_string);
          
          gavl_time_prettyprint(parameters[i].val_max.val_time, tmp_string);
          fprintf(out, "%s, ", tmp_string);
          }
        gavl_time_prettyprint(parameters[i].val_default.val_time, tmp_string);
        fprintf(out, "default: %s)\n", tmp_string);
        fprintf(out, spaces);
        fprintf(out, "  Seconds can be fractional (i.e. with decimal point)\n");
        
        break;
      }

    fprintf(out, spaces);
    fprintf(out, "  %s\n", parameters[i].long_name);

    if(parameters[i].help_string)
      dump_string_term(out, parameters[i].help_string, indent);

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
          if(parameters[i].opt)
            fprintf(out, "  Suboptions for %s=%s\n\n",
                    parameters[i].opt,parameters[i].multi_names[j]);
          else
            fprintf(out, "  Suboptions for %s=%s\n\n",
                    parameters[i].name,parameters[i].multi_names[j]);

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
