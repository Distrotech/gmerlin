/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

/* DVD input module (inspired by tcvp, vlc and MPlayer) */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avdec_private.h>

#define LOG_DOMAIN "in_dvd"

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

  /* Only there if all chapters are in one track */
  int num_chapters;
  struct
    {
    int start_cell;
    int end_cell;
    gavl_time_t duration;
    } * chapters;
  
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


static int open_vts(const bgav_options_t * opt,
                    dvd_t * dvd, int vts_index, int open_file)
  {
  if(vts_index == dvd->current_vts)
    {
    if(!dvd->dvd_file && open_file)
      {
      dvd->dvd_file = DVDOpenFile(dvd->dvd_reader, vts_index, DVD_READ_TITLE_VOBS);
      if(!dvd->dvd_file)
        {
        bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Opening vts %d failed", vts_index);
        return 0;
        }
      }
    return 1;
    }
  if(dvd->vts_ifo)
    ifoClose(dvd->vts_ifo);

  if(dvd->dvd_file)
    DVDCloseFile(dvd->dvd_file);
  if(open_file)
    {
    dvd->dvd_file = DVDOpenFile(dvd->dvd_reader, vts_index, DVD_READ_TITLE_VOBS);
    if(!dvd->dvd_file)
      {
      bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Opening vts %d failed", vts_index);
      return 0;
      }
    }
  dvd->vts_ifo = ifoOpen(dvd->dvd_reader, vts_index);

  if(!dvd->vts_ifo)
    {
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Opening IFO for vts %d failed", vts_index);
    return 0;
    }
  dvd->current_vts = vts_index;
  return 1;
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

static gavl_time_t get_duration(pgc_t * pgc, int start_cell, int end_cell, int angle)
  {
  int i;
  gavl_time_t ret = 0;
  i = start_cell;

  while(i < end_cell)
    {
    if(pgc->cell_playback[i].block_type == BLOCK_TYPE_ANGLE_BLOCK)
      {
      ret += convert_time(&pgc->cell_playback[i+angle].playback_time);

      while(pgc->cell_playback[i].block_mode != BLOCK_MODE_LAST_CELL)
        i++;

      i++;
      }
    else
      {
      ret += convert_time(&pgc->cell_playback[i].playback_time);
      i++;
      }
    }
  return ret;
  
  }

static const struct
  {
  int image_width;
  int image_height;
  int pixel_width_4_3;
  int pixel_height_4_3;
  int pixel_width_16_9;
  int pixel_height_16_9;
  }
frame_sizes[] =
  {
    { 720, 576, 59, 54, 118, 81 }, /* PAL  */
    { 720, 480, 10, 11, 40,  33 }, /* NTSC */
    { 352, 576, 59, 27,  0,   0 }, /* PAL CVD */
    { 352, 480, 20, 11,  0,   0 }, /* NTSC CVD */
    { 352, 288, 59, 54,  0,   0 }, /* PAL VCD */
    { 352, 480, 10, 11,  0,   0 }, /* NTSC VCD */
  };

static void guess_pixel_aspect(int width, int height, int aspect,
                               int * pixel_width, int * pixel_height)
  {
  int i;
  for(i = 0; i < sizeof(frame_sizes)/sizeof(frame_sizes[0]); i++)
    {
    if((frame_sizes[i].image_width == width) &&
       (frame_sizes[i].image_height == height))
      {
      if(aspect == 0) /* 4:3 */
        {
        *pixel_width  = frame_sizes[i].pixel_width_4_3;
        *pixel_height = frame_sizes[i].pixel_height_4_3;
        }
      else if(aspect == 3) /* 16:9 */
        {
        *pixel_width  = frame_sizes[i].pixel_width_16_9;
        *pixel_height = frame_sizes[i].pixel_height_16_9;
        }
      return;
      }
    }
  }

static int setup_track(bgav_input_context_t * ctx,
                        int title, int chapter, int angle)
  {
  int video_width = 0, video_height;
  const char * audio_codec = (const char *)0;
  audio_attr_t * audio_attr;
  video_attr_t * video_attr;
  subp_attr_t *  subp_attr;
  int i;
  int stream_position;
  bgav_stream_t * s;
  bgav_track_t * new_track;
  tt_srpt_t *ttsrpt;
  pgc_t * pgc = (pgc_t*)0;
  vts_ptt_srpt_t *vts_ptt_srpt;
  int ttn, pgn;
  int pgc_id;
  track_priv_t * track_priv;
  const char * language_3cc;
  char language_2cc[3];
  dvd_t * dvd = (dvd_t*)(ctx->priv);
  ttsrpt = dvd->vmg_ifo->tt_srpt;

  /* Open VTS */
  
  if(!open_vts(ctx->opt, dvd, ttsrpt->title[title].title_set_nr, 0))
    return 0;
  
  new_track = bgav_track_table_append_track(ctx->tt);
  track_priv = calloc(1, sizeof(*track_priv));
  track_priv->title = title;
  track_priv->chapter = chapter;
  track_priv->angle = angle;
  new_track->priv = track_priv;
  
  /* Get the program chain for this track/chapter */
  ttn = ttsrpt->title[title].vts_ttn;
  vts_ptt_srpt = dvd->vts_ifo->vts_ptt_srpt;

#if 0  
  /* Get name and chapters */
  if(ctx->opt->dvd_chapters_as_tracks)
    {
    pgc_id = vts_ptt_srpt->title[ttn - 1].ptt[chapter].pgcn;
    pgc = dvd->vts_ifo->vts_pgcit->pgci_srp[pgc_id - 1].pgc;
    
    pgn = vts_ptt_srpt->title[ttn - 1].ptt[track_priv->chapter].pgn;
    track_priv->start_cell = pgc->program_map[pgn - 1] - 1;
    
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
  /* All chapters in one track */
  else
    {
#endif
    if(ttsrpt->title[title].nr_of_angles > 1)
      new_track->name = bgav_sprintf("Title %02d Angle %d",
                                     title+1, angle+1);
    else
      new_track->name = bgav_sprintf("Title %02d", title+1);
    
    /* Set up chapters */
    track_priv->num_chapters = ttsrpt->title[title].nr_of_ptts;
    track_priv->chapters = calloc(track_priv->num_chapters,
                                  sizeof(*track_priv->chapters));
    
    for(i = 0; i < track_priv->num_chapters; i++)
      {
      pgc_id = vts_ptt_srpt->title[ttn - 1].ptt[i].pgcn;
      pgc = dvd->vts_ifo->vts_pgcit->pgci_srp[pgc_id - 1].pgc;
      
      pgn = vts_ptt_srpt->title[ttn - 1].ptt[i].pgn;
      track_priv->chapters[i].start_cell = pgc->program_map[pgn - 1] - 1;

      if(chapter < ttsrpt->title[title].nr_of_ptts-1)
        {
        /* Next chapter is in the same program chain */
        if(pgc_id == vts_ptt_srpt->title[ttn - 1].ptt[i+1].pgcn)
          {
          track_priv->chapters[i].end_cell =
            pgc->program_map[vts_ptt_srpt->title[ttn - 1].ptt[i+1].pgn-1]-1;
          }
        else
          track_priv->chapters[i].end_cell = pgc->nr_of_cells;
        }
      else
        track_priv->chapters[i].end_cell = pgc->nr_of_cells;
      track_priv->chapters[i].duration =
        get_duration(pgc, track_priv->chapters[i].start_cell,
                     track_priv->chapters[i].end_cell, angle);
      }
    
    track_priv->end_cell =
      track_priv->chapters[track_priv->num_chapters-1].end_cell;
#if 0
    }
#endif
  /* Get duration */
#if 0  

  if(ctx->opt->dvd_chapters_as_tracks)
    new_track->duration =
      get_duration(pgc, track_priv->start_cell, track_priv->end_cell, angle);
  else
    {
#endif
    new_track->chapter_list =
      bgav_chapter_list_create(GAVL_TIME_SCALE, track_priv->num_chapters);
    new_track->duration = 0;
    for(i = 0; i < track_priv->num_chapters; i++)
      {
      if(i)
        new_track->chapter_list->chapters[i].time =
          new_track->chapter_list->chapters[i-1].time +
          track_priv->chapters[i-1].duration;
      new_track->duration += track_priv->chapters[i].duration;
      }
#if 0  
    }
#endif
  
  /* Setup streams */

  pgc_id = vts_ptt_srpt->title[ttn - 1].ptt[chapter].pgcn;
  pgc = dvd->vts_ifo->vts_pgcit->pgci_srp[pgc_id - 1].pgc;
  
  s = bgav_track_add_video_stream(new_track, ctx->opt);
  s->fourcc = BGAV_MK_FOURCC('m', 'p', 'g', 'v');
  s->stream_id = 0xE0;
  s->timescale = 90000;
  s->flags  |= STREAM_PARSE_FULL;
  video_attr = &dvd->vts_ifo->vtsi_mat->vts_video_attr;

  video_height = 480;
  if(video_attr->video_format != 0) 
    video_height = 576;

  switch(video_attr->picture_size)
    {
    case 0:
      video_width = 720;
      break;
    case 1:
      video_width = 704;
      break;
    case 2:
      video_width = 352;
      break;
    case 3:
      video_width = 352;
      video_height /= 2;
      break;      
    }
  
  guess_pixel_aspect(video_width, video_height,
                     video_attr->display_aspect_ratio,
                     &s->data.video.format.pixel_width,
                     &s->data.video.format.pixel_height);
#if 0
  /* Check if we have a still image. This is a bit lousy for now,
     we just check for a nonzero still time at the start cell of
     each track (which can be either a title or a chaptzer */

  if(pgc->cell_playback[track_priv->start_cell].still_time != 0)
    {
    bgav_log(ctx->opt, BGAV_LOG_INFO, LOG_DOMAIN, "Detected still cell");
    }
#endif
  
  /* Audio streams */
  for(i = 0; i < dvd->vts_ifo->vtsi_mat->nr_of_vts_audio_streams; i++)
    {
    if(!(pgc->audio_control[i] & 0x8000))
      continue;

    stream_position = (pgc->audio_control[i] & 0x7F00 ) >> 8;
    
    s = bgav_track_add_audio_stream(new_track, ctx->opt);
    s->timescale = 90000;
    
    audio_attr = &dvd->vts_ifo->vtsi_mat->vts_audio_attr[i];

    switch(audio_attr->audio_format)
      {
      case 0:
        //        printf("ac3 ");
        s->fourcc = BGAV_MK_FOURCC('.', 'a', 'c', '3');
        audio_codec = "AC3";
        s->stream_id = 0xbd80 + stream_position;
        s->flags |= STREAM_PARSE_FULL;
        break;
      case 2:
        //        printf("mpeg1 ");
        s->fourcc = BGAV_MK_FOURCC('.', 'm', 'p', '2');
        s->stream_id = 0xc0 + stream_position;
        s->flags |= STREAM_PARSE_FULL;
        audio_codec = "MPA";
        break;
      case 3:
        //        printf("mpeg2ext ");
        /* Unsupported */
        s->fourcc = BGAV_MK_FOURCC('m', 'p', 'a', 'e');
        audio_codec = "MPAext";
        break;
      case 4:
        //        printf("lpcm ");
        s->fourcc = BGAV_MK_FOURCC('L', 'P', 'C', 'M');
        s->stream_id = 0xbda0 + stream_position;
        audio_codec = "LPCM";
        
        break;
      case 6:
        //        printf("dts ");
        s->fourcc = BGAV_MK_FOURCC('d', 't', 's', ' ');
        s->stream_id = 0xbd88 + stream_position;
        s->flags |= STREAM_PARSE_FULL;
        audio_codec = "DTS";
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
      if(language_3cc)
        strcpy(s->language, language_3cc);
      }

    /* Set description */

    switch(audio_attr->code_extension)
      {
      case 0:
        s->info = bgav_sprintf("Unspecified (%s, %dch)",
                               audio_codec, audio_attr->channels+1);
        break;
      case 1:
        s->info = bgav_sprintf("Audio stream (%s, %dch)",
                               audio_codec, audio_attr->channels+1);
        break;
      case 2:
        s->info = bgav_sprintf("Audio for visually impaired (%s, %dch)",
                               audio_codec, audio_attr->channels+1);
        break;
      case 3:
        s->info = bgav_sprintf("Director's comments 1 (%s, %dch)",
                               audio_codec, audio_attr->channels+1);
        break;
      case 4:
        s->info = bgav_sprintf("Director's comments 2 (%s, %dch)",
                               audio_codec, audio_attr->channels+1);
        break;
      }
    }

  /* Subtitle streams */
  for(i = 0; i < dvd->vts_ifo->vtsi_mat->nr_of_vts_subp_streams; i++)
    {
    if(!(pgc->subp_control[i] & 0x80000000))
      continue;

    s = bgav_track_add_subtitle_stream(new_track, ctx->opt, 0, (char*)0);
    s->fourcc = BGAV_MK_FOURCC('D', 'V', 'D', 'S');

    /*  there are several streams for one spu */
    if(dvd->vts_ifo->vtsi_mat->vts_video_attr.display_aspect_ratio)
      {
      /* 16:9 */
      switch(dvd->vts_ifo->vtsi_mat->vts_video_attr.permitted_df)
        {
        case 1: /* letterbox */
          stream_position = pgc->subp_control[i] & 0xff;
          break;
        case 2: /* pan&scan */
          stream_position = ( pgc->subp_control[i] >> 8 ) & 0xff;
          break;
        default: /* widescreen */
          stream_position = ( pgc->subp_control[i] >> 16 ) & 0xff;
          break;
        }
      }
    else
      {
      /* 4:3 */
      stream_position = ( pgc->subp_control[i] >> 24 ) & 0x7F;
      }
    s->stream_id = 0xbd20 + stream_position;
    s->timescale = 90000;

    subp_attr = &dvd->vts_ifo->vtsi_mat->vts_subp_attr[i];
    
    if(subp_attr->type == 1)
      {
      language_2cc[0] = subp_attr->lang_code >> 8;
      language_2cc[1] = subp_attr->lang_code & 0xff;
      language_2cc[2] = '\0';
      language_3cc = bgav_lang_from_twocc(language_2cc);
      if(language_3cc)
        strcpy(s->language, language_3cc);
      }

    switch(subp_attr->code_extension)
      {
      case 0:
        //        printf("Not specified ");
        break;
      case 1:
        s->info = bgav_sprintf("Caption");
        //        printf("Caption with normal size character ");
        break;
      case 2:
        s->info = bgav_sprintf("Caption, big");
        // printf("Caption with bigger size character ");
        break;
      case 3:
        s->info = bgav_sprintf("Caption for children");
        //        printf("Caption for children ");
        break;
      case 4:
        //        printf("reserved ");
        break;
      case 5:
        s->info = bgav_sprintf("Closed caption");
        //        printf("Closed Caption with normal size character ");
        break;
      case 6:
        s->info = bgav_sprintf("Closed caption, big");
        //        printf("Closed Caption with bigger size character ");
        break;
      case 7:
        s->info = bgav_sprintf("Closed caption for children");
        //        printf("Closed Caption for children ");
        break;
      case 8:
        //        printf("reserved ");
        break;
      case 9:
        s->info = bgav_sprintf("Forced caption");
        //        printf("Forced Caption");
        break;
      case 10:
        //        printf("reserved ");
        break;
      case 11:
        //        printf("reserved ");
        break;
      case 12:
        //        printf("reserved ");
        break;
      case 13:
        //        printf("Director's comments with normal size character ");
        s->info = bgav_sprintf("Directors comments");
        break;
      case 14:
        //        printf("Director's comments with bigger size character ");
        s->info = bgav_sprintf("Directors comments, big");
        break;
      case 15:
        s->info = bgav_sprintf("Directors comments for children");
        //        printf("Director's comments for children ");
        break;
      default:
        //        printf("(please send a bug report) ");
        break;
      }

    s->data.subtitle.video_stream = new_track->video_streams;
    }
  return 1;
  }

#if 0
static void dump_vmg_ifo(ifo_handle_t * vmg_ifo)
  {
  bgav_dprintf( "VMG IFO:\n");
  bgav_dprintf( "  Title sets: %d\n",
          vmg_ifo->vmgi_mat->vmg_nr_of_title_sets);
  }
#endif

static int open_dvd(bgav_input_context_t * ctx, const char * url, char ** r)
  {
  int i, j;
  dvd_t * priv;
  tt_srpt_t *ttsrpt;
  char volid[32];
  unsigned char volsetid[128];
  driver_return_code_t err;
  int is_image = 0;
  
  const char * pos;

  pos = strrchr(url, '/');
  if(pos && !strcasecmp(pos, "/video_ts"))
    is_image = 1;
  
  priv = calloc(1, sizeof(*priv));
  
  ctx->priv = priv;

  /* Close the tray, hope it will be harmless if it's already
     closed */

  if(!is_image)
    {
    if((err = cdio_close_tray(url, NULL)))
#if LIBCDIO_VERSION_NUM >= 77
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "cdio_close_tray failed: %s",
              cdio_driver_errmsg(err));
#else
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "cdio_close_tray failed");
#endif  
    }
  
  /* Try to open dvd */
  priv->dvd_reader = DVDOpen(url);
  if(!priv->dvd_reader)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "DVDOpen failed");
    return 0;
    }

  /* Get disc name */
  if(!DVDUDFVolumeInfo(priv->dvd_reader, volid, 32,
                      volsetid, 128))
    {
    ctx->disc_name = bgav_strdup(volid);
    }
    
  /* Open Video Manager */

  priv->vmg_ifo = ifoOpen(priv->dvd_reader, 0);
  if(!priv->vmg_ifo)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "ifoOpen failed");
    return 0;
    }
