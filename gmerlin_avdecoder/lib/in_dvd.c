/*****************************************************************
 
  in_dvd.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/* DVD input module (inspired by tcvp, vlc and MPlayer) */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avdec_private.h>

#if defined HAVE_CDIO && defined HAVE_DVDREAD

#include <cdio/cdio.h>
#include <cdio/device.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <dvdread/nav_read.h>
#include <dvdread/nav_print.h>

#define CELL_START 1
#define CELL_LOOP  2
#define BLOCK_LOOP 3

#ifndef PCI_START_BYTE
#define PCI_START_BYTE 45
#endif

extern bgav_demuxer_t bgav_demuxer_mpegps;

typedef struct
  {
  int title;
  int chapter;
  int angle;
  
  /* Start and and cells are according to the
     pgc. The angle offsets still have to be added */
  int start_cell;
  int end_cell;
  } track_priv_t;

typedef struct
  {
  dvd_reader_t * dvd_reader;
  dvd_file_t * dvd_file;

  ifo_handle_t * vmg_ifo;
  ifo_handle_t * vts_ifo;

  int current_vts;

  pgc_t * pgc;

  track_priv_t * current_track_priv;
  
  int state;

  int start_sector;
  int cell, next_cell, pack, npack;
  int blocks;

  int64_t last_vobu_end_pts;
  
  } dvd_t;


static void open_vts(dvd_t * dvd, int vts_index, int open_file)
  {
  if(vts_index == dvd->current_vts)
    {
    if(!dvd->dvd_file && open_file)
      dvd->dvd_file = DVDOpenFile(dvd->dvd_reader, vts_index, DVD_READ_TITLE_VOBS);
    return;
    }
  if(dvd->vts_ifo)
    ifoClose(dvd->vts_ifo);

  if(dvd->dvd_file)
    DVDCloseFile(dvd->dvd_file);
  if(open_file)
    dvd->dvd_file = DVDOpenFile(dvd->dvd_reader, vts_index, DVD_READ_TITLE_VOBS);
  
  dvd->vts_ifo = ifoOpen(dvd->dvd_reader, vts_index);
  dvd->current_vts = vts_index;
  }

static gavl_time_t convert_time(dvd_time_t * time)
  {
  gavl_time_t ret;
  int hours, minutes, seconds;

  hours   = (time->hour   >> 4) * 10 + (time->hour   & 0x0f);
  minutes = (time->minute >> 4) * 10 + (time->minute & 0x0f);
  seconds = (time->second >> 4) * 10 + (time->second & 0x0f);

  ret = seconds + 60 * (minutes + 60 * hours);
  ret *= GAVL_TIME_SCALE;
  
  switch((time->frame_u & 0xc0) >> 6)
    {
    case 1:
      ret += gavl_frames_to_time(25, 1, time->frame_u & 0x3f);
      break;
    case 3:
      ret += gavl_frames_to_time(30000, 1001, time->frame_u & 0x3f);
      break;
    }
  
  return ret;
  }

