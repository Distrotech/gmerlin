/*****************************************************************
 
  bgav.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <avdec_private.h>

#include <stdlib.h>
#include <stdio.h>


#define DUMP_SIZE 1024

bgav_input_context_t * create_input(bgav_t * b)
  {
  bgav_input_context_t * ret;

  //  fprintf(stderr, "CREATE INPUT %p\n", b->name_change_callback);
  
  ret = bgav_input_create(&(b->opt));
  
  return ret;
  }

static bgav_demuxer_context_t *
create_demuxer(bgav_t * b, bgav_demuxer_t * demuxer)
  {
  bgav_demuxer_context_t * ret;

  //  fprintf(stderr, "CREATE DEMUXER %p\n", b->name_change_callback);

  ret = bgav_demuxer_create(&(b->opt), demuxer, b->input);
  return ret;
  }

int bgav_init(bgav_t * ret)
  {
  uint8_t dump_buffer[DUMP_SIZE];
  int dump_len;
    
  bgav_demuxer_t * demuxer = (bgav_demuxer_t *)0;
  bgav_redirector_t * redirector = (bgav_redirector_t*)0;

  /*
   *  If the input already has it's track table,
   *  we can stop here
   */
  if(ret->input->tt)
    {
    ret->tt = ret->input->tt;
    bgav_track_table_ref(ret->tt);

    ret->demuxer = ret->input->demuxer;

    if(ret->demuxer)
      ret->demuxer->tt = ret->input->tt;
   
    if(ret->tt->num_tracks > 1)
      return 1;
    
    }
  /*
   *  Autodetect the format and fire up the demuxer
   */
  if(!ret->input->demuxer)
    {
    /* First, we try the redirector, because we never need to
       skip bytes for them. */

    redirector = bgav_redirector_probe(ret->input);
    if(redirector)
      {
      ret->redirector = calloc(1, sizeof(*(ret->redirector)));
      ret->redirector->input = ret->input;
      
      if(!redirector->parse(ret->redirector))
        goto fail;
      else
        return 1;
      }

    /* Check for ID3V2 tags here, they can be prepended to
       many different file types */
    
    if(bgav_id3v2_probe(ret->input))
      ret->input->id3v2 = bgav_id3v2_read(ret->input);
    
    demuxer = bgav_demuxer_probe(ret->input);
    
    if(demuxer)
      {
      ret->demuxer = create_demuxer(ret, demuxer);
      if(!bgav_demuxer_start(ret->demuxer, &(ret->redirector)))
        {
        if(ret->demuxer->error_msg)
          ret->error_msg = bgav_strndup(ret->demuxer->error_msg, (char*)0);
        goto fail;
        }
      }
    if(!ret->demuxer)
      goto fail;
    }
  else
    {
    ret->demuxer = ret->input->demuxer;
    }
  
  ret->tt = ret->demuxer->tt;
  bgav_track_table_ref(ret->tt);

  bgav_track_table_remove_unsupported(ret->tt);
  
  bgav_track_table_merge_metadata(ret->tt,
                                  &(ret->input->metadata));
  
  return 1;
    
  fail:

  if(!demuxer && !redirector)
    {
    dump_len =  bgav_input_get_data(ret->input, dump_buffer, DUMP_SIZE);

    fprintf(stderr, "Cannot detect stream type, first %d bytes of stream follow\n",
            dump_len);
    bgav_hexdump(dump_buffer, dump_len, 16);
    }
  
  if(ret->demuxer)
    {
    bgav_demuxer_destroy(ret->demuxer);
    ret->demuxer = (bgav_demuxer_context_t*)0;
    }
  if(ret->redirector)
    {
    bgav_redirector_destroy(ret->redirector);
    ret->redirector = (bgav_redirector_context_t*)0;
    }
  return 0;
  }

int bgav_num_tracks(bgav_t * b)
  {
  return b->tt->num_tracks;
  }

bgav_t * bgav_create()
  {
  bgav_t * ret;
  ret = calloc(1, sizeof(*ret));

  bgav_options_set_defaults(&ret->opt);
  
  return ret;
  }

int bgav_open(bgav_t * ret, const char * location)
  {
  bgav_codecs_init();
  ret->input = create_input(ret);
  if(!bgav_input_open(ret->input, location))
    {
    ret->error_msg = bgav_strndup(ret->input->error_msg, NULL);
    goto fail;
    }
  if(!bgav_init(ret))
    goto fail;

  ret->location = bgav_strndup(location, NULL);
  
  return 1;
  fail:

  if(ret->input)
    {
    bgav_input_close(ret->input);
    free(ret->input);
    ret->input = (bgav_input_context_t*)0;
    }
  return 0;
  }



