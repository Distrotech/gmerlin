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

static void get_long_le(uint8_t * data, uint32_t * ret1)
  {
  uint32_t ret;
  ret = data[3];
  ret <<= 8;
  ret |= data[2];
  ret <<= 8;
  ret |= data[1];
  ret <<= 8;
  ret |= data[0];
  *ret1 = ret;
  }

static void get_short_le(uint8_t * data, uint16_t * ret1)
  {
  uint16_t ret;
  ret = data[1];
  ret <<= 8;
  ret |= data[0];
  *ret1 = ret;
  }

static void get_long_be(uint8_t * data, uint32_t * ret1)
  {
  uint32_t ret;
  ret = data[0];
  ret <<= 8;
  ret |= data[1];
  ret <<= 8;
  ret |= data[2];
  ret <<= 8;
  ret |= data[3];
  *ret1 = ret;
  }

static void get_short_be(uint8_t * data, uint16_t * ret1)
  {
  uint16_t ret;
  ret = data[0];
  ret <<= 8;
  ret |= data[1];
  *ret1 = ret;
  }

static uint32_t get_long(uint8_t * data, ExifByteOrder bo)
  {
  uint32_t ret;
  if(bo == EXIF_BYTE_ORDER_INTEL)
    get_long_le(data, &ret);
  else
    get_long_be(data, &ret);
  return ret;
  }

static int32_t get_slong(uint8_t * data, ExifByteOrder bo)
  {
  int32_t ret;
  if(bo == EXIF_BYTE_ORDER_INTEL)
    get_long_le(data, (uint32_t*)&ret);
  else
    get_long_be(data, (uint32_t*)&ret);
  return ret;
  }

static uint16_t get_short(uint8_t * data, ExifByteOrder bo)
  {
  uint16_t ret;
  if(bo == EXIF_BYTE_ORDER_INTEL)
    get_short_le(data, &ret);
  else
    get_short_be(data, &ret);
  return ret;
  }

typedef struct
  {
  gavl_metadata_t * m;
  bg_charset_converter_t * cnv;
  ExifByteOrder bo;
  } foreach_data_t;

typedef struct
  {
  int val;
  const char * str;
  } enum_tab_t;

static const char * get_enum_label(const enum_tab_t * tab, int val)
  {
  int i = 0;
  while(tab[i].str)
    {
    if(tab[i].val == val)
      return tab[i].str;
    i++;
    }
  return "unknown";
  }

static void set_enum(foreach_data_t * fd,
                     ExifEntry * e,
                     const char * key,
                     const enum_tab_t * tab)
  {
  int val;
  const char * str;
  
  if(e->format == EXIF_FORMAT_SHORT)
    val = get_short(e->data, fd->bo);
  else if((e->format == EXIF_FORMAT_UNDEFINED) &&
          (e->size == 1))
    val = *e->data;
  
  else
    return;

  str = get_enum_label(tab, val);

  gavl_metadata_set_nocpy(fd->m, key,
                          bg_sprintf("%d (%s)", val, str));
  }

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
                            gavl_strndup((char*)e->data, end));
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

  num = get_long(e->data, fd->bo);
  den = get_long(e->data + 4, fd->bo);

  gavl_metadata_set_nocpy(fd->m, key,
                          bg_sprintf("%.4f [%d/%d]",
                                     (float)num / (float)den,
                                     num, den));
  }

static void set_srational(foreach_data_t * fd,
                          ExifEntry * e,
                          const char * key)
  {
  int32_t num, den;
  if((e->format !=  EXIF_FORMAT_SRATIONAL) ||
     (e->size != 8))
    return;

  num = get_slong(e->data, fd->bo);
  den = get_slong(e->data + 4, fd->bo);
  
  gavl_metadata_set_nocpy(fd->m, key,
                          bg_sprintf("%.4f [%d/%d]",
                                     (float)num / (float)den,
                                     num, den));
  }

