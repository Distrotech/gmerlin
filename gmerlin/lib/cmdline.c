#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <cfg_registry.h>
#include <cmdline.h>

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

int bg_cmdline_apply_options(bg_cfg_section_t * section,
                             bg_set_parameter_func set_parameter,
                             void * data,
                             bg_parameter_info_t * parameters,
                             const char * option_string)
  {
  int parameter_index;
  int len;
  const char * start;
  const char * end;
  //  bg_parameter_info_t * parameter;

  start = option_string;
  
  while(1)
    {
    /* Get the options name */
    end = start;
    while((*end != '=') && (*end != '\0'))
      end++;
    if(*end == '\0')
      break;
    /* Now, find the parameter info */
    parameter_index = 0;

    while(parameters[parameter_index].name)
      {
      if(!parameters[parameter_index].opt)
        continue;
      if((strlen(parameters[parameter_index].opt) == (end - start)) &&
         !strncmp(parameters[parameter_index].opt, start, end - start))
        break;
      parameter_index++;
      }

    if(!parameters[parameter_index].name)
      {
      return 0;
      }
    
    fprintf(stderr, "Found option %s\n", parameters[parameter_index].long_name);
    end++;
    start = end;
    fprintf(stderr, "start: %s\n", start);
    len = bg_cfg_section_set_parameter_from_string(section,
                                                   &(parameters[parameter_index]),
                                                   start);
    if(!len)
      return 0;
    end = start + len;
    if(*end == ',')
      end++;
    else
      break;

    start = end;
    }

  /* Now, apply the section */

  bg_cfg_section_apply(section, parameters,
                       set_parameter,
                       data);
  
  return 1;
  }

void bg_cmdline_print_help_parameters(bg_parameter_info_t * parameters)
  {
  int i = 0;
  int j;
  fprintf(stderr, "  Supported Options:\n");
  while(parameters[i].name)
    {
    if(!parameters[i].opt)
      {
      i++;
      continue;
      }
    fprintf(stderr, "    %s=", parameters[i].opt);

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
          fprintf(stderr, "min: %d, max: %d, ",
                  parameters[i].val_min.val_i, parameters[i].val_max.val_i);
          }
        fprintf(stderr, "default: %d)\n", parameters[i].val_default.val_i);
        break;
      case BG_PARAMETER_SLIDER_FLOAT:
      case BG_PARAMETER_FLOAT:
        fprintf(stderr, "<num> (");
        if(parameters[i].val_min.val_f < parameters[i].val_max.val_f)
          {
          fprintf(stderr, "min: %.2f, max: %.2f, ",
                  parameters[i].val_min.val_f, parameters[i].val_max.val_f);
          }
        fprintf(stderr, "default: %.2f)\n", parameters[i].val_default.val_f);
        break;
      case BG_PARAMETER_STRING:
      case BG_PARAMETER_STRING_HIDDEN:
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
        break;
      case BG_PARAMETER_COLOR_RGBA:
        break;
      case BG_PARAMETER_FONT:
        break;
      case BG_PARAMETER_DEVICE:
        break;
      case BG_PARAMETER_FILE:
        break;
      case BG_PARAMETER_DIRECTORY:
        break;
      case BG_PARAMETER_MULTI_MENU:
        break;
      case BG_PARAMETER_MULTI_LIST:
        break;
      case BG_PARAMETER_TIME:
        break;
      }

    fprintf(stderr, "    %s\n", parameters[i].long_name);
    if(parameters[i].help_string)
      fprintf(stderr, "    %s\n", parameters[i].help_string);
    
    i++;
    fprintf(stderr, "\n");
    }
  }
