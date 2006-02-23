/*****************************************************************
 
  gtkutils.c
 
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

#include <locale.h>

#include <stdlib.h>
#include <string.h>

#include <inttypes.h>
#include <gtk/gtk.h>
#include <stdio.h>

#include <gui_gtk/gtkutils.h>

#include <fontconfig/fontconfig.h>

#include <pango/pangofc-fontmap.h>

#include <utils.h>

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

void bg_gtk_init(int * argc, char *** argv)
  {
  //  gtk_disable_setlocale();
  gtk_init(argc, argv);

  /* No, we don't like commas as decimal separators */
  
  setlocale(LC_NUMERIC, "C");
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
    
    *mask_return = gdk_bitmap_create_from_data((GdkDrawable*)0,
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
    FcPatternAddString (pattern, FC_FAMILY, families[i]);
  
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
      return (char*)0;
    pos--;
    }
  pos++;
  if(isdigit(*pos) || (*pos == '.'))
    size = strtod(pos, (char**)0);
  else
    size = 12.0;
  
  description = pango_font_description_from_string(name);
  
  pattern = pango_fc_make_pattern(description, size);

  ret = FcNameUnparse(pattern);

  FcPatternDestroy(pattern);
  pango_font_description_free(description);
  fprintf(stderr, "bg_gtk_convert_font_name_from_pango: %s -> %s\n",
          name, ret);
  return ret;
  }

char * bg_gtk_convert_font_name_to_pango(const char * name)
  {
  char * ret, *tmp;
  PangoFontDescription *description;
  FcPattern *pattern;

  pattern = FcNameParse(name);
  description = pango_fc_font_description_from_pattern(pattern, TRUE);

  tmp = pango_font_description_to_string(description);
  ret = bg_strdup((char*)0, tmp);
  g_free(tmp);
  FcPatternDestroy(pattern);
  pango_font_description_free(description);

  
  fprintf(stderr, "bg_gtk_convert_font_name_to_pango: %s -> %s\n", name, ret);
  return ret;
  
  }