#if 0
  dump_vmg_ifo(priv->vmg_ifo);
#endif
  ttsrpt = priv->vmg_ifo->tt_srpt;

  /* Create track table */
  ctx->tt = bgav_track_table_create(0);

  for(i = 0; i < ttsrpt->nr_of_srpts; i++)
    {
    for(j = 0; j < ttsrpt->title[i].nr_of_angles; j++)
      {
#if 0
      if(ctx->opt->dvd_chapters_as_tracks)
        {
        /* Add individual chapters as tracks */
        for(k = 0; k < ttsrpt->title[i].nr_of_ptts; k++)
          setup_track(ctx, i, k, j);
        }
      else
        {
#endif
        /* Add entire titles as tracks */
        setup_track(ctx, i, 0, j);
#if 0
        }
#endif
      }
    }
  
  /* Create demuxer */
  
  ctx->demuxer = bgav_demuxer_create(ctx->opt, &bgav_demuxer_mpegps, ctx);
  ctx->demuxer->tt = ctx->tt;

  /* We must set this here, because we don't initialize the demuxer here */
  ctx->demuxer->flags |= BGAV_DEMUXER_CAN_SEEK;
  
  ctx->sector_size        = 2048;
  ctx->sector_size_raw    = 2048;
  ctx->sector_header_size = 0;
  ctx->can_pause = 1;
  
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

  /* Check if we are at the end of the pgc */
  if(ret >= pgc->nr_of_cells)
    return -1;
  
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
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Reading NAV packet at sector %d failed", sector);
    return -1;
    }
  
  if(!is_nav_pack(buf))
    return -1;

  //  printf("*** Got nav pack ***\n");
  
  navRead_DSI(&dsi_pack, buf + DSI_START_BYTE);
  navRead_PCI(&pci_pack, buf + PCI_START_BYTE);


  if(d->last_vobu_end_pts != BGAV_TIMESTAMP_UNDEFINED)
    {
    if(d->last_vobu_end_pts != pci_pack.pci_gi.vobu_s_ptm)
      {
      if(d->last_vobu_end_pts != BGAV_TIMESTAMP_UNDEFINED)
        ctx->demuxer->timestamp_offset += d->last_vobu_end_pts - pci_pack.pci_gi.vobu_s_ptm;
      else
        ctx->demuxer->timestamp_offset = -pci_pack.pci_gi.vobu_s_ptm;
      }
    }
  else
    {
    ctx->demuxer->timestamp_offset = -((int64_t)pci_pack.pci_gi.vobu_s_ptm);
    }
  ctx->demuxer->flags |= BGAV_DEMUXER_HAS_TIMESTAMP_OFFSET;
  
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

