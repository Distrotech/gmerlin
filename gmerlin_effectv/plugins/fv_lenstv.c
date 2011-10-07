/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * lensTV - old skool Demo lens Effect
 * Code taken from "The Demo Effects Colletion" 0.0.4
 * http://www.paassen.tmfweb.nl/retrodemo.html
 *
 *
 * Ported to EffecTV BSB by Buddy Smith
 * Modified from BSB for EffecTV 0.3.x by Ed Tannenbaaum
 * ET added interactive control via mouse as follows....
 * Spacebar toggles interactive mode (off by default)
 * In interactive mode:
 *   Mouse with no buttons pressed moves magnifier
 *   Left button and y movement controls size of magnifier
 *   Right Button and y movement controls magnification.
 *
 * This works best in Fullscreen mode due to mouse trapping
 *
 * You can now read the fine print in the TV advertisements!
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gmerlin_effectv.h>
#include <utils.h>

static int start(effect *e);
static int stop(effect *e);
static int draw(effect *e, RGB32 *src, RGB32 *dst);
static void init(effect *e);

typedef struct
  {
  float x;
  float y;
  int xi;
  int yi;
  
  float xf;
  float yf;
  float lens_width;

  int lens_width_i;
  int lens_zoom;
  int state;
  int *lens;
  } lenstv_t;

static void apply_lens(effect *e, int ox, int oy,RGB32 *src,RGB32 *dst)
{
    int x, y, noy,pos, nox;
	int *p;
        lenstv_t * priv = e->priv;
        
	p = priv->lens;
	for (y = 0; y < priv->lens_width_i; y++) {
		for (x = 0; x < priv->lens_width_i; x++) {
			noy=(y+oy); nox=(x+ox);
			if ((nox>=0)&&(noy>=0)&&(nox<e->video_width)&&(noy<e->video_height)){
				pos = (noy * e->video_width) + nox;
				*(dst+pos) = *(src+pos + *p);
			}
			p++;
		}
	}
}


static effect *lensRegister(void)
{
lenstv_t * priv;
effect *entry;
        //	mode=1;
	entry = calloc(1, sizeof(effect));
	if(entry == NULL) return NULL;

	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
        priv = calloc(1, sizeof(*priv));
        entry->priv = priv;

	return entry;
}

static int start(effect *e)
  {
  lenstv_t * priv = e->priv;
  
  init(e);
  priv->state = 1;
  return 0;
  }

static int stop(effect *e)
  {
  lenstv_t * priv = e->priv;
  priv->state = 0;
  if(priv->lens)
    {
    free(priv->lens);
    priv->lens = NULL;
    }
  return 0;
  }

static int draw(effect *e, RGB32 *src, RGB32 *dst)
  {
  lenstv_t * priv = e->priv;
  memcpy(dst, src, e->video_area * PIXEL_SIZE);
  apply_lens(e,priv->xi,priv->yi,src,dst);
  return 0;
  }

