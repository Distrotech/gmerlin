#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>

#include <cfg_dialog.h>
#include <gui_gtk/gtkutils.h>


static bg_parameter_info_t multimenu_1_info[] =
  {
    {
      name:      "multimenu_1_checkbutton 1",
      long_name: "Multimenu 1 Checkbutton 1",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    {
      name:      "multimenu_1_checkbutton 2",
      long_name: "Multimenu 1 Checkbutton 2",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of Parameters */ }
  };

static bg_parameter_info_t multimenu_2_info[] =
  {
    {
      name:      "multimenu_2_checkbutton 1",
      long_name: "Multimenu 2 Checkbutton 1",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    {
      name:      "multimenu_2_checkbutton 2",
      long_name: "Multimenu 2 Checkbutton 2",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of Parameters */ }
  };

static bg_parameter_info_t multilist_1_info[] =
  {
    {
      name:      "multilist_1_checkbutton 1",
      long_name: "Multilist 1 Checkbutton 1",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    {
      name:      "multilist_1_checkbutton 2",
      long_name: "Multilist 1 Checkbutton 2",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of Parameters */ }
  };

static bg_parameter_info_t multilist_2_info[] =
  {
    {
      name:      "multilist_2_checkbutton 1",
      long_name: "Multilist 2 Checkbutton 1",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    {
      name:      "multilist_2_checkbutton 2",
      long_name: "Multilist 2 Checkbutton 2",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of Parameters */ }
  };

bg_parameter_info_t * multilist_parameters[] =
  {
    multilist_1_info,
    multilist_2_info,
    (bg_parameter_info_t *)0
  };

bg_parameter_info_t * multimenu_parameters[] =
  {
    multimenu_1_info,
    multimenu_2_info,
    (bg_parameter_info_t *)0
  };

static bg_parameter_info_t info[] =
  {
    {
      name:      "section_1",
      long_name: "Section 1",
      type:      BG_PARAMETER_SECTION
    },
    {
      name:        "checkbutton1",
      long_name:   "Check Button 1",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
      help_string: "Check Button 1 help"
    },
    {
      name:      "spinbutton_float",
      long_name: "Floating point Spinbutton",
      type:      BG_PARAMETER_FLOAT,
      val_default: { val_f: 250.0 },
      val_min:     { val_f: 200.0 },
      val_max:     { val_f: 300.0 },
      num_digits: 3,
      help_string: "Floating point Spinbutton help",
    },
    {
      name:      "spinbutton_int",
      long_name: "Integer Spinbutton",
      type:      BG_PARAMETER_INT,
      val_default: { val_i: 250 },
      val_min:     { val_i: 200 },
      val_max:     { val_i: 300 },
      help_string: "Integer Spinbutton help",
    },
    {
      name:      "time",
      long_name: "Time",
      type:      BG_PARAMETER_TIME,
      help_string: "Time help",
    },
    {
      name:        "slider_float",
      long_name:   "Floating point Slider",
      type:        BG_PARAMETER_SLIDER_FLOAT,
      val_default: { val_f: 250.0 },
      val_min:     { val_f: 1.0 },
      val_max:     { val_f: 300.0 },
      num_digits:  1,
      help_string: "Floating point Slider help",
    },
    {
      name:        "slider_int",
      long_name:   "Integer Slider",
      type:        BG_PARAMETER_SLIDER_INT,
      val_default: { val_i: 250 },
      val_min:     { val_i: 200 },
      val_max:     { val_i: 300 },
      help_string:   "Integer Slider help",
    },
    {
      name:      "string",
      long_name: "String",
      type:      BG_PARAMETER_STRING,
      val_default: { val_str: "Some string" },
      help_string:   "String help",
    },
    {
      name:      "string_hidden",
      long_name: "String (hidden)",
      type:      BG_PARAMETER_STRING_HIDDEN,
      val_default: { val_str: "Some string (hidden)" },
      help_string:   "String hidden help",
    },
    {
      name:        "stringlist",
      long_name:   "Stringlist",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Option 2" },
      multi_names:     (char *[]){ "Option 1", "Option 2", "Option 3", NULL },
      help_string:   "Stringlist help",
    },
    {
      name:      "section_2",
      long_name: "Section 2",
      type:      BG_PARAMETER_SECTION
    },
    {
      name:      "color_rgb",
      long_name: "Color RGB",
      type:      BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 0.0, 1.0, 0.0 } },
      help_string:   "Color RGB help",
    },
    {
      name:      "color_rgba",
      long_name: "Color RGBA",
      type:      BG_PARAMETER_COLOR_RGBA,
      val_default: { val_color: (float[]){ 0.0, 1.0, 0.0, 0.5 } },
      help_string:   "Color RGBA help",
    },
    {
      name:        "file",
      long_name:   "File",
      type:        BG_PARAMETER_FILE,
      val_default: { val_str: "/usr/include/stdio.h" },
      help_string:   "File help",
    },
    {
      name:        "directory",
      long_name:   "Directory",
      type:        BG_PARAMETER_DIRECTORY,
      val_default: { val_str: "/usr/local" },
      help_string:   "Directory help",
    },
    {
      name:      "font",
      long_name: "Font",
      type:      BG_PARAMETER_FONT,
      val_default: { val_str: "rudelsberg 12" },
      help_string:   "Font help",
    },
    {
      name:      "device",
      long_name: "Device",
      type:      BG_PARAMETER_DEVICE,
      val_default: { val_str: "/dev/cdrom" },
      help_string:   "Device help",
    },
    {
      name:      "section_3",
      long_name: "Section 3",
      type:      BG_PARAMETER_SECTION
    },
    {
      name:               "multimenu",
      long_name:          "Multimenu",
      type:               BG_PARAMETER_MULTI_MENU,
      val_default:        { val_str: "Multimenu 1" },
      multi_names:        (char *[]){ "multimenu_1", "multimenu_2", NULL },
      multi_labels:   (char *[]){ "Multimenu 1", "Multimenu 2", NULL },
      multi_descriptions: (char *[]){ "Multimenu 1", "Multimenu 2", NULL },
      multi_parameters:   multimenu_parameters,
      help_string:   "Multimenu help",
    },
#if 1
    {
      name:               "multilist",
      long_name:          "Multilist",
      type:               BG_PARAMETER_MULTI_LIST,
      multi_names:        (char *[]){ "multilist_1", "multilist_2", NULL },
      multi_labels:   (char *[]){ "Multilist 1", "Multilist 2", NULL },
      multi_descriptions: (char *[]){ "Multilist 1", "Multilist 2", NULL },
      multi_parameters:   multimenu_parameters,
      help_string:   "Multilist help",
    },
#endif
    { /* End of parameters */ }
  };

static void set_param(void * data, char * name,
                      bg_parameter_value_t * v)
  {
  bg_parameter_info_t * tmp_info;
  int i;
  char time_buf[GAVL_TIME_STRING_LEN];

  i = 0;
  tmp_info = (bg_parameter_info_t *)0;

  if(!name)
    return;
  
  while(1)
    {
    if(!info[i].name)
      break;
    
    if(!strcmp(info[i].name, name))
      {
      tmp_info = &(info[i]);
      break;
      }
    i++;
    }
  
  if(!tmp_info)
    {
    fprintf(stderr, "No valid parameter\n");
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

int main(int argc, char ** argv)
  {
  bg_dialog_t * test_dialog;
  bg_cfg_registry_t * registry;
  bg_cfg_section_t  * section;
    
  bg_gtk_init(&argc, &argv, (char*)0);

  registry = bg_cfg_registry_create();
  bg_cfg_registry_load(registry, "config.xml");

  section = bg_cfg_registry_find_section(registry, "section_1");
  
  test_dialog = bg_dialog_create(section, set_param, NULL, info, "Test dialog");

  bg_dialog_show(test_dialog);
  bg_cfg_registry_save(registry, "config.xml");

  bg_cfg_registry_destroy(registry);
  bg_dialog_destroy(test_dialog);
  
  return 0;
  }
                      