static void setup_track(bgav_input_context_t * ctx,
                        int title, int chapter, int angle)
  {
  audio_attr_t * audio_attr;
  int i;
  int audio_position;
  bgav_stream_t * s;
  bgav_track_t * new_track;
  tt_srpt_t *ttsrpt;
  pgc_t * pgc;
  vts_ptt_srpt_t *vts_ptt_srpt;
  int ttn, pgn;
  int pgc_id;
  track_priv_t * track_priv;
  const char * language_3cc;
  char language_2cc[3];
  dvd_t * dvd = (dvd_t*)(ctx->priv);
  ttsrpt = dvd->vmg_ifo->tt_srpt;

  new_track = bgav_track_table_append_track(ctx->tt);
  track_priv = calloc(1, sizeof(*track_priv));
  track_priv->title = title;
  track_priv->chapter = chapter;
  track_priv->angle = angle;

  new_track->priv = track_priv;

  /* Open VTS */
  open_vts(dvd, ttsrpt->title[title].title_set_nr, 0);

  /* Get the program chain for this track/chapter */
  ttn = ttsrpt->title[title].vts_ttn;
  vts_ptt_srpt = dvd->vts_ifo->vts_ptt_srpt;
  pgc_id = vts_ptt_srpt->title[ttn - 1].ptt[chapter].pgcn;
  pgc = dvd->vts_ifo->vts_pgcit->pgci_srp[pgc_id - 1].pgc;
  pgn = vts_ptt_srpt->title[ttn - 1].ptt[track_priv->chapter].pgn;
  
  track_priv->start_cell = pgc->program_map[pgn - 1] - 1;
  
  /* Get name */
  if(ctx->opt->dvd_chapters_as_tracks)
    {
    if(ttsrpt->title[title].nr_of_angles > 1)
      new_track->name = bgav_sprintf("Title %02d Angle %d Chapter %02d",
                                     title+1, angle+1, chapter+1);
    else
      new_track->name = bgav_sprintf("Title %02d Chapter %02d",
                                     title+1, chapter+1);

    if(chapter < ttsrpt->title[title].nr_of_ptts-1)
      {
      /* Next chapter is in the same program chain */
      if(vts_ptt_srpt->title[ttn - 1].ptt[chapter].pgcn ==
         vts_ptt_srpt->title[ttn - 1].ptt[chapter+1].pgcn)
        {
        track_priv->end_cell =
          pgc->program_map[vts_ptt_srpt->title[ttn - 1].ptt[chapter+1].pgn-1]-1;
        }
      else
        track_priv->end_cell = pgc->nr_of_cells;
      }
    else
      track_priv->end_cell = pgc->nr_of_cells;
    }
  else
    {
    if(ttsrpt->title[title].nr_of_angles > 1)
      new_track->name = bgav_sprintf("Title %02d Angle %d",
                                     title+1, angle+1);
    else
      new_track->name = bgav_sprintf("Title %02d", title+1);
    track_priv->end_cell = pgc->nr_of_cells;
    }

  /* Get duration */
  new_track->duration = 0;
  i = track_priv->start_cell;

  while(i < track_priv->end_cell)
    {
    if(pgc->cell_playback[i].block_type == BLOCK_TYPE_ANGLE_BLOCK)
      {
      new_track->duration += convert_time(&(pgc->cell_playback[i+angle].playback_time));

      while(pgc->cell_playback[i].block_mode != BLOCK_MODE_LAST_CELL)
        i++;

      i++;
      }
    else
      {
      new_track->duration += convert_time(&(pgc->cell_playback[i].playback_time));
      i++;
      }
    }
  //  fprintf(stderr, "Name: %s, start_cell: %d, end_cell: %d\n", new_track->name,
  //          start_cell, end_cell);
  
  /* Setup streams */
  s = bgav_track_add_video_stream(new_track);
  s->fourcc = BGAV_MK_FOURCC('m', 'p', 'g', 'v');
  s->stream_id = 0xE0;
  s->timescale = 90000;
  
  /* Audio streams */
  for(i = 0; i < dvd->vts_ifo->vtsi_mat->nr_of_vts_audio_streams; i++)
    {
    if(!(pgc->audio_control[i] & 0x8000))
      continue;

    audio_position = (pgc->audio_control[i] & 0x7F00 ) >> 8;
    
    s = bgav_track_add_audio_stream(new_track);
    s->timescale = 90000;
    
    audio_attr = &dvd->vts_ifo->vtsi_mat->vts_audio_attr[i];

    switch(audio_attr->audio_format)
      {
      case 0:
        //        printf("ac3 ");
        s->fourcc = BGAV_MK_FOURCC('.', 'a', 'c', '3');
        s->stream_id = 0xbd80 + audio_position;
        break;
      case 2:
        //        printf("mpeg1 ");
        s->fourcc = BGAV_MK_FOURCC('.', 'm', 'p', '3');
        s->stream_id = 0xc0 + audio_position;
        break;
      case 3:
        //        printf("mpeg2ext ");
        /* Unsupported */
        s->fourcc = BGAV_MK_FOURCC('m', 'p', 'a', 'e');
        break;
      case 4:
        //        printf("lpcm ");
        s->fourcc = BGAV_MK_FOURCC('l', 'p', 'c', 'm');
        s->stream_id = 0xbda0 + audio_position;
        
        break;
      case 6:
        //        printf("dts ");
        s->fourcc = BGAV_MK_FOURCC('d', 't', 's', ' ');
        s->stream_id = 0xbd88 + audio_position;
        break;
      default:
        //        printf("(please send a bug report) ");
        break;
      }
    /* Set language */
    if(audio_attr->lang_type == 1)
      {
      language_2cc[0] = audio_attr->lang_code >> 8;
      language_2cc[1] = audio_attr->lang_code & 0xff;
      language_2cc[2] = '\0';
      language_3cc = bgav_lang_from_twocc(language_2cc);
      strcpy(s->language, language_3cc);
      }

    /* Set description */

    switch(audio_attr->code_extension)
      {
      case 0:
        s->info = bgav_strndup("Unspecified", (char*)0);
        break;
      case 1:
        s->info = bgav_strndup("Audio stream", (char*)0);
        break;
      case 2:
        s->info = bgav_strndup("Audio for visually impaired", (char*)0);
        break;
      case 3:
        s->info = bgav_strndup("Director's comments 1", (char*)0);
        break;
      case 4:
        s->info = bgav_strndup("Director's comments 2", (char*)0);
        break;
      }
    }

  /* Subtitle streams */
  for(i = 0; i < dvd->vts_ifo->vtsi_mat->nr_of_vts_subp_streams; i++)
    {
    if(!(pgc->subp_control[i] & 0x80000000))
      continue;

    //    fprintf(stderr, "Got subtitle stream\n");
    }

  /* Duration */
  
  }

