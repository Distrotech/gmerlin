/*****************************************************************
 * gavl - a general purpose audio/video processing library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <stdlib.h>
#include <stdio.h>

#include <gavl/gavl.h>
#include <video.h>
#include <transform.h>
#include <scale.h>

#define ROUND(val) (val >= 0.0) ? (int)(val+0.5):(int)(val-0.5)

void gavl_transform_table_init(gavl_transform_table_t * tab,
                               gavl_video_options_t * opt,
                               gavl_image_transform_func func, void * priv,
                               float off_x, float off_y, float scale_x,
                               float scale_y, int width, int height)
  {
  int i, j, k, l;
  double x_src_f, y_src_f, x_dst_f, y_dst_f;
  int x_src_nearest, y_src_nearest;
  double t;
  double w_x[MAX_TRANSFORM_FILTER];
  double w_y;
  
  gavl_video_scale_get_weight weight_func;

  /* (re)alloc */
  if(tab->pixels)
    {
    if(tab->pixels[0])
      free(tab->pixels[0]);
    free(tab->pixels);
    }
  
  /* Get factors per pixel and filter_func */
  weight_func =
    gavl_video_scale_get_weight_func(opt, &(tab->factors_per_pixel));

  if(tab->factors_per_pixel > MAX_TRANSFORM_FILTER)
    {
    fprintf(stderr, "BUG: tab->factors_per_pixel > MAX_TRANSFORM_FILTER\n");
    return;
    }
  
  tab->pixels = malloc(height * sizeof(*tab->pixels));
  tab->pixels[0] = calloc(width * height, sizeof(**tab->pixels));
  
  for(i = 1; i < height; i++)
    tab->pixels[i] = tab->pixels[0] + i * width;
  
  for(i = 0; i < height; i++)
    {
    y_dst_f = scale_y * (double)i + off_y;
    for(j = 0; j < width; j++)
      {
      x_dst_f = scale_x * (double)j + off_x;
      
      func(priv, x_dst_f, y_dst_f, &x_src_f, &y_src_f);

      //      fprintf(stderr, "Transform: %f %f -> %f %f\n",
      //              x_src_f, y_src_f, x_dst_f, y_dst_f);

      x_src_f = (x_src_f - off_x) / scale_x;
      y_src_f = (y_src_f - off_y) / scale_y;
      
      if((x_src_f < 0.0) || (x_src_f > (double)width) || 
         (y_src_f < 0.0) || (y_src_f > (double)height))
        {
        tab->pixels[i][j].outside = 1;
        //        fprintf(stderr, "Outside\n");
        continue;
        }

      /* ... */
      x_src_nearest = ROUND(x_src_f);
      y_src_nearest = ROUND(y_src_f);

      tab->pixels[i][j].index_x = x_src_nearest - tab->factors_per_pixel/2;
      tab->pixels[i][j].index_y = y_src_nearest - tab->factors_per_pixel/2;

      if(tab->factors_per_pixel == 1)
        {
        if(tab->pixels[i][j].index_x < 0)
          tab->pixels[i][j].index_x = 0;
        if(tab->pixels[i][j].index_x > width - 1)
          tab->pixels[i][j].index_x = width - 1;
        if(tab->pixels[i][j].index_y < 0)
          tab->pixels[i][j].index_y = 0;
        if(tab->pixels[i][j].index_y > height - 1)
          tab->pixels[i][j].index_y = height - 1;
        continue;
        }
      
      /* x weights */
      t = (x_src_f - tab->pixels[i][j].index_x);
      
      for(k = 0; k < tab->factors_per_pixel; k++)
        {
        w_x[k] = weight_func(opt, t);
        t -= 1.0;
        }
      for(k = 0; k < tab->factors_per_pixel; k++)
        {
        /* Y-weight */
        t = (y_src_f - tab->pixels[i][j].index_y);
        w_y = weight_func(opt, t);
        
        for(l = 0; l < tab->factors_per_pixel; l++)
          tab->pixels[i][j].factors[k][l] = w_y * w_x[l];
        
        }
      
      }
    
    }
  
  }
