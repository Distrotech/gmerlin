
/*
 *  Remove the nth arg from an argc/argv pair
 */

void bg_cmdline_remove_arg(int * argc, char *** argv, int arg);

/* cmdline options */

typedef struct
  {
  char * short_arg;
  char * long_arg;
  char * help_string;
  void (*callback)(void * data, int * argc, char *** argv, int arg);
  } bg_cmdline_arg;

void bg_cmdline_parse(bg_cmdline_arg *, int * argc, char *** argv, void * callback_data);

void bg_cmdline_print_help(bg_cmdline_arg *);

char ** bg_cmdline_get_locations_from_args(int * argc, char *** argv);