static void dump_vmg_ifo(ifo_handle_t * vmg_ifo)
  {
  fprintf(stderr, "VMG IFO:\n");
  fprintf(stderr, "  Title sets: %d\n",
          vmg_ifo->vmgi_mat->vmg_nr_of_title_sets);
  }

static int open_dvd(bgav_input_context_t * ctx, const char * url)
  {
  int i, j, k;
  dvd_t * priv;
  tt_srpt_t *ttsrpt;
  fprintf(stderr, "OPEN DVD\n");

  //  DVDInit();
  
  priv = calloc(1, sizeof(*priv));
  
  ctx->priv = priv;
  
  /* Try to open dvd */
  priv->dvd_reader = DVDOpen(url);
  if(!priv->dvd_reader)
    {
    fprintf(stderr, "DVDOpen failed\n");
    return 0;
    }

  /* Open Video Manager */

  priv->vmg_ifo = ifoOpen(priv->dvd_reader, 0);
  if(!priv->vmg_ifo)
    {
    fprintf(stderr, "ifoOpen failed\n");
    return 0;
    }
  dump_vmg_ifo(priv->vmg_ifo);
  
  ttsrpt = priv->vmg_ifo->tt_srpt;
  //  fprintf(stderr, "

  /* Create track table */
  ctx->tt = bgav_track_table_create(0);

  for(i = 0; i < ttsrpt->nr_of_srpts; i++)
    {
    for(j = 0; j < ttsrpt->title[i].nr_of_angles; j++)
      {
      if(ctx->opt->dvd_chapters_as_tracks)
        {
        /* Add individual chapters as tracks */
        for(k = 0; k < ttsrpt->title[i].nr_of_ptts; k++)
          {
          setup_track(ctx, i, k, j);
          }
        }
      else
        {
        /* Add entire titles as tracks */
        setup_track(ctx, i, 0, j);
        }
      }
    }
  
  /* Create demuxer */
  
  ctx->demuxer = bgav_demuxer_create(ctx->opt, &bgav_demuxer_mpegps, ctx);
  ctx->demuxer->tt = ctx->tt;

  /* We must set this here, because we don't initialize the demuxer here */
  ctx->demuxer->can_seek = 1;
  
  ctx->sector_size        = 2048;
  ctx->sector_size_raw    = 2048;
  ctx->sector_header_size = 0;
  
  return 1;
  }

static int
next_cell(pgc_t *pgc, int cell, int angle)
  {
  int ret;
  ret = cell;

  /* If we are in an angle block right now, seek the last cell */
  if(pgc->cell_playback[ret].block_type == BLOCK_TYPE_ANGLE_BLOCK)
    {
    while(pgc->cell_playback[ret].block_mode != BLOCK_MODE_LAST_CELL)
      ret++;
    }

  /* Advance by one cell */
  ret++;

  /* If we entered an angle block, go to the right angle */
  if(pgc->cell_playback[ret].block_type == BLOCK_TYPE_ANGLE_BLOCK)
    {
    ret += angle;
    }
  
  return ret;
  }

static int
is_nav_pack(uint8_t *buffer)
  {
  return buffer[41] == 0xbf && buffer[1027] == 0xbf;
  }

