#include <gavl.h>
#include <stdio.h>

int main(int argc, char ** argv)
  {
  int i, j, num;
  gavl_pixelformat_t src, dst;
  num = gavl_num_pixelformats();

  for(i = 0; i < num; i++)
    {
    src = gavl_get_pixelformat(i);
    for(j = 0; j < num; j++)
      {
      dst = gavl_get_pixelformat(j);
      fprintf(stderr, "src: %23s, dst: %23s, penalty: %6d\n",
              gavl_pixelformat_to_string(src),
              gavl_pixelformat_to_string(dst),
              gavl_pixelformat_conversion_penalty(src, dst));
      }
    }
  }
