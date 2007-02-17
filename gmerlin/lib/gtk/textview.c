/*****************************************************************
 
  textview.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gui_gtk/textview.h>

/* Font stuff */


static GtkTextTag      * text_tag  = (GtkTextTag *)0;
static GtkTextTagTable * tag_table = (GtkTextTagTable *)0;

struct bg_gtk_textview_s
  {
  GtkWidget * textview;
  GtkTextBuffer * buffer;
  };

static void set_bg(GtkWidget * widget, gpointer data)
  {
  GtkRcStyle *rc_style;

  rc_style = gtk_rc_style_new ();
  rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BASE;
#if 1
  rc_style->base[GTK_STATE_NORMAL].red =   widget->style->bg[GTK_STATE_NORMAL].red;
  rc_style->base[GTK_STATE_NORMAL].green = widget->style->bg[GTK_STATE_NORMAL].green;
  rc_style->base[GTK_STATE_NORMAL].blue =  widget->style->bg[GTK_STATE_NORMAL].blue;
  rc_style->base[GTK_STATE_NORMAL].pixel = widget->style->bg[GTK_STATE_NORMAL].pixel;
#endif
  gtk_widget_modify_style(widget, rc_style);
  gtk_rc_style_unref (rc_style);
  }

bg_gtk_textview_t * bg_gtk_textview_create()
  {
  bg_gtk_textview_t * t;
  
  t = calloc(1, sizeof(*t));
  
  if(!tag_table)
    {
    tag_table = gtk_text_tag_table_new();
    
    text_tag = gtk_text_tag_new("Font");
    g_object_set(text_tag, "editable", 0, NULL);
    
    gtk_text_tag_table_add(tag_table,
                           text_tag);
    }
  
  t->buffer = gtk_text_buffer_new(tag_table);
  t->textview = gtk_text_view_new_with_buffer(t->buffer);

  g_signal_connect(G_OBJECT(t->textview), "realize",
                   G_CALLBACK(set_bg), NULL);
  
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(t->textview), FALSE);
  
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(t->textview), GTK_WRAP_NONE);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(t->textview), 0);
  
  gtk_widget_show(t->textview);
  return t;
  }

void bg_gtk_textview_destroy(bg_gtk_textview_t * t)
  {
  g_object_unref(t->buffer);
  free(t);
  }

void bg_gtk_textview_update(bg_gtk_textview_t * t,
                            const char * text)
  {
  const char * next_tab;
  const char * pos;
  const char * end_pos;
  int line;
  int tab_pos;
  int line_width;
  int i;
  PangoTabArray * tab_array;
  GtkTextIter start_iter;
  GtkTextIter end_iter;

  GdkRectangle start_rect;
  GdkRectangle end_rect;
    
  pos = text;
  
  next_tab = strchr(pos, '\t');

  /* No tabs here, just copy the text */
  if(!next_tab)
    {
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(t->textview), GTK_WRAP_WORD);
    
    gtk_text_buffer_set_text(t->buffer, text, -1);
    
    }
  else /* This is the complicated version */
    {
    line    = 0;
    tab_pos = 0;

    
    while(1)
      {
      next_tab = strchr(pos, '\t');
      end_pos  = strchr(pos, '\n');
      if(!end_pos)
        end_pos = pos + strlen(pos);
      if(next_tab > end_pos)
        next_tab = NULL;
      
      /* Insert everything before the tab and calculate width */

      gtk_text_buffer_get_end_iter(t->buffer,
                                   &end_iter);
      
      if(!next_tab)
        {
        gtk_text_buffer_insert(t->buffer, &end_iter, pos, (int)(end_pos - pos));
        }
      else
        {
        gtk_text_buffer_insert(t->buffer, &end_iter, pos, (int)(next_tab - pos));
        }
      
      gtk_text_buffer_get_bounds(t->buffer, &start_iter, &end_iter);
      for(i = 0; i < line; i++)
        {
        gtk_text_view_forward_display_line(GTK_TEXT_VIEW(t->textview),
                                           &start_iter);
        }

      gtk_text_view_get_iter_location(GTK_TEXT_VIEW(t->textview),
                                      &start_iter, &start_rect);
      gtk_text_view_get_iter_location(GTK_TEXT_VIEW(t->textview),
                                      &end_iter, &end_rect);
      line_width = end_rect.x + end_rect.width;

      if(tab_pos < line_width)
        tab_pos = line_width;
      
      /* Insert everything after the tab */

      if(next_tab)
        {
        gtk_text_buffer_get_end_iter(t->buffer, &end_iter);
        gtk_text_buffer_insert(t->buffer, &end_iter, next_tab, (int)(end_pos - next_tab));
        }
      pos = end_pos;
      line++;
      if(*pos == '\0')
        break;
      else
        {
        while(*pos == '\n')
          {
          gtk_text_buffer_get_end_iter(t->buffer, &end_iter);
          gtk_text_buffer_insert(t->buffer, &end_iter, pos, 1);
          pos++;
          }
        }
      }
    /* Set the tab positions */

    tab_array =
      pango_tab_array_new_with_positions(1, /* gint size, */
                                         1, /* gboolean positions_in_pixels, */
                                         PANGO_TAB_LEFT, /* PangoTabAlign first_alignment, */
                                         tab_pos+10 /* gint first_position, */ );
    gtk_text_view_set_tabs(GTK_TEXT_VIEW(t->textview), tab_array);
    pango_tab_array_free(tab_array);
    
    }
  
  gtk_text_buffer_get_bounds(t->buffer,
                             &start_iter,
                             &end_iter);
  gtk_text_buffer_apply_tag(t->buffer,
                            text_tag,
                            &start_iter,
                            &end_iter);
  };

GtkWidget * bg_gtk_textview_get_widget(bg_gtk_textview_t * t)
  {
  return t->textview;
  }