static int
read_nav(bgav_input_context_t * ctx, int sector, int *next)
  {
  uint8_t buf[DVD_VIDEO_LB_LEN];
  dsi_t dsi_pack;
  pci_t pci_pack;
  int blocks;
  dvd_t * d;

  d = (dvd_t*)ctx->priv;
  
  if(DVDReadBlocks(d->dvd_file, sector, 1, buf) != 1)
    {
    fprintf(stderr, "DVD: error reading NAV packet @%i\n", sector);
    return -1;
    }
  
  if(!is_nav_pack(buf))
    return -1;

  //  printf("*** Got nav pack ***\n");
  
  navRead_DSI(&dsi_pack, buf + DSI_START_BYTE);
  navRead_PCI(&pci_pack, buf + PCI_START_BYTE);

  //  printf("DSI\n");
  //  navPrint_DSI(&dsi_pack);
  //  printf("PCI\n");
  //  navPrint_PCI(&pci_pack);
  
  //  fprintf(stderr, "PCI timestamps: %d -> %d\n",
  //          pci_pack.pci_gi.vobu_s_ptm, pci_pack.pci_gi.vobu_e_ptm);

  if(d->last_vobu_end_pts >= 0)
    {
    if(d->last_vobu_end_pts != pci_pack.pci_gi.vobu_s_ptm)
      {
      fprintf(stderr, "** Detected PTS discontinuity: %d -> %d (Diff: %d)\n",
              (int)d->last_vobu_end_pts, (int)pci_pack.pci_gi.vobu_s_ptm,
              (int)(d->last_vobu_end_pts - pci_pack.pci_gi.vobu_s_ptm));
      if(d->last_vobu_end_pts >= 0)
        ctx->demuxer->timestamp_offset += d->last_vobu_end_pts - pci_pack.pci_gi.vobu_s_ptm;
      else
        ctx->demuxer->timestamp_offset = d->last_vobu_end_pts - pci_pack.pci_gi.vobu_s_ptm;
      }
    }
  else
    {
    ctx->demuxer->timestamp_offset = -((int64_t)pci_pack.pci_gi.vobu_s_ptm);
    }
  ctx->demuxer->have_timestamp_offset = 1;
  
  d->last_vobu_end_pts = pci_pack.pci_gi.vobu_e_ptm;
    
  blocks = dsi_pack.dsi_gi.vobu_ea;
  
  if(dsi_pack.vobu_sri.next_vobu != SRI_END_OF_CELL)
    {
    *next = sector + (dsi_pack.vobu_sri.next_vobu & 0x7fffffff);
    }
  else
    {
    *next = sector + blocks + 1;
    }
  return blocks;
  }

static int read_sector_dvd(bgav_input_context_t * ctx, uint8_t * data)
  {
  dvd_t * d;
  int l;
  d = (dvd_t*)(ctx->priv);
  
  switch(d->state)
    {
    case CELL_START:
      /* TODO: EOF at chapter boundaries */
      if(d->next_cell >= d->current_track_priv->end_cell)
        return 0;
      d->cell = d->next_cell;
      fprintf(stderr, "DVD: entering cell %i\n", d->cell+1);
      d->next_cell = next_cell(d->pgc, d->cell, d->current_track_priv->angle);
      d->npack = d->pgc->cell_playback[d->cell].first_sector;
      d->state = CELL_LOOP;
      
    case CELL_LOOP:
      d->pack = d->npack;
      l = read_nav(ctx, d->pack, &d->npack);
      if(l < 0)
        return -1;
      //      fprintf(stderr, "DVD: cell %i, %i blocks @%i\n", d->cell+1, l, d->pack);
      d->blocks = l;
      d->pack++;
      d->state = BLOCK_LOOP;
      
    case BLOCK_LOOP:
      /* 	    fprintf(stderr, "DVD: reading %i blocks @%i -> %p\n", */
      /* 		    r, d->pack, d->buf); */
      l = DVDReadBlocks(d->dvd_file, d->pack, 1, data);
      if(l < 1)
        {
        fprintf(stderr, "DVD: error reading blocks @%i\n", d->pack);
        return -1;
        }
      d->blocks -= l;
      
      if(!d->blocks)
        {
        if(d->pack < d->pgc->cell_playback[d->cell].last_sector)
          d->state = CELL_LOOP;
        else
          d->state = CELL_START;
        }
      else
        {
        d->pack += l;
	}
    }
  
  return 1;
  }