static int check_next_cell(bgav_input_context_t * ctx)
  {
  dvd_t * d;
  d = (dvd_t*)(ctx->priv);

  if(d->next_cell < 0)
    return 0;
  
  if(d->next_cell >= d->current_track_priv->end_cell)
    {
    return 0;
    }
  return 1;
  }

static int read_sector_dvd(bgav_input_context_t * ctx, uint8_t * data)
  {
  dvd_t * d;
  int l;
  d = (dvd_t*)(ctx->priv);
  switch(d->state)
    {
    case CELL_START:
      if(!check_next_cell(ctx))
        return 0;
      d->cell = d->next_cell;
      d->next_cell = next_cell(d->pgc, d->cell, d->current_track_priv->angle);
      d->npack = d->pgc->cell_playback[d->cell].first_sector;
      d->state = CELL_LOOP;
      /* Fallthrough */
    case CELL_LOOP:
      d->pack = d->npack;
      l = read_nav(ctx, d->pack, &d->npack);
      if(l < 0)
        return -1;
      d->blocks = l;
      d->pack++;
      d->state = BLOCK_LOOP;
      /* Fallthrough */
    case BLOCK_LOOP:
      /* Do some additional EOF checking */
      if((d->pack > d->pgc->cell_playback[d->cell].last_sector) &&
         (d->next_cell < 0))
        {
        return 0;
        }
      l = DVDReadBlocks(d->dvd_file, d->pack, 1, data);
      if(l < 1)
        {
        bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Reading blocks at %d failed", d->pack);
        return 0;
        }
      //      bgav_hexdump(data, 32, 16);

      d->blocks -= l;
      
      if(!d->blocks)
        {
        if(d->pack < d->pgc->cell_playback[d->cell].last_sector)
          d->state = CELL_LOOP;
        else
          d->state = CELL_START;
        }
      else
        d->pack += l;
    }
  
  return 1;
  }

