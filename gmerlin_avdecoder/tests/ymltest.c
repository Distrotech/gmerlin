#include <avdec_private.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char ** argv)
  {
  bgav_yml_node_t * n;
  bgav_input_context_t * input;
  bgav_options_t opt;
  bgav_options_set_defaults(&opt);

  input = bgav_input_create(&opt);
  
  if(!bgav_input_open(input, argv[1]))
    {
    fprintf(stderr, "Cannot open file %s\n",
            argv[1]);
    return -1;
    }
  
  
  n = bgav_yml_parse(input);
  if(!n)
    {
    fprintf(stderr, "Cannot parse file %s\n",
            argv[1]);
    return -1;
    }
  bgav_yml_dump(n);
  bgav_yml_free(n);
  
  bgav_input_close(input);
  
  return 0;
  }
