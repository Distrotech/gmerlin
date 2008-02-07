/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>
#include <md5.h>

static void dump_index(bgav_file_index_t * idx)
  {
  int i;
  for(i = 0; i < idx->num_entries; i++)
    {
    bgav_dprintf("      K: %d, P: %"PRId64", T: %"PRId64"\n",
                 idx->entries[i].keyframe,
                 idx->entries[i].position,
                 idx->entries[i].time);
    
    }
  }

static void dump_file_index(bgav_t * b)
  {
  int i, j;
  bgav_dprintf("Generated index table(s)\n");
  for(i = 0; i < b->tt->num_tracks; i++)
    {
    bgav_dprintf(" Track %d\n", i+1);
    
    for(j = 0; j < b->tt->tracks[i].num_audio_streams; j++)
      {
      bgav_dprintf("   Audio stream %d [%08x]\n", j+1,
                   b->tt->tracks[i].audio_streams[j].stream_id);
      dump_index(b->tt->tracks[i].audio_streams[j].file_index);
      }
    for(j = 0; j < b->tt->tracks[i].num_video_streams; j++)
      {
      bgav_dprintf("   Video stream %d [%08x]\n", j+1,
                   b->tt->tracks[i].video_streams[j].stream_id);
      dump_index(b->tt->tracks[i].video_streams[j].file_index);
      }
    for(j = 0; j < b->tt->tracks[i].num_subtitle_streams; j++)
      {
      bgav_dprintf("   Subtitle stream %d [%08x]\n", j+1,
                   b->tt->tracks[i].subtitle_streams[j].stream_id);
      dump_index(b->tt->tracks[i].subtitle_streams[j].file_index);
      }
    }
  }

