#include <stdio.h>
#include <gtk/gtk.h>
#include <gui_gtk/auth.h>
#include <gui_gtk/gtkutils.h>

int main(int argc, char ** argv)
  {
  char * user = (char*)0;
  char * pass = (char*)0;
  int save_auth = 0;
  bg_gtk_init(&argc, &argv);

  if(bg_gtk_get_userpass("Area 51",
                         &user, &pass,
                         &save_auth,
                         NULL))
    {
    fprintf(stderr, "User: %s\n", user);
    fprintf(stderr, "Pass: %s\n", pass);
    fprintf(stderr, "Save: %d\n", save_auth);
    }
  else
    fprintf(stderr, "Cancel\n");
   
  
  return 0;
  }
