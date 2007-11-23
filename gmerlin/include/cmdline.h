
/* Automatic generation of user documentation */

typedef enum
  {
    BG_HELP_FORMAT_PLAIN,
    BG_HELP_FORMAT_TERM,
    BG_HELP_FORMAT_MAN,
    BG_HELP_FORMAT_TEXI,
  } bg_help_format_t;

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

typedef struct
  {
  char             * name;
  bg_cmdline_arg_t * args;
  } bg_cmdline_arg_array_t;

typedef struct
  {
  char * package;
  char * version;
  char * name;
  char * synopsis;
  char * help_before;
  bg_cmdline_arg_array_t * args; /* Null terminated */
  char * help_after;
  } bg_cmdline_app_data_t;

void bg_cmdline_parse(bg_cmdline_arg_t *,
                      int * argc, char *** argv, void * callback_data,
                      bg_cmdline_app_data_t * app_data);


char ** bg_cmdline_get_locations_from_args(int * argc, char *** argv);

void bg_cmdline_print_help(bg_cmdline_app_data_t * app_data, char * argv0, bg_help_format_t);

/* Commandline -> Config registry and parameters */


void bg_cmdline_print_help_parameters(bg_parameter_info_t * parameters,
                                      bg_help_format_t);

int bg_cmdline_apply_options(bg_cfg_section_t * section,
                              bg_set_parameter_func_t set_parameter,
                              void * data,
                              bg_parameter_info_t * parameters,
                              const char * option_string);

void bg_cmdline_print_version(const char * application);

