/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>

#include <cfg_dialog.h>
#include <cmdline.h>

#include <gui_gtk/gtkutils.h>

bg_cfg_section_t  * section_1;
bg_cfg_section_t  * section_2;
bg_cfg_section_t  * section_3;
bg_cfg_section_t  * section_4;

#define PARAMETER_FLAGS BG_PARAMETER_SYNC

static const bg_parameter_info_t multimenu_1_info[] =
  {
    {
      .name =      "multimenu_1_checkbutton_1",
      .long_name = "Multimenu 1 Checkbutton 1",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    {
      .name =      "multimenu_1_checkbutton_2",
      .long_name = "Multimenu 1 Checkbutton 2",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    { /* End of Parameters */ }
  };

static const bg_parameter_info_t multimenu_2_info[] =
  {
    {
      .name =      "multimenu_2_checkbutton_1",
      .long_name = "Multimenu 2 Checkbutton 1",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    {
      .name =      "multimenu_2_checkbutton_2",
      .long_name = "Multimenu 2 Checkbutton 2",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    { /* End of Parameters */ }
  };

static const bg_parameter_info_t multilist_1_info[] =
  {
    {
      .name =      "multilist_1_checkbutton_1",
      .long_name = "Multilist 1 Checkbutton 1",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    {
      .name =      "multilist_1_checkbutton_2",
      .long_name = "Multilist 1 Checkbutton 2",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    { /* End of Parameters */ }
  };

static const bg_parameter_info_t multilist_2_info[] =
  {
    {
      .name =      "multilist_2_checkbutton_1",
      .long_name = "Multilist 2 Checkbutton 1",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    {
      .name =      "multilist_2_checkbutton_2",
      .long_name = "Multilist 2 Checkbutton 2",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    { /* End of Parameters */ }
  };

static const bg_parameter_info_t multichain_1_info[] =
  {
    {
      .name =      "multichain_1_checkbutton_1",
      .long_name = "Multichain 1 Checkbutton 1",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    {
      .name =      "multichain_1_checkbutton_2",
      .long_name = "Multichain 1 Checkbutton 2",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    { /* End of Parameters */ }
  };

static const bg_parameter_info_t multichain_2_info[] =
  {
    {
      .name =      "multichain_2_checkbutton_1",
      .long_name = "Multichain 2 Checkbutton 1",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    {
      .name =      "multichain_2_checkbutton_2",
      .long_name = "Multichain 2 Checkbutton 2",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
    },
    { /* End of Parameters */ }
  };


static const bg_parameter_info_t * multilist_parameters[] =
  {
    multilist_1_info,
    multilist_2_info,
    (bg_parameter_info_t *)0
  };

static const bg_parameter_info_t * multimenu_parameters[] =
  {
    multimenu_1_info,
    multimenu_2_info,
    (bg_parameter_info_t *)0
  };

static const bg_parameter_info_t * multichain_parameters[] =
  {
    multichain_1_info,
    multichain_2_info,
    (bg_parameter_info_t *)0
  };

static const bg_parameter_info_t info_1[] =
  {
#if 0
    {
      .name =      "section_1",
      .long_name = "Section 1",
      .type =      BG_PARAMETER_SECTION
    },
#endif
    {
      .name =        "checkbutton",
      .long_name =   "Check Button",
      .type =        BG_PARAMETER_CHECKBUTTON,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_i = 1 },
      .help_string = "Checkbutton help"
    },
    {
      .name =      "spinbutton_float",
      .long_name = "Floating point Spinbutton",
      .type =      BG_PARAMETER_FLOAT,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_f = 250.0 },
      .val_min =     { .val_f = 200.0 },
      .val_max =     { .val_f = 300.0 },
      .num_digits = 3,
      .help_string = "Floating point Spinbutton help",
    },
    {
      .name =      "spinbutton_int",
      .long_name = "Integer Spinbutton",
      .type =      BG_PARAMETER_INT,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_i = 250 },
      .val_min =     { .val_i = 200 },
      .val_max =     { .val_i = 300 },
      .help_string = "Integer Spinbutton help",
    },
    {
      .name =      "time",
      .long_name = "Time",
      .type =      BG_PARAMETER_TIME,
      .flags =     PARAMETER_FLAGS,
      .help_string = "Time help",
    },
    {
      .name =        "slider_float",
      .long_name =   "Floating point Slider",
      .type =        BG_PARAMETER_SLIDER_FLOAT,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_f = 250.0 },
      .val_min =     { .val_f = 1.0 },
      .val_max =     { .val_f = 300.0 },
      .num_digits =  1,
      .help_string = "Floating point Slider help",
    },
    {
      .name =        "slider_int",
      .long_name =   "Integer Slider",
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_i = 250 },
      .val_min =     { .val_i = 200 },
      .val_max =     { .val_i = 300 },
      .help_string =   "Integer Slider help",
    },
    {
      .name =      "string",
      .long_name = "String",
      .type =      BG_PARAMETER_STRING,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_str = "Some string" },
      .help_string =   "String help",
    },
    {
      .name =      "string_hidden",
      .long_name = "String (hidden)",
      .type =      BG_PARAMETER_STRING_HIDDEN,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_str = "Some string (hidden)" },
      .help_string =   "String hidden help",
    },
    {
      .name =        "stringlist",
      .long_name =   "Stringlist",
      .type =        BG_PARAMETER_STRINGLIST,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_str = "option_2" },
      .multi_names =  (char const *[]){ "option_1", "option_2", "option_3", NULL },
      .multi_labels = (char const *[]){ "Option 1", "Option 2", "Option 3", NULL },
      .help_string =   "Stringlist help"
    },
    { /* End of parameters */ }
  };
    
static const bg_parameter_info_t info_2[] =
  {
#if 0
    {
      .name =      "section_2",
      .long_name = "Section 2",
      .type =      BG_PARAMETER_SECTION
    },
#endif
    {
      .name =      "color_rgb",
      .long_name = "Color RGB",
      .type =      BG_PARAMETER_COLOR_RGB,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_color = { 0.0, 1.0, 0.0 } },
      .help_string =   "Color RGB help",
    },
    {
      .name =      "color_rgba",
      .long_name = "Color RGBA",
      .type =      BG_PARAMETER_COLOR_RGBA,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_color = { 0.0, 1.0, 0.0, 0.5 } },
      .help_string =   "Color RGBA help",
    },
    {
      .name =        "file",
      .long_name =   "File",
      .type =        BG_PARAMETER_FILE,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_str = "/usr/include/stdio.h" },
      .help_string =   "File help",
    },
    {
      .name =        "directory",
      .long_name =   "Directory",
      .type =        BG_PARAMETER_DIRECTORY,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_str = "/usr/local" },
      .help_string =   "Directory help",
    },
    {
      .name =      "font",
      .long_name = "Font",
      .type =      BG_PARAMETER_FONT,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_str = "Sans-12:slant=0:weight=100:width=100" },
      .help_string =   "Font help",
    },
    {
      .name =      "device",
      .long_name = "Device",
      .type =      BG_PARAMETER_DEVICE,
      .flags =     PARAMETER_FLAGS,
      .val_default = { .val_str = "/dev/cdrom" },
      .help_string =   "Device help",
    },
    { /* End of parameters */ }
  };