static void set_long(foreach_data_t * fd,
                     ExifEntry * e,
                     const char * key)
  {
  uint32_t num;
  if((e->format !=  EXIF_FORMAT_LONG) ||
     (e->size != 4))
    return;

  num = get_long(e->data, fd->bo);
  
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

  num = get_short(e->data, fd->bo);
  
  gavl_metadata_set_int(fd->m, key, num);
  }

static void set_version(foreach_data_t * fd,
                        ExifEntry * e,
                        const char * key)
  {
  if(e->format !=  EXIF_FORMAT_UNDEFINED)
    return;
  
  gavl_metadata_set_nocpy(fd->m, key,
                          gavl_strndup((char*)e->data,
                                       (char*)(e->data + e->size)));
  }

const enum_tab_t resolution_units[] =
  {
    { 2, "inches" },
    { 3, "centimeters" },
    { },
  };

const enum_tab_t compressions[] =
  {
    { 1, "uncompressed" },
    { 6, "JPEG"         },
    { },
  };

const enum_tab_t colorspaces[] =
  {
    { 1, "sRGB" },
    { 0xFFFF, "Uncalibrated" },
    { },
  };

const enum_tab_t metering_modes[] =
  {
    { 0, "unknown" },
    { 1, "Average" },
    { 2, "CenterWeightedAverage" },
    { 3, "Spot" },
    { 4, "MultiSpot" },
    { 5, "Pattern" },
    { 6, "Partial" },
    { 255, "other" },
    { },
  };

const enum_tab_t sensing_methods[] =
  {
    { 1, "Not defined" },
    { 2, "One-chip color area sensor" },
    { 3, "Two-chip color area sensor" },
    { 4, "Three-chip color area sensor" },
    { 5, "Color sequential area sensor" },
    { 7, "Trilinear sensor" },
    { 8, "Color sequential linear sensor" },
    { },
  };

const enum_tab_t custom_rendered[] =
  {
    { 0, "Normal process" },
    { 1, "Custom process" },
    { },
  };

const enum_tab_t exposure_modes[] =
  {
    { 0, "Auto exposure" },
    { 1, "Manual exposure" },
    { 2, "Auto bracket" },
    { },
  };

const enum_tab_t white_balance[] =
  {
    { 0, "Auto white balance"   },
    { 1, "Manual white balance" },
    { },
  };

const enum_tab_t scene_capture_types[] =
  {

    { 0, "Standard" },
    { 1, "Landscape" },
    { 2, "Portrait" },
    { 3, "Night scene" },
    { },
  };

const enum_tab_t flash_modes[] =
  {

    { 0x0000, "Flash did not fire" },
    { 0x0001, "Flash fired" },
    { 0x0005, "Strobe return light not detected" },
    { 0x0007, "Strobe return light detected" },
    { 0x0009, "Flash fired, compulsory flash mode" },
    { 0x000D, "Flash fired, compulsory flash mode, return light not detected" },
    { 0x000F, "Flash fired, compulsory flash mode, return light detected" },
    { 0x0010, "Flash did not fire, compulsory flash mode" },
    { 0x0018, "Flash did not fire, auto mode" },
    { 0x0019, "Flash fired, auto mode" },
    { 0x001D, "Flash fired, auto mode, return light not detected" },
    { 0x001F, "Flash fired, auto mode, return light detected" },
    { 0x0020, "No flash function" },
    { 0x0041, "Flash fired, red-eye reduction mode" },
    { 0x0045, "Flash fired, red-eye reduction mode, return light not detected" },
    { 0x0047, "Flash fired, red-eye reduction mode, return light detected" },
    { 0x0049, "Flash fired, compulsory flash mode, red-eye reduction mode" },
    { 0x004D, "Flash fired, compulsory flash mode, red-eye reduction mode, return light not detected" },
    { 0x004F, "Flash fired, compulsory flash mode, red-eye reduction mode, return light detected" },
    { 0x0059, "Flash fired, auto mode, red-eye reduction mode" },
    { 0x005D, "Flash fired, auto mode, return light not detected, red-eye reduction mode" },
    { 0x005F, "Flash fired, auto mode, return light detected, red-eye reduction mode" },
    { },
  };