static void init(effect *e) {

  int x,y,r,d;
  lenstv_t * priv = e->priv;

  if(!e->video_width || !e->video_height)
    return;
  priv->lens_width_i =
    (e->video_width > e->video_height) ?
    (int)(priv->lens_width * e->video_height + 0.5) :
    (int)(priv->lens_width * e->video_width + 0.5);

  priv->xi = (int)(priv->x * (e->video_width-priv->lens_width_i) + 0.5);
  priv->yi = (int)(priv->y * (e->video_height-priv->lens_width_i) + 0.5);
  
  
  if(priv->lens != NULL) {
	  free(priv->lens);
  }
  priv->lens = malloc(priv->lens_width_i * priv->lens_width_i * sizeof(int));
  memset(priv->lens, 0, priv->lens_width_i * priv->lens_width_i * sizeof(int));

    /* generate the lens distortion */
    r = priv->lens_width_i/2;
    d = priv->lens_zoom;

    /* the shift in the following expression is a function of the
     * distance of the current point from the center of the sphere.
     * If you imagine:
     *
     *       eye
     *
     *   .-~~~~~~~-.    sphere surface
     * .`           '.
     * ---------------  viewing plane
     *        .         center of sphere
     *
     * For each point across the viewing plane, draw a line from the
     * point on the sphere directly above that point to the center of
     * the sphere.  It will intersect the viewing plane somewhere
     * closer to the center of the sphere than the original point.
     * The shift function below is the end result of the above math,
     * given that the height of the point on the sphere can be derived
     * from:
     *
     * x^2 + y^2 + z^2 = radius^2
     *
     * x and y are known, z is based on the height of the viewing
     * plane.
     *
     * The radius of the sphere is the distance from the center of the
     * sphere to the edge of the viewing plane, which is a neat little
     * triangle.  If d = the distance from the center of the sphere to
     * the center of the plane (aka, lens_zoom) and r = half the width
     * of the plane (aka, lens_width/2) then radius^2 = d^2 + r^2.
     *
     * Things become simpler if we take z=0 to be at the plane's
     * height rather than the center of the sphere, turning the z^2 in
     * the expression above to (z+d)^2, since the center is now at
     * (0, 0, -d).
     *
     * So, the resulting function looks like:
     *
     * x^2 + y^2 + (z+d)^2 = d^2 + r^2
     *
     * Expand the (z-d)^2:
     *
     * x^2 + y^2 + z^2 + 2dz + d^2 = d^2 + r^2
     *
     * Rearrange things to be a quadratic in terms of z:
     *
     * z^2 + 2dz + x^2 + y^2 - r^2 = 0
     *
     * Note that x, y, and r are constants, so apply the quadratic
     * formula:
     *
     * For ax^2 + bx + c = 0,
     * 
     * x = (-b +- sqrt(b^2 - 4ac)) / 2a
     *
     * We can ignore the negative result, because we want the point at
     * the top of the sphere, not at the bottom.
     *
     * x = (-2d + sqrt(4d^2 - 4 * (x^2 + y^2 - r^2))) / 2
     *
     * Note that you can take the -4 out of both expressions in the
     * square root to put -2 outside, which then cancels out the
     * division:
     *
     * z = -d + sqrt(d^2 - (x^2 + y^2 - r^2))
     *
     * This now gives us the height of the point on the sphere
     * directly above the equivalent point on the plane.  Next we need
     * to find where the line between this point and the center of the
     * sphere at (0, 0, -d) intersects the viewing plane at (?, ?, 0).
     * This is a matter of the ratio of line below the plane vs the
     * total line length, multiplied by the (x,y) coordinates.  This
     * ratio can be worked out by the height of the line fragment
     * below the plane, which is d, and the total height of the line,
     * which is d + z, or the height above the plane of the sphere
     * surface plus the height of the plane above the center of the
     * sphere.
     *
     * ratio = d/(d + z)
     *
     * Subsitute in the formula for z:
     *
     * ratio = d/(d + -d + sqrt(d^2 - (x^2 + y^2 - r^2))
     *
     * Simplify to:
     *
     * ratio = d/sqrt(d^2 - (x^2 + y^2 - r^2))
     *
     * Since d and r are constant, we now have a formula we can apply
     * for each (x,y) point within the sphere to give the (x',y')
     * coordinates of the point we should draw to project the image on
     * the plane to the surface of the sphere.  I subtract the
     * original (x,y) coordinates to give an offset rather than an
     * absolute coordinate, then convert that offset to the image
     * dimensions, and store the offset in a matrix the size of the
     * intersecting circle.  Drawing the lens is then a matter of:
     *
     * screen[coordinate] = image[coordinate + lens[y][x]]
     *
     */

    /* it is sufficient to generate 1/4 of the lens and reflect this
     * around; a sphere is mirrored on both the x and y axes */
    for (y = 0; y < r; y++) {
        for (x = 0; x < r; x++) {
            int ix, iy, offset, dist;
			dist = x*x + y*y - r*r;
			if(dist < 0) {
                double shift = d/sqrt(d*d - dist);
                ix = x * shift - x;
                iy = y * shift - y;
            } else {
                ix = 0;
                iy = 0;
            }
            offset = (iy * e->video_width + ix);
            priv->lens[(r - y)*priv->lens_width_i + r - x] = -offset;
            priv->lens[(r + y)*priv->lens_width_i + r + x] = offset;
            offset = (-iy * e->video_width + ix);
            priv->lens[(r + y)*priv->lens_width_i + r - x] = -offset;
            priv->lens[(r - y)*priv->lens_width_i + r + x] = offset;
        }
    }
}

static void clipmag(effect *e)
  {
  lenstv_t * priv = e->priv;
  if(priv->yi<0-(priv->lens_width_i/2)+1)
    priv->yi=0-(priv->lens_width_i/2)+1;
  if(priv->yi>=e->video_height-priv->lens_width_i/2-1)
    priv->yi=e->video_height-priv->lens_width_i/2-1;

  if(priv->xi<0-(priv->lens_width_i/2)+1)
    priv->xi=0-priv->lens_width_i/2+1;
  if(priv->xi>=e->video_width-priv->lens_width_i/2-1)
    priv->xi=e->video_width-priv->lens_width_i/2-1;
  }



static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "x",
      .long_name = TRS("X"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 1.0 },
      .val_default = { .val_f = 0.25 },
      .num_digits = 2,
    },
    {
      .name =      "y",
      .long_name = TRS("Y"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 1.0 },
      .val_default = { .val_f = 0.25 },
      .num_digits = 2,
    },
    {
      .name =      "lens_width",
      .long_name = TRS("Diameter"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 1.0 },
      .val_default = { .val_f = 0.25 },
      .num_digits = 2,
    },
    {
      .name =      "lens_zoom",
      .long_name = TRS("Zoom"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 5   },
      .val_max =     { .val_i = 200 },
      .val_default = { .val_i = 30  },
    },
    { /* */ },
  };

static const bg_parameter_info_t * get_parameters(void * data)
  {
  return parameters;
  }

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t *val)
  {
  bg_effectv_plugin_t * vp = data;
  lenstv_t * priv = vp->e->priv;
  int changed = 0;
  if(!name)
    return;
  EFFECTV_SET_PARAM_FLOAT(x);
  EFFECTV_SET_PARAM_FLOAT(y);
  EFFECTV_SET_PARAM_FLOAT(lens_width);
  EFFECTV_SET_PARAM_INT(lens_zoom);
  if(changed)
    {
    clipmag(vp->e);
    init(vp->e);
    }
  }

static void * create_lenstv()
  {
  return bg_effectv_create(lensRegister, BG_EFFECTV_COLOR_AGNOSTIC);
  }



const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_lenstv",
      .long_name = TRS("LensTV"),
      .description = TRS("LensTV - Based on Old school Demo Lens Effect. Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_lenstv,
      .destroy =   bg_effectv_destroy,
      .get_parameters =   get_parameters,
      .set_parameter =    set_parameter,
      .priority =         1,
    },
    
    .connect_input_port = bg_effectv_connect_input_port,
    
    .set_input_format = bg_effectv_set_input_format,
    .get_output_format = bg_effectv_get_output_format,

    .read_video = bg_effectv_read_video,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;


