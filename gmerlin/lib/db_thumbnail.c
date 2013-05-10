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
#include <unistd.h>

#include <mediadb_private.h>
#include <gmerlin/log.h>
#include <gmerlin/utils.h>
#include <string.h>

#define LOG_DOMAIN "db.thumbnail"

int bg_nmj_make_thumbnail(bg_plugin_registry_t * plugin_reg,
                          const char * in_file,
                          const char * out_file,
                          int thumb_size, int force_scale)
  {
  int ret = 0;
  /* Formats */
  
  gavl_video_format_t input_format;
  gavl_video_format_t output_format;
  
  /* Frames */
  
  gavl_video_frame_t * input_frame = NULL;
  gavl_video_frame_t * output_frame = NULL;

  gavl_video_converter_t * cnv = 0;
  int do_convert;
  bg_image_writer_plugin_t * output_plugin;
  bg_plugin_handle_t * output_handle = NULL;
  const bg_plugin_info_t * plugin_info;
  const char * extension;
  uint8_t * file_buf = NULL;
  int file_len;

  extension = strrchr(in_file, '.');
  if(!extension)
    return 0;
  extension++;

  input_frame = bg_plugin_registry_load_image(plugin_reg,
                                              in_file,
                                              &input_format, NULL);
  
  /* Return early for copying */
  if(!force_scale &&
     bg_string_match(extension, "jpg jpeg") &&
     (input_format.image_width <= thumb_size) &&
     (input_format.image_height <= thumb_size))
    {
    file_buf = bg_read_file(in_file, &file_len);
    if(!file_buf || !bg_write_file(out_file, file_buf, file_len))
      goto end;
    else
      {
      ret = 1;
      goto end;
      }
    }
  
  gavl_video_format_copy(&output_format, &input_format);
  
  if(force_scale)
    {
    /* Scale the image to square pixels */
    output_format.image_width *= output_format.pixel_width;
    output_format.image_height *= output_format.pixel_height;
  
    if(output_format.image_width > input_format.image_height)
      {
      output_format.image_height = (thumb_size * output_format.image_height) /
        output_format.image_width;
      output_format.image_width = thumb_size;
      }
    else
      {
      output_format.image_width      = (thumb_size * output_format.image_width) /
        output_format.image_height;
      output_format.image_height = thumb_size;
      }
    }
  /* Scale to max size */
  else
    {
    double w, h;
    double x;
    double par = (double)(output_format.pixel_width) / (double)(output_format.pixel_height);
    
    /* Enlarge vertically */
    if(par > 1.0)
      {
      w = (double)(output_format.image_width);
      h = (double)(output_format.image_height) / par;
      }
    /* Enlarge horizontally */
    else
      {
      w = (double)(output_format.image_width) * par;
      h = (double)(output_format.image_height);
      }
    
    if(w > h)
      {
      if(w > (double)thumb_size)
        {
        output_format.image_width = thumb_size;
        x = (double)thumb_size * h/w;
        output_format.image_height = (int)(x+0.5);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Downscaling image %s to %dx%d",
               in_file, output_format.image_width, output_format.image_height);
        }
      else
        {
        output_format.image_width = (int)(w+0.5);
        output_format.image_height = (int)(h+0.5);
        }
      }
    else
      {
      if(h > (double)thumb_size)
        {
        output_format.image_height = thumb_size;
        x = (double)thumb_size * w / h;
        output_format.image_width = (int)(x+0.5);
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Downscaling image %s to %dx%d",
               in_file, output_format.image_width, output_format.image_height);
        }
      else
        {
        output_format.image_width = (int)(w+0.5);
        output_format.image_height = (int)(h+0.5);
        }
      }
    }

  output_format.pixel_width = 1;
  output_format.pixel_height = 1;
  output_format.interlace_mode = GAVL_INTERLACE_NONE;

  output_format.frame_width = output_format.image_width;
  output_format.frame_height = output_format.image_height;

  plugin_info =
    bg_plugin_find_by_name(plugin_reg, "iw_jpeg");

  if(!plugin_info)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No plugin for %s", out_file);
    goto end;
    }

  output_handle = bg_plugin_load(plugin_reg, plugin_info);

  if(!output_handle)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Loading %s failed", plugin_info->long_name);
    goto end;
    }
  
  output_plugin = (bg_image_writer_plugin_t*)output_handle->plugin;

  if(!output_plugin->write_header(output_handle->priv,
                                  out_file, &output_format, NULL))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing image header failed");
    goto end;
    }

  /* Initialize video converter */
  cnv = gavl_video_converter_create();
  do_convert = gavl_video_converter_init(cnv, &input_format, &output_format);

  if(do_convert)
    {
    output_frame = gavl_video_frame_create(&output_format);
    gavl_video_convert(cnv, input_frame, output_frame);
    if(!output_plugin->write_image(output_handle->priv,
                                   output_frame))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing image failed");
      goto end;
      }
    }
  else
    {
    if(!output_plugin->write_image(output_handle->priv,
                                   input_frame))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing image failed");
      goto end;
      }
    }
  
  ret = 1;
  end:

  if(file_buf)
    free(file_buf);
  if(input_frame)
    gavl_video_frame_destroy(input_frame);
  if(output_frame)
    gavl_video_frame_destroy(output_frame);
  if(output_handle)
    bg_plugin_unref(output_handle);
  if(cnv)
    gavl_video_converter_destroy(cnv);
  return ret;
  }


bg_db_object_t * bg_db_get_thumbnail(bg_db_t * db, int64_t id, int max_width, int max_height)
  {

  }

void bg_db_browse_thumbnails(bg_db_t * db, int64_t parent_id, 
                             bg_db_query_callback cb, void * data)
  {

  }
