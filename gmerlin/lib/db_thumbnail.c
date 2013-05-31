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

#define TH_CACHE_SIZE 32


typedef struct
  {
  char * filename;
  } iw_t;

static int create_file(void * data, const char * filename)
  {
  iw_t * iw = data;
  iw->filename = gavl_strdup(filename);
  return 1;
  }

static char * save_image(bg_db_t * db,
                         gavl_video_frame_t * in_frame,
                         gavl_video_format_t * in_format,
                         gavl_video_format_t * out_format,
                         gavl_video_converter_t * cnv,
                         int64_t id, const char * mimetype)
  {
  int result = 0;
  int do_convert;
  gavl_video_frame_t * output_frame = NULL;
  bg_image_writer_plugin_t * output_plugin;
  bg_plugin_handle_t * output_handle = NULL;
  const bg_plugin_info_t * plugin_info;
  iw_t iw;
  bg_iw_callbacks_t cb;

  char * out_file_base = bg_sprintf("gmerlin-db/thumbnails/%016"PRId64, id);

  out_file_base = bg_db_filename_to_abs(db, out_file_base);
  memset(&iw, 0, sizeof(iw));
  memset(&cb, 0, sizeof(cb));
  
  cb.create_output_file = create_file;
  cb.data = &iw;
  
  out_format->pixel_width = 1;
  out_format->pixel_height = 1;
  out_format->interlace_mode = GAVL_INTERLACE_NONE;

  out_format->frame_width = out_format->image_width;
  out_format->frame_height = out_format->image_height;

  plugin_info =
    bg_plugin_find_by_mimetype(db->plugin_reg, mimetype, BG_PLUGIN_IMAGE_WRITER);
  
  if(!plugin_info)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No plugin for %s", mimetype);
    goto end;
    }

  output_handle = bg_plugin_load(db->plugin_reg, plugin_info);

  if(!output_handle)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Loading %s failed", plugin_info->long_name);
    goto end;
    }
  
  output_plugin = (bg_image_writer_plugin_t*)output_handle->plugin;

  output_plugin->set_callbacks(output_handle->priv, &cb);
  
  if(!output_plugin->write_header(output_handle->priv,
                                  out_file_base, out_format, NULL))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing image header failed");
    goto end;
    }

  /* Initialize video converter */
  do_convert = gavl_video_converter_init(cnv, in_format, out_format);

  if(do_convert)
    {
    output_frame = gavl_video_frame_create(out_format);
    gavl_video_frame_clear(output_frame, out_format);
    gavl_video_convert(cnv, in_frame, output_frame);
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
                                   in_frame))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing image failed");
      goto end;
      }
    }
  result = 1;
  
  end:

  if(output_frame)
    gavl_video_frame_destroy(output_frame);
  if(output_handle)
    bg_plugin_unref(output_handle);
  if(out_file_base)
    free(out_file_base);
  
  if(result)
    return iw.filename;
  
  if(iw.filename)
    free(iw.filename);
  return NULL;
  
  }
                         

static void *
make_thumbnail(bg_db_t * db,
               bg_db_object_t * obj,
               int max_width, int max_height,
               const char * mimetype)
  {
  bg_db_file_t * thumb;
  int ret = 0;
  /* Formats */
  gavl_video_format_t input_format;
  gavl_video_format_t output_format;
  
  /* Frames */
  gavl_video_frame_t * input_frame = NULL;

  gavl_video_converter_t * cnv = 0;
  const char * src_ext;
  bg_db_image_file_t * image = (bg_db_image_file_t *)obj;

  char * path_abs;
  bg_db_scan_item_t item;

  double ext_x, ext_y;
  double ar;
  
  src_ext = strrchr(image->file.path, '.');
  if(src_ext)
    src_ext++;
  
  /* Return early */
  if(image->file.mimetype &&
     !strcasecmp(image->file.mimetype, mimetype) &&
     (image->width <= max_width) &&
     (image->height <= max_height))
    {
    bg_db_object_ref(image);
    return obj;
    }

  memset(&input_format, 0, sizeof(input_format));
  
  cnv = gavl_video_converter_create();
  input_frame = bg_plugin_registry_load_image(db->plugin_reg,
                                              image->file.path,
                                              &input_format, NULL);

  ar = (double)input_format.image_width / (double)input_format.image_height;
  
  gavl_video_format_copy(&output_format, &input_format);
    
  ext_x = (double)input_format.image_width / (double)max_width;
  ext_y = (double)input_format.image_height / (double)max_height;

    
  if((ext_x > 1.0) || (ext_y > 1.0))
    {
    if(ext_x > ext_y) // Fit to max_width
      {
      output_format.image_width  = max_width;
      output_format.image_height = (int)((double)max_width / ar + 0.5);
      }
    else // Fit to max_height
      {
      output_format.image_height  = max_height;
      output_format.image_width = (int)((double)max_height * ar + 0.5);
      }
    }
  
  /* Save image */

  thumb = bg_db_object_create(db);
  
  path_abs = save_image(db, input_frame, &input_format, &output_format,
                        cnv, bg_db_object_get_id(thumb), mimetype);
  if(!path_abs)
    goto end;
  
  /* Create a new image object */

  memset(&item, 0, sizeof(item));
  if(!bg_db_scan_item_set(&item, path_abs))
    goto end;

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Made thumbnail %dx%d in %s",
         output_format.image_width,
         output_format.image_height, path_abs);
  
  
  thumb = bg_db_file_create_from_object(db, (bg_db_object_t*)thumb, ~0, &item, -1);
  if(!thumb)
    goto end;
  
  bg_db_object_set_type(thumb, BG_DB_OBJECT_THUMBNAIL);
  thumb->obj.ref_id = bg_db_object_get_id(image);
  bg_db_object_set_parent_id(db, thumb, -1);
  
  
  bg_db_scan_item_free(&item);
  
  ret = 1;
  
  end:

  if(input_frame)
    gavl_video_frame_destroy(input_frame);
  if(cnv)
    gavl_video_converter_destroy(cnv);

  if(!ret)
    {
    bg_db_object_delete(db, thumb);
    return NULL;
    }
  return thumb;
  }

