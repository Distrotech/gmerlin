#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <cfg_registry.h>
#include <cmdline.h>
#include <utils.h>

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

  while(args[i].arg)
    {
    fprintf(stderr, "%s", args[i].arg);

    if(args[i].help_arg)
      fprintf(stderr, " %s\n", args[i].help_arg);
    else
      fprintf(stderr, "\n");
    fprintf(stderr, "  %s\n", args[i].help_string);

    if(args[i].parameters)
      bg_cmdline_print_help_parameters(args[i].parameters);
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

  char * spaces;
  char tmp_string[GAVL_TIME_STRING_LEN];

  spaces = calloc(indent + 1, 1);
  memset(spaces, ' ', indent);
  
  fprintf(stderr, spaces);
  fprintf(stderr, "  Supported options:\n");
  while(parameters[i].name)
    {
    if((parameters[i].type == BG_PARAMETER_SECTION) ||
       (parameters[i].flags & BG_PARAMETER_HIDE_DIALOG))
      {
      i++;
      continue;
      }
      
    fprintf(stderr, spaces);
    if(parameters[i].opt)
      fprintf(stderr, "  %s=", parameters[i].opt);
    else
      fprintf(stderr, "  %s=", parameters[i].name);
    
    switch(parameters[i].type)
      {
      case BG_PARAMETER_SECTION:
        break;
      case BG_PARAMETER_CHECKBUTTON:
        fprintf(stderr, "[1|0] (default: %d)\n", parameters[i].val_default.val_i);
        break;
      case BG_PARAMETER_INT:
      case BG_PARAMETER_SLIDER_INT:
        fprintf(stderr, "<num> (");
        if(parameters[i].val_min.val_i < parameters[i].val_max.val_i)
          {
          fprintf(stderr, "%d..%d, ",
                  parameters[i].val_min.val_i, parameters[i].val_max.val_i);
          }
        fprintf(stderr, "default: %d)\n", parameters[i].val_default.val_i);
        break;
      case BG_PARAMETER_SLIDER_FLOAT:
      case BG_PARAMETER_FLOAT:
        fprintf(stderr, "<num> (");
        if(parameters[i].val_min.val_f < parameters[i].val_max.val_f)
          {
          fprintf(stderr, "%.2f..%.2f, ",
                  parameters[i].val_min.val_f, parameters[i].val_max.val_f);
          }
        fprintf(stderr, "default: %.2f)\n", parameters[i].val_default.val_f);
        break;
      case BG_PARAMETER_STRING:
      case BG_PARAMETER_FONT:
      case BG_PARAMETER_DEVICE:
      case BG_PARAMETER_FILE:
      case BG_PARAMETER_DIRECTORY:
        fprintf(stderr, "<string>");
        if(parameters[i].val_default.val_str)
          fprintf(stderr, " (Default: %s)\n", parameters[i].val_default.val_str);
        else
          fprintf(stderr, "\n");
        break;
      case BG_PARAMETER_STRING_HIDDEN:
        fprintf(stderr, "<string>\n");
        break;
      case BG_PARAMETER_STRINGLIST:
        j = 0;
        while(parameters[i].multi_names[j])
          {
          if(j) fprintf(stderr, "|");
          fprintf(stderr, parameters[i].multi_names[j]);
          j++;
          }
        fprintf(stderr, " (default: %s)\n", parameters[i].val_default.val_str);
        break;
      case BG_PARAMETER_COLOR_RGB:
        fprintf(stderr, "R,G,B (default: %.3f,%.3f,%.3f)\n",
                parameters[i].val_default.val_color[0],
                parameters[i].val_default.val_color[1],
                parameters[i].val_default.val_color[2]);
        fprintf(stderr, spaces);
        fprintf(stderr, "  R, G, and B are in the range 0.0..1.0\n");
        break;
      case BG_PARAMETER_COLOR_RGBA:
        fprintf(stderr, "R,G,B,A (default: %.3f,%.3f,%.3f,%.3f)\n",
                parameters[i].val_default.val_color[0],
                parameters[i].val_default.val_color[1],
                parameters[i].val_default.val_color[2],
                parameters[i].val_default.val_color[3]);
        fprintf(stderr, spaces);
        fprintf(stderr, "  R, G, B and A are in the range 0.0..1.0\n");
        break;
      case BG_PARAMETER_MULTI_MENU:
        j = 0;
        while(parameters[i].multi_names[j])
          {
          if(j) fprintf(stderr, "|");
          fprintf(stderr, parameters[i].multi_names[j]);
          j++;
          }
        fprintf(stderr, "[{<suboptions>}] (default: %s)\n", parameters[i].val_default.val_str);
        break;
      case BG_PARAMETER_MULTI_LIST:
        break;
      case BG_PARAMETER_TIME:
        fprintf(stderr, "{[[HH:]MM:]SS} (");
        if(parameters[i].val_min.val_time < parameters[i].val_max.val_time)
          {
          gavl_time_prettyprint(parameters[i].val_min.val_time, tmp_string);
          fprintf(stderr, "%s..", tmp_string);
          
          gavl_time_prettyprint(parameters[i].val_max.val_time, tmp_string);
          fprintf(stderr, "%s, ", tmp_string);
          }
        gavl_time_prettyprint(parameters[i].val_default.val_time, tmp_string);
        fprintf(stderr, "default: %s)\n", tmp_string);
        fprintf(stderr, spaces);
        fprintf(stderr, "  Seconds can be fractional (i.e. with decimal point)\n");
        
        break;
      }

    fprintf(stderr, spaces);
    fprintf(stderr, "  %s\n", parameters[i].long_name);
    if(parameters[i].help_string)
      {
      fprintf(stderr, spaces);
      fprintf(stderr, "  %s\n", parameters[i].help_string);
      }
    fprintf(stderr, "\n");

    /* Print suboptions */

    if(parameters[i].multi_parameters)
      {
      j = 0;
      while(parameters[i].multi_names[j])
        {
        if(parameters[i].multi_parameters[j])
          {
          fprintf(stderr, spaces);
          if(parameters[i].opt)
            fprintf(stderr, "  Suboptions for %s=%s\n",
                    parameters[i].opt,parameters[i].multi_names[j]);
          else
            fprintf(stderr, "  Suboptions for %s=%s\n",
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