static const bg_parameter_info_t info_3[] =
  {
#if 0
    {
      .name =      "section_3",
      .long_name = "Section 3",
      .type =      BG_PARAMETER_SECTION
    },
#endif
    {
      .name =               "multimenu",
      .long_name =          "Multimenu",
      .type =               BG_PARAMETER_MULTI_MENU,
      .flags =     PARAMETER_FLAGS,
      .val_default =        { .val_str = "multimenu_1" },
      .multi_names =        (char const *[]){ "multimenu_1", "multimenu_2", NULL },
      .multi_labels =   (char const *[]){ "Multimenu 1", "Multimenu 2", NULL },
      .multi_descriptions = (char const *[]){ "Multimenu 1", "Multimenu 2", NULL },
      .multi_parameters =   multimenu_parameters,
      .help_string =   "Multimenu help",
    },
#if 1
    {
      .name =               "multilist",
      .long_name =          "Multilist",
      .type =               BG_PARAMETER_MULTI_LIST,
      .flags =     PARAMETER_FLAGS,
      .multi_names =        (char const *[]){ "multilist_1", "multilist_2", NULL },
      .multi_labels =   (char const *[]){ "Multilist 1", "Multilist 2", NULL },
      .multi_descriptions = (char const *[]){ "Multilist 1", "Multilist 2", NULL },
      .multi_parameters =   multilist_parameters,
      .help_string =   "Multilist help",
    },
#endif
    { /* End of parameters */ }
  };

