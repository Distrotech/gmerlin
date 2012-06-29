/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <locale.h>

#include <stdlib.h>
#include <string.h>

#include <inttypes.h>
#include <limits.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <ctype.h>

#include <config.h>


#include <gui_gtk/gtkutils.h>

#include <fontconfig/fontconfig.h>

#include <pango/pangofc-fontmap.h>

#include <gmerlin/utils.h>

GdkPixbuf * bg_gtk_pixbuf_scale_alpha(GdkPixbuf * src,
                                      int dest_width,
                                      int dest_height,
                                      float * foreground,
                                      float * background)
  {
  int i, j;
  GdkPixbuf * ret;
  int rowstride;
  uint8_t * data;
  uint8_t * ptr;

  int background_i[3];
  int foreground_i[3];

  for(i = 0; i < 3; i++)
    {
    background_i[i] = (int)(255.0 * background[i]);
    foreground_i[i] = (int)(255.0 * foreground[i]);
    }
  ret = gdk_pixbuf_scale_simple(src,
                                dest_width,
                                dest_height,
                                GDK_INTERP_BILINEAR);


  
  rowstride = gdk_pixbuf_get_rowstride(ret);

  data      = gdk_pixbuf_get_pixels(ret);

  for(i = 0; i < dest_height; i++)
    {
    ptr = data;
    for(j = 0; j < dest_width; j++)
      {
      ptr[0] =
        (ptr[3] * foreground_i[0] + (0xFF - ptr[3]) * background_i[0]) >> 8;
      ptr[1] =
        (ptr[3] * foreground_i[1] + (0xFF - ptr[3]) * background_i[1]) >> 8;
      ptr[2] =
        (ptr[3] * foreground_i[2] + (0xFF - ptr[3]) * background_i[2]) >> 8;
      ptr[3] = 0xff;
      ptr += 4;
      }
    data += rowstride;
    }
  
  return ret;
  }

static GdkPixbuf * window_pixbuf = NULL;

static char * default_window_name = NULL;
static char * default_window_class = NULL;

static void set_default_window_icon(const char * icon)
  {
  char * tmp;
  tmp = bg_search_file_read("icons", icon);
  if(tmp)
    {
    if(window_pixbuf)
      g_object_unref(window_pixbuf);
    window_pixbuf = gdk_pixbuf_new_from_file(tmp, NULL);
    free(tmp);
    }
  }

GtkWidget * bg_gtk_window_new(GtkWindowType type)
  {
  GtkWidget * ret = gtk_window_new(type);
  if(window_pixbuf)
    gtk_window_set_icon(GTK_WINDOW(ret), window_pixbuf);
  if(default_window_name && default_window_class)
    {
    gtk_window_set_wmclass(GTK_WINDOW(ret), 
                           default_window_name, 
                           default_window_class);
    }
  return ret;
  }


void bg_gtk_init(int * argc, char *** argv, 
                 const char * default_window_icon,
                 const char * win_name, const char * win_class)
  {
  gtk_init(argc, argv);

  /* No, we don't like commas as decimal separators */
  setlocale(LC_NUMERIC, "C");

  /* Set the default window icon */
  set_default_window_icon(default_window_icon);

  /* Set default class hints */
  default_window_name = bg_strdup(default_window_name, win_name);
  default_window_class = bg_strdup(default_window_class, win_class);

  }

void bg_gdk_pixbuf_render_pixmap_and_mask(GdkPixbuf *pixbuf,
                                          GdkPixmap **pixmap_return,
                                          GdkBitmap **mask_return)
  {
  int width, height;
  char * mask_data;
    
  gdk_pixbuf_render_pixmap_and_mask(pixbuf,
                                    pixmap_return,
                                    mask_return,
                                    0x80);

  if(mask_return && !(*mask_return))
    {
    width = gdk_pixbuf_get_width(pixbuf);
    height = gdk_pixbuf_get_height(pixbuf);
    
    mask_data = malloc(width * height);
    memset(mask_data, 0xff, width * height);
    
    *mask_return = gdk_bitmap_create_from_data(NULL,
                                               mask_data, width, height);

    free(mask_data);
    }
  }

