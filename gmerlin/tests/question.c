#include <stdio.h>
#include <gtk/gtk.h>
#include <gui_gtk/question.h>
#include <gui_gtk/gtkutils.h>

int main(int argc, char ** argv)
  {
  int ans;
  bg_gtk_init(&argc, &argv, (char*)0);
  
  ans = bg_gtk_question("Switch coffemachine to backwards\rOr not?");

  fprintf(stderr, "Got answer: %s\n", (ans)? "yes" : "no");
  
  return 0;
  }