const enum_tab_t file_sources[] =
  {
    { 3, "DSC" },
    { },
  };

const enum_tab_t components[] =
  {
    { 0, "-"  },
    { 1, "Y"  },
    { 2, "Cb" },
    { 3, "Cr" },
    { 4, "R"  },
    { 5, "G"  },
    { 6, "B"  },
    { },
  };

static void foreach2(ExifEntry * e, void * priv)
  {
  int done;
  foreach_data_t  * fd;
  char * tmp_string = NULL;

  ExifIfd ifd = exif_content_get_ifd(e->parent);

  fd = priv;

  done = 1;
  
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
    default:
      done = 0;
      break;
    }

  if(done)
    return;

  tmp_string =
    bg_sprintf("Exif::%s", exif_tag_get_name_in_ifd(e->tag, ifd));

  switch(e->tag)
    {
    case EXIF_TAG_COMPONENTS_CONFIGURATION:
      if(e->size == 4)
        gavl_metadata_set_nocpy(fd->m, tmp_string,
                                bg_sprintf("%s, %s, %s, %s",
                                           get_enum_label(components, e->data[0]),
                                           get_enum_label(components, e->data[1]),
                                           get_enum_label(components, e->data[2]),
                                           get_enum_label(components, e->data[3])));
      break;
    case EXIF_TAG_FILE_SOURCE:
      set_enum(fd, e, tmp_string, file_sources);
      break;
    case EXIF_TAG_FLASH:
      set_enum(fd, e, tmp_string, flash_modes);
      break;
    case EXIF_TAG_SCENE_CAPTURE_TYPE:
      set_enum(fd, e, tmp_string, scene_capture_types);
      break;
    case EXIF_TAG_WHITE_BALANCE:
      set_enum(fd, e, tmp_string, white_balance);
      break;
    case EXIF_TAG_EXPOSURE_MODE:
      set_enum(fd, e, tmp_string, exposure_modes);
      break;
    case EXIF_TAG_CUSTOM_RENDERED:
      set_enum(fd, e, tmp_string, custom_rendered);
      break;
    case EXIF_TAG_SENSING_METHOD:
      set_enum(fd, e, tmp_string, sensing_methods);
      break;
    case EXIF_TAG_RESOLUTION_UNIT:
    case EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT:
      set_enum(fd, e, tmp_string, resolution_units);
      break;
    case EXIF_TAG_METERING_MODE:
      set_enum(fd, e, tmp_string, metering_modes);
      break;
    case EXIF_TAG_COMPRESSION:
      set_enum(fd, e, tmp_string, compressions);
      break;
    case EXIF_TAG_COLOR_SPACE:
      set_enum(fd, e, tmp_string, colorspaces);
      break;
    case EXIF_TAG_INTEROPERABILITY_VERSION:
    case EXIF_TAG_EXIF_VERSION:
    case EXIF_TAG_FLASH_PIX_VERSION:
      set_version(fd, e, tmp_string);
      break;
    default:
      switch(e->format)
        {
        case EXIF_FORMAT_RATIONAL:
          set_rational(fd, e, tmp_string);
          break;
        case EXIF_FORMAT_SRATIONAL:
          set_srational(fd, e, tmp_string);
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
#if 0
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
#endif
          break;
          
        }
      break;
    }
  if(tmp_string)
    free(tmp_string);
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
  
  d = exif_data_new_from_file(filename);
  if(!d)
    return;

  fd.cnv = bg_charset_converter_create("UTF-16LE", "UTF-8");
  fd.bo = exif_data_get_byte_order(d);
  
  exif_data_foreach_content(d, foreach1, &fd);
  exif_data_unref(d);

  bg_charset_converter_destroy(fd.cnv);
  }