int bgav_open_fd(bgav_t * ret, int fd, int64_t total_size, const char * mimetype)
  {
  bgav_codecs_init();
  ret->input = bgav_input_open_fd(fd, total_size, mimetype);
  if(!bgav_init(ret))
    return 0;
  return 1;
  }

void bgav_close(bgav_t * b)
  {
  fprintf(stderr, "bgav_close\n");
  
  if(b->location)
    free(b->location);
  
  if(b->is_running)
    bgav_stop(b);

  if(b->demuxer)
    bgav_demuxer_destroy(b->demuxer);
  if(b->redirector)
    bgav_redirector_destroy(b->redirector);

  if(b->input)
    {
    bgav_input_close(b->input);
    free(b->input);
    }
  if(b->tt)
    bgav_track_table_unref(b->tt);
  if(b->error_msg)
    free(b->error_msg);

  bgav_options_free(&b->opt);
  
  free(b);
  }


void bgav_dump(bgav_t * bgav)
  {
  bgav_track_dump(bgav, bgav->tt->current_track);
  
  }

gavl_time_t bgav_get_duration(bgav_t * bgav, int track)
  {
  return bgav->tt->tracks[track].duration;
  }

const char * bgav_get_track_name(bgav_t * b, int track)
  {
  return b->tt->tracks[track].name;
  }

void bgav_stop(bgav_t * b)
  {
  bgav_track_stop(b->tt->current_track);
  b->is_running = 0;
  }

void bgav_select_track(bgav_t * b, int track)
  {
//  fprintf(stderr, "bgav_select_track %d\n", track);
  
  if(b->is_running)
    bgav_stop(b);
  
  if(b->input->input->select_track)
    {
    /* Input switches track, recreate the demuxer */

    bgav_demuxer_stop(b->demuxer);
    bgav_track_table_select_track(b->tt, track);
    b->input->input->select_track(b->input, track);
    bgav_demuxer_start(b->demuxer, &(b->redirector));
    }
  else if(b->demuxer && b->demuxer->demuxer->select_track)
    {
    /* Demuxer switches track */
    bgav_track_table_select_track(b->tt, track);
    b->demuxer->demuxer->select_track(b->demuxer, track);
    }
  }

int bgav_start(bgav_t * b)
  {
  b->is_running = 1;
  /* Create buffers */
  //  fprintf(stderr, "bgav_start\n");
  bgav_input_buffer(b->input);
  if(!bgav_track_start(b->tt->current_track, b->demuxer))
    {
    if(b->demuxer->error_msg)
       b->error_msg = bgav_strndup(b->demuxer->error_msg, (char*)0);
    return 0;
    }
  return 1;
  }

int bgav_can_seek(bgav_t * b)
  {
  return b->demuxer->can_seek;
  }

const bgav_metadata_t * bgav_get_metadata(bgav_t*b, int track)
  {
  return &(b->tt->tracks[track].metadata);
  }

const char * bgav_get_description(bgav_t * b)
  {
  return b->demuxer->stream_description;
  }

void
bgav_options_set_name_change_callback(bgav_options_t * opt,
                              void (callback)(void*data, const char * name),
                              void * data)
  {
  opt->name_change_callback      = callback;
  opt->name_change_callback_data = data;
  //  fprintf(stderr, "bgav_set_name_change_callback\n");
  }

void
bgav_options_set_metadata_change_callback(bgav_options_t * opt,
                                  void (callback)(void*data, const bgav_metadata_t*),
                                  void * data)
  {
  opt->metadata_change_callback      = callback;
  opt->metadata_change_callback_data = data;
  }

void
bgav_options_set_user_pass_callback(bgav_options_t * opt,
                            int (callback)(void*data, const char * resource, char ** username, char ** password),
                            void * data)
  {
  opt->user_pass_callback      = callback;
  opt->user_pass_callback_data = data;
  }


void
bgav_options_set_track_change_callback(bgav_options_t * opt,
                               void (callback)(void*data, int track),
                               void * data)
  {
  opt->track_change_callback      = callback;
  opt->track_change_callback_data = data;
  }


void
bgav_options_set_buffer_callback(bgav_options_t * opt,
                         void (callback)(void*data, float percentage),
                         void * data)
  {
  opt->buffer_callback      = callback;
  opt->buffer_callback_data = data;
  }

const char * bgav_get_error(bgav_t * b)
  {
  return b->error_msg;
  }

bgav_options_t * bgav_get_options(bgav_t * b)
  {
  return &(b->opt);
  }