static void    close_dvd(bgav_input_context_t * ctx)
  {
  int i;
  track_priv_t * track_priv;
  dvd_t * dvd;
  dvd = (dvd_t*)(ctx->priv);

  if(dvd->dvd_reader)
    DVDClose(dvd->dvd_reader);
  
  if(dvd->dvd_file)
    DVDCloseFile(dvd->dvd_file);
  
  if(dvd->vmg_ifo)
    ifoClose(dvd->vmg_ifo);
  
  if(dvd->vts_ifo)
    ifoClose(dvd->vts_ifo);

  if(ctx->tt)
    {
    for(i = 0; i < ctx->tt->num_tracks; i++)
      {
      track_priv = (track_priv_t*)(ctx->tt->tracks[i].priv);
      if(track_priv)
        {
        if(track_priv->chapters)
          free(track_priv->chapters);
        }
      free(track_priv);
      }
    }
  
  free(dvd);
  return;
  }

static void select_track_dvd(bgav_input_context_t * ctx, int track)
  {
  dvd_t * dvd;
  int ttn, pgn;
  int pgc_id, i;
  tt_srpt_t *ttsrpt;
  track_priv_t * track_priv;
  vts_ptt_srpt_t *vts_ptt_srpt;
  
  dvd = (dvd_t*)(ctx->priv);
  dvd->last_vobu_end_pts = BGAV_TIMESTAMP_UNDEFINED;
  ctx->demuxer->flags &= ~BGAV_DEMUXER_HAS_TIMESTAMP_OFFSET;
  
  ttsrpt = dvd->vmg_ifo->tt_srpt;
  track_priv = (track_priv_t*)(ctx->tt->cur->priv);

  dvd->current_track_priv = track_priv;
  
  ttn = ttsrpt->title[track_priv->title].vts_ttn;

  /* Open VTS */
  open_vts(ctx->opt, dvd, ttsrpt->title[track_priv->title].title_set_nr, 1);
  
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
  /* Set the subtitle palettes */
  for(i = 0; i < ctx->tt->cur->num_subtitle_streams; i++)
    {
    ctx->tt->cur->subtitle_streams[i].ext_data = (uint8_t*)(dvd->pgc->palette);
    ctx->tt->cur->subtitle_streams[i].ext_size = sizeof(dvd->pgc->palette);
    }
  }

