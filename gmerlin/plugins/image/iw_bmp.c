/*****************************************************************
 
  iw_bmp.c
 
  Copyright (c) 2003-2005 by Michael Gruenert - one78@web.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <plugin.h>
#include <utils.h>
#include <inttypes.h>

typedef struct
  {
  gavl_video_format_t format;
  FILE *bmp_file;

  /* Bmp header */
  uint16_t type;               /* Magic identifier            */
  uint32_t file_size;          /* File size in bytes          */
  uint16_t reserved1;          /* 0                           */  
  uint16_t reserved2;          /* 0                           */
  uint32_t offset;             /* Offset to image data, bytes */

  /* Bmp info header */
  uint32_t header_size;        /* Header size in bytes      */
  int width;                   /* Width and height of image */
  int height;                  /* Width and height of image */
  uint16_t planes;             /* Number of colour planes   */
  uint16_t bits;               /* Bits per pixel            */
  uint32_t compression;        /* Compression type          */
  uint32_t imagesize;          /* Image size in bytes       */
  int xresolution;             /* Pixels per meter          */
  int yresolution;             /* Pixels per meter          */
  uint32_t ncolours;           /* Number of colours         */
  uint32_t importantcolours;   /* Important colours         */
  
  } bmp_t;

static void write_16(FILE * output, uint32_t val)
  {
  uint8_t c[2];
  
  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  
  fwrite(c, 1, 2, output);
  }

static void write_32(FILE * output, uint32_t val)
  {
  uint8_t c[4];
  
  c[0] = val & 0xff;
  c[1] = (val >> 8) & 0xff;
  c[2] = (val >> 16) & 0xff;
  c[3] = (val >> 24) & 0xff;
  fwrite(c, 1, 4, output);
  }

static void * create_bmp()
  {
  bmp_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_bmp(void * priv)
  {
  bmp_t * bmp = (bmp_t*)priv;
  free(bmp);
  }

static int write_header_bmp(void * priv, const char * filename,
                            gavl_video_format_t * format)
  {
  bmp_t * bmp = (bmp_t*)priv;

  /* set header */
  bmp->type = 19778;                    /* 19228 = BM */
  bmp->file_size = 0;                   /* at this time no set */
  bmp->reserved1 = 0;                   /* must always be set to zero */
  bmp->reserved2 = 0;                   /* must always be set to zero */
  bmp->offset = 54;                     /* 14 for header + 40 for info header */

  /* set info header */
  bmp->header_size = 40;                /* 40 = big header / 12 = small header */
  bmp->width = format->image_width;
  bmp->height = format->image_height;
  bmp->planes = 1;                      /* set planes = 0 */
  bmp->bits = 24;                       /* set colorspace */
  format->pixelformat = GAVL_BGR_24;     /* set pixelformat */
  bmp->compression = 0;                 /* 0 = BI_RGB / 1 = BI_RLE4 / 2 = BI_RLE8 / 3 = BI_BITFIELDS */
  bmp->imagesize = (bmp->width * bmp->height * bmp->bits)/8;
  bmp->xresolution = format->pixel_width;
  bmp->yresolution = format->pixel_height;
  bmp->ncolours = 0;                     /* BI_RGB = 0 */
  bmp->importantcolours = 0;             /* BI_RGB = 0 */

  /* open file */
  bmp->bmp_file = fopen(filename,"wb");
  
  if(!bmp->bmp_file)
    {
    fprintf(stderr,"Can't open File %s\n", filename);
    return 0;
    }
  
  /* write header */
  write_16(bmp->bmp_file, bmp->type);
  write_32(bmp->bmp_file, bmp->file_size);
  write_16(bmp->bmp_file, bmp->reserved1);
  write_16(bmp->bmp_file, bmp->reserved2);
  write_32(bmp->bmp_file, bmp->offset);

  /* write info header */
  write_32(bmp->bmp_file, bmp->header_size);
  write_32(bmp->bmp_file, bmp->width);
  write_32(bmp->bmp_file, bmp->height);
  write_16(bmp->bmp_file, bmp->planes);
  write_16(bmp->bmp_file, bmp->bits);
  write_32(bmp->bmp_file, bmp->compression); 
  write_32(bmp->bmp_file, bmp->imagesize); 
  write_32(bmp->bmp_file, bmp->xresolution);
  write_32(bmp->bmp_file, bmp->yresolution);
  write_32(bmp->bmp_file, bmp->ncolours);
  write_32(bmp->bmp_file, bmp->importantcolours);

  return 1;
  }

static int write_image_bmp(void * priv, gavl_video_frame_t * frame)
  {
  bmp_t * bmp = (bmp_t*)priv;
  uint8_t *frame_ptr;
  uint8_t *frame_ptr_start;
  int i, skip;
  uint8_t zero = 0x00;

  /* write image data */
  frame_ptr_start = frame->planes[0] + (bmp->height - 1) * frame->strides[0];

  skip = (4 - ((bmp->width * 3) % 4)) & 3;
#if 0  
  if(bmp->width % 2)
    skip = 1;
  else
    skip = 0;
#endif
  for (i = 0; i < bmp->height; i++)
    {
    frame_ptr = frame_ptr_start ;
    fwrite(frame_ptr, 3, bmp->width, bmp->bmp_file);
    for(i = 0; i < skip; i++)
      fwrite(&zero, 1, 1, bmp->bmp_file);
    frame_ptr_start -= frame->strides[0];
    }

  /* find file size */
  fseek(bmp->bmp_file, 0, SEEK_END);

  if((bmp->file_size = ftell(bmp->bmp_file))<0)
    return 0;

  // fprintf(stderr," file_size = %d \n",bmp->file_size);
  fseek(bmp->bmp_file, 2, SEEK_SET);

  /* write file size to header */
  write_32(bmp->bmp_file, bmp->file_size);

  if(bmp->bmp_file)
    fclose(bmp->bmp_file);

  return 1;
  }

static char * bmp_extension = ".bmp";

static const char * get_extension_bmp(void * p)
  {
  return bmp_extension;
  }

bg_image_writer_plugin_t the_plugin =
  {
    common:
    {
      name:           "iw_bmp",
      long_name:      "BMP writer",
      mimetypes:      (char*)0,
      extensions:     "bmp",
      type:           BG_PLUGIN_IMAGE_WRITER,
      flags:          BG_PLUGIN_FILE,
      priority:       5,
      create:         create_bmp,
      destroy:        destroy_bmp,
    },
    write_header: write_header_bmp,
    get_extension: get_extension_bmp,
    write_image:  write_image_bmp,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
