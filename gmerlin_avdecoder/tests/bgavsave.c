#include <stdlib.h>
#include <avdec_private.h>

#define BUFFER_SIZE 4096

int main(int argc, char ** argv)
  {
  FILE * out;
  uint8_t buffer[BUFFER_SIZE];
  int bytes_read;
  bgav_input_context_t * input;

  input = calloc(1, sizeof(*input));
  
  if(!bgav_input_open(input, argv[1]))
    return -1;
  
  out = fopen(argv[2], "w");
  do
    {
    bytes_read = bgav_input_read_data(input, buffer, BUFFER_SIZE);
    fwrite(buffer, 1, bytes_read, out);
    fprintf(stderr, "Wrote %d bytes\n", bytes_read);
    } while(bytes_read == BUFFER_SIZE);

  fclose(out);
  return 0;
  }