bgav_file_index_t * bgav_file_index_create()
  {
  bgav_file_index_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bgav_file_index_destroy(bgav_file_index_t * idx)
  {
  if(idx->entries)
    free(idx->entries);
  free(idx);
  }

void
bgav_file_index_append_packet(bgav_file_index_t * idx,
                              int64_t position,
                              int64_t time,
                              int keyframe)
  {
  if(idx->num_entries >= idx->entries_alloc)
    {
    idx->entries_alloc += 512;
    idx->entries = realloc(idx->entries,
                           idx->entries_alloc * sizeof(*idx->entries));
    }
  idx->entries[idx->num_entries].position = position;
  idx->entries[idx->num_entries].time     = time;
  idx->entries[idx->num_entries].keyframe = keyframe;
  idx->num_entries++;
  }

/* File I/O.
 *
 * Format:
 *
 * All multibyte numbers are big endian
 * (network byte order)
 *
 * 1. Signature "BGAVINDEX\n"
 * 2. Filename terminated with \n
 * 3. File time (st_mtime returned by stat(2)) (64
 * 4. Number of tracks (32)
 * 5. Tracks consising of
 *    5.1 Number of streams (32)
 *    5.2 Stream entries consisting of
 *    5.2.2 Stream ID (32)
 *    5.2.3 Number of entries (32)
 *    5.2.4 Duration (64)
 *    5.2.5 Index entries consiting of
 *          5.2.5.1 1 if frame is a keyframe, 0 else (8)
 *          5.2.5.2 position (64)
 *          5.2.5.3 time (64)
 */

#define SIG "BGAVINDEX"

int bgav_file_index_read_header(const char * filename,
                                bgav_input_context_t * input,
                                int * num_tracks)
  {
  int ret = 0;
  uint64_t file_time;
  char * line = (char *)0;
  int line_alloc = 0;
  uint32_t ntracks;
  struct stat stat_buf;
  
  if(!bgav_input_read_line(input, &line, &line_alloc, 0, NULL))
    goto fail;
  if(strcmp(line, SIG))
    goto fail;
  if(!bgav_input_read_line(input, &line, &line_alloc, 0, NULL))
    goto fail;
  if(strcmp(line, filename))
    goto fail;
  if(!bgav_input_read_64_be(input, &file_time))
    goto fail;
  fprintf(stderr, "File time read: %"PRId64"\n", file_time);
  
  /* Don't do this check if we have uuid's as names */
  
  if(filename[0] == '/')
    {
    if(stat(filename, &stat_buf))
      goto fail;
    //    fprintf(stderr, "File time stat: %d\n", stat_buf.st_mtime);
    
    if(file_time != stat_buf.st_mtime)
      goto fail;
    }
  if(!bgav_input_read_32_be(input, &ntracks))
    goto fail;
  
  *num_tracks = ntracks;
  ret = 1;
  fail:
  if(line)
    free(line);
  return ret;
  }

static void write_64(FILE * out, uint64_t i)
  {
  uint8_t buf[8];
  BGAV_64BE_2_PTR(i, buf);
  fwrite(buf, 8, 1, out);
  }

static void write_32(FILE * out, uint32_t i)
  {
  uint8_t buf[4];
  BGAV_32BE_2_PTR(i, buf);
  fwrite(buf, 4, 1, out);
  }

static void write_8(FILE * out, uint8_t i)
  {
  fwrite(&i, 1, 1, out);
  }

void bgav_file_index_write_header(const char * filename,
                                  FILE * output,
                                  int num_tracks)
  {
  uint64_t file_time = 0;
  struct stat stat_buf;

  fprintf(output, "%s\n", SIG);
  fprintf(output, "%s\n", filename);

  if(filename[0] == '/')
    {
    if(stat(filename, &stat_buf))
      return;
    file_time = stat_buf.st_mtime;
    }
  fprintf(stderr, "File time write: %"PRId64"\n", file_time);
  write_64(output, file_time);
  write_32(output, num_tracks);
  }

static bgav_file_index_t *
file_index_read_stream(bgav_input_context_t * input, bgav_stream_t * s)
  {
  int i;
  uint8_t tmp_8;
  bgav_file_index_t * ret = calloc(1, sizeof(*ret));
  if(!bgav_input_read_32_be(input, &ret->num_entries))
    return NULL;
  if(!bgav_input_read_64_be(input, (uint64_t*)&s->duration))
    return NULL;
  
  ret->entries = calloc(ret->num_entries, sizeof(*ret->entries));

  for(i = 0; i < ret->num_entries; i++)
    {
    if(!bgav_input_read_8(input, &tmp_8) ||
       !bgav_input_read_64_be(input, &ret->entries[i].position) ||
       !bgav_input_read_64_be(input, &ret->entries[i].time))
      return NULL;
    ret->entries[i].keyframe = tmp_8;
    }
  return ret;
  }

static void
file_index_write_stream(FILE * output,
                        bgav_file_index_t * idx, bgav_stream_t * s)
  {
  int i;
  write_32(output, idx->num_entries);
  write_64(output, s->duration);

  for(i = 0; i < idx->num_entries; i++)
    {
    write_8(output, idx->entries[i].keyframe);
    write_64(output, idx->entries[i].position);
    write_64(output, idx->entries[i].time);
    }
  }

static void set_has_file_index(bgav_t * b)
  {
  int i;
  for(i = 0; i < b->tt->num_tracks; i++)
    b->tt->tracks[i].has_file_index = 1;
  }

void bgav_read_file_index(bgav_t * b)
  {
  int i, j;
  bgav_input_context_t * input = (bgav_input_context_t*)0;
  int num_tracks;
  uint32_t num_streams;
  int stream_id;
  char * filename;
  bgav_stream_t * s;
  /* Check if the input provided an index filename */
  if(!b->input->index_file || !b->input->filename)
    return;

  filename =
    bgav_search_file_read(&b->opt,
                          "indices", b->input->index_file);
  if(!filename)
    goto fail;
  
  input = bgav_input_create(&b->opt);
  if(!bgav_input_open(input, filename))
    goto fail;

  if(!bgav_file_index_read_header(b->input->filename,
                                  input, &num_tracks))
    goto fail;

  if(num_tracks != b->tt->num_tracks)
    goto fail;

  for(i = 0; i < num_tracks; i++)
    {
    if(!bgav_input_read_32_be(input, &num_streams))
      goto fail;
    
    for(j = 0; j < num_streams; j++)
      {
      if(!bgav_input_read_32_be(input, (uint32_t*)&stream_id))
        goto fail;
      s = bgav_track_find_stream_all(&b->tt->tracks[i], stream_id);
      if(!s)
        goto fail;
      s->file_index = file_index_read_stream(input, s);

      if(!s->file_index)
        {
        goto fail;
        }
      
      }
        
    }
  bgav_input_destroy(input);
  fprintf(stderr, "Read file index:\n");
  dump_file_index(b);
  set_has_file_index(b);
  free(filename);
  return;
  fail:

  if(input)
    bgav_input_destroy(input);
  
  if(b->opt.build_index)
    bgav_build_file_index(b);
  if(filename)
    free(filename);
  return;
  }

void bgav_write_file_index(bgav_t * b)
  {
  int i, j;
  FILE * output;
  char * filename;
  bgav_stream_t * s;
  /* Check if the input provided an index filename */
  if(!b->input->index_file || !b->input->filename)
    return;
  
  filename = 
    bgav_search_file_write(&b->opt,
                           "indices", b->input->index_file);

  output = fopen(filename, "w");
  
  bgav_file_index_write_header(b->input->filename,
                               output,
                               b->tt->num_tracks);
  for(i = 0; i < b->tt->num_tracks; i++)
    {
    write_32(output,
             b->tt->tracks[i].num_audio_streams +
             b->tt->tracks[i].num_video_streams +
             b->tt->tracks[i].num_subtitle_streams);

    for(j = 0; j < b->tt->tracks[i].num_audio_streams; j++)
      {
      s = &b->tt->tracks[i].audio_streams[j];
      write_32(output, s->stream_id);
      file_index_write_stream(output, s->file_index, s);
      }
    for(j = 0; j < b->tt->tracks[i].num_video_streams; j++)
      {
      s = &b->tt->tracks[i].video_streams[j];
      write_32(output, s->stream_id);
      file_index_write_stream(output, s->file_index, s);
      }
    for(j = 0; j < b->tt->tracks[i].num_subtitle_streams; j++)
      {
      s = &b->tt->tracks[i].subtitle_streams[j];
      write_32(output, s->stream_id);
      file_index_write_stream(output, s->file_index, s);
      }
    }
  fclose(output);
  }

/*
 *  Top level packets contain complete frames of one elemtary stream
 */

static void flush_stream_simple(bgav_stream_t * s, int64_t position)
  {
  bgav_packet_t * p;
  while(bgav_demuxer_peek_packet_read(s->demuxer, s, 0))
    {
    p = bgav_demuxer_get_packet_read(s->demuxer, s);
    bgav_file_index_append_packet(s->file_index,
                                  position, p->pts, p->keyframe);
    bgav_demuxer_done_packet_read(s->demuxer, p);
    s->duration = p->pts + p->duration;
    }
  }

static int build_file_index_simple(bgav_t * b)
  {
  int j;
  int64_t position;
  int64_t old_position;
  old_position = b->input->position;
  
  while(1)
    {
    position = b->input->position;
    if(!bgav_demuxer_next_packet(b->demuxer))
      return 1;
    
    for(j = 0; j < b->tt->cur->num_audio_streams; j++)
      flush_stream_simple(&b->tt->cur->audio_streams[j], position);
    for(j = 0; j < b->tt->cur->num_video_streams; j++)
      flush_stream_simple(&b->tt->cur->video_streams[j], position);
    for(j = 0; j < b->tt->cur->num_subtitle_streams; j++)
      flush_stream_simple(&b->tt->cur->subtitle_streams[j], position);
    }

  bgav_input_seek(b->input, old_position, SEEK_SET);
  
  return 1;
  }

/*
 *  Top level packets contain frames of one elemtary stream
 *  framed at codec level
 */

static int flush_stream_mpeg_audio(bgav_stream_t * s)
  {
  if(!s->data.audio.decoder->decoder->parse)
    return 0;
  s->data.audio.decoder->decoder->parse(s);
  return 1;
  }

static int flush_stream_mpeg_video(bgav_stream_t * s)
  {
  if(!s->data.video.decoder->decoder->parse)
    return 0;
  s->data.video.decoder->decoder->parse(s);
  return 1;
  }

static void flush_stream_mpeg_subtitle(bgav_stream_t * s)
  {
  
  }

static int build_file_index_mpeg(bgav_t * b)
  {
  int j;
  int64_t old_position;
  old_position = b->input->position;
  
  while(1)
    {
    /* Process one packet */
    if(!bgav_demuxer_next_packet(b->demuxer))
      return 1;

    for(j = 0; j < b->tt->cur->num_audio_streams; j++)
      flush_stream_mpeg_audio(&b->tt->cur->audio_streams[j]);
    for(j = 0; j < b->tt->cur->num_video_streams; j++)
      flush_stream_mpeg_video(&b->tt->cur->video_streams[j]);
    for(j = 0; j < b->tt->cur->num_subtitle_streams; j++)
      flush_stream_mpeg_subtitle(&b->tt->cur->subtitle_streams[j]);
    if(b->opt.index_callback)
      b->opt.index_callback(b->opt.index_callback_data,
                            (float)b->input->position / 
                            (float)b->input->total_bytes);
    }
  
  bgav_input_seek(b->input, old_position, SEEK_SET);
  return 1;
  }

int bgav_build_file_index(bgav_t * b)
  {
  int i, j;
  int ret = 0;

  if(b->demuxer->index_mode == INDEX_MODE_PCM)
    return 1;
  
  for(i = 0; i < b->tt->num_tracks; i++)
    {
    bgav_select_track(b, i);
    b->demuxer->flags |= BGAV_DEMUXER_BUILD_INDEX;
    for(j = 0; j < b->tt->cur->num_audio_streams; j++)
      {
      b->tt->cur->audio_streams[j].file_index = bgav_file_index_create();
      bgav_set_audio_stream(b, j, BGAV_STREAM_PARSE);
      }
    for(j = 0; j < b->tt->cur->num_video_streams; j++)
      {
      b->tt->cur->video_streams[j].file_index = bgav_file_index_create();
      bgav_set_video_stream(b, j, BGAV_STREAM_PARSE);
      }
    for(j = 0; j < b->tt->cur->num_subtitle_streams; j++)
      {
      b->tt->cur->subtitle_streams[j].file_index = bgav_file_index_create();
      bgav_set_subtitle_stream(b, j, BGAV_STREAM_PARSE);
      }

    bgav_start(b);
    
    switch(b->demuxer->index_mode)
      {
      case INDEX_MODE_SIMPLE:
        build_file_index_simple(b);
        ret = 1;
        break;
      case INDEX_MODE_MPEG:
        build_file_index_mpeg(b);
        ret = 1;
        break;
      }
    b->demuxer->flags &= ~BGAV_DEMUXER_BUILD_INDEX;
    }
  
  if(!ret)
    fprintf(stderr, "No idea how to make an index\n");
  else
    {
    fprintf(stderr, "Built index\n");
    dump_file_index(b);    
    bgav_write_file_index(b);
    set_has_file_index(b);
    }
  return ret;
  }

int bgav_demuxer_next_packet_fileindex(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  
  if(s->file_index->entries[s->index_position].position != ctx->input->position)
    bgav_input_seek(ctx->input,
                    s->file_index->entries[s->index_position].position,
                    SEEK_SET);

  if(!ctx->demuxer->next_packet(ctx))
    return 0;

  return 1;
  }