static void seek_time_dvd(bgav_input_context_t * ctx, int64_t t1, int scale)
  {
  uint8_t buf[DVD_VIDEO_LB_LEN];
  dsi_t dsi_pack;
  pci_t pci_pack;
  uint32_t next_vobu_offset;
  gavl_time_t time, diff_time, cell_start_time;
  dvd_t * dvd;
  int64_t time_scaled;
  gavl_time_t t;
  t = gavl_time_unscale(scale, t1);
  
  dvd = (dvd_t*)(ctx->priv);

  time = 0;
  cell_start_time = 0;
  
  /* Get entry cell */
  dvd->cell = dvd->current_track_priv->start_cell;

  while(1)
    {
    /* If we are in an angle block, go to the right angle */
    if(dvd->pgc->cell_playback[dvd->cell].block_type == BLOCK_TYPE_ANGLE_BLOCK ) 
      dvd->cell += dvd->current_track_priv->angle;
    
    diff_time = convert_time(&dvd->pgc->cell_playback[dvd->cell].playback_time);

    if(cell_start_time + diff_time > t)
      break;

    cell_start_time += diff_time;
    
    /* If we are in an angle block right now, seek the last cell */
    if(dvd->pgc->cell_playback[dvd->cell].block_type == BLOCK_TYPE_ANGLE_BLOCK)
      {
      while(dvd->pgc->cell_playback[dvd->cell].block_mode != BLOCK_MODE_LAST_CELL)
        dvd->cell++;
      }
    /* Advance by one */
    dvd->cell++;
    
    }

  /* Get the next cell, so we won't get screwed up later on */
  
  dvd->next_cell = next_cell(dvd->pgc, dvd->cell, dvd->current_track_priv->angle);

  if(dvd->cell >= dvd->current_track_priv->end_cell)
    {
    bgav_track_set_eof_d(ctx->demuxer->tt->cur);
    return;
    }
  /* Now, seek forward using the seek information in the dsi packets */
  dvd->npack = dvd->pgc->cell_playback[dvd->cell].first_sector;

  time = cell_start_time;
  
  while(1)
    {
    if(DVDReadBlocks(dvd->dvd_file, dvd->npack, 1, buf) != 1)
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Reading NAV packet at sector %d failed", dvd->npack);
      return;
      }
    if(!is_nav_pack(buf))
      {
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
    time = cell_start_time + convert_time(&pci_pack.pci_gi.e_eltm);

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

    //    http://dvd.sourceforge.net/dvdinfo/dsi_pkt.html
    //    says, bit 31 indicates a valid pointer
    //
    //    but on the Procupine Tree DVD, valid offsets are given
    //    without bit 31 set :(
    //
    //    if(!(next_vobu_offset & 0x80000000))
    //      break;

    if(next_vobu_offset == 0x3fffffff)
      break;
    
    next_vobu_offset &= 0x3fffffff;
    dvd->npack += next_vobu_offset;
    }
  dvd->state = CELL_LOOP;
  
  time_scaled = gavl_time_scale(90000, time);
  
  ctx->demuxer->timestamp_offset = time_scaled - (int64_t)pci_pack.pci_gi.vobu_s_ptm;
  dvd->last_vobu_end_pts = pci_pack.pci_gi.vobu_s_ptm;
  }

