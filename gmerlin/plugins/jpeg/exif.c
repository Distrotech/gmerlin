/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <config.h>

#include <gavl/metadata.h>
#include <gmerlin/utils.h>


#include <stdio.h>
#include <libexif/exif-byte-order.h>
#include <libexif/exif-data-type.h>
#include <libexif/exif-ifd.h>
#include <libexif/exif-log.h>
#include <libexif/exif-tag.h>
#include <libexif/exif-content.h>
#include <libexif/exif-mnote-data.h>
#include <libexif/exif-mem.h>

#include "exif.h"


static void foreach2(ExifEntry * e, void * priv)
  {
  ExifIfd ifd = exif_content_get_ifd(e->parent);
  fprintf(stderr, "Got exif tag\n");
  fprintf(stderr, "  Name:        %s\n",
          exif_tag_get_name_in_ifd(e->tag, ifd));
  fprintf(stderr, "  Title:       %s\n",
          exif_tag_get_title_in_ifd(e->tag, ifd));
  fprintf(stderr, "  Description: %s\n",
          exif_tag_get_description_in_ifd(e->tag, ifd));
  fprintf(stderr, "  Format:      %s\n",
          exif_format_get_name(e->format));
  fprintf(stderr, "  Size:        %d\n",
          e->size);
  bg_hexdump(e->data, e->size, 16);
  }

static void foreach1(ExifContent * c, void * priv)
  {
  exif_content_foreach_entry(c, foreach2, priv);
  }

void bg_exif_get_metadata(const char * filename,
                          gavl_metadata_t * ret)
  {
  ExifData * d;
  
  d = exif_data_new_from_file(filename);
  if(!d)
    return;
  exif_data_foreach_content(d, foreach1, ret);
  exif_data_unref(d);
  }