static void    close_dvd(bgav_input_context_t * ctx)
  {
  dvd_t * dvd;
  //  fprintf(stderr, "CLOSE DVD\n");
  dvd = (dvd_t*)(ctx->priv);

  //  if(dvd->dvd_file)

  if(dvd->dvd_reader)
    DVDClose(dvd->dvd_reader);
  
  if(dvd->dvd_file)
    DVDCloseFile(dvd->dvd_file);
  
  if(dvd->vmg_ifo)
    ifoClose(dvd->vmg_ifo);
  
  if(dvd->vts_ifo)
    ifoClose(dvd->vts_ifo);
  
  free(dvd);
  return;
  }

static void select_track_dvd(bgav_input_context_t * ctx, int track)
  {
  dvd_t * dvd;
  int ttn, pgn;
  int pgc_id;
  tt_srpt_t *ttsrpt;
  track_priv_t * track_priv;
  vts_ptt_srpt_t *vts_ptt_srpt;
  
  dvd = (dvd_t*)(ctx->priv);
  dvd->last_vobu_end_pts = -1;
  ctx->demuxer->have_timestamp_offset = 0;
  
  ttsrpt = dvd->vmg_ifo->tt_srpt;
  track_priv = (track_priv_t*)(ctx->tt->current_track->priv);

  dvd->current_track_priv = track_priv;
  
  ttn = ttsrpt->title[track_priv->title].vts_ttn;

  /* Open VTS */
  open_vts(dvd, ttsrpt->title[track_priv->title].title_set_nr, 1);
  
  vts_ptt_srpt = dvd->vts_ifo->vts_ptt_srpt;
  pgc_id = vts_ptt_srpt->title[ttn - 1].ptt[track_priv->chapter].pgcn;
  pgn    = vts_ptt_srpt->title[ttn - 1].ptt[track_priv->chapter].pgn;

  
  dvd->pgc = dvd->vts_ifo->vts_pgcit->pgci_srp[pgc_id - 1].pgc;

  dvd->next_cell = dvd->pgc->program_map[pgn - 1] - 1;

  /* Enter the right angle */
  if(dvd->pgc->cell_playback[dvd->next_cell].block_type == BLOCK_TYPE_ANGLE_BLOCK ) 
    dvd->next_cell += dvd->current_track_priv->angle;
  
  
  dvd->state = CELL_START;
  dvd->start_sector = dvd->pgc->cell_playback[dvd->next_cell].first_sector;
  
  fprintf(stderr, "Select track: t: %d, c: %d, pgc_id: %d, pgn: %d, start_sector: %d\n",
          track_priv->title, track_priv->chapter, pgc_id, pgn, dvd->start_sector);
  
  }

