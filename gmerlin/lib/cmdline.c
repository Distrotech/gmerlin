#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <cmdline.h>

void bg_remove_arg(int * argc, char *** _argv, int arg)
  {
  char ** argv = *_argv;
  /* Move the upper args down */
  if(arg < *argc - 1)
    memmove(argv + arg, argv + arg + 1, 
            (*argc - arg - 1) * sizeof(*argv));
  (*argc)--;
  }

void bg_cmdline_parse(bg_cmdline_arg * args, int * argc, char *** _argv, void * callback_data)
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
    
    while(args[j].short_arg)
      {
      if(!strcmp(args[j].short_arg, argv[i]) ||
         (args[j].long_arg && !strcmp(args[j].long_arg, argv[i])))
        {
        bg_remove_arg(argc, _argv, i);
        args[j].callback(callback_data, argc, _argv, i);
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
      bg_remove_arg(argc, _argv, i);
      }
    else if(!strcmp(argv[i], "--"))
      {
      seen_dashdash = 1;
      bg_remove_arg(argc, _argv, i);
      }
    else
      {
      i++;
      }
    }
  return ret;
  }

void bg_cmdline_print_help(bg_cmdline_arg * args)
  {
  
  }