static const bg_parameter_info_t info_4[] =
  {
    {
      .name =               "multichain",
      .long_name =          "Multichain",
      .type =               BG_PARAMETER_MULTI_CHAIN,
      .flags =     PARAMETER_FLAGS,
      .multi_names =        (char const *[]){ "multichain_1", "multichain_2", NULL },
      .multi_labels =   (char const *[]){ "Multichain 1", "Multichain 2", NULL },
      .multi_descriptions = (char const *[]){ "Multichain 1", "Multichain 2", NULL },
      .multi_parameters =   multichain_parameters,
      .help_string =   "Multichain help",
    },
    { /* End of parameters */ }
  };


static const bg_parameter_info_t * find_parameter(const bg_parameter_info_t * arr,
                                            const char * name)
  {
  char * pos;
  
  const bg_parameter_info_t * ret;
  int i = 0, j;
  
  i = 0;
  pos = strrchr(name, '.');
  if(pos)
    name = pos + 1;
  
  while(1)
    {
    if(!arr[i].name)
      break;
    
    if(!strcmp(arr[i].name, name))
      {
      return &(arr[i]);
      break;
      }
    else if(arr[i].multi_parameters)
      {
      j = 0;
      while(arr[i].multi_names[j])
        {
        if(arr[i].multi_parameters[j])
          {
          ret = find_parameter(arr[i].multi_parameters[j], name);
          if(ret) return ret;

          }
        j++;
        }
      }
    i++;
    }
  return (bg_parameter_info_t*)0;
  }

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * v)
  {
  const bg_parameter_info_t * tmp_info;
  int i;
  char time_buf[GAVL_TIME_STRING_LEN];

  tmp_info = (const bg_parameter_info_t *)0;

  if(!name)
    {
    fprintf(stderr, "NULL Parameter\n");
    return;
    }
  i = 0;

  tmp_info = find_parameter(info_1, name);
  if(!tmp_info)
    tmp_info = find_parameter(info_2, name);
  if(!tmp_info)
    tmp_info = find_parameter(info_3, name);
  if(!tmp_info)
    tmp_info = find_parameter(info_4, name);
  
  if(!tmp_info)
    {
    fprintf(stderr, "No valid parameter %s\n", name);
    return;
    }

  switch(tmp_info->type)
    {
    case BG_PARAMETER_CHECKBUTTON:
    case BG_PARAMETER_INT:
    case BG_PARAMETER_SLIDER_INT:
      fprintf(stderr, "Integer value %s: %d\n", tmp_info->name, v->val_i);
      break;
    case BG_PARAMETER_TIME:
      gavl_time_prettyprint(v->val_time, time_buf);
      fprintf(stderr, "Time %s\n", time_buf);
      break;
    case BG_PARAMETER_FLOAT:
    case BG_PARAMETER_SLIDER_FLOAT:
      fprintf(stderr, "Float value %s: %f\n", tmp_info->name, v->val_f);
      break;
    case BG_PARAMETER_COLOR_RGB:
    case BG_PARAMETER_COLOR_RGBA:
      fprintf(stderr, "Color %s: %f %f %f %f\n", tmp_info->name,
              v->val_color[0], v->val_color[1],
              v->val_color[2], v->val_color[3]);
      break;
    case BG_PARAMETER_FILE:
    case BG_PARAMETER_DIRECTORY:
    case BG_PARAMETER_STRING:
    case BG_PARAMETER_STRING_HIDDEN:
    case BG_PARAMETER_STRINGLIST:
    case BG_PARAMETER_FONT:
    case BG_PARAMETER_DEVICE:
    case BG_PARAMETER_MULTI_MENU:
    case BG_PARAMETER_MULTI_LIST:
    case BG_PARAMETER_MULTI_CHAIN:
      fprintf(stderr, "String %s: %s\n", tmp_info->name,
              v->val_str);
      break;
    case BG_PARAMETER_SECTION:
      fprintf(stderr, "Section\n");
      break;
    default:
      fprintf(stderr, "Unknown type\n");
      break;
    }
  }

