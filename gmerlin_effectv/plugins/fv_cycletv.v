/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * cycleTV - no effect.
 * Written by clifford smith <nullset@dookie.net>
 *
 */

#include <stdlib.h>
#include <string.h>
#include "EffecTV.h"
#include "utils.h"

static int start(effect*);
static int stop(effect*);
static int draw(effect*, RGB32 *src, RGB32 *dst);

static char *effectname = "cycleTV";

typedef struct
  {
  int state = 0;
  int roff,goff,boff; /* Offset values */
  } cycle_t;

static effect *cycleRegister(void)
{
	effect *entry;
        cycle_t * priv;

	entry = (effect *)calloc(1, sizeof(effect));
	if(entry == NULL) return NULL;
        priv = calloc(1, sizeof(*priv));
        entry->priv = priv;
	entry->name = effectname;
	entry->start = start;
	entry->stop = stop;
	entry->draw = draw;
	entry->event = NULL;

	return entry;
}

static int start(effect*)
{
	roff = goff = boff = 0;
	state = 1;
	return 0;
}

static int stop(effect*)
{
	state = 0;
	return 0;
}

#define NEWCOLOR(c,o) ((c+o)%230)
static int draw(effect* e, RGB32 *src, RGB32 *dst)
{
  int i;
  
  roff += 1;
  goff += 3;        
  if (stretch)
    image_stretch_to_screen();
  
  boff += 7;
  for (i=0 ; i < video_area ; i++) {
    RGB32 t;
    t = *src++;
    *dst++ = RGB(NEWCOLOR(RED(t),roff),
	       NEWCOLOR(GREEN(t),goff),
	       NEWCOLOR(BLUE(t),boff));
  }

  return 0;
}

