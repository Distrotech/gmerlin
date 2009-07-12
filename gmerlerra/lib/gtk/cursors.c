#include <gtk/gtk.h>
#include <gui_gtk/utils.h>

GdkCursor * bg_nle_cursor_doublearrow = NULL;
GdkCursor * bg_nle_cursor_xterm       = NULL;
GdkCursor * bg_nle_cursor_left_side   = NULL;
GdkCursor * bg_nle_cursor_right_side  = NULL;

void bg_nle_init_cursors()
  {
  bg_nle_cursor_doublearrow = gdk_cursor_new(GDK_SB_H_DOUBLE_ARROW);
  bg_nle_cursor_xterm       = gdk_cursor_new(GDK_XTERM);
  bg_nle_cursor_left_side   = gdk_cursor_new(GDK_LEFT_SIDE);
  bg_nle_cursor_right_side   = gdk_cursor_new(GDK_RIGHT_SIDE);
  }
