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
#include <gavl/metatags.h>
#include <gmerlin/utils.h>
#include <gmerlin/charset.h>

#include <stdlib.h>


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

static uint32_t get_long(uint8_t * data)
  {
  uint32_t ret;
  ret = data[3];
  ret <<= 8;
  ret |= data[2];
  ret <<= 8;
  ret |= data[1];
  ret <<= 8;
  ret |= data[0];
  return ret;
  }

static uint16_t get_short(uint8_t * data)
  {
  uint16_t ret;
  ret = data[1];
  ret <<= 8;
  ret |= data[0];
  return ret;
  }


typedef struct
  {
  gavl_metadata_t * m;
  bg_charset_converter_t * cnv;
  } foreach_data_t;

static void set_utf16le(foreach_data_t * fd,
                        ExifEntry * e,
                        const char * key)
  {
  if(e->format == EXIF_FORMAT_BYTE)
    gavl_metadata_set_nocpy(fd->m, key,
                            bg_convert_string(fd->cnv, (char*)e->data,
                                              e->size, NULL));
  }

static void set_ascii(foreach_data_t * fd,
                      ExifEntry * e,
                      const char * key)
  {
  char * end = (char*)e->data + e->size - 1;

  if(*end != '\0')
    end++;
  
  if(e->format == EXIF_FORMAT_ASCII)
    gavl_metadata_set_nocpy(fd->m, key,
                            bg_strndup(NULL, (char*)e->data, end));
  }
                              

static void set_date_time(foreach_data_t * fd,
                          ExifEntry * e,
                          const char * key)
  {
  int year, month, day, hour, minute, second;

  if(e->format != EXIF_FORMAT_ASCII)
    return;
  
  if(sscanf((char*)e->data, "%d:%d:%d %d:%d:%d",
            &year, &month, &day, &hour, &minute, &second) < 6)
    return;
  
  gavl_metadata_set_date_time(fd->m, key,
                              year, month, day,
                              hour, minute, second);
  }

static void set_rational(foreach_data_t * fd,
                         ExifEntry * e,
                         const char * key)
  {
  uint32_t num, den;
  if((e->format !=  EXIF_FORMAT_RATIONAL) ||
     (e->size != 8))
    return;

  num = get_long(e->data);
  den = get_long(e->data + 4);

  gavl_metadata_set_nocpy(fd->m, key,
                          bg_sprintf("%d/%d", num, den));
  }

static void set_long(foreach_data_t * fd,
                     ExifEntry * e,
                     const char * key)
  {
  uint32_t num;
  if((e->format !=  EXIF_FORMAT_LONG) ||
     (e->size != 4))
    return;

  num = get_long(e->data);
  
  gavl_metadata_set_int(fd->m, key, num);
  }

static void set_short(foreach_data_t * fd,
                      ExifEntry * e,
                      const char * key)
  {
  uint32_t num;
  if((e->format !=  EXIF_FORMAT_SHORT ) ||
     (e->size != 2))
    return;

  num = get_short(e->data);
  
  gavl_metadata_set_int(fd->m, key, num);
  }


static void foreach2(ExifEntry * e, void * priv)
  {
  foreach_data_t  * fd;
  ExifIfd ifd = exif_content_get_ifd(e->parent);

  fd = priv;

  
  switch(e->tag)
    {
    case EXIF_TAG_XP_AUTHOR:
      set_utf16le(fd, e, GAVL_META_AUTHOR);
      break;
    case EXIF_TAG_XP_TITLE:
      set_utf16le(fd, e, GAVL_META_TITLE);
      break;
    case EXIF_TAG_XP_COMMENT:
      set_utf16le(fd, e, GAVL_META_COMMENT);
      break;
    case EXIF_TAG_MAKE:
      set_ascii(fd, e, GAVL_META_VENDOR);
      break;
    case EXIF_TAG_MODEL:
      set_ascii(fd, e, GAVL_META_DEVICE);
      break;
    case EXIF_TAG_SOFTWARE:
      set_ascii(fd, e, GAVL_META_SOFTWARE);
      break;
    case EXIF_TAG_DATE_TIME_ORIGINAL:
      set_date_time(fd, e, GAVL_META_DATE_CREATE);
      break;
    case EXIF_TAG_DATE_TIME:
      set_date_time(fd, e, GAVL_META_DATE_MODIFY);
      break;
#if 0
    case EXIF_TAG_EXPOSURE_TIME:
      set_rational(fd, e, GAVL_META_EXPOSURE_TIME);
      break;
    case EXIF_TAG_FNUMBER:
      set_rational(fd, e, GAVL_META_F_NUMBER);
      break;
    case EXIF_TAG_X_RESOLUTION:
      set_rational(fd, e, GAVL_META_X_RESOLUTION);
      break;
    case EXIF_TAG_Y_RESOLUTION:
      set_rational(fd, e, GAVL_META_Y_RESOLUTION);
      break;
#endif
    default:
      {
      char * tmp_string;
      tmp_string =
        bg_sprintf("Exif::%s", exif_tag_get_name_in_ifd(e->tag, ifd));
      
      switch(e->format)
        {
        case EXIF_FORMAT_RATIONAL:
          set_rational(fd, e, tmp_string);
          break;
        case EXIF_FORMAT_SHORT:
          set_short(fd, e, tmp_string);
          break;
        case EXIF_FORMAT_LONG:
          set_long(fd, e, tmp_string);
          break;
        case EXIF_FORMAT_ASCII:
          set_ascii(fd, e, tmp_string);
          break;
        default:
          fprintf(stderr, "Got unknown exif tag %x\n", e->tag);
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
          break;
          
        }
      free(tmp_string);
      }
      break;
    }
  
  }

static void foreach1(ExifContent * c, void * priv)
  {
  exif_content_foreach_entry(c, foreach2, priv);
  }

void bg_exif_get_metadata(const char * filename,
                          gavl_metadata_t * ret)
  {
  ExifData * d;
  foreach_data_t fd;
  fd.m = ret;
  fd.cnv = bg_charset_converter_create("UTF-16LE", "UTF-8");
  
  d = exif_data_new_from_file(filename);
  if(!d)
    return;
  exif_data_foreach_content(d, foreach1, &fd);
  exif_data_unref(d);

  bg_charset_converter_destroy(fd.cnv);
  }