void bg_db_browse_thumbnails(bg_db_t * db, int64_t id, 
                             bg_db_query_callback cb, void * data)
  {
  char * sql;
  int result;
  int i;
  bg_db_object_t * image;
  bg_sqlite_id_tab_t tab;
  bg_sqlite_id_tab_init(&tab);
  
  image = bg_db_object_query(db, id);
  cb(data, image);
  bg_db_object_unref(image);
  
  sql = sqlite3_mprintf("SELECT ID FROM OBJECTS WHERE (TYPE = %"PRId64") & (REF_ID = %"PRId64");",
                        BG_DB_OBJECT_THUMBNAIL, id);
  result = bg_sqlite_exec(db->db, sql, bg_sqlite_append_id_callback, &tab);
  sqlite3_free(sql);

  for(i = 0; i < tab.num_val; i++)
    {
    image = bg_db_object_query(db, tab.val[i]);
    cb(data, image);
    bg_db_object_unref(image);
    }

  bg_sqlite_id_tab_free(&tab);
  
  }

typedef struct
  {
  int max_width;
  int max_height;
  const char * mimetype;
  bg_db_image_file_t * ret;
  } browse_t;

static void browse_callback(void * data, void * obj)
  {
  browse_t * b = data;
  bg_db_image_file_t * image = obj;

  if(b->mimetype && strcmp(image->file.mimetype, b->mimetype))
    return;
  
  if((image->width > b->max_width) || (image->height > b->max_height))
    return;
  
  if(!b->ret || (b->ret->width < image->width))
    {
    if(b->ret)
      bg_db_object_unref(b->ret);
    b->ret = image;
    bg_db_object_ref(b->ret);
    }
  }


void bg_db_thumbnail_cache_init(bg_db_thumbnail_cache_t * c)
  {
  c->alloc = TH_CACHE_SIZE;
  c->items = calloc(c->alloc, sizeof(*c->items));
  }

void bg_db_thumbnail_cache_free(bg_db_thumbnail_cache_t * c)
  {
  int i;
  for(i = 0; i < c->size; i++)
    {
    if(c->items[i].mimetype)
      free(c->items[i].mimetype);
    }
  free(c->items);
  }

static int64_t cache_get(bg_db_thumbnail_cache_t * c,
                         int64_t id,
                         int max_width, int max_height,
                         const char * mimetype)
  {
  int i;
  for(i = 0; i < c->size; i++)
    {
    if((c->items[i].ref_id == id) &&
       (c->items[i].max_width == max_width) &&
       (c->items[i].max_height == max_height) &&
       !(strcmp(c->items[i].mimetype, mimetype)))
      return c->items[i].thumb_id;
    }
  return -1;
  }

static void cache_put(bg_db_thumbnail_cache_t * c,
                      int64_t id,
                      int max_width, int max_height,
                      const char * mimetype, int thumb_id)
  {
  if(c->size == c->alloc)
    {
    if(c->items[c->size - 1].mimetype)
      free(c->items[c->size - 1].mimetype);
    c->size--;
    memmove(c->items + 1, c->items, c->size * sizeof(*c->items));
    }
  c->items[0].ref_id = id;
  c->items[0].thumb_id = thumb_id;
  c->items[0].max_width = max_width;
  c->items[0].max_height = max_height;
  c->items[0].mimetype = gavl_strdup(mimetype);
  c->size++;
  }
  
void * bg_db_get_thumbnail(bg_db_t * db, int64_t id,
                           int max_width, int max_height,
                           const char * mimetype)
  {
  browse_t b;
  int64_t thumb_id;

  if((thumb_id = cache_get(&db->th_cache,
               id, max_width, max_height,
               mimetype)) > 0)
    return bg_db_object_query(db, thumb_id);
  
  memset(&b, 0, sizeof(b));
  b.max_width = max_width;
  b.max_height = max_height;
  b.mimetype = mimetype;

  bg_db_browse_thumbnails(db, id, browse_callback, &b);
  
  if(!b.ret)
    {
    bg_db_object_t * img = bg_db_object_query(db, id);
    b.ret = make_thumbnail(db,
                           img, max_width, max_height, mimetype);
    bg_db_object_unref(img);
    }
  cache_put(&db->th_cache,
            id,
            max_width, max_height,
            mimetype, bg_db_object_get_id(b.ret));
  return b.ret;
  }

