/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * spiral.c: a 'spiraling' effect (even though it isn't really a spiral)
 *  code originally derived from quark.c; additions and changes are
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Public License.
 *
 * ---------
 *  2001/04/20
 *      The code looks to be about done.  The extra junk I add to the
 *      title bar looks ugly - I just don't know how much to take off.
 *      The TAB key no longer toggles animation: 'A' does.
 *      A lot of these comments can probably be removed/reduced for the next
 *      stable release.
 * ---------
 *  2001/04/16
 *      I've made quick adjustments to my most recent 'experimental' code so
 *      it'll compile with the latest EffecTV-BSB code.  More proper
 *      intergration will commence shortly.
 *      Animation (more accurately, automatic movement of the wave loci along
 *      a fixed path) is implemented.  I'm dissapointed by the results, though.
 *      The following temporary additions have been made to the user interface
 *      to allow testing of the animation:
 *        The window title now displays the current waveform, plus
 *        several animation parameters.  Those parameters are formatted as
 *        "(%0.2fint,%df, %dd)"; the first number is the size of the interval
 *        between steps along the closed path.  The second number is the
 *        number of frames to wait before each step.  The third number is
 *        a quick and dirty way to control wave amplitude; wave table indices
 *        are bitshifted right by that value.
 *      The TAB key toggles animation on and off; animation is off by
 *      default.
 *      INSERT/DELETE increment and decrement the step interval value,
 *      respectively.
 *
 *      HOME/END increment and decrement the # of frames between each
 *      movement of the waves' centerpoint, respectively.
 *
 *      PAGE UP/PAGE DOWN increment and decrement the amount that wave table
 *      entries are bitshifted by, respectively.
 *
 *  Recent changes in the user interface:
 *  1. Hitting space will now cycle among 8 different wave shapes.
 *      The active waveshape's name is displayed in the titlebar.
 *  2. The mouse can be used to recenter the effect.  Left-clicking
 *      moves the center of the effect (which defaults to the center
 *      of the video image) to the mouse pointer's location.  Any other
 *      mouse button will toggle mouse visibility, so the user can see
 *      where they're clicking.
 *  3. The keys '1','2','3','4' and '0' also move the effect center.
 *      '1' through '4' move the center midway between the middle of the
 *      image and each of the four corners, respectively. '0' returns
 *      the center to its default position.
 *
 *	-Sam Mertens
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gmerlin_effectv.h>
#include "utils.h"

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef M_PI
#define M_PI 3.141592653589793238462643383
#endif /* M_PI */

/* Several values must be powers of 2. The *_POWER predefines keep them so. */

#define PLANE_POWER         (4)     // 2 exp 4 = 16
#define WAVE_COUNT_POWER    (3)     // 2 exp 3 = 8
#define WAVE_LENGTH_POWER   (9)     // 2 exp 9 = 512

#define PLANES              (1 << PLANE_POWER)  // 16
#define PLANE_MASK          (PLANES - 1)
#define PLANE_MAX           (PLANES - 1)

#define WAVE_COUNT          (1 << WAVE_COUNT_POWER)   // 8
#define WAVE_MASK           (WAVE_COUNT - 1)
#define WAVE_MAX            (WAVE_COUNT - 1)

#define WAVE_LENGTH         (1 << WAVE_LENGTH_POWER) // 512
#define WAVE_LENGTH_MASK    (WAVE_LENGTH - 1)


#define WAVE_CONCENTRIC_A       0
#define WAVE_SAWTOOTH_UP        1
#define WAVE_SAWTOOTH_DOWN      2 
#define WAVE_TRIANGLE           3

#define WAVE_SINUS              4
#define WAVE_CONCENTRIC_B       5
#define WAVE_LENS               6
#define WAVE_FLAT               7

/* The *_OFFSET predefines are just precalculations.  There shouldn't normally
** be any need to change them.
*/

#define WAVE_CONCENTRIC_A_OFFSET    (WAVE_CONCENTRIC_A * WAVE_LENGTH)
#define WAVE_SAW_UP_OFFSET          (WAVE_SAWTOOTH_UP * WAVE_LENGTH)
#define WAVE_SAW_DOWN_OFFSET        (WAVE_SAWTOOTH_DOWN * WAVE_LENGTH)
#define WAVE_TRIANGLE_OFFSET        (WAVE_TRIANGLE * WAVE_LENGTH)

#define WAVE_CONCENTRIC_B_OFFSET    (WAVE_CONCENTRIC_B * WAVE_LENGTH)
#define WAVE_LENS_OFFSET            (WAVE_LENS * WAVE_LENGTH)
#define WAVE_SINUS_OFFSET           (WAVE_SINUS * WAVE_LENGTH)
#define WAVE_FLAT_OFFSET            (WAVE_FLAT * WAVE_LENGTH)

typedef char WaveEl;

#define WAVE_ELEMENT_SIZE       (sizeof(WaveEl))
#define WAVE_TABLE_SIZE         (WAVE_COUNT * WAVE_LENGTH * WAVE_ELEMENT_SIZE)

#define FOCUS_INCREMENT_PRESET  (M_PI/2.0)

static int start(effect * e);
static int stop(effect * e);
static int draw(effect * e, RGB32 *src, RGB32 *dest);
// static int event(SDL_Event *event);

static void spiralCreateMap(effect * e);
static WaveEl* spiralDefineWaves(effect * e);
static void spiralMoveFocus(effect* e);

// static char *effectname_base = "SpiralTV";
// static char effectname[128] = "";

typedef struct
  {
  int state;
  unsigned int *buffer;
  unsigned int *planetable[PLANES];
  int plane;
  
  int *depthmap;
  int mode;
  
  /*
  **  'g_' is for global.  I'm trying to reduce namespace pollution
  **  within the spiral module.
  **
  */
  int g_focus_x;
  int g_focus_y;
  
  float center_x;
  float center_y;
  
  WaveEl* g_wave_table;
  
  int g_toggle_xor;
  int g_animate_focus;
  int g_focus_counter;
  unsigned int g_depth_shift; // Cheesy way to adjust intensity

  // The following are needed only for this specific animation algorithm
  int g_focus_radius;
  double g_focus_degree;
  int g_focus_interval;
  double g_focus_increment;
  } spiral_t;

//  = FOCUS_INCREMENT_PRESET


// static int g_cursor_state = SDL_DISABLE;
// static int g_cursor_local = SDL_DISABLE;

    
static effect *spiralRegister(void)
{
	effect *entry;
        spiral_t * priv;
        priv = calloc(1, sizeof(*priv));
        
	entry = (effect *)calloc(1, sizeof(effect));
        
        //    strcpy(effectname, effectname_base);
	entry->name = "SpiralTV";
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
        entry->priv = priv;
        
	return entry;
}

static int start(effect * e)
  {
  int i;
  spiral_t * priv = (spiral_t*)e->priv;
  
  priv->depthmap = (int *)malloc(e->video_width * e->video_height * sizeof(int));

  priv->g_focus_x = (e->video_width/2);
  priv->g_focus_y = (e->video_height/2);
  
  if(priv->depthmap == NULL)
    {
    return 0;
    }
  
  priv->g_focus_radius = e->video_width / 2;
  /*
    ** Allocate space for the frame buffers.  A lot of memory is required -
    ** with the default settings, it totals nearly 5 megs.
    **  (320 * 240 * 4 * 16) = 4915200 bytes
    **
    ** Multiply by 4 for 640x480!
    */
  priv->buffer = (unsigned int *)malloc(e->video_area * PIXEL_SIZE * PLANES);
  if(priv->buffer == NULL)
    return 0;
  memset(priv->buffer, 0, e->video_area * PIXEL_SIZE * PLANES);
  
  /*
    ** Set up the array of pointers to the frame buffers
    */
  for(i=0;i<PLANES;i++)
    {
    priv->planetable[i] = &priv->buffer[e->video_area * i];
    }
  
    /*
    **  We call this code here, and not earlier, so that we don't
    **  waste memory unnecessarily.  (Although it's a trivial amount: 4k)
    */
    if (NULL == priv->g_wave_table)
      {
      priv->g_wave_table = spiralDefineWaves(e);
      if (NULL == priv->g_wave_table)
        {
            free(priv->buffer);
            return -1;
        }
      }
    spiralCreateMap(e);
    
    priv->plane = PLANE_MAX;
    
    priv->state = 1;
    return 0;
  }

static int stop(effect * e)
  {
  spiral_t * priv = (spiral_t*)e->priv;
  if(priv->state)
    {
    if(priv->buffer)
      free(priv->buffer);
    if(priv->depthmap)
      free(priv->depthmap);
      
    /*
    ** Prevent a small memory leak (4k)
    */
    if (NULL != priv->g_wave_table)
      {
      free(priv->g_wave_table);
      priv->g_wave_table = NULL;
      }
    
    priv->state = 0;
    }
  
  return 0;
  }

static int draw(effect * e, RGB32 *src, RGB32 *dest)
{
    int x, y, i;
	int cf;
  spiral_t * priv = (spiral_t*)e->priv;

	memcpy(priv->planetable[priv->plane], src, e->video_width * e->video_height * PIXEL_SIZE);

    if (priv->g_animate_focus)
    {
        spiralMoveFocus(e);
    }
    
	i = 0;
	for(y = 0; y < e->video_height; y++) {
		for(x = 0; x < e->video_width; x++) {
			cf = (priv->plane + priv->depthmap[i]) & PLANE_MASK;
			dest[i] = (priv->planetable[cf])[i];
			i++;
		}
	}

	priv->plane--;
	priv->plane &= PLANE_MASK;

	return 0;
}

static char const * const g_wave_names[] = { TRS("Concentric A"),
                                             TRS("Sawtooth Up"),
                                             TRS("Sawtooth Down"),
                                             TRS("Triangle"),
                                             TRS("Sinusoidal"),
                                             TRS("Concentric B"),
                                             TRS("Lens"),
                                             TRS("Flat"), (char*)0 };


static const bg_parameter_info_t parameters[] =
  {
    {
      .name = "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "Concentric A" },
      .multi_names = g_wave_names,
      .multi_labels = g_wave_names,
    },
    {
      .name = "g_focus_interval",
      .long_name = TRS("Focus interval"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min     = { .val_i = 0 },
      .val_max     = { .val_i = 60 },
      .val_default = { .val_i = 6 },
    },
    {
      .name = "center_x",
      .long_name = TRS("Center x"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_f = 0.0 },
      .val_max = { .val_f = 1.0 },
      .val_default = { .val_f = 0.5 },
      .num_digits = 3,
    },
    {
      .name = "center_y",
      .long_name = TRS("Center y"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_f = 0.0 },
      .val_max = { .val_f = 1.0 },
      .val_default = { .val_f = 0.5 },
      .num_digits = 3,
    },
    {
      .name = "g_depth_shift",
      .long_name = TRS("Depth shift"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_i = 0 },
      .val_max = { .val_i = 5 },
    },
    
    { },
  };

static const bg_parameter_info_t * get_parameters(void * data)
  {
  return parameters;
  }

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t *val)
  {
  bg_effectv_plugin_t * vp = (bg_effectv_plugin_t *)data;
  spiral_t * priv = (spiral_t*)vp->e->priv;
  int changed = 0;
  if(!name)
    return;
  EFFECTV_SET_PARAM_INT(g_focus_interval);
  EFFECTV_SET_PARAM_FLOAT(center_x);
  EFFECTV_SET_PARAM_FLOAT(center_y);
  EFFECTV_SET_PARAM_INT(g_depth_shift);

  if(!strcmp(name, "mode"))
    {
    int i = 0;
    while(g_wave_names[i])
      {
      if(!strcmp(g_wave_names[i], val->val_str))
        {
        if(priv->mode != i)
          changed = 1;
        break;
        }
      i++;
      }
    }
  if(changed)
    spiralCreateMap(vp->e);
  }


#if 0
static int event(SDL_Event *event)
{
	if(event->type == SDL_KEYDOWN) {
		switch(event->key.keysym.sym) {
		case SDLK_SPACE:
            mode++;
            mode &= WAVE_MASK;

            spiralSetName();
            spiralCreateMap();
			break;

        case SDLK_a:
            priv->g_animate_focus = ~priv->g_animate_focus;
            break;

        case SDLK_x:
            g_toggle_xor = ~g_toggle_xor;
            break;
            
        case SDLK_0:
            g_focus_y = (e->video_height/2);
            g_focus_x = (e->video_width/2);
            //            g_focus_increment = FOCUS_INCREMENT_PRESET;
            spiralCreateMap();
            spiralSetName();
            break;

        case SDLK_INSERT:
            g_focus_increment *= 1.25;
            spiralSetName();
            break;
            
        case SDLK_DELETE:
            g_focus_increment *= 0.80;
            spiralSetName();
            break;

        case SDLK_HOME:
            g_focus_interval++;
            if (60 < g_focus_interval)
            {
                g_focus_interval = 60;      // More than enough, I think
            }
            spiralSetName();
            break;
            
        case SDLK_END:
            g_focus_interval--;
            if (0 >= g_focus_interval)
            {
                g_focus_interval = 1;       // Smaller would be pointless
            }
            spiralSetName();
            break;

        case SDLK_PAGEUP:
            if (5 > g_depth_shift)
            {
                g_depth_shift++;
            }
            spiralSetName();
            spiralCreateMap();
            break;
            
        case SDLK_PAGEDOWN:
            if (0 < g_depth_shift)
            {
                g_depth_shift--;
            }
            spiralSetName();
            spiralCreateMap();
            break;
            
        case SDLK_1:
            g_focus_y = e->video_height/4;
            g_focus_x = e->video_width/4;
            spiralCreateMap();
            break;
            
        case SDLK_2:
            g_focus_y = e->video_height/4;
            g_focus_x = 3 * e->video_width/4;
            spiralCreateMap();
            break;
            
        case SDLK_3:
            g_focus_y = 3 * e->video_height/4;
            g_focus_x = e->video_width/4;
            spiralCreateMap();
            break;
            
        case SDLK_4:
            g_focus_y = 3 * e->video_height/4;
            g_focus_x = 3 * e->video_width/4;
            spiralCreateMap();
            break;
            
		default:
			break;
		}
	}
    else if (SDL_MOUSEBUTTONDOWN == event->type)
    {
        if (SDL_BUTTON_LEFT == event->button.button)
        {
            g_focus_y = event->button.y / screen_scale;
            g_focus_x = event->button.x / screen_scale;
            spiralCreateMap();
        }
        else
        {
            // Toggle the mouse pointer visibility
            if (SDL_DISABLE == g_cursor_local)
            {
                g_cursor_local = SDL_ENABLE;
            }
            else
            {
                g_cursor_local = SDL_DISABLE;
            }
            SDL_ShowCursor(g_cursor_local);
        }
    }
    
	return 0;
}

#endif


static void spiralCreateMap(effect * e)
{
    int x;
    int y;
    int rel_x;
    int rel_y;
    int yy;
    float x_ratio;
    float y_ratio;
    int v;
    int i;
    int wave_offset;
    spiral_t * priv = (spiral_t*)e->priv;
    priv->g_focus_x = (int)(priv->center_x * e->video_width);
    priv->g_focus_y = (int)(priv->center_y * e->video_height);
    
    /*
    ** The following code generates the default depth map.
    */
    i = 0;
    wave_offset = priv->mode * WAVE_LENGTH;

    x_ratio = 320.0 / e->video_width;
    y_ratio = 240.0 / e->video_height;
    
	for (y=0; y<e->video_height; y++) {

        rel_y = (priv->g_focus_y - y) * y_ratio;
        yy = rel_y * rel_y;
        
		for(x=0; x<e->video_width; x++) {
            rel_x = (priv->g_focus_x - x) * x_ratio;
#ifdef PS2
            v = ((int)sqrtf(yy + rel_x*rel_x)) & WAVE_LENGTH_MASK;
#else
            v = ((int)sqrt(yy + rel_x*rel_x)) & WAVE_LENGTH_MASK;
#endif
            priv->depthmap[i++] = priv->g_wave_table[wave_offset + v] >> priv->g_depth_shift;
		}
	}
    
    return;
}

static WaveEl* spiralDefineWaves(effect * e)
{
    WaveEl* wave_table;
    int     i;
    int     w;
    int     iw;
#ifdef PS2
    float  sinus_val = M_PI/2.0;
#else
    double  sinus_val = M_PI/2.0;
#endif
    //  spiral_t * priv = (spiral_t*)e->priv;

    // This code feels a little like a hack, but at least it contains
    // all like-minded hacks in one place.
    
  wave_table = (WaveEl*)malloc(WAVE_TABLE_SIZE);
    if (NULL == wave_table)
    {
        return NULL;
    }

    w = ((int)sqrt(e->video_height * e->video_height + e->video_width * e->video_width));
    for (i = 0; i < WAVE_LENGTH; i++)
    {
        // The 'flat' wave is very easy to compute :)
        wave_table[WAVE_FLAT_OFFSET + i] = 0;
        
        wave_table[WAVE_SAW_UP_OFFSET + i] = i & PLANE_MASK;
        wave_table[WAVE_SAW_DOWN_OFFSET + i] = PLANE_MAX - (i & PLANE_MASK);
        if (i & PLANES)
        {
            wave_table[WAVE_TRIANGLE_OFFSET + i] = (~i) & PLANE_MASK;
        }
        else
        {
            wave_table[WAVE_TRIANGLE_OFFSET + i] = i & PLANE_MASK;
        }

        iw = i / (w/(PLANES*2));

        if (iw & PLANES)
        {
            wave_table[WAVE_CONCENTRIC_A_OFFSET + i] = (~iw) & PLANE_MASK;
        }
        else
        {
            wave_table[WAVE_CONCENTRIC_A_OFFSET + i] = iw & PLANE_MASK;
        }

        wave_table[WAVE_CONCENTRIC_B_OFFSET + i] = (i*PLANES)/w;
        
        wave_table[WAVE_LENS_OFFSET + i] = i >> 3;

#ifdef PS2
        wave_table[WAVE_SINUS_OFFSET + i] = ((PLANES/2) +
                                             (int)((PLANES/2 - 1) * sinf(sinus_val))) & PLANE_MASK;
#else
        wave_table[WAVE_SINUS_OFFSET + i] = ((PLANES/2) +
                                             (int)((PLANES/2 - 1) * sin(sinus_val))) & PLANE_MASK;
#endif
        sinus_val += M_PI/PLANES;
    }
    
    return (wave_table);
}

static void spiralMoveFocus(effect * e)
{
  spiral_t * priv = (spiral_t*)e->priv;
  priv->g_focus_counter++;

    //  We'll only switch maps every X frames.
    if (priv->g_focus_interval <= priv->g_focus_counter)
    {
        priv->g_focus_counter = 0;

        priv->g_focus_x = (priv->g_focus_radius * cos(priv->g_focus_degree)) + (e->video_width/2);
        
        priv->g_focus_y = (priv->g_focus_radius * sin(priv->g_focus_degree*2.0)) + (e->video_height/2);

        spiralCreateMap(e);
        priv->g_focus_degree += priv->g_focus_increment;
        if ((2.0*M_PI) <= priv->g_focus_degree)
        {
            priv->g_focus_degree -= (2.0*M_PI);
        }
    }
    return;
}

static void * create_1dtv()
  {
  return bg_effectv_create(spiralRegister, 0);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_spiraltv",
      .long_name = TRS("SpiralTV"),
      .description = TRS("I admit that 'SpiralTV' is a misnomer; it doesn't actually spiral. What it does do is segment the screen image into a series of concentric circles, each of which is slightly out of phase (timewise) from its neighbors. Or to put it more simply, it really messes with changing (i.e. Moving) objects onscreen! Ported from EffecTV (http://effectv.sourceforge.net)."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_1dtv,
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
