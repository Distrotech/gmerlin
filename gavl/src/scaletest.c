#include <stdlib.h>
#include <gavl.h>
//#include "colorspace.h" // Common routines
#include <stdio.h>
#include <png.h>



static void write_png(char * filename, gavl_video_format_t * format, gavl_video_frame_t * frame)
  {
  int i;
  unsigned char ** rows;

  int color_type;
  FILE * output;

  png_structp png_ptr;
  png_infop   info_ptr;
  
  gavl_video_options_t opt;
  gavl_video_converter_t * cnv;
    
  gavl_video_format_t format_1;
  gavl_video_frame_t * frame_1 = (gavl_video_frame_t*)0;

  gavl_video_default_options(&opt);
  
  if((format->colorspace != GAVL_RGB_24) && (format->colorspace != GAVL_RGBA_32))
    {
    gavl_video_default_options(&opt);
    cnv = gavl_video_converter_create();
    
    gavl_video_format_copy(&format_1, format);

    if(gavl_colorspace_has_alpha(format->colorspace))
      {
      format_1.colorspace = GAVL_RGBA_32;
      color_type = PNG_COLOR_TYPE_RGBA;
      }
    else
      {
      format_1.colorspace = GAVL_RGB_24;
      color_type = PNG_COLOR_TYPE_RGB;
      }
    frame_1 = gavl_video_frame_create(&format_1);
    
    gavl_video_converter_init(cnv, &opt, format, &format_1);
    
    gavl_video_convert(cnv, frame, frame_1);
    gavl_video_converter_destroy(cnv);
    }
  else if(format->colorspace == GAVL_RGB_24)
    {
    color_type = PNG_COLOR_TYPE_RGB;
    }
  else
    {
    color_type = PNG_COLOR_TYPE_RGBA;
    }
  
  output = fopen(filename, "wb");
  if(!output)
    return;

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                                         NULL, NULL);

  info_ptr = png_create_info_struct(png_ptr);
  setjmp(png_jmpbuf(png_ptr));
  png_init_io(png_ptr, output);
  
  png_set_IHDR(png_ptr, info_ptr,
               format->image_width,
               format->image_height,
               8, color_type, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  rows = malloc(format->image_height * sizeof(*rows));

  if(frame_1)
    {
    for(i = 0; i < format->image_height; i++)
      rows[i] = frame_1->planes[0] + i * frame_1->strides[0];
    }
  else
    {
    for(i = 0; i < format->image_height; i++)
      rows[i] = frame->planes[0] + i * frame->strides[0];
    }
  
  png_set_rows(png_ptr, info_ptr, rows);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(output);
  free(rows);
  if(frame_1)
    gavl_video_frame_destroy(frame_1);
  }

