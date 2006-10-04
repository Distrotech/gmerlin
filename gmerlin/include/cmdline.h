
/*
 *  Remove the nth arg from an argc/argv pair
 */

void bg_cmdline_remove_arg(int * argc, char *** argv, int arg);

/* cmdline options */

typedef struct
  {
  char * arg;
  char * help_arg; /* Something like <file> */
  char * help_string;
  /* Callback will be called if present */
  void (*callback)(void * data, int * argc, char *** argv, int arg);
  /* Parameter interface */
  bg_parameter_info_t * parameters;
  int param_single;
  } bg_cmdline_arg_t;

void bg_cmdline_parse(bg_cmdline_arg_t *,
                      int * argc, char *** argv, void * callback_data);


char ** bg_cmdline_get_locations_from_args(int * argc, char *** argv);

void bg_cmdline_print_help(bg_cmdline_arg_t* args);

/* Commandline -> Config registry and parameters */

void bg_cmdline_print_help_parameters(bg_parameter_info_t * parameters);

int bg_cmdline_apply_options(bg_cfg_section_t * section,
                              bg_set_parameter_func_t set_parameter,
                              void * data,
                              bg_parameter_info_t * parameters,
                              const char * option_string);