static void opt_opt1(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -opt1 requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(section_1,
                               set_parameter,
                               (void*)0,
                               info_1,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_opt2(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -opt2 requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(section_2,
                               set_parameter,
                               (void*)0,
                               info_2,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_opt3(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -opt3 requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(section_3,
                               set_parameter,
                               (void*)0,
                               info_3,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_opt4(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -opt4 requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(section_4,
                               set_parameter,
                               (void*)0,
                               info_4,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-opt1",
      .help_string = "Set Options 1",
      .callback =    opt_opt1,
      .parameters =  info_1,
    },
    {
      .arg =         "-opt2",
      .help_string = "Set Options 2",
      .callback =    opt_opt2,
      .parameters =  info_2,
    },
    {
      .arg =         "-opt3",
      .help_string = "Set Options 3",
      .callback =    opt_opt3,
      .parameters =  info_3,
    },
    {
      .arg =         "-opt4",
      .help_string = "Set Options 4",
      .callback =    opt_opt4,
      .parameters =  info_4,
    },
    { /* End of options */ }
  };


bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "cfgtest",
    .synopsis = TRS("[Options]"),
    .help_before = TRS("Configure test\n"),
    .args = (bg_cmdline_arg_array_t[]) { { TRS("Options"),
                                         global_options },
                                       {  } },
  };


int main(int argc, char ** argv)
  {
  bg_dialog_t * test_dialog;
  bg_cfg_registry_t * registry;

  registry = bg_cfg_registry_create();
  bg_cfg_registry_load(registry, "config.xml");

  section_1 = bg_cfg_registry_find_section(registry, "section_1");
  section_2 = bg_cfg_registry_find_section(registry, "section_2");
  section_3 = bg_cfg_registry_find_section(registry, "section_3");
  section_4 = bg_cfg_registry_find_section(registry, "section_4");

  bg_cmdline_parse(global_options, &argc, &argv, NULL);
    
  bg_gtk_init(&argc, &argv, (char*)0);

  
  //  test_dialog = bg_dialog_create(section, set_param, NULL, info, "Test dialog");

  test_dialog = bg_dialog_create_multi("Test dialog");

  bg_dialog_add(test_dialog, "Section 1", section_1,set_parameter,(void *)0,info_1);
  bg_dialog_add(test_dialog, "Section 2", section_2,set_parameter,(void *)0,info_2);
  bg_dialog_add(test_dialog, "Section 3", section_3,set_parameter,(void *)0,info_3);
  bg_dialog_add(test_dialog, "Section 4", section_4,set_parameter,(void *)0,info_4);
  
  bg_dialog_show(test_dialog, NULL);

  /* Apply sections */
  fprintf(stderr, "*** Applying section ***\n");  
  bg_cfg_section_apply(section_1,info_1,set_parameter,(void *)0);
  bg_cfg_section_apply(section_2,info_2,set_parameter,(void *)0);
  bg_cfg_section_apply(section_3,info_3,set_parameter,(void *)0);
  bg_cfg_section_apply(section_4,info_4,set_parameter,(void *)0);
  
  bg_cfg_registry_save(registry, "config.xml");

  bg_cfg_registry_destroy(registry);
  bg_dialog_destroy(test_dialog);
  
  return 0;
  }
                      