static gavl_video_frame_t * read_png(const char * filename,
                                     gavl_video_format_t * format,
                                     gavl_colorspace_t colorspace)
  {
  int i;
  unsigned char ** rows;
  
  gavl_video_converter_t * cnv;
  gavl_video_options_t opt;
  gavl_video_format_t format_1;
  gavl_video_frame_t * frame, * frame_1;
    
  int bit_depth;
  int color_type;
  int has_alpha = 0;

  png_structp png_ptr;
  png_infop info_ptr;
  png_infop end_info;


  FILE * file;
  
  file = fopen(filename, "rb");

  if(!file)
    {
    fprintf(stderr, "Cannot open file %s\n", filename);
    return NULL;
    }
  
  png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)0,
     NULL, NULL);
  
  setjmp(png_jmpbuf(png_ptr));
  info_ptr = png_create_info_struct(png_ptr);


  end_info = png_create_info_struct(png_ptr);

  png_init_io(png_ptr, file);

  png_read_info(png_ptr, info_ptr);

  format->frame_width  = png_get_image_width(png_ptr, info_ptr);
  format->frame_height = png_get_image_height(png_ptr, info_ptr);

  format->image_width  = format->frame_width;
  format->image_height = format->frame_height;
  format->pixel_width = 1;
  format->pixel_height = 1;

  bit_depth  = png_get_bit_depth(png_ptr,  info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  switch(color_type)
    {
    case PNG_COLOR_TYPE_GRAY:       /*  (bit depths 1, 2, 4, 8, 16) */
      if(bit_depth < 8)
        png_set_gray_1_2_4_to_8(png_ptr);
      if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
        png_set_tRNS_to_alpha(png_ptr);
        has_alpha = 1;
        }
      png_set_gray_to_rgb(png_ptr);
      break;
    case PNG_COLOR_TYPE_GRAY_ALPHA: /*  (bit depths 8, 16) */
      if(bit_depth == 16)
        png_set_strip_16(png_ptr);
      png_set_gray_to_rgb(png_ptr);
      break;
    case PNG_COLOR_TYPE_PALETTE:    /*  (bit depths 1, 2, 4, 8) */
      png_set_palette_to_rgb(png_ptr);
      if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
        png_set_tRNS_to_alpha(png_ptr);
        has_alpha = 1;
        }
      break;
    case PNG_COLOR_TYPE_RGB:        /*  (bit_depths 8, 16) */
      if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
        png_set_tRNS_to_alpha(png_ptr);
        has_alpha = 1;
        }
      if(bit_depth == 16)
        png_set_strip_16(png_ptr);
      break;
    case PNG_COLOR_TYPE_RGB_ALPHA:  /*  (bit_depths 8, 16) */
      if(bit_depth == 16)
        png_set_strip_16(png_ptr);
      has_alpha = 1;
      break;
    }
  if(has_alpha)
    format->colorspace = GAVL_RGBA_32;
  else
    format->colorspace = GAVL_RGB_24;

  frame = gavl_video_frame_create(format);
  rows = malloc(format->frame_height * sizeof(*rows));
  for(i = 0; i < format->frame_height; i++)
    rows[i] = frame->planes[0] + i * frame->strides[0];

  png_read_image(png_ptr, rows);
  png_read_end(png_ptr, end_info);

  png_destroy_read_struct(&(png_ptr), &(info_ptr),
                          &(end_info));
  fclose(file);
  free(rows);
  
  /* Check wether to set up the converter */

  if(format->colorspace != colorspace)
    {
    gavl_video_default_options(&opt);
    cnv = gavl_video_converter_create();

    gavl_video_format_copy(&format_1, format);
    format_1.colorspace = colorspace;
    frame_1 = gavl_video_frame_create(&format_1);
    
    gavl_video_converter_init(cnv, &opt, format, &format_1);
    
    gavl_video_convert(cnv, frame, frame_1);
    gavl_video_converter_destroy(cnv);
    format->colorspace = colorspace;
    }
  else
    frame_1 = (gavl_video_frame_t*)0;

  if(frame_1)
    {
    free(frame);
    return frame_1;
    }
  else
    return frame;
  }

int dst_width;
int dst_height;

int main(int argc, char ** argv)
  {
  char filename_buffer[1024];
  int i, imax;
  gavl_video_scaler_t *scaler;
    
  gavl_video_format_t format, format_1;
  gavl_video_frame_t * frame, * frame_1;

  gavl_colorspace_t csp = GAVL_RGB_32;
  gavl_scale_mode_t scale_mode = GAVL_SCALE_BILINEAR;

  imax = gavl_num_colorspaces();
  scaler = gavl_video_scaler_create();
  
  dst_width = atoi(argv[2]);
  dst_height = atoi(argv[3]);
  
  for(i = 0; i < imax; i++)
    {
    csp = gavl_get_colorspace(i);
    frame = read_png(argv[1], &format, csp);

    
    gavl_video_scaler_init(scaler,
                           scale_mode,
                           format.colorspace,
                           0, // int input_x,
                           0, // int input_y,
                           format.image_width, // int input_width,
                           format.image_height, // int input_height,
                           0, // int output_x,
                           0, // int output_y,
                           dst_width, // int output_width,
                           dst_height);  // int output_height
    
    gavl_video_format_copy(&format_1, &format);
    
    format_1.image_width = dst_width;
    format_1.image_height = dst_height;
    
    format_1.frame_width = dst_width;
    format_1.frame_height = dst_height;
    
    frame_1 = gavl_video_frame_create(&format_1);
    
    gavl_video_scaler_scale(scaler, frame, frame_1);

    sprintf(filename_buffer, "%s-scaled.png", gavl_colorspace_to_string(csp));
    write_png(filename_buffer, &format_1, frame_1);
    fprintf(stderr, "Wrote %s\n", filename_buffer);
    
    }

  
    return 0;
  }