void bg_gtk_set_widget_bg_pixmap(GtkWidget * widget, GdkPixmap *pixmap)
  {
  GtkStyle   *style;

  style = gtk_style_copy (widget->style);
  if (style->bg_pixmap[0])
    g_object_unref (style->bg_pixmap[0]);
  style->bg_pixmap[0] = g_object_ref (pixmap);
  gtk_widget_set_style (widget, style);
  g_object_unref (style);
  }

/* Font name conversion */

/* Really stupid, that pango font names are incompatible to
 * fontconfig names, so we must convert them ourselves
 */

/* Ripped from pango sourcecode */
static int
pango_fc_convert_weight_to_fc (PangoWeight pango_weight)
{
  if (pango_weight < (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_LIGHT) / 2)
    return FC_WEIGHT_LIGHT;
  else if (pango_weight < (PANGO_WEIGHT_NORMAL + 600) / 2)
    return FC_WEIGHT_MEDIUM;
  else if (pango_weight < (600 + PANGO_WEIGHT_BOLD) / 2)
    return FC_WEIGHT_DEMIBOLD;
  else if (pango_weight < (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2)
    return FC_WEIGHT_BOLD;
  else
    return FC_WEIGHT_BLACK;
}

static int
pango_fc_convert_slant_to_fc (PangoStyle pango_style)
{
  switch (pango_style)
    {
    case PANGO_STYLE_NORMAL:
      return FC_SLANT_ROMAN;
    case PANGO_STYLE_ITALIC:
      return FC_SLANT_ITALIC;
    case PANGO_STYLE_OBLIQUE:
      return FC_SLANT_OBLIQUE;
    default:
      return FC_SLANT_ROMAN;
    }
}

#ifdef FC_WIDTH
static int
pango_fc_convert_width_to_fc (PangoStretch pango_stretch)
{
  switch (pango_stretch)
    {
    case PANGO_STRETCH_NORMAL:
      return FC_WIDTH_NORMAL;
    case PANGO_STRETCH_ULTRA_CONDENSED:
      return FC_WIDTH_ULTRACONDENSED;
    case PANGO_STRETCH_EXTRA_CONDENSED:
      return FC_WIDTH_EXTRACONDENSED;
    case PANGO_STRETCH_CONDENSED:
      return FC_WIDTH_CONDENSED;
    case PANGO_STRETCH_SEMI_CONDENSED:
      return FC_WIDTH_SEMICONDENSED;
    case PANGO_STRETCH_SEMI_EXPANDED:
      return FC_WIDTH_SEMIEXPANDED;
    case PANGO_STRETCH_EXPANDED:
      return FC_WIDTH_EXPANDED;
    case PANGO_STRETCH_EXTRA_EXPANDED:
      return FC_WIDTH_EXTRAEXPANDED;
    case PANGO_STRETCH_ULTRA_EXPANDED:
      return FC_WIDTH_ULTRAEXPANDED;
    default:
      return FC_WIDTH_NORMAL;
    }
}
#endif


static FcPattern *
pango_fc_make_pattern (const  PangoFontDescription *description,
                       double size)
  {
  FcPattern *pattern;
  int slant;
  int weight;
  char **families;
  int i;
#ifdef FC_WIDTH
  int width;
#endif

  slant = pango_fc_convert_slant_to_fc (pango_font_description_get_style (description));
  weight = pango_fc_convert_weight_to_fc (pango_font_description_get_weight (description));
#ifdef FC_WIDTH
  width = pango_fc_convert_width_to_fc (pango_font_description_get_stretch (description));
#endif

  /* The reason for passing in FC_SIZE as well as FC_PIXEL_SIZE is
   * to work around a bug in libgnomeprint where it doesn't look
   * for FC_PIXEL_SIZE. See http://bugzilla.gnome.org/show_bug.cgi?id=169020
   *
   * Putting FC_SIZE in here slightly reduces the efficiency
   * of caching of patterns and fonts when working with multiple different
   * dpi values.
   */
  pattern = FcPatternBuild (NULL,
                            FC_WEIGHT, FcTypeInteger, weight,
                            FC_SLANT,  FcTypeInteger, slant,
#ifdef FC_WIDTH
                            FC_WIDTH,  FcTypeInteger, width,
#endif
                            FC_SIZE,  FcTypeDouble,  size,
                            NULL);
  
  families =
    g_strsplit (pango_font_description_get_family (description), ",", -1);
  
  for (i = 0; families[i]; i++)
    FcPatternAddString (pattern, FC_FAMILY, (const FcChar8*)families[i]);
  
  g_strfreev (families);
  
  return pattern;
  }

char * bg_gtk_convert_font_name_from_pango(const char * name)
  {
  char * ret;
  PangoFontDescription *description;
  FcPattern *pattern;
  double size = 12.0;

  const char * pos;

  pos = &name[strlen(name) - 1];
  while(!isspace(*pos))
    {
    if(pos == name)
      return NULL;
    pos--;
    }
  pos++;
  if(isdigit(*pos) || (*pos == '.'))
    size = strtod(pos, NULL);
  else
    size = 12.0;
  
  description = pango_font_description_from_string(name);
  
  pattern = pango_fc_make_pattern(description, size);

  ret = (char*)FcNameUnparse(pattern);

  FcPatternDestroy(pattern);
  pango_font_description_free(description);
  return ret;
  }

char * bg_gtk_convert_font_name_to_pango(const char * name)
  {
  char * ret, *tmp;
  PangoFontDescription *description;
  FcPattern *pattern;

  pattern = FcNameParse((const FcChar8 *)name);
  description = pango_fc_font_description_from_pattern(pattern, TRUE);

  tmp = pango_font_description_to_string(description);
  ret = bg_strdup(NULL, tmp);
  g_free(tmp);
  FcPatternDestroy(pattern);
  pango_font_description_free(description);

  
  return ret;
  
  }


static int show_tooltips = 1;



#if GTK_MINOR_VERSION < 12
static GtkTooltips * tooltips = NULL;

void bg_gtk_tooltips_set_tip(GtkWidget * w, const char * str,
                             const char * translation_domain)
  {
  str = dgettext(translation_domain, str);

  if(!tooltips)
    {
    tooltips = gtk_tooltips_new();
    
    g_object_ref (G_OBJECT (tooltips));
    
#if GTK_MINOR_VERSION < 10
    gtk_object_sink (GTK_OBJECT (tooltips));
#else
    g_object_ref_sink(G_OBJECT(tooltips));
#endif
    }

  if(show_tooltips)
    gtk_tooltips_enable(tooltips);
  else
    gtk_tooltips_disable(tooltips);

  gtk_tooltips_set_tip(tooltips, w, str, str);
  }

void bg_gtk_set_tooltips(int enable)
  {
  show_tooltips = enable;
  if(tooltips)
    {
    if(show_tooltips)
      gtk_tooltips_enable(tooltips);
    else
      gtk_tooltips_disable(tooltips);
    }
  }


#else

static GQuark tooltip_quark = 0;

static gboolean tooltip_callback(GtkWidget  *widget,
                                 gint        x,
                                 gint        y,
                                 gboolean    keyboard_mode,
                                 GtkTooltip *tooltip,
                                 gpointer    user_data)
  {
  char * str;
  if(show_tooltips)
    {
    str = g_object_get_qdata(G_OBJECT(widget), tooltip_quark);
    gtk_tooltip_set_text(tooltip, str);
    return TRUE;
    }
  else
    return FALSE;
  }


void bg_gtk_tooltips_set_tip(GtkWidget * w, const char * str,
                             const char * translation_domain)
  {
  GValue val = { 0 };
  
  str = TR_DOM(str);
  //  gtk_widget_set_tooltip_text(w, str);

  if(!tooltip_quark)
    tooltip_quark = g_quark_from_string("gmerlin-tooltip");
  
  g_object_set_qdata_full(G_OBJECT(w), tooltip_quark, g_strdup(str), g_free);

  g_value_init(&val, G_TYPE_BOOLEAN);
  g_value_set_boolean(&val, 1);

  g_object_set_property(G_OBJECT(w), "has-tooltip", &val);
  g_signal_connect(G_OBJECT(w), "query-tooltip",
                   G_CALLBACK(tooltip_callback),
                   NULL);
  }

void bg_gtk_set_tooltips(int enable)
  {
  show_tooltips = enable;
  }

#endif



int bg_gtk_get_tooltips()
  {
  return show_tooltips;
  }

GtkWidget * bg_gtk_get_toplevel(GtkWidget * w)
  {
  GtkWidget * toplevel;

  if(!w)
    return NULL;
  
  toplevel = gtk_widget_get_toplevel(w);
  if(!bg_gtk_widget_is_toplevel(toplevel))
    toplevel = NULL;
  return toplevel;
  }

static void pixbuf_destroy_notify(guchar *pixels,
                           gpointer data)
  {
  gavl_video_frame_destroy(data);
  }


GdkPixbuf * bg_gtk_pixbuf_from_frame(gavl_video_format_t * format,
                                     gavl_video_frame_t * frame)
  {
  if(format->pixelformat == GAVL_RGB_24)
    {
    return gdk_pixbuf_new_from_data(frame->planes[0],
                                    GDK_COLORSPACE_RGB,
                                    FALSE,
                                    8,
                                    format->image_width,
                                    format->image_height,
                                    frame->strides[0],
                                    pixbuf_destroy_notify,
                                    frame);
    }
  else if(format->pixelformat == GAVL_RGBA_32)
    {
    return gdk_pixbuf_new_from_data(frame->planes[0],
                                    GDK_COLORSPACE_RGB,
                                    TRUE,
                                    8,
                                    format->image_width,
                                    format->image_height,
                                    frame->strides[0],
                                    pixbuf_destroy_notify,
                                    frame);
    
    }
  else
    return NULL;
  }

int bg_gtk_widget_is_realized(GtkWidget * w)
  {
#if GTK_CHECK_VERSION(2,20,0)
  return gtk_widget_get_realized(w);
#else
  return GTK_WIDGET_REALIZED(w);
#endif
  }

int bg_gtk_widget_is_toplevel(GtkWidget * w)
  {
#if GTK_CHECK_VERSION(2,18,0)
  return gtk_widget_is_toplevel(w);
#else
  return GTK_WIDGET_TOPLEVEL(w);
#endif
  }

void bg_gtk_widget_set_can_default(GtkWidget *w, gboolean can_default)
  {
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_can_default(w, can_default);
#else
  if(can_default)
    GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
  else
    GTK_WIDGET_UNSET_FLAGS(w, GTK_CAN_DEFAULT);
#endif
  }

void bg_gtk_widget_set_can_focus(GtkWidget *w, gboolean can_focus)
  {
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_can_focus(w, can_focus);
#else
  if(can_focus)
    GTK_WIDGET_SET_FLAGS(w, GTK_CAN_FOCUS);
  else
    GTK_WIDGET_UNSET_FLAGS(w, GTK_CAN_FOCUS);
#endif
  }


GtkWidget * bg_gtk_combo_box_new_text()
  {
#if GTK_CHECK_VERSION(2,24,0)
  return gtk_combo_box_text_new();
#else
  return gtk_combo_box_new_text();
#endif
  }

void bg_gtk_combo_box_append_text(GtkWidget *combo_box, const gchar *text)
  {
#if GTK_CHECK_VERSION(2,24,0)
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), text);
#else
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box), text);
#endif
  }

void bg_gtk_combo_box_remove_text(GtkWidget * b, int index)
  {
#if GTK_CHECK_VERSION(2,24,0)
  int i;
  GtkTreeIter it;
  GtkTreeModel * model;
  model = gtk_combo_box_get_model(GTK_COMBO_BOX(b));

  if(!gtk_tree_model_get_iter_first(model, &it))
    return;
  for(i = 0; i < index; i++)
    {
    if(!gtk_tree_model_iter_next(model, &it))
      return;
    }
  gtk_list_store_remove(GTK_LIST_STORE(model), &it);
#else
  gtk_combo_box_remove_text(GTK_COMBO_BOX(b), index);
#endif
  }