static void seek_time_dvd(bgav_input_context_t * ctx, gavl_time_t t)
  {
  int i;
  uint8_t buf[DVD_VIDEO_LB_LEN];
  dsi_t dsi_pack;
  pci_t pci_pack;
  uint32_t next_vobu_offset;
  gavl_time_t time, diff_time, cell_start_time;
  dvd_t * dvd;
  int64_t time_scaled;
  dvd = (dvd_t*)(ctx->priv);

  time = 0;
  cell_start_time = 0;
  
  /* Get entry cell */
  dvd->cell = dvd->current_track_priv->start_cell;

  fprintf(stderr, "DVD Seek time\n");
  
  while(1)
    {
    /* If we are in an angle block, go to the right angle */
    if(dvd->pgc->cell_playback[dvd->cell].block_type == BLOCK_TYPE_ANGLE_BLOCK ) 
      dvd->cell += dvd->current_track_priv->angle;
    
    diff_time = convert_time(&(dvd->pgc->cell_playback[dvd->cell].playback_time));

    if(cell_start_time + diff_time > t)
      break;

    cell_start_time += diff_time;
    
    /* If we are in an angle block right now, seek the last cell */
    if(dvd->pgc->cell_playback[dvd->cell].block_type == BLOCK_TYPE_ANGLE_BLOCK)
      {
      while(dvd->pgc->cell_playback[dvd->cell].block_mode != BLOCK_MODE_LAST_CELL)
        dvd->cell++;
      }
#if 0
    fprintf(stderr, "Skipping cell (time: %f, cell_start: %f, diff: %f)\n",
            gavl_time_to_seconds(t),
            gavl_time_to_seconds(cell_start_time),
            gavl_time_to_seconds(diff_time));
#endif       
    /* Advance by one */
    dvd->cell++;
    
    }

  /* Get the next cell, so we won't get screwed up later on */
  
  dvd->next_cell = next_cell(dvd->pgc, dvd->cell, dvd->current_track_priv->angle);

  if(dvd->cell >= dvd->current_track_priv->end_cell)
    {
    fprintf(stderr, "Hit EOF during seek (%d >= %d)\n", dvd->cell, dvd->current_track_priv->end_cell);
    ctx->demuxer->eof = 1;
    return;
    }
  /* Now, seek forward using the seek information in the dsi packets */
  dvd->npack = dvd->pgc->cell_playback[dvd->cell].first_sector;

  time = cell_start_time;
  
  while(1)
    {
    if(DVDReadBlocks(dvd->dvd_file, dvd->npack, 1, buf) != 1)
      {
      fprintf(stderr, "DVD: error reading NAV packet @%i\n", dvd->npack);
      return;
      }
    if(!is_nav_pack(buf))
      {
      printf("*** Got NO nav pack ***\n");
      return;
      }
    //    printf("*** Got nav pack ***\n");
    
    navRead_DSI(&dsi_pack, buf + DSI_START_BYTE);
    navRead_PCI(&pci_pack, buf + PCI_START_BYTE);
#if 0
    printf("DSI:\n");
    navPrint_DSI(&dsi_pack);
    printf("PCI:\n");
    navPrint_PCI(&pci_pack);
#endif
    time = cell_start_time + convert_time(&(pci_pack.pci_gi.e_eltm));

    /* Now, decide on the next VOBU to go to. We don't use the .5 second steps */
    
    if(t - time > 120 * GAVL_TIME_SCALE)
      next_vobu_offset = dsi_pack.vobu_sri.fwda[0];
    else if(t - time > 60 * GAVL_TIME_SCALE)
      next_vobu_offset = dsi_pack.vobu_sri.fwda[1];
    else if(t - time > 30 * GAVL_TIME_SCALE)
      next_vobu_offset = dsi_pack.vobu_sri.fwda[2];
    else if(t - time > 10 * GAVL_TIME_SCALE)
      next_vobu_offset = dsi_pack.vobu_sri.fwda[3];
    else if(t - time > 7 * GAVL_TIME_SCALE)
      next_vobu_offset = dsi_pack.vobu_sri.fwda[5];
    else if(t - time > 6 * GAVL_TIME_SCALE)
      next_vobu_offset = dsi_pack.vobu_sri.fwda[7];
    else if(t - time > 5 * GAVL_TIME_SCALE)
      next_vobu_offset = dsi_pack.vobu_sri.fwda[9];
    else if(t - time > 4 * GAVL_TIME_SCALE)
      next_vobu_offset = dsi_pack.vobu_sri.fwda[11];
    else if(t - time > 3 * GAVL_TIME_SCALE)
      next_vobu_offset = dsi_pack.vobu_sri.fwda[13];
    else if(t - time > 2 * GAVL_TIME_SCALE)
      next_vobu_offset = dsi_pack.vobu_sri.fwda[15];
    else if(t - time > 1 * GAVL_TIME_SCALE)
      next_vobu_offset = dsi_pack.vobu_sri.fwda[17];
    else
      break;

    if(!(next_vobu_offset & 0x80000000))
      break;
    
    next_vobu_offset &= 0x3fffffff;
    dvd->npack += next_vobu_offset;
    }
  dvd->state = CELL_LOOP;

  time_scaled = gavl_time_scale(90000, time);
  
  //  fprintf(stderr, "Seeked %f -> %f (%lld), start_pts: %f\n",
  //          gavl_time_to_seconds(t),
  //          gavl_time_to_seconds(time), time_scaled,
  //          pci_pack.pci_gi.vobu_s_ptm / 90000.0);
  
  
  for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
    ctx->tt->current_track->audio_streams[i].time_scaled = time_scaled;
  for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
    ctx->tt->current_track->video_streams[i].time_scaled = time_scaled;
  for(i = 0; i < ctx->tt->current_track->num_subtitle_streams; i++)
    ctx->tt->current_track->subtitle_streams[i].time_scaled = time_scaled;
  

  ctx->demuxer->timestamp_offset = time_scaled - (int64_t)pci_pack.pci_gi.vobu_s_ptm;
  dvd->last_vobu_end_pts = pci_pack.pci_gi.vobu_s_ptm;
  
  fprintf(stderr, "ctx->demuxer->timestamp_offset: %f - %f = %f\n",
          time_scaled / 90000.0, pci_pack.pci_gi.vobu_s_ptm / 90000.0,
          ctx->demuxer->timestamp_offset / 90000.0);
  }

