/*
 *  Compile this file with
 *  gcc -g `pkg-config --cflags schroedinger-1.0` schro_settings.c `pkg-config --libs schroedinger-1.0`
 */

#include <schroedinger/schro.h>
#include <stdio.h>
#include <string.h>

static char humanize_buf[1024];

static char * humanize(const char * str)
  {
  char * pos;
  strcpy(humanize_buf, "TRS(\"");
  strcat(humanize_buf, str);
  strcat(humanize_buf, "\")");

  pos = &humanize_buf[5];
  *pos = toupper(*pos);

  while(*pos != '"')
    {
    if(*pos == '_')
      *pos = ' ';
    pos++;
    }
  return humanize_buf;
  }

int main(int argc, char ** argv)
  {
  int i, j, num;
  const SchroEncoderSetting * s;
  
  num = schro_encoder_get_n_settings();

  for(i = 0; i < num; i++)
    {
    s = schro_encoder_get_setting_info(i);
    
    printf("    {\n");
    printf("    .name = \"schro_%s\",\n", s->name);
    printf("    .long_name = %s,\n", humanize(s->name));
    switch(s->type)
      {
      case SCHRO_ENCODER_SETTING_TYPE_BOOLEAN:
        printf("    .type = BG_PARAMETER_CHECKBUTTON,\n");
        printf("    .val_default = { .val_i = %d },\n", (int)(s->default_value));
        break;
      case SCHRO_ENCODER_SETTING_TYPE_INT:
        printf("    .type = BG_PARAMETER_INT,\n");
        printf("    .val_min = { .val_i = %d },\n", (int)(s->min));
        printf("    .val_max = { .val_i = %d },\n", (int)(s->max));
        printf("    .val_default = { .val_i = %d },\n", (int)(s->default_value));
        break;
      case SCHRO_ENCODER_SETTING_TYPE_ENUM:
        printf("    .type = BG_PARAMETER_STRINGLIST,\n");
        j = 0;
        printf("    .multi_names = (const char*[])\n    {\n");
        while(j <= (int)s->max)
          {
          printf("      \"%s\",\n", s->enum_list[j]);
          j++;
          }
        printf("      NULL\n");
        printf("      },\n");

        printf("    .multi_labels = (const char*[])\n    {\n");

        j = 0;
        while(j <= (int)s->max)
          {
          printf("      %s,\n", humanize(s->enum_list[j]));
          j++;
          }
        printf("      NULL\n");
        printf("      },\n");


        printf("    .val_default = { .val_str = \"%s\" },\n", s->enum_list[(int)(s->default_value)]);
        
        break;
      case SCHRO_ENCODER_SETTING_TYPE_DOUBLE:
        if(s->max > s->min)
          printf("    .type = BG_PARAMETER_SLIDER_FLOAT,\n");
        else
          printf("    .type = BG_PARAMETER_FLOAT,\n");
        printf("    .val_min = { .val_f = %f },\n", s->min);
        printf("    .val_max = { .val_f = %f },\n", s->max);
        printf("    .val_default = { .val_f = %f },\n", s->default_value);
        break;
      default:
        break;
      }
    printf("    },\n");
    }
  }
