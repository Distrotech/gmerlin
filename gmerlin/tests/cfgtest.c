#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>

#include <cfg_dialog.h>
#include <gui_gtk/gtkutils.h>


static bg_parameter_info_t encoder_1_info[] =
  {
    {
      name:      "encoder_1_checkbutton 1",
      long_name: "Encoder 1 Checkbutton 1",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    {
      name:      "encoder_1_checkbutton 2",
      long_name: "Encoder 1 Checkbutton 2",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of Parameters */ }
  };

static bg_parameter_info_t encoder_2_info[] =
  {
    {
      name:      "encoder_2_checkbutton 1",
      long_name: "Encoder 2 Checkbutton 1",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    {
      name:      "encoder_2_checkbutton 2",
      long_name: "Encoder 2 Checkbutton 2",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of Parameters */ }
  };

static bg_parameter_info_t decoder_1_info[] =
  {
    {
      name:      "decoder_1_checkbutton 1",
      long_name: "Decoder 1 Checkbutton 1",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    {
      name:      "decoder_1_checkbutton 2",
      long_name: "Decoder 1 Checkbutton 2",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of Parameters */ }
  };

static bg_parameter_info_t decoder_2_info[] =
  {
    {
      name:      "decoder_1_checkbutton 1",
      long_name: "Decoder 1 Checkbutton 1",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    {
      name:      "decoder_1_checkbutton 2",
      long_name: "Decoder 1 Checkbutton 2",
      type:      BG_PARAMETER_CHECKBUTTON,
    },
    { /* End of Parameters */ }
  };

bg_parameter_info_t * decoder_parameters[] =
  {
    decoder_1_info,
    decoder_2_info,
    (bg_parameter_info_t *)0
  };

bg_parameter_info_t * encoder_parameters[] =
  {
    encoder_1_info,
    encoder_2_info,
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
    },
    {
      name:      "spinbutton_float",
      long_name: "Floating point Spinbutton",
      type:      BG_PARAMETER_FLOAT,
      val_default: { val_f: 250.0 },
      val_min:     { val_f: 200.0 },
      val_max:     { val_f: 300.0 },
      num_digits: 3
    },
    {
      name:      "spinbutton_int",
      long_name: "Integer Spinbutton",
      type:      BG_PARAMETER_INT,
      val_default: { val_i: 250 },
      val_min:     { val_i: 200 },
      val_max:     { val_i: 300 },
    },
    {
      name:        "slider_float",
      long_name:   "Floating point Slider",
      type:        BG_PARAMETER_SLIDER_FLOAT,
      val_default: { val_f: 250.0 },
      val_min:     { val_f: 1.0 },
      val_max:     { val_f: 300.0 },
      num_digits:  1
    },
    {
      name:        "slider_int",
      long_name:   "Integer Slider",
      type:        BG_PARAMETER_SLIDER_INT,
      val_default: { val_i: 250 },
      val_min:     { val_i: 200 },
      val_max:     { val_i: 300 },
    },
    {
      name:      "string",
      long_name: "String",
      type:      BG_PARAMETER_STRING,
      val_default: { val_str: "Some string" },
    },
    {
      name:        "stringlist",
      long_name:   "Stringlist",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Option 2" },
      options:     (char *[]){ "Option 1", "Option 2", "Option 3", NULL }
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
    },
    {
      name:      "color_rgba",
      long_name: "Color RGBA",
      type:      BG_PARAMETER_COLOR_RGBA,
      val_default: { val_color: (float[]){ 0.0, 1.0, 0.0, 0.5 } }
    },
    {
      name:        "file",
      long_name:   "File: ",
      type:        BG_PARAMETER_FILE,
      val_default: { val_str: "/usr/include/stdio.h" }
    },
    {
      name:        "directory",
      long_name:   "Directory: ",
      type:        BG_PARAMETER_DIRECTORY,
      val_default: { val_str: "/usr/local" }
    },
    {
      name:      "font",
      long_name: "Font: ",
      type:      BG_PARAMETER_FONT,
      val_default: { val_str: "rudelsberg 12" }
    },
    {
      name:      "device",
      long_name: "Device: ",
      type:      BG_PARAMETER_DEVICE,
      val_default: { val_str: "/dev/cdrom" }
    },
    {
      name:      "section_3",
      long_name: "Section 3",
      type:      BG_PARAMETER_SECTION
    },
    {
      name:               "encoder",
      long_name:          "Encoder",
      type:               BG_PARAMETER_MULTI_MENU,
      val_default:        { val_str: "Encoder 1" },
      multi_names:        (char *[]){ "encoder_1", "encoder_2", NULL },
      multi_labels:   (char *[]){ "Encoder 1", "Encoder 2", NULL },
      multi_descriptions: (char *[]){ "Encoder 1", "Encoder 2", NULL },
      multi_parameters:   encoder_parameters,
    },
#if 0
    {
      name:               "decoder",
      long_name:          "Decoder",
      type:               BG_PARAMETER_ENCODER,
      codec_names:        (char *[]){ "decoder_1", "decoder_2", NULL },
      codec_long_names:   (char *[]){ "Decoder 1", "Decoder 2", NULL },
      codec_descriptions: (char *[]){ "Decoder 1", "Decoder 2", NULL },
      codec_parameters:   encoder_parameters,
    },
#endif
    { /* End of parameters */ }
  };

static void set_param(void * data, char * name,
                      bg_parameter_value_t * v)
  {
  bg_parameter_info_t * tmp_info;
  int i;

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
    case BG_PARAMETER_STRINGLIST:
    case BG_PARAMETER_FONT:
    case BG_PARAMETER_DEVICE:
    case BG_PARAMETER_MULTI_MENU:
      fprintf(stderr, "String %s: %s\n", tmp_info->name,
              v->val_str);
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
    
  bg_gtk_init(&argc, &argv);

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
                      