const bgav_input_t bgav_input_dvd =
  {
    .name =          "dvd",
    .open =          open_dvd,
    .read_sector =   read_sector_dvd,
    .seek_time =     seek_time_dvd,
    .close =         close_dvd,
    .select_track =  select_track_dvd,
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
  if(!ret->input->open(ret, device, NULL))
    {
    bgav_log(ret->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Cannot open DVD Device %s", device);
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
  bgav_codecs_init(&b->opt);
  b->input = bgav_input_open_dvd(device, &b->opt);
  if(!b->input)
    return 0;
  if(!bgav_init(b))
    goto fail;
  return 1;
  fail:
  return 0;
  
  }

#if DVDREAD_VERSION >= 905

#if defined(__GNUC__) && defined(__ELF__)

#ifdef HAVE_DVDFINISH
static void __cleanup() __attribute__ ((destructor));
 
static void __cleanup()
  {
  DVDFinish();
  }
#endif

#ifdef HAVE_DVDINIT
static void __init() __attribute__ ((constructor));
 
static void __init()
  {
  DVDInit();
  }
#endif


#endif


#endif


#else /* No DVD support */

int bgav_check_device_dvd(const char * device, char ** name)
  {
  return 0;
  }

bgav_device_info_t * bgav_find_devices_dvd()
  {
  return (bgav_device_info_t*)0;
  
  }

int bgav_open_dvd(bgav_t * b, const char * device)
  {
  return 0;
  }


#endif