bgav_input_t bgav_input_dvd =
  {
    name:          "dvd",
    open:          open_dvd,
    read_sector:   read_sector_dvd,
    seek_time:     seek_time_dvd,
    close:         close_dvd,
    select_track:  select_track_dvd,
  };

static char * get_device_name(CdIo_t * cdio,
                              cdio_drive_read_cap_t  read_cap,
                              cdio_drive_write_cap_t write_cap,
                              const char * device)
  {
  cdio_hwinfo_t driveid;

  if(cdio_get_hwinfo(cdio, &driveid) &&
     (driveid.psz_model[0] != '\0'))
    {
    return bgav_sprintf("%s %s", driveid.psz_vendor, driveid.psz_model);
    }
  
  if(write_cap & CDIO_DRIVE_CAP_WRITE_DVD_R)
    {
    return bgav_sprintf("DVD Writer (%s)", device);
    }
  else if(write_cap & CDIO_DRIVE_CAP_WRITE_CD_R)
    {
    return bgav_sprintf("CD Writer (%s)", device);
    }
  else if(read_cap & CDIO_DRIVE_CAP_READ_DVD_ROM)
    {
    return bgav_sprintf("DVD Drive (%s)", device);
    }
  return bgav_sprintf("CDrom Drive (%s)", device);
  }

int bgav_check_device_dvd(const char * device, char ** name)
  {
  CdIo_t * cdio;
  cdio_drive_read_cap_t  read_cap;
  cdio_drive_write_cap_t write_cap;
  cdio_drive_misc_cap_t  misc_cap;

  cdio = cdio_open (device, DRIVER_DEVICE);
  if(!cdio)
    return 0;
  
  cdio_get_drive_cap(cdio, &read_cap, &write_cap, &misc_cap);

  if(!(read_cap & CDIO_DRIVE_CAP_READ_DVD_ROM))
    {
    cdio_destroy(cdio);
    return 0;
    }
  
  /* Seems the drive is ok */

  if(name)
    *name = get_device_name(cdio, read_cap, write_cap, device);
  cdio_destroy(cdio);
  return 1;
  }

bgav_device_info_t * bgav_find_devices_dvd()
  {
  int i;
  char * device_name;
  char ** devices;
  bgav_device_info_t * ret = (bgav_device_info_t *)0;

  devices = cdio_get_devices(DRIVER_DEVICE);

  if(!devices)
    return 0;
    
  i = 0;
  while(devices[i])
    {
    //    fprintf(stderr, "Checking %s\n", devices[i]);
    device_name = (char*)0;
    if(bgav_check_device_dvd(devices[i], &device_name))
      {
      ret = bgav_device_info_append(ret,
                                  devices[i],
                                  device_name);
      if(device_name)
        free(device_name);
      }
    i++;
    }
  cdio_free_device_list(devices);
  return ret;

  }

static
bgav_input_context_t * bgav_input_open_dvd(const char * device,
                                           bgav_options_t * opt)
  {
  bgav_input_context_t * ret = (bgav_input_context_t *)0;
  ret = bgav_input_create(opt);
  ret->input = &bgav_input_dvd;
  if(!ret->input->open(ret, device))
    {
    fprintf(stderr, "Cannot open DVD Device %s\n", device);
    goto fail;
    }
  return ret;
  fail:
  if(ret)
    free(ret);
  return (bgav_input_context_t *)0;
  }

int bgav_open_dvd(bgav_t * b, const char * device)
  {
  bgav_codecs_init();
  b->input = bgav_input_open_dvd(device, &b->opt);
  if(!b->input)
    return 0;
  if(!bgav_init(b))
    goto fail;
  return 1;
  fail:
  return 0;
  
  }


#else /* No DVD support */

int bgav_check_device_dvd(const char * device, char ** name)
  {
  fprintf(stderr, "DVD not supported (need libcdio and libdvdread)\n");
  return 0;
  }

bgav_device_info_t * bgav_find_devices_dvd()
  {
  fprintf(stderr, "DVD not supported (need libcdio and libdvdread)\n");
  return (bgav_device_info_t*)0;
  
  }

int bgav_open_dvd(bgav_t * b, const char * device)
  {
  fprintf(stderr, "DVD not supported (need libcdio and libdvdread)\n");
  return 0;
  }


#endif
