#include <avdec_private.h>
#include <yml.h>

int main(int argc, char ** argv)
  {
  bgav_yml_node_t * n;
  bgav_input_context_t * input;

  input = bgav_input_open(argv[1], 5000);
    
  n = bgav_yml_parse(input);
  if(!n)
    {
    fprintf(stderr, "Could not read yml file %s\n",
            argv[1]);
    }
  bgav_yml_dump(n);
  bgav_yml_free(n);
  
  bgav_input_close(input);
  return 0;
  }
